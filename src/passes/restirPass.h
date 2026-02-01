#pragma once

#include "../vulkanConfig.h"

#include "pass.h"
class RestirPass : public Pass {
	friend Pass;
public:
	RestirPass() = default;
	RestirPass(RestirPass&&) = default;
	RestirPass& operator=(RestirPass&&) = default;

	[[nodiscard]] vk::DescriptorSetLayout getStaticDescriptorSetLayout() const {
		return _staticDescriptorSetLayout.get();
	}
	[[nodiscard]] vk::DescriptorSetLayout getFrameDescriptorSetLayout() const {
		return _frameDescriptorSetLayout.get();
	}

	void freeDeviceMemory(vk::Device dev)
	{
		for (int i = 0; i < memories.size(); i++)
		{
			dev.freeMemory(memories.at(i));
		}
	}

	void issueCommands(vk::CommandBuffer commandBuffer, vk::Framebuffer) const override {
		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eAllCommands,
			vk::PipelineStageFlagBits::eAllCommands,
			{}, {}, {}, {}
		);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, getPipelines()[0].get());
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute, _sdfPipelineLayout.get(), 0,
			{ staticDescriptorSet, frameDescriptorSet }, {}
		);
		commandBuffer.dispatch(
			ceilDiv<uint32_t>(bufferExtent.width, OMNI_GROUP_SIZE_X),
			ceilDiv<uint32_t>(bufferExtent.height, OMNI_GROUP_SIZE_Y),
			1
		);
	}


	void initializeStaticDescriptorSetFor(
		vk::Buffer emissiveSampleBuffer, vk::DeviceSize emissiveSampleBufferSize,
		vk::Buffer uniformBuffer, vk::Device device, vk::DescriptorSet set
	) {
		std::array<vk::WriteDescriptorSet, 2> writes;

		vk::DescriptorBufferInfo emissiveSampleInfo(emissiveSampleBuffer, 0, emissiveSampleBufferSize);
		vk::DescriptorBufferInfo uniformBufferInfo(uniformBuffer, 0, sizeof(shader::RestirUniforms));

		writes[0]
			.setDstSet(set)
			.setDstBinding(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(emissiveSampleInfo);

		writes[1]
			.setDstSet(set)
			.setDstBinding(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setBufferInfo(uniformBufferInfo);

		device.updateDescriptorSets(writes, {});
	}

	void initializeFrameDescriptorSetFor(
		const GBuffer& gbuffer, const GBuffer& prevFrameGBuffer,
		vk::Buffer reservoirBuffer, vk::Buffer prevFrameReservoirBuffer, vk::DeviceSize reservoirBufferSize,
		vk::Device device, vk::DescriptorSet set
	) {
		std::vector<vk::WriteDescriptorSet> writes;

		std::vector<vk::DescriptorImageInfo> imageInfo{
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getWorldPositionView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getAlbedoView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getNormalView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getMaterialPropertiesView(), vk::ImageLayout::eShaderReadOnlyOptimal),

			vk::DescriptorImageInfo(_sampler.get(), prevFrameGBuffer.getWorldPositionView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), prevFrameGBuffer.getAlbedoView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), prevFrameGBuffer.getNormalView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), prevFrameGBuffer.getDepthView(), vk::ImageLayout::eShaderReadOnlyOptimal)
		};

		vk::DescriptorBufferInfo prevReservoirInfo(prevFrameReservoirBuffer, 0, reservoirBufferSize);
		vk::DescriptorBufferInfo reservoirInfo(reservoirBuffer, 0, reservoirBufferSize);


		for (vk::DescriptorImageInfo &info : imageInfo) {
			writes.emplace_back()
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setImageInfo(info);
		}

		writes.emplace_back()
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(reservoirInfo);
		writes.emplace_back()
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(prevReservoirInfo);


		for (std::size_t i = 0; i < writes.size(); ++i) {
			writes[i]
				.setDstSet(set)
				.setDstBinding(static_cast<uint32_t>(i));
		}
		device.updateDescriptorSets(writes, {});
	}

	vk::DescriptorSet staticDescriptorSet;
	vk::DescriptorSet frameDescriptorSet;

	vk::Extent2D bufferExtent;
protected:
	Shader _sdfShader;

	vk::UniqueSampler _sampler;
	vk::UniquePipelineLayout _sdfPipelineLayout;
	vk::UniqueDescriptorSetLayout _staticDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout _frameDescriptorSetLayout;

	[[nodiscard]] vk::UniqueRenderPass _createPass(vk::Device) override {
		return {};
	}

	[[nodiscard]] std::vector<PipelineCreationInfo> _getPipelineCreationInfo() override {
		return {};
	}

	[[nodiscard]] std::vector<vk::UniquePipeline> _createPipelines(vk::Device dev) override {
		std::vector<vk::UniquePipeline> pipelines;

		{ // compute pipeline
			vk::ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo
				.setStage(_sdfShader.getStageInfo())
				.setLayout(_sdfPipelineLayout.get());
			auto [res, pipeline] = dev.createComputePipelineUnique(nullptr, pipelineInfo);
			vkCheck(res);
			pipelines.emplace_back(std::move(pipeline));
		}

		return pipelines;
	}

	void _initialize(vk::Device dev) override {
		constexpr vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eCompute;

		_sampler = createSampler(dev, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);

		_sdfShader = Shader::load(dev, "shaders/restirOmniSoftware.comp.spv", "main", vk::ShaderStageFlagBits::eCompute);


		std::array<vk::DescriptorSetLayoutBinding, 2> staticBindings{
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, stageFlags)
		};

		vk::DescriptorSetLayoutCreateInfo staticLayoutInfo;
		staticLayoutInfo.setBindings(staticBindings);
		_staticDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(staticLayoutInfo);


		std::array<vk::DescriptorSetLayoutBinding, 10> frameBindings{
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, stageFlags)
		};

		vk::DescriptorSetLayoutCreateInfo frameDescriptorInfo;
		frameDescriptorInfo.setBindings(frameBindings);
		_frameDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(frameDescriptorInfo);


		std::array<vk::DescriptorSetLayout, 2> sdfDescriptorLayouts{
			_staticDescriptorSetLayout.get(), _frameDescriptorSetLayout.get()
		};

		vk::PipelineLayoutCreateInfo sdfPipelineLayoutInfo;
		sdfPipelineLayoutInfo.setSetLayouts(sdfDescriptorLayouts);
		_sdfPipelineLayout = dev.createPipelineLayoutUnique(sdfPipelineLayoutInfo);


		Pass::_initialize(dev);
	}
private:
	std::vector<vk::DeviceMemory> memories;
};
