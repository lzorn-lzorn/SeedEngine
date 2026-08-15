
#pragma once

#include "UICommon.hpp"
#include <wrappers/Flag.hpp>
#include <math/MathCommon.hpp>
#include <math/Geometry.hpp>

namespace ui
{
enum class EInteractionState_t : uint32_t
{
	None = 0,
	Hovered = 1U << 0U,
	Pressed = 1U << 1U,
	Focused = 1U << 2U,
	Disabled = 1U << 3U,
	Checked = 1U << 4U,
	Selected = 1U << 5U,
};

using EInteractionState = core::wrappers::Flags<EInteractionState_t>;

enum class EDirtyFlags_t : uint32_t 
{
	None = 0,
	Style = 1U << 0U,
	Measure = 1U << 1U,
	Arrange = 1U << 2U,
	Paint = 1U << 3U,
	Transform = 1U << 4U,
	HitTest = 1U << 5U,
	Accessibility = 1U << 6U,
	All = 0xFFFFFFFFU
};

using EDirtyFlags = core::wrappers::Flags<EDirtyFlags_t>;

class UIElement
{
public:
	virtual ~UIElement() = default;

	void addChild(std::unique_ptr<UIElement> Child);
	UIElement* getParent();
	const std::vector<std::unique_ptr<UIElement>>& getChildren() const;

	void setDocumentId(std::string Id);
	const std::string& getDocumentId() const;
	void setTypeName(std::string Type);
	void setClasses(std::vector<std::string> Classes);
	bool hasClass(const std::string& ClassName) const;

	void setState(EInteractionState State, bool IsActive);
	bool hasState(EInteractionState State) const;
	EInteractionState getState() const;

	void invalidate(EDirtyFlags DirtyFlags);
	EDirtyFlags getDirtyFlags() const;
	void clearDirtyFlags(EDirtyFlags DirtyFlags = EDirtyFlags::enum_type::All);
};

}