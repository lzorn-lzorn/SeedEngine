#pragma once

#include "generic_application/UICommon.hpp"
namespace ui
{

struct Anchor
{
	UIVector AnchorMin; // 锚框左下角
	UIVector AnchorMax; // 锚框右上角

	UIVector OffsetMin; // 控件左下角 AnchorMin 的偏移
	UIVector OffsetMax; // 控件右上角 AnchorMax 的偏移

	UIVector Pivot; // 轴心, 即控件自身归一化点 [0, 1]
};

class UILayout
{


};

}