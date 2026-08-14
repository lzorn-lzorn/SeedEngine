#pragma once
#include <ApplicationMessageHandler.hpp>
#include <AppCommon.hpp>
#include <GenericWindow.hpp>

#include <memory>
namespace app
{
class IGenericApplication
{
public:
	virtual ~IGenericApplication() = default;

	[[nodiscard]] virtual GenericWindowPointer 
	makeWindow(const WindowDescriptor& Descriptor) = 0;

	// 从事件循环中取出事件并翻译为底层平台的事件
	virtual void pumpMessages() = 0; 

	virtual void processDeferredEvents() {}
	virtual void tick(float DeltaTime) {}

	virtual void setMessageHandler(std::shared_ptr<IApplicationMessageHandler> Handler) = 0;

	[[nodiscard]] virtual std::shared_ptr<IApplicationMessageHandler> 
	getMessageHandler() const = 0;

	virtual void setCapture(const GenericWindowPointer& Window) = 0;
	virtual void releaseCapture() = 0;
};

using GenericWindowPointer = std::shared_ptr<IGenericWindow>;
class GenericApplication : public IGenericApplication
{


};

}