#include "VulkanPipeline.hpp"

#include "VulkanDevice.h"
#include "VulkanRHI.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace rhi
{
namespace
{
constexpr uint64_t HashOffset = 14695981039346656037ull;
constexpr uint64_t HashPrime = 1099511628211ull;

void hashBytes(uint64_t& InOutHash, const void* Data, size_t Size)
{
	const auto* bytes = static_cast<const std::byte*>(Data);
	for (size_t index = 0; index < Size; ++index)
	{
		InOutHash ^= static_cast<uint64_t>(std::to_integer<uint8_t>(bytes[index]));
		InOutHash *= HashPrime;
	}
}

template <typename ValueType>
void hashValue(uint64_t& InOutHash, const ValueType& Value)
{
	static_assert(std::is_trivially_copyable_v<ValueType>);
	hashBytes(InOutHash, &Value, sizeof(Value));
}

uint64_t calculateShaderHash(const ShaderDescriptor& Desc)
{
	uint64_t hash = HashOffset;
	hashValue(hash, Desc.Stage);
	hashBytes(hash, Desc.EntryPoint.data(), Desc.EntryPoint.size());
	if (!Desc.ByteCode.empty())
		hashBytes(hash, Desc.ByteCode.data(), Desc.ByteCode.size());
	return hash;
}

vk::ShaderStageFlagBits toVk(EShaderStage_t Stage)
{
	switch (Stage)
	{
	case EShaderStage_t::Vertex: return vk::ShaderStageFlagBits::eVertex;
	case EShaderStage_t::Pixel: return vk::ShaderStageFlagBits::eFragment;
	case EShaderStage_t::Compute: return vk::ShaderStageFlagBits::eCompute;
	case EShaderStage_t::Geometry: return vk::ShaderStageFlagBits::eGeometry;
	case EShaderStage_t::Hull: return vk::ShaderStageFlagBits::eTessellationControl;
	case EShaderStage_t::Domain: return vk::ShaderStageFlagBits::eTessellationEvaluation;
#ifdef VK_EXT_mesh_shader
	case EShaderStage_t::Mesh: return vk::ShaderStageFlagBits::eMeshEXT;
	case EShaderStage_t::Amplification:
	case EShaderStage_t::Task: return vk::ShaderStageFlagBits::eTaskEXT;
#else
	case EShaderStage_t::Mesh:
	case EShaderStage_t::Amplification:
	case EShaderStage_t::Task:
		throw std::invalid_argument("Mesh and task shaders are not available in this Vulkan build.");
#endif
	default:
		throw std::invalid_argument("Unsupported shader stage.");
	}
}

vk::DescriptorType toVk(EDescriptorType Type)
{
	switch (Type)
	{
	case EDescriptorType::UniformBuffer: return vk::DescriptorType::eUniformBuffer;
	case EDescriptorType::DynamicUniformBuffer: return vk::DescriptorType::eUniformBufferDynamic;
	case EDescriptorType::StorageBuffer:
	case EDescriptorType::ReadOnlyStorageBuffer: return vk::DescriptorType::eStorageBuffer;
	case EDescriptorType::DynamicStorageBuffer: return vk::DescriptorType::eStorageBufferDynamic;
	case EDescriptorType::Sampler: return vk::DescriptorType::eSampler;
	case EDescriptorType::SampledTexture: return vk::DescriptorType::eSampledImage;
	case EDescriptorType::StorageTexture: return vk::DescriptorType::eStorageImage;
	case EDescriptorType::CombinedImageSampler: return vk::DescriptorType::eCombinedImageSampler;
#ifdef VK_KHR_acceleration_structure
	case EDescriptorType::AccelerationStructure: return vk::DescriptorType::eAccelerationStructureKHR;
#else
	case EDescriptorType::AccelerationStructure:
		throw std::invalid_argument("Acceleration structures are not available in this Vulkan build.");
#endif
	default:
		throw std::invalid_argument("Unsupported descriptor type.");
	}
}

vk::Format toVk(EVertexFormat Format)
{
	switch (Format)
	{
	case EVertexFormat::Float1: return vk::Format::eR32Sfloat;
	case EVertexFormat::Float2: return vk::Format::eR32G32Sfloat;
	case EVertexFormat::Float3: return vk::Format::eR32G32B32Sfloat;
	case EVertexFormat::Float4: return vk::Format::eR32G32B32A32Sfloat;
	case EVertexFormat::UInt1: return vk::Format::eR32Uint;
	case EVertexFormat::UInt2: return vk::Format::eR32G32Uint;
	case EVertexFormat::UInt3: return vk::Format::eR32G32B32Uint;
	case EVertexFormat::UInt4: return vk::Format::eR32G32B32A32Uint;
	case EVertexFormat::Short2: return vk::Format::eR16G16Sint;
	case EVertexFormat::Short4: return vk::Format::eR16G16B16A16Sint;
	default: throw std::invalid_argument("Unsupported vertex format.");
	}
}

vk::FrontFace toVk(EFrontFace Face)
{
	return Face == EFrontFace::Clockwise
		? vk::FrontFace::eClockwise
		: vk::FrontFace::eCounterClockwise;
}

vk::VertexInputRate toVk(EVertexInputRate Rate)
{
	return Rate == EVertexInputRate::PerInstance
		? vk::VertexInputRate::eInstance
		: vk::VertexInputRate::eVertex;
}

vk::ColorComponentFlags toVk(EColorWriteMask Mask)
{
	vk::ColorComponentFlags result;
	if (Mask.has(EColorWriteMask_t::R)) result |= vk::ColorComponentFlagBits::eR;
	if (Mask.has(EColorWriteMask_t::G)) result |= vk::ColorComponentFlagBits::eG;
	if (Mask.has(EColorWriteMask_t::B)) result |= vk::ColorComponentFlagBits::eB;
	if (Mask.has(EColorWriteMask_t::A)) result |= vk::ColorComponentFlagBits::eA;
	return result;
}

vk::StencilOpState toVk(const StencilFaceState& State)
{
	return vk::StencilOpState(
		toVk(State.FailOp),
		toVk(State.PassOp),
		toVk(State.DepthFailOp),
		toVk(State.CompareOp),
		State.CompareMask,
		State.WriteMask,
		State.Reference);
}

vk::DynamicState toVk(EDynamicState_t State)
{
	switch (State)
	{
	case EDynamicState_t::Viewport: return vk::DynamicState::eViewport;
	case EDynamicState_t::Scissor: return vk::DynamicState::eScissor;
	case EDynamicState_t::BlendConstants: return vk::DynamicState::eBlendConstants;
	case EDynamicState_t::StencilReference: return vk::DynamicState::eStencilReference;
	case EDynamicState_t::DepthBias: return vk::DynamicState::eDepthBias;
	case EDynamicState_t::LineWidth: return vk::DynamicState::eLineWidth;
	default:
		throw std::invalid_argument(
			"The requested extended dynamic state is not enabled by this Vulkan backend.");
	}
}

struct StageStorage
{
	std::vector<vk::SpecializationMapEntry> MapEntries;
	std::vector<std::byte> Data;
	vk::SpecializationInfo SpecializationInfo;
	vk::PipelineShaderStageCreateInfo CreateInfo;
};

StageStorage createStageStorage(
	const PipelineShaderStage& Stage,
	EShaderStage_t ExpectedStage,
	VulkanDevice& Device)
{
	auto shader = std::dynamic_pointer_cast<VulkanShader>(Stage.Shader);
	if (!shader || &shader->getDevice() != &Device)
		throw std::invalid_argument("Pipeline shader belongs to another backend or device.");
	if (shader->getStage() != ExpectedStage)
		throw std::invalid_argument("Pipeline shader stage does not match its descriptor slot.");

	StageStorage result;
	std::unordered_set<uint32_t> ids;
	for (const auto& constant : Stage.SpecializationConstants)
	{
		if (constant.Data.empty() || !ids.emplace(constant.Id).second)
			throw std::invalid_argument("Specialization constants require unique ids and non-empty data.");
		const uint32_t offset = static_cast<uint32_t>(result.Data.size());
		result.Data.insert(result.Data.end(), constant.Data.begin(), constant.Data.end());
		result.MapEntries.emplace_back(constant.Id, offset, constant.Data.size());
	}
	result.SpecializationInfo = vk::SpecializationInfo(
		static_cast<uint32_t>(result.MapEntries.size()),
		result.MapEntries.data(),
		result.Data.size(),
		result.Data.data());
	result.CreateInfo = vk::PipelineShaderStageCreateInfo(
		{},
		toVk(ExpectedStage),
		shader->getVkShaderModule(),
		shader->getEntryPoint().c_str(),
		Stage.SpecializationConstants.empty() ? nullptr : &result.SpecializationInfo);
	return result;
}

void validateRenderingSignature(const RenderingSignature& Signature, const DeviceLimits& Limits)
{
	if (Signature.ColorAttachmentCount > MaxColorAttachments ||
		Signature.ColorAttachmentCount > Limits.MaxColorAttachments)
	{
		throw std::invalid_argument("Pipeline rendering signature has too many color attachments.");
	}
	for (uint32_t index = 0; index < MaxColorAttachments; ++index)
	{
		const bool used = index < Signature.ColorAttachmentCount;
		if (used == (Signature.ColorFormats[index] == EFormat::Undefined))
			throw std::invalid_argument("Pipeline color formats are not normalized.");
		if (used && isDepthFormat(Signature.ColorFormats[index]))
			throw std::invalid_argument("Pipeline color attachment uses a depth/stencil format.");
	}
	if (Signature.DepthFormat != EFormat::Undefined && !hasDepthAspect(Signature.DepthFormat))
		throw std::invalid_argument("Pipeline depth format has no depth aspect.");
	if (Signature.StencilFormat != EFormat::Undefined && !hasStencilAspect(Signature.StencilFormat))
		throw std::invalid_argument("Pipeline stencil format has no stencil aspect.");
}

VulkanPipelineLayout& requireLayout(
	const std::shared_ptr<RPipelineLayout>& Layout,
	VulkanDevice& Device)
{
	auto* result = dynamic_cast<VulkanPipelineLayout*>(Layout.get());
	if (!result || &result->getDevice() != &Device || !result->isValid())
		throw std::invalid_argument("Pipeline layout belongs to another backend or device.");
	return *result;
}

VulkanPipelineCache* getCache(
	const std::shared_ptr<RPipelineCache>& Cache,
	VulkanDevice& Device)
{
	if (!Cache)
		return nullptr;
	auto* result = dynamic_cast<VulkanPipelineCache*>(Cache.get());
	if (!result || &result->getDevice() != &Device || !result->isValid())
		throw std::invalid_argument("Pipeline cache belongs to another backend or device.");
	return result;
}

vk::PipelineCreateFlags getPipelineFlags(EPipelineCompileFlags Flags)
{
	vk::PipelineCreateFlags result;
#ifdef VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT
	if (Flags.has(EPipelineCompileFlag_t::FailIfCompileNeeded))
		result |= vk::PipelineCreateFlagBits::eFailOnPipelineCompileRequired;
#else
	if (Flags.has(EPipelineCompileFlag_t::FailIfCompileNeeded))
		throw std::invalid_argument("FailIfCompileNeeded is unavailable in this Vulkan build.");
#endif
	if (!Flags.has(EPipelineCompileFlag_t::Optimize))
		result |= vk::PipelineCreateFlagBits::eDisableOptimization;
	return result;
}

uint64_t calculateGraphicsPipelineHash(const GraphicsPipelineDescriptor& Desc)
{
	uint64_t hash = HashOffset;
	const uint64_t layout_hash = Desc.Layout->getCompatibilityHash();
	hashValue(hash, layout_hash);
	for (const PipelineShaderStage* stage : {
		&Desc.Vertex, &Desc.Pixel, &Desc.Geometry, &Desc.Hull,
		&Desc.Domain, &Desc.Task, &Desc.Mesh })
	{
		const uint64_t shader_hash = stage->Shader ? stage->Shader->getContentHash() : 0;
		hashValue(hash, shader_hash);
		auto constants = stage->SpecializationConstants;
		std::ranges::sort(constants, {}, &SpecializationConstant::Id);
		for (const auto& constant : constants)
		{
			hashValue(hash, constant.Id);
			hashBytes(hash, constant.Data.data(), constant.Data.size());
		}
	}
	if (!Desc.DynamicStates.has(EDynamicState_t::VertexInput))
	{
		auto buffers = Desc.VertexInput.Buffers;
		auto attributes = Desc.VertexInput.Attributes;
		std::ranges::sort(buffers, {}, &VertexBufferLayout::Binding);
		std::ranges::sort(attributes, {}, &VertexAttribute::Location);
		for (const auto& buffer : buffers)
		{
			hashValue(hash, buffer.Binding);
			hashValue(hash, buffer.Stride);
			hashValue(hash, buffer.InputRate);
		}
		for (const auto& attribute : attributes)
		{
			hashValue(hash, attribute.Location);
			hashValue(hash, attribute.Binding);
			hashValue(hash, attribute.Format);
			hashValue(hash, attribute.Offset);
		}
	}
	if (!Desc.DynamicStates.has(EDynamicState_t::PrimitiveTopology))
		hashValue(hash, Desc.InputAssembly.Topology);
	hashValue(hash, Desc.InputAssembly.PrimitiveRestartEnable);
	hashValue(hash, Desc.InputAssembly.PatchControlPoints);
	hashValue(hash, Desc.Rasterizer.FillMode);
	if (!Desc.DynamicStates.has(EDynamicState_t::CullMode))
		hashValue(hash, Desc.Rasterizer.CullMode);
	if (!Desc.DynamicStates.has(EDynamicState_t::FrontFace))
		hashValue(hash, Desc.Rasterizer.FrontFace);
	hashValue(hash, Desc.Rasterizer.DepthClampEnable);
	hashValue(hash, Desc.Rasterizer.RasterizerDiscardEnable);
	hashValue(hash, Desc.Rasterizer.DepthBiasEnable);
	if (!Desc.DynamicStates.has(EDynamicState_t::DepthBias))
	{
		hashValue(hash, Desc.Rasterizer.DepthBiasConstantFactor);
		hashValue(hash, Desc.Rasterizer.DepthBiasClamp);
		hashValue(hash, Desc.Rasterizer.DepthBiasSlopeFactor);
	}
	if (!Desc.DynamicStates.has(EDynamicState_t::LineWidth))
		hashValue(hash, Desc.Rasterizer.LineWidth);
	hashValue(hash, Desc.Multisample.SampleShadingEnable);
	hashValue(hash, Desc.Multisample.MinSampleShading);
	hashValue(hash, Desc.Multisample.SampleMask);
	hashValue(hash, Desc.Multisample.AlphaToCoverageEnable);
	hashValue(hash, Desc.Multisample.AlphaToOneEnable);
	if (!Desc.DynamicStates.has(EDynamicState_t::DepthTestEnable))
		hashValue(hash, Desc.DepthStencil.DepthTestEnable);
	if (!Desc.DynamicStates.has(EDynamicState_t::DepthWriteEnable))
		hashValue(hash, Desc.DepthStencil.DepthWriteEnable);
	if (!Desc.DynamicStates.has(EDynamicState_t::DepthCompareOp))
		hashValue(hash, Desc.DepthStencil.DepthCompareOp);
	hashValue(hash, Desc.DepthStencil.DepthBoundsTestEnable);
	hashValue(hash, Desc.DepthStencil.MinDepthBounds);
	hashValue(hash, Desc.DepthStencil.MaxDepthBounds);
	if (!Desc.DynamicStates.has(EDynamicState_t::StencilTestEnable))
		hashValue(hash, Desc.DepthStencil.StencilTestEnable);
	for (const auto* stencil : { &Desc.DepthStencil.Front, &Desc.DepthStencil.Back })
	{
		if (!Desc.DynamicStates.has(EDynamicState_t::StencilOperations))
		{
			hashValue(hash, stencil->FailOp);
			hashValue(hash, stencil->PassOp);
			hashValue(hash, stencil->DepthFailOp);
			hashValue(hash, stencil->CompareOp);
		}
		if (!Desc.DynamicStates.has(EDynamicState_t::StencilCompareMask))
			hashValue(hash, stencil->CompareMask);
		if (!Desc.DynamicStates.has(EDynamicState_t::StencilWriteMask))
			hashValue(hash, stencil->WriteMask);
		if (!Desc.DynamicStates.has(EDynamicState_t::StencilReference))
			hashValue(hash, stencil->Reference);
	}
	for (uint32_t index = 0; index < Desc.Rendering.ColorAttachmentCount; ++index)
	{
		const auto& attachment = Desc.Blend.Attachments[index];
		hashValue(hash, attachment.BlendEnable);
		hashValue(hash, attachment.Color.SrcFactor);
		hashValue(hash, attachment.Color.DstFactor);
		hashValue(hash, attachment.Color.Operation);
		hashValue(hash, attachment.Alpha.SrcFactor);
		hashValue(hash, attachment.Alpha.DstFactor);
		hashValue(hash, attachment.Alpha.Operation);
		hashValue(hash, attachment.WriteMask.Value);
		hashValue(hash, Desc.Rendering.ColorFormats[index]);
	}
	hashValue(hash, Desc.Rendering.ColorAttachmentCount);
	hashValue(hash, Desc.Rendering.DepthFormat);
	hashValue(hash, Desc.Rendering.StencilFormat);
	hashValue(hash, Desc.Rendering.SampleCount);
	hashValue(hash, Desc.Rendering.ViewMask);
	hashValue(hash, Desc.DynamicStates.Value);
	return hash;
}

uint64_t calculateComputePipelineHash(const ComputePipelineDescriptor& Desc)
{
	uint64_t hash = HashOffset;
	const uint64_t layout_hash = Desc.Layout->getCompatibilityHash();
	const uint64_t shader_hash = Desc.Compute.Shader->getContentHash();
	hashValue(hash, layout_hash);
	hashValue(hash, shader_hash);
	auto constants = Desc.Compute.SpecializationConstants;
	std::ranges::sort(constants, {}, &SpecializationConstant::Id);
	for (const auto& constant : constants)
	{
		hashValue(hash, constant.Id);
		hashBytes(hash, constant.Data.data(), constant.Data.size());
	}
	return hash;
}

} // namespace

VulkanShader::VulkanShader(VulkanDevice& InDevice, const ShaderDescriptor& Desc)
	: Device(&InDevice)
	, Stage(Desc.Stage)
	, ContentHash(Desc.ContentHash != 0 ? Desc.ContentHash : calculateShaderHash(Desc))
	, EntryPoint(Desc.EntryPoint)
	, DebugName(Desc.DebugName)
{
	if (Desc.ByteCode.empty() || Desc.ByteCode.size() % sizeof(uint32_t) != 0)
		throw std::invalid_argument("SPIR-V bytecode must be non-empty and four-byte aligned in size.");
	if (EntryPoint.empty())
		throw std::invalid_argument("Shader entry point cannot be empty.");

	std::vector<uint32_t> words(Desc.ByteCode.size() / sizeof(uint32_t));
	std::memcpy(words.data(), Desc.ByteCode.data(), Desc.ByteCode.size());
	if (words.front() != 0x07230203u)
		throw std::invalid_argument("Shader bytecode is not a SPIR-V module.");
	ShaderModule = Device->getVkDevice().createShaderModuleUnique(
		vk::ShaderModuleCreateInfo({}, Desc.ByteCode.size(), words.data()));
}

RDevice& VulkanShader::getDevice() const noexcept { return *Device; }
void* VulkanShader::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkShaderModule>(ShaderModule.get()));
}

