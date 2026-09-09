// GeoTxTool.cpp — see header.

#include "GeoTxTool.h"

GeoTxTool::GeoTxTool() = default;

bool GeoTxTool::onLMBDown(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	// CSurToolGeoTx::onOperationOnMap: re-render the world (the texture
	// application itself is commented out in the original).
	if(bridge())
		bridge()->worldRender();
	return true;
}

bool GeoTxTool::onDrawAuxData(ToolAuxPainter& /*painter*/)
{
	return false;
}