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

[[nodiscard]] GenericWindowPointer 
SDLApplication::makeWindow(const WindowDescriptor& Descriptor) 
{
	SDL_WindowFlags flags = 0;
	if (Descriptor.IsResizable) flags |= SDL_WINDOW_RESIZABLE;
	if (Descriptor.HasBorder) flags |= SDL_WINDOW_BORDERLESS;
	if (Descriptor.IsVisiable) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	else flags |= SDL_WINDOW_HIDDEN;

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

	auto window = std::make_shared<SDLWindow>(native_window);
	Windows.emplace(SDL_GetWindowID(native_window), std::weak_ptr<SDLWindow>());

	return window;
}

SDLApplication::SDLApplication()
{
	if(!SDL_Init(SDL_INIT_VIDEO) || !SDL_Init(SDL_INIT_GAMEPAD))
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
	default:
		break;
	}
}

void SDLApplication::setMessageHandler(std::shared_ptr<IApplicationMessageHandler> Handler)
{
	MessageHandler = std::move(Handler);
}

std::shared_ptr<IApplicationMessageHandler> SDLApplication::getMessageHandler() const
{
	return MessageHandler;
}

GenericWindowPointer SDLApplication::findWindow(SDL_WindowID Id) const
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