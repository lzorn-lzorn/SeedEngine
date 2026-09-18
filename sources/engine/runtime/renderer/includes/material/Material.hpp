#pragma once

#include <array>
#include <cstddef>
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

class MaterialInstanceBase
{
public:
    virtual ~MaterialInstanceBase() = default;

    MaterialInstanceBase(const MaterialInstanceBase&) = delete;
    MaterialInstanceBase& operator=(const MaterialInstanceBase&) = delete;

    // ---- 参数写入（非虚，统一实现）----
    void setFloat(std::string_view Name, float V) noexcept
    {
        writeParameter(Name, EParameterType::Float, &V, sizeof(V));
    }

    void setInt(std::string_view Name, int32_t V) noexcept
    {
        writeParameter(Name, EParameterType::Int, &V, sizeof(V));
    }

    void setUInt(std::string_view Name, uint32_t V) noexcept
    {
        writeParameter(Name, EParameterType::UInt, &V, sizeof(V));
    }

    void setBool(std::string_view Name, bool V) noexcept
    {
        uint32_t I = V ? 1u : 0u;
        writeParameter(Name, EParameterType::Bool, &I, sizeof(I));
    }

    void setVec2(std::string_view Name, float X, float Y) noexcept
    {
        float V[2] = { X, Y };
        writeParameter(Name, EParameterType::Vec2, V, sizeof(V));
    }

    void setVec3(std::string_view Name, float X, float Y, float Z) noexcept
    {
        float V[3] = { X, Y, Z };
        writeParameter(Name, EParameterType::Vec3, V, sizeof(V));
    }

    void setVec4(std::string_view Name, const std::array<float, 4>& V) noexcept
    {
        writeParameter(Name, EParameterType::Vec4, V.data(), sizeof(float) * 4);
    }

    void setVec4(std::string_view Name, float X, float Y, float Z, float W) noexcept
    {
        float V[4] = { X, Y, Z, W };
        writeParameter(Name, EParameterType::Vec4, V, sizeof(V));
    }

    void setMat4(std::string_view Name, const float M[16]) noexcept
    {
        writeParameter(Name, EParameterType::Mat4, M, sizeof(float) * 16);
    }

    void writeRaw(std::string_view Name, std::span<const std::byte> Data) noexcept
    {
        auto* P = Template->findParameter(Name);
        if (!P) return;
        if (P->Offset + Data.size() > UniformData.size()) return;
        std::memcpy(UniformData.data() + P->Offset, Data.data(), Data.size());
        IsUniformDirty = true;
    }

    // ---- 纹理绑定 ----
    void setTexture(std::string_view Name, std::shared_ptr<rhi::RImageView> View)
    {
        auto* Desc = Template->findTexture(Name);
        if (!Desc) return;
        if (isCombinedType(Desc->Type))
        {
            // 组合纹理：更新已有 View，保留 Sampler
            auto& Binding = TextureBindings[Desc->Binding];
            if (auto* CIS = std::get_if<rhi::CombinedImageSamplerBinding>(&Binding))
            {
                CIS->View = std::move(View);
            }
            else
            {
                Binding = rhi::CombinedImageSamplerBinding{
                    .View = std::move(View),
                    .Sampler = nullptr,
                    .Layout = rhi::EDescriptorImageLayout::ShaderReadOnly
                };
            }
        }
        else
        {
            TextureBindings[Desc->Binding] = rhi::TextureBinding{
                .View = std::move(View),
                .Layout = rhi::EDescriptorImageLayout::ShaderReadOnly
            };
        }
        IsTextureDirty = true;
    }

    void setSampler(std::string_view Name, std::shared_ptr<rhi::RSampler> Sampler)
    {
        auto* Desc = Template->findTexture(Name);
        if (!Desc) return;
        if (isCombinedType(Desc->Type))
        {
            auto& Binding = TextureBindings[Desc->Binding];
            if (auto* CIS = std::get_if<rhi::CombinedImageSamplerBinding>(&Binding))
            {
                CIS->Sampler = std::move(Sampler);
            }
            else
            {
                Binding = rhi::CombinedImageSamplerBinding{
                    .View = nullptr,
                    .Sampler = std::move(Sampler),
                    .Layout = rhi::EDescriptorImageLayout::ShaderReadOnly
                };
            }
        }
        else
        {
            TextureBindings[Desc->Binding] = rhi::SamplerBinding{
                .Sampler = std::move(Sampler)
            };
        }
        IsTextureDirty = true;
    }

