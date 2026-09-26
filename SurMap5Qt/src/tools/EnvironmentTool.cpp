// EnvironmentTool.cpp — see header.

#include "EnvironmentTool.h"

namespace {
const unsigned kBrushColor = 0xFF90FF90; // green, distinct from unit/source/brushes
}

EnvironmentTool::EnvironmentTool()
{
	params_.brushRadius = brushRadius_;
}

void EnvironmentTool::onActivate()
{
	active_ = true;
	refreshPreview(true);
}

void EnvironmentTool::onDeactivate()
{
	active_ = false;
	hasCursor_ = false;
	if(bridge())
		bridge()->killEnvironmentPreview();
}

void EnvironmentTool::setBrushRadius(float radius)
{
	brushRadius_ = radius;
	params_.brushRadius = radius;
	if(active_ && hasCursor_)
		refreshPreview(true);
}

void EnvironmentTool::applyParams()
{
	params_.brushRadius = brushRadius_;
	refreshPreview(true);
}

void EnvironmentTool::refreshPreview(bool rebuild)
{
	if(!bridge() || !active_ || !hasCursor_)
		return;
	bridge()->updateEnvironmentPreview(params_, lastWorld_.x, lastWorld_.y, rebuild);
}

bool EnvironmentTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	lastWorld_ = worldCoord;
	if(bridge()){
		bridge()->placeEnvironment(params_, worldCoord.x, worldCoord.y);
		// The engine reseeds and rebuilds the spread layout after placement
		// (CSurToolEnvironment::onOperationOnMap); refresh the preview.
		refreshPreview(true);
	}
	return true;
}

bool EnvironmentTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	lastWorld_ = worldCoord;
	// Move the existing preview; do not rebuild it per motion event.
	refreshPreview(false);
	return true;
}

bool EnvironmentTool::onDrawAuxData(ToolAuxPainter& painter)
{
	// CSurToolBase::drawCursorCircle — the brush radius around the cursor.
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, (int)brushRadius_, kBrushColor);
	return hasCursor_;
}
