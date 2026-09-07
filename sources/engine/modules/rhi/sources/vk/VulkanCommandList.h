#pragma once

#include <RHI.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rhi
{

class VulkanDevice;

class VulkanCommandList final : public RCommandList
{
public:
	VulkanCommandList(
		VulkanDevice& Device,
		uint32_t QueueFamilyIndex,
		const CommandListDescriptor& Desc);
	~VulkanCommandList() override = default;

	[[nodiscard]] ECommandQueueType getQueueType() const noexcept override
	{
		return Descriptor.QueueType;
	}
	[[nodiscard]] ECommandListLevel getLevel() const noexcept override
	{
		return Descriptor.Level;
	}
	[[nodiscard]] ECommandListState getState() const noexcept override { return State; }
	[[nodiscard]] void* getNativeHandle() const noexcept override;
	[[nodiscard]] vk::CommandBuffer getVkCommandBuffer() const noexcept
	{
		return CommandBuffer.get();
	}

	void begin() override;
	void end() override;
	void reset() override;

	void imageBarriers(std::span<const ImageBarrier> Barriers) override;
	void beginRendering(const RenderingInfo& Info) override;
	void endRendering() override;

	void setViewports(std::span<const Viewport> Viewports) override;
	void setScissors(std::span<const RenderArea> Scissors) override;
	void setBlendConstants(const std::array<float, 4>& Constants) override;
	void setStencilReference(uint32_t FrontReference, uint32_t BackReference) override;
	void setDepthBias(float ConstantFactor, float Clamp, float SlopeFactor) override;
	void setLineWidth(float Width) override;
	void pushConstants(
		const std::shared_ptr<RPipelineLayout>& Layout,
		EShaderStage Stages,
		uint32_t Offset,
		std::span<const std::byte> Data) override;
	void bindPipeline(const std::shared_ptr<RPipeline>& Pipeline) override;

	void draw(
		uint32_t VertexCount,
		uint32_t InstanceCount,
		uint32_t FirstVertex,
		uint32_t FirstInstance) override;
	void drawIndexed(
		uint32_t IndexCount,
		uint32_t InstanceCount,
		uint32_t FirstIndex,
		int32_t VertexOffset,
		uint32_t FirstInstance) override;
	void dispatch(
		uint32_t GroupCountX,
		uint32_t GroupCountY,
		uint32_t GroupCountZ) override;

private:
	void requireRecording(const char* Operation) const;
	void requireInsideRendering(const char* Operation) const;
	void requireOutsideRendering(const char* Operation) const;
	void validateRenderingInfo(const RenderingInfo& Info) const;
	void validateRenderingCompatibility(const RPipeline& Pipeline) const;
	void validateGraphicsPipeline() const;
	void validateDynamicStates(const RPipeline& Pipeline) const;
	void retainRenderingResources(const RenderingInfo& Info);

	VulkanDevice* Device { nullptr };
	CommandListDescriptor Descriptor;
	ECommandListState State { ECommandListState::Initial };
	bool InsideRendering { false };
	std::shared_ptr<RPipeline> BoundGraphicsPipeline;
	std::shared_ptr<RPipeline> BoundComputePipeline;
	RenderingSignature ActiveRenderingSignature {};
	EDynamicStates InitializedDynamicStates {};
	std::vector<std::shared_ptr<void>> RetainedResources;
	vk::UniqueCommandPool CommandPool;
	vk::UniqueCommandBuffer CommandBuffer;
};

} // namespace rhi
