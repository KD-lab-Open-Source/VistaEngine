// UnitTool.cpp — see header.

#include "UnitTool.h"

namespace {
const unsigned kBrushColor = 0xFFFFE060; // yellow, distinct from the brushes
const int kCursorRadius = 12;
}

UnitTool::UnitTool() = default;

void UnitTool::onActivate()
{
	active_ = true;
	refreshPreview();
}

void UnitTool::onDeactivate()
{
	active_ = false;
	hasCursor_ = false;
	previewValid_ = false;
	if(bridge())
		bridge()->killPreviewUnit();
}

void UnitTool::setAttributeIndex(int i)
{
	attributeIndex_ = i;
	previewValid_ = false;
	refreshPreview();
}

void UnitTool::refreshPreview()
{
	if(!bridge() || !active_ || !hasCursor_ || attributeIndex_ < 0)
		return;
	bridge()->previewUnit(attributeIndex_, lastWorld_.x, lastWorld_.y, angle_, angleDelta_);
	previewValid_ = true;
}

bool UnitTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	lastWorld_ = worldCoord;
	if(bridge() && attributeIndex_ >= 0)
		bridge()->placeUnit(attributeIndex_, worldCoord.x, worldCoord.y, selectAfterPlace_);
	return true;
}

bool UnitTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	lastWorld_ = worldCoord;
	// CSurToolUnit::updateUnitOnMouse: build the preview unit on the first move
	// (the tool may have activated before the cursor entered the view), then
	// repose it under the cursor.
	if(!previewValid_)
		refreshPreview();
	else if(bridge())
		bridge()->movePreviewUnit(worldCoord.x, worldCoord.y, angle_, angleDelta_);
	return true;
}

bool UnitTool::onDrawAuxData(ToolAuxPainter& painter)
{
	// Kept as a light cursor marker; the real unit preview follows the cursor.
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, kCursorRadius, kBrushColor);
	return hasCursor_;
}
