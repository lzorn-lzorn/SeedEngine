#include "SDL3/SDL_events.h"
#include <memory>
#include <expected>
#include <string>
#include <string_view>
#include <chrono>
#include <utility>

#include <sdl/SDLApplication.hpp>
#include <sdl/SDLWindow.hpp>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>


constexpr int32_t YieldThresholdValue = 1; // 1 millisecond

namespace app::details
{

[[nodiscard]] ui::GenericWindowPointer 
SDLApplication::makeWindow(const ui::WindowDescriptor& Descriptor) 
{
	SDL_WindowFlags flags = 0;
	if (Descriptor.IsResizable) flags |= SDL_WINDOW_RESIZABLE;
	if (!Descriptor.HasBorder) flags |= SDL_WINDOW_BORDERLESS;
	flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	flags |= SDL_WINDOW_VULKAN;
	if (Descriptor.WindowType == ui::EWindowType::Fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
	if (!Descriptor.IsVisiable) flags |= SDL_WINDOW_HIDDEN;

	SDL_Window* native_window = SDL_CreateWindow(
		Descriptor.Title.c_str(),
		Descriptor.Width,
		Descriptor.Height,
		flags
	);

	if (!native_window)
	{
		throw std::runtime_error(std::string("Failed to create SDL window: ") + SDL_GetError());
	}
	
	SDL_SetWindowPosition(
		native_window, 
		Descriptor.LeftTopPoint.x(), 
		Descriptor.LeftTopPoint.y()
	);

	auto window = std::make_shared<SDLWindow>(native_window, Descriptor.WindowType);
	Windows.insert_or_assign(SDL_GetWindowID(native_window), window);

	return window;
}

SDLApplication::SDLApplication()
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
	}
}

SDLApplication::~SDLApplication()
{
	Windows.clear();
	MessageHandler.reset();
	SDL_Quit();
}

void SDLApplication::pumpMessages()
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		handleEvent(event);
	}
}

void SDLApplication::handleEvent(const SDL_Event& Event)
{
	if (!MessageHandler)
	{
		return;
	}
	switch (Event.type)
	{
	case SDL_EVENT_QUIT:
		// Handle quit event
		break;
	case SDL_EVENT_TERMINATING:
		MessageHandler->onAppTermination();
		break;
	case SDL_EVENT_LOCALE_CHANGED:
		MessageHandler->onLocaleChanged();
		break;
	case SDL_EVENT_SYSTEM_THEME_CHANGED:
		MessageHandler->onThemeChanged();
		break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		MessageHandler->onWindowClose();
		break;
	case SDL_EVENT_WINDOW_MOVED:
		MessageHandler->onWindowMotion();
		break;
	case SDL_EVENT_WINDOW_RESIZED:
		MessageHandler->onWindowResize();
		break;
	case SDL_EVENT_WINDOW_MINIMIZED:
		MessageHandler->onWindowMinimized();
		break;
	case SDL_EVENT_WINDOW_MAXIMIZED:
		MessageHandler->onWindowMaximized();
		break;
	case SDL_EVENT_WINDOW_HIDDEN:
		MessageHandler->onWindowHide();
		break;
	case SDL_EVENT_WINDOW_RESTORED:
		MessageHandler->onWindowRestored();
		break;
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
		MessageHandler->onMouseEnter();
		break;
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		MessageHandler->onMouseLeave();
		break;
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		MessageHandler->onMouseFocusGained();
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		MessageHandler->onMouseFocusLost();
		break;
	case SDL_EVENT_KEY_DOWN:
		MessageHandler->onKeyDown();
		break;
	case SDL_EVENT_KEY_UP:
		MessageHandler->onKeyUp();
		break;
	case SDL_EVENT_TEXT_INPUT:
		MessageHandler->onTextInput();
		break;
	case SDL_EVENT_TEXT_EDITING:
		MessageHandler->onTextEdit();
		break;
	case SDL_EVENT_MOUSE_MOTION:
		MessageHandler->onMouseMotion();
		break;
	case SDL_EVENT_MOUSE_WHEEL:
		MessageHandler->onMouseWheel();
		break;
	case SDL_EVENT_MOUSE_BUTTON_UP:
		MessageHandler->onMouseUp();
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		MessageHandler->onMouseDown();
		break;
	case SDL_EVENT_DROP_BEGIN:
		MessageHandler->onDropBegin();
		break;
	case SDL_EVENT_DROP_COMPLETE:
		MessageHandler->onDropEnd();
		break;
	case SDL_EVENT_DROP_FILE:
		MessageHandler->onDropFile();
		break;
	default:
		break;
	}
}