    void setCombinedImageSampler(
        std::string_view Name,
        std::shared_ptr<rhi::RImageView> View,
        std::shared_ptr<rhi::RSampler>   Sampler)
    {
        auto* Desc = Template->findTexture(Name);
        if (!Desc) return;
        TextureBindings[Desc->Binding] = rhi::CombinedImageSamplerBinding{
            .View = std::move(View),
            .Sampler = std::move(Sampler),
            .Layout = rhi::EDescriptorImageLayout::ShaderReadOnly
        };
        IsTextureDirty = true;
    }

    void setAccelerationStructure(
        std::string_view Name,
        std::shared_ptr<rhi::RAccelerationStructure> AS)
    {
        auto* Desc = Template->findTexture(Name);
        if (!Desc) return;
        TextureBindings[Desc->Binding] = rhi::AccelerationStructureBinding{
            .AccelerationStructure = std::move(AS)
        };
        IsTextureDirty = true;
    }

    void commit(rhi::RDevice& Device, const std::shared_ptr<rhi::RBuffer>& DynamicUBO, uint32_t UBOOffset)
    {
        const uint32_t BlockSize = Template->getUniformBlockSize();

        // 写入 Uniform 
        if(BlockSize > 0 && IsUniformDirty)
        {
            void* Mapped = DynamicUBO->map(UBOOffset, BlockSize);
            std::memcpy(Mapped, UniformData.data(), BlockSize);
            DynamicUBO->unmap();
            DynamicUBO->flush(UBOOffset, BlockSize);
            UniformData.clear();
            IsUniformDirty = false;
        }

        // 重建 BindGroup, 如果纹理变化或首次时重建
        if (!BindGroup || IsTextureDirty)
        {
                        rhi::BindGroupDescriptor Desc;
            Desc.Layout = Template->getMaterialSetLayout();

            // binding 0 = 动态 UBO(实际 offset 通过 dynamicOffset 传入)
            if (BlockSize > 0)
            {
                Desc.Entries.push_back({
                    .Binding      = 0,
                    .ArrayElement = 0,
                    .Resource     = rhi::BufferBinding{
                        .Buffer = DynamicUBO,
                        .Offset = 0,
                        .Size   = BlockSize
                    }
                });
            }

            // 纹理绑定
            for (const auto& [Binding, Resource] : TextureBindings)
            {
                Desc.Entries.push_back({
                    .Binding      = Binding,
                    .ArrayElement = 0,
                    .Resource     = Resource
                });
            }

            Desc.DebugName = Template->getName() + ".BindGroup";
            BindGroup = Device.createBindGroup(Desc);
            IsTextureDirty = false;
        }
    }

    [[nodiscard]] const std::shared_ptr<rhi::RBindGroup>& getBindGroup() const noexcept 
    { 
        return BindGroup; 
    }
    [[nodiscard]] const std::shared_ptr<MaterialTemplateBase>& getTemplate() const noexcept 
    { 
        return Template; 
    }

    [[nodiscard]] bool isDirty() const noexcept
    {
        return IsUniformDirty || IsTextureDirty;
    }
protected:
    explicit MaterialInstanceBase(std::shared_ptr<MaterialTemplateBase> InTemplate) 
        : Template(std::move(InTemplate))
    {
        UniformData.resize(Template->getUniformBlockSize(), std::byte{ 0 });
    }

    std::shared_ptr<MaterialTemplateBase> Template;
    std::vector<std::byte> UniformData;
    std::unordered_map<uint32_t, rhi::BindGroupResource> TextureBindings;
    std::shared_ptr<rhi::RBindGroup> BindGroup;
    bool IsUniformDirty { true };
    bool IsTextureDirty { true };

private:
    void writeParameter(std::string_view Name, EParameterType Expected, const void* Data, size_t Size) noexcept
    {
        auto* P = Template->findParameter(Name);
        if (!P || P->Type != Expected || P->Offset + Size > UniformData.size()) 
        {
            return ;
        }

        std::memcpy(UniformData.data() + P->Offset, Data, Size);
        IsUniformDirty = true;
    }

    static bool isCombinedType(EParameterType T) noexcept
    {
        return T == EParameterType::CombinedImageSampler2D
            || T == EParameterType::CombinedImageSampler3D
            || T == EParameterType::CombinedImageSamplerCube;
    }
};
} // namespace renderer