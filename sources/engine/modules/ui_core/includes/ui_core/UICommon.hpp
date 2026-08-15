#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <math/MathCommon.hpp>
#include <math/Vector.hpp>
#include <math/Color.hpp>
#include <math/Geometry.hpp>
namespace ui
{
using WindowId_t = std::uint64_t;
using InputDeviceId_t = std::uint64_t;

struct UISize_t
{
	int32_t Width;
	int32_t Height;
};

using UIVector = core::math::Vec2i;
using UIColor = core::math::LinearColor4D;
using UIRectangle = core::math::Rectangle;

}