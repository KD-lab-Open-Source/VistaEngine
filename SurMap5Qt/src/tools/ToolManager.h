// ToolManager.h — the editor's current-tool holder, Qt port of the tool
// selection machinery (SurMap5: getToolWindow().currentTool(), the tools tree).
//
// Owns the concrete tool instances and routes view input to the current one,
// exactly the way CGeneralView::WindowProc called currentTool()->on* — but
// through the engine-free EditorTool interface (see editor/EditorTool.h), so
// this file compiles in the Qt executable with no engine flags.

#pragma once

#include "editor/EditorTool.h"

#include <vector>

class QWidget;
class SelectTool;
class MoveTool;
class RotateTool;
class ScaleTool;
class GeoNetTool;
class GeoTxTool;
class ToolzerTool;
class KindTool;
class ColorPicTool;
class UnitTool;
class SourceTool;
class AnchorTool;
class EnvironmentTool;
class TransformPropertyPanel;
class GeoNetPropertyPanel;
class GeoTxPropertyPanel;
class ToolzerPropertyPanel;
class KindPropertyPanel;
class ColorPicPropertyPanel;
class UnitPropertyPanel;
class SourcePropertyPanel;
class AnchorPropertyPanel;
class EnvironmentPropertyPanel;

class ToolManager
{
public:
	ToolManager();
	~ToolManager();

	// Install the engine-facing world bridge on the tools (RenderViewWidget
	// hands EngineViewport's bridge here once it exists).
	void setWorldBridge(IWorldBridge* bridge);

	// The installed bridge (null before a world load). The tools tree uses it to
	// list the object catalog (units/sources/environment models).
	IWorldBridge* bridge() const { return bridge_; }

	// The current tool (CGeneralView::getCurCtrl / currentTool()).
	EditorTool* currentTool() { return current_; }

	// The Properties dock's panel for the current tool (the original's
	// CSurToolBase dialog). Returns a QWidget the MainWindow puts in the
	// propertiesDock_; null when the current tool has no panel.
	QWidget* propertyWidget();

	// Switch the active tool by index (0 = Select, 1 = Move, 2 = Rotate,
	// 3 = Scale — the tools tree order in SurMap5).
	void setCurrentTool(int index);
	int currentIndex() const { return currentIndex_; }

	// Set the brush radius for every brush tool (CSurToolBase::getBrushRadius
	// read a shared value from the tools toolbar combo). Tools without a brush
	// ignore it.
	void setBrushRadius(float radius);

	// All tools, in tree order (CSurToolBase* list the tools tree held).
	const std::vector<EditorTool*>& tools() const { return tools_; }

	// Route an event to the current tool; returns true if handled.
	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord);
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord);
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord);
	bool onRMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord);
	bool onRMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord);
	bool onKeyDown(unsigned keyCode, bool shift, bool control, bool alt);
	bool onDelete();
	void quant(float dt);

private:
	SelectTool* select_ = nullptr;
	MoveTool* move_ = nullptr;
	RotateTool* rotate_ = nullptr;
	ScaleTool* scale_ = nullptr;
	GeoNetTool* geoNet_ = nullptr;
	GeoTxTool* geoTx_ = nullptr;
	ToolzerTool* toolzer_ = nullptr;
	KindTool* kind_ = nullptr;
	ColorPicTool* colorPic_ = nullptr;
	UnitTool* unit_ = nullptr;
	SourceTool* source_ = nullptr;
	AnchorTool* anchor_ = nullptr;
	EnvironmentTool* environment_ = nullptr;

	std::vector<EditorTool*> tools_;
	EditorTool* current_ = nullptr;
	int currentIndex_ = 0;
	IWorldBridge* bridge_ = nullptr;

	// The Properties dock's panel for the transform tools (created lazily).
	TransformPropertyPanel* propertyPanel_ = nullptr;
	// The Properties dock's panel for the GeoNet tool (created lazily).
	GeoNetPropertyPanel* geoNetPanel_ = nullptr;
	// The Properties dock's panel for the GeoTx tool (created lazily).
	GeoTxPropertyPanel* geoTxPanel_ = nullptr;
	// The Properties dock's panel for the Toolzer tool (created lazily).
	ToolzerPropertyPanel* toolzerPanel_ = nullptr;
	// The Properties dock's panel for the surface-kind brush (created lazily).
	KindPropertyPanel* kindPanel_ = nullptr;
	// The Properties dock's panel for the texture brush (created lazily).
	ColorPicPropertyPanel* colorPicPanel_ = nullptr;
	// The Properties dock's panel for the unit-placement tool (created lazily).
	UnitPropertyPanel* unitPanel_ = nullptr;
	// Source/anchor placement panels (created lazily).
	SourcePropertyPanel* sourcePanel_ = nullptr;
	AnchorPropertyPanel* anchorPanel_ = nullptr;
	// The Environment tool's parameter panel (created lazily).
	EnvironmentPropertyPanel* environmentPanel_ = nullptr;
};
