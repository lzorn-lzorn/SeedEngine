#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <RHI.hpp>

namespace renderer
{

enum class EParameterType : uint8_t
{
    None,
    Float, Vec2, Vec3, Vec4, Mat3, Mat4, 
    Int, Int2, Int3, Int4,
    UInt, UInt2, UInt3, UInt4,
    Bool,
    Texture2D, Texture3D, TextureCube, Texture2DArray,
    Sampler,
    CombinedImageSampler2D,
    CombinedImageSampler3D,
    CombinedImageSamplerCube,
    AccelerationStructure,
};

[[nodiscard]] constexpr bool isUniformParam(EParameterType T) noexcept
{
    switch (T) {
        case EParameterType::Texture2D:
        case EParameterType::Texture3D:
        case EParameterType::TextureCube:
        case EParameterType::Texture2DArray:
        case EParameterType::Sampler:
        case EParameterType::CombinedImageSampler2D:
        case EParameterType::CombinedImageSampler3D:
        case EParameterType::CombinedImageSamplerCube:
        case EParameterType::AccelerationStructure:
            return false;
        default:
            return true;
    }
}

[[nodiscard]] constexpr uint32_t getParamSize(EParameterType T) noexcept
{
    switch (T) {
        case EParameterType::Float:  return 4;
        case EParameterType::Vec2:   return 8;
        case EParameterType::Vec3:   return 12;
        case EParameterType::Vec4:   return 16;
        case EParameterType::Mat3:   return 36;
        case EParameterType::Mat4:   return 64;
        case EParameterType::Int:    return 4;
        case EParameterType::Int2:   return 8;
        case EParameterType::Int3:   return 12;
        case EParameterType::Int4:   return 16;
        case EParameterType::UInt:   return 4;
        case EParameterType::UInt2:  return 8;
        case EParameterType::UInt3:  return 12;
        case EParameterType::UInt4:  return 16;
        case EParameterType::Bool:   return 4;
        default: return 0;
    }
}

struct ParameterDescriptor
{
    std::string Name;
    EParameterType Type { EParameterType::None };

    // Uniform parameters
    uint32_t Offset { 0 };
    uint32_t Size { 0 };
    uint32_t ArrayCount { 1 };

    // 纹理参数
    uint32_t Binding { 0 };

    rhi::EShaderStage Visibility { 
        rhi::EShaderStage_t::Vertex | rhi::EShaderStage_t::Pixel 
    };
};

struct TextureBindingDescriptor
{
    std::string  Name;
    EParameterType Type { EParameterType::CombinedImageSampler2D };
    uint32_t     Binding { 1 };
    uint32_t     ArrayCount { 1 };
    rhi::EShaderStage Visibility { rhi::EShaderStage_t::Pixel };
};

struct UniformBlockDescriptor
{
    std::string Name;
    uint32_t    Size { 0 };
    std::vector<ParameterDescriptor> Parameters;
};

struct ShaderDescriptor
{
    std::optional<rhi::ShaderDescriptor> Vertex;
    std::optional<rhi::ShaderDescriptor> Pixel;
    std::optional<rhi::ShaderDescriptor> Compute;
    std::optional<rhi::ShaderDescriptor> RayGeneration;
    std::optional<rhi::ShaderDescriptor> Miss;
    std::optional<rhi::ShaderDescriptor> ClosestHit;
    std::optional<rhi::ShaderDescriptor> AnyHit;
    std::optional<rhi::ShaderDescriptor> Intersection;
    std::optional<rhi::ShaderDescriptor> Callable;
};

struct MaterialTemplateDescriptor
{
    std::string Name;
    std::string Category;   // "3D" / "UI" / "Post" / "Compute" / "RT"

    ShaderDescriptor Shaders;

    // Set 1：材质参数
    UniformBlockDescriptor                MaterialUniforms;
    std::vector<TextureBindingDescriptor> MaterialTextures;

