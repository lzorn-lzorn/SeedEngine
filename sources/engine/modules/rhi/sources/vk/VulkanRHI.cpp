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

vk::Format toVkFormat(EFormat Fomat)
{
	switch (Fomat)
	{
	case EFormat::RGBA8_UNorm: return vk::Format::eR8G8B8A8Unorm;
	case EFormat::RGBA8_sRGB: return vk::Format::eR8G8B8A8Srgb;
	case EFormat::BGRA8_UNorm: return vk::Format::eB8G8R8A8Unorm;
	case EFormat::RGBA16_Float: return vk::Format::eR16G16B16A16Sfloat;
	case EFormat::RGBA32_Float: return vk::Format::eR32G32B32A32Sfloat;
	case EFormat::D24_UNorm_S8_UInt: return vk::Format::eD24UnormS8Uint;
	case EFormat::D32_Float: return vk::Format::eD32Sfloat;
	case EFormat::Undefined:
	default:
		return vk::Format::eUndefined;
	}
}

vk::IndexType toVkIndexType(EIndexFormat Fomat)
{
	switch (Fomat)
	{
	case EIndexFormat::UInt16: return vk::IndexType::eUint16;
	case EIndexFormat::UInt32: return vk::IndexType::eUint32;
	case EIndexFormat::None:
	default:
	#ifdef VK_INDEX_TYPE_NONE_KHR
		return vk::IndexType::eNoneKHR;
	#else
		return vk::IndexType::eUint32;
	#endif
	}
}

vk::ShaderStageFlags toVkShaderStageFlags(EShaderStage Stage)
{
	vk::ShaderStageFlags result = vk::ShaderStageFlags();
	if (Stage.has(EShaderStage_t::Vertex)) result |= vk::ShaderStageFlags::BitsType::eVertex;
	if (Stage.has(EShaderStage_t::Pixel)) result |= vk::ShaderStageFlags::BitsType::eFragment;
	if (Stage.has(EShaderStage_t::Compute)) result |= vk::ShaderStageFlags::BitsType::eCompute;
	if (Stage.has(EShaderStage_t::Geometry)) result |= vk::ShaderStageFlags::BitsType::eGeometry;
#ifdef VK_SHADER_STAGE_MESH_BIT_EXT
	if (Stage.has(EShaderStage_t::Mesh)) result |= vk::ShaderStageFlags::BitsType::eMesh;
#endif
#ifdef VK_SHADER_STAGE_TASK_BIT_EXT
	if (Stage.has(EShaderStage_t::Amplification)) result |= vk::ShaderStageFlags::BitsType::eTask;
#endif
	return result;
}

vk::BufferUsageFlags toVkBufferUsage(EBufferUsage Usage)
{
	vk::BufferUsageFlags flags = vk::BufferUsageFlags();
	if (Usage.has(EBufferUsage_t::Vertex)) flags |= vk::BufferUsageFlags::BitsType::eVertexBuffer;
	if (Usage.has(EBufferUsage_t::Index)) flags |= vk::BufferUsageFlags::BitsType::eIndexBuffer;
	if (Usage.has(EBufferUsage_t::Uniform)) flags |= vk::BufferUsageFlags::BitsType::eUniformBuffer;
	if (Usage.has(EBufferUsage_t::Storage)) flags |= vk::BufferUsageFlags::BitsType::eStorageBuffer;
	if (Usage.has(EBufferUsage_t::Indirect)) flags |= vk::BufferUsageFlags::BitsType::eIndirectBuffer;
	if (Usage.has(EBufferUsage_t::TransferSrc)) flags |= vk::BufferUsageFlags::BitsType::eTransferSrc;
	if (Usage.has(EBufferUsage_t::TransferDst)) flags |= vk::BufferUsageFlags::BitsType::eTransferDst;
	return flags;
}

vk::ImageUsageFlags toVkImageUsage(EImageUsage Usage)
{
	vk::ImageUsageFlags flags = vk::ImageUsageFlags();
	if (Usage.has(EImageUsage_t::Sampled)) flags |= vk::ImageUsageFlags::BitsType::eSampled;
	if (Usage.has(EImageUsage_t::Storage)) flags |= vk::ImageUsageFlags::BitsType::eStorage;
	if (Usage.has(EImageUsage_t::Target)) flags |= vk::ImageUsageFlags::BitsType::eColorAttachment;
	if (Usage.has(EImageUsage_t::DepthStencil)) flags |= vk::ImageUsageFlags::BitsType::eDepthStencilAttachment;
	if (Usage.has(EImageUsage_t::TransferSrc)) flags |= vk::ImageUsageFlags::BitsType::eTransferSrc;
	if (Usage.has(EImageUsage_t::TransferDst)) flags |= vk::ImageUsageFlags::BitsType::eTransferDst;
	return flags;
}

