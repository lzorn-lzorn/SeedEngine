#include "VulkanImage.h"

#include "VulkanDevice.h"
#include "VulkanDeviceMemory.h"
#include "VulkanRHI.h"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace rhi
{
namespace
{
uint32_t calculateMaximumMipLevels(const RImage::Descriptor_t& Desc)
{
	return std::bit_width(std::max({ Desc.Width, Desc.Height, Desc.Depth }));
}

vk::ImageCreateFlags getImageCreateFlags(EImageDimension Dimension)
{
	if (Dimension == EImageDimension::Cube || Dimension == EImageDimension::CubeArray)
	{
		return vk::ImageCreateFlagBits::eCubeCompatible;
	}
	return {};
}
} // namespace

VulkanImage::VulkanImage(VulkanDevice& InDevice, const Descriptor_t& Desc, vk::UniqueImage Image)
	: RImage(Desc), Device(&InDevice), Image(std::move(Image))
{
}

VulkanImage::VulkanImage(VulkanDevice& InDevice, const Descriptor_t& Desc, vk::Image Image)
	: RImage(Desc), Device(&InDevice), Image(Image)
{
}

std::shared_ptr<VulkanImage> VulkanImage::create(
	VulkanDevice& Device,
	const Descriptor_t& Desc,
	DeviceMemoryAllocator* Allocator)
{
	auto image = createUnbound(Device, Desc);
	image->allocateAndBindMemory(Allocator);
	return image;
}

std::shared_ptr<VulkanImage> VulkanImage::createUnbound(VulkanDevice& Device, const Descriptor_t& Desc)
{
	validateDescriptor(Desc);
	auto image = Device.getVkDevice().createImageUnique(makeCreateInfo(Desc));
	return std::shared_ptr<VulkanImage>(new VulkanImage(Device, Desc, std::move(image)));
}

std::shared_ptr<VulkanImage> VulkanImage::wrapExternal(
	VulkanDevice& Device,
	const Descriptor_t& Desc,
	vk::Image Image)
{
	validateDescriptor(Desc);
	if (!Image)
	{
		throw std::invalid_argument("Cannot wrap a null Vulkan image.");
	}
	return std::shared_ptr<VulkanImage>(new VulkanImage(Device, Desc, Image));
}

void VulkanImage::allocateAndBindMemory(DeviceMemoryAllocator* Allocator)
{
	if (isExternal())
	{
		throw std::logic_error("External Vulkan images cannot allocate owned memory.");
	}
	auto& owned_image = std::get<vk::UniqueImage>(Image);
	if (!owned_image || Memory)
	{
		throw std::logic_error(Memory ? "Vulkan image memory is already bound." : "Vulkan image is invalid.");
	}

	const vk::MemoryRequirements vk_requirements =
		Device->getVkDevice().getImageMemoryRequirements(owned_image.get());
	const MemoryRequirements requirements {
		.Size = vk_requirements.size,
		.Alignment = vk_requirements.alignment,
		.MemoryTypeBits = vk_requirements.memoryTypeBits
	};

	auto memory = Allocator
		? Allocator->allocateMemory(requirements, getDescriptor().MemoryProperty)
		: Device->allocateMemory(requirements, getDescriptor().MemoryProperty);
	auto vulkan_memory = std::dynamic_pointer_cast<VulkanDeviceMemory>(memory);
	if (!vulkan_memory)
	{
		throw std::invalid_argument("Image allocator returned non-Vulkan device memory.");
	}

	Device->getVkDevice().bindImageMemory(owned_image.get(), vulkan_memory->getVkDeviceMemory(), 0);
	Memory = std::move(memory);
}

void* VulkanImage::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkImage>(getVkImage()));
}

vk::Image VulkanImage::getVkImage() const noexcept
{
	if (const auto* external_image = std::get_if<vk::Image>(&Image))
	{
		return *external_image;
	}
	return std::get<vk::UniqueImage>(Image).get();
}

void VulkanImage::validateDescriptor(const Descriptor_t& Desc)
{
	if (Desc.Format == EFormat::Undefined)
		throw std::invalid_argument("Vulkan image format must be defined.");
	if (Desc.Width == 0 || Desc.Height == 0 || Desc.Depth == 0 || Desc.MipLevels == 0 || Desc.ArrayLayers == 0)
		throw std::invalid_argument("Image dimensions, mip levels and array layers must be non-zero.");
	if (toVk(Desc.Usage) == vk::ImageUsageFlags{})
		throw std::invalid_argument("Vulkan image usage must not be empty.");
	if (Desc.MipLevels > calculateMaximumMipLevels(Desc))
		throw std::invalid_argument("Image mip level count exceeds its dimensions.");
	if (Desc.SampleCount != ESampleCount::Count1 && Desc.MipLevels != 1)
		throw std::invalid_argument("Multisampled images must have exactly one mip level.");

	switch (Desc.Dimension)
	{
	case EImageDimension::Texture1D:
		if (Desc.Height != 1 || Desc.Depth != 1 || Desc.ArrayLayers != 1)
			throw std::invalid_argument("A 1D image requires height, depth and array layers equal to one.");
		break;
	case EImageDimension::Texture1DArray:
		if (Desc.Height != 1 || Desc.Depth != 1)
			throw std::invalid_argument("A 1D array image requires height and depth equal to one.");
		break;
	case EImageDimension::Texture2D:
		if (Desc.Depth != 1 || Desc.ArrayLayers != 1)
			throw std::invalid_argument("A 2D image requires depth and array layers equal to one.");
		break;
	case EImageDimension::Texture2DArray:
		if (Desc.Depth != 1)
			throw std::invalid_argument("A 2D array image requires depth equal to one.");
		break;
	case EImageDimension::Texture3D:
		if (Desc.ArrayLayers != 1)
			throw std::invalid_argument("A 3D image requires exactly one array layer.");
		break;
	case EImageDimension::Cube:
		if (Desc.Width != Desc.Height || Desc.Depth != 1 || Desc.ArrayLayers != 6)
			throw std::invalid_argument("A cube image requires square faces, depth one and six layers.");
		break;
	case EImageDimension::CubeArray:
		if (Desc.Width != Desc.Height || Desc.Depth != 1 || Desc.ArrayLayers % 6 != 0)
			throw std::invalid_argument("A cube array requires square faces and a layer count divisible by six.");
		break;
	}
}

vk::ImageCreateInfo VulkanImage::makeCreateInfo(const Descriptor_t& Desc)
{
	return vk::ImageCreateInfo()
		.setFlags(getImageCreateFlags(Desc.Dimension))
		.setImageType(toVk(Desc.Dimension))
		.setFormat(toVk(Desc.Format))
		.setExtent(vk::Extent3D(Desc.Width, Desc.Height, Desc.Depth))
		.setMipLevels(Desc.MipLevels)
		.setArrayLayers(Desc.ArrayLayers)
		.setSamples(toVk(Desc.SampleCount))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(toVk(Desc.Usage))
		.setSharingMode(Desc.SharingMode == ESharingMode::Concurrent
			? vk::SharingMode::eConcurrent
			: vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);
}

} // namespace rhi