VulkanBindGroupLayout::VulkanBindGroupLayout(
	VulkanDevice& InDevice,
	const BindGroupLayoutDescriptor& Desc)
	: Device(&InDevice), DebugName(Desc.DebugName)
{
	std::vector<BindGroupLayoutEntry> entries = Desc.Entries;
	std::ranges::sort(entries, {}, &BindGroupLayoutEntry::Binding);
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.reserve(entries.size());
	CompatibilityHash = HashOffset;
	uint32_t previous_binding = 0;
	bool has_previous_binding = false;
	for (const auto& entry : entries)
	{
		if (entry.ArrayCount == 0 || !entry.Visibility ||
			(has_previous_binding && entry.Binding == previous_binding))
			throw std::invalid_argument("Bind group layout entries require unique bindings, visibility and array count.");
		previous_binding = entry.Binding;
		has_previous_binding = true;
		bindings.emplace_back(
			entry.Binding,
			toVk(entry.Type),
			entry.ArrayCount,
			toVk(entry.Visibility));
		hashValue(CompatibilityHash, entry.Binding);
		hashValue(CompatibilityHash, entry.Type);
		hashValue(CompatibilityHash, entry.ArrayCount);
		hashValue(CompatibilityHash, entry.Visibility.Value);
	}
	DescriptorSetLayout = Device->getVkDevice().createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo({}, bindings));
}

