#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "vulkan/vulkan.hpp"

#include <iostream>
#include <vector>
#include <set>
#include <print>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace 
{
static std::vector<const char*> getRequiredInstanceExtensions()
{
 	// 若无窗口系统, 可不启用任何实例扩展; 否则按平台添加
    std::vector<const char*> extensions;
    // example Windows:
    // extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    // extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    return extensions;
}

static bool checkInstanceExtensions(const std::vector<const char*>& Required)
{
    if (Required.empty()) return true;
    auto available = vk::enumerateInstanceExtensionProperties();
    std::set<std::string> available_set;
    for (const auto& ext : available)
    {
		available_set.insert(ext.extensionName);
	}
    for (const char* ext : Required) 
	{
        if (available_set.find(ext) == available_set.end()) 
		{
            std::cerr << "Missing instance extension: " << ext << std::endl;
            return false;
        }
    }
    return true;
}

static bool checkDeviceExtensions(vk::PhysicalDevice RealGPU, const std::vector<const char*>& Required)
{
    if (Required.empty()) return true;
    auto available = RealGPU.enumerateDeviceExtensionProperties();
    std::set<std::string> available_set;
    for (const auto& ext : available)
    {
		available_set.insert(ext.extensionName);
	}
    for (const char* ext : Required) 
	{
        if (available_set.find(ext) == available_set.end()) 
		{
            std::cerr << "Missing device extension: " << ext << std::endl;
            return false;
        }
    }
    return true;
}

static bool checkDeviceFeatures(vk::PhysicalDevice RealGPU, const vk::PhysicalDeviceFeatures& Required)
{
    auto supported = RealGPU.getFeatures();
    // 仅检查我们需要的特性
    if (Required.samplerAnisotropy && !supported.samplerAnisotropy) return false;
    if (Required.fillModeNonSolid  && !supported.fillModeNonSolid)  return false;
    if (Required.geometryShader    && !supported.geometryShader)    return false;
    if (Required.tessellationShader && !supported.tessellationShader) return false;
    return true;
}


static uint64_t getDeviceLocalMemorySize(vk::PhysicalDevice RealGPU)
{
    auto mem_props = RealGPU.getMemoryProperties();
    uint64_t max_device_local_size = 0;
    for (uint32_t i = 0; i < mem_props.memoryHeapCount; ++i) 
	{
        if (mem_props.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) 
		{
            max_device_local_size = std::max(max_device_local_size, mem_props.memoryHeaps[i].size);
        }
    }
    return max_device_local_size;
}

static int scoreDevice(vk::PhysicalDevice RealGPU)
{
    int score = 0;
    auto props = RealGPU.getProperties();

    // 设备类型基础分
    switch (props.deviceType) {
    case vk::PhysicalDeviceType::eDiscreteGpu:   score += 1000; break;
    case vk::PhysicalDeviceType::eIntegratedGpu: score += 500;  break;
    case vk::PhysicalDeviceType::eVirtualGpu:    score += 200;  break;
    case vk::PhysicalDeviceType::eCpu:           score += 100;  break;
    default:                                     score += 0;    break;
    }

    // 显存大小（每 MB 加 1 分）
    uint64_t mem_size = getDeviceLocalMemorySize(RealGPU);
    score += static_cast<int>(mem_size / (1024 * 1024));

    // 队列能力加分
    auto queue_families = RealGPU.getQueueFamilyProperties();
    bool has_graphics = false;
    bool has_compute = false;
    bool has_separate_compute = false;
    for (const auto& qf : queue_families) {
        if (qf.queueFlags & vk::QueueFlagBits::eGraphics) has_graphics = true;
        if (qf.queueFlags & vk::QueueFlagBits::eCompute)  has_compute  = true;
        if ((qf.queueFlags & vk::QueueFlagBits::eCompute) &&
            !(qf.queueFlags & vk::QueueFlagBits::eGraphics)) {
            has_separate_compute = true;
        }
    }
    if (has_graphics && has_compute) score += 100;
    if (has_separate_compute)        score += 50;

    return score;
}


}
namespace rhi
{

uint64_t clampCopySize(uint64_t Offset, uint64_t RequestedSize, uint64_t MaxSize)
{
	if (Offset >= MaxSize)
	{
		return 0;
	}

	const uint64_t available_size = MaxSize - Offset;
	if (RequestedSize == 0)
	{
		return available_size;
	}
	return std::min<uint64_t>(RequestedSize, available_size);
}



void VulkanRHI::createVkInstance()
{
    vk::ApplicationInfo app_info(
        "SeedEngine",
        VK_MAKE_VERSION(1, 0, 0),
        "SeedEngine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3   // 请求 Vulkan 1.3, 驱动不支持时会回退
    );

    std::vector<const char*> layers;
#ifndef NDEBUG
    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    auto available_layers = vk::enumerateInstanceLayerProperties();
    for (const auto& layer : available_layers) 
	{
        if (strcmp(layer.layerName, validation_layer) == 0) 
		{
            layers.push_back(validation_layer);
            break;
        }
    }
// TODO: 接入日志系统
    if (layers.empty())
	{
		std::cerr << "Warning: Validation layer requested but not available." << std::endl;
	}
#endif

    auto required_instance_extensions = getRequiredInstanceExtensions();
    if (!checkInstanceExtensions(required_instance_extensions))
	{
        throw std::runtime_error("Required instance extensions are not supported.");
    }

    vk::InstanceCreateInfo create_info(
        vk::InstanceCreateFlags(),
        &app_info,
        static_cast<uint32_t>(layers.size()),
        layers.empty() ? nullptr : layers.data(),
        static_cast<uint32_t>(required_instance_extensions.size()),
        required_instance_extensions.empty() ? nullptr : required_instance_extensions.data()
    );

    Instance = vk::createInstanceUnique(create_info);
}

void VulkanRHI::pickPhysicalDevice()
{
	if (!Instance)
	{
		throw std::runtime_error("Vulkan instance not created before picking physical device.");
	}
        

    auto physical_devices = Instance->enumeratePhysicalDevices();
    if (physical_devices.empty())
    {
		throw std::runtime_error("No Vulkan physical devices found.");
	}

    // TODO: 需要支持的物理特性
    vk::PhysicalDeviceFeatures required_features{};
    required_features.samplerAnisotropy = VK_TRUE;
    required_features.fillModeNonSolid  = VK_TRUE;
    required_features.geometryShader    = VK_TRUE;
    required_features.tessellationShader = VK_TRUE;

    // TODO: 需要支持的设备扩展
    std::vector<const char*> requiredDeviceExtensions = 
	{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,   // 交换链
    };

    vk::PhysicalDevice best_device = nullptr;
    int best_score = -1;

    for (const auto& device : physical_devices) 
	{
        // ---- 检查核心特性 ----
        if (!checkDeviceFeatures(device, required_features)) 
		{
			std::println("Device {} does not support required features, skipping.", 
				device.getProperties().deviceName);
            continue;
        }

        // ---- 检查设备扩展 ----
        if (!checkDeviceExtensions(device, requiredDeviceExtensions)) 
		{
            std::println("Device {} does not support required extensions, skipping.", 
				device.getProperties().deviceName);
            continue;
        }

        // ---- 检查队列族(至少需要图形和计算) ----
        auto queue_families = device.getQueueFamilyProperties();
        bool has_graphics = false;
        bool has_compute = false;
        for (const auto& qf : queue_families) 
		{
            if (qf.queueFlags & vk::QueueFlagBits::eGraphics) has_graphics = true;
            if (qf.queueFlags & vk::QueueFlagBits::eCompute)  has_compute  = true;
        }
        if (!has_graphics || !has_compute) 
		{
            std::println("Device {} lacks required queue families, skipping.", 
				device.getProperties().deviceName);
            continue;
        }

        int score = scoreDevice(device);
        if (score > best_score) {
            best_score = score;
            best_device = device;
        }
    }

    if (!best_device)
    {
		throw std::runtime_error("No suitable Vulkan physical device found.");
	}
    RealGPU = best_device;
    std::println("Selected device: {} (score {})\n", 
                RealGPU.getProperties().deviceName, best_score);
}

void VulkanRHI::createLogicalDevice()
{
	if (!RealGPU)
    {
		throw std::runtime_error("Physical device not selected.");
	}

    auto queue_families = RealGPU.getQueueFamilyProperties();

    // ---- 查找图形队列族 ----
    uint32_t graphics_queue_family = UINT32_MAX;
    for (uint32_t i = 0; i < queue_families.size(); ++i) 
	{
        if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) 
		{
            graphics_queue_family = i;
            break;
        }
    }
    if (graphics_queue_family == UINT32_MAX)
    {
		throw std::runtime_error("No graphics queue family found.");
	}

