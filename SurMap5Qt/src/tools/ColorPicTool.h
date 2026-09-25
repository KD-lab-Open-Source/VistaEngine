// ColorPicTool.h — Qt port of CSurToolColorPic's texture brush
// (SurMap5/SurToolColorPic.cpp).
//
// Paints a bitmap texture (Resource\TerrainData\Pictures\*.tga) on the terrain
// under a circular brush, tinted by a ColorModificator: a left-drag keeps
// calling vMap.drawBitmapCircle. The texture path and the tint/modifier values
// come from the Properties panel (ColorPicPropertyPanel); the tool is
// engine-free and reaches the terrain through the world bridge's
// applyTexturePaint / putTextureToAllWorld.
//
// The original's live texture preview (UpdateTexture/onDrawPreview) is not
// ported: the panel shows the file name and the modifiers, not a rendered
// swatch.

#pragma once

#include <string>

#include "editor/EditorTool.h"

class ColorPicTool : public EditorTool
{
public:
	ColorPicTool();

	const char* name() const override { return "Texture"; }

	bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	// Parameters (read/written by the Properties panel).
	const std::string& texturePath() const { return texturePath_; }
	void setTexturePath(const std::string& p) { texturePath_ = p; }
	int centerAlpha() const { return centerAlpha_; }
	void setCenterAlpha(int a) { centerAlpha_ = a; }
	int kColor() const { return kColor_; }
	void setKColor(int k) { kColor_ = k; }
	int saturation() const { return saturation_; }
	void setSaturation(int s) { saturation_ = s; }
	int brightness() const { return brightness_; }
	void setBrightness(int b) { brightness_ = b; }
	unsigned tintRGB() const { return tintRGB_; }
	void setTintRGB(unsigned rgb) { tintRGB_ = rgb; }

	float brushRadius() const { return brushRadius_; }
	void setBrushRadius(float r) { brushRadius_ = r; }

	// Apply the texture to the whole world (the original's Put2World button).
	// Returns false when there is no texture/bridge/world.
	bool putToAllWorld();

private:
	void applyAt(const ToolVec3& worldCoord);

	bool painting_ = false;
	ToolVec2 cursorScreen_;
	bool hasCursor_ = false;

	// CSurToolColorPic defaults: no file, centre alpha 0, K 0, saturation/
	// brightness 100, tint white.
	std::string texturePath_;
	int centerAlpha_ = 0;
	int kColor_ = 0;
	int saturation_ = 100;
	int brightness_ = 100;
	unsigned tintRGB_ = 0xFFFFFF;
	float brushRadius_ = 20.f;
};