RDevice& VulkanBindGroupLayout::getDevice() const noexcept { return *Device; }
void* VulkanBindGroupLayout::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkDescriptorSetLayout>(DescriptorSetLayout.get()));
}

VulkanPipelineLayout::VulkanPipelineLayout(
	VulkanDevice& InDevice,
	const PipelineLayoutDescriptor& Desc)
	: Device(&InDevice)
	, DebugName(Desc.DebugName)
	, BindGroupLayouts(Desc.BindGroupLayouts)
	, PushConstantRanges(Desc.PushConstantRanges)
{
	if (BindGroupLayouts.size() > Device->getLimits().MaxBoundBindGroups)
		throw std::invalid_argument("Pipeline layout uses too many bind groups.");

	std::vector<vk::DescriptorSetLayout> layouts;
	layouts.reserve(BindGroupLayouts.size());
	CompatibilityHash = HashOffset;
	for (const auto& layout : BindGroupLayouts)
	{
		auto* vk_layout = dynamic_cast<VulkanBindGroupLayout*>(layout.get());
		if (!vk_layout || &vk_layout->getDevice() != Device || !vk_layout->isValid())
			throw std::invalid_argument("Bind group layout belongs to another backend or device.");
		layouts.emplace_back(vk_layout->getVkDescriptorSetLayout());
		const uint64_t layout_hash = vk_layout->getCompatibilityHash();
		hashValue(CompatibilityHash, layout_hash);
	}

	std::vector<PushConstantRange> ranges = Desc.PushConstantRanges;
	std::ranges::sort(ranges, {}, &PushConstantRange::Offset);
	std::vector<vk::PushConstantRange> vk_ranges;
	vk_ranges.reserve(ranges.size());
	uint32_t previous_end = 0;
	for (const auto& range : ranges)
	{
		if (!range.Stages || range.Size == 0 || range.Offset % 4 != 0 || range.Size % 4 != 0 ||
			range.Offset < previous_end || range.Size > Device->getLimits().MaxPushConstantSize ||
			range.Offset > Device->getLimits().MaxPushConstantSize - range.Size)
		{
			throw std::invalid_argument("Push constant ranges are invalid, overlapping or out of device limits.");
		}
		previous_end = range.Offset + range.Size;
		vk_ranges.emplace_back(toVk(range.Stages), range.Offset, range.Size);
		hashValue(CompatibilityHash, range.Stages.Value);
		hashValue(CompatibilityHash, range.Offset);
		hashValue(CompatibilityHash, range.Size);
	}
	PipelineLayout = Device->getVkDevice().createPipelineLayoutUnique(
		vk::PipelineLayoutCreateInfo({}, layouts, vk_ranges));
}

