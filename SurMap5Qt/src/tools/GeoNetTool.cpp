// GeoNetTool.cpp — see header.

#include "GeoNetTool.h"

namespace {
const unsigned kBrushColor = 0xFF60A0FF; // light blue, like the selection box
}

GeoNetTool::GeoNetTool() = default;

void GeoNetTool::applyAt(const ToolVec3& worldCoord)
{
	if(bridge())
		bridge()->applyGeoNet(worldCoord.x, worldCoord.y, brushRadius_,
		                      height_, noise_, mesh_);
}

bool GeoNetTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	// CSurToolGeoNet painted continuously while the button is held.
	if(painting_)
		applyAt(worldCoord);
	return true;
}

bool GeoNetTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	painting_ = true;
	applyAt(worldCoord);
	return true;
}

bool GeoNetTool::onLMBUp(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	painting_ = false;
	return true;
}

bool GeoNetTool::onDrawAuxData(ToolAuxPainter& painter)
{
	// CSurToolBase::drawCursorCircle — the brush radius around the cursor.
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, (int)brushRadius_, kBrushColor);
	return hasCursor_;
}