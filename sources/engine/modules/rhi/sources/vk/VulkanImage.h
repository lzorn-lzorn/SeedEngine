#pragma once
#include <RHI.h>
#include <expected>
#include <vulkan/vulkan.hpp>
#include "VulkanRHI.h"
#include "vulkan/vulkan.hpp"


namespace rhi
{

class VulkanImage : public RImage
{

protected:
	void initializeImage() override;
	void allocateMemory(DeviceMemoryAllocator* Allocator) override;
	void bindMemory() override;

};


static std::shared_ptr<VulkanImage> create(const RImage::Descriptor_t& Desc,  DeviceMemoryAllocator* Allocator = nullptr)
{

}

static std::shared_ptr<VulkanImage> createUnbound(const RImage::Descriptor_t& Desc)
{

}
}