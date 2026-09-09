// MoveTool.cpp — see header.

#include "MoveTool.h"

namespace {
// Visitor that translates every selected object by delta (UniverseObjectActions::Move).
class MoveVisitor : public IEditorObjectVisitor
{
public:
	MoveVisitor(IWorldBridge* bridge, const ToolVec3& delta) : bridge_(bridge), delta_(delta) {}
	void visit(EditorObjectId id) override
	{
		if(!bridge_)
			return;
		EditorPose pose = bridge_->objectPose(id);
		pose.pos.x += delta_.x;
		pose.pos.y += delta_.y;
		pose.pos.z += delta_.z;
		bridge_->setObjectPose(id, pose, false);
	}
	IWorldBridge* bridge_;
	ToolVec3 delta_;
};
} // namespace

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
	const ToolVec3 delta{ worldCoord.x - endPoint_.x,
	                      worldCoord.y - endPoint_.y,
	                      worldCoord.z - endPoint_.z };
	endPoint_ = worldCoord;
	selectionCenter_.x += delta.x;
	selectionCenter_.y += delta.y;
	selectionCenter_.z += delta.z;

	// Apply the delta to every selected object (forEachSelected(Move(delta))).
	if(bridge()){
		MoveVisitor visitor(bridge(), delta);
		bridge()->forEachSelected(visitor);
	}
	return true;
}

bool MoveTool::onDrawAuxData(ToolAuxPainter& painter)
{
	drawAxis(painter);
	return true;
}

void MoveTool::beginTransformation()
{
	// CSurToolMove::beginTransformation: store the selection poses.
	storePoses();
	recomputeSelection();
}

void MoveTool::finishTransformation()
{
	// CSurToolMove::finishTransformation: commit the move (the poses were
	// already applied live in onTrackingMouse; just refresh the gizmo).
	recomputeSelection();
}
