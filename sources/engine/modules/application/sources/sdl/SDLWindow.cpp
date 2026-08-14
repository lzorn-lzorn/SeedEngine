#include <sdl/SDLWindow.hpp>
#include <SDL3/SDL.h>

app::details::SDLWindow::~SDLWindow()
{
	if (NativeWindow)
	{
		::SDL_DestroyWindow(NativeWindow);
		NativeWindow = nullptr;
	}
}
