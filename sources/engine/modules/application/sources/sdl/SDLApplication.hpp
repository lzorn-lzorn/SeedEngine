#pragma once 


#include <string_view>
#include <memory>
#include <string>
#include <flat_map>

#include <SDL3/SDL.h>
#include <ui_core/GenericApplication.hpp>
#include <ui_core/window/GenericWindow.hpp>
#include <ui_core/event/MessageHandler.hpp>
#include <sdl/SDLWindow.hpp>
#include <ui_core/UICommon.hpp>

namespace app::details
{

class SDLApplication final : public ui::IGenericApplication
{
public:
	SDLApplication();
	~SDLApplication();

	SDLApplication(const SDLApplication&) = delete;
	SDLApplication(SDLApplication&&) = default;
	SDLApplication& operator=(const SDLApplication&) = delete;
	SDLApplication& operator=(SDLApplication&&) = default;

	[[nodiscard]] ui::GenericWindowPointer 
	makeWindow(const ui::WindowDescriptor& Descriptor) override;

	// 从事件循环中取出事件并翻译为底层平台的事件
	void pumpMessages() override;

	void processDeferredEvents() override {}
	void tick(float DeltaTime) override {}

	void setMessageHandler(std::shared_ptr<ui::IApplicationMessageHandler> Handler) override;

	[[nodiscard]] std::shared_ptr<ui::IApplicationMessageHandler> 
	getMessageHandler() const override;

	void setCapture(const ui::GenericWindowPointer& Window) override;
	void releaseCapture() override;


private:
	SDLApplication& setTargetFPS(int32_t FPS);
	void handleEvent(const SDL_Event& Event);
	
	[[nodiscard]] 
	ui::GenericWindowPointer findWindow(SDL_WindowID Id) const;

	[[nodiscard]]
	static ui::EKeyModifierType translateModifiers(SDL_Keymod Modifiers);

	[[nodiscard]]
	static ui::EMouseType translateMouseButton(uint8_t Button);

private:
	std::shared_ptr<ui::IApplicationMessageHandler> MessageHandler;
	std::flat_map<ui::WindowId_t, std::weak_ptr<SDLWindow>> Windows;
};	

}