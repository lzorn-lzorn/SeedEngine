#pragma once 

#include "ui_core/reflection/Registry.hpp"

#include <expected>
namespace ui
{

void ElementRegistry::registerElement(const ElementDescriptor& Descriptor)
{
	Elements[Descriptor.TypeName] = std::make_unique<ElementDescriptor>(std::move(Descriptor));
}

const ElementDescriptor* ElementRegistry::findElement(const std::string& TypeName) const
{
	if(Elements.contains(TypeName))
	{
		return Elements.at(TypeName).get();
	}
	return nullptr;
}


}