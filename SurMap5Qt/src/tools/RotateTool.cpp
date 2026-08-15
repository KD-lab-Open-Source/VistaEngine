// RotateTool.cpp — see header.

#include "RotateTool.h"

#include <cmath>

RotateTool::RotateTool()
{
	// CSurToolRotate's ctor: Z axis by default.
	setAxis(2);
}

bool RotateTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	cursorScreen_ = screenCoord;
	if(!buttonPressed_)
		return false;

	// The original computed the rotation from the drag angle around the
	// selection centre (CSurToolRotate::onTrackingMouse: atan2 of the ground
	// vectors). Screen-space stand-in until the world exists: rotate by the
	// horizontal cursor delta.
	static const float kDeg2Rad = 3.14159265f / 180.f;
	const float dx = float(worldCoord.x - endPoint_.x);
	endPoint_ = worldCoord;
	angleDelta_ += dx * 0.5f * kDeg2Rad;   // 0.5 deg per unit of ground motion
	return true;
}

bool RotateTool::onDrawAuxData(ToolAuxPainter& painter)
{
	drawAxis(painter);
	// CSurToolRotate::onDrawAuxData drew a circle through the selection
	// (drawCircle(selectionCenter_, selectionRadius_)). Screen-space circle at
	// the cursor for now.
	painter.drawCircle2D(cursorScreen_, (int)selectionRadius_ / 4, 0xFFC0C0C0);
	return true;
}

void RotateTool::beginTransformation()
{
	// CSurToolRotate::beginTransformation: store poses + start angle (Phase 3b).
	startAngle_ = 0.f;
	angleDelta_ = 0.f;
}

void RotateTool::finishTransformation()
{
	// CSurToolRotate::finishTransformation: commit the rotation (Phase 3b).
}
