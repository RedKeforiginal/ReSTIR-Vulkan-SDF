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
	[[nodiscard]] vk::DescriptorSetLayout getSoftwareRaytraceDescriptorSetLayout() const {
		return _swRaytraceDescriptorLayout.get();
	}

	void issueCommands(vk::CommandBuffer commandBuffer) {
		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			{}, {}, {}, {}
		);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _softwarePipeline.get());
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute, _swPipelineLayout.get(), 0,
			{ frameDescriptorSet, raytraceDescriptorSet }, {}
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

	void initializeSoftwareRaytraceDescriptorSet(vk::Device dev, const AabbTreeBuffers &aabbTree, vk::DescriptorSet set) {
		std::array<vk::WriteDescriptorSet, 2> writes;
		vk::DescriptorBufferInfo nodeInfo(aabbTree.nodeBuffer.get(), 0, aabbTree.nodeBufferSize);
		vk::DescriptorBufferInfo triangleInfo(aabbTree.triangleBuffer.get(), 0, aabbTree.triangleBufferSize);

		writes[0]
			.setDstSet(set)
			.setDstBinding(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(nodeInfo);
		writes[1]
			.setDstSet(set)
			.setDstBinding(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(triangleInfo);

		dev.updateDescriptorSets(writes, {});
	}

	inline static UnbiasedReusePass create(vk::Device dev)
	{
		UnbiasedReusePass pass = UnbiasedReusePass();
		pass._initialize(dev);
		return std::move(pass);
	}

	//vk::DescriptorSet descriptorSet;
	vk::DescriptorSet frameDescriptorSet;
	vk::DescriptorSet raytraceDescriptorSet;
	vk::Extent2D bufferExtent;
protected:
	Shader _software;
	vk::Format _swapchainFormat;
	vk::UniqueSampler _sampler;
	vk::UniquePipelineLayout _swPipelineLayout;
	vk::UniqueDescriptorSetLayout _frameDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout _swRaytraceDescriptorLayout;

	void _initialize(vk::Device dev) {
		_sampler = createSampler(dev, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);

		_software = Shader::load(dev, "shaders/unbiasedReuseSoftware.comp.spv", "main", vk::ShaderStageFlagBits::eCompute);

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


		std::array<vk::DescriptorSetLayoutBinding, 2> swRaytraceBindings{
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
		};

		vk::DescriptorSetLayoutCreateInfo swRaytraceLayoutInfo;
		swRaytraceLayoutInfo.setBindings(swRaytraceBindings);
		_swRaytraceDescriptorLayout = dev.createDescriptorSetLayoutUnique(swRaytraceLayoutInfo);


		std::array<vk::DescriptorSetLayout, 2> swDescriptorLayouts{ _frameDescriptorSetLayout.get(), _swRaytraceDescriptorLayout.get() };

		vk::PipelineLayoutCreateInfo swPipelineLayoutInfo;
		swPipelineLayoutInfo.setSetLayouts(swDescriptorLayouts);
		_swPipelineLayout = dev.createPipelineLayoutUnique(swPipelineLayoutInfo);

		vk::ComputePipelineCreateInfo swPipelineInfo;
		swPipelineInfo
			.setLayout(_swPipelineLayout.get())
			.setStage(_software.getStageInfo());
		auto [res, pipeline] = dev.createComputePipelineUnique(nullptr, swPipelineInfo);
		vkCheck(res);
		_softwarePipeline = std::move(pipeline);
	}
private:
	vk::UniqueRenderPass _pass;
	vk::UniquePipeline _softwarePipeline;
};
