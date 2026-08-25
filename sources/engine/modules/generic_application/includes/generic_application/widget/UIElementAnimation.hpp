#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <memory>

#include <core/math/MathCommon.hpp>
#include "generic_application/UICommon.hpp"
#include "generic_application/widget/WidgetCommon.hpp"

namespace ui
{

namespace helper
{
constexpr float easeOutCubic(float t)
{
	return 1.0f - std::pow(1.0f - t, 3);	
}

}

class UIElementAnimation
{
public:
	using update_fn = std::move_only_function<bool(float)>;

	UIElementAnimation(float Duration, update_fn UpdateCallback)
		: Duration(Duration)
		, Elapsed(0)
		, UpdateCallback(std::move(UpdateCallback))
	{}

	bool tick(float DeltaTime);

private:
	float Duration;
	float Elapsed;
	update_fn UpdateCallback;

};

class UIElementAnimationPlayer
{
public:
	void add(std::shared_ptr<UIElementAnimation> Animation);
	void update(float DeltaTime);

private:
	std::vector<std::shared_ptr<UIElementAnimation>> Animations;
};

}