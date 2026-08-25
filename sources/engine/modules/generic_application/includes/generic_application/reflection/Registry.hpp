#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "generic_application/widget/UIProperties.hpp"
#include "generic_application/widget/UIElement.hpp"
namespace ui
{

enum class ERegistryError
{
	DuplicateWidget,
	WidgetNotFound
};

class ElementRegistry
{
public:
	static ElementRegistry& getInstance()
	{
		static ElementRegistry Instance;
		return Instance;
	}
	~ElementRegistry() = default;
	void registerElement(const ElementDescriptor& Descriptor);
	const ElementDescriptor* findElement(const std::string& TypeName) const;

private:
	ElementRegistry() = default;
	ElementRegistry(const ElementRegistry&) = delete;
	ElementRegistry& operator=(const ElementRegistry&) = delete;
	std::unordered_map<std::string, std::unique_ptr<ElementDescriptor>> Elements;

};

}