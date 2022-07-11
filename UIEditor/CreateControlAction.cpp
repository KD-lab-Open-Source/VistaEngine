#include "StdAfx.h"
#include "UIEditor.h"
#include "MainFrame.h"
#include "ControlsTreeCtrl.h"

#include "CreateControlAction.h"
#include "SelectionManager.h"
#include "ClassCreatorFactory.h"
#include "..\UserInterface\UI_Types.h"

CreateControlAction::CreateControlAction(UI_ControlContainer& _container, int _type_index)
: control_container_ (_container)
, type_index_ (_type_index)
{

}

void CreateControlAction::act()
{
    added_control_ = ClassCreatorFactory<UI_ControlBase>::instance ().createByIndex (type_index_);
    added_control_->setName(ClassCreatorFactory<UI_ControlBase>::instance ().findByIndex(type_index_).nameAlt());
    added_control_->states().push_back(UI_ControlState("Default", true));
    bool result = control_container_.addControl(added_control_);
    xassert(result && "Unable to create control!");
    SelectionManager::the().select(added_control_);
}

void CreateControlAction::undo()
{
    CControlsTreeCtrl& tree = *CUIEditorApp::GetMainFrame()->GetControlsTree();
    tree.deleteByData(*added_control_);
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
