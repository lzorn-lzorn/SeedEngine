#pragma once 
#include <expected>
#include <string_view>
#include <memory>
#include <string>
#include <chrono>

#include <SDL3/SDL.h>
#include "Application.hpp"

namespace app::details
{

class SDLApplication final
{
public:
	static std::expected<std::unique_ptr<SDLApplication>, std::string> create(std::string_view Title, int Width, int Height, bool IsResizable = true, bool IsFullScreen = false);

public:
	SDLApplication();
	~SDLApplication();

	SDLApplication(const SDLApplication&) = delete;
	SDLApplication(SDLApplication&&) = default;
	SDLApplication& operator=(const SDLApplication&) = delete;
	SDLApplication& operator=(SDLApplication&&) = default;

	bool initialize(std::string_view Title, int Width, int Height
		, bool IsResizable, bool IsFullScreen, std::string& OutError);

	void run();

	SDLApplication& setTargetFPS(int32_t FPS);
	void handleEvent(const SDL_Event& Event);
	void handleRender() {}
	void handleQuit() {}
	void tick(float DeltaTime);
private:
	using WindowUniquePtr = std::unique_ptr<SDL_Window, 
		/* Deleter */ decltype([](SDL_Window* W) { SDL_DestroyWindow(W); })>;
	
	using RendererUniquePtr = std::unique_ptr<SDL_Renderer, 
		/* Deleter */ decltype([](SDL_Renderer* R) { SDL_DestroyRenderer(R); })>;

	WindowUniquePtr Window;
	RendererUniquePtr Renderer;
	bool IsRunning = false;
	std::chrono::duration<float> TargetFrameDuration { };
};	

}