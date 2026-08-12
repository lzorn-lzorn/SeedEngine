#include "../includes/Application.hpp"

#include "sdl/SDLApplication.hpp"

namespace app 
{

class Application::AppImpl
{
public:
	AppImpl(std::string_view Title, int Width, int Height, bool Fullscreen, bool VSync)
	{
		using result_type = std::expected<std::unique_ptr<details::SDLApplication>, std::string>;

		result_type result = details::SDLApplication::create(Title, Width, Height, Fullscreen, VSync);
		if(result.has_value())
		{
			SDLApp = std::move(result.value());
		}
		else
		{
			throw std::runtime_error(result.error());
		}
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