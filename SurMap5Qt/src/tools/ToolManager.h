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

class SelectTool;
class MoveTool;
class RotateTool;
class ScaleTool;

class ToolManager
{
public:
	ToolManager();
	~ToolManager();

	// The current tool (CGeneralView::getCurCtrl / currentTool()).
	EditorTool* currentTool() { return current_; }

	// Switch the active tool by index (0 = Select, 1 = Move, 2 = Rotate,
	// 3 = Scale — the tools tree order in SurMap5).
	void setCurrentTool(int index);
	int currentIndex() const { return currentIndex_; }

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

	std::vector<EditorTool*> tools_;
	EditorTool* current_ = nullptr;
	int currentIndex_ = 0;
};
