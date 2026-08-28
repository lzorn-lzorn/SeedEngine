#include "VulkanSwapchain.h"
#include <vulkan/vulkan.hpp>
#include <cassert>

namespace 
{

}
namespace rhi
{

VulkanSwapchain::VulkanSwapchain(vk::PhysicalDevice& RealGPU, vk::Instance& VulkanInstance)
	: RealGPU(RealGPU)
	, VulkanInstance(VulkanInstance)
	, NativeWindow(nullptr)
	, Surface(VK_NULL_HANDLE)
{
}

void VulkanSwapchain::setFormat(EFormat Format) 
{
	Properties.Format = toVkFormat(Format);
}

void VulkanSwapchain::setPresentMode(EPresentMode PresentMode)
{
	Properties.PresentMode = toVkPresentMode(PresentMode);
}


void VulkanSwapchain::setGenericWindow(ui::IGenericWindow* Window)
{
	NativeWindow = Window;
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

	return details;
}

void VulkanSwapchain::createSurface()
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
		HWND hwnd = static_cast<HWND>(NativeWindow->getNativeHandle());
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

void VulkanSwapchain::createVkSwapchain()
{
	assert(NativeWindow != nullptr && "NativeWindow must be set before creating the swapchain.");
	createSurface();
}
}