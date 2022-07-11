#include "StdAfx.h"
#include "UIEditor.h"
#include "MainFrame.h"
#include "ControlsTreeCtrl.h"
#include "EraseAction.h"
#include "SelectionManager.h"
#include "UITreeObjects.h"

#include "UserInterface\UI_Types.h"

EraseControlAction::EraseControlAction(const Selection& _selection, UI_ControlContainer& _container)
: selection_ (_selection)
, container_ (_container)
{}


void EraseControlAction::act()
{

	Selection::iterator it;
	FOR_EACH(selection_, it){
		// FIXME: красиво отработать, когда в селекте присутствуют и child-ы и parent-ы.
		if(UITreeObject* object = controlsTree()->root()->findByControlContainer(*it)){
			object->remove();
		}
	}

	SelectionManager::the().deselectAll();
    FOR_EACH(selection_, it){
        removed_controls_.push_back(*it);
        bool result = container_.removeControl(*it);
    }
}

void EraseControlAction::undo()
{
    while (!removed_controls_.empty ()) {
        container_.addControl (removed_controls_.front ());
        removed_controls_.erase (removed_controls_.begin ());
    }
    SelectionManager::the ().select(selection_);
	if(UITreeObject* object = controlsTree()->root()->findByControlContainer(&container_)){
		object->rebuild();
		controlsTree()->update();
	}
}

std::string EraseControlAction::description() const
{
    if (selection_.size () == 1) {
        return "Delete";
    }
    else {
        return "Group Delete";
    }
}
