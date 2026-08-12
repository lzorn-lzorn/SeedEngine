#pragma once

namespace renderer
{


/**
 * @brief UI 渲染器, 其内部不会使用 RDG, 而是以逐帧渲染为基础, 在一个 RenderTarget 上渲染UI
 */
class UIRenderer
{
public:
	explicit UIRenderer() = default;

	UIRenderer(UIRenderer&&) = delete;
	UIRenderer& operator=(UIRenderer&&) = delete;

	UIRenderer(const UIRenderer&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	void beginFrame();
	void draw();
	void endFrame();
};
}