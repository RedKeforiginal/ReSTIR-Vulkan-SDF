#pragma once

#include "../vulkanConfig.h"

class UnbiasedReusePass {
public:
	UnbiasedReusePass() = default;
	UnbiasedReusePass(UnbiasedReusePass&&) = default;
	UnbiasedReusePass& operator=(UnbiasedReusePass&&) = default;

	[[nodiscard]] vk::DescriptorSetLayout getFrameDescriptorSetLayout() const {
		return _frameDescriptorSetLayout.get();
	}

	void issueCommands(vk::CommandBuffer commandBuffer) {
		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			{}, {}, {}, {}
		);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _sdfPipeline.get());
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute, _sdfPipelineLayout.get(), 0,
			{ frameDescriptorSet }, {}
		);
		commandBuffer.dispatch(
			ceilDiv<uint32_t>(bufferExtent.width, UNBIASED_REUSE_GROUP_SIZE_X),
			ceilDiv<uint32_t>(bufferExtent.height, UNBIASED_REUSE_GROUP_SIZE_Y),
			1
		);
	}

	void initializeFrameDescriptorSet(
		vk::Device dev,
		const GBuffer& gbuffer, vk::Buffer uniformBuffer,
		vk::Buffer reservoirBuffer, vk::Buffer resultReservoirBuffer, vk::DeviceSize reservoirBufferSize,
		vk::DescriptorSet set
	) {
		std::array<vk::WriteDescriptorSet, 8> descriptorWrite;

		// GBuffer Data
		std::array<vk::DescriptorImageInfo, 5> imageInfo{
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getWorldPositionView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getAlbedoView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getNormalView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getMaterialPropertiesView(), vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::DescriptorImageInfo(_sampler.get(), gbuffer.getDepthView(), vk::ImageLayout::eShaderReadOnlyOptimal)
		};
		for (std::size_t i = 0; i < imageInfo.size(); ++i) {
			descriptorWrite[i]
				.setDstSet(set)
				.setDstBinding(static_cast<uint32_t>(i))
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setImageInfo(imageInfo[i]);
		}

		std::array<vk::DescriptorBufferInfo, 3> reservoirsBufferInfo{
			vk::DescriptorBufferInfo(reservoirBuffer, 0, reservoirBufferSize),
			vk::DescriptorBufferInfo(resultReservoirBuffer, 0, reservoirBufferSize),
			vk::DescriptorBufferInfo(uniformBuffer, 0, sizeof(shader::RestirUniforms))
		};

		descriptorWrite[5]
			.setDstSet(set)
			.setDstBinding(5)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(reservoirsBufferInfo[0]);
		descriptorWrite[6]
			.setDstSet(set)
			.setDstBinding(6)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(reservoirsBufferInfo[1]);
		descriptorWrite[7]
			.setDstSet(set)
			.setDstBinding(7)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setBufferInfo(reservoirsBufferInfo[2]);

		dev.updateDescriptorSets(descriptorWrite, {});
	}

	inline static UnbiasedReusePass create(vk::Device dev)
	{
		UnbiasedReusePass pass = UnbiasedReusePass();
		pass._initialize(dev);
		return std::move(pass);
	}

	//vk::DescriptorSet descriptorSet;
	vk::DescriptorSet frameDescriptorSet;
	vk::Extent2D bufferExtent;
protected:
	Shader _sdfShader;
	vk::Format _swapchainFormat;
	vk::UniqueSampler _sampler;
	vk::UniquePipelineLayout _sdfPipelineLayout;
	vk::UniqueDescriptorSetLayout _frameDescriptorSetLayout;

	void _initialize(vk::Device dev) {
		_sampler = createSampler(dev, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);

		_sdfShader = Shader::load(dev, "shaders/unbiasedReuseSoftware.comp.spv", "main", vk::ShaderStageFlagBits::eCompute);

		vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eCompute;

		std::array<vk::DescriptorSetLayoutBinding, 8> frameBindings{
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, stageFlags),
			vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eUniformBuffer, 1, stageFlags)
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo;
		layoutInfo.setBindings(frameBindings);
		_frameDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(layoutInfo);


		std::array<vk::DescriptorSetLayout, 1> sdfDescriptorLayouts{ _frameDescriptorSetLayout.get() };

		vk::PipelineLayoutCreateInfo sdfPipelineLayoutInfo;
		sdfPipelineLayoutInfo.setSetLayouts(sdfDescriptorLayouts);
		_sdfPipelineLayout = dev.createPipelineLayoutUnique(sdfPipelineLayoutInfo);

		vk::ComputePipelineCreateInfo sdfPipelineInfo;
		sdfPipelineInfo
			.setLayout(_sdfPipelineLayout.get())
			.setStage(_sdfShader.getStageInfo());
		auto [res, pipeline] = dev.createComputePipelineUnique(nullptr, sdfPipelineInfo);
		vkCheck(res);
		_sdfPipeline = std::move(pipeline);
	}
private:
	vk::UniqueRenderPass _pass;
	vk::UniquePipeline _sdfPipeline;
};
