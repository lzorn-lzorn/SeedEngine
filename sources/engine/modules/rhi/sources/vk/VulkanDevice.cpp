
#include "VulkanRHI.h"
#include "VulkanDevice.h"

#include <stdexcept>

namespace rhi
{

namespace
{
[[noreturn]] void throwResourceNotImplemented(const char* Resource)
{
	throw std::logic_error(std::string("Vulkan ") + Resource + " creation is not implemented yet.");
}
}

RBuffer* VulkanDevice::createBuffer()
{
	throwResourceNotImplemented("buffer");
}

RImage* VulkanDevice::createImage()
{
	throwResourceNotImplemented("image");
}

RSampler* VulkanDevice::createSampler()
{
	throwResourceNotImplemented("sampler");
}

RShader* VulkanDevice::createShader()
{
	throwResourceNotImplemented("shader");
}

RPipeline* VulkanDevice::createPipeline()
{
	throwResourceNotImplemented("pipeline");
}

RRenderPass* VulkanDevice::createRenderPass()
{
	throwResourceNotImplemented("render pass");
}

RCommandList* VulkanDevice::createCommandList()
{
	throwResourceNotImplemented("command list");
}

RSwapchain* VulkanDevice::createSwapchain()
{
	throwResourceNotImplemented("swapchain");
}

RTexture* VulkanDevice::createTexture()
{
	throwResourceNotImplemented("texture");
}

void VulkanDevice::waitIdle()
{
	LogicalDevice->waitIdle();
}

void* VulkanDevice::getNativeHandle() const
{
	return static_cast<VkDevice>(LogicalDevice.get());
}


}
