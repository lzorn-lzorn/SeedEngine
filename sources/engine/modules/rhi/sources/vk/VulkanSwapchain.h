#pragma once
#include <RHI.h>

namespace rhi
{

class VulkanSwapchain final : public RSwapchain
{
public:
	
	void resize(uint32_t width, uint32_t height) override;
	void present() override;
};

}