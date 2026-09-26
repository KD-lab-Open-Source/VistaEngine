// EnvironmentTool.h — Qt port of CSurToolEnvironment / CSurTool3DM
// (SurMap5/SurTool3DM.cpp).
//
// CSurToolEnvironment placed UnitEnvironment objects (trees, buildings, ...)
// from a picked .3dx model: angle/scale/spread sliders, an EnvironmentType
// combo, spread/vertical checkboxes, and a live model preview that followed the
// cursor (terScene->CreateObject3dx models, not units). The Properties panel
// (EnvironmentPropertyPanel) edits EnvironmentParams; the tool is engine-free
// and reaches the world through the bridge.

#pragma once

#include "editor/EditorTool.h"

class EnvironmentTool : public EditorTool
{
public:
	EnvironmentTool();

	const char* name() const override { return "Environment"; }

	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	void onActivate() override;
	void onDeactivate() override;

	// The dialog's parameters (the panel edits these in place, then calls
	// applyParams()).
	EnvironmentParams& params() { return params_; }
	const EnvironmentParams& params() const { return params_; }
	// Rebuild/move the cursor preview from the current params (panel change).
	void applyParams();

	// The shared brush radius (CSurToolBase::getBrushRadius) — spread fills
	// this area.
	void setBrushRadius(float radius);

private:
	void refreshPreview(bool rebuild);

	EnvironmentParams params_;
	float brushRadius_ = 25.f;

	ToolVec3 lastWorld_;
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;
	bool active_ = false;
};
