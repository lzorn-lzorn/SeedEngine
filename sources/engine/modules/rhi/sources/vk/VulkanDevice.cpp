#include "RHI.h"
#include "VulkanRHI.h"
#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include "VulkanDeviceMemory.h"
#include "VulkanImageView.h"
#include "VulkanPipeline.hpp"
#include <stdexcept>

namespace rhi
{

VulkanDevice::VulkanDevice(
	vk::PhysicalDevice& InRealGPU,
	vk::UniqueDevice& InLogicalDevice,
	uint32_t InGraphicsQueueFamilyIndex)
	: RealGPU(InRealGPU)
	, LogicalDevice(InLogicalDevice)
	, GraphicsQueueFamilyIndex(InGraphicsQueueFamilyIndex)
	, GraphicsQueue(LogicalDevice->getQueue(InGraphicsQueueFamilyIndex, 0))
{
	const auto queue_families = RealGPU.getQueueFamilyProperties();
	if (InGraphicsQueueFamilyIndex >= queue_families.size())
		throw std::out_of_range("Vulkan queue family index is out of range.");
	QueueCapabilities = queue_families[InGraphicsQueueFamilyIndex].queueFlags;

	const auto properties = RealGPU.getProperties();
	Limits.MaxColorAttachments = properties.limits.maxColorAttachments;
	Limits.MaxViewports = properties.limits.maxViewports;
	Limits.MaxFramebufferWidth = properties.limits.maxFramebufferWidth;
	Limits.MaxFramebufferHeight = properties.limits.maxFramebufferHeight;
	Limits.MaxVertexInputBindings = properties.limits.maxVertexInputBindings;
	Limits.MaxVertexInputAttributes = properties.limits.maxVertexInputAttributes;
	Limits.MaxPushConstantSize = properties.limits.maxPushConstantsSize;
	Limits.MaxBoundBindGroups = properties.limits.maxBoundDescriptorSets;

	vk::PhysicalDeviceVulkan13Features vulkan13_features;
	vk::PhysicalDeviceMultiviewFeatures multiview_features;
	vk::PhysicalDeviceSeparateDepthStencilLayoutsFeatures separate_layout_features;
	vulkan13_features.pNext = &multiview_features;
	multiview_features.pNext = &separate_layout_features;
	vk::PhysicalDeviceFeatures2 features;
	features.pNext = &vulkan13_features;
	RealGPU.getFeatures2(&features);
	Features.DynamicRendering = vulkan13_features.dynamicRendering == VK_TRUE;
	Features.Synchronization2 = vulkan13_features.synchronization2 == VK_TRUE;
	Features.Multiview = multiview_features.multiview == VK_TRUE;
	Features.SeparateDepthStencilLayouts = separate_layout_features.separateDepthStencilLayouts == VK_TRUE;
	Features.GeometryShader = features.features.geometryShader == VK_TRUE;
	Features.TessellationShader = features.features.tessellationShader == VK_TRUE;
	Features.FillModeNonSolid = features.features.fillModeNonSolid == VK_TRUE;
	Features.WideLines = features.features.wideLines == VK_TRUE;
	Features.DepthClamp = features.features.depthClamp == VK_TRUE;
	Features.DepthBounds = features.features.depthBounds == VK_TRUE;
	Features.SampleRateShading = features.features.sampleRateShading == VK_TRUE;
	Features.AlphaToOne = features.features.alphaToOne == VK_TRUE;
	Features.IndependentBlend = features.features.independentBlend == VK_TRUE;
}

namespace
{
[[noreturn]] void throwResourceNotImplemented(const char* Resource)
{
	throw std::logic_error(std::string("Vulkan ") + Resource + " creation is not implemented yet.");
}
}

RBuffer* VulkanDevice::createBuffer()
{
	throwResourceNotImplemented("buffer");
}

RImage* VulkanDevice::createImage()
{
	throwResourceNotImplemented("image");
}

std::shared_ptr<RImageView> VulkanDevice::createImageView(
	const RImageView::Descriptor_t& Desc)
{
	return VulkanImageView::create(*this, Desc);
}

RSampler* VulkanDevice::createSampler()
{
	throwResourceNotImplemented("sampler");
}

std::shared_ptr<RShader> VulkanDevice::createShader(const ShaderDescriptor& Desc)
{
	return std::make_shared<VulkanShader>(*this, Desc);
}

std::shared_ptr<RBindGroupLayout> VulkanDevice::createBindGroupLayout(
	const BindGroupLayoutDescriptor& Desc)
{
	return std::make_shared<VulkanBindGroupLayout>(*this, Desc);
}

std::shared_ptr<RPipelineLayout> VulkanDevice::createPipelineLayout(
	const PipelineLayoutDescriptor& Desc)
{
	return std::make_shared<VulkanPipelineLayout>(*this, Desc);
}

std::shared_ptr<RPipelineCache> VulkanDevice::createPipelineCache(
	const PipelineCacheDescriptor& Desc)
{
	return std::make_shared<VulkanPipelineCache>(*this, Desc);
}

std::shared_ptr<RPipeline> VulkanDevice::createGraphicsPipeline(
	const GraphicsPipelineDescriptor& Desc)
{
	return createVulkanGraphicsPipeline(*this, Desc);
}

std::shared_ptr<RPipeline> VulkanDevice::createComputePipeline(
	const ComputePipelineDescriptor& Desc)
{
	return createVulkanComputePipeline(*this, Desc);
}

std::shared_ptr<RCommandList> VulkanDevice::createCommandList(
	const CommandListDescriptor& Desc)
{
	if (!Features.DynamicRendering || !Features.Synchronization2)
	{
		throw std::logic_error(
			"Vulkan command lists require dynamic rendering and synchronization2.");
	}
	const vk::QueueFlagBits required_capability = [&]
	{
		switch (Desc.QueueType)
		{
		case ECommandQueueType::Graphics: return vk::QueueFlagBits::eGraphics;
		case ECommandQueueType::Compute: return vk::QueueFlagBits::eCompute;
		case ECommandQueueType::Copy: return vk::QueueFlagBits::eTransfer;
		}
		return vk::QueueFlagBits::eGraphics;
	}();
	if (!(QueueCapabilities & required_capability))
		throw std::invalid_argument("The Vulkan queue does not support the requested command list type.");
	return std::make_shared<VulkanCommandList>(
		*this,
		GraphicsQueueFamilyIndex,
		Desc);
}

RSwapchain* VulkanDevice::createSwapchain()
{
	throwResourceNotImplemented("swapchain");
}

RTexture* VulkanDevice::createTexture()
{
	throwResourceNotImplemented("texture");
}

std::shared_ptr<DeviceMemory> VulkanDevice::allocateMemory(
	MemoryRequirements Requirements,
	EMemoryProperty Property)
{
	if (Requirements.Size == 0 || Requirements.MemoryTypeBits == 0)
	{
		throw std::invalid_argument("Vulkan memory requirements are invalid.");
	}

	const vk::MemoryAllocateInfo allocation_info(
		Requirements.Size,
		findMemoryType(Requirements.MemoryTypeBits, toVk(Property)));
	vk::DeviceMemory device_memory = LogicalDevice->allocateMemory(allocation_info);

	auto& memory = VulkanDeviceMemoryPool::self().allocateMemory();
	memory.OwnerDevice = LogicalDevice.get();
	memory.VkDeviceMemory = device_memory;
	memory.Requirements = Requirements;
	memory.Property = Property;
	memory.OwnershipState = DeviceMemory::EState::OwnsMemory;

	return std::shared_ptr<DeviceMemory>(
		&memory,
		VulkanDeviceMemoryDeleter(&VulkanDeviceMemoryPool::self()));
}

void VulkanDevice::freeMemory(std::shared_ptr<DeviceMemory> Memory)
{
	if (Memory)
	{
		Memory->release();
	}
}

void VulkanDevice::waitIdle()
{
	LogicalDevice->waitIdle();
}

void* VulkanDevice::getNativeHandle() const
{
	return static_cast<VkDevice>(LogicalDevice.get());
}

uint32_t VulkanDevice::findMemoryType(uint32_t TypeBits, vk::MemoryPropertyFlags Properties)
{
	vk::PhysicalDeviceMemoryProperties memory_properties = RealGPU.getMemoryProperties();

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((TypeBits & (1u << i)) &&
			(memory_properties.memoryTypes[i].propertyFlags & Properties) == Properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find a suitable Vulkan memory type.");
}

}


 