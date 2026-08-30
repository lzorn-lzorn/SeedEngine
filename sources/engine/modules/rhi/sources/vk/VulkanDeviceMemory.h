#pragma once
#include <RHI.h>
#include <vulkan/vulkan.hpp>
#include "vulkan/vulkan.hpp"

namespace rhi
{

class VulkanDeviceMemory final : public DeviceMemory
{
public:
	VulkanDeviceMemory() = default;
	~VulkanDeviceMemory() override = default;
	VulkanDeviceMemory(const VulkanDeviceMemory&) = delete;
	VulkanDeviceMemory& operator=(const VulkanDeviceMemory&) = delete;

	VulkanDeviceMemory(VulkanDeviceMemory&&);
	VulkanDeviceMemory& operator=(VulkanDeviceMemory&&);
	VulkanDeviceMemory(MemoryRequirements InRequirements, EMemoryProperty InProperty)
		: Requirements(InRequirements), Property(InProperty) {}

	void* map(DeviceSizeType Offset = 0, DeviceSizeType Size = 0) override;
	void unmap() override;
	void flush(DeviceSizeType Offset, DeviceSizeType Size) override;
	void invalidate(DeviceSizeType Offset, DeviceSizeType Size) override;
	void release() override;

	VulkanDeviceMemory& setVkDeviceMemory(vk::DeviceMemory InVkDeviceMemory) { VkDeviceMemory = InVkDeviceMemory; return *this; }

	vk::DeviceMemory getVkDeviceMemory() const { return VkDeviceMemory; } 
	vk::DeviceMemory getVkDeviceMemory() { return VkDeviceMemory; } 
	MemoryRequirements getMemoryRequirements() const override;
	EMemoryProperty getMemoryProperty() const override;

private:
	void reset();
	
	vk::DeviceMemory VkDeviceMemory { VK_NULL_HANDLE };	
	MemoryRequirements Requirements;
	EMemoryProperty Property;
	class VulkanDeviceMemoryAllocator* Allocator { nullptr };
	
};

class VulkanDeviceMemoryAllocator final : public DeviceMemoryAllocator
{
public:
	~VulkanDeviceMemoryAllocator() override = default;

	std::shared_ptr<DeviceMemory> allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property) override;
	void freeMemory(std::shared_ptr<DeviceMemory> Memory) override;

	vk::Device getVkDevice() const { return VulkanDevice; }
private:
	uint32_t findMemoryType(uint32_t TypeBits, vk::MemoryPropertyFlags Properties);

	vk::Device VulkanDevice;
	vk::PhysicalDevice VulkanPhysicalDevice;
};

}
