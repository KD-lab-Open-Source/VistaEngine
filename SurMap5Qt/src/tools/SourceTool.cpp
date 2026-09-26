// SourceTool.cpp — see header.

#include "SourceTool.h"

namespace {
const unsigned kBrushColor = 0xFFFFB060; // orange, distinct from unit/brushes
const int kCursorRadius = 14;
}

SourceTool::SourceTool() = default;

void SourceTool::onActivate()
{
	hasCursor_ = false;
	if(bridge() && sourceIndex_ >= 0)
		bridge()->previewSource(sourceIndex_);
}

void SourceTool::onDeactivate()
{
	hasCursor_ = false;
	if(bridge())
		bridge()->previewSource(-1);   // kill the preview
}

void SourceTool::setSourceIndex(int i)
{
	sourceIndex_ = i;
	if(bridge())
		bridge()->previewSource(i);
}

void SourceTool::applySourceEdit()
{
	if(bridge() && sourceIndex_ >= 0)
		bridge()->previewSource(sourceIndex_);   // recreate from edited params
}

bool SourceTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(bridge() && sourceIndex_ >= 0)
		bridge()->placeSource(sourceIndex_, worldCoord.x, worldCoord.y);
	return true;
}

bool SourceTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(bridge())
		bridge()->movePreviewSource(worldCoord.x, worldCoord.y);
	return true;
}

bool SourceTool::onDrawAuxData(ToolAuxPainter& painter)
{
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, kCursorRadius, kBrushColor);
	return hasCursor_;
}
