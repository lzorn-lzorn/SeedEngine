#pragma once
#include <RHI.h>
#include <list>
#include <vulkan/vulkan.hpp>
#include "vulkan/vulkan.hpp"

namespace rhi
{

class VulkanDeviceMemory final : public DeviceMemory
{
	friend class VulkanDeviceMemoryDeleter;
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

// @note: 线程不安全只能在 RHI 线程中使用
class VulkanDeviceMemoryPool final
{
	using value_type = VulkanDeviceMemory;
	using pointer = VulkanDeviceMemory*;
	constexpr static int32_t InitialSize = 512;
public:
	static VulkanDeviceMemoryPool& self() {
		static VulkanDeviceMemoryPool instance;
		return instance;
	}

	~VulkanDeviceMemoryPool();
	VulkanDeviceMemoryPool(const VulkanDeviceMemoryPool&) = delete;
	VulkanDeviceMemoryPool& operator=(const VulkanDeviceMemoryPool&) = delete;

	VulkanDeviceMemory& allocateMemory();
	
	void reclaim(VulkanDeviceMemory* Memory);

	struct Block {
		pointer MemoryBlock;
		size_t Count;
	};
private:
	VulkanDeviceMemoryPool();

	void expand();
	std::list<pointer> FreeList;
	std::vector<Block> Blocks; // 原始内存块及对象数量
	std::list<pointer> UsedList;                     // 已分配出去的对象
	size_t Capacity { InitialSize };                 // 当前池总容量
};

class VulkanDeviceMemoryDeleter
{
public:
    VulkanDeviceMemoryDeleter() = default;
    VulkanDeviceMemoryDeleter(class VulkanDeviceMemoryAllocator*, VulkanDeviceMemoryPool*);

    void operator()(VulkanDeviceMemory* ptr) const;

private:
    class VulkanDeviceMemoryAllocator* Allocator = nullptr;
    VulkanDeviceMemoryPool* Pool = nullptr;
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