    // ---- 查找计算队列族（优先独立的） ----
    uint32_t compute_queue_family = UINT32_MAX;
    for (uint32_t i = 0; i < queue_families.size(); ++i) {
        if ((queue_families[i].queueFlags & vk::QueueFlagBits::eCompute) &&
            !(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics)) 
		{
            compute_queue_family = i;
            break;
        }
    }
    if (compute_queue_family == UINT32_MAX) 
	{
        if (queue_families[graphics_queue_family].queueFlags & vk::QueueFlagBits::eCompute)
        {
			compute_queue_family = graphics_queue_family;
		}
        else
        {
			throw std::runtime_error("No compute-capable queue family found.");
		}
    }

    // ---- 准备队列创建信息 ----
    std::vector<vk::DeviceQueueCreateInfo> queue_createInfos;
    float queue_priority = 1.0f;
    std::set<uint32_t> unique_queue_families = { graphics_queue_family, compute_queue_family };
    for (uint32_t family : unique_queue_families) 
	{
        vk::DeviceQueueCreateInfo queue_creation_info(
            vk::DeviceQueueCreateFlags(),
            family,
            1,                // 每个族申请一个队列
            &queue_priority
        );
        queue_createInfos.push_back(queue_creation_info);
    }

    // ---- 启用特性(与物理设备检查保持一致) ----
    vk::PhysicalDeviceFeatures enabled_features{};
    enabled_features.samplerAnisotropy = VK_TRUE;
    enabled_features.fillModeNonSolid  = VK_TRUE;
    enabled_features.geometryShader    = VK_TRUE;

    // ---- 启用扩展(与之前检查对应) ----
    std::vector<const char*> enabledExtensions;
    enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    vk::DeviceCreateInfo device_creation_info(
        vk::DeviceCreateFlags(),
        static_cast<uint32_t>(queue_createInfos.size()),
        queue_createInfos.data(),
#ifndef NDEBUG
        1, 
		&validation_layer,
#else
		0,
		nullptr,
#endif
        static_cast<uint32_t>(enabledExtensions.size()),
        enabledExtensions.empty() ? nullptr : enabledExtensions.data(),
        &enabled_features
    );

    LogicalDevice = RealGPU.createDeviceUnique(device_creation_info);
}

std::shared_ptr<RDevice> VulkanRHI::createDevice()
{
	if (!LogicalDevice)
	{
		throw std::runtime_error("Logical device not created.");
	}
	return std::make_shared<VulkanDevice>(RealGPU, LogicalDevice);
}

} // namespace rhi