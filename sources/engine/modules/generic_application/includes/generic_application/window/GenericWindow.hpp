#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <memory>

#include <core/math/Vector.hpp>

#include "generic_application/UICommon.hpp"


namespace ui
{

enum class EWindowType : uint8_t
{
	Windowed,
	Borderless,
	Fullscreen,
};

struct WindowDescriptor
{
	std::string Title;
	core::math::Vec2i LeftTopPoint;
	int Width, Height;

	bool IsResizable;
	bool IsVisiable;
	bool HasBorder;
	bool AcceptsInputs;
	bool IsDialog;
};
class IGenericWindow
{
public:
	IGenericWindow() = default;
	virtual ~IGenericWindow() = default;

	[[nodiscard]] virtual WindowId_t getWindowId() = 0;

	virtual void show() = 0;
	virtual void close() = 0; 
	virtual void minimize() = 0;
	virtual void maximize() = 0;

	virtual void setTitle(const std::string& InTitle) = 0;
	virtual void setPosition(int32_t X, int32_t Y) = 0;
	virtual void setSize(int32_t Width, int32_t Height) = 0;
	virtual void setWindowType(EWindowType Type) = 0;

	[[nodiscard]] virtual UIVector getPosition() = 0;
	[[nodiscard]] virtual UIVector getSize() = 0;
	[[nodiscard]] virtual void* getNativeHandle() const noexcept = 0;
};

using GenericWindowPointer = std::shared_ptr<IGenericWindow>;
}