#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

#include <math/MathCommon.hpp>

#include <ui_core/UICommon.hpp>
namespace ui
{
struct LayoutConstraints
{
	UISize_t Minimum;
	UISize_t Maximum {
		core::math::Infinity<int32_t>, 
		core::math::Infinity<int32_t>
	};	

	[[nodiscard]]
	UISize_t constrain(UISize_t Size) const
	{
		return {
			std::clamp(Size.Width, Minimum.Width, Maximum.Width),
			std::clamp(Size.Height, Minimum.Height, Maximum.Height)
		};
	}

	static LayoutConstraints tight(UISize_t Size)
	{
		return { Size, Size };
	}

	static LayoutConstraints loose(UISize_t Maximum)
	{
		return {{}, Maximum};
	}
};

struct UIAssetReference
{
	std::string Uri;
	std::string GUid;
};

struct BindingExpression 
{
	enum class EMode : uint8_t
	{
		OneTime,
		OneWay,
		TwoWay,
	};

	std::string Path;
	std::string Fallback;
	EMode Mode = EMode::OneTime;
};

struct LocalizationExpression
{
	std::string Key;
};

struct UIArray;
struct UIObject;

using UIValue = std::variant<
	std::monostate,
	bool,
	int64_t,
	double,
	std::string,
	UIColor,
	UIVector,
	std::shared_ptr<UIArray>,
	std::shared_ptr<UIObject>,
	UIAssetReference,
	BindingExpression,
	LocalizationExpression
>;

struct UIArray
{
	std::vector<UIValue> Values;
};

struct UIObject
{
	std::unordered_map<std::string, UIValue> Properties;
};
}