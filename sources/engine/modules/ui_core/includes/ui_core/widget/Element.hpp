
#pragma once

#include <string>
#include <unordered_map>

#include <core/wrappers/Flag.hpp>
#include <core/math/Geometry.hpp>
#include <core/math/MathCommon.hpp>

#include <ui_core/widget/WidgetCommon.hpp>
#include <ui_core/widget/UIDocument.hpp>
#include <ui_core/event/UIEvent.hpp>
#include <ui_core/UICommon.hpp>

namespace ui
{

class PropertyDescriptor;
class StyleEngine;
class WidgetRegistry;

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

	// 状态
	void setState(EInteractionState State, bool IsActive);
	bool hasState(EInteractionState State) const;
	EInteractionState getState() const;

	// 脏标记
	void invalidate(EDirtyFlags DirtyFlags);
	EDirtyFlags getDirtyFlags() const;
	void clearDirtyFlags(EDirtyFlags DirtyFlags = EDirtyFlags::enum_type::All);

	// 属性访问
	void setDescriptor(const PropertyDescriptor* Descriptor);
	PropertyDescriptor* getDescriptor() const;

	void setInlineProperty(const std::string& PropertyName, UIValue Value);
	const std::unordered_map<std::string, UIValue>& getInlineProperties() const;

	UISize_t mesure(const LayoutConstraints& Constraints);
	void arrange(const UIRectangle& FinalBounds);

	void invalidateMeasure();
	void invalidateArrange();
	UIRectangle getBounds() const;
	UISize_t getMeasuredSize() const;

	virtual bool hitTest(const UIVector& Point) const;
	virtual void onMouseEvent(const MouseEvent& Event);

	virtual bool shouldCreateRenderNode() const { return true; }
	virtual void generateRenderCommand(/* class RenderCommandList& CommandList */) const;

	void addEventBinding(UIEventBinding Binding);
	const std::vector<UIEventBinding>& getEventBindings() const;

	virtual void transferStateFrom(const UIElement& Other);

protected:
	virtual UISize_t mesureOverride(const LayoutConstraints& Constraints) = 0;
	virtual void arrangeOverride(const UIRectangle& FinalBounds) = 0;

	UIRectangle Bounds;
	UISize_t MeasuredSize;
	bool HasMeasureDirty = true;
	bool HasArrangeDirty = true;
	bool IsConstraintsEqual = false;
	LayoutConstraints LastConstraints;

private:
	UIElement* Parent = nullptr;
	std::vector<std::unique_ptr<UIElement>> Children;
	std::string DocumentId;
	std::string TypeName;
	std::vector<std::string> Classes;
	EInteractionState InteractionState = EInteractionState::enum_type::None;
	EDirtyFlags DirtyFlags = EDirtyFlags::enum_type::All;
	const PropertyDescriptor* Descriptor = nullptr;
	std::unordered_map<std::string, UIValue> InlineProperties;
	std::vector<UIEventBinding> EventBindings;
};

}