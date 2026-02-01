#include "gBufferPass.h"

#include <cassert>

bool _formatsInitialized = false;
GBuffer::Formats _gBufferFormats;

void GBuffer::resize(vma::Allocator &allocator, vk::Device device, vk::Extent2D extent, Pass &pass) {
	_framebuffer.reset();

	_albedoView.reset();
	_normalView.reset();
	_materialPropertiesView.reset();
	_worldPosView.reset();
	_depthView.reset();

	_albedoBuffer.reset();
	_normalBuffer.reset();
	_materialPropertiesBuffer.reset();
	_worldPosBuffer.reset();
	_depthBuffer.reset();

	const Formats &formats = Formats::get();

	vk::ImageUsageFlags storageUsage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

	_albedoBuffer = allocator.createImage2D(
		extent, formats.albedo, storageUsage
	);
	_normalBuffer = allocator.createImage2D(
		extent, formats.normal, storageUsage
	);
	_materialPropertiesBuffer = allocator.createImage2D(
		extent, formats.materialProperties, storageUsage
	);
	_worldPosBuffer = allocator.createImage2D(
		extent, formats.worldPosition, storageUsage
	);
	_depthBuffer = allocator.createImage2D(
		extent, formats.depth, storageUsage
	);

	_albedoView = createImageView2D(
		device, _albedoBuffer.get(), formats.albedo, vk::ImageAspectFlagBits::eColor
	);
	_normalView = createImageView2D(
		device, _normalBuffer.get(), formats.normal, vk::ImageAspectFlagBits::eColor
	);
	_materialPropertiesView = createImageView2D(
		device, _materialPropertiesBuffer.get(), formats.materialProperties, vk::ImageAspectFlagBits::eColor
	);
	_worldPosView = createImageView2D(
		device, _worldPosBuffer.get(), formats.worldPosition, vk::ImageAspectFlagBits::eColor
	);
	_depthView = createImageView2D(
		device, _depthBuffer.get(), formats.depth, formats.depthAspect
	);

	if (pass.getPass()) {
		std::array<vk::ImageView, 5> attachments{
			_albedoView.get(), _normalView.get(), _materialPropertiesView.get(), _worldPosView.get(), _depthView.get()
		};
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo
			.setRenderPass(pass.getPass())
			.setAttachments(attachments)
			.setWidth(extent.width)
			.setHeight(extent.height)
			.setLayers(1);
		_framebuffer = device.createFramebufferUnique(framebufferInfo);
	}
}

void GBuffer::Formats::initialize(vk::PhysicalDevice physicalDevice) {
	assert(!_formatsInitialized);
	vk::FormatFeatureFlags storageFeatures =
		vk::FormatFeatureFlagBits::eStorageImage | vk::FormatFeatureFlagBits::eSampledImage;
	_gBufferFormats.albedo = findSupportedFormat(
		{ vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat },
		physicalDevice, vk::ImageTiling::eOptimal, storageFeatures
	);
	_gBufferFormats.normal = findSupportedFormat(
		{
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Sfloat
		},
		physicalDevice, vk::ImageTiling::eOptimal, storageFeatures
	);
	_gBufferFormats.depth = findSupportedFormat(
		{ vk::Format::eR32Sfloat },
		physicalDevice, vk::ImageTiling::eOptimal, storageFeatures
	);
	_gBufferFormats.materialProperties = findSupportedFormat(
		{ vk::Format::eR16G16Sfloat, vk::Format::eR32G32Sfloat },
		physicalDevice, vk::ImageTiling::eOptimal, storageFeatures
	);
	_gBufferFormats.worldPosition = findSupportedFormat(
		{ vk::Format::eR32G32B32A32Sfloat },
		physicalDevice, vk::ImageTiling::eOptimal, storageFeatures
	);
	_gBufferFormats.depthAspect = vk::ImageAspectFlagBits::eColor;
	_formatsInitialized = true;
}

const GBuffer::Formats &GBuffer::Formats::get() {
	assert(_formatsInitialized);
	return _gBufferFormats;
}


