#include <sdl/SDLWindow.hpp>
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace
{
void ensureSDLResult(bool Result, const char* Operation)
{
	if (!Result)
	{
		throw std::runtime_error(std::string(Operation) + ": " + SDL_GetError());
	}
}
}

namespace app::details
{

SDLWindow::~SDLWindow()
{
	if (NativeWindow)
	{
		::SDL_DestroyWindow(NativeWindow);
		NativeWindow = nullptr;
	}
}

void SDLWindow::show()
{
	ensureSDLResult(SDL_ShowWindow(NativeWindow), "Failed to show SDL window");
}

void SDLWindow::close()
{
	ensureSDLResult(SDL_HideWindow(NativeWindow), "Failed to hide SDL window");
}

void SDLWindow::minimize()
{
	ensureSDLResult(SDL_MinimizeWindow(NativeWindow), "Failed to minimize SDL window");
}

void SDLWindow::maximize()
{
	ensureSDLResult(SDL_MaximizeWindow(NativeWindow), "Failed to maximize SDL window");
}

void SDLWindow::setTitle(const std::string& InTitle)
{
	ensureSDLResult(SDL_SetWindowTitle(NativeWindow, InTitle.c_str()), "Failed to set SDL window title");
}

void SDLWindow::setPosition(int32_t X, int32_t Y)
{
	ensureSDLResult(SDL_SetWindowPosition(NativeWindow, X, Y), "Failed to set SDL window position");
}

void SDLWindow::setSize(int32_t Width, int32_t Height)
{
	ensureSDLResult(SDL_SetWindowSize(NativeWindow, Width, Height), "Failed to set SDL window size");
}

void SDLWindow::setWindowType(ui::EWindowType Type)
{
	switch (Type)
	{
	case ui::EWindowType::Windowed:
		ensureSDLResult(SDL_SetWindowFullscreen(NativeWindow, false), "Failed to leave SDL fullscreen mode");
		ensureSDLResult(SDL_SetWindowBordered(NativeWindow, true), "Failed to enable SDL window border");
		break;
	case ui::EWindowType::Borderless:
		ensureSDLResult(SDL_SetWindowFullscreen(NativeWindow, false), "Failed to leave SDL fullscreen mode");
		ensureSDLResult(SDL_SetWindowBordered(NativeWindow, false), "Failed to disable SDL window border");
		break;
	case ui::EWindowType::Fullscreen:
		ensureSDLResult(SDL_SetWindowFullscreen(NativeWindow, true), "Failed to enter SDL fullscreen mode");
		break;
	}
	WindowType = Type;
}

ui::WindowId_t SDLWindow::getWindowId()
{
	return static_cast<ui::WindowId_t>(SDL_GetWindowID(NativeWindow));
}

ui::UIVector SDLWindow::getPosition()
{
	int x = 0;
	int y = 0;
	ensureSDLResult(SDL_GetWindowPosition(NativeWindow, &x, &y), "Failed to get SDL window position");
	return ui::UIVector(x, y);
}

ui::UIVector SDLWindow::getSize()
{
	int width = 0;
	int height = 0;
	ensureSDLResult(SDL_GetWindowSizeInPixels(NativeWindow, &width, &height), "Failed to get SDL window size");
	return ui::UIVector(width, height);
}

void* SDLWindow::getNativeHandle() const noexcept
{
	return NativeWindow;
}

} // namespace app::details
