#include <Application.hpp>
#include <sdl/SDLApplication.hpp>

namespace app 
{

std::unique_ptr<ui::IGenericApplication> createGenericApplication()
{
	return std::make_unique<details::SDLApplication>();
}

void Application::initialize()
{
}

} // namespace app