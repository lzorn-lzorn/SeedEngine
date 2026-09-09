#include "VulkanCommandList.h"

#include "VulkanBindGroup.hpp"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanPipeline.hpp"
#include "VulkanRHI.h"

#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <string>

namespace rhi
{
namespace
{
struct VulkanResourceState
{
	vk::PipelineStageFlags2 Stages;
	vk::AccessFlags2 Access;
	vk::ImageLayout Layout;
};

VulkanResourceState convertResourceState(EResourceState State)
{
	using Stage = vk::PipelineStageFlagBits2;
	using Access = vk::AccessFlagBits2;

	switch (State)
	{
	case EResourceState::Undefined:
		return { Stage::eNone, Access::eNone, vk::ImageLayout::eUndefined };
	case EResourceState::Common:
		return {
			Stage::eAllCommands,
			Access::eMemoryRead | Access::eMemoryWrite,
			vk::ImageLayout::eGeneral };
	case EResourceState::UnorderedAccess:
	case EResourceState::StorageBuffer:
		return {
			Stage::eAllCommands,
			Access::eShaderRead | Access::eShaderWrite,
			vk::ImageLayout::eGeneral };
	case EResourceState::DepthWrite:
		return {
			Stage::eEarlyFragmentTests | Stage::eLateFragmentTests,
			Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite,
			vk::ImageLayout::eDepthStencilAttachmentOptimal };
	case EResourceState::DepthRead:
		return {
			Stage::eEarlyFragmentTests | Stage::eLateFragmentTests | Stage::eFragmentShader,
			Access::eDepthStencilAttachmentRead | Access::eShaderRead,
			vk::ImageLayout::eDepthStencilReadOnlyOptimal };
	case EResourceState::CopySrc:
		return { Stage::eCopy, Access::eTransferRead, vk::ImageLayout::eTransferSrcOptimal };
	case EResourceState::CopyDst:
		return { Stage::eCopy, Access::eTransferWrite, vk::ImageLayout::eTransferDstOptimal };
	case EResourceState::Present:
		return { Stage::eAllCommands, Access::eMemoryRead, vk::ImageLayout::ePresentSrcKHR };
	case EResourceState::RenderTarget:
		return {
			Stage::eColorAttachmentOutput,
			Access::eColorAttachmentRead | Access::eColorAttachmentWrite,
			vk::ImageLayout::eColorAttachmentOptimal };
	case EResourceState::PixelShaderResource:
		return { Stage::eFragmentShader, Access::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal };
	case EResourceState::NonPixelShaderResource:
		return {
			Stage::eVertexShader | Stage::eComputeShader,
			Access::eShaderRead,
			vk::ImageLayout::eShaderReadOnlyOptimal };
	case EResourceState::VertexBuffer:
	case EResourceState::IndexBuffer:
	case EResourceState::UniformBuffer:
	case EResourceState::ConstantBuffer:
		throw std::invalid_argument("A buffer-only resource state cannot be used for an image barrier.");
	}
	throw std::invalid_argument("Unsupported resource state.");
}

vk::AttachmentLoadOp convertLoadOp(ELoadOp Op)
{
	switch (Op)
	{
	case ELoadOp::Load: return vk::AttachmentLoadOp::eLoad;
	case ELoadOp::Clear: return vk::AttachmentLoadOp::eClear;
	case ELoadOp::DontCare: return vk::AttachmentLoadOp::eDontCare;
	}
	throw std::invalid_argument("Unsupported attachment load operation.");
}

vk::AttachmentStoreOp convertStoreOp(EStoreOp Op)
{
	switch (Op)
	{
	case EStoreOp::Store: return vk::AttachmentStoreOp::eStore;
	case EStoreOp::DontCare: return vk::AttachmentStoreOp::eDontCare;
	}
	throw std::invalid_argument("Unsupported attachment store operation.");
}

vk::ResolveModeFlagBits convertResolveMode(EResolveMode Mode)
{
	switch (Mode)
	{
	case EResolveMode::None: return vk::ResolveModeFlagBits::eNone;
	case EResolveMode::SampleZero: return vk::ResolveModeFlagBits::eSampleZero;
	case EResolveMode::Average: return vk::ResolveModeFlagBits::eAverage;
	case EResolveMode::Min: return vk::ResolveModeFlagBits::eMin;
	case EResolveMode::Max: return vk::ResolveModeFlagBits::eMax;
	}
	throw std::invalid_argument("Unsupported resolve mode.");
}

vk::ClearValue convertClearColor(const ClearColorValue& Value)
{
	switch (Value.Type)
	{
	case EClearColorType::Float:
		return vk::ClearValue(vk::ClearColorValue(Value.Float32));
	case EClearColorType::UInt:
		return vk::ClearValue(vk::ClearColorValue(Value.UInt32));
	case EClearColorType::SInt:
		return vk::ClearValue(vk::ClearColorValue(Value.SInt32));
	}
	throw std::invalid_argument("Unsupported clear color type.");
}

EImageAspect inferAspect(EFormat Format)
{
	if (hasDepthAspect(Format) && hasStencilAspect(Format))
		return EImageAspect::DepthStencil;
	if (hasDepthAspect(Format))
		return EImageAspect::Depth;
	if (hasStencilAspect(Format))
		return EImageAspect::Stencil;
	return EImageAspect::Color;
}

uint32_t mipExtent(uint32_t Extent, uint32_t MipLevel)
{
	return std::max(1u, Extent >> MipLevel);
}

const VulkanImageView& getVulkanView(const std::shared_ptr<RImageView>& View)
{
	if (!View || !View->isValid())
		throw std::invalid_argument("Rendering requires a valid image view.");
	const auto* result = dynamic_cast<const VulkanImageView*>(View.get());
	if (!result)
		throw std::invalid_argument("Rendering received an image view from another backend.");
	return *result;
}

const VulkanImage& getVulkanImage(const std::shared_ptr<RImage>& Image)
{
	if (!Image || !Image->isValid())
		throw std::invalid_argument("Image barrier requires a valid image.");
	const auto* result = dynamic_cast<const VulkanImage*>(Image.get());
	if (!result)
		throw std::invalid_argument("Image barrier received an image from another backend.");
	return *result;
}

void validateResolve(
	const std::shared_ptr<RImageView>& Source,
	const std::shared_ptr<RImageView>& Resolve,
	EResolveMode Mode,
	bool IsColor)
{
	if (!Resolve)
	{
		if (Mode != EResolveMode::None)
			throw std::invalid_argument("A resolve mode requires a resolve image view.");
		return;
	}
	if (Mode == EResolveMode::None)
		throw std::invalid_argument("A resolve image view requires a resolve mode.");
	if (IsColor && Mode != EResolveMode::Average)
		throw std::invalid_argument("Color attachment resolve currently requires Average mode.");

	const auto& source_image = Source->getImage()->getDescriptor();
	const auto& resolve_image = Resolve->getImage()->getDescriptor();
	if (source_image.SampleCount == ESampleCount::Count1)
		throw std::invalid_argument("A resolve source must be multisampled.");
	if (resolve_image.SampleCount != ESampleCount::Count1)
		throw std::invalid_argument("A resolve destination must have one sample.");
	if (Source->getDescriptor().Format != Resolve->getDescriptor().Format)
		throw std::invalid_argument("Resolve source and destination formats must match.");
	if (source_image.Width != resolve_image.Width || source_image.Height != resolve_image.Height)
		throw std::invalid_argument("Resolve source and destination extents must match.");
}
} // namespace

VulkanCommandList::VulkanCommandList(
	VulkanDevice& InDevice,
	uint32_t QueueFamilyIndex,
	const CommandListDescriptor& Desc)
	: Device(&InDevice), Descriptor(Desc)
{
	if (Desc.Level != ECommandListLevel::Primary)
		throw std::invalid_argument("Secondary command lists are not implemented yet.");

	CommandPool = Device->getVkDevice().createCommandPoolUnique(
		vk::CommandPoolCreateInfo(
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			QueueFamilyIndex));

	vk::CommandBufferAllocateInfo allocation_info(
		CommandPool.get(),
		vk::CommandBufferLevel::ePrimary,
		1);
	auto command_buffers = Device->getVkDevice().allocateCommandBuffersUnique(allocation_info);
	CommandBuffer = std::move(command_buffers.front());
}

void* VulkanCommandList::getNativeHandle() const noexcept
{
	return reinterpret_cast<void*>(static_cast<VkCommandBuffer>(CommandBuffer.get()));
}

void VulkanCommandList::begin()
{
	if (State != ECommandListState::Initial)
		throw std::logic_error("Command list begin requires the Initial state.");

	try
	{
		CommandBuffer->begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		State = ECommandListState::Recording;
	}
	catch (...)
	{
		State = ECommandListState::Invalid;
		throw;
	}
}

void VulkanCommandList::end()
{
	requireRecording("end");
	requireOutsideRendering("end");
	try
	{
		CommandBuffer->end();
		State = ECommandListState::Executable;
	}
	catch (...)
	{
		State = ECommandListState::Invalid;
		throw;
	}
}

void VulkanCommandList::reset()
{
	if (State == ECommandListState::Recording)
		throw std::logic_error("A recording command list cannot be reset.");
	CommandBuffer->reset();
	State = ECommandListState::Initial;
	InsideRendering = false;
	BoundGraphicsPipeline.reset();
	BoundComputePipeline.reset();
	ActiveRenderingSignature = {};
	InitializedDynamicStates = {};
	RetainedResources.clear();
}

void VulkanCommandList::imageBarriers(std::span<const ImageBarrier> Barriers)
{
	requireRecording("imageBarriers");
	requireOutsideRendering("imageBarriers");
	if (Barriers.empty())
		return;

	std::vector<vk::ImageMemoryBarrier2> vk_barriers;
	vk_barriers.reserve(Barriers.size());
	for (const auto& barrier : Barriers)
	{
		const auto& image = getVulkanImage(barrier.Image);
		const auto& desc = barrier.Image->getDescriptor();
		if (barrier.Range.MipLevelCount == 0 || barrier.Range.ArrayLayerCount == 0 ||
			barrier.Range.BaseMipLevel >= desc.MipLevels ||
			barrier.Range.MipLevelCount > desc.MipLevels - barrier.Range.BaseMipLevel ||
			barrier.Range.BaseArrayLayer >= desc.ArrayLayers ||
			barrier.Range.ArrayLayerCount > desc.ArrayLayers - barrier.Range.BaseArrayLayer)
		{
			throw std::out_of_range("Image barrier subresource range exceeds the image.");
		}

		const EImageAspect aspect = barrier.Range.Aspect == EImageAspect::Auto
			? inferAspect(desc.Format)
			: barrier.Range.Aspect;
		const auto before = convertResourceState(barrier.Before);
		const auto after = convertResourceState(barrier.After);
		vk_barriers.emplace_back(
			before.Stages,
			before.Access,
			after.Stages,
			after.Access,
			before.Layout,
			after.Layout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image.getVkImage(),
			vk::ImageSubresourceRange(
				toVk(aspect),
				barrier.Range.BaseMipLevel,
				barrier.Range.MipLevelCount,
				barrier.Range.BaseArrayLayer,
				barrier.Range.ArrayLayerCount));
		RetainedResources.emplace_back(barrier.Image);
	}

	CommandBuffer->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, vk_barriers));
}

