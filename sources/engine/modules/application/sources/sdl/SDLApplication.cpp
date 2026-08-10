#include <memory>
#include <expected>
#include <ranges>
#include <string>
#include <string_view>
#include <chrono>
#include <thread>

#include "sdl/SDLApplication.hpp"
#include "SDL3/SDL_render.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <utility>

constexpr int32_t YieldThresholdValue = 1; // 1 millisecond

namespace app::details
{
static std::expected<std::unique_ptr<SDLApplication>, std::string> create(std::string_view Title, int Width, int Height, bool IsResizable, bool IsFullScreen)
{
	std::unique_ptr<SDLApplication> app = std::make_unique<SDLApplication>();
	std::string error;
	if(!app->initialize(Title, Width, Height, IsResizable, IsFullScreen, error))
	{
		return std::unexpected{error};
	}

	return app;
}

bool SDLApplication::initialize(std::string_view Title, int Width, int Height, bool IsResizable, bool IsFullScreen, std::string& OutError)
{
	if(!SDL_Init(SDL_INIT_VIDEO))
	{
		OutError = SDL_GetError();
		return false;
	}

	SDL_Window* window = nullptr;
	{
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, std::string{Title}.c_str());
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, Width);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, Height);
		SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, IsResizable);
		SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, IsFullScreen);

		window = SDL_CreateWindowWithProperties(props);
		SDL_DestroyProperties(props);
		if(!window)
		{
			OutError = SDL_GetError();
			return false;
		}
	}
	

	SDL_Renderer* renderer = nullptr;
	{
		SDL_PropertiesID props = SDL_CreateProperties();

		SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);
		SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 1);

		renderer = SDL_CreateRendererWithProperties(props);
		SDL_DestroyProperties(props);
		if(!renderer)
		{
			OutError = SDL_GetError();
			SDL_DestroyWindow(window);
			SDL_Quit();
			return false;
		}
	}
	
	Window.reset(window);
	Renderer.reset(renderer);
	return true;
}

void SDLApplication::handleEvent(const SDL_Event& Event)
{
	if (Event.type == SDL_EVENT_QUIT)
	{
		IsRunning = false;
	}
}

void SDLApplication::run()
{
	using namespace std::chrono;
	using Clock = high_resolution_clock;

	auto last_time = Clock::now();

	while(IsRunning)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			handleEvent(event);
		}

		auto now = Clock::now();
		float delta_time = duration_cast<duration<float>>(now - last_time).count();
		last_time = now;

		tick(delta_time);

		SDL_RenderClear(Renderer.get());
		handleRender();
		SDL_RenderPresent(Renderer.get());

		if (TargetFrameDuration.count() > 0.0f)
		{
			auto frame_end = last_time + duration_cast<Clock::duration>(TargetFrameDuration);
			auto now_after_render = Clock::now();
			auto remain_time = frame_end - now_after_render;

			if (remain_time > milliseconds(YieldThresholdValue))
			{
				auto ms = duration_cast<milliseconds>(remain_time) - milliseconds(YieldThresholdValue);
				SDL_Delay(static_cast<Uint32>(ms.count()));
				remain_time = frame_end - Clock::now();
			}

			if (Clock::now() < frame_end)
			{
#if defined(_M_IX86) || defined(_M_X64) || defined(__x86_64__)			
#	if defined(_MSC_VER)
				_mm_pause();
#	elif defined(__GNUC__) || defined(__clang__)
				__builtin_ia32_pause();
#	else
				std::this_thread::yield();
#	endif
#endif
			}
		}
	}

	handleQuit();
}

SDLApplication& SDLApplication::setTargetFPS(int32_t FPS) 
{ 
	if(FPS <= 0)
	{
		std::unreachable();
	}
		
	[[assume(FPS > 0)]];
	TargetFrameDuration = std::chrono::duration<float>(1.0f / static_cast<float>(FPS));
	return *this; 
}


SDLApplication::~SDLApplication()
{
	Renderer.reset();
	Window.reset();	
	SDL_Quit();
}


}