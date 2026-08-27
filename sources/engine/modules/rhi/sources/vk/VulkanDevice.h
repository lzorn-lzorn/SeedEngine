#pragma once

#include <memory>
#include <RHI.h>
#include <vulkan/vulkan.hpp>

namespace rhi
{

class VulkanDevice final : public RDevice
{
public:
	VulkanDevice(vk::PhysicalDevice& RealGPU, vk::UniqueDevice& LogicalDevice)
		: RealGPU(RealGPU), LogicalDevice(LogicalDevice) {}
	
	~VulkanDevice() = default;
	
public:
	RBuffer* createBuffer() override;
	RImage* createImage() override;
	RSampler* createSampler() override;
	RShader* createShader() override;
	RPipeline* createPipeline() override;
	RRenderPass* createRenderPass() override;
	RCommandList* createCommandList() override;
	RSwapchain* createSwapchain() override;
	RTexture* createTexture() override;

	void waitIdle() override;
	void* getNativeHandle() const override;
private:
	vk::PhysicalDevice& RealGPU;
	vk::UniqueDevice& LogicalDevice;
};

} // namespace rhi