#pragma once

#include <unordered_map>
#include <string>

#include <core/wrappers/Observer.h>

#include "ui_core/UICommon.hpp"
#include "ui_core/widget/WidgetCommon.hpp"

namespace ui
{

class BindingContext
{
public:
	template<typename Ty>
	void expose(std::string Path, core::wrappers::Observer<Ty> Obs);

	void* getObserver(const std::string& Path) const;

private:
	std::unordered_map<std::string, void*> Bindings;
};
}