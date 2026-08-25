#pragma once

#include <vector>

#include "generic_application/UICommon.hpp"
#include "generic_application/widget/UIElement.hpp"


namespace ui
{
struct RenderCommand
{
	UIRectangle Bounds;
	UIColor Color;
	//  TODO: 流程走通之后追加纹理, 文字
};

struct RenderNode
{
	UIRectangle Bounds;
	UIColor Tint;
	float Opacity = 1.0f;
	std::vector<RenderCommand> Commands;
	std::vector<RenderNode> Children;
	bool ShouldClipToBounds = false;
};

class RenderTreeBuilder
{
public:
	RenderNode build(const UIElement& Root);

private:
	void traverse(const UIElement& Elem, RenderNode& Parent);

};
} // namespace ui