    // Push constant
    uint32_t          PushConstantSize { 0 };
    rhi::EShaderStage PushConstantStages {
        rhi::EShaderStage_t::Vertex | rhi::EShaderStage_t::Pixel
    };

    // 顶点输入
    rhi::VertexInputState  VertexInput;
    rhi::EPrimitiveTopology Topology { rhi::EPrimitiveTopology::TriangleList };

    // 图形管线状态
    rhi::RasterizerState   Rasterizer;
    rhi::MultisampleState  Multisample;
    rhi::DepthStencilState DepthStencil;
    rhi::BlendState        Blend;

    // 光追管线状态
    uint32_t MaxRecursionDepth { 1 };
    std::vector<rhi::RayTracingShaderGroup> ShaderGroups;

    // 目标签名
    rhi::RenderingSignature Rendering;

    rhi::EPipelineCompileFlags CompileFlags {};
};

class MaterialTemplateBase
{
public:
    virtual ~MaterialTemplateBase() = default;

    MaterialTemplateBase(const MaterialTemplateBase&) = delete;
    MaterialTemplateBase& operator=(const MaterialTemplateBase&) = delete;

    // ---- 类型查询 ----
    [[nodiscard]] virtual const MaterialTemplateDescriptor& getDescriptor() const noexcept = 0;
    [[nodiscard]] const std::string& getName() const noexcept { return Descriptor.Name; }
    [[nodiscard]] const std::string& getCategory() const noexcept { return Descriptor.Category; }
    [[nodiscard]] const MaterialTemplateDescriptor& getDesc() const noexcept { return Descriptor; }

    // ---- RHI 资源查询 ----
    [[nodiscard]] const std::shared_ptr<rhi::RPipeline>& getPipeline() const noexcept { return Pipeline; }
    [[nodiscard]] const std::shared_ptr<rhi::RPipelineLayout>& getPipelineLayout() const noexcept { return PipelineLayout; }
    [[nodiscard]] const std::shared_ptr<rhi::RBindGroupLayout>& getMaterialSetLayout() const noexcept { return MaterialSetLayout; }

    // ---- 材质 Set 索引 ----
    [[nodiscard]] uint32_t getMaterialSetIndex() const noexcept { return MaterialSetIndex; }

    // ---- 参数查询 ----
    // TODO: 返回裸指针的方式可能有问题
    [[nodiscard]] const ParameterDescriptor* findParameter(std::string_view Name) const noexcept
    {
        auto it = ParameterLookup.find(std::string(Name));
        return it != ParameterLookup.end() ? it->second : nullptr;
    }

    [[nodiscard]] const TextureBindingDescriptor* findTexture(std::string_view Name) const noexcept
    {
        auto it = TextureLookup.find(std::string(Name));
        return it != TextureLookup.end() ? it->second : nullptr;
    }

    [[nodiscard]] uint32_t getUniformBlockSize() const noexcept { return Descriptor.MaterialUniforms.Size; }

    // ---- 生命周期 ----
    [[nodiscard]] bool isValid() const noexcept { return Pipeline != nullptr; }
protected:
    explicit MaterialTemplateBase(const MaterialTemplateDescriptor& InDescriptor) : Descriptor(std::move(InDescriptor)) {}

    void buildMaterialSetLayout(rhi::RDevice& Device)
    {
        std::vector<rhi::BindGroupLayoutEntry> Entries;

        // binding 0: 材质参数 UBO
        if (Descriptor.MaterialUniforms.Size > 0)
        {
            Entries.push_back({
                .Binding = 0,
                .Type = rhi::EDescriptorType::UniformBuffer,
                .ArrayCount = 1,
                .Visibility = rhi::EShaderStage_t::Vertex | rhi::EShaderStage_t::Pixel,
                .Flags = rhi::EDescriptorBindingFlags(rhi::EDescriptorBindingFlag_t::DynamicOffset)
            });
        }

        // binding 1..N = 纹理 / 采样器
        for (const auto& Tex : Descriptor.MaterialTextures)
        {
            Entries.push_back({
                .Binding = Tex.Binding,
                .Type = toDescriptorType(Tex.Type),
                .ArrayCount = Tex.ArrayCount,
                .Visibility = Tex.Visibility,
                .Flags = {}
            });
        }

        MaterialSetLayout = Device.createBindGroupLayout({ .Entries = std::move(Entries) });
    }

