// TransformTool.cpp — see header.

#include "TransformTool.h"

#include <algorithm>

namespace {
const unsigned kAxisXColor = 0xFFFF4040;
const unsigned kAxisYColor = 0xFF40FF40;
const unsigned kAxisZColor = 0xFF4080FF;
const unsigned kAxisDisabledColor = 0xFF404040;
}

TransformTool::TransformTool() = default;

void TransformTool::setAxis(int index)
{
	// One active axis at a time (the original's radio trio OnXAxisCheck &
	// co.: the checked axis is the transform axis).
	axisIndex_ = std::clamp(index, 0, 2);
	for(int i = 0; i < 3; ++i)
		axisEnabled_[i] = (i == axisIndex_);
}

bool TransformTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	// CSurToolTransform::onLMBDown: beginTransformation + hold state.
	if(buttonPressed_)
		return false;
	buttonPressed_ = true;
	startPoint_ = worldCoord;
	endPoint_ = worldCoord;
	cursorScreen_ = screenCoord;
	beginTransformation();
	return true;
}

bool TransformTool::onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	if(!buttonPressed_)
		return false;
	endPoint_ = worldCoord;
	cursorScreen_ = screenCoord;
	buttonPressed_ = false;
	finishTransformation();
	return true;
}

bool TransformTool::onRMBDown(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	// CSurToolTransform::onRMBDown: cancel the in-progress transformation.
	if(buttonPressed_){
		buttonPressed_ = false;
		finishTransformation();   // TODO: revert to the stored poses (Phase 3b)
		return true;
	}
	return false;
}

bool TransformTool::onRMBUp(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	return false;
}

bool TransformTool::onKeyDown(unsigned keyCode, bool /*shift*/, bool /*control*/, bool /*alt*/)
{
	// Escape cancels the drag (cancelTransformation in the original).
	if(keyCode == 0x01000000 /* Qt::Key_Escape */ && buttonPressed_){
		buttonPressed_ = false;
		finishTransformation();
		return true;
	}
	return false;
}

void TransformTool::quant(float /*dt*/)
{
	// CSurToolTransform's per-frame work (pose interpolation) happens in
	// onTrackingMouse in the originals; nothing to integrate until Phase 3b.
}

bool TransformTool::onDrawAuxData(ToolAuxPainter& painter)
{
	drawAxis(painter);
	return true;
}

void TransformTool::drawAxis(ToolAuxPainter& painter)
{
	// CSurToolTransform::drawAxis drew the three gizmo lines from the
	// selection centre, sized by selectionRadius_. Screen-space stand-in:
	// three short coloured rays. (A proper 3D gizmo needs the view matrix,
	// which lands with the engine drawing pass in Phase 3b.)
	const int len = std::max(8, (int)(selectionRadius_ / 8.0f));
	ToolVec2 base = cursorScreen_;
	if(buttonPressed_){
		// During a drag, draw from the (screen) start toward the cursor.
		base = cursorScreen_;
	}
	const ToolVec2 x(base.x + len, base.y);
	const ToolVec2 y(base.x, base.y + len);
	const ToolVec2 z(base.x - len / 2, base.y - len / 2);
	painter.drawLine2D(base, x, axisEnabled_[0] ? kAxisXColor : kAxisDisabledColor);
	painter.drawLine2D(base, y, axisEnabled_[1] ? kAxisYColor : kAxisDisabledColor);
	painter.drawLine2D(base, z, axisEnabled_[2] ? kAxisZColor : kAxisDisabledColor);
}
