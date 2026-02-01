#pragma once

#include "pass.h"

class EmissiveSamplePass : public Pass {
public:
	[[nodiscard]] vk::DescriptorSetLayout getDescriptorSetLayout() const {
		return _descriptorLayout.get();
	}

	void issueCommands(vk::CommandBuffer buffer, vk::Framebuffer) const override {
		buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			{}, {}, {}, {}
		);
		buffer.bindPipeline(vk::PipelineBindPoint::eCompute, getPipelines()[0].get());
		buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _layout.get(), 0, { descriptorSet }, {});
		SampleParams params{ sampleCount, seed };
		buffer.pushConstants(_layout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(SampleParams), &params);
		buffer.dispatch(ceilDiv<uint32_t>(sampleCount, 64u), 1, 1);

		vk::MemoryBarrier barrier;
		barrier
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
		buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			{}, barrier, {}, {}
		);
	}

	vk::DescriptorSet descriptorSet;
	uint32_t sampleCount = 0;
	uint32_t seed = 0;
protected:
	struct SampleParams {
		uint32_t sampleCount;
		uint32_t seed;
	};

	Shader _shader;
	vk::UniqueDescriptorSetLayout _descriptorLayout;
	vk::UniquePipelineLayout _layout;

	vk::UniqueRenderPass _createPass(vk::Device) override {
		return {};
	}

	std::vector<PipelineCreationInfo> _getPipelineCreationInfo() override {
		std::vector<PipelineCreationInfo> result;
		vk::ComputePipelineCreateInfo pipelineInfo;
		pipelineInfo
			.setStage(_shader.getStageInfo())
			.setLayout(_layout.get());
		result.emplace_back(pipelineInfo);
		return result;
	}

	void _initialize(vk::Device dev) override {
		_shader = Shader::load(dev, "shaders/emissiveSample.comp.spv", "main", vk::ShaderStageFlagBits::eCompute);

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute)
		};

		vk::DescriptorSetLayoutCreateInfo descriptorInfo;
		descriptorInfo.setBindings(bindings);
		_descriptorLayout = dev.createDescriptorSetLayoutUnique(descriptorInfo);

		std::array<vk::DescriptorSetLayout, 1> descriptorLayouts{ _descriptorLayout.get() };

		std::array<vk::PushConstantRange, 1> ranges{
			vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(SampleParams))
		};

		vk::PipelineLayoutCreateInfo layoutInfo;
		layoutInfo
			.setSetLayouts(descriptorLayouts)
			.setPushConstantRanges(ranges);

		_layout = dev.createPipelineLayoutUnique(layoutInfo);

		Pass::_initialize(dev);
	}
};
