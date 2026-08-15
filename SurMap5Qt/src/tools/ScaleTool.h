// ScaleTool.h — Qt port of CSurToolScale (SurMap5/SurToolScale.cpp).
//
// Scale stretches the selection from the selection centre along the active
// axis (uniform on the ground plane in the original). The scale factor comes
// from the drag distance; poses are empty until Phase 3b.

#pragma once

#include "TransformTool.h"

class ScaleTool : public TransformTool
{
public:
	ScaleTool();
	const char* name() const override { return "Scale"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

protected:
	void beginTransformation() override;
	void finishTransformation() override;

private:
	float startRadius_ = 1.f;    // distance from centre at drag start
	float scaleDelta_ = 1.f;     // accumulated scale factor
};
