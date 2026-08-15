// RotateTool.h — Qt port of CSurToolRotate (SurMap5/SurToolRotate.cpp).
//
// Rotate spins the selection around the active axis through the selection
// centre. The angle is derived from the drag (original: angle between the
// start and current ground vectors around the axis); the rotation is applied
// to the stored poses, which are empty until Phase 3b.

#pragma once

#include "TransformTool.h"

class RotateTool : public TransformTool
{
public:
	RotateTool();
	const char* name() const override { return "Rotate"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

protected:
	void beginTransformation() override;
	void finishTransformation() override;

private:
	float startAngle_ = 0.f;   // angle of startPoint_ around the axis
	float angleDelta_ = 0.f;   // accumulated rotation
};
