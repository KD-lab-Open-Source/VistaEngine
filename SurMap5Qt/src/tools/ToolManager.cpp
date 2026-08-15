// ToolManager.cpp — see header.

#include "ToolManager.h"

#include "MoveTool.h"
#include "RotateTool.h"
#include "ScaleTool.h"
#include "SelectTool.h"

ToolManager::ToolManager()
{
	// The tools tree's root order in SurMap5: Select, Move, Rotate, Scale
	// (folders come first in the real tree; these four are the transform set).
	select_ = new SelectTool;
	move_   = new MoveTool;
	rotate_ = new RotateTool;
	scale_  = new ScaleTool;

	tools_.push_back(select_);
	tools_.push_back(move_);
	tools_.push_back(rotate_);
	tools_.push_back(scale_);

	current_ = select_;
	currentIndex_ = 0;
}

ToolManager::~ToolManager()
{
	delete select_;
	delete move_;
	delete rotate_;
	delete scale_;
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
