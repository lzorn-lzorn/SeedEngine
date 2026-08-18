
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include <core/wrappers/Flag.hpp>
#include <core/math/Geometry.hpp>
#include <core/math/MathCommon.hpp>
#include <core/common/Common.hpp>

#include "ui_core/widget/WidgetCommon.hpp"
#include "ui_core/widget/UIDocument.hpp"
#include "ui_core/event/UIEvent.hpp"
#include "ui_core/UICommon.hpp"

namespace ui
{

class PropertyDescriptor;
class StyleEngine;
class WidgetRegistry;
class UIElement;

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


class UIElementChildBase
{
public:
	using element_type = UIElement;

	UIElement* getOnwer();
	UIElement* getOnwer() const;
private:
	friend class UIElement;
	UIElement* Onwer;
};

class UIElementNoChild : public UIElementChildBase
{
public:
	using child_type = void;
private:
	
};

class UIElementChild : public UIElementChildBase
{
public:
	using child_type = std::unique_ptr<UIElement>;

private:
	child_type Child;
};

class UIElementChildren : public UIElementChildBase
{
public:
	using child_type = std::unique_ptr<UIElement>;

	CompatibilityLayer_STLContainer(std::vector<child_type>, Children)
	


	uint32_t getChildrenNum() const { return Children.size(); }
	element_type* getChildrenAt(uint32_t Index) { return Children[Index].get(); }
private:
	std::vector<child_type> Children;
};


class UIElement
	: std::enable_shared_from_this<UIElement>
{
public:
	virtual ~UIElement() = default;

	UIElement* getParent();

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
	virtual UIElementChildBase* getChildren() const = 0;
	virtual UIElementChildBase* getChildren() = 0;

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
	std::string DocumentId;
	std::string TypeName;
	std::vector<std::string> Classes;
	EInteractionState InteractionState = EInteractionState::enum_type::None;
	EDirtyFlags DirtyFlags = EDirtyFlags::enum_type::All;
	const PropertyDescriptor* Descriptor = nullptr;
	std::unordered_map<std::string, UIValue> InlineProperties;
	std::vector<UIEventBinding> EventBindings;
};

class LeafElement : public UIElement
{

	virtual UIElementChildBase* getChildren() const override { return nullptr; }
	virtual UIElementChildBase* getChildren() override { return nullptr; }
protected:
	virtual UISize_t mesureOverride(const LayoutConstraints& Constraints) override;
	virtual void arrangeOverride(const UIRectangle& FinalBounds) override;
};

class WrapperElement : public UIElement
{
public:
	virtual UIElementChildBase* getChildren() const override { return Child; }
	virtual UIElementChildBase* getChildren() override { return Child; }

	void repalceChild(UIElement*) noexcept;

protected:
	virtual UISize_t mesureOverride(const LayoutConstraints& Constraints) override;
	virtual void arrangeOverride(const UIRectangle& FinalBounds) override;

private:
	UIElementChild* Child;
};


class PanelElement : public UIElement
{
public:
	virtual UIElementChildBase* getChildren() const override { return Children; }
	virtual UIElementChildBase* getChildren() override { return Children; }

	void addChild(UIElement*);
	void removeChild(UIElement*) noexcept;
	void replaceChild(uint32_t, UIElement*) noexcept;
	UIElement* getChildAt(uint32_t) noexcept;
protected:
	virtual UISize_t mesureOverride(const LayoutConstraints& Constraints) override;
	virtual void arrangeOverride(const UIRectangle& FinalBounds) override;

private:
	UIElementChildren* Children;
};

class ElementDescriptor
{
public:
	std::string TypeName;
	std::function<std::unique_ptr<UIElement>()> Factory;
	std::unordered_map<std::string, class UIProperty> Properties;
};

template <typename Ty>
concept has_register = requires {
    { Ty::doRegister() } -> std::same_as<void>;
};

#define REGISTER_ELEMENT(Class)                      \
static struct Class##_AutoRegister {                \
	static_assert(has_register<Class>);             \
	Class##_AutoRegister() { Class::doRegister(); } \
} Class##_auto_register_instance;



class HorizontalBox : public UIElement
{
public:
	explicit HorizontalBox(float InSpacing = 0.0f) 
		: Spacing(InSpacing) 
	{};

protected:

	virtual UISize_t mesureOverride(const LayoutConstraints& Constraints) override;
	virtual void arrangeOverride(const UIRectangle& FinalBounds) override;
private:
	float Spacing = 0.0f;
};

class VerticalBox : public UIElement
{
public:
	explicit VerticalBox(float InSpacing = 0.0f) 
		: Spacing(InSpacing) 
	{};

protected:
	virtual UISize_t mesureOverride(const LayoutConstraints& Constraints) override;
	virtual void arrangeOverride(const UIRectangle& FinalBounds) override;

private:
	float Spacing = 0.0f;

};

class FlexElement : public UIElement
{


};

class GridElement : public UIElement
{


};

}