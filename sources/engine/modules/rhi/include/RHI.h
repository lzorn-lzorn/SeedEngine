#pragma once
#include <cstdint>
#include <core/wrappers/Flag.hpp>
#include <core/math/MathCommon.hpp>

namespace rhi
{

using DeviceSizeType = uint64_t; // device memory size and offset values
// Vk: typedef uint64_t VkDeviceSize;

enum class ESupportedBackendAPI
{
	Vulkan
};
enum class EResourceType
{
	Buffer,
	Image,
	Texture,
	Sampler,
	Shader,
	Pipeline,
	DescriptorSet,
	DescriptorSetLayout,
	Fence,
	Swapchain
};

enum class EResourceState {
    Undefined,
    Common,
    VertexBuffer,
    IndexBuffer,
	UniformBuffer,
    ConstantBuffer,
	StorageBuffer,
    UnorderedAccess,
    DepthWrite,
    DepthRead,
    CopySrc,
    CopyDst,
    Present,
	RenderTarget,
	PixelShaderResource,
	NonPixelShaderResource
};

enum class EFormat : uint32_t
{
	Undefined,
	RGBA8_UNorm,
	RGBA8_sRGB,
	BGRA8_UNorm,
	RGBA16_Float,
	RGBA32_Float,
	D16_UNorm,
	D24_UNorm_S8_UInt,
	D32_Float,
	// ...
};

inline bool isDepthFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt || Format == EFormat::D32_Float;
}

inline bool isDepthOnlyFormat(EFormat Format)
{
	switch (Format) {
        case EFormat::D16_UNorm:
        case EFormat::D32_Float:
            return true;
        default:
            return false;
    }
}

inline bool isStencilOnlyFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt;
}

inline bool isDepthStencilFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt || Format == EFormat::D32_Float;
}

inline uint32_t calPixelSizeFormEFormat(EFormat Format)
{
	switch (Format)
	{
	case EFormat::RGBA8_UNorm:
	case EFormat::RGBA8_sRGB:
	case EFormat::BGRA8_UNorm:
		return 4;
	case EFormat::RGBA16_Float:
		return 8;
	case EFormat::RGBA32_Float:
		return 16;
	case EFormat::D24_UNorm_S8_UInt:
		return 4;
	case EFormat::D32_Float:
		return 4;
	default:
		return 0;
	}
}


enum class EMemoryProperty_t : uint8_t
{
    None           = 0,
    DeviceLocal    = 1 << 0,  // 位于 GPU 显存，访问最快
    HostVisible    = 1 << 1,  // CPU 可映射访问（必须配合 HostVisible 才能用 vkMapMemory）
    HostCoherent   = 1 << 2,  // 自动同步 CPU/GPU 缓存（免去手动 Flush/Invalidate）
    HostCached     = 1 << 3,  // CPU 缓存中保留副本（适合频繁读回的场景）
    LazilyAllocated = 1 << 4, // 惰性分配（用于深度/模板缓冲，节省显存）
};

using EMemoryProperty = core::wrappers::Flags<EMemoryProperty_t>;
enum class EBufferUsage_t : uint32_t
{
	None        = 0,
	Vertex      = 1 << 0,
	Index       = 1 << 1,
	Uniform     = 1 << 2,
	Storage     = 1 << 3,
	Indirect    = 1 << 4,
	TransferSrc = 1 << 5,
	TransferDst = 1 << 6,
};

using EBufferUsage = core::wrappers::Flags<EBufferUsage_t>;

enum class EBufferMapMode : uint8_t
{
	Read,
	Write,
	ReadWrite,
	WriteDiscard
};

enum class EImageUsage_t : uint32_t
{
	None         = 0,
	Sampled      = 1 << 0,
	Storage      = 1 << 1,
	Target       = 1 << 2,
	DepthStencil = 1 << 3,
	TransferSrc  = 1 << 4,
	TransferDst  = 1 << 5,
	Present      = 1 << 6,
};

using EImageUsage = core::wrappers::Flags<EImageUsage_t>;

enum class EImageDimension : uint8_t
{
    Texture1D,          // 1D 纹理
    Texture1DArray,     // 1D 纹理数组
    Texture2D,          // 2D 纹理
    Texture2DArray,     // 2D 纹理数组
    Texture3D,          // 3D 纹理(整个 Depth 当作第三维，不能单独切片)
    Cube,               // 立方体贴图(6 个面，不可独立扩展)
    CubeArray           // 立方体贴图数组(6 的整数倍面)
};

enum class ETextureAspect : uint8_t
{
	Auto,
	Color,
	Depth,
	Stencil,
	DepthStencil
};

enum class ESharingMode
{
	Exclusive,   // GPU 独占模式, 性能更高
	Concurrent   // GPU 并发模式, 允许多个队列同时访问资源, 但性能较低
};

enum class EShaderStage_t : uint32_t
{
	Vertex        = 1 << 0,
	Pixel         = 1 << 1,
	Compute       = 1 << 2,
	Geometry      = 1 << 3,
	Hull		  = 1 << 4,
	Domain		  = 1 << 5,
	Mesh	      = 1 << 6,
	Amplification = 1 << 7,
	Task		  = 1 << 8,
};

using EShaderStage = core::wrappers::Flags<EShaderStage_t>;


enum class ECommandQueueType
{
	Graphics,
	Compute,
	Copy
};

enum class EPipelineType
{
	None,
	Graphics,
	Compute
};

enum class ELoadOp
{
	Load,
	Clear,
	DontCare
};

enum class EStoreOp
{
	Store,
	DontCare
};

enum class EVertexFormat {
    Float1,
    Float2,
    Float3,
    Float4,
    UInt1,
    UInt2,
    UInt3,
    UInt4,
    Short2,
    Short4,
    // 更多...
};

enum class EIndexFormat
{
	None,
	UInt16,
	UInt32
};

enum class EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip
};

enum class EDescriptorType
{
	UniformBuffer,
	StorageBuffer,
	Sampler,
	SampledTexture,
	StorageTexture
};

enum class EBlendFactor
{
	Zero,
	One,
	SrcColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
    OneMinusDstAlpha,
    DstColor,
    OneMinusDstColor
};

enum class EBlendOp
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
};

enum class ECompareOp
{
	Less,
	LessEqual,
	Equal,
	NotEqual,
	GreaterEqual,
	Greater,
	Always,
	Never
};

enum class EStencilOp
{
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	IncrementAndWrap,
	DecrementAndWrap
};

enum class ECullMode
{
	None,
	Front,
	Back
};

enum class EFillMode
{
	Solid,
	Wireframe
};

enum class ESampleCount
{
	Count1 = 1,
	Count2 = 2,
	Count4 = 4,
	Count8 = 8,
	Count16 = 16,
	Count32 = 32,
	Count64 = 64
};

enum class EPresentMode {
    Immediate,
    Fifo,
    FifoRelaxed,
    Mailbox
};



}