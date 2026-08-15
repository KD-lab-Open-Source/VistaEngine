// MoveTool.cpp — see header.

#include "MoveTool.h"

MoveTool::MoveTool()
{
	// CSurToolMove's ctor: X and Y axes active (move on the ground plane).
	setAxis(0);
}

bool MoveTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	cursorScreen_ = screenCoord;
	if(!buttonPressed_)
		return false;

	// CSurToolMove::onTrackingMouse: on the ground plane, move the selection
	// by (point - endPoint); on the Z axis, project onto the vertical line.
	// The delta is computed but not applied until there is a selection
	// (Phase 3b) — selectionCenter_/endPoint_ track it for the gizmo.
	const ToolVec3 delta{ worldCoord.x - endPoint_.x,
	                      worldCoord.y - endPoint_.y,
	                      worldCoord.z - endPoint_.z };
	endPoint_ = worldCoord;
	selectionCenter_.x += delta.x;
	selectionCenter_.y += delta.y;
	selectionCenter_.z += delta.z;
	return true;
}

bool MoveTool::onDrawAuxData(ToolAuxPainter& painter)
{
	drawAxis(painter);
	return true;
}

void MoveTool::beginTransformation()
{
	// CSurToolMove::beginTransformation: store the selection poses (Phase 3b).
}

void MoveTool::finishTransformation()
{
	// CSurToolMove::finishTransformation: commit the move (Phase 3b).
}
