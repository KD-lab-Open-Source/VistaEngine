// EnvironmentTool.cpp — see header.

#include "EnvironmentTool.h"

#include <cctype>

namespace {
const unsigned kBrushColor = 0xFF90FF90; // green, distinct from unit/source/brushes
}

int environmentTypeForModel(const std::string& model, bool& vertical)
{
	// Lower-case copy for the substring tests.
	std::string n;
	n.reserve(model.size());
	for(char c : model)
		n.push_back((char)std::tolower((unsigned char)c));
	auto has = [&n](const char* s){ return n.find(s) != std::string::npos; };

	vertical = false;
	// Order matters: boulder/rock before stone (a "stone_boulder" is a rock).
	if(has("boulder") || has("rock"))                          { vertical = false; return 7; }  // ENVIRONMENT_ROCK
	if(has("tree"))                                            { vertical = true;  return 3; }  // ENVIRONMENT_TREE
	if(has("bush"))                                            { vertical = false; return 2; }  // ENVIRONMENT_BUSH
	if(has("fence"))                                           { vertical = true;  return 4; }  // ENVIRONMENT_FENCE
	if(has("stone"))                                           { vertical = false; return 6; }  // ENVIRONMENT_STONE
	if(has("basement") || has("cellar"))                       { vertical = true;  return 8; }  // ENVIRONMENT_BASEMENT
	if(has("barn"))                                            { vertical = true;  return 9; }  // ENVIRONMENT_BARN
	if(has("bridge"))                                          { vertical = true;  return 11; } // ENVIRONMENT_BRIDGE
	if(has("building") || has("house") || has("tower") ||
	   has("factory") || has("castle") || has("barrack"))      { vertical = true;  return 10; } // ENVIRONMENT_BUILDING
	return -1;
}

EnvironmentTool::EnvironmentTool()
{
	params_.brushRadius = brushRadius_;
}

void EnvironmentTool::onActivate()
{
	active_ = true;
	refreshPreview(true);
}

void EnvironmentTool::onDeactivate()
{
	active_ = false;
	hasCursor_ = false;
	if(bridge())
		bridge()->killEnvironmentPreview();
}

void EnvironmentTool::setBrushRadius(float radius)
{
	brushRadius_ = radius;
	params_.brushRadius = radius;
	if(active_ && hasCursor_)
		refreshPreview(true);
}

void EnvironmentTool::applyParams()
{
	params_.brushRadius = brushRadius_;
	refreshPreview(true);
}

void EnvironmentTool::setModel(const std::string& model)
{
	params_.model = model;
	// Pull the type/vertical from the model name (a tree model places as
	// ENVIRONMENT_TREE upright, a rock as ENVIRONMENT_ROCK, ...).
	const int typeIndex = environmentTypeForModel(model, params_.vertical);
	if(typeIndex >= 0)
		params_.typeIndex = typeIndex;
	applyParams();
}

void EnvironmentTool::refreshPreview(bool rebuild)
{
	if(!bridge() || !active_ || !hasCursor_)
		return;
	bridge()->updateEnvironmentPreview(params_, lastWorld_.x, lastWorld_.y, rebuild);
}

bool EnvironmentTool::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	lastWorld_ = worldCoord;
	if(bridge()){
		bridge()->placeEnvironment(params_, worldCoord.x, worldCoord.y);
		// The engine reseeds and rebuilds the spread layout after placement
		// (CSurToolEnvironment::onOperationOnMap); refresh the preview.
		refreshPreview(true);
	}
	return true;
}

bool EnvironmentTool::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	hasCursor_ = true;
	cursorScreen_ = screenCoord;
	lastWorld_ = worldCoord;
	// Move the existing preview; do not rebuild it per motion event.
	refreshPreview(false);
	return true;
}

bool EnvironmentTool::onDrawAuxData(ToolAuxPainter& painter)
{
	// CSurToolBase::drawCursorCircle — the brush radius around the cursor.
	if(hasCursor_)
		painter.drawCircle2D(cursorScreen_, (int)brushRadius_, kBrushColor);
	return hasCursor_;
}
