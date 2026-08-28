#pragma once
#include <RHI.h>
#include <vulkan/vulkan.hpp>
#include <generic_application/window/GenericWindow.hpp>
#include "VulkanRHI.h"
#include "vulkan/vulkan.hpp"
namespace rhi
{

class VulkanSwapchain final : public RSwapchain
{
public:
	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR Capabilities;
		std::vector<vk::SurfaceFormatKHR> Formats;
		std::vector<vk::PresentModeKHR> PresentModes;
	};

	struct SwapchainProperties {
		vk::Format Format {VK_FORMAT_B8G8R8A8_SRGB}; // 图像颜色格式
		vk::ColorSpaceKHR ColorSpace {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		vk::PresentModeKHR PresentMode {VK_PRESENT_MODE_MAILBOX_KHR};
		vk::Extent2D Extent;                      // 交换链图像的像素尺寸
		vk::SurfaceTransformFlagBitsKHR PreTransform {VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR}; // 表面变换(如旋转 90 度、水平翻转)
		vk::CompositeAlphaFlagBitsKHR CompositeAlpha {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR}; // 与窗口系统合成的 alpha 通道处理方式
		vk::ImageUsageFlags ImageUsage;           // 图像的用途标志, 例如 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		uint32_t ImageCount {2};                  // 交换链图像数量
		vk::SharingMode ImageSharingMode {VK_SHARING_MODE_EXCLUSIVE}; // 图像在队列族之间的共享模式
		vk::Bool32 Clipped {VK_TRUE};
		vk::SwapchainKHR OldSwapchain {VK_NULL_HANDLE}; // 重建交换链时, 用于传递旧的交换链以复用资源
	};
public:
	VulkanSwapchain(vk::PhysicalDevice& RealGPU, vk::Instance& VulkanInstance);
	virtual ~VulkanSwapchain() = default;

	void setGenericWindow(ui::IGenericWindow*) override;
	void setFormat(EFormat) override;
	void setPresentMode(EPresentMode) override;
	

	void resize(uint32_t width, uint32_t height) override;
	void present() override;


	void setPhysicalDevice(vk::PhysicalDevice& RealGPU) { RealGPU = RealGPU; }
	void setVkInstance(vk::Instance& Instance) { VulkanInstance = Instance; }
private:
	void createSurface();
	void createVkSwapchain();
	SwapchainSupportDetails querySwapChainSupport();
	void setSwapExtent();
private:
	ui::IGenericWindow* NativeWindow;
	vk::PhysicalDevice& RealGPU;
	vk::Instance& VulkanInstance;
	vk::SurfaceKHR Surface;
	SwapchainProperties Properties;
};

}