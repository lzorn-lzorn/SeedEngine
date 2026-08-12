#include "EngineCycle.hpp"
#include "Application.hpp"

void EngineCycle::preinitialize() 
{
	app::Application::self();

}
void EngineCycle::initialize() {}
void EngineCycle::postinitialize() {}
void EngineCycle::run() {}
void EngineCycle::processEvents() {}
void EngineCycle::tick(float DeltaTime) {}
void EngineCycle::render() {}
void EngineCycle::quit() { bShouldQuit = true; }
void EngineCycle::destroy() {}