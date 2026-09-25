// UnitTool.h — Qt port of CSurToolUnit's unit placement (SurMap5/SurToolUnit.cpp).
//
// Picks an attribute from AttributeLibrary and places a unit on the terrain with
// a left-click (SurToolUnit::onOperationOnMap -> Player::buildUnit + setPose).
// The chosen attribute index comes from the Properties panel
// (UnitPropertyPanel), which lists the library's names through the bridge; the
// tool is engine-free and reaches the world through bridge()->placeUnit.

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

	// The chosen AttributeLibrary index (-1 = none).
	int attributeIndex() const { return attributeIndex_; }
	void setAttributeIndex(int i) { attributeIndex_ = i; }

	bool selectAfterPlace() const { return selectAfterPlace_; }
	void setSelectAfterPlace(bool on) { selectAfterPlace_ = on; }

private:
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;

	int attributeIndex_ = -1;
	bool selectAfterPlace_ = true;
};