void VulkanCommandList::beginRendering(const RenderingInfo& Info)
{
	requireRecording("beginRendering");
	requireOutsideRendering("beginRendering");
	if (Descriptor.QueueType != ECommandQueueType::Graphics)
		throw std::logic_error("Rendering requires a graphics command list.");
	validateRenderingInfo(Info);

	std::array<vk::RenderingAttachmentInfo, MaxColorAttachments> color_attachments;
	ActiveRenderingSignature = {};
	ActiveRenderingSignature.ColorAttachmentCount = static_cast<uint32_t>(Info.ColorAttachments.size());
	ActiveRenderingSignature.ViewMask = Info.ViewMask;
	bool sample_count_set = false;

	for (size_t index = 0; index < Info.ColorAttachments.size(); ++index)
	{
		const auto& source = Info.ColorAttachments[index];
		const auto& view = getVulkanView(source.View);
		const auto& view_desc = source.View->getDescriptor();
		const auto& image_desc = source.View->getImage()->getDescriptor();
		ActiveRenderingSignature.ColorFormats[index] = view_desc.Format;
		if (!sample_count_set)
		{
			ActiveRenderingSignature.SampleCount = image_desc.SampleCount;
			sample_count_set = true;
		}

		auto& target = color_attachments[index];
		target
			.setImageView(view.getVkImageView())
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setLoadOp(convertLoadOp(source.LoadOp))
			.setStoreOp(convertStoreOp(source.StoreOp))
			.setClearValue(convertClearColor(source.ClearValue));
		if (source.ResolveView)
		{
			const auto& resolve_view = getVulkanView(source.ResolveView);
			target
				.setResolveMode(convertResolveMode(source.ResolveMode))
				.setResolveImageView(resolve_view.getVkImageView())
				.setResolveImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
		}
	}

	vk::RenderingAttachmentInfo depth_attachment;
	vk::RenderingAttachmentInfo stencil_attachment;
	const vk::RenderingAttachmentInfo* depth_pointer = nullptr;
	const vk::RenderingAttachmentInfo* stencil_pointer = nullptr;

	if (Info.DepthStencil)
	{
		const auto& attachment = *Info.DepthStencil;
		const auto& view = getVulkanView(attachment.View);
		const EFormat format = attachment.View->getDescriptor().Format;
		const auto sample_count = attachment.View->getImage()->getDescriptor().SampleCount;
		if (!sample_count_set)
			ActiveRenderingSignature.SampleCount = sample_count;
		if (hasDepthAspect(format))
		{
			ActiveRenderingSignature.DepthFormat = format;
			depth_attachment
				.setImageView(view.getVkImageView())
				.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.setLoadOp(convertLoadOp(attachment.Depth.LoadOp))
				.setStoreOp(convertStoreOp(attachment.Depth.StoreOp))
				.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(
					attachment.ClearValue.Depth,
					attachment.ClearValue.Stencil)));
			depth_pointer = &depth_attachment;
		}
		if (hasStencilAspect(format))
		{
			ActiveRenderingSignature.StencilFormat = format;
			stencil_attachment
				.setImageView(view.getVkImageView())
				.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.setLoadOp(convertLoadOp(attachment.Stencil.LoadOp))
				.setStoreOp(convertStoreOp(attachment.Stencil.StoreOp))
				.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(
					attachment.ClearValue.Depth,
					attachment.ClearValue.Stencil)));
			stencil_pointer = &stencil_attachment;
		}
		if (attachment.ResolveView)
		{
			const auto& resolve_view = getVulkanView(attachment.ResolveView);
			if (depth_pointer && attachment.Depth.ResolveMode != EResolveMode::None)
				depth_attachment
					.setResolveMode(convertResolveMode(attachment.Depth.ResolveMode))
					.setResolveImageView(resolve_view.getVkImageView())
					.setResolveImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
			if (stencil_pointer && attachment.Stencil.ResolveMode != EResolveMode::None)
				stencil_attachment
					.setResolveMode(convertResolveMode(attachment.Stencil.ResolveMode))
					.setResolveImageView(resolve_view.getVkImageView())
					.setResolveImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
		}
	}

	vk::RenderingInfo rendering_info;
	rendering_info
		.setRenderArea(vk::Rect2D(
			vk::Offset2D(Info.Area.X, Info.Area.Y),
			vk::Extent2D(Info.Area.Width, Info.Area.Height)))
		.setLayerCount(Info.LayerCount)
		.setViewMask(Info.ViewMask)
		.setColorAttachmentCount(static_cast<uint32_t>(Info.ColorAttachments.size()))
		.setPColorAttachments(color_attachments.data())
		.setPDepthAttachment(depth_pointer)
		.setPStencilAttachment(stencil_pointer);

	if (BoundGraphicsPipeline)
		validateRenderingCompatibility(*BoundGraphicsPipeline);
	CommandBuffer->beginRendering(rendering_info);
	InsideRendering = true;
	retainRenderingResources(Info);
}

