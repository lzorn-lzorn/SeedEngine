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
	// 与 RGBA8_UNorm 类似, 但通道顺序为 B,G,R,A. 这是许多平台(尤其是 Windows)交换链的默认格式, 因为桌面合成器使用这种通道顺序
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
	return Format == EFormat::D16_UNorm ||
		Format == EFormat::D24_UNorm_S8_UInt ||
		Format == EFormat::D32_Float;
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
	return false;
}

inline bool isDepthStencilFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt;
}

inline bool hasDepthAspect(EFormat Format)
{
	return isDepthFormat(Format);
}

inline bool hasStencilAspect(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt;
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
	case EFormat::D16_UNorm:
		return 2;
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

enum class EImageViewDimension : uint8_t
{
	Auto,
	Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    Cube,
    CubeArray
};

enum class ESharingMode
{
	Auto,        
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

enum class ESurfaceTransform
{
	Identity,
	Rotate90,
	Rotate180,
	Rotate270,
	HorizontalMirror,
	HorizontalMirrorRotate90,
	HorizontalMirrorRotate180,
	HorizontalMirrorRotate270
};


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

// note: 交换链图像通常带有 alpha 通道(例如 VK_FORMAT_B8G8R8A8_SRGB), 
// note: 但最终显示到屏幕时, 窗口系统需要将这些图像与桌面或其他窗口进行合成
enum class  ECompositeAlpha
{
	Opaque,         // 图像视为不透明, 忽略 alpha 通道
	PreMultiplied,	// alpha 通道按预乘方式参与合成, 即 RGB 分量已经乘上了 alpha
	PostMultiplied, // alpha 通道按非预乘方式参与合成, RGB 与 alpha 独立
	Inherit         // 表示由窗口系统继承之前的 alpha 设置
};

enum class EColorSpace
{
	SRGB_Nonlinear, // sRGB 非线性空间, 适合最终输出到显示器的图像
	AdobeRGB,       // Adobe RGB 色彩空间, 提供更广的色域
	DCIP3,          // DCI-P3 色彩空间, 常用于数字电影投影
	Rec2020         // Rec. 2020 色彩空间, 用于超高清电视和 HDR 内容
};

enum class EImageAspect : uint8_t
{
	Auto,
	Color,
	Depth,
	Stencil,
	DepthStencil
};

class RBuffer;
class RImage;
class RSampler;
class RShader;
class RPipeline;
class RRenderPass;
class RCommandList;
class DeviceMemoryAllocator;
class RTexture;
class RImageView;
class DeviceMemory;
struct MemoryRequirements;


struct MemoryRequirements {
    uint64_t Size;           // 内存大小
    uint64_t Alignment;      // 对齐要求
	uint32_t MemoryTypeBits; 
};

class DeviceMemory
{
public:
	enum class EState {
		None,
		InValid, // 持有内存已经释放
		ReadOnly, // 未持有所有权, 仅读取
		Writeable,// 未持有所有权, 可读写
		OwnsMemory // 持有内存所有权, 可读写
	};

	DeviceMemory() = default;
	virtual ~DeviceMemory() = default;
	DeviceMemory(const DeviceMemory&) = delete;
	DeviceMemory& operator=(const DeviceMemory&) = delete;
	DeviceMemory(DeviceMemory&&) = default;
	DeviceMemory& operator=(DeviceMemory&&) = default;

	virtual void* map(DeviceSizeType Offset = 0, DeviceSizeType Size = 0) = 0;
	virtual void unmap() = 0;

	// note: 当内存被映射到主机地址空间(通过 map)后, CPU 和 GPU 对同一块内存的访问并不是自动同步的, 
    // note: 尤其是对于非主机一致性(non-coherent)内存
	// @breif: 确保 CPU 写入到映射内存的数据对 GPU 可见, 通常在 CPU 写入数据之后, 提交 GPU 命令读取该数据之前调用
	virtual void flush(DeviceSizeType Offset, DeviceSizeType Size) = 0;
	// @breif: 确保 GPU 写入到该内存的数据对 CPU 可见, 通常在 GPU 写入数据之后, CPU 读取该数据之前调用
	virtual void invalidate(DeviceSizeType Offset, DeviceSizeType Size) = 0;
	virtual void release() = 0;

	virtual MemoryRequirements getMemoryRequirements() const = 0;
	virtual EMemoryProperty getMemoryProperty() const = 0;

	EState getState() const { return OwnershipState; }

protected:
	EState OwnershipState { EState::None };
};

class DeviceMemoryAllocator
{
public:
	virtual ~DeviceMemoryAllocator() = default;

	virtual std::shared_ptr<DeviceMemory> allocateMemory(MemoryRequirements Requirements, EMemoryProperty Property) = 0;
	virtual void freeMemory(std::shared_ptr<DeviceMemory> Memory) = 0;
};

class RImageView
{
public:
    struct Descriptor_t
    {
        std::shared_ptr<RImage> Image;

        // Undefined 表示继承 Image 的格式。
        EFormat Format { EFormat::Undefined };

        // Auto 表示根据 Image dimension 和 layer 范围推导。
        EImageViewDimension Dimension { EImageViewDimension::Auto };

        // Auto 表示根据最终格式推导 Color/Depth/DepthStencil。
        EImageAspect Aspect { EImageAspect::Auto };

        uint32_t BaseMipLevel { 0 };
        uint32_t MipLevelCount { 1 };
        uint32_t BaseArrayLayer { 0 };
        uint32_t ArrayLayerCount { 1 };
    };

    virtual ~RImageView() = default;

    RImageView(const RImageView&) = delete;
    RImageView& operator=(const RImageView&) = delete;
    RImageView(RImageView&&) = delete;
    RImageView& operator=(RImageView&&) = delete;

    [[nodiscard]] const Descriptor_t& getDescriptor() const noexcept
    {
        return Descriptor;
    }

    [[nodiscard]] const std::shared_ptr<RImage>& getImage() const noexcept
    {
        return Descriptor.Image;
    }

    [[nodiscard]] virtual bool isValid() const noexcept = 0;
    [[nodiscard]] virtual void* getNativeHandle() const noexcept = 0;

protected:
    explicit RImageView(Descriptor_t Desc)
        : Descriptor(std::move(Desc))
    {
    }

private:
    const Descriptor_t Descriptor;
};

class RImage
{
public:
	struct Descriptor_t
	{
		EFormat Format { EFormat::Undefined };
		EImageDimension Dimension { EImageDimension::Texture2D };
		uint32_t Width { 1 };
		uint32_t Height { 1 };
		uint32_t Depth { 1 };
		uint32_t MipLevels { 1 };
		uint32_t ArrayLayers { 1 };
		ESharingMode SharingMode { ESharingMode::Exclusive };
		EMemoryProperty MemoryProperty { EMemoryProperty_t::DeviceLocal };
		EImageUsage Usage {};
		ESampleCount SampleCount { ESampleCount::Count1 };
	};

public:
	virtual ~RImage() = default;
	RImage(const RImage&) = delete;
	RImage& operator=(const RImage&) = delete;
	RImage(RImage&&) = delete;
	RImage& operator=(RImage&&) = delete;

	[[nodiscard]] const Descriptor_t& getDescriptor() const noexcept { return Descriptor; }
	[[nodiscard]] virtual bool isValid() const noexcept = 0;
	[[nodiscard]] virtual bool isMemoryBound() const noexcept = 0;
	[[nodiscard]] virtual void* getNativeHandle() const noexcept = 0;

protected:
	explicit RImage(const Descriptor_t& Desc) : Descriptor(Desc) {}

private:
	const Descriptor_t Descriptor;
};

class RSwapchain
{
public:
	struct SwapchainProperties {
		EFormat Format { EFormat::RGBA8_sRGB }; 
		EColorSpace ColorSpace {EColorSpace::SRGB_Nonlinear};
		EPresentMode PresentMode {EPresentMode::Mailbox};
		ESurfaceTransform PreTransform {ESurfaceTransform::Identity}; // 表面变换(如旋转 90 度、水平翻转)
		ECompositeAlpha CompositeAlpha {ECompositeAlpha::Opaque}; // 与窗口系统合成的 alpha 通道处理方式
		EImageUsage ImageUsage;
		uint32_t ImageCount;
		ESharingMode ImageSharingMode {ESharingMode::Auto };
		bool Clipped {true};
		RSwapchain* OldSwapchain {nullptr}; // 重建交换链时, 用于传递旧的交换链以复用资源
	};

public:
	RSwapchain() = default;
	virtual ~RSwapchain() = default;
	
	virtual void present() = 0;
	virtual void resize(uint32_t width, uint32_t height) = 0;

	RSwapchain& setFormat(EFormat Format)
	{
		Properties.Format = Format;
		return *this;
	}

	RSwapchain& setPresentMode(EPresentMode PresentMode)
	{
		Properties.PresentMode = PresentMode;
		return *this;
	}

	RSwapchain& setEColorSpace(EColorSpace ColorSpace)
	{
		Properties.ColorSpace = ColorSpace;
		return *this;
	}

	RSwapchain& setESurfaceTransform(ESurfaceTransform PreTransform)
	{
		Properties.PreTransform = PreTransform;
		return *this;
	}

	RSwapchain& setECompositeAlpha(ECompositeAlpha CompositeAlpha)
	{
		Properties.CompositeAlpha = CompositeAlpha;
		return *this;
	}

	RSwapchain& setEImageUsage(EImageUsage ImageUsage)
	{
		Properties.ImageUsage = ImageUsage;
		return *this;
	}

	RSwapchain& setuint32_t(uint32_t ImageCount)
	{
		Properties.ImageCount = ImageCount;
		return *this;
	}

	RSwapchain& setESharingMode(ESharingMode ImageSharingMode)
	{
		Properties.ImageSharingMode = ImageSharingMode;
		return *this;
	}

	RSwapchain& setClipped(bool Clipped)
	{
		Properties.Clipped = Clipped;
		return *this;
	}

	RSwapchain& setGenericWindow(ui::IGenericWindow* InNativeWindow)
	{
		NativeWindow = InNativeWindow;
		return *this;
	}

	ui::IGenericWindow* getNativeWindow() const { return NativeWindow; }
	SwapchainProperties getProperties() const { return Properties; }
	int32_t getWidth() const { return NativeWindow->getWidth(); }
	int32_t getHeight() const { return NativeWindow->getHeight(); }
private:
	ui::IGenericWindow* NativeWindow;
	SwapchainProperties Properties;

};

class RDevice
{
public:
	
	RDevice() = default;
	virtual ~RDevice() = default;

	virtual RBuffer* createBuffer() = 0;
	virtual RImage* createImage() = 0;
    virtual std::shared_ptr<RImageView> createImageView(const RImageView::Descriptor_t& Desc) = 0;
	virtual RSampler* createSampler() = 0;
	virtual RShader* createShader() = 0;
	virtual RPipeline* createPipeline() = 0;
	virtual RRenderPass* createRenderPass() = 0;
	virtual RCommandList* createCommandList() = 0;
	virtual RSwapchain* createSwapchain() = 0;
	virtual RTexture* createTexture() = 0;
	virtual std::shared_ptr<DeviceMemory> allocateMemory(
		MemoryRequirements Requirements,
		EMemoryProperty Property) = 0;
	virtual void freeMemory(std::shared_ptr<DeviceMemory> Memory) = 0;

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
	
	virtual void initialize(const ui::GenericWindowPointer& Window) = 0;
	virtual bool isInitialized() const noexcept = 0;
	virtual ESupportedBackendAPI getBackendAPI() const = 0;
	virtual std::shared_ptr<RDevice> createDevice() = 0;
};
}