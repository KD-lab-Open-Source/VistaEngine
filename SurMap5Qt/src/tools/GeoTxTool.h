// GeoTxTool.h — Qt port of CSurToolGeoTx (SurMap5/SurToolGeoTx.cpp).
//
// The geo-texture tool lets the user pick two .tga texture files (the
// material and looped geo textures) and re-renders the world. The original's
// putNewGeoTexture call is commented out in both SurToolGeoTx.cpp and Terra,
// so the tool's map action is just vMap.WorldRender() — the texture files are
// held for the (future) texture application. Engine-free: reaches the world
// through the bridge's worldRender().

#pragma once

#include "editor/EditorTool.h"

#include <string>

class GeoTxTool : public EditorTool
{
public:
	GeoTxTool();

	const char* name() const override { return "GeoTx"; }

	bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord) override;
	bool onDrawAuxData(ToolAuxPainter& painter) override;

	// The two geo-texture file names (read/written by the Properties panel).
	const std::string& textureFile() const { return textureFile_; }
	void setTextureFile(const std::string& f) { textureFile_ = f; }
	const std::string& textureFile2() const { return textureFile2_; }
	void setTextureFile2(const std::string& f) { textureFile2_ = f; }

private:
	std::string textureFile_;
	std::string textureFile2_;
};