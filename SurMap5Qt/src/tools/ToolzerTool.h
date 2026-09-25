// ToolzerTool.h — Qt port of CSurToolToolzer's terrain brush
// (SurMap5/SurToolToolzer.cpp).
//
// The toolzer raises or lowers the terrain under a circular brush: a left-drag
// keeps applying vMap.deltaZone at the cursor. Dig/put and the delta height come
// from the Properties panel (ToolzerPropertyPanel); the tool is engine-free and
// reaches the terrain through the world bridge's applyToolzer.
//
// The original also had square/Exp/PNoise/MPD variants and a height filter;
// only the circle Toolzer brush is ported here (the variant the default
// non-extended tool bar exposes).

#pragma once

#include "editor/EditorTool.h"

class ToolzerTool : public EditorTool
{
public:
	ToolzerTool();

	const char* name() const override { return "Toolzer"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	// Parameters (read by the Properties panel). deltaH_ is signed: negative
	// digs, positive puts. dig_ is the panel's radio (0 = dig, 1 = put) and
	// only affects the sign the panel writes into deltaH_.
	int deltaH() const { return deltaH_; }
	void setDeltaH(int h) { deltaH_ = h; }
	int smooth() const { return smooth_; }
	void setSmooth(int s) { smooth_ = s; }

	// The shared brush radius (ToolManager::setBrushRadius).
	float brushRadius() const { return brushRadius_; }
	void setBrushRadius(float r) { brushRadius_ = r; }

private:
	// Apply the toolzer at a world point (one stroke sample).
	void applyAt(const ToolVec3& worldCoord);

	bool painting_ = false;
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;

	// CSurToolToolzer defaults: deltaH 0 (the original started at 0 and the
	// user raised it), roughness/smooth 90, radius from the brush combo.
	int deltaH_ = 0;
	int smooth_ = 90;
	float brushRadius_ = 20.f;
};
