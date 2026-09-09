// GeoNetTool.h — Qt port of CSurToolGeoNet (SurMap5/SurToolGeoNet.cpp).
//
// The geo-net tool generates hills/relief by painting with a brush: each
// left-drag applies geoGeneration over the brush area at the cursor. The
// parameters (height, noise, mesh density) live in the Properties panel
// (GeoNetPropertyPanel); the tool itself is engine-free and reaches the
// terrain through the world bridge's applyGeoNet.

#pragma once

#include "editor/EditorTool.h"

class GeoNetTool : public EditorTool
{
public:
	GeoNetTool();

	const char* name() const override { return "GeoNet"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	// Parameters (read by the Properties panel).
	int height() const { return height_; }
	void setHeight(int h) { height_ = h; }
	int noise() const { return noise_; }
	void setNoise(int n) { noise_ = n; }
	int mesh() const { return mesh_; }
	void setMesh(int m) { mesh_ = m; }
	float brushRadius() const { return brushRadius_; }
	void setBrushRadius(float r) { brushRadius_ = r; }

private:
	// Apply the geo-net generation at a world point (the brush stroke).
	void applyAt(const ToolVec3& worldCoord);

	bool painting_ = false;
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;

	// CSurToolGeoNet defaults (SurToolGeoNet.cpp): height 0..MAX_VX_HEIGHT,
	// noise 0..100, mesh 20..4000.
	int height_ = 64;
	int noise_ = 100;
	int mesh_ = 1600;
	float brushRadius_ = 32.f;
};