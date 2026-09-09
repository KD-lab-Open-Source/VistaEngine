// TransformTool.cpp — see header.

#include "TransformTool.h"

#include <algorithm>
#include <cmath>

namespace {
const unsigned kAxisXColor = 0xFFFF4040;
const unsigned kAxisYColor = 0xFF40FF40;
const unsigned kAxisZColor = 0xFF4080FF;
const unsigned kAxisDisabledColor = 0xFF404040;

// Visitor that snapshots the selected objects' poses (StorePose).
class PoseStoreVisitor : public IEditorObjectVisitor
{
public:
	PoseStoreVisitor(IWorldBridge* bridge, std::vector<std::pair<EditorObjectId, EditorPose>>& out)
		: bridge_(bridge), out_(out) {}
	void visit(EditorObjectId id) override
	{
		if(bridge_)
			out_.push_back({ id, bridge_->objectPose(id) });
	}
	IWorldBridge* bridge_;
	std::vector<std::pair<EditorObjectId, EditorPose>>& out_;
};

// Visitor that restores poses from a snapshot (RestorePose).
class PoseRestoreVisitor : public IEditorObjectVisitor
{
public:
	PoseRestoreVisitor(IWorldBridge* bridge, std::vector<std::pair<EditorObjectId, EditorPose>>& poses)
		: bridge_(bridge), poses_(poses) {}
	void visit(EditorObjectId id) override
	{
		if(!bridge_)
			return;
		for(const auto& entry : poses_)
			if(entry.first == id){
				bridge_->setObjectPose(id, entry.second, true);
				break;
			}
	}
	IWorldBridge* bridge_;
	std::vector<std::pair<EditorObjectId, EditorPose>>& poses_;
};

// Visitor that computes the selection's AABB centre + radius (RadiusExtractor).
class SelectionExtractVisitor : public IEditorObjectVisitor
{
public:
	SelectionExtractVisitor(IWorldBridge* bridge) : bridge_(bridge) {}
	void visit(EditorObjectId id) override
	{
		if(!bridge_)
			return;
		const EditorPose pose = bridge_->objectPose(id);
		const float r = bridge_->objectRadius(id);
		++count_;
		if(count_ == 1){
			minX_ = maxX_ = pose.pos.x;
			minY_ = maxY_ = pose.pos.y;
			firstRadius_ = r;
		}
		else{
			minX_ = std::min(minX_, pose.pos.x); maxX_ = std::max(maxX_, pose.pos.x);
			minY_ = std::min(minY_, pose.pos.y); maxY_ = std::max(maxY_, pose.pos.y);
		}
	}
	IWorldBridge* bridge_;
	int count_ = 0;
	float minX_ = 0, maxX_ = 0, minY_ = 0, maxY_ = 0, firstRadius_ = 0;
};

} // namespace

TransformTool::TransformTool() = default;

void TransformTool::setAxis(int index)
{
	// One active axis at a time (the original's radio trio OnXAxisCheck &
	// co.: the checked axis is the transform axis).
	axisIndex_ = std::clamp(index, 0, 2);
	for(int i = 0; i < 3; ++i)
		axisEnabled_[i] = (i == axisIndex_);
}

void TransformTool::storePoses()
{
	poses_.clear();
	if(bridge()){
		PoseStoreVisitor visitor(bridge(), poses_);
		bridge()->forEachSelected(visitor);
	}
}

void TransformTool::restorePoses()
{
	if(bridge()){
		PoseRestoreVisitor visitor(bridge(), poses_);
		bridge()->forEachSelected(visitor);
	}
}

void TransformTool::recomputeSelection()
{
	selectionCenter_ = ToolVec3{ 0, 0, 0 };
	selectionRadius_ = 100.0f;
	if(!bridge())
		return;
	SelectionExtractVisitor extract(bridge());
	bridge()->forEachSelected(extract);
	if(extract.count_ == 0)
		return;
	selectionCenter_ = ToolVec3{ (extract.minX_ + extract.maxX_) * 0.5f,
	                             (extract.minY_ + extract.maxY_) * 0.5f, 0 };
	const float dx = extract.maxX_ - extract.minX_;
	const float dy = extract.maxY_ - extract.minY_;
	const float diag = std::sqrt(dx * dx + dy * dy);
	selectionRadius_ = diag > 1e-6f ? diag * 0.5f : extract.firstRadius_;
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
		restorePoses();   // revert to the stored poses (cancelTransformation)
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
		restorePoses();
		return true;
	}
	return false;
}

void TransformTool::quant(float /*dt*/)
{
	// CSurToolTransform's per-frame work (pose interpolation) happens in
	// onTrackingMouse in the originals; nothing to integrate until Phase 3b.
}

void TransformTool::onSelectionChanged()
{
	// The selection changed elsewhere (Select tool, delete): refresh the
	// gizmo centre/radius and drop any stale pose snapshot.
	poses_.clear();
	recomputeSelection();
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
