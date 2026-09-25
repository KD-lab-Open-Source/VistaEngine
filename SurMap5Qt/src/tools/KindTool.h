// KindTool.h — Qt port of CSurToolKind's surface-kind brush
// (SurMap5/SurToolKind.cpp).
//
// Paints the terrain type under a circular brush: a left-drag keeps calling
// vMap.drawInGrid with GRIDAT_MASK_SURFACE_KIND. The type (0..15) is chosen in
// the Properties panel (KindPropertyPanel); the tool is engine-free and reaches
// the terrain through the world bridge's applySurKind.
//
// The original also painted hardness/impassability (the commented-out cases in
// onOperationOnMap) — only the surface-kind path is live in the reference too.

#pragma once

#include "editor/EditorTool.h"

class KindTool : public EditorTool
{
public:
	KindTool();

	const char* name() const override { return "Hardness"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	void onActivate() override;
	void onDeactivate() override;

	// The chosen surface kind (0..TERRAIN_TYPES_NUMBER-1, 16 here).
	int kind() const { return kind_; }
	void setKind(int k) { kind_ = k; }

	float brushRadius() const { return brushRadius_; }
	void setBrushRadius(float r) { brushRadius_ = r; }

private:
	void applyAt(const ToolVec3& worldCoord);

	bool painting_ = false;
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;

	int kind_ = 0;
	float brushRadius_ = 20.f;
};