void GBufferPass::issueCommands(vk::CommandBuffer commandBuffer, vk::Framebuffer framebuffer) const {
	(void)framebuffer;
	assert(descriptorSets);
	assert(imageDescriptorSet);
	assert(targetGBuffer);

	auto makeBarrier = [](vk::Image image, vk::AccessFlags srcAccess, vk::AccessFlags dstAccess,
	                      vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
		vk::ImageMemoryBarrier barrier;
		barrier
			.setSrcAccessMask(srcAccess)
			.setDstAccessMask(dstAccess)
			.setOldLayout(oldLayout)
			.setNewLayout(newLayout)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
			));
		return barrier;
	};

	std::array<vk::ImageMemoryBarrier, 5> toStorage{
		makeBarrier(targetGBuffer->getAlbedoBuffer(), vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite,
		            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral),
		makeBarrier(targetGBuffer->getNormalBuffer(), vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite,
		            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral),
		makeBarrier(targetGBuffer->getMaterialPropertiesBuffer(), vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite,
		            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral),
		makeBarrier(targetGBuffer->getWorldPositionBuffer(), vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite,
		            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral),
		makeBarrier(targetGBuffer->getDepthBuffer(), vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite,
		            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral)
	};

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlagBits::eComputeShader,
		{}, {}, {}, toStorage
	);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, getPipelines()[0].get());
	commandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eCompute, _pipelineLayout.get(), 0,
		{ descriptorSets->uniformDescriptor.get(), imageDescriptorSet }, {}
	);
	commandBuffer.dispatch(
		ceilDiv<uint32_t>(_bufferExtent.width, 8u),
		ceilDiv<uint32_t>(_bufferExtent.height, 8u),
		1
	);

	std::array<vk::ImageMemoryBarrier, 5> toSample{
		makeBarrier(targetGBuffer->getAlbedoBuffer(), vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
		            vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal),
		makeBarrier(targetGBuffer->getNormalBuffer(), vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
		            vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal),
		makeBarrier(targetGBuffer->getMaterialPropertiesBuffer(), vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
		            vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal),
		makeBarrier(targetGBuffer->getWorldPositionBuffer(), vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
		            vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal),
		makeBarrier(targetGBuffer->getDepthBuffer(), vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
		            vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal)
	};

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eAllCommands,
		{}, {}, {}, toSample
	);
}

vk::UniqueRenderPass GBufferPass::_createPass(vk::Device device) {
	return {};
}

std::vector<Pass::PipelineCreationInfo> GBufferPass::_getPipelineCreationInfo() {
	std::vector<PipelineCreationInfo> result;
	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo
		.setStage(_compute.getStageInfo())
		.setLayout(_pipelineLayout.get());
	result.emplace_back(pipelineInfo);
	return result;
}

void GBufferPass::_initialize(vk::Device dev) {
	_compute = Shader::load(dev, "shaders/gBuffer.comp.spv", "main", vk::ShaderStageFlagBits::eCompute);

	std::array<vk::DescriptorSetLayoutBinding, 1> uniformsDescriptorBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute)
	};
	vk::DescriptorSetLayoutCreateInfo uniformsDescriptorSetInfo;
	uniformsDescriptorSetInfo.setBindings(uniformsDescriptorBindings);
	_uniformsDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(uniformsDescriptorSetInfo);

	std::array<vk::DescriptorSetLayoutBinding, 5> imageBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
	};
	vk::DescriptorSetLayoutCreateInfo imageDescriptorInfo;
	imageDescriptorInfo.setBindings(imageBindings);
	_imagesDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(imageDescriptorInfo);

	std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts{
		_uniformsDescriptorSetLayout.get(),
		_imagesDescriptorSetLayout.get()
	};

	vk::PipelineLayoutCreateInfo pipelineInfo;
	pipelineInfo
		.setSetLayouts(descriptorSetLayouts);
	_pipelineLayout = dev.createPipelineLayoutUnique(pipelineInfo);

	Pass::_initialize(dev);
}