vk::PrimitiveTopology toVkPrimitiveTopology(EPrimitiveTopology Topology)
{
	switch (Topology)
	{
	case EPrimitiveTopology::PointList: return vk::PrimitiveTopology::ePointList;
	case EPrimitiveTopology::LineList: return vk::PrimitiveTopology::eLineList;
	case EPrimitiveTopology::LineStrip: return vk::PrimitiveTopology::eLineStrip;
	case EPrimitiveTopology::TriangleStrip: return vk::PrimitiveTopology::eTriangleStrip;
	case EPrimitiveTopology::TriangleList:
	default:
		return vk::PrimitiveTopology::eTriangleList;
	}
}

vk::PolygonMode toVkPolygonMode(EFillMode Mode)
{
	return Mode == EFillMode::Wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
}

vk::CullModeFlags toVkCullMode(ECullMode Mode)
{
	switch (Mode)
	{
	case ECullMode::Front: return vk::CullModeFlagBits::eFront;
	case ECullMode::Back: return vk::CullModeFlagBits::eBack;
	case ECullMode::None:
	default:
		return vk::CullModeFlagBits::eNone;
	}
}

vk::CompareOp toVkCompareOp(ECompareOp Op)
{
	switch (Op)
	{
	case ECompareOp::Less: return vk::CompareOp::eLess;
	case ECompareOp::LessEqual: return vk::CompareOp::eLessOrEqual;
	case ECompareOp::Equal: return vk::CompareOp::eEqual;
	case ECompareOp::NotEqual: return vk::CompareOp::eNotEqual;
	case ECompareOp::GreaterEqual: return vk::CompareOp::eGreaterOrEqual;
	case ECompareOp::Greater: return vk::CompareOp::eGreater;
	case ECompareOp::Never: return vk::CompareOp::eNever;
	case ECompareOp::Always:
	default:
		return vk::CompareOp::eAlways;
	}
}

vk::StencilOp toVkStencilOp(EStencilOp Op)
{
	switch (Op)
	{
	case EStencilOp::Keep: return vk::StencilOp::eKeep;
	case EStencilOp::Zero: return vk::StencilOp::eZero;
	case EStencilOp::Replace: return vk::StencilOp::eReplace;
	case EStencilOp::IncrementAndClamp: return vk::StencilOp::eIncrementAndClamp;
	case EStencilOp::DecrementAndClamp: return vk::StencilOp::eDecrementAndClamp;
	case EStencilOp::Invert: return vk::StencilOp::eInvert;
	case EStencilOp::IncrementAndWrap: return vk::StencilOp::eIncrementAndWrap;
	case EStencilOp::DecrementAndWrap: return vk::StencilOp::eDecrementAndWrap;
	default:
		return vk::StencilOp::eKeep;
	}
}

vk::SampleCountFlagBits toVkSampleCount(ESampleCount count)
{
	switch (count)
	{
	case ESampleCount::Count2: return vk::SampleCountFlagBits::e2;
	case ESampleCount::Count4: return vk::SampleCountFlagBits::e4;
	case ESampleCount::Count8: return vk::SampleCountFlagBits::e8;
	case ESampleCount::Count16: return vk::SampleCountFlagBits::e16;
	case ESampleCount::Count32: return vk::SampleCountFlagBits::e32;
	case ESampleCount::Count64: return vk::SampleCountFlagBits::e64;
	case ESampleCount::Count1:
	default:
		return vk::SampleCountFlagBits::e1;
	}
}

vk::BlendFactor toVkBlendFactor(EBlendFactor factor)
{
	switch (factor)
	{
	case EBlendFactor::Zero: return vk::BlendFactor::eZero;
	case EBlendFactor::SrcAlpha: return vk::BlendFactor::eSrcAlpha;
	case EBlendFactor::OneMinusSrcAlpha: return vk::BlendFactor::eOneMinusSrcAlpha;
	case EBlendFactor::One:
	default:
		return vk::BlendFactor::eOne;
	}
}