void VulkanCommandList::endRendering()
{
	requireRecording("endRendering");
	requireInsideRendering("endRendering");
	CommandBuffer->endRendering();
	InsideRendering = false;
	ActiveRenderingSignature = {};
}

void VulkanCommandList::setViewports(std::span<const Viewport> Viewports)
{
	requireRecording("setViewports");
	if (Viewports.empty() || Viewports.size() > Device->getLimits().MaxViewports)
		throw std::invalid_argument("Viewport count is invalid for this device.");
	std::vector<vk::Viewport> vk_viewports;
	vk_viewports.reserve(Viewports.size());
	for (const auto& viewport : Viewports)
	{
		if (viewport.Width <= 0.0f || viewport.Height <= 0.0f ||
			viewport.MinDepth < 0.0f || viewport.MaxDepth > 1.0f ||
			viewport.MinDepth > viewport.MaxDepth)
		{
			throw std::invalid_argument("Viewport dimensions or depth range are invalid.");
		}
		vk_viewports.emplace_back(
			viewport.X,
			viewport.Y,
			viewport.Width,
			viewport.Height,
			viewport.MinDepth,
			viewport.MaxDepth);
	}
	CommandBuffer->setViewport(0, vk_viewports);
	InitializedDynamicStates.set(EDynamicState_t::Viewport);
}

