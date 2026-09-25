// ColorPicTool.cpp — see header.

#include "ColorPicTool.h"

namespace {
const unsigned kBrushColor = 0xFFFF60C0; // pink, distinct from the other brushes
}

ColorPicTool::ColorPicTool() = default;

void ColorPicTool::applyAt(const ToolVec3& worldCoord)
{
	if(!bridge() || texturePath_.empty())
		return;
	const int r = (tintRGB_ >> 16) & 0xff;
	const int g = (tintRGB_ >> 8) & 0xff;
	const int b = tintRGB_ & 0xff;
	bridge()->applyTexturePaint(worldCoord.x, worldCoord.y, brushRadius_,
	                            texturePath_, centerAlpha_, kColor_,
	                            saturation_, brightness_, r, g, b, 0, 0);
}

bool ColorPicTool::putToAllWorld()
{
	if(!bridge() || texturePath_.empty())
		return false;
	const int r = (tintRGB_ >> 16) & 0xff;
	const int g = (tintRGB_ >> 8) & 0xff;
	const int b = tintRGB_ & 0xff;
	return bridge()->putTextureToAllWorld(texturePath_, kColor_, saturation_,
	                                      brightness_, r, g, b, 0, 0);
}

bool ColorPicTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(painting_)
		applyAt(worldCoord);
	return true;
}

bool ColorPicTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	painting_ = true;
	applyAt(worldCoord);
	return true;
}

bool ColorPicTool::onLMBUp(const ToolVec3& /*worldCoord*/, const ToolVec2& /*screenCoord*/)
{
	painting_ = false;
	return true;
}

bool ColorPicTool::onDrawAuxData(ToolAuxPainter& painter)
{
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, (int)brushRadius_, kBrushColor);
	return hasCursor_;
}
