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
VulkanDeviceMemory& VulkanDeviceMemory::operator=(VulkanDeviceMemory&& Other)
{
    if (this != &Other)
    {
        DeviceMemory::operator=(std::move(Other));
        VkDeviceMemory = std::exchange(Other.VkDeviceMemory, VK_NULL_HANDLE);
        Requirements = std::move(Other.Requirements);
        Property = Other.Property;
        Allocator = std::exchange(Other.Allocator, nullptr);
    }
    return *this;
}

void VulkanDeviceMemory::reset()
{
	VkDeviceMemory = VK_NULL_HANDLE;
	OwnershipState = DeviceMemory::EState::None;
	Requirements = {};
	Property = EMemoryProperty::enum_type::None;
	Allocator = nullptr;
}

void VulkanDeviceMemory::release() {
	if (OwnershipState == DeviceMemory::EState::OwnsMemory && VkDeviceMemory != VK_NULL_HANDLE)
    {
        if (Allocator)
        {
            Allocator->getVkDevice().freeMemory(VkDeviceMemory);
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
	assert(Allocator != nullptr);
	Allocator->getVkDevice().unmapMemory(VkDeviceMemory);
}
void VulkanDeviceMemory::flush(DeviceSizeType Offset, DeviceSizeType Size)
{
	vk::Device device = Allocator->getVkDevice();

    vk::MappedMemoryRange range;
    range.setMemory(VkDeviceMemory);
    range.setOffset(Offset);
    range.setSize(Size == 0 ? VK_WHOLE_SIZE : Size);

    // 刷新 CPU 写入的数据, 使其对 GPU 可见
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



VulkanDeviceMemoryPool::VulkanDeviceMemoryPool()
{
	VulkanDeviceMemory* new_memory =  (VulkanDeviceMemory*) std::malloc(sizeof(VulkanDeviceMemory) * InitialSize);
	if (!new_memory)
	{
		// TODO: bad_alloc
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
    UsedList.remove(Memory);
    FreeList.push_back(Memory);
}

void VulkanDeviceMemoryPool::expand()
{
	// 使用两倍扩容策略:
	// TODO: 使用自定义的内存分配器接口, 而不是直接使用 malloc
	size_t new_size = UsedList.size() * 2; 

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

	Capacity = new_size;
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

VulkanDeviceMemoryDeleter::VulkanDeviceMemoryDeleter(VulkanDeviceMemoryAllocator* InAllocator, VulkanDeviceMemoryPool* InPool)
    : Allocator(InAllocator), Pool(InPool)
{}

void VulkanDeviceMemoryDeleter::operator()(VulkanDeviceMemory* ptr) const
{
    if (ptr->OwnershipState == DeviceMemory::EState::OwnsMemory && ptr->VkDeviceMemory != VK_NULL_HANDLE)
    {
        if (Allocator)
        {
            Allocator->getVkDevice().freeMemory(ptr->VkDeviceMemory);
        }
    }

    ptr->reset();

    if (Pool)
    {
        Pool->reclaim(ptr);
    }
}



std::shared_ptr<DeviceMemory> VulkanDeviceMemoryAllocator::allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property)
{
	vk::MemoryAllocateInfo alloc_info;
	alloc_info.setAllocationSize(Requirements.Size)
			.setMemoryTypeIndex(findMemoryType(Requirements.MemoryTypeBits, toVk(Property)));

	vk::DeviceMemory device_memory = VulkanDevice.allocateMemory(alloc_info);

	VulkanDeviceMemory& memory_ref = VulkanDeviceMemoryPool::self().allocateMemory();
    VulkanDeviceMemory* memory_ptr = &memory_ref;

	memory_ptr->Allocator = this;
    memory_ptr->VkDeviceMemory = device_memory;
    memory_ptr->Requirements = Requirements;
    memory_ptr->Property = Property;
    memory_ptr->OwnershipState = DeviceMemory::EState::OwnsMemory;

	VulkanDeviceMemoryDeleter deleter(this, &VulkanDeviceMemoryPool::self());
	return std::shared_ptr<DeviceMemory>(memory_ptr, deleter);
}

void VulkanDeviceMemoryAllocator::freeMemory(std::shared_ptr<DeviceMemory> Memory)
{
	// 通过 reset 触发删除器，自动释放 Vulkan 内存并回收对象
    if (Memory)
    {
        Memory.reset();
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