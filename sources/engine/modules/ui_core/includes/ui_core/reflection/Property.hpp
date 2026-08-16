#pragma once 

#include <cstdint>
#include <expected>
#include <functional>
#include <string>

#include <core/wrappers/Flag.hpp>

#include "ui_core/UICommon.hpp"
#include "ui_core/widget/UIElement.hpp"


namespace ui
{
enum class Property_t : uint16_t
{
	Boolean,
	Integer,
	Number,
	String,
	Color,
	Vector2D,
	Length,
	Asset,
	Enumeration,
	Object,
	Array,
	Binding
};

enum class EPropertyFlags_t : uint32_t
{

	None = 0,
	AffectsStyle = 1U << 0U,
	AffectsMesure = 1U << 1U,
	AffectsArrange = 1U << 2U,
	AffectsPaint = 1U << 3U,
	Bindable = 1U << 4U,
	Editable = 1U << 5U,
	Animatable = 1U << 6U
};
using EPropertyFlags = core::wrappers::Flags<EPropertyFlags_t>;

struct PropertyError
{
	std::string Message;
};

struct PropertyDescriptor
{
	std::string Name;
	std::string DisplayName;
	std::string Category;
	Property_t Type = Property_t::String;
	EPropertyFlags Flags = EPropertyFlags::enum_type::None;
	UIValue DefaultValue;
	EDirtyFlags DirtyFlags = EDirtyFlags::enum_type::None;

	std::function<std::expected<void, PropertyError>(UIElement&, const UIValue&)> Setter;
};


}