void VulkanCommandList::setScissors(std::span<const RenderArea> Scissors)
{
	requireRecording("setScissors");
	if (Scissors.empty() || Scissors.size() > Device->getLimits().MaxViewports)
		throw std::invalid_argument("Scissor count is invalid for this device.");
	std::vector<vk::Rect2D> vk_scissors;
	vk_scissors.reserve(Scissors.size());
	for (const auto& scissor : Scissors)
	{
		if (scissor.X < 0 || scissor.Y < 0 || scissor.Width == 0 || scissor.Height == 0)
			throw std::invalid_argument("Scissor rectangle is invalid.");
		vk_scissors.emplace_back(
			vk::Offset2D(scissor.X, scissor.Y),
			vk::Extent2D(scissor.Width, scissor.Height));
	}
	CommandBuffer->setScissor(0, vk_scissors);
	InitializedDynamicStates.set(EDynamicState_t::Scissor);
}

void VulkanCommandList::setBlendConstants(const std::array<float, 4>& Constants)
{
	requireRecording("setBlendConstants");
	CommandBuffer->setBlendConstants(Constants.data());
	InitializedDynamicStates.set(EDynamicState_t::BlendConstants);
}

void VulkanCommandList::setStencilReference(
	uint32_t FrontReference,
	uint32_t BackReference)
{
	requireRecording("setStencilReference");
	CommandBuffer->setStencilReference(vk::StencilFaceFlagBits::eFront, FrontReference);
	CommandBuffer->setStencilReference(vk::StencilFaceFlagBits::eBack, BackReference);
	InitializedDynamicStates.set(EDynamicState_t::StencilReference);
}