vk::BlendOp toVkBlendOp(EBlendOp Op)
{
	switch (Op)
	{
	case EBlendOp::Subtract: return vk::BlendOp::eSubtract;
	case EBlendOp::ReverseSubtract: return vk::BlendOp::eReverseSubtract;
	case EBlendOp::Min: return vk::BlendOp::eMin;
	case EBlendOp::Max: return vk::BlendOp::eMax;
	case EBlendOp::Add:
	default:
		return vk::BlendOp::eAdd;
	}
}

vk::ImageLayout toVkImageLayout(EResourceState State)
{
	switch (State)
	{
	case EResourceState::Undefined: return vk::ImageLayout::eUndefined;
    case EResourceState::Common: return vk::ImageLayout::eGeneral;
    case EResourceState::VertexBuffer: return vk::ImageLayout::eShaderReadOnlyOptimal;
    case EResourceState::IndexBuffer: return vk::ImageLayout::eShaderReadOnlyOptimal;
	case EResourceState::UniformBuffer: return vk::ImageLayout::eShaderReadOnlyOptimal;
    case EResourceState::ConstantBuffer: return vk::ImageLayout::eShaderReadOnlyOptimal;
	case EResourceState::StorageBuffer: return vk::ImageLayout::eGeneral;
    case EResourceState::UnorderedAccess: return vk::ImageLayout::eGeneral;
    case EResourceState::DepthWrite: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case EResourceState::DepthRead: return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case EResourceState::CopySrc: return vk::ImageLayout::eTransferSrcOptimal;
    case EResourceState::CopyDst: return vk::ImageLayout::eTransferDstOptimal;
    case EResourceState::Present: return vk::ImageLayout::ePresentSrcKHR;
	case EResourceState::RenderTarget: return vk::ImageLayout::eColorAttachmentOptimal;
	case EResourceState::PixelShaderResource: return vk::ImageLayout::eShaderReadOnlyOptimal;
	case EResourceState::NonPixelShaderResource: return vk::ImageLayout::eShaderReadOnlyOptimal;
	default:
		return vk::ImageLayout::eGeneral;
	}
}

vk::PipelineStageFlags toVkPipelineStage(EResourceState State)
{
	switch (State)
	{
	case EResourceState::VertexBuffer:
	case EResourceState::IndexBuffer:
		return vk::PipelineStageFlags::BitsType::eVertexInput;
	case EResourceState::ConstantBuffer:
	case EResourceState::UniformBuffer:
	case EResourceState::StorageBuffer:
		return vk::PipelineStageFlags::BitsType::eVertexShader | vk::PipelineStageFlags::BitsType::eFragmentShader | vk::PipelineStageFlags::BitsType::eComputeShader;
	case EResourceState::UnorderedAccess:
		return vk::PipelineStageFlags::BitsType::eComputeShader;
	case EResourceState::RenderTarget:
		return vk::PipelineStageFlags::BitsType::eColorAttachmentOutput;
	case EResourceState::DepthWrite:
	case EResourceState::DepthRead:
		return vk::PipelineStageFlags::BitsType::eEarlyFragmentTests | vk::PipelineStageFlags::BitsType::eLateFragmentTests;
	case EResourceState::CopySrc:
	case EResourceState::CopyDst:
		return vk::PipelineStageFlags::BitsType::eTransfer;
	case EResourceState::Present:
		return vk::PipelineStageFlags::BitsType::eBottomOfPipe;
	case EResourceState::Undefined:
	case EResourceState::Common:
	default:
		return vk::PipelineStageFlags::BitsType::eTopOfPipe;
	}
}

