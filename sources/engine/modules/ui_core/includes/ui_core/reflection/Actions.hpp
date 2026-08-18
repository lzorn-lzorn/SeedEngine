#pragma once

#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ui_core/UICommon.hpp"
#include "ui_core/widget/UIElement.hpp"
namespace ui
{

struct ActionContext
{
	UIElement* Sender;
	std::string_view EventName;
	const UIObject& Arguments;
};

enum class EActionRegistrationError
{
	DuplicateAction,
};

class ActionRegistry
{
public:
	using action = std::move_only_function<void(const ActionContext&)>;

	std::expected<void, EActionRegistrationError> registerAction(const std::string& ActionName, action ActionCallback);

	bool invoke(const std::string& ActionName, const ActionContext& Context);

private:
	std::unordered_map<std::string, action> Actions;
};

}