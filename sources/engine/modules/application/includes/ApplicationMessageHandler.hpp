#pragma once

#include <AppCommon.hpp>
#include <GenericWindow.hpp>
#include <AppEvent.hpp>

namespace app
{

class IApplicationMessageHandler
{
public:
	virtual ~IApplicationMessageHandler() = default;
};

}