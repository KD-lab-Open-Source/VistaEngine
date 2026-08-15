// MoveTool.h — Qt port of CSurToolMove (SurMap5/SurToolMove.cpp).
//
// Move drags the selection along the ground plane (or the Z axis when the Z
// axis is active). Without the world there is no selection to move; the tool
// keeps the drag bookkeeping and applies nothing yet — the delta computation
// (screenPointToGround / projectScreenPointOnPlane) lands with Phase 3b.

#pragma once

#include "TransformTool.h"

class MoveTool : public TransformTool
{
public:
	MoveTool();
	const char* name() const override { return "Move"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

protected:
	void beginTransformation() override;
	void finishTransformation() override;
};
