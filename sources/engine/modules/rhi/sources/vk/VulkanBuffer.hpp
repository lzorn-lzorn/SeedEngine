#pragma once

#include <RHI.h>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace rhi
{

class VulkanDevice;

/** @brief Owns a Vulkan buffer and its bound device memory. */
class VulkanBuffer final : public RBuffer
{
public:
	/**
	 * @brief Creates a buffer and allocates compatible memory.
	 * @param Device Device that owns the resource.
	 * @param Desc Immutable buffer description.
	 * @return A fully bound Vulkan buffer.
	 */
	[[nodiscard]] static std::shared_ptr<VulkanBuffer> create(
		VulkanDevice& Device,
		const BufferDescriptor& Desc);

	~VulkanBuffer() override = default;

	[[nodiscard]] RDevice& getDevice() const noexcept override;
	[[nodiscard]] const BufferDescriptor& getDescriptor() const noexcept override { return Descriptor; }
	[[nodiscard]] bool isValid() const noexcept override { return static_cast<bool>(Buffer); }
	[[nodiscard]] void* getNativeHandle() const noexcept override;
	[[nodiscard]] void* map(DeviceSizeType Offset = 0, DeviceSizeType Size = 0) override;
	void unmap() override;
	void flush(DeviceSizeType Offset, DeviceSizeType Size) override;
	void invalidate(DeviceSizeType Offset, DeviceSizeType Size) override;

	[[nodiscard]] vk::Buffer getVkBuffer() const noexcept { return Buffer.get(); }

private:
	VulkanBuffer(
		VulkanDevice& Device,
		BufferDescriptor Desc,
		std::shared_ptr<DeviceMemory> Memory,
		vk::UniqueBuffer Buffer);

	VulkanDevice* Device { nullptr };
	BufferDescriptor Descriptor;
	// Declared before Buffer so the Vulkan object is destroyed before its bound memory.
	std::shared_ptr<DeviceMemory> Memory;
	vk::UniqueBuffer Buffer;
	bool IsMapped { false };
};

} // namespace rhi
