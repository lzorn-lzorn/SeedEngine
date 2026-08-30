#include "VulkanDeviceMemory.h"
#include "vulkan/vulkan.hpp"
#include "vma/vk_mem_alloc.h"
#include "VulkanRHI.h"

namespace rhi
{

VulkanDeviceMemory::VulkanDeviceMemory(VulkanDeviceMemory&& Other)
	: DeviceMemory(std::move(Other))
	, VkDeviceMemory(std::move(Other.VkDeviceMemory))
	, Requirements(std::move(Other.Requirements))
	, Property(Other.Property)
	, Allocator(Other.Allocator)
{
	reset();
}
VulkanDeviceMemory& VulkanDeviceMemory::operator=(VulkanDeviceMemory&&) = default;

void VulkanDeviceMemory::reset()
{
	VkDeviceMemory = VK_NULL_HANDLE;
	OwnershipState = DeviceMemory::EState::None;
	Requirements = {};
	Property = EMemoryProperty::enum_type::None;
	Allocator = nullptr;
}

void VulkanDeviceMemory::release() {
	if (OwnershipState == DeviceMemory::EState::OwnsMemory && VkDeviceMemory != VK_NULL_HANDLE) {
		if (Allocator) {
			Allocator->freeMemory(std::shared_ptr<DeviceMemory>(this));
		} else {
			vkFreeMemory(Allocator->getVkDevice(), VkDeviceMemory, nullptr);
		}
	}
	reset();
}

void* VulkanDeviceMemory::map(DeviceSizeType Offset, DeviceSizeType Size)
{
	assert(Allocator != nullptr && "Allocator must be set before mapping memory.");
	void* data = nullptr;
	vk::Result result = Allocator->getVkDevice().mapMemory(VkDeviceMemory, Offset, Size, vk::MemoryMapFlags(), &data);
	if (result != vk::Result::eSuccess) {
		// TODO: 处理映射失败的情况
		throw std::runtime_error("Failed to map Vulkan device memory.");
	}
	return data;
}
void VulkanDeviceMemory::unmap()
{
	Allocator->getVkDevice().unmapMemory(VkDeviceMemory);
}
void VulkanDeviceMemory::flush(DeviceSizeType Offset, DeviceSizeType Size)
{
	vk::Device device = Allocator->getVkDevice();

    vk::MappedMemoryRange range;
    range.setMemory(VkDeviceMemory);
    range.setOffset(Offset);
    range.setSize(Size == 0 ? VK_WHOLE_SIZE : Size);

    // 刷新 CPU 写入的数据，使其对 GPU 可见
    device.flushMappedMemoryRanges({ range });
}
void VulkanDeviceMemory::invalidate(DeviceSizeType Offset, DeviceSizeType Size)
{
	vk::Device device = Allocator->getVkDevice();

    vk::MappedMemoryRange range;
    range.setMemory(VkDeviceMemory);
    range.setOffset(Offset);
    range.setSize(Size == 0 ? VK_WHOLE_SIZE : Size);

    // 使 GPU 写入的数据对 CPU 可见
    device.invalidateMappedMemoryRanges({ range });
}
MemoryRequirements VulkanDeviceMemory::getMemoryRequirements() const
{
	return Requirements;
}
EMemoryProperty VulkanDeviceMemory::getMemoryProperty() const
{
	return Property;
}


std::shared_ptr<DeviceMemory> VulkanDeviceMemoryAllocator::allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property)
{
	vk::MemoryAllocateInfo alloc_info;
	alloc_info.setAllocationSize(Requirements.Size)
			.setMemoryTypeIndex(findMemoryType(Requirements.MemoryTypeBits, toVk(Property)));

	vk::DeviceMemory device_memory = VulkanDevice.allocateMemory(alloc_info);

	// TODO: 池化, 不要直接用 new 去创建
	VulkanDeviceMemory *memory = new VulkanDeviceMemory(Requirements, Property);
	memory->setVkDeviceMemory(device_memory);
	return std::shared_ptr<DeviceMemory>(memory);
}

void VulkanDeviceMemoryAllocator::freeMemory(std::shared_ptr<DeviceMemory> Memory)
{
	if (Memory && Memory->getState() == DeviceMemory::EState::OwnsMemory)
	{
		assert(dynamic_cast<VulkanDeviceMemory*>(Memory.get()) && "Memory must be of type VulkanDeviceMemory.");
		assert(static_cast<VulkanDeviceMemory*>(Memory.get())->getVkDeviceMemory() != VK_NULL_HANDLE && "VkDeviceMemory must be valid.");
		Memory->release();
	}
}

uint32_t VulkanDeviceMemoryAllocator::findMemoryType(uint32_t TypeBits, vk::MemoryPropertyFlags Properties)
{
    vk::PhysicalDeviceMemoryProperties memory_properties = VulkanPhysicalDevice.getMemoryProperties();

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