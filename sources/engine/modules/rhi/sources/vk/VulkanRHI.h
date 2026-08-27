#pragma once

#include <memory>
#include <RHI.h>
#include <vulkan/vulkan.hpp>

namespace rhi
{

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