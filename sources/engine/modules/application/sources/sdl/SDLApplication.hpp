#pragma once 
#include <expected>
#include <string_view>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include "Application.hpp"

namespace app::details
{

class SDLApplication final
{
public:
	static std::expected<std::unique_ptr<SDLApplication>, std::string> create();

public:
	SDLApplication();
	~SDLApplication();

	SDLApplication(const SDLApplication&) = delete;
	SDLApplication(SDLApplication&&) = delete;
	SDLApplication& operator=(const SDLApplication&) = delete;
	SDLApplication& operator=(SDLApplication&&) = delete;

	void tick();
private:

	void initialize(std::string_view Title, int Width, int Height
		, SDL_WindowFlags WindowFlags, std::string& OutError);
	void shutdown();
	void run();
	void pollEvents();

private:
	using WindowUniquePtr = std::unique_ptr<SDL_Window, 
		decltype([](SDL_Window* W) { SDL_DestroyWindow(W); })>;
	
	using RendererUniquePtr = std::unique_ptr<SDL_Renderer, 
		decltype([](SDL_Renderer* R) { SDL_DestroyRenderer(R); })>;

	WindowUniquePtr Window;
	RendererUniquePtr Renderer;
	bool IsRunning = false;
};	

}