#include "RHI.h"
#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "VulkanDeviceMemory.h"
#include "VulkanImageView.h"
#include <stdexcept>

namespace rhi
{

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

RShader* VulkanDevice::createShader()
{
	throwResourceNotImplemented("shader");
}

RPipeline* VulkanDevice::createPipeline()
{
	throwResourceNotImplemented("pipeline");
}

RRenderPass* VulkanDevice::createRenderPass()
{
	throwResourceNotImplemented("render pass");
}

RCommandList* VulkanDevice::createCommandList()
{
	throwResourceNotImplemented("command list");
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
            if ((TypeBits & (1 << i)) && 
                (memory_properties.memoryTypes[i].propertyFlags & Properties) == Properties) 
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    }

}


 