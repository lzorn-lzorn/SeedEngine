#pragma once 

#include <cstdint>
namespace core
{

enum class ESystemType : uint8_t
{
	Global = 0,
	Game,
	Renderer,
	Physics,
	Audio,
	Video,
	Network,
	AI,
	Script,
	Animation,
	Editor
};

}