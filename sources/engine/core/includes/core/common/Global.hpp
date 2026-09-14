#pragma once 

#include <cstdint>
#include <atomic>
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

using HandleIdType = uint64_t;

inline constexpr HandleIdType InvalidHandleId = 0ull;
inline std::atomic<uint64_t> GNextHandleId { 0 };
}