#include "VulkanBuffer.hpp"

#include "VulkanDevice.h"
#include "VulkanDeviceMemory.h"
#include "VulkanRHI.h"

#include <stdexcept>
#include <utility>

namespace rhi
{

VulkanBuffer::VulkanBuffer(
	VulkanDevice& InDevice,
	BufferDescriptor Desc,
	std::shared_ptr<DeviceMemory> InMemory,
	vk::UniqueBuffer InBuffer)
	: Device(&InDevice)
	, Descriptor(std::move(Desc))
	, Memory(std::move(InMemory))
	, Buffer(std::move(InBuffer))
{
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::create(
	VulkanDevice& Device,
	const BufferDescriptor& Desc)
{
	if (Desc.Size == 0)
		throw std::invalid_argument("Vulkan buffer size must be non-zero.");
	if (toVk(Desc.Usage) == vk::BufferUsageFlags{})
		throw std::invalid_argument("Vulkan buffer usage must not be empty.");
	if (Desc.MemoryProperty.has(EMemoryProperty_t::HostCoherent) &&
		!Desc.MemoryProperty.has(EMemoryProperty_t::HostVisible))
	{
		throw std::invalid_argument("Host-coherent buffer memory must also be host-visible.");
	}
	if (Desc.MemoryProperty.has(EMemoryProperty_t::HostCached) &&
		!Desc.MemoryProperty.has(EMemoryProperty_t::HostVisible))
	{
		throw std::invalid_argument("Host-cached buffer memory must also be host-visible.");
	}

	auto buffer = Device.getVkDevice().createBufferUnique(vk::BufferCreateInfo(
		{}, Desc.Size, toVk(Desc.Usage), vk::SharingMode::eExclusive));
	const auto vk_requirements = Device.getVkDevice().getBufferMemoryRequirements(buffer.get());
	const MemoryRequirements requirements {
		.Size = vk_requirements.size,
		.Alignment = vk_requirements.alignment,
		.MemoryTypeBits = vk_requirements.memoryTypeBits
	};
	auto memory = Device.allocateMemory(requirements, Desc.MemoryProperty);
	auto vulkan_memory = std::dynamic_pointer_cast<VulkanDeviceMemory>(memory);
	if (!vulkan_memory)
		throw std::logic_error("Vulkan device returned non-Vulkan memory for a buffer.");
	Device.getVkDevice().bindBufferMemory(buffer.get(), vulkan_memory->getVkDeviceMemory(), 0);

	return std::shared_ptr<VulkanBuffer>(new VulkanBuffer(
		Device, Desc, std::move(memory), std::move(buffer)));
}

RDevice& VulkanBuffer::getDevice() const noexcept
{
	return *Device;
}

void* VulkanBuffer::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkBuffer>(Buffer.get()));
}

void* VulkanBuffer::map(DeviceSizeType Offset, DeviceSizeType Size)
{
	if (!Descriptor.MemoryProperty.has(EMemoryProperty_t::HostVisible))
		throw std::logic_error("Only host-visible Vulkan buffers can be mapped.");
	if (IsMapped)
		throw std::logic_error("Vulkan buffer memory is already mapped.");
	if (Offset >= Descriptor.Size || (Size != 0 && Size > Descriptor.Size - Offset))
		throw std::out_of_range("Vulkan buffer map range is out of bounds.");
	void* result = Memory->map(Offset, Size == 0 ? Descriptor.Size - Offset : Size);
	IsMapped = true;
	return result;
}

void VulkanBuffer::unmap()
{
	if (!IsMapped)
		throw std::logic_error("Vulkan buffer memory is not mapped.");
	Memory->unmap();
	IsMapped = false;
}

void VulkanBuffer::flush(DeviceSizeType Offset, DeviceSizeType Size)
{
	if (!IsMapped)
		throw std::logic_error("Vulkan buffer must be mapped before flushing it.");
	if (Offset >= Descriptor.Size || (Size != 0 && Size > Descriptor.Size - Offset))
		throw std::out_of_range("Vulkan buffer flush range is out of bounds.");
	Memory->flush(Offset, Size == 0 ? Descriptor.Size - Offset : Size);
}

void VulkanBuffer::invalidate(DeviceSizeType Offset, DeviceSizeType Size)
{
	if (!IsMapped)
		throw std::logic_error("Vulkan buffer must be mapped before invalidating it.");
	if (Offset >= Descriptor.Size || (Size != 0 && Size > Descriptor.Size - Offset))
		throw std::out_of_range("Vulkan buffer invalidate range is out of bounds.");
	Memory->invalidate(Offset, Size == 0 ? Descriptor.Size - Offset : Size);
}

} // namespace rhi
