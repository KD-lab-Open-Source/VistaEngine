// SourceTool.h — Qt port of CSurToolSource (SurMap5/SurToolSource.cpp).
//
// CSurToolSource was a CSurToolEditable: it picked a SourcesLibrary element,
// kept a live preview source under the cursor and stamped a saved copy on a
// left-click (sourceManager->addSource + setPose). The Properties panel
// (SourcePropertyPanel) lists the library elements and edits the picked one's
// parameters through the bridge's PropertyRow tree; the tool is engine-free.

#pragma once

#include "editor/EditorTool.h"

class SourceTool : public EditorTool
{
public:
	SourceTool();

	const char* name() const override { return "Source"; }

	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	void onActivate() override;
	void onDeactivate() override;

	// The chosen SourcesLibrary index (-1 = none).
	int sourceIndex() const { return sourceIndex_; }
	// Rebuild the live preview from the new index (and its edited parameters).
	void setSourceIndex(int i);

	// Push the edited element tree back into the library element and refresh
	// the preview so the change shows under the cursor.
	void applySourceEdit();

private:
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;

	int sourceIndex_ = -1;
};
