#pragma once

#include "generic_application/UICommon.hpp"
#include "generic_application/widget/WidgetCommon.hpp"
#include "generic_application/widget/UIElementAnimation.hpp"
#include "generic_application/widget/UIDocument.hpp"
#include "generic_application/widget/UIElement.hpp"
#include "generic_application/widget/Style.hpp"
#include "generic_application/widget/Instantiator.hpp"
#include "generic_application/widget/RenderTree.hpp"
#include "generic_application/window/Viewport.hpp"
#include "generic_application/window/GenericWindow.hpp"
#include "generic_application/reflection/Binding.hpp"
#include "generic_application/reflection/Actions.hpp"
#include "generic_application/event/UIEvent.hpp"
#include "generic_application/window/SafeZone.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace ui
{

class WindowContext
{
public:
	explicit WindowContext(std::shared_ptr<ui::IGenericWindow> WindowPointer);

	UIInstance* LoadDoucument(const std::string& AssetPath, BindingContext& Bindings, ActionRegistry& Actions);

	void tick(float DeltaTime);
	void render();

	Viewport& getViewport();
	StyleEngine& getStyleEngine();
	SafeZone& getSafeZone();

	void setDesignResolution(int32_t Width, int32_t Height);
	void setSafeZoneInsets(const SafeZoneInsets& Insets);

private:
	void processMouseEvent(const ui::MouseEvent& Event);
	void processKeyEvent(const ui::KeyEvent& Event);
	void processWindowResizeEvent();

private:
	std::shared_ptr<ui::IGenericWindow> Window;
	Viewport ViewportData;
	StyleEngine StyleEngineData;
	SafeZone SafeZoneData;
	UIElementAnimationPlayer AnimationPlayer;
	std::unordered_map<std::string, UIInstance> Instances;
};
}
