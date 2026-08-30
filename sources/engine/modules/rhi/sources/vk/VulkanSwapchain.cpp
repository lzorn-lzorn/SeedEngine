#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan.hpp>
#include <cassert>

namespace 
{

}
namespace rhi
{

VulkanSwapchain::VulkanSwapchain(vk::PhysicalDevice& RealGPU, vk::Instance& InVkInstance, vk::Device& InDevice)
	: RealGPU(RealGPU)
	, VulkanInstance(InVkInstance)
	, VulkanDevice(InDevice)
	, Surface(VK_NULL_HANDLE)
{
}


VulkanSwapchain::SwapchainSupportDetails VulkanSwapchain::querySwapChainSupport() 
{
	VulkanSwapchain::SwapchainSupportDetails details;
	{		
		vk::Result result = RealGPU.getSurfaceCapabilitiesKHR(Surface, &details.Capabilities);
		assert(result == vk::Result::eSuccess && "Failed to get surface capabilities.");
	}

	uint32_t format_count;
	{
		vk::Result result = RealGPU.getSurfaceFormatsKHR(Surface, &format_count, nullptr);
		assert(result == vk::Result::eSuccess && "Failed to get surface formats.");
		if (format_count != 0) {
			details.Formats.resize(format_count);
			result = RealGPU.getSurfaceFormatsKHR(Surface, &format_count, details.Formats.data());
		}
	}
	assert(!details.Formats.empty() && "No supported surface formats.");
	
	uint32_t present_mode_count;
	{
		vk::Result result = RealGPU.getSurfacePresentModesKHR(Surface, &present_mode_count, nullptr);
		assert(result == vk::Result::eSuccess && "Failed to get surface present modes.");
		if (present_mode_count != 0) 
		{
			details.PresentModes.resize(present_mode_count);
			result = RealGPU.getSurfacePresentModesKHR(Surface, &present_mode_count, details.PresentModes.data());
		}
	}
	assert(!details.PresentModes.empty() && "No supported present modes.");
	
	return details;
}

std::expected<bool, std::string> VulkanSwapchain::checkSwapChainSupport()
{
	SwapchainSupportDetails supported = querySwapChainSupport();
	std::expected<bool, std::string> result = std::expected<bool, std::string>(true);

	if (vk::FormatProperties props = RealGPU.getFormatProperties(toVk(getProperties().Format));
		!(props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
		&& (toVk(getProperties().ImageUsage) & vk::ImageUsageFlagBits::eColorAttachment))
	{
		// 不支持作为颜色附件
		// TODO: 日志
		result = std::unexpected("Selected format does not support color attachment usage.");
	}

	// 检查交换链的宽度和高度是否在支持的范围内
	if ((getWidth() != 0xFFFFFFFF && getHeight() != 0xFFFFFFFF) &&
		(getWidth() <= supported.Capabilities.minImageExtent.width ||
		getWidth() >= supported.Capabilities.maxImageExtent.width ||
		getHeight() <= supported.Capabilities.minImageExtent.height ||
		getHeight() >= supported.Capabilities.maxImageExtent.height))
	{
		result = std::unexpected("Swapchain extent is out of supported range.");
	}

	// 检查交换链图像数量是否在支持的范围内
	if (getProperties().ImageCount < supported.Capabilities.minImageCount || 
		(getProperties().ImageCount > 0 && getProperties().ImageCount > supported.Capabilities.maxImageCount))
	{
		result = std::unexpected("Swapchain image count is out of supported range.");
	}

	// 检查当前呈现模式硬件是否支持
	if (auto SupportedPresetModes =  RealGPU.getSurfacePresentModesKHR();
		std::find(SupportedPresetModes.begin(), SupportedPresetModes.end(), toVk(getProperties().PresentMode)) == SupportedPresetModes.end())
	{
		result = std::unexpected("Selected present mode is not supported.");
	}

	// 检查预变换是否被支持
	if (!(supported.Capabilities.supportedTransforms & toVk(getProperties().PreTransform)))
	{
		result = std::unexpected("Selected pre-transform is not supported.");
	}

	// 检查复合 alpha 是否被支持
	if (!(supported.Capabilities.supportedCompositeAlpha & toVk(getProperties().CompositeAlpha)))
	{
		result = std::unexpected("Selected composite alpha is not supported.");
	}

	if (!(supported.Capabilities.supportedUsageFlags & toVk(getProperties().ImageUsage)))
	{
		result = std::unexpected("Selected image usage is not supported.");
	}
	return result;
}

void VulkanSwapchain::initializeVkSurface()
{
#ifdef USE_SDL
	{
		// SDL3 创建 Vulkan Surface
		SDL_Window* SDLWindow = static_cast<SDL_Window*>(NativeWindow->getNativeHandle());
		if (SDLWindow == nullptr)
		{
			throw std::runtime_error("Failed to get SDL_Window from GenericWindow.");
		}
		VkSurfaceKHR vkSurface;
		if (SDL_Vulkan_CreateSurface(SDLWindow, VulkanRHI::self().getVkInstance(), &vkSurface) == VK_FALSE)
		{
			throw std::runtime_error("Failed to create Vulkan surface using SDL.");
		}
		Surface = vk::SurfaceKHR(vkSurface);
	}
#else
#	if defined(WIN32)
	{
		// Windows 平台创建 Vulkan Surface
		HWND hwnd = static_cast<HWND>(getNativeWindow()->getNativeHandle());
		if (hwnd == nullptr)
		{
			throw std::runtime_error("Failed to get HWND from GenericWindow.");
		}
		vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo(
			vk::Win32SurfaceCreateFlagsKHR(),
			GetModuleHandle(nullptr),
			hwnd
		);
		Surface = VulkanInstance.createWin32SurfaceKHR(surfaceCreateInfo);
	}
#	elif defined(APPLE)
	{


	}
#	else
	static_assert(false, "Unsupported platform for Vulkan surface creation.");

#	endif
#endif
}

void VulkanSwapchain::initializeVkSwapchain()
{
	assert(getNativeWindow() != nullptr && "NativeWindow must be set before creating the swapchain.");
	assert(Surface != VK_NULL_HANDLE && "Surface must be created before creating the swapchain.");

	QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(RealGPU, Surface);
	uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

	SwapchainExtent = vk::Extent2D{
		static_cast<uint32_t>(getWidth()),
		static_cast<uint32_t>(getHeight())
	};
	vk::SwapchainCreateInfoKHR create_info = vk::SwapchainCreateInfoKHR()
		.setSurface(Surface)
		.setMinImageCount(getProperties().ImageCount)
		.setImageFormat(toVk(getProperties().Format))
		.setImageColorSpace(toVk(getProperties().ColorSpace))
		.setImageExtent(SwapchainExtent)
		.setImageArrayLayers(1)
		.setImageUsage(toVk(getProperties().ImageUsage))
		.setImageSharingMode(toVk(getProperties().ImageSharingMode))
		.setPreTransform(toVk(getProperties().PreTransform))
		.setCompositeAlpha(toVk(getProperties().CompositeAlpha))
		.setPresentMode(toVk(getProperties().PresentMode))
		.setClipped(getProperties().Clipped)
		.setOldSwapchain(static_cast<VulkanSwapchain*>(getProperties().OldSwapchain)->getVkSwapchain());

	Swapchain = VulkanDevice.createSwapchainKHR(create_info);
	SwapchainImages = VulkanDevice.getSwapchainImagesKHR(Swapchain);
	SwapchainImageFormat = toVk(getProperties().Format);

	SwapchainImageViews.reserve(SwapchainImages.size());
    for (auto image : SwapchainImages) {
        vk::ImageViewCreateInfo view_info;
		view_info.setImage(image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(SwapchainImageFormat)
				.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        SwapchainImageViews.push_back(VulkanDevice.createImageView(view_info));
    }
}

}