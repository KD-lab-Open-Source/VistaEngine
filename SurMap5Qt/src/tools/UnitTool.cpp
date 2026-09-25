// UnitTool.cpp — see header.

#include "UnitTool.h"

namespace {
const unsigned kBrushColor = 0xFFFFE060; // yellow, distinct from the brushes
const int kCursorRadius = 12;
}

UnitTool::UnitTool() = default;

bool UnitTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	if(bridge() && attributeIndex_ >= 0)
		bridge()->placeUnit(attributeIndex_, worldCoord.x, worldCoord.y, selectAfterPlace_);
	return true;
}

bool UnitTool::onTrackingMouse(const ToolVec3& /*worldCoord*/, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	return true;
}

bool UnitTool::onDrawAuxData(ToolAuxPainter& painter)
{
	// A small marker under the cursor (the original showed the unit itself on
	// the map; a circle is enough here).
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, kCursorRadius, kBrushColor);
	return hasCursor_;
}
