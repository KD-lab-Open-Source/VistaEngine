// KindTool.cpp — see header.

#include "KindTool.h"

namespace {
const unsigned kBrushColor = 0xFF60FF60; // green, distinct from Toolzer/GeoNet
}

KindTool::KindTool() = default;

void KindTool::applyAt(const ToolVec3& worldCoord)
{
	if(bridge())
		bridge()->applySurKind(worldCoord.x, worldCoord.y, brushRadius_,
		                       kind_, 0, 0);
}

void KindTool::onActivate()
{
	// SurToolKind::OnInitDialog turned the surface-kind tint on while the tool
	// was active.
	if(bridge())
		bridge()->setShowSurKind(true);
}

void KindTool::onDeactivate()
{
	// ... and OnDestroy cleared it.
	if(bridge())
		bridge()->setShowSurKind(false);
}

bool KindTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(painting_)
		applyAt(worldCoord);
	return true;
}

bool KindTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	painting_ = true;
	applyAt(worldCoord);
	return true;
}

bool KindTool::onLMBUp(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	painting_ = false;
	return true;
}

bool KindTool::onDrawAuxData(ToolAuxPainter& painter)
{
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, (int)brushRadius_, kBrushColor);
	return hasCursor_;
}
