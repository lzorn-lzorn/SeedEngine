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

private:
	RHIServer();
	~RHIServer();

	std::unique_ptr<IRHI> RHIInstance;
	std::unique_ptr<RDevice> DeviceInstance;

};


}