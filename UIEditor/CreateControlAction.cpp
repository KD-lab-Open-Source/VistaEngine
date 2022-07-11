#include "StdAfx.h"
#include "UIEditor.h"
#include "MainFrame.h"
#include "ControlsTreeCtrl.h"

#include "CreateControlAction.h"
#include "SelectionManager.h"
#include "Serialization\SerializationFactory.h"
#include "UserInterface\UI_Types.h"
#include "UITreeObjects.h"

CreateControlAction::CreateControlAction(UI_ControlContainer& _container, int _type_index)
: control_container_ (_container)
, type_index_ (_type_index)
{

}

void CreateControlAction::act()
{
	added_control_ = FactorySelector<UI_ControlBase>::Factory::instance ().createByIndex (type_index_);
	added_control_->setName(FactorySelector<UI_ControlBase>::Factory::instance ().findByIndex(type_index_).nameAlt());
    added_control_->states().push_back(UI_ControlState("Default", true));
    control_container_.addControl(added_control_);
	added_control_->setState(0);
    SelectionManager::the().select(added_control_);
}

void CreateControlAction::undo()
{
    CControlsTreeCtrl& tree = *CUIEditorApp::GetMainFrame()->GetControlsTree();
	if(UITreeObject* object = tree.root()->findByControlContainer(added_control_))
		object->remove();
    control_container_.removeControl(added_control_);
    added_control_ = 0;

    SelectionManager::the ().deselectAll ();
}

std::string CreateControlAction::description() const
{
    return "Control Creation";
}

UI_ControlBase* CreateControlAction::added_control()
{
    return added_control_;
}