void SDLApplication::setMessageHandler(std::shared_ptr<ui::IApplicationMessageHandler> Handler)
{
	MessageHandler = std::move(Handler);
}

std::shared_ptr<ui::IApplicationMessageHandler> SDLApplication::getMessageHandler() const
{
	return MessageHandler;
}

void SDLApplication::setCapture(const ui::GenericWindowPointer& Window)
{
	if (!Window || !Window->getNativeHandle())
	{
		throw std::invalid_argument("Mouse capture requires a valid generic window.");
	}
	auto* native_window = static_cast<SDL_Window*>(Window->getNativeHandle());
	if (!SDL_SetWindowMouseGrab(native_window, true))
	{
		throw std::runtime_error(std::string("Failed to capture mouse: ") + SDL_GetError());
	}
}

void SDLApplication::releaseCapture()
{
	for (auto it = Windows.begin(); it != Windows.end(); ++it)
	{
		if (auto window = it->second.lock())
		{
			auto* native_window = static_cast<SDL_Window*>(window->getNativeHandle());
			if (!SDL_SetWindowMouseGrab(native_window, false))
			{
				throw std::runtime_error(std::string("Failed to release mouse capture: ") + SDL_GetError());
			}
		}
	}
}

ui::GenericWindowPointer SDLApplication::findWindow(SDL_WindowID Id) const
{
	auto it = Windows.find(Id);
	if (it != Windows.end())
	{
		if (auto window = it->second.lock())
		{
			return window;
		}
	}
	return nullptr;
}

// void run()
// {
// 	using namespace std::chrono;
// 	using Clock = high_resolution_clock;

// 	auto last_time = Clock::now();

// 	while(IsRunning)
// 	{
// 		SDL_Event event;
// 		while(SDL_PollEvent(&event))
// 		{
// 			handleEvent(event);
// 		}

// 		auto now = Clock::now();
// 		float delta_time = duration_cast<duration<float>>(now - last_time).count();
// 		last_time = now;

// 		tick(delta_time);

// 		SDL_RenderClear(Renderer.get());
// 		handleRender();
// 		SDL_RenderPresent(Renderer.get());

// 		if (TargetFrameDuration.count() > 0.0f)
// 		{
// 			auto frame_end = last_time + duration_cast<Clock::duration>(TargetFrameDuration);
// 			auto now_after_render = Clock::now();
// 			auto remain_time = frame_end - now_after_render;

// 			if (remain_time > milliseconds(YieldThresholdValue))
// 			{
// 				auto ms = duration_cast<milliseconds>(remain_time) - milliseconds(YieldThresholdValue);
// 				SDL_Delay(static_cast<Uint32>(ms.count()));
// 				remain_time = frame_end - Clock::now();
// 			}

// 			if (Clock::now() < frame_end)
// 			{
// #if defined(_M_IX86) || defined(_M_X64) || defined(__x86_64__)			
// #	if defined(_MSC_VER)
// 				_mm_pause();
// #	elif defined(__GNUC__) || defined(__clang__)
// 				__builtin_ia32_pause();
// #	else
// 				std::this_thread::yield();
// #	endif
// #endif
// 			}
// 		}
// 	}

// 	handleQuit();
// }

// SDLApplication& SDLApplication::setTargetFPS(int32_t FPS) 
// { 
// 	if(FPS <= 0)
// 	{
// 		std::unreachable();
// 	}
		
// 	[[assume(FPS > 0)]];
// 	TargetFrameDuration = std::chrono::duration<float>(1.0f / static_cast<float>(FPS));
// 	return *this; 
// }



}