void VulkanCommandList::setDepthBias(float ConstantFactor, float Clamp, float SlopeFactor)
{
	requireRecording("setDepthBias");
	CommandBuffer->setDepthBias(ConstantFactor, Clamp, SlopeFactor);
	InitializedDynamicStates.set(EDynamicState_t::DepthBias);
}

void VulkanCommandList::setLineWidth(float Width)
{
	requireRecording("setLineWidth");
	if (Width <= 0.0f)
		throw std::invalid_argument("Dynamic line width must be positive.");
	if (Width != 1.0f && !Device->getFeatures().WideLines)
		throw std::invalid_argument("Wide lines are unsupported by this device.");
	CommandBuffer->setLineWidth(Width);
	InitializedDynamicStates.set(EDynamicState_t::LineWidth);
}

void VulkanCommandList::pushConstants(
	const std::shared_ptr<RPipelineLayout>& Layout,
	EShaderStage Stages,
	uint32_t Offset,
	std::span<const std::byte> Data)
{
	requireRecording("pushConstants");
	auto vk_layout = std::dynamic_pointer_cast<VulkanPipelineLayout>(Layout);
	if (!vk_layout || &vk_layout->getDevice() != Device || !vk_layout->isValid())
		throw std::invalid_argument("Push constant layout belongs to another backend or device.");
	if (!Stages || Data.empty() || Offset % 4 != 0 || Data.size() % 4 != 0 ||
		Data.size() > Device->getLimits().MaxPushConstantSize ||
		Offset > Device->getLimits().MaxPushConstantSize - Data.size())
	{
		throw std::invalid_argument("Push constant update is empty, unaligned or exceeds device limits.");
	}
	if (!vk_layout->supportsPushConstants(Stages, Offset, static_cast<uint32_t>(Data.size())))
		throw std::invalid_argument("Push constant update is not covered by the pipeline layout.");
	CommandBuffer->pushConstants(
		vk_layout->getVkPipelineLayout(),
		toVk(Stages),
		Offset,
		static_cast<uint32_t>(Data.size()),
		Data.data());
	RetainedResources.emplace_back(Layout);
}

void VulkanCommandList::bindPipeline(const std::shared_ptr<RPipeline>& Pipeline)
{
	requireRecording("bindPipeline");
	auto vk_pipeline = std::dynamic_pointer_cast<VulkanPipeline>(Pipeline);
	if (!vk_pipeline || &vk_pipeline->getDevice() != Device || !vk_pipeline->isValid())
		throw std::invalid_argument("Cannot bind an invalid pipeline.");
	const vk::Pipeline native_pipeline = vk_pipeline->getVkPipeline();
	switch (Pipeline->getType())
	{
	case EPipelineType::Graphics:
		if (InsideRendering)
			validateRenderingCompatibility(*Pipeline);
		CommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, native_pipeline);
		BoundGraphicsPipeline = Pipeline;
		break;
	case EPipelineType::Compute:
		requireOutsideRendering("bind compute pipeline");
		CommandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, native_pipeline);
		BoundComputePipeline = Pipeline;
		break;
	case EPipelineType::None:
	case EPipelineType::RayTracing:
	default:
		throw std::invalid_argument("This command list cannot bind the requested pipeline type.");
	}
	RetainedResources.emplace_back(Pipeline);
}

