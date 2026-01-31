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

	_albedoBuffer = allocator.createImage2D(
		extent, formats.albedo,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
	);
	_normalBuffer = allocator.createImage2D(
		extent, formats.normal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
	);
	_materialPropertiesBuffer = allocator.createImage2D(
		extent, formats.materialProperties,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
	);
	_worldPosBuffer = allocator.createImage2D(
		extent, formats.worldPosition,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
	);
	_depthBuffer = allocator.createImage2D(
		extent, formats.depth,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
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

void GBuffer::Formats::initialize(vk::PhysicalDevice physicalDevice) {
	assert(!_formatsInitialized);
	_gBufferFormats.albedo = findSupportedFormat(
		{ vk::Format::eR8G8B8A8Srgb },
		physicalDevice, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eColorAttachment
	);
	_gBufferFormats.normal = findSupportedFormat(
		{
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32Sfloat
		},
		physicalDevice, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eColorAttachment
	);
	_gBufferFormats.depth = findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		physicalDevice, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	_gBufferFormats.materialProperties = findSupportedFormat(
		{ vk::Format::eR16G16Unorm },
		physicalDevice, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eColorAttachment
	);
	_gBufferFormats.worldPosition = findSupportedFormat(
		{ vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32A32Sfloat },
		physicalDevice, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eColorAttachment
	);
	_gBufferFormats.depthAspect = vk::ImageAspectFlagBits::eDepth;
	if (_gBufferFormats.depth != vk::Format::eD32Sfloat) {
		_gBufferFormats.depthAspect |= vk::ImageAspectFlagBits::eStencil;
	}
	_formatsInitialized = true;
}

const GBuffer::Formats &GBuffer::Formats::get() {
	assert(_formatsInitialized);
	return _gBufferFormats;
}


void GBufferPass::issueCommands(vk::CommandBuffer commandBuffer, vk::Framebuffer framebuffer) const {
	std::array<vk::ClearValue, 5> clearValues{
		vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }),
		vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }),
		vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }),
		vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }),
		vk::ClearDepthStencilValue(1.0f)
	};
	vk::RenderPassBeginInfo passBeginInfo;
	passBeginInfo
		.setRenderPass(getPass())
		.setFramebuffer(framebuffer)
		.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), _bufferExtent))
		.setClearValues(clearValues);
	commandBuffer.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, getPipelines()[0].get());
	commandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, _pipelineLayout.get(), 0,
		{ descriptorSets->uniformDescriptor.get() }, {}
	);
	commandBuffer.draw(4, 1, 0, 0);

	commandBuffer.endRenderPass();
}

vk::UniqueRenderPass GBufferPass::_createPass(vk::Device device) {
	const GBuffer::Formats &formats = GBuffer::Formats::get();

	std::vector<vk::AttachmentDescription> attachments;
	attachments.emplace_back()
		.setFormat(formats.albedo)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	attachments.emplace_back()
		.setFormat(formats.normal)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	attachments.emplace_back()
		.setFormat(formats.materialProperties)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	attachments.emplace_back()
		.setFormat(formats.worldPosition)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	attachments.emplace_back()
		.setFormat(formats.depth)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

	std::array<vk::AttachmentReference, 4> colorAttachmentReferences{
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(1, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(2, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(3, vk::ImageLayout::eColorAttachmentOptimal)
	};

	vk::AttachmentReference depthAttachmentReference;
	depthAttachmentReference
		.setAttachment(4)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	std::vector<vk::SubpassDescription> subpasses;
	subpasses.emplace_back()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachments(colorAttachmentReferences)
		.setPDepthStencilAttachment(&depthAttachmentReference);

	std::vector<vk::SubpassDependency> dependencies;
	dependencies.emplace_back()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo
		.setAttachments(attachments)
		.setSubpasses(subpasses)
		.setDependencies(dependencies);

	return device.createRenderPassUnique(renderPassInfo);
}

std::vector<Pass::PipelineCreationInfo> GBufferPass::_getPipelineCreationInfo() {
	std::vector<PipelineCreationInfo> result;

	GraphicsPipelineCreationInfo info;

	info.vertexInputState
		.setVertexBindingDescriptions(info.vertexInputBindingStorage)
		.setVertexAttributeDescriptions(info.vertexInputAttributeStorage);

	info.inputAssemblyState
		.setTopology(vk::PrimitiveTopology::eTriangleStrip)
		.setPrimitiveRestartEnable(false);

	info.viewportStorage.emplace_back(
		0.0f, 0.0f, static_cast<float>(_bufferExtent.width), static_cast<float>(_bufferExtent.height), 0.0f, 1.0f
	);
	info.scissorStorage.emplace_back(vk::Offset2D(0, 0), _bufferExtent);
	info.viewportState
		.setViewports(info.viewportStorage)
		.setScissors(info.scissorStorage);

	info.rasterizationState = GraphicsPipelineCreationInfo::getDefaultRasterizationState();
	info.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);

	info.depthStencilState = GraphicsPipelineCreationInfo::getDefaultDepthTestState();

	info.multisampleState = GraphicsPipelineCreationInfo::getNoMultisampleState();

	info.attachmentColorBlendStorage.emplace_back(GraphicsPipelineCreationInfo::getNoBlendAttachment());
	info.attachmentColorBlendStorage.emplace_back(GraphicsPipelineCreationInfo::getNoBlendAttachment());
	info.attachmentColorBlendStorage.emplace_back(GraphicsPipelineCreationInfo::getNoBlendAttachment());
	info.attachmentColorBlendStorage.emplace_back(GraphicsPipelineCreationInfo::getNoBlendAttachment());
	info.colorBlendState.setAttachments(info.attachmentColorBlendStorage);

	info.shaderStages.emplace_back(_frag.getStageInfo());
	info.shaderStages.emplace_back(_vert.getStageInfo());

	info.pipelineLayout = _pipelineLayout.get();

	result.emplace_back(std::move(info));

	return result;
}

void GBufferPass::_initialize(vk::Device dev) {
	_vert = Shader::load(dev, "shaders/gBuffer.vert.spv", "main", vk::ShaderStageFlagBits::eVertex);
	_frag = Shader::load(dev, "shaders/gBuffer.frag.spv", "main", vk::ShaderStageFlagBits::eFragment);

	std::array<vk::DescriptorSetLayoutBinding, 1> uniformsDescriptorBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
	};
	vk::DescriptorSetLayoutCreateInfo uniformsDescriptorSetInfo;
	uniformsDescriptorSetInfo.setBindings(uniformsDescriptorBindings);
	_uniformsDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(uniformsDescriptorSetInfo);

	std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{
		_uniformsDescriptorSetLayout.get()
	};

	vk::PipelineLayoutCreateInfo pipelineInfo;
	pipelineInfo
		.setSetLayouts(descriptorSetLayouts);
	_pipelineLayout = dev.createPipelineLayoutUnique(pipelineInfo);

	Pass::_initialize(dev);
}
