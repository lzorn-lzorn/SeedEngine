#include "VulkanDeviceMemory.h"
#include "VulkanDevice.h"
#include "VulkanRHI.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <utility>

namespace rhi
{

VulkanDeviceMemory::VulkanDeviceMemory(VulkanDeviceMemory&& Other)
	: DeviceMemory(std::move(Other))
	, VkDeviceMemory(std::move(Other.VkDeviceMemory))
	, Requirements(std::move(Other.Requirements))
	, Property(Other.Property)
	, OwnerDevice(Other.OwnerDevice)
{
	Other.reset();
}
VulkanDeviceMemory& VulkanDeviceMemory::operator=(VulkanDeviceMemory&& Other)
{
    if (this != &Other)
    {
		release();
        DeviceMemory::operator=(std::move(Other));
        VkDeviceMemory = std::exchange(Other.VkDeviceMemory, VK_NULL_HANDLE);
        Requirements = std::move(Other.Requirements);
        Property = Other.Property;
        OwnerDevice = std::exchange(Other.OwnerDevice, vk::Device{ VK_NULL_HANDLE });
		Other.reset();
    }
    return *this;
}

void VulkanDeviceMemory::reset()
{
	VkDeviceMemory = VK_NULL_HANDLE;
	OwnershipState = DeviceMemory::EState::None;
	Requirements = {};
	Property = EMemoryProperty::enum_type::None;
	OwnerDevice = vk::Device{ VK_NULL_HANDLE };
}

void VulkanDeviceMemory::release() {
	if (OwnershipState == DeviceMemory::EState::OwnsMemory && VkDeviceMemory != VK_NULL_HANDLE)
    {
        if (OwnerDevice)
        {
            OwnerDevice.freeMemory(VkDeviceMemory);
        }
    }
	reset();
}

void* VulkanDeviceMemory::map(DeviceSizeType Offset, DeviceSizeType Size)
{
    if (!OwnerDevice || VkDeviceMemory == VK_NULL_HANDLE)
    {
        throw std::logic_error("Cannot map invalid Vulkan device memory.");
    }
    if (Offset >= Requirements.Size || (Size != 0 && Size > Requirements.Size - Offset))
    {
        throw std::out_of_range("Vulkan device memory map range is out of bounds.");
    }
	void* data = nullptr;
    vk::Result result = OwnerDevice.mapMemory(
        VkDeviceMemory,
        Offset,
        Size == 0 ? VK_WHOLE_SIZE : Size,
        vk::MemoryMapFlags(),
        &data);
	if (result != vk::Result::eSuccess) {
		// TODO: 处理映射失败的情况
		throw std::runtime_error("Failed to map Vulkan device memory.");
	}
	return data;
}
void VulkanDeviceMemory::unmap()
{
	assert(OwnerDevice);
	OwnerDevice.unmapMemory(VkDeviceMemory);
}
void VulkanDeviceMemory::flush(DeviceSizeType Offset, DeviceSizeType Size)
{
	vk::Device device = OwnerDevice;

    vk::MappedMemoryRange range;
    range.setMemory(VkDeviceMemory);
    range.setOffset(Offset);
    range.setSize(Size == 0 ? VK_WHOLE_SIZE : Size);

    // 刷新 CPU 写入的数据, 使其对 GPU 可见
    device.flushMappedMemoryRanges({ range });
}
void VulkanDeviceMemory::invalidate(DeviceSizeType Offset, DeviceSizeType Size)
{
	vk::Device device = OwnerDevice;

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



VulkanDeviceMemoryPool::VulkanDeviceMemoryPool()
{
    pointer new_memory = static_cast<pointer>(std::malloc(sizeof(VulkanDeviceMemory) * InitialSize));
	if (!new_memory)
	{
        throw std::bad_alloc();
	}
	Blocks.emplace_back(new_memory, InitialSize);

    // 对每个对象执行 placement new，并放入 FreeList
    for (int32_t i = 0; i < InitialSize; ++i) {
        pointer obj = new (&new_memory[i]) VulkanDeviceMemory(); // 调用默认构造函数
        FreeList.push_back(obj);
    }
}

VulkanDeviceMemoryPool::~VulkanDeviceMemoryPool()
{
    assert(UsedList.empty() && "Vulkan device memory objects are still in use during pool destruction.");
    UsedList.clear();
    FreeList.clear();

    // 对每个内存块中的每个对象调用析构函数
    for (auto& block : Blocks) 
	{
        pointer raw = block.MemoryBlock;
        size_t count = block.Count;
        for (size_t i = 0; i < count; ++i) 
		{
            raw[i].~VulkanDeviceMemory(); // 显式析构
        }
        std::free(raw); // 释放原始内存
    }
}

void VulkanDeviceMemoryPool::reclaim(VulkanDeviceMemory* Memory) 
{
    if (!Memory)
    {
        return;
    }
    auto used = std::find(UsedList.begin(), UsedList.end(), Memory);
    if (used == UsedList.end())
    {
        return;
    }
    UsedList.erase(used);
    FreeList.push_back(Memory);
}

void VulkanDeviceMemoryPool::expand()
{
	// 使用两倍扩容策略:
	// TODO: 使用自定义的内存分配器接口, 而不是直接使用 malloc
    const size_t new_size = Capacity;

    pointer raw = static_cast<pointer>(
        std::malloc(sizeof(VulkanDeviceMemory) * new_size)
    );
    if (!raw) {
        throw std::bad_alloc();
    }

    Blocks.emplace_back(raw, new_size);

    for (size_t i = 0; i < new_size; ++i) {
        pointer obj = new (&raw[i]) VulkanDeviceMemory();
        FreeList.push_back(obj);
    }

    Capacity += new_size;
}
VulkanDeviceMemory& VulkanDeviceMemoryPool::allocateMemory()
{
	if (FreeList.empty()) {
		expand();
	}
	// 从 FreeList 取出一个指针
    pointer mem = FreeList.front();
    FreeList.pop_front();

    UsedList.push_back(mem);

    return *mem;
}

void VulkanDeviceMemoryDeleter::operator()(VulkanDeviceMemory* ptr) const
{
	if (!ptr)
	{
		return;
	}
	ptr->release();

    if (Pool)
    {
        Pool->reclaim(ptr);
    }
}



std::shared_ptr<DeviceMemory> VulkanDeviceMemoryAllocator::allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property)
{
    if (!Device)
	{
        throw std::invalid_argument("Vulkan memory allocation requires a valid device.");
    }
    return Device->allocateMemory(Requirements, Property);
}

void VulkanDeviceMemoryAllocator::freeMemory(std::shared_ptr<DeviceMemory> Memory)
{
    if (!Device)
	{
        throw std::logic_error("Vulkan memory allocator has no device.");
	}
    Device->freeMemory(std::move(Memory));
}
}