void VulkanCommandList::bindBindGroups(
	EPipelineType PipelineType,
	const std::shared_ptr<RPipelineLayout>& Layout,
	uint32_t FirstGroup,
	std::span<const std::shared_ptr<RBindGroup>> Groups,
	std::span<const uint32_t> DynamicOffsets)
{
	requireRecording("bindBindGroups");
	auto vk_layout = std::dynamic_pointer_cast<VulkanPipelineLayout>(Layout);
	if (!vk_layout || &vk_layout->getDevice() != Device || !vk_layout->isValid())
		throw std::invalid_argument("BindGroup pipeline layout belongs to another backend or device.");
	if (PipelineType != EPipelineType::Graphics && PipelineType != EPipelineType::Compute)
		throw std::invalid_argument("BindGroups can only be bound to graphics or compute pipelines.");
	if (PipelineType == EPipelineType::Compute)
		requireOutsideRendering("bind compute BindGroups");
	if (FirstGroup > vk_layout->getBindGroupLayoutCount() ||
		Groups.size() > vk_layout->getBindGroupLayoutCount() - FirstGroup)
		throw std::out_of_range("BindGroup range exceeds the pipeline layout.");

	std::vector<vk::DescriptorSet> descriptor_sets;
	descriptor_sets.reserve(Groups.size());
	size_t dynamic_offset_index = 0;
	for (size_t group_index = 0; group_index < Groups.size(); ++group_index)
	{
		auto group = std::dynamic_pointer_cast<VulkanBindGroup>(Groups[group_index]);
		if (!group || &group->getDevice() != Device || !group->isValid())
			throw std::invalid_argument("Cannot bind an invalid or foreign BindGroup.");
		const auto& expected_layout = vk_layout->getBindGroupLayout(
			FirstGroup + static_cast<uint32_t>(group_index));
		const auto expected_key = expected_layout->getCompatibilityKey();
		const auto actual_key = group->getLayout()->getCompatibilityKey();
		if (!std::ranges::equal(expected_key, actual_key))
			throw std::invalid_argument("BindGroup layout is incompatible with the pipeline layout slot.");

		for (const auto& layout_entry : group->getLayout()->getEntries())
		{
			if (!layout_entry.Flags.has(EDescriptorBindingFlag_t::DynamicOffset))
				continue;
			for (uint32_t element = 0; element < group->getDescriptorCount(layout_entry); ++element)
			{
				if (dynamic_offset_index >= DynamicOffsets.size())
					throw std::invalid_argument("Too few dynamic offsets for the bound BindGroups.");
				const uint32_t dynamic_offset = DynamicOffsets[dynamic_offset_index++];
				const uint64_t alignment = layout_entry.Type == EDescriptorType::UniformBuffer
					? Device->getLimits().MinUniformBufferOffsetAlignment
					: Device->getLimits().MinStorageBufferOffsetAlignment;
				if (dynamic_offset % alignment != 0)
					throw std::invalid_argument("A dynamic buffer offset violates device alignment.");

				const auto resource = std::ranges::find_if(group->getEntries(), [&](const BindGroupEntry& entry)
				{
					return entry.Binding == layout_entry.Binding && entry.ArrayElement == element;
				});
				if (resource != group->getEntries().end())
				{
					const auto* buffer_binding = std::get_if<BufferBinding>(&resource->Resource);
					if (!buffer_binding || !buffer_binding->Buffer)
						throw std::logic_error("Dynamic descriptor does not reference a buffer.");
					const auto& desc = buffer_binding->Buffer->getDescriptor();
					const uint64_t range = buffer_binding->Size == 0
						? desc.Size - buffer_binding->Offset
						: buffer_binding->Size;
					if (dynamic_offset > desc.Size - buffer_binding->Offset ||
						range > desc.Size - buffer_binding->Offset - dynamic_offset)
						throw std::out_of_range("Dynamic buffer offset moves the descriptor range out of bounds.");
				}
			}
		}
		descriptor_sets.emplace_back(group->getVkDescriptorSet());
		RetainedResources.emplace_back(std::move(group));
	}
	if (dynamic_offset_index != DynamicOffsets.size())
		throw std::invalid_argument("Too many dynamic offsets for the bound BindGroups.");
	if (descriptor_sets.empty())
		return;

	CommandBuffer->bindDescriptorSets(
		PipelineType == EPipelineType::Graphics
			? vk::PipelineBindPoint::eGraphics
			: vk::PipelineBindPoint::eCompute,
		vk_layout->getVkPipelineLayout(),
		FirstGroup,
		descriptor_sets,
		DynamicOffsets);
	RetainedResources.emplace_back(Layout);
}

