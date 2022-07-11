#include "StdAfx.h"
#include "CreateStateAction.h"
#include "..\UserInterface\UI_Types.h"

CreateStateAction::CreateStateAction (UI_ControlBase& control)
: control_ (control)
{
    state_index_ = -1;
}

void CreateStateAction::act()
{
    state_index_ = control_.states ().size ();
    UI_ControlState newState;
    control_.states ().push_back (UI_ControlState ());
    control_.init ();
}

void CreateStateAction::undo()
{
    control_.states ().pop_back ();
    control_.init ();
}

std::string CreateStateAction::description() const
{
    return "State Creation";
}

int CreateStateAction::stateIndex()
{
    return state_index_;
}
