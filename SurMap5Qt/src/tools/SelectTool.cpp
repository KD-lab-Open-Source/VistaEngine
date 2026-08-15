// SelectTool.cpp — see header.

#include "SelectTool.h"

#include <algorithm>

namespace {
const unsigned kSelectionBoxColor = 0xFF60A0FF; // light blue
const unsigned kCursorCircleColor = 0xFFFFFFFF;
}

SelectTool::SelectTool() = default;

bool SelectTool::onTrackingMouse(const ToolVec3& /*worldCoord*/, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(dragging_)
		boxEnd_ = screenCoord;
	return true;
}

bool SelectTool::onLMBDown(const ToolVec3& /*worldCoord*/, const ToolVec2& screenCoord)
{
	// CSurToolSelect::onLMBDown began a selection box drag.
	dragging_ = true;
	boxStart_ = screenCoord;
	boxEnd_ = screenCoord;
	return true;
}

bool SelectTool::onLMBUp(const ToolVec3& /*worldCoord*/, const ToolVec2& screenCoord)
{
	// CSurToolSelect::onLMBUp finalized the box and selected inside it. With
	// no world (Phase 3b) the box is all there is to keep.
	dragging_ = false;
	boxEnd_ = screenCoord;
	return true;
}

bool SelectTool::onKeyDown(unsigned keyCode, bool /*shift*/, bool /*control*/, bool /*alt*/)
{
	// Escape cancels a drag box.
	if(keyCode == 0x01000000 /* Qt::Key_Escape */ && dragging_){
		dragging_ = false;
		return true;
	}
	return false;
}

bool SelectTool::onDrawAuxData(ToolAuxPainter& painter)
{
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, 6, kCursorCircleColor);

	if(dragging_){
		const ToolVec2 a(std::min(boxStart_.x, boxEnd_.x), std::min(boxStart_.y, boxEnd_.y));
		const ToolVec2 b(std::max(boxStart_.x, boxEnd_.x), std::max(boxStart_.y, boxEnd_.y));
		painter.drawRect2D(a, b, kSelectionBoxColor);
	}

	return hasCursor_ || dragging_;
}
