// MoveTool.cpp — see header.

#include "MoveTool.h"

namespace {
// Visitor that translates every selected object by delta (UniverseObjectActions::Move).
class MoveVisitor : public IEditorObjectVisitor
{
public:
	MoveVisitor(IWorldBridge* bridge, const ToolVec3& delta, bool init)
		: bridge_(bridge), delta_(delta), init_(init) {}
	void visit(EditorObjectId id) override
	{
		if(!bridge_)
			return;
		EditorPose pose = bridge_->objectPose(id);
		pose.pos.x += delta_.x;
		pose.pos.y += delta_.y;
		pose.pos.z += delta_.z;
		// UniverseObjectActions::Move: setPose(delta, init_) + awakePhysics.
		bridge_->setObjectPose(id, pose, init_);
		bridge_->awakePhysics(id);
	}
	IWorldBridge* bridge_;
	ToolVec3 delta_;
	bool init_;
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
	// The original's ground-plane call uses Move's default init=true (only the
	// Z-axis branch passes false), which updates the rigid body each frame — an
	// init=false pose is overwritten by UnitEnvironment::Quant/UnitReal physics.
	if(bridge()){
		MoveVisitor visitor(bridge(), delta, true);
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
	// CSurToolMove::onLMBUp: forEachSelected(Move(ZERO, true)) — re-apply the
	// current pose with init=true so the move is committed to the rigid body
	// (a plain init=false pose is overwritten by the next physics step, which
	// made units snap back to where they started).
	if(bridge()){
		MoveVisitor visitor(bridge(), ToolVec3{ 0, 0, 0 }, true);
		bridge()->forEachSelected(visitor);
	}
	// CSurToolTransform's commit (the poses were applied live in
	// onTrackingMouse; just refresh the gizmo).
	recomputeSelection();
}
