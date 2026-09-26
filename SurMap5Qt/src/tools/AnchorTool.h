// AnchorTool.h — Qt port of CSurToolAnchor (SurMap5/SurToolAnchor.cpp).
//
// Like SourceTool but without a library: the original edited one Anchor
// instance and stamped a copy on a left-click (sourceManager->addAnchor +
// setPose + a unique label). The Properties panel (AnchorPropertyPanel) edits
// that anchor's parameters through the bridge's PropertyRow tree.

#pragma once

#include "editor/EditorTool.h"

class AnchorTool : public EditorTool
{
public:
	AnchorTool();

	const char* name() const override { return "Anchor"; }

	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	void onActivate() override;
	void onDeactivate() override;

	// Rebuild the cursor preview from the edited parameters.
	void applyAnchorEdit();

private:
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;
};
