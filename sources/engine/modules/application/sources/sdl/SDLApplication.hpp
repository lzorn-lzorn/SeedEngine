#pragma once 


#include <string_view>
#include <memory>
#include <string>
#include <flat_map>

#include <SDL3/SDL.h>
#include <GenericApplication.hpp>
#include <GenericWindow.hpp>
#include <ApplicationMessageHandler.hpp>
#include <sdl/SDLWindow.hpp>
#include <AppCommon.hpp>

namespace app::details
{

class SDLApplication final : public IGenericApplication
{
public:
	SDLApplication();
	~SDLApplication();

	SDLApplication(const SDLApplication&) = delete;
	SDLApplication(SDLApplication&&) = default;
	SDLApplication& operator=(const SDLApplication&) = delete;
	SDLApplication& operator=(SDLApplication&&) = default;

	[[nodiscard]] GenericWindowPointer 
	makeWindow(const WindowDescriptor& Descriptor) override;

	// 从事件循环中取出事件并翻译为底层平台的事件
	void pumpMessages() override;

	void processDeferredEvents() override {}
	void tick(float DeltaTime) override {}

	void setMessageHandler(std::shared_ptr<IApplicationMessageHandler> Handler) override;

	[[nodiscard]] std::shared_ptr<IApplicationMessageHandler> 
	getMessageHandler() const override;

	void setCapture(const GenericWindowPointer& Window) override;
	void releaseCapture() override;


private:
	SDLApplication& setTargetFPS(int32_t FPS);
	void handleEvent(const SDL_Event& Event);
	
	[[nodiscard]] 
	GenericWindowPointer findWindow(SDL_WindowID Id) const;

	[[nodiscard]]
	static EKeyModifierType translateModifiers(SDL_Keymod Modifiers);

	[[nodiscard]]
	static EMouseType translateMouseButton(uint8_t Button);

private:
	std::shared_ptr<IApplicationMessageHandler> MessageHandler;
	std::flat_map<WindowId_t, std::weak_ptr<SDLWindow>> Windows;
};	

}