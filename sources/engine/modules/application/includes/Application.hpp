
#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
namespace app
{


class Application final
{
public:
	static Application& self()
	{
		static Application instance;
		return instance;
	}
	~Application();

	void initialize(std::string_view Title, int Width, int Height, bool Fullscreen, bool VSync);
	void tick(float DeltaTime);


private:
	Application();
private:
	class AppImpl;
	std::unique_ptr<AppImpl> Impl;
};
}