RDevice& VulkanPipelineLayout::getDevice() const noexcept { return *Device; }
bool VulkanPipelineLayout::supportsPushConstants(
	EShaderStage Stages,
	uint32_t Offset,
	uint32_t Size) const noexcept
{
	if (!Stages || Size == 0 || Offset > std::numeric_limits<uint32_t>::max() - Size)
		return false;
	const uint32_t end = Offset + Size;
	return std::ranges::any_of(PushConstantRanges, [&](const PushConstantRange& range)
	{
		return Offset >= range.Offset && end <= range.Offset + range.Size &&
			!(Stages & ~range.Stages);
	});
}

void* VulkanPipelineLayout::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkPipelineLayout>(PipelineLayout.get()));
}

VulkanPipelineCache::VulkanPipelineCache(
	VulkanDevice& InDevice,
	const PipelineCacheDescriptor& Desc)
	: Device(&InDevice), DebugName(Desc.DebugName)
{
	PipelineCache = Device->getVkDevice().createPipelineCacheUnique(
		vk::PipelineCacheCreateInfo({}, Desc.InitialData.size(), Desc.InitialData.data()));
}

void VulkanPipelineCache::merge(std::span<const std::shared_ptr<RPipelineCache>> Sources)
{
	std::vector<vk::PipelineCache> caches;
	std::vector<std::mutex*> mutexes { &Mutex };
	caches.reserve(Sources.size());
	mutexes.reserve(Sources.size() + 1);
	for (const auto& source : Sources)
	{
		auto* cache = dynamic_cast<VulkanPipelineCache*>(source.get());
		if (!cache || cache == this || &cache->getDevice() != Device || !cache->isValid())
			throw std::invalid_argument("Pipeline cache merge source is invalid.");
		caches.emplace_back(cache->getVkPipelineCache());
		mutexes.emplace_back(&cache->getMutex());
	}
	std::ranges::sort(mutexes);
	mutexes.erase(std::ranges::unique(mutexes).begin(), mutexes.end());
	std::vector<std::unique_lock<std::mutex>> locks;
	locks.reserve(mutexes.size());
	for (auto* mutex : mutexes)
		locks.emplace_back(*mutex);
	Device->getVkDevice().mergePipelineCaches(PipelineCache.get(), caches);
}

