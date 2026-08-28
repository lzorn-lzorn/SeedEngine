#pragma once

#include <memory>
#include <RHI.h>
#include <vulkan/vulkan.hpp>

namespace rhi
{

vk::Format toVkFormat(EFormat);
vk::IndexType toVkIndexType(EIndexFormat);
vk::ShaderStageFlags toVkShaderStageFlags(EShaderStage);
vk::BufferUsageFlags toVkBufferUsage(EBufferUsage);
vk::ImageUsageFlags toVkImageUsage(EImageUsage);
vk::PrimitiveTopology toVkPrimitiveTopology(EPrimitiveTopology);
vk::PolygonMode toVkPolygonMode(EFillMode);
vk::CullModeFlags toVkCullMode(ECullMode);
vk::CompareOp toVkCompareOp(ECompareOp);
vk::StencilOp toVkStencilOp(EStencilOp);
vk::SampleCountFlagBits toVkSampleCount(ESampleCount);
vk::BlendFactor toVkBlendFactor(EBlendFactor);
vk::BlendOp toVkBlendOp(EBlendOp);
vk::ImageLayout toVkImageLayout(EResourceState);
vk::PipelineStageFlags toVkPipelineStage(EResourceState);
vk::AccessFlags toVkAccessMask(EResourceState);
vk::SharingMode toVkSharingMode(ESharingMode);
vk::MemoryPropertyFlags toVkMemoryPropertyFlags(EMemoryProperty);
vk::ImageAspectFlags toVkImageAspectMask(ETextureAspect Aspect, EFormat TextureFormat);
uint64_t clampCopySize(uint64_t Offset, uint64_t RequestedSize, uint64_t MaxSize);
vk::ImageType toVkImageType(EImageDimension Dimension);
vk::PresentModeKHR toVkPresentMode(EPresentMode PresentMode);


class VulkanRHI final : public IRHI
{
public:

public:
	VulkanRHI() = default;
	virtual ~VulkanRHI() = default;

	ESupportedBackendAPI getBackendAPI() const override { return ESupportedBackendAPI::Vulkan; }
	std::shared_ptr<RDevice> createDevice() override;


public:
	vk::Device& getVkDevice() { return LogicalDevice.get(); }
private:
	void createVkInstance();
	void pickPhysicalDevice();
	void createLogicalDevice();
	vk::UniqueInstance Instance;
	vk::PhysicalDevice RealGPU;
	vk::UniqueDevice LogicalDevice;
};


} // namespace rhi