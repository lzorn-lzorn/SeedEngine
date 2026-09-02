#pragma once
#include <RHI.h>
#include <list>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rhi
{

class VulkanDevice;

class VulkanDeviceMemory final : public DeviceMemory
{
	friend class VulkanDevice;
	friend class VulkanDeviceMemoryDeleter;
	friend class VulkanDeviceMemoryPool;
	friend class VulkanDeviceMemoryAllocator;
public:
	VulkanDeviceMemory() 
		: VkDeviceMemory(VK_NULL_HANDLE)
		, Requirements({})
		, Property(EMemoryProperty::enum_type::None)
		, OwnerDevice(nullptr)
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
	MemoryRequirements Requirements;
	vk::DeviceMemory VkDeviceMemory { VK_NULL_HANDLE };	
	vk::Device OwnerDevice { VK_NULL_HANDLE };
	EMemoryProperty Property;
	
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
	explicit VulkanDeviceMemoryDeleter(VulkanDeviceMemoryPool* InPool) : Pool(InPool) {}

    void operator()(VulkanDeviceMemory* ptr) const;

private:
    VulkanDeviceMemoryPool* Pool = nullptr;
};

class VulkanDeviceMemoryAllocator final : public DeviceMemoryAllocator
{
public:
	explicit VulkanDeviceMemoryAllocator(VulkanDevice* InDevice = nullptr) noexcept
		: Device(InDevice) {}
	~VulkanDeviceMemoryAllocator() override = default;

	// note: 从池中分配一个 VulkanDeviceMemory, 
	// note: 在 GPU 上分配一段显存, 将其与 VulkanDeviceMemory 绑定, 并设置内部的对应的所有权字段
	std::shared_ptr<DeviceMemory> allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property) override;
	void freeMemory(std::shared_ptr<DeviceMemory> Memory) override;

	void setDevice(VulkanDevice* InDevice) noexcept { Device = InDevice; }
	[[nodiscard]] VulkanDevice* getDevice() const noexcept { return Device; }
private:
	VulkanDevice* Device = nullptr;
};
	
} // namespace rhi
