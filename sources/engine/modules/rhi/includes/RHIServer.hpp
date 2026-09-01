#pragma once 

#include "RHI.h"
#include <RHI.h>
#include <memory>
namespace rhi
{

class RHIServer
{
public:
	static RHIServer& self() 
	{
		static RHIServer instance;
		return instance;
	}
	RHIServer(const RHIServer&) = delete;
	RHIServer& operator=(const RHIServer&) = delete;
	RHIServer(RHIServer&&) = delete;
	RHIServer& operator=(RHIServer&&) = delete;

	void initialize(ESupportedBackendAPI BackendAPI, const ui::GenericWindowPointer& Window);
	void shutdown();
	[[nodiscard]] bool isInitialized() const noexcept;
	[[nodiscard]] IRHI* getRHI() const noexcept { return RHIInstance.get(); }

private:
	RHIServer();
	~RHIServer();

	std::unique_ptr<IRHI> RHIInstance;
	std::shared_ptr<RDevice> DeviceInstance;

};

using RenderServer = RHIServer;

}