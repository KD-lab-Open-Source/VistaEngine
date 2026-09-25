// ToolzerTool.cpp — see header.

#include "ToolzerTool.h"

namespace {
const unsigned kBrushColor = 0xFFFFC060; // amber, distinct from the GeoNet circle
}

ToolzerTool::ToolzerTool() = default;

void ToolzerTool::applyAt(const ToolVec3& worldCoord)
{
	if(bridge())
		bridge()->applyToolzer(worldCoord.x, worldCoord.y, brushRadius_,
		                       deltaH_, smooth_, 0, 0);
}

bool ToolzerTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	// The original painted continuously while the button was held.
	if(painting_)
		applyAt(worldCoord);
	return true;
}

bool ToolzerTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	painting_ = true;
	applyAt(worldCoord);
	return true;
}

bool ToolzerTool::onLMBUp(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	painting_ = false;
	return true;
}

bool ToolzerTool::onDrawAuxData(ToolAuxPainter& painter)
{
	// CSurToolBase::drawCursorCircle — the brush radius around the cursor.
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, (int)brushRadius_, kBrushColor);
	return hasCursor_;
}