    // 构建 PipelineLayout（Set 0 / Set 1 / Set 2 / Push Constant）
    void buildPipelineLayout(
        rhi::RDevice& Device,
        const std::shared_ptr<rhi::RBindGroupLayout>& FrameLayout,
        std::span<const std::shared_ptr<rhi::RBindGroupLayout>> ExtraLayouts,
        uint32_t MaterialSetIndex
    ){
        this->MaterialSetIndex = MaterialSetIndex;

        std::vector<std::shared_ptr<rhi::RBindGroupLayout>> Layouts;

        // Set 0
        Layouts.push_back(FrameLayout);

        // Set 1：材质
        Layouts.push_back(MaterialSetLayout);

        // Set 2..N
        for (const auto& L : ExtraLayouts) Layouts.push_back(L);

        std::vector<rhi::PushConstantRange> PushRanges;
        if (Descriptor.PushConstantSize > 0)
        {
            PushRanges.push_back({
                .Stages = Descriptor.PushConstantStages,
                .Offset = 0,
                .Size   = Descriptor.PushConstantSize
            });
        }

        PipelineLayout = Device.createPipelineLayout({
            .BindGroupLayouts = std::move(Layouts),
            .PushConstantRanges = std::move(PushRanges),
            .DebugName = Descriptor.Name + ".PipelineLayout"
        });
    }

    // 构建参数查找表
    void buildParameterLookup()
    {
        for (const auto& P : Descriptor.MaterialUniforms.Parameters)
        {
            ParameterLookup[P.Name] = &P;
        }
        for (const auto& P : Descriptor.MaterialTextures)
        {
            TextureLookup[P.Name] = &P;
        }
    }

    // 创建 Shader
    [[nodiscard]] std::shared_ptr<rhi::RShader> createShader(
        rhi::RDevice& Device,
        const rhi::ShaderDescriptor& Desc)
    {
        return Device.createShader(Desc);
    }

    // 创建 Pipeline(派生类实现)
    virtual void createPipeline(rhi::RDevice& Device) = 0;

protected:
    MaterialTemplateDescriptor Descriptor;

    // TODO: 可能要变成 Handle 的形式
    std::shared_ptr<rhi::RBindGroupLayout> MaterialSetLayout;
    std::shared_ptr<rhi::RPipelineLayout> PipelineLayout;
    std::shared_ptr<rhi::RPipeline> Pipeline;

    uint32_t MaterialSetIndex { 0 };

    std::unordered_map<std::string, const ParameterDescriptor*> ParameterLookup;
    std::unordered_map<std::string, const TextureBindingDescriptor*> TextureLookup;
private:
    static rhi::EDescriptorType toDescriptorType(EParameterType T) noexcept
    {
        using DT = rhi::EDescriptorType;
        switch (T) {
            case EParameterType::Texture2D:
            case EParameterType::Texture3D:
            case EParameterType::TextureCube:
            case EParameterType::Texture2DArray:
                return DT::SampledTexture;
            case EParameterType::Sampler:
                return DT::Sampler;
            case EParameterType::CombinedImageSampler2D:
            case EParameterType::CombinedImageSampler3D:
            case EParameterType::CombinedImageSamplerCube:
                return DT::CombinedImageSampler;
            case EParameterType::AccelerationStructure:
                return DT::AccelerationStructure;
            default:
                return DT::UniformBuffer;
        }
    }
};

} // namespace renderer