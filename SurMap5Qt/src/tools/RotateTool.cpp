// RotateTool.cpp — see header.

#include "RotateTool.h"

#include <algorithm>
#include <cmath>

namespace {
// Rotate every selected object by `angle` around `axis` through `origin`
// (UniverseObjectActions::RotateAroundPoint). The quaternion is built from
// the axis-angle and applied to the object's orientation; the position is
// rotated around the origin.
class RotateVisitor : public IEditorObjectVisitor
{
public:
	RotateVisitor(IWorldBridge* bridge, const ToolVec3& origin, const ToolVec3& axis, float angle)
		: bridge_(bridge), origin_(origin), axis_(axis), angle_(angle) {}
	void visit(EditorObjectId id) override
	{
		if(!bridge_)
			return;
		EditorPose pose = bridge_->objectPose(id);
		// Rotate the position around the origin.
		ToolVec3 p{ pose.pos.x - origin_.x, pose.pos.y - origin_.y, pose.pos.z - origin_.z };
		p = rotate(p);
		pose.pos = ToolVec3{ p.x + origin_.x, p.y + origin_.y, p.z + origin_.z };
		// Rotate the orientation by the same axis-angle (premult).
		pose = rotateOrientation(pose);
		bridge_->setObjectPose(id, pose, true);
	}
	// Rotate a vector around axis_ by angle_ (Rodrigues' rotation formula).
	ToolVec3 rotate(const ToolVec3& v) const
	{
		const float c = std::cos(angle_), s = std::sin(angle_);
		const float dot = v.x * axis_.x + v.y * axis_.y + v.z * axis_.z;
		ToolVec3 cross{ axis_.y * v.z - axis_.z * v.y,
		                axis_.z * v.x - axis_.x * v.z,
		                axis_.x * v.y - axis_.y * v.x };
		return ToolVec3{ v.x * c + cross.x * s + axis_.x * dot * (1.f - c),
		                 v.y * c + cross.y * s + axis_.y * dot * (1.f - c),
		                 v.z * c + cross.z * s + axis_.z * dot * (1.f - c) };
	}
	// Rotate the orientation quaternion by the axis-angle (premultiply).
	EditorPose rotateOrientation(const EditorPose& pose) const
	{
		// Build the rotation quaternion q = (cos(a/2), axis*sin(a/2)).
		const float half = angle_ * 0.5f;
		const float c = std::cos(half), s = std::sin(half);
		const float qw = c, qx = axis_.x * s, qy = axis_.y * s, qz = axis_.z * s;
		// q * p (Hamilton product).
		const float pw = pose.ow, px = pose.ox, py = pose.oy, pz = pose.oz;
		EditorPose r = pose;
		r.ow = qw * pw - qx * px - qy * py - qz * pz;
		r.ox = qw * px + qx * pw + qy * pz - qz * py;
		r.oy = qw * py - qx * pz + qy * pw + qz * px;
		r.oz = qw * pz + qx * py - qy * px + qz * pw;
		return r;
	}
	IWorldBridge* bridge_;
	ToolVec3 origin_;
	ToolVec3 axis_;
	float angle_;
};
} // namespace

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

	// CSurToolRotate::onTrackingMouse: the angle is the sweep of the ground
	// vector around the selection centre, projected onto the active axis.
	// Compute the angle from the start/current vectors around the axis.
	const ToolVec3 a{ startPoint_.x - selectionCenter_.x,
	                  startPoint_.y - selectionCenter_.y,
	                  startPoint_.z - selectionCenter_.z };
	const ToolVec3 b{ worldCoord.x - selectionCenter_.x,
	                  worldCoord.y - selectionCenter_.y,
	                  worldCoord.z - selectionCenter_.z };
	const float la = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	const float lb = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
	float angle = 0.f;
	if(la > 1e-6f && lb > 1e-6f){
		const float dot = (a.x * b.x + a.y * b.y + a.z * b.z) / (la * lb);
		angle = std::acos(std::clamp(dot, -1.f, 1.f));
		// Sign from the cross product's component along the active axis.
		const float cx = a.y * b.z - a.z * b.y;
		const float cy = a.z * b.x - a.x * b.z;
		const float cz = a.x * b.y - a.y * b.x;
		const float sign = (axis() == 0) ? cx : (axis() == 1) ? cy : cz;
		if(sign < 0.f)
			angle = -angle;
	}
	endPoint_ = worldCoord;
	angleDelta_ = angle;

	// Restore the stored poses, then apply the rotation (the original did
	// RestorePose(poses_, true) then RotateAroundPoint each frame).
	restorePoses();
	if(bridge()){
		ToolVec3 axisVec{ 0, 0, 0 };
		if(axis() == 0) axisVec.x = 1.f;
		else if(axis() == 1) axisVec.y = 1.f;
		else axisVec.z = 1.f;
		RotateVisitor visitor(bridge(), selectionCenter_, axisVec, angleDelta_);
		bridge()->forEachSelected(visitor);
	}
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
	// CSurToolRotate::beginTransformation: store poses + start angle.
	storePoses();
	recomputeSelection();
	startPoint_ = endPoint_;
	angleDelta_ = 0.f;
}

void RotateTool::finishTransformation()
{
	// CSurToolRotate::finishTransformation: commit the rotation (already
	// applied live in onTrackingMouse); refresh the gizmo.
	recomputeSelection();
}
