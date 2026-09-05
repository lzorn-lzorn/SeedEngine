#pragma once

#include <memory>
#include <RHI.h>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace rhi
{
struct QueueFamilyIndices {
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;

    bool isComplete() {
        return GraphicsFamily.has_value() && PresentFamily.has_value();
    }
	
    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice InDevice, vk::SurfaceKHR Surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.GraphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(InDevice, i, Surface, &presentSupport);

            if (presentSupport) {
                indices.PresentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }
};

class VulkanDevice final : public RDevice
{
public:
	VulkanDevice(vk::PhysicalDevice& RealGPU, vk::UniqueDevice& LogicalDevice)
		: RealGPU(RealGPU), LogicalDevice(LogicalDevice) {}
	
	~VulkanDevice() = default;
	
    uint32_t findMemoryType(uint32_t TypeBits, vk::MemoryPropertyFlags Properties);
    vk::PhysicalDevice& getVkPhysicalDevice() { return RealGPU; }
    vk::Device& getVkDevice() { return LogicalDevice.get(); }
public:
	RBuffer* createBuffer() override;
	RImage* createImage() override;
    std::shared_ptr<RImageView> createImageView(const RImageView::Descriptor_t& Desc) override;
	RSampler* createSampler() override;
	RShader* createShader() override;
	RPipeline* createPipeline() override;
	RRenderPass* createRenderPass() override;
	RCommandList* createCommandList() override;
	RSwapchain* createSwapchain() override;
	RTexture* createTexture() override;
    std::shared_ptr<DeviceMemory> allocateMemory(
        MemoryRequirements Requirements,
        EMemoryProperty Property) override;
    void freeMemory(std::shared_ptr<DeviceMemory> Memory) override;

	void waitIdle() override;
	void* getNativeHandle() const override;
private:
	vk::PhysicalDevice& RealGPU;
	vk::UniqueDevice& LogicalDevice;
};

} // namespace rhi