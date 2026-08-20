#include "ui_core/reflection/Actions.hpp"

namespace ui
{
std::expected<void, EActionRegistrationError> 
ActionRegistry::registerAction(const std::string& ActionName, action_type ActionCallback)
{
	Actions[std::move(ActionName)] = std::move(ActionCallback);
	return {};
}

bool ActionRegistry::invoke(const std::string& ActionName, const ActionContext& Context)
{
	if (Actions.contains(ActionName))
	{
		Actions[ActionName](Context);
		return true;
	}
	return false;
}

}