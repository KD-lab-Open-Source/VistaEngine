// ScaleTool.cpp — see header.

#include "ScaleTool.h"

#include <cmath>

ScaleTool::ScaleTool()
{
	// CSurToolScale's ctor: all three axes.
	setAxis(0);
}

bool ScaleTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	cursorScreen_ = screenCoord;
	if(!buttonPressed_)
		return false;

	// CSurToolScale::onTrackingMouse scaled by the ratio of the current to the
	// start distance from the selection centre. Screen-space stand-in: scale
	// by the vertical cursor delta, clamped positive.
	const float dy = float(worldCoord.y - endPoint_.y);
	endPoint_ = worldCoord;
	scaleDelta_ *= 1.0f + dy * 0.01f;
	if(scaleDelta_ < 0.05f)
		scaleDelta_ = 0.05f;
	return true;
}

bool ScaleTool::onDrawAuxData(ToolAuxPainter& painter)
{
	drawAxis(painter);
	return true;
}

void ScaleTool::beginTransformation()
{
	// CSurToolScale::beginTransformation: store poses + startRadius_ (Phase 3b).
	startRadius_ = 1.f;
	scaleDelta_ = 1.f;
}

void ScaleTool::finishTransformation()
{
	// CSurToolScale::finishTransformation: commit the scale (Phase 3b).
}
