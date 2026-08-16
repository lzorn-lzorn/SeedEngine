#pragma once

#include "ui_core/UICommon.hpp"
#include "ui_core/widget/WidgetCommon.hpp"
#include "ui_core/widget/WidgetAnimation.hpp"
#include "ui_core/widget/WidgetDocument.hpp"
#include "ui_core/widget/UIElement.hpp"
#include "ui_core/widget/Style.hpp"
#include "ui_core/widget/Instantiator.hpp"
#include "ui_core/widget/RenderTree.hpp"
#include "ui_core/window/Viewport.hpp"
#include "ui_core/window/GenericWindow.hpp"
#include "ui_core/reflection/Binding.hpp"
#include "ui_core/reflection/Actions.hpp"
#include "ui_core/event/UIEvent.hpp"

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
	WidgetAnimationPlayer AnimationPlayer;
	std::unordered_map<std::string, UIInstance> Instances;
};
}
