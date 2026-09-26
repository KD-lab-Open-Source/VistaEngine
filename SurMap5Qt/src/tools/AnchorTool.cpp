// AnchorTool.cpp — see header.

#include "AnchorTool.h"

namespace {
const unsigned kBrushColor = 0xFFB0B0FF; // blue, distinct from the other tools
const int kCursorRadius = 14;
}

AnchorTool::AnchorTool() = default;

void AnchorTool::onActivate()
{
	hasCursor_ = false;
	if(bridge())
		bridge()->previewAnchor(true);
}

void AnchorTool::onDeactivate()
{
	hasCursor_ = false;
	if(bridge())
		bridge()->previewAnchor(false);   // remove the preview
}

void AnchorTool::applyAnchorEdit()
{
	if(bridge())
		bridge()->previewAnchor(true);   // recreate from edited params
}

bool AnchorTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(bridge())
		bridge()->placeAnchor(worldCoord.x, worldCoord.y);
	return true;
}

bool AnchorTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(bridge())
		bridge()->movePreviewAnchor(worldCoord.x, worldCoord.y);
	return true;
}

bool AnchorTool::onDrawAuxData(ToolAuxPainter& painter)
{
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, kCursorRadius, kBrushColor);
	return hasCursor_;
}