RDevice& VulkanPipelineCache::getDevice() const noexcept { return *Device; }
std::vector<std::byte> VulkanPipelineCache::serialize() const
{
	std::scoped_lock lock(Mutex);
	const auto bytes = Device->getVkDevice().getPipelineCacheData(PipelineCache.get());
	std::vector<std::byte> result(bytes.size());
	std::memcpy(result.data(), bytes.data(), bytes.size());
	return result;
}

void* VulkanPipelineCache::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkPipelineCache>(PipelineCache.get()));
}

VulkanPipeline::VulkanPipeline(
	VulkanDevice& InDevice,
	EPipelineType InType,
	std::shared_ptr<RPipelineLayout> InLayout,
	RenderingSignature InRendering,
	EDynamicStates InDynamicStates,
	uint64_t InCacheKey,
	std::string InDebugName,
	vk::UniquePipeline InPipeline)
	: Device(&InDevice)
	, Type(InType)
	, Layout(std::move(InLayout))
	, Rendering(InRendering)
	, DynamicStates(InDynamicStates)
	, CacheKey(InCacheKey)
	, DebugName(std::move(InDebugName))
	, Pipeline(std::move(InPipeline))
{
}

RDevice& VulkanPipeline::getDevice() const noexcept { return *Device; }
void* VulkanPipeline::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkPipeline>(Pipeline.get()));
}

