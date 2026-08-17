#pragma once

#include "ui_core/UICommon.hpp"
#include "WidgetCommon.hpp"
#include "UIDocument.hpp"
#include "widget/UIElement.hpp"
#include "ui_core/reflection/Registry.hpp"
#include "ui_core/reflection/Binding.hpp"
#include "ui_core/reflection/Actions.hpp"

#include <vector>
#include <string>
namespace ui
{

enum class EUIDiagnosticSeverity
{
	Warning,
	Error
};

struct UIDiagnostic
{
	EUIDiagnosticSeverity Severity;
	std::string Message;
	SourceLocation Source;
};

struct UIInstance 
{
	std::unique_ptr<UIElement> Root;
	std::vector<UIDiagnostic> Diagnostics;
	
	[[nodiscard]] bool hasError() const;
};

struct UIInstantiationContext
{
	const WidgetRegistry& Registry;
	ActionRegistry& Actions;
	BindingContext& Bindings;
	// TODO: AssertManager, LocalizationServer, StyleEngine
};

UIInstance InstantiateUIDocument(const UIDocument& Document, const UIInstantiationContext& Context);

}