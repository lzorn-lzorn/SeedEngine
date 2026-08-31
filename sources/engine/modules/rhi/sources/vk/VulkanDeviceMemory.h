#pragma once
#include <RHI.h>
#include <list>
#include <vulkan/vulkan.hpp>
#include "vulkan/vulkan.hpp"

namespace rhi
{

class VulkanDeviceMemory final : public DeviceMemory
{
	friend class VulkanDeviceMemoryPool;
	friend class VulkanDeviceMemoryAllocator;
public:
	VulkanDeviceMemory() 
		: VkDeviceMemory(VK_NULL_HANDLE)
		, Requirements({})
		, Property(EMemoryProperty::enum_type::None)
		, Allocator(nullptr)
	{}
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

class VulkanDeviceMemoryPool final
{
	struct VulkanDeviceMemoryDeleter
	{
		void operator()(VulkanDeviceMemory* Memory) const
		{
			if (Memory)
			{
				Memory->reset();
				VulkanDeviceMemoryPool::self().FreeList.emplace_back(Memory, VulkanDeviceMemoryDeleter{ });
			}
		}
	};
	using value_type = std::unique_ptr<VulkanDeviceMemory, VulkanDeviceMemoryDeleter>;

public:
	static VulkanDeviceMemoryPool& self() {
		static VulkanDeviceMemoryPool instance;
		return instance;
	}

	~VulkanDeviceMemoryPool() = default;
	VulkanDeviceMemoryPool(const VulkanDeviceMemoryPool&) = delete;
	VulkanDeviceMemoryPool& operator=(const VulkanDeviceMemoryPool&) = delete;

	VulkanDeviceMemory const & allocateMemory();
private:
	VulkanDeviceMemoryPool() = default;

	void expand();
	std::list<value_type> FreeList;
	std::list<value_type> UsedList;
};


class VulkanDeviceMemoryAllocator final : public DeviceMemoryAllocator
{
public:
	~VulkanDeviceMemoryAllocator() override = default;

	// note: 从池中分配一个 VulkanDeviceMemory, 
	// note: 在 GPU 上分配一段显存, 将其与 VulkanDeviceMemory 绑定, 并设置内部的对应的所有权字段
	std::shared_ptr<DeviceMemory> allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property) override;
	void freeMemory(std::shared_ptr<DeviceMemory> Memory) override;

	VulkanDeviceMemory bindBuffer(vk::Buffer);
	VulkanDeviceMemory bindImage(vk::Image);

	vk::Device getVkDevice() const { return VulkanDevice; }
private:
	uint32_t findMemoryType(uint32_t TypeBits, vk::MemoryPropertyFlags Properties);

	vk::Device VulkanDevice;
	vk::PhysicalDevice VulkanPhysicalDevice;
};

}