std::shared_ptr<RPipeline> createVulkanGraphicsPipeline(
	VulkanDevice& Device,
	const GraphicsPipelineDescriptor& Desc)
{
	auto& layout = requireLayout(Desc.Layout, Device);
	validateRenderingSignature(Desc.Rendering, Device.getLimits());
	if (!Desc.Vertex.Shader && !Desc.Mesh.Shader)
		throw std::invalid_argument("Graphics pipeline requires a vertex or mesh shader.");
	if (!Desc.Pixel.Shader && Desc.Rendering.ColorAttachmentCount != 0)
		throw std::invalid_argument("Graphics pipeline with color attachments requires a pixel shader.");
	if (Desc.Mesh.Shader || Desc.Task.Shader)
		throw std::invalid_argument("Mesh shaders require optional Vulkan feature integration that is not enabled.");
	if ((Desc.Hull.Shader == nullptr) != (Desc.Domain.Shader == nullptr))
		throw std::invalid_argument("Hull and domain shaders must be provided together.");
	if (Desc.Geometry.Shader && !Device.getFeatures().GeometryShader)
		throw std::invalid_argument("Geometry shaders are unsupported by this device.");
	if (Desc.Hull.Shader && !Device.getFeatures().TessellationShader)
		throw std::invalid_argument("Tessellation shaders are unsupported by this device.");
	if (Desc.InputAssembly.Topology == EPrimitiveTopology::PatchList && !Desc.Hull.Shader)
		throw std::invalid_argument("Patch topology requires hull and domain shaders.");
	if (Desc.InputAssembly.Topology != EPrimitiveTopology::PatchList && Desc.Hull.Shader)
		throw std::invalid_argument("Tessellation shaders require patch topology.");
	if (Desc.Rendering.ViewMask != 0 && !Device.getFeatures().Multiview)
		throw std::invalid_argument("Multiview graphics pipeline is unsupported by this device.");
	if (!Desc.DynamicStates.has(EDynamicState_t::Viewport) ||
		!Desc.DynamicStates.has(EDynamicState_t::Scissor))
	{
		throw std::invalid_argument(
			"Viewport and scissor must be dynamic because the descriptor contains no static values.");
	}
	if ((Desc.DepthStencil.DepthTestEnable || Desc.DepthStencil.DepthWriteEnable) &&
		Desc.Rendering.DepthFormat == EFormat::Undefined)
	{
		throw std::invalid_argument("Depth testing or writing requires a depth attachment format.");
	}
	if (Desc.Rasterizer.FillMode != EFillMode::Solid && !Device.getFeatures().FillModeNonSolid)
		throw std::invalid_argument("Non-solid rasterization is unsupported by this device.");
	if (Desc.Rasterizer.DepthClampEnable && !Device.getFeatures().DepthClamp)
		throw std::invalid_argument("Depth clamp is unsupported by this device.");
	if (Desc.DepthStencil.DepthBoundsTestEnable && !Device.getFeatures().DepthBounds)
		throw std::invalid_argument("Depth bounds testing is unsupported by this device.");
	if (Desc.Multisample.SampleShadingEnable && !Device.getFeatures().SampleRateShading)
		throw std::invalid_argument("Sample-rate shading is unsupported by this device.");
	if (Desc.Multisample.AlphaToOneEnable && !Device.getFeatures().AlphaToOne)
		throw std::invalid_argument("Alpha-to-one is unsupported by this device.");
	if (!Desc.DynamicStates.has(EDynamicState_t::LineWidth) &&
		Desc.Rasterizer.LineWidth != 1.0f && !Device.getFeatures().WideLines)
	{
		throw std::invalid_argument("Wide lines are unsupported by this device.");
	}
	if (!Device.getFeatures().IndependentBlend && Desc.Rendering.ColorAttachmentCount > 1)
	{
		const auto& first = Desc.Blend.Attachments[0];
		for (uint32_t index = 1; index < Desc.Rendering.ColorAttachmentCount; ++index)
		{
			if (first != Desc.Blend.Attachments[index])
				throw std::invalid_argument("Independent attachment blending is unsupported by this device.");
		}
	}
#if !RHI_ENABLE_PIPELINE_STATISTICS
	if (Desc.Compile.Flags.has(EPipelineCompileFlag_t::CaptureStatistics))
		throw std::invalid_argument("Pipeline statistics were disabled at build time.");
#endif

	std::vector<StageStorage> stage_storage;
	stage_storage.reserve(5);
	auto add_stage = [&](const PipelineShaderStage& stage, EShaderStage_t expected)
	{
		if (stage.Shader)
			stage_storage.emplace_back(createStageStorage(stage, expected, Device));
	};
	add_stage(Desc.Vertex, EShaderStage_t::Vertex);
	add_stage(Desc.Pixel, EShaderStage_t::Pixel);
	add_stage(Desc.Geometry, EShaderStage_t::Geometry);
	add_stage(Desc.Hull, EShaderStage_t::Hull);
	add_stage(Desc.Domain, EShaderStage_t::Domain);
	std::vector<vk::PipelineShaderStageCreateInfo> stages;
	stages.reserve(stage_storage.size());
	for (auto& stage : stage_storage)
	{
		if (!stage.MapEntries.empty())
		{
			stage.SpecializationInfo.setMapEntries(stage.MapEntries)
				.setDataSize(stage.Data.size())
				.setPData(stage.Data.data());
			stage.CreateInfo.setPSpecializationInfo(&stage.SpecializationInfo);
		}
		stages.emplace_back(stage.CreateInfo);
	}

	if (Desc.VertexInput.Buffers.size() > Device.getLimits().MaxVertexInputBindings ||
		Desc.VertexInput.Attributes.size() > Device.getLimits().MaxVertexInputAttributes)
	{
		throw std::invalid_argument("Vertex input exceeds device limits.");
	}
	std::unordered_set<uint32_t> bindings;
	std::vector<vk::VertexInputBindingDescription> vertex_bindings;
	for (const auto& binding : Desc.VertexInput.Buffers)
	{
		if (binding.Stride == 0 || !bindings.emplace(binding.Binding).second)
			throw std::invalid_argument("Vertex bindings require a non-zero stride and unique binding index.");
		vertex_bindings.emplace_back(binding.Binding, binding.Stride, toVk(binding.InputRate));
	}
	std::unordered_set<uint32_t> locations;
	std::vector<vk::VertexInputAttributeDescription> attributes;
	for (const auto& attribute : Desc.VertexInput.Attributes)
	{
		if (!bindings.contains(attribute.Binding) || !locations.emplace(attribute.Location).second)
			throw std::invalid_argument("Vertex attributes require an existing binding and unique location.");
		attributes.emplace_back(attribute.Location, attribute.Binding, toVk(attribute.Format), attribute.Offset);
	}
	vk::PipelineVertexInputStateCreateInfo vertex_input({}, vertex_bindings, attributes);
	vk::PipelineInputAssemblyStateCreateInfo input_assembly(
		{}, toVk(Desc.InputAssembly.Topology), Desc.InputAssembly.PrimitiveRestartEnable);
	vk::PipelineTessellationStateCreateInfo tessellation(
		{}, Desc.InputAssembly.PatchControlPoints);
	if (Desc.Hull.Shader && Desc.InputAssembly.PatchControlPoints == 0)
		throw std::invalid_argument("Tessellation pipeline requires patch control points.");

	vk::PipelineViewportStateCreateInfo viewport({}, 1, nullptr, 1, nullptr);
	vk::PipelineRasterizationStateCreateInfo rasterizer(
		{},
		Desc.Rasterizer.DepthClampEnable,
		Desc.Rasterizer.RasterizerDiscardEnable,
		toVk(Desc.Rasterizer.FillMode),
		toVk(Desc.Rasterizer.CullMode),
		toVk(Desc.Rasterizer.FrontFace),
		Desc.Rasterizer.DepthBiasEnable,
		Desc.Rasterizer.DepthBiasConstantFactor,
		Desc.Rasterizer.DepthBiasClamp,
		Desc.Rasterizer.DepthBiasSlopeFactor,
		Desc.Rasterizer.LineWidth);
	const std::array<vk::SampleMask, 2> sample_mask {
		static_cast<vk::SampleMask>(Desc.Multisample.SampleMask),
		static_cast<vk::SampleMask>(Desc.Multisample.SampleMask >> 32)
	};
	vk::PipelineMultisampleStateCreateInfo multisample(
		{},
		toVk(Desc.Rendering.SampleCount),
		Desc.Multisample.SampleShadingEnable,
		Desc.Multisample.MinSampleShading,
		sample_mask.data(),
		Desc.Multisample.AlphaToCoverageEnable,
		Desc.Multisample.AlphaToOneEnable);
	vk::PipelineDepthStencilStateCreateInfo depth_stencil(
		{},
		Desc.DepthStencil.DepthTestEnable,
		Desc.DepthStencil.DepthWriteEnable,
		toVk(Desc.DepthStencil.DepthCompareOp),
		Desc.DepthStencil.DepthBoundsTestEnable,
		Desc.DepthStencil.StencilTestEnable,
		toVk(Desc.DepthStencil.Front),
		toVk(Desc.DepthStencil.Back),
		Desc.DepthStencil.MinDepthBounds,
		Desc.DepthStencil.MaxDepthBounds);

	std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments;
	blend_attachments.reserve(Desc.Rendering.ColorAttachmentCount);
	for (uint32_t index = 0; index < Desc.Rendering.ColorAttachmentCount; ++index)
	{
		const auto& state = Desc.Blend.Attachments[index];
		blend_attachments.emplace_back(
			state.BlendEnable,
			toVk(state.Color.SrcFactor),
			toVk(state.Color.DstFactor),
			toVk(state.Color.Operation),
			toVk(state.Alpha.SrcFactor),
			toVk(state.Alpha.DstFactor),
			toVk(state.Alpha.Operation),
			toVk(state.WriteMask));
	}
	vk::PipelineColorBlendStateCreateInfo color_blend(
		{}, false, vk::LogicOp::eCopy, blend_attachments);

	constexpr std::array dynamic_candidates {
		EDynamicState_t::Viewport,
		EDynamicState_t::Scissor,
		EDynamicState_t::BlendConstants,
		EDynamicState_t::StencilReference,
		EDynamicState_t::DepthBias,
		EDynamicState_t::LineWidth,
		EDynamicState_t::CullMode,
		EDynamicState_t::FrontFace,
		EDynamicState_t::PrimitiveTopology,
		EDynamicState_t::DepthTestEnable,
		EDynamicState_t::DepthWriteEnable,
		EDynamicState_t::DepthCompareOp,
		EDynamicState_t::StencilTestEnable,
		EDynamicState_t::StencilOperations,
		EDynamicState_t::StencilCompareMask,
		EDynamicState_t::StencilWriteMask,
		EDynamicState_t::VertexInput
	};
	std::vector<vk::DynamicState> dynamic_states;
	for (const auto state : dynamic_candidates)
		if (Desc.DynamicStates.has(state)) dynamic_states.emplace_back(toVk(state));
	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_states);

	std::array<vk::Format, MaxColorAttachments> color_formats;
	for (uint32_t index = 0; index < Desc.Rendering.ColorAttachmentCount; ++index)
		color_formats[index] = toVk(Desc.Rendering.ColorFormats[index]);
	vk::PipelineRenderingCreateInfo rendering(
		Desc.Rendering.ViewMask,
		Desc.Rendering.ColorAttachmentCount,
		color_formats.data(),
		toVk(Desc.Rendering.DepthFormat),
		toVk(Desc.Rendering.StencilFormat));

	vk::GraphicsPipelineCreateInfo create_info;
	create_info.setFlags(getPipelineFlags(Desc.Compile.Flags))
		.setStages(stages)
		.setPVertexInputState(&vertex_input)
		.setPInputAssemblyState(&input_assembly)
		.setPTessellationState(Desc.Hull.Shader ? &tessellation : nullptr)
		.setPViewportState(&viewport)
		.setPRasterizationState(&rasterizer)
		.setPMultisampleState(&multisample)
		.setPDepthStencilState(&depth_stencil)
		.setPColorBlendState(&color_blend)
		.setPDynamicState(&dynamic_state)
		.setLayout(layout.getVkPipelineLayout())
		.setPNext(&rendering);

	auto* cache = getCache(Desc.Compile.Cache, Device);
	std::unique_lock<std::mutex> cache_lock;
	if (cache) cache_lock = std::unique_lock(cache->getMutex());
	auto pipeline_result = Device.getVkDevice().createGraphicsPipelineUnique(
		cache ? cache->getVkPipelineCache() : vk::PipelineCache(), create_info);
	return std::make_shared<VulkanPipeline>(
		Device,
		EPipelineType::Graphics,
		Desc.Layout,
		Desc.Rendering,
		Desc.DynamicStates,
		calculateGraphicsPipelineHash(Desc),
		Desc.DebugName,
		std::move(pipeline_result.value));
}

