#include <Application.hpp>
#include <sdl/SDLApplication.hpp>

namespace app 
{

class Application::AppImpl
{
public:
	AppImpl(std::string_view Title, int Width, int Height, bool Fullscreen, bool VSync)
	{
		
	}
	~AppImpl()
	{
		SDLApp.reset();
	}
private:
	std::unique_ptr<details::SDLApplication> SDLApp;
};

void Application::initialize(std::string_view Title, int Width, int Height, bool Fullscreen, bool VSync)
{
	Impl = std::make_unique<AppImpl>(Title, Width, Height, Fullscreen, VSync);
}

} // namespace app