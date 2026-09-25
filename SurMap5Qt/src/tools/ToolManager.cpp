// ToolManager.cpp — see header.

#include "ToolManager.h"

#include "MoveTool.h"
#include "RotateTool.h"
#include "ScaleTool.h"
#include "SelectTool.h"
#include "GeoNetTool.h"
#include "GeoNetPropertyPanel.h"
#include "GeoTxTool.h"
#include "GeoTxPropertyPanel.h"
#include "ToolzerTool.h"
#include "ToolzerPropertyPanel.h"
#include "KindTool.h"
#include "KindPropertyPanel.h"
#include "ColorPicTool.h"
#include "ColorPicPropertyPanel.h"
#include "TransformPropertyPanel.h"
#include "TransformTool.h"

ToolManager::ToolManager()
{
	// The tools tree's root order in SurMap5: Select, Move, Rotate, Scale
	// (folders come first in the real tree; these four are the transform set).
	select_ = new SelectTool;
	move_   = new MoveTool;
	rotate_ = new RotateTool;
	scale_  = new ScaleTool;
	toolzer_ = new ToolzerTool;
	kind_    = new KindTool;
	colorPic_ = new ColorPicTool;
	geoNet_ = new GeoNetTool;
	geoTx_  = new GeoTxTool;

	tools_.push_back(select_);
	tools_.push_back(move_);
	tools_.push_back(rotate_);
	tools_.push_back(scale_);
	tools_.push_back(toolzer_);
	tools_.push_back(kind_);
	tools_.push_back(colorPic_);
	tools_.push_back(geoNet_);
	tools_.push_back(geoTx_);

	current_ = select_;
	currentIndex_ = 0;
}

ToolManager::~ToolManager()
{
	delete select_;
	delete move_;
	delete rotate_;
	delete scale_;
	delete geoNet_;
	delete geoTx_;
	delete toolzer_;
	delete kind_;
	delete colorPic_;
}

void ToolManager::setWorldBridge(IWorldBridge* bridge)
{
	// Propagate to every tool; RenderViewWidget calls this once after the
	// viewport's bridge is constructed (a null bridge just detaches tools
	// from the world — e.g. before any world is loaded).
	for(EditorTool* tool : tools_)
		if(tool)
			tool->setWorldBridge(bridge);
}

void ToolManager::setBrushRadius(float radius)
{
	// The brush radius is shared across brush tools (the original's
	// CSurToolBase::getBrushRadius read one value from the toolbar combo).
	// Only GeoNet has a brush today; the assignment is type-checked so a
	// future brush tool opts in by overloading the same setter.
	if(geoNet_)
		geoNet_->setBrushRadius(radius);
	if(toolzer_)
		toolzer_->setBrushRadius(radius);
	if(kind_)
		kind_->setBrushRadius(radius);
	if(colorPic_)
		colorPic_->setBrushRadius(radius);
}

void ToolManager::setCurrentTool(int index)
{
	if(index < 0 || index >= (int)tools_.size())
		return;
	if(index == currentIndex_)
		return;
	if(current_)
		current_->onDeactivate();
	currentIndex_ = index;
	current_ = tools_[index];
	current_->onActivate();
	// Re-bind the Properties panel to the new tool (the panel reflects the
	// transform axis; non-transform tools get no panel).
	if(propertyPanel_)
		propertyPanel_->setTool(dynamic_cast<TransformTool*>(current_));
	if(geoNetPanel_)
		geoNetPanel_->setTool(dynamic_cast<GeoNetTool*>(current_));
	if(geoTxPanel_)
		geoTxPanel_->setTool(dynamic_cast<GeoTxTool*>(current_));
	if(toolzerPanel_)
		toolzerPanel_->setTool(dynamic_cast<ToolzerTool*>(current_));
	if(kindPanel_)
		kindPanel_->setTool(dynamic_cast<KindTool*>(current_));
	if(colorPicPanel_)
		colorPicPanel_->setTool(dynamic_cast<ColorPicTool*>(current_));
}

QWidget* ToolManager::propertyWidget()
{
	// The transform tools share one axis panel (the original's CSurToolTransform
	// dialog); the GeoNet tool has its own parameter panel. Created lazily so a
	// tool-less editor never allocates them.
	if(dynamic_cast<GeoNetTool*>(current_)){
		if(!geoNetPanel_){
			geoNetPanel_ = new GeoNetPropertyPanel;
			geoNetPanel_->setTool(geoNet_);
		}
		return geoNetPanel_;
	}
	if(dynamic_cast<GeoTxTool*>(current_)){
		if(!geoTxPanel_){
			geoTxPanel_ = new GeoTxPropertyPanel;
			geoTxPanel_->setTool(geoTx_);
		}
		return geoTxPanel_;
	}
	if(dynamic_cast<ToolzerTool*>(current_)){
		if(!toolzerPanel_){
			toolzerPanel_ = new ToolzerPropertyPanel;
			toolzerPanel_->setTool(toolzer_);
		}
		return toolzerPanel_;
	}
	if(dynamic_cast<KindTool*>(current_)){
		if(!kindPanel_){
			kindPanel_ = new KindPropertyPanel;
			kindPanel_->setTool(kind_);
		}
		return kindPanel_;
	}
	if(dynamic_cast<ColorPicTool*>(current_)){
		if(!colorPicPanel_){
			colorPicPanel_ = new ColorPicPropertyPanel;
			colorPicPanel_->setTool(colorPic_);
		}
		return colorPicPanel_;
	}
	if(!propertyPanel_){
		propertyPanel_ = new TransformPropertyPanel;
		propertyPanel_->setTool(dynamic_cast<TransformTool*>(current_));
	}
	return propertyPanel_;
}

bool ToolManager::onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	return current_ ? current_->onTrackingMouse(worldCoord, screenCoord) : false;
}

bool ToolManager::onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	return current_ ? current_->onLMBDown(worldCoord, screenCoord) : false;
}

bool ToolManager::onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	return current_ ? current_->onLMBUp(worldCoord, screenCoord) : false;
}

bool ToolManager::onRMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	return current_ ? current_->onRMBDown(worldCoord, screenCoord) : false;
}

bool ToolManager::onRMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord)
{
	return current_ ? current_->onRMBUp(worldCoord, screenCoord) : false;
}

bool ToolManager::onKeyDown(unsigned keyCode, bool shift, bool control, bool alt)
{
	return current_ ? current_->onKeyDown(keyCode, shift, control, alt) : false;
}

bool ToolManager::onDelete()
{
	return current_ ? current_->onDelete() : false;
}

void ToolManager::quant(float dt)
{
	if(current_)
		current_->quant(dt);
}
