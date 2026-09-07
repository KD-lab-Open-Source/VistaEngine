// SelectTool.h — Qt port of CSurToolSelect (SurMap5/SurToolSelect.cpp).
//
// The original is the heart of the editor: it shows the world object tree,
// the attribute editor for the current selection, and the selection box on
// the map. Without the world (Phase 3b) the attribute editor and the object
// tree have nothing to show; this port keeps the selection behaviour —
// left-drag a box, hover tracking — and draws the box as aux data. The
// attribute panel lands with Phase 3b when there are objects to select.

#pragma once

#include "editor/EditorTool.h"

class SelectTool : public EditorTool
{
public:
	SelectTool();

	const char* name() const override { return "Select"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onKeyDown(unsigned keyCode, bool shift, bool control, bool alt) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	// The engine side reads the finished selection drag (RenderViewWidget
	// turns it into EngineViewport::selectObjectsInRect): the box is a
	// click when start == end. isDragging() is true between LMB down/up.
	bool isDragging() const { return dragging_; }
	ToolVec2 boxStart() const { return boxStart_; }
	ToolVec2 boxEnd() const { return boxEnd_; }

private:
	// The drag selection box, in screen pixels (ends inclusive).
	bool dragging_ = false;
	ToolVec2 boxStart_;
	ToolVec2 boxEnd_;

	// Hover cursor (screen-space circle, CSurToolBase::drawCursorCircle).
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;
};
