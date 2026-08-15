// TransformTool.h — Qt port of CSurToolTransform (SurMap5/SurToolTransform.h),
// the base of the Move/Rotate/Scale tools.
//
// The original stores the selection's poses (std::vector<PoseRadius>) and
// drags them through the transform tools' onTrackingMouse. The engine side
// (forEachSelected over BaseUniverseObject) cannot exist until the world loads
// (Phase 3b), so the pose storage here is the tool's own bookkeeping of a
// "selection" — a centre and a radius — ready to be wired to real objects.
//
// Tools implement beginTransformation()/finishTransformation() and the
// onTrackingMouse drag, exactly like the MFC trio.

#pragma once

#include "editor/EditorTool.h"

class TransformTool : public EditorTool
{
public:
	TransformTool();

	// CSurToolTransform's drag lifecycle.
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onRMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onRMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onKeyDown(unsigned keyCode, bool shift, bool control, bool alt) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;
	void quant(float dt) override;

	// The transform axis (transformAxis_): 0=X, 1=Y, 2=Z.
	int axis() const { return axisIndex_; }
	void setAxis(int index);

protected:
	// Selection state (PoseRadius-stand-ins; filled by Phase 3b's real
	// selection).
	ToolVec3 selectionCenter_;
	float   selectionRadius_ = 100.0f;

	// Drag state.
	bool    buttonPressed_ = false;
	ToolVec2 cursorScreen_;
	ToolVec3 startPoint_;   // ground point where the drag began
	ToolVec3 endPoint_;     // current ground point

	virtual void beginTransformation() {}
	virtual void finishTransformation() {}

	// Draw the transform axis gizmo at selectionCenter_ (drawAxis).
	void drawAxis(ToolAuxPainter& painter);

private:
	int axisIndex_ = 0;   // 0=X, 1=Y, 2=Z — matches transformAxis_[]
	bool axisEnabled_[3] = { true, true, false };
};
