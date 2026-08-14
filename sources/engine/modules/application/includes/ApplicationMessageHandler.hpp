#pragma once

#include <AppCommon.hpp>
#include <GenericWindow.hpp>
#include <AppEvent.hpp>

namespace app
{

class IApplicationMessageHandler
{
public:
	virtual ~IApplicationMessageHandler() = default;
	virtual void onQuitRequested() {}
	virtual void onAppTermination() {}
	virtual void onLocaleChanged() {}
	virtual void onThemeChanged() {}
	virtual void onWindowClose() {}
	virtual void onWindowResize() {}
	virtual void onWindowMotion() {}
	virtual void onWindowMinimized() {}
	virtual void onWindowMaximized() {}
	virtual void onWindowHide() {}
	virtual void onWindowRestored() {}
	virtual void onMouseEnter() {}
	virtual void onMouseLeave() {}
	virtual void onMouseFocusGained() {}
	virtual void onMouseFocusLost() {}
	virtual bool onKeyDown() { return false; }
	virtual bool onKeyUp() { return false; }
	virtual bool onTextInput() { return false; }
	virtual bool onTextEdit() { return false; }
	virtual bool onMouseMotion() { return false; }
	virtual bool onMouseWheel() { return false; }
	virtual bool onMouseDown() { return false; }
	virtual bool onMouseUp() { return false; }
	virtual bool onDropFile() { return false; }
	virtual bool onDropBegin() { return false; }
	virtual bool onDropEnd() { return false; }
};

}