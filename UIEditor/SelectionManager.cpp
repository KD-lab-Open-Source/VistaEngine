#include "StdAfx.h"
#include "SelectionManager.h"

#include "ControlUtils.h"
#include "SelectionCorner.h"
#include "EditorAction.h"
#include "RectEditor.h"
#include "ActionManager.h"
#include "PositionChangeAction.h"

SelectionManager::SelectionManager()
: move_action_ (0)
, scale_action_ (0)
{
}

SelectionManager::~SelectionManager()
{
}

SelectionManager& SelectionManager::the()
{
    static SelectionManager theOnlyOne;
    return theOnlyOne;
}

void SelectionManager::deselectAll()
{
    selection_.clear();
}

void SelectionManager::select(UI_ControlBase* _control)
{
    selection_.clear();
    selection_.push_back(_control);
}

void SelectionManager::select(const Selection& _selection)
{
    selection_ = _selection;
}

void SelectionManager::toggleSelection(UI_ControlBase* _control)
{
    Selection::iterator it;
    it = find(selection_.begin(), selection_.end(), _control);

    if(it == selection_.end()){
        selection_.push_back(_control);
    }
    else{
        selection_.erase(it);
    }
}

bool SelectionManager::isSelected(UI_ControlBase* _control) const
{
    Selection& selection = const_cast <Selection&>(selection_);
    Selection::const_iterator it;
    it = find(selection.begin(),
              selection.end(), _control);
    if(it == selection.end())
        return false;
    else
        return true;
}

Rectf SelectionManager::getRect(int index) const
{
    if(selection_.size() == 1)
        return ::get_rect(*selection_.front());
    else
        return selection_.calculateBounds();
}

int SelectionManager::getRectCount() const
{
    return 1;
}

void SelectionManager::setRect(int index, const Rectf& rect)
{
    if(selection_.size() == 1)
        ::set_rect(*selection_.front(), rect);
    else
        selection_.setRect(rect);
    if(move_action_)
        move_action_->setNewRect(rect);
    if(scale_action_)
        scale_action_->setNewRect(rect);
}

bool SelectionManager::onStartMoving(const Vect2f& point)
{
    if(selection_.empty()){
        return false;
    }
    else{
        move_action_ = new PositionChangeAction(selection_);
        return true;
    }
}

void SelectionManager::onMove(const Vect2f& delta)
{
    ActionManager::the().act(move_action_);
    move_action_ = 0;
}

bool SelectionManager::onStartScaling(const Vect2f& point, const Vect2i& corner)
{
    if(selection_.empty())
        return false;

    scale_action_ = new PositionChangeAction(selection_);
    return true;
}

void SelectionManager::onScale(const Vect2f& delta)
{
    ActionManager::the().act(scale_action_);
}
