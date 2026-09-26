// UnitTool.h — Qt port of CSurToolUnit's unit placement (SurMap5/SurToolUnit.cpp).
//
// Picks an attribute from AttributeLibrary and places a unit on the terrain with
// a left-click (SurToolUnit::onOperationOnMap -> Player::buildUnit + setPose).
// CSurToolUnit kept a real auxiliary unit under the cursor (unitOnMouse_) that
// followed the mouse; the tool does the same through the bridge. The chosen
// attribute index comes from the Properties panel (UnitPropertyPanel) or the
// tools tree's Units catalog; the tool is engine-free.

#pragma once

#include "editor/EditorTool.h"

class UnitTool : public EditorTool
{
public:
	UnitTool();

	const char* name() const override { return "Unit"; }

	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	void onActivate() override;
	void onDeactivate() override;

	// The chosen AttributeLibrary index (-1 = none). Changing it rebuilds the
	// cursor preview unit.
	int attributeIndex() const { return attributeIndex_; }
	void setAttributeIndex(int i);

	bool selectAfterPlace() const { return selectAfterPlace_; }
	void setSelectAfterPlace(bool on) { selectAfterPlace_ = on; }

	// CSurToolUnit's angle sliders (calculateUnitPose): degrees + random spread.
	float angle() const { return angle_; }
	void setAngle(float a) { angle_ = a; previewValid_ = false; refreshPreview(); }
	float angleDelta() const { return angleDelta_; }
	void setAngleDelta(float d) { angleDelta_ = d; previewValid_ = false; refreshPreview(); }

private:
	void refreshPreview();

	ToolVec2 cursorScreen_;
	ToolVec3 lastWorld_;
	bool hasCursor_ = false;
	bool active_ = false;
	bool previewValid_ = false;

	int attributeIndex_ = -1;
	bool selectAfterPlace_ = true;
	float angle_ = 0.f;
	float angleDelta_ = 0.f;
};
