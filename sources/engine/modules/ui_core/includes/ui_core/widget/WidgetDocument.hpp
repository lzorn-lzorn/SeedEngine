#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <ui_core/UICommon.hpp>
#include <ui_core/widget/WidgetCommon.hpp>
namespace ui
{

struct SourceLocation
{
	std::string File;
	uint32_t Line;
	uint32_t Column;
};

struct UIEventBinding
{
	std::string EventName;
	std::string ActionName;
	UIObject Arguments;
	SourceLocation Source;
};

struct UIDocumentNode
{
	std::string Type;
	std::string Id;
	std::vector<std::string> Classes;
	std::unordered_map<std::string, UIValue> Properties;
	std::vector<UIEventBinding> Events;
	std::vector<UIDocumentNode> Children;
	SourceLocation Source;
};

struct UIDocumentMetadata
{
	std::string Name;
	std::string Controller;
};

struct UIDocument
{
	uint32_t Version = 1;
	UIDocumentMetadata Metadata;
	std::vector<UIAssetReference> StyleSheet;
	std::unordered_map<std::string, UIAssetReference> Resources;
	UIDocumentNode Root;
};
}