std::shared_ptr<RPipeline> createVulkanComputePipeline(
	VulkanDevice& Device,
	const ComputePipelineDescriptor& Desc)
{
	auto& layout = requireLayout(Desc.Layout, Device);
	if (!Desc.Compute.Shader)
		throw std::invalid_argument("Compute pipeline requires a compute shader.");
#if !RHI_ENABLE_PIPELINE_STATISTICS
	if (Desc.Compile.Flags.has(EPipelineCompileFlag_t::CaptureStatistics))
		throw std::invalid_argument("Pipeline statistics were disabled at build time.");
#endif
	StageStorage stage = createStageStorage(Desc.Compute, EShaderStage_t::Compute, Device);
	if (!stage.MapEntries.empty())
	{
		stage.SpecializationInfo.setMapEntries(stage.MapEntries)
			.setDataSize(stage.Data.size())
			.setPData(stage.Data.data());
		stage.CreateInfo.setPSpecializationInfo(&stage.SpecializationInfo);
	}
	vk::ComputePipelineCreateInfo create_info(
		getPipelineFlags(Desc.Compile.Flags), stage.CreateInfo, layout.getVkPipelineLayout());
	auto* cache = getCache(Desc.Compile.Cache, Device);
	std::unique_lock<std::mutex> cache_lock;
	if (cache) cache_lock = std::unique_lock(cache->getMutex());
	auto pipeline_result = Device.getVkDevice().createComputePipelineUnique(
		cache ? cache->getVkPipelineCache() : vk::PipelineCache(), create_info);
	return std::make_shared<VulkanPipeline>(
		Device,
		EPipelineType::Compute,
		Desc.Layout,
		RenderingSignature {},
		EDynamicStates {},
		calculateComputePipelineHash(Desc),
		Desc.DebugName,
		std::move(pipeline_result.value));
}

} // namespace rhi
