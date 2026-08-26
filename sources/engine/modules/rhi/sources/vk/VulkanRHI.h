#pragma once

#include "vulkan/vulkan.hpp"
#include <memory>
#include <RHI.h>
#include <vulkan/vulkan.hpp>

namespace rhi
{

class VulkanRHI : public IRHI
{
public:

public:
	VulkanRHI() = default;
	virtual ~VulkanRHI() = default;

	virtual ESupportedBackendAPI getBackendAPI() const override { return ESupportedBackendAPI::Vulkan; }
	virtual std::shared_ptr<RDevice> createDevice() override;

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