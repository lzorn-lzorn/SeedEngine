#pragma once

#include <memory>
#include <cstdint>
#include <core/wrappers/Flag.hpp>
#include <core/math/MathCommon.hpp>
#include <generic_application/window/GenericWindow.hpp>

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

/**
 * @note:
 * > _UNorm: 表示线性颜色空间, 纹理采样时不会自动进行 gamma 校正, 适合后处理或需要线性混合的场景
 * > _sRGB: 表示 sRGB 非线性空间, GPU 在写入时自动将线性值转换为 sRGB 编码, 在读取时反向转换, 
 * >		符合标准显示器的 gamma 特性, 能避免颜色过暗或过亮
 */
enum class EFormat : uint32_t
{
	Undefined,
	// 每通道 8 位无符号归一化整数, 取值范围 [0,1], 存储时直接映射, 适用于普通颜色纹理(如漫反射贴图),无 HDR 要求的渲染目标
	RGBA8_UNorm,
	// 与 RGBA8_UNorm 相同位宽, 但纹理采样时 GPU 会自动将数据从 sRGB 色彩空间转换到线性空间, 写入时自动反向转换。适合用于最终输出到显示器的交换链, 以及存储人眼感知颜色的纹理(如照片、UI)
	RGBA8_sRGB,
	// 与 RGBA8_UNorm 类似, 但通道顺序为 B,G,R,A. 这是许多平台(尤其是 Windows)交换链的默认格式，因为桌面合成器使用这种通道顺序
	BGRA8_UNorm,
	// 每通道 16 位浮点数 RGBA, 用于 HDR 中间渲染目标, 支持高动态范围
	RGBA16_Float, 
	// 每通道 32 位浮点数 RGBA, 用于高精度 HDR 渲染或科学计算
	RGBA32_Float, 
	// 16 位深度格式(无符号归一化), 通常用于深度缓冲, 精度较低
	D16_UNorm,
	// 24 位深度(无符号归一化)+ 8 位模板(无符号整数), 经典深度模板格式, 兼容性好
	D24_UNorm_S8_UInt, 
	// 32 位浮点深度格式, 提供更高精度, 常用于现代渲染管线
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
    DeviceLocal    = 1 << 0,  // 位于 GPU 显存, 访问最快
    HostVisible    = 1 << 1,  // CPU 可映射访问(必须配合 HostVisible 才能用 vkMapMemory)
    HostCoherent   = 1 << 2,  // 自动同步 CPU/GPU 缓存(免去手动 Flush/Invalidate)
    HostCached     = 1 << 3,  // CPU 缓存中保留副本(适合频繁读回的场景)
    LazilyAllocated = 1 << 4, // 惰性分配(用于深度/模板缓冲, 节省显存)
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
    Texture3D,          // 3D 纹理(整个 Depth 当作第三维, 不能单独切片)
    Cube,               // 立方体贴图(6 个面, 不可独立扩展)
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

class RBuffer;
class RImage;
class RSampler;
class RShader;
class RPipeline;
class RRenderPass;
class RCommandList;

class RTexture;

class RSwapchain
{
public:
	RSwapchain() = default;
	virtual ~RSwapchain() = default;

	virtual void resize(uint32_t width, uint32_t height) = 0;

	virtual void setFormat(EFormat) = 0;
	virtual void setPresentMode(EPresentMode) = 0;
	virtual void setGenericWindow(ui::IGenericWindow*) = 0;
	virtual void present() = 0;

};

class RDevice
{
public:
	
	RDevice() = default;
	virtual ~RDevice() = default;

	virtual RBuffer* createBuffer() = 0;
	virtual RImage* createImage() = 0;
	virtual RSampler* createSampler() = 0;
	virtual RShader* createShader() = 0;
	virtual RPipeline* createPipeline() = 0;
	virtual RRenderPass* createRenderPass() = 0;
	virtual RCommandList* createCommandList() = 0;
	virtual RSwapchain* createSwapchain() = 0;
	virtual RTexture* createTexture() = 0;

	virtual void waitIdle() = 0;
	virtual void* getNativeHandle() const = 0;

protected:
	class IRHI* OnwerRHI = nullptr; 
};

class IRHI
{
public:
	IRHI() = default;
	virtual ~IRHI() = default;
	
	virtual ESupportedBackendAPI getBackendAPI() const = 0;
	virtual std::shared_ptr<RDevice> createDevice() = 0;
};
}