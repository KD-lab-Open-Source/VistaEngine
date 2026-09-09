// ScaleTool.cpp — see header.

#include "ScaleTool.h"

#include <cmath>

namespace {
// Scale every selected object by `scale` from `origin` (ScaleOnCenter): the
// position is scaled around the origin and the radius is multiplied.
class ScaleVisitor : public IEditorObjectVisitor
{
public:
	ScaleVisitor(IWorldBridge* bridge, const ToolVec3& origin, float scale)
		: bridge_(bridge), origin_(origin), scale_(scale) {}
	void visit(EditorObjectId id) override
	{
		if(!bridge_)
			return;
		EditorPose pose = bridge_->objectPose(id);
		pose.pos.x = origin_.x + (pose.pos.x - origin_.x) * scale_;
		pose.pos.y = origin_.y + (pose.pos.y - origin_.y) * scale_;
		pose.pos.z = origin_.z + (pose.pos.z - origin_.z) * scale_;
		bridge_->setObjectPose(id, pose, true);
		bridge_->setObjectRadius(id, bridge_->objectRadius(id) * scale_);
	}
	IWorldBridge* bridge_;
	ToolVec3 origin_;
	float scale_;
};
} // namespace

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
	// start distance from the selection centre.
	const float startDist = std::sqrt(startPoint_.x * startPoint_.x +
	                                  startPoint_.y * startPoint_.y +
	                                  startPoint_.z * startPoint_.z);
	const float curDist = std::sqrt(worldCoord.x * worldCoord.x +
	                                worldCoord.y * worldCoord.y +
	                                worldCoord.z * worldCoord.z);
	float scale = (startDist > 1e-6f) ? (curDist / startDist) : 1.f;
	if(scale < 0.05f)
		scale = 0.05f;
	endPoint_ = worldCoord;
	scaleDelta_ = scale;

	// Restore the stored poses, then apply the scale (the original did
	// RestorePose(poses_, false) then ScaleOnCenter each frame).
	restorePoses();
	if(bridge()){
		ScaleVisitor visitor(bridge(), selectionCenter_, scaleDelta_);
		bridge()->forEachSelected(visitor);
	}
	return true;
}

bool ScaleTool::onDrawAuxData(ToolAuxPainter& painter)
{
	drawAxis(painter);
	return true;
}

void ScaleTool::beginTransformation()
{
	// CSurToolScale::beginTransformation: store poses + startRadius_.
	storePoses();
	recomputeSelection();
	startPoint_ = endPoint_;
	scaleDelta_ = 1.f;
}

void ScaleTool::finishTransformation()
{
	// CSurToolScale::finishTransformation: commit the scale (already applied
	// live in onTrackingMouse); refresh the gizmo.
	recomputeSelection();
}