void VulkanCommandList::draw(
	uint32_t VertexCount,
	uint32_t InstanceCount,
	uint32_t FirstVertex,
	uint32_t FirstInstance)
{
	requireRecording("draw");
	requireInsideRendering("draw");
	validateGraphicsPipeline();
	if (BoundGraphicsPipeline->usesMeshShaders())
		throw std::logic_error("A mesh graphics pipeline requires drawMeshTasks.");
	if (VertexCount == 0 || InstanceCount == 0)
		return;
	CommandBuffer->draw(VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void VulkanCommandList::drawIndexed(
	uint32_t IndexCount,
	uint32_t InstanceCount,
	uint32_t FirstIndex,
	int32_t VertexOffset,
	uint32_t FirstInstance)
{
	requireRecording("drawIndexed");
	requireInsideRendering("drawIndexed");
	validateGraphicsPipeline();
	if (BoundGraphicsPipeline->usesMeshShaders())
		throw std::logic_error("A mesh graphics pipeline requires drawMeshTasks.");
	if (IndexCount == 0 || InstanceCount == 0)
		return;
	CommandBuffer->drawIndexed(IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanCommandList::drawMeshTasks(
	uint32_t GroupCountX,
	uint32_t GroupCountY,
	uint32_t GroupCountZ)
{
	requireRecording("drawMeshTasks");
	requireInsideRendering("drawMeshTasks");
	validateGraphicsPipeline();
	if (!BoundGraphicsPipeline->usesMeshShaders())
		throw std::logic_error("drawMeshTasks requires a mesh graphics pipeline.");
	if (!Device->getFeatures().MeshShader)
		throw std::logic_error("Mesh shaders are not enabled on this device.");
	if (GroupCountX == 0 || GroupCountY == 0 || GroupCountZ == 0)
		return;
	const auto draw_mesh_tasks = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(
		Device->getVkDevice().getProcAddr("vkCmdDrawMeshTasksEXT"));
	if (!draw_mesh_tasks)
		throw std::runtime_error("vkCmdDrawMeshTasksEXT is unavailable on the logical device.");
	draw_mesh_tasks(
		static_cast<VkCommandBuffer>(CommandBuffer.get()),
		GroupCountX,
		GroupCountY,
		GroupCountZ);
}

void VulkanCommandList::dispatch(
	uint32_t GroupCountX,
	uint32_t GroupCountY,
	uint32_t GroupCountZ)
{
	requireRecording("dispatch");
	requireOutsideRendering("dispatch");
	if (!BoundComputePipeline)
		throw std::logic_error("Dispatch requires a bound compute pipeline.");
	if (GroupCountX == 0 || GroupCountY == 0 || GroupCountZ == 0)
		return;
	CommandBuffer->dispatch(GroupCountX, GroupCountY, GroupCountZ);
}

void VulkanCommandList::requireRecording(const char* Operation) const
{
	if (State != ECommandListState::Recording)
		throw std::logic_error(std::string(Operation) + " requires a recording command list.");
}

void VulkanCommandList::requireInsideRendering(const char* Operation) const
{
	if (!InsideRendering)
		throw std::logic_error(std::string(Operation) + " requires an active rendering scope.");
}

void VulkanCommandList::requireOutsideRendering(const char* Operation) const
{
	if (InsideRendering)
		throw std::logic_error(std::string(Operation) + " is not allowed inside a rendering scope.");
}

void VulkanCommandList::validateRenderingInfo(const RenderingInfo& Info) const
{
	if (Info.Area.X < 0 || Info.Area.Y < 0 || Info.Area.Width == 0 || Info.Area.Height == 0)
		throw std::invalid_argument("Rendering area is invalid.");
	if (Info.ColorAttachments.size() > MaxColorAttachments ||
		Info.ColorAttachments.size() > Device->getLimits().MaxColorAttachments)
	{
		throw std::invalid_argument("Rendering uses too many color attachments.");
	}
	if (Info.ColorAttachments.empty() && !Info.DepthStencil)
		throw std::invalid_argument("Rendering requires at least one attachment.");
	if (Info.LayerCount == 0 || (Info.ViewMask != 0 && Info.LayerCount != 1))
		throw std::invalid_argument("Rendering layer count is invalid.");
	if (Info.ViewMask != 0 && !Device->getFeatures().Multiview)
		throw std::invalid_argument("Multiview rendering is not supported by this device.");

	ESampleCount common_samples = ESampleCount::Count1;
	bool samples_set = false;
	auto validate_view = [&](const std::shared_ptr<RImageView>& view, bool color)
	{
		getVulkanView(view);
		const auto& view_desc = view->getDescriptor();
		const auto& image_desc = view->getImage()->getDescriptor();
		if (color ? hasDepthAspect(view_desc.Format) : !hasDepthAspect(view_desc.Format))
			throw std::invalid_argument(color
				? "A color attachment cannot use a depth/stencil format."
				: "A depth attachment requires a depth/stencil format.");
		if (view_desc.MipLevelCount != 1)
			throw std::invalid_argument("A rendering attachment view must select exactly one mip level.");
		const uint32_t required_layers = Info.ViewMask == 0
			? Info.LayerCount
			: std::bit_width(Info.ViewMask);
		if (view_desc.ArrayLayerCount < required_layers)
			throw std::invalid_argument("An attachment view does not contain enough array layers.");
		if (color && !image_desc.Usage.has(EImageUsage_t::Target))
			throw std::invalid_argument("A color attachment image requires Target usage.");
		if (!color && !image_desc.Usage.has(EImageUsage_t::DepthStencil))
			throw std::invalid_argument("A depth/stencil attachment image requires DepthStencil usage.");
		const uint32_t width = mipExtent(image_desc.Width, view_desc.BaseMipLevel);
		const uint32_t height = mipExtent(image_desc.Height, view_desc.BaseMipLevel);
		const uint64_t right = static_cast<uint64_t>(Info.Area.X) + Info.Area.Width;
		const uint64_t bottom = static_cast<uint64_t>(Info.Area.Y) + Info.Area.Height;
		if (right > width || bottom > height)
			throw std::out_of_range("Rendering area exceeds an attachment extent.");
		if (!samples_set)
		{
			common_samples = image_desc.SampleCount;
			samples_set = true;
		}
		else if (common_samples != image_desc.SampleCount)
		{
			throw std::invalid_argument("All rendering attachments must use the same sample count.");
		}
	};

	for (const auto& attachment : Info.ColorAttachments)
	{
		validate_view(attachment.View, true);
		validateResolve(attachment.View, attachment.ResolveView, attachment.ResolveMode, true);
		if (attachment.ResolveView)
			getVulkanView(attachment.ResolveView);
	}
	if (Info.DepthStencil)
	{
		validate_view(Info.DepthStencil->View, false);
		const EFormat format = Info.DepthStencil->View->getDescriptor().Format;
		if (!hasDepthAspect(format) && Info.DepthStencil->Depth.ResolveMode != EResolveMode::None)
			throw std::invalid_argument("Depth resolve requires a depth aspect.");
		if (!hasStencilAspect(format) && Info.DepthStencil->Stencil.ResolveMode != EResolveMode::None)
			throw std::invalid_argument("Stencil resolve requires a stencil aspect.");
		if (Info.DepthStencil->ResolveView)
		{
			getVulkanView(Info.DepthStencil->ResolveView);
			const EResolveMode mode = Info.DepthStencil->Depth.ResolveMode != EResolveMode::None
				? Info.DepthStencil->Depth.ResolveMode
				: Info.DepthStencil->Stencil.ResolveMode;
			validateResolve(
				Info.DepthStencil->View,
				Info.DepthStencil->ResolveView,
				mode,
				false);
		}
		else if (Info.DepthStencil->Depth.ResolveMode != EResolveMode::None ||
			Info.DepthStencil->Stencil.ResolveMode != EResolveMode::None)
		{
			throw std::invalid_argument("Depth/stencil resolve modes require a resolve view.");
		}
	}
}

void VulkanCommandList::validateGraphicsPipeline() const
{
	if (!BoundGraphicsPipeline)
		throw std::logic_error("Draw requires a bound graphics pipeline.");
	validateRenderingCompatibility(*BoundGraphicsPipeline);
	validateDynamicStates(*BoundGraphicsPipeline);
}

void VulkanCommandList::validateRenderingCompatibility(const RPipeline& Pipeline) const
{
	const auto* signature = Pipeline.getRenderingSignature();
	if (!signature)
		throw std::logic_error("A graphics pipeline must expose a rendering signature.");
	if (!isRenderingCompatible(*signature, ActiveRenderingSignature))
		throw std::logic_error("The graphics pipeline is incompatible with the active rendering attachments.");
}

void VulkanCommandList::validateDynamicStates(const RPipeline& Pipeline) const
{
	const EDynamicStates missing = Pipeline.getDynamicStates() & ~InitializedDynamicStates;
	if (missing)
		throw std::logic_error("Draw requires every dynamic pipeline state to be initialized.");
}

void VulkanCommandList::retainRenderingResources(const RenderingInfo& Info)
{
	for (const auto& attachment : Info.ColorAttachments)
	{
		RetainedResources.emplace_back(attachment.View);
		if (attachment.ResolveView)
			RetainedResources.emplace_back(attachment.ResolveView);
	}
	if (Info.DepthStencil)
	{
		RetainedResources.emplace_back(Info.DepthStencil->View);
		if (Info.DepthStencil->ResolveView)
			RetainedResources.emplace_back(Info.DepthStencil->ResolveView);
	}
}

} // namespace rhi
