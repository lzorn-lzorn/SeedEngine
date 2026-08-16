#pragma once

#include "ui_core/UICommon.hpp"

namespace ui
{
struct SafeZoneInsets
{
	float Left = 0.0f;
	float Top = 0.0f;
	float Right = 0.0f;
	float Bottom = 0.0f;
};

class SafeZone
{
public:
	void setInsets(SafeZoneInsets Insets);
	[[nodiscard]] SafeZoneInsets getInsets() const;

	UIRectangle applySafeZone(const UIRectangle& Bounds) const;

private:
	SafeZoneInsets Insets;
};

}