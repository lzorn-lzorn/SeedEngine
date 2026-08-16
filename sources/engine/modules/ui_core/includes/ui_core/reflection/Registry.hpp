#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Property.hpp"
#include "ui_core/widget/UIElement.hpp"
namespace ui
{

struct WidgetDescrptor
{
	std::string TypeName;
	std::string DisplayName;
	std::string Category;
	std::function<std::unique_ptr<UIElement>()> Factory;
	std::unordered_map<std::string, PropertyDescriptor> Properties;
};

enum class ERegistryError
{
	DuplicateWidget,
	WidgetNotFound
};

class WidgetRegistry
{
public:
	std::expected<void, ERegistryError> registerWidget(const WidgetDescrptor& Descriptor);
	const WidgetDescrptor* findWidget(const std::string& TypeName) const;

private:
	std::unordered_map<std::string, WidgetDescrptor> Widgets;

};

}