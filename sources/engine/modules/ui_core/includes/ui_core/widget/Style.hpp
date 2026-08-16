#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "ui_core/widget/WidgetCommon.hpp"
#include "ui_core/widget/UIElement.hpp"

namespace ui
{

struct StyleSelector
{
	std::string TypeName;
	std::string ClassName;
	EInteractionState InteractionState = EInteractionState::enum_type::None;
};

struct StyleRule
{
	StyleSelector Selector;
	std::unordered_map<std::string, UIValue> Declarations;
	uint32_t Priority = 0;
};

class StyleEngine
{
public:
	void addStyleSheet(const std::vector<StyleRule>& Rules);
	std::unordered_map<std::string, UIValue> computeStyle(const UIElement* Element) const;

private:
	bool match(const StyleSelector& Selector, const UIElement& Element) const;

	std::vector<StyleRule> StyleRules;
};

} // namespace ui