vk::AccessFlags toVkAccessMask(EResourceState State)
{
	switch (State)
	{
	case EResourceState::VertexBuffer: return vk::AccessFlags::BitsType::eVertexAttributeRead;
	case EResourceState::IndexBuffer: return vk::AccessFlags::BitsType::eIndexRead;
	case EResourceState::ConstantBuffer: return vk::AccessFlags::BitsType::eUniformRead;
	case EResourceState::UniformBuffer: return vk::AccessFlags::BitsType::eShaderRead;
	case EResourceState::StorageBuffer: return vk::AccessFlags::BitsType::eShaderRead | vk::AccessFlags::BitsType::eShaderWrite;
	case EResourceState::UnorderedAccess: return vk::AccessFlags::BitsType::eShaderRead | vk::AccessFlags::BitsType::eShaderWrite;
	case EResourceState::RenderTarget: return vk::AccessFlags::BitsType::eColorAttachmentRead | vk::AccessFlags::BitsType::eColorAttachmentWrite;
	case EResourceState::DepthWrite: return vk::AccessFlags::BitsType::eDepthStencilAttachmentRead | vk::AccessFlags::BitsType::eDepthStencilAttachmentWrite;
	case EResourceState::DepthRead: return vk::AccessFlags::BitsType::eDepthStencilAttachmentRead;
	case EResourceState::CopySrc: return vk::AccessFlags::BitsType::eTransferRead;
	case EResourceState::CopyDst: return vk::AccessFlags::BitsType::eTransferWrite;
	case EResourceState::Present:
	case EResourceState::Undefined:
	case EResourceState::Common:
	default:
		return vk::AccessFlags();
	}
}

vk::SharingMode toVkSharingMode(ESharingMode Mode)
{
	return (Mode == ESharingMode::Exclusive) ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
}

vk::MemoryPropertyFlags toVkMemoryPropertyFlags(EMemoryProperty Properties)
{
    vk::MemoryPropertyFlags Flags = {};

    if (Properties.has(EMemoryProperty_t::DeviceLocal))
        Flags |= vk::MemoryPropertyFlagBits::eDeviceLocal;
    
    if (Properties.has(EMemoryProperty_t::HostVisible))
        Flags |= vk::MemoryPropertyFlagBits::eHostVisible;
    
    if (Properties.has(EMemoryProperty_t::HostCoherent))
        Flags |= vk::MemoryPropertyFlagBits::eHostCoherent;
    
    if (Properties.has(EMemoryProperty_t::HostCached))
        Flags |= vk::MemoryPropertyFlagBits::eHostCached;
    
    if (Properties.has(EMemoryProperty_t::LazilyAllocated))
        Flags |= vk::MemoryPropertyFlagBits::eLazilyAllocated;

    return Flags;
}

vk::ImageAspectFlags toVkImageAspectMask(ETextureAspect Aspect, EFormat TextureFormat)
{
	switch (Aspect)
	{
	case ETextureAspect::Color:
		return vk::ImageAspectFlags::BitsType::eColor;
	case ETextureAspect::Depth:
		return vk::ImageAspectFlags::BitsType::eDepth;
	case ETextureAspect::Stencil:
		return vk::ImageAspectFlags::BitsType::eStencil;
	case ETextureAspect::DepthStencil:
		return vk::ImageAspectFlags::BitsType::eDepth | vk::ImageAspectFlags::BitsType::eStencil;
	case ETextureAspect::Auto:
	default:
		if (isDepthStencilFormat(TextureFormat))
		{
			if (isDepthOnlyFormat(TextureFormat))
			{
				return vk::ImageAspectFlags::BitsType::eDepth;
			}
			if (isStencilOnlyFormat(TextureFormat))
			{
				return vk::ImageAspectFlags::BitsType::eStencil;
			}
			return vk::ImageAspectFlags::BitsType::eDepth | vk::ImageAspectFlags::BitsType::eStencil;
		}
		return vk::ImageAspectFlags::BitsType::eColor;
	}
}

vk::ImageType toVkImageType(EImageDimension Dimension)
{
	switch (Dimension)
	{
	case EImageDimension::Texture1D:
	case EImageDimension::Texture1DArray:
		return vk::ImageType::e1D;
	case EImageDimension::Texture3D:
		return vk::ImageType::e3D;
	case EImageDimension::Texture2D:
	case EImageDimension::Texture2DArray:
	case EImageDimension::Cube:
	case EImageDimension::CubeArray:
	default:
		return vk::ImageType::e2D;
	}
}

vk::PresentModeKHR toVkPresentMode(EPresentMode PresentMode)
{
	switch (PresentMode)
	{
	case EPresentMode::Immediate:
		return vk::PresentModeKHR::eImmediate;
		break;
	case EPresentMode::Fifo:
		return vk::PresentModeKHR::eFifo;
		break;
	case EPresentMode::FifoRelaxed:
		return vk::PresentModeKHR::eFifoRelaxed;
		break;
	case EPresentMode::Mailbox:
		return vk::PresentModeKHR::eMailbox;
		break;
	default:
		throw std::runtime_error("Unsupported present mode.");
	}
}

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