#pragma once

#include "GenericWindow.hpp"
#include "ui_core/UICommon.hpp"

namespace ui
{

enum class EViewportScaleMode
{
	Fixed, // 固定大小
	Stretch, // 拉伸适配
	FitWidth, // 适配宽度
	FitHeight, // 适配高度
	Expand, // 扩展适配
};

class Viewport
{
public:
	void setDesignResolution(int32_t Width, int32_t Height);
	void setScaleMode(EViewportScaleMode Mode);
	void setWindowSize(int32_t Width, int32_t Height);

	UIVector doLogicalToPixel(const UIVector& LogicalPoint) const;
	UIVector doPixelToLogical(const UIVector& PixelPoint) const;

	[[nodiscard]] UIVector getDesignResolution() const;
	[[nodiscard]] EViewportScaleMode getScaleMode() const;
	[[nodiscard]] UIRectangle getLogicalViewport() const;
	[[nodiscard]] UIVector getViewportSize() const;
	[[nodiscard]] UIVector getViewportOffset() const;
	[[nodiscard]] float getScaleFactor() const;

private:
	void recalc();
	
};
}