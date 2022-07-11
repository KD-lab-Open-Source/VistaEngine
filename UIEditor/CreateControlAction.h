#ifndef __UIEDITOR_ADD_CONTROL_ACTION_H_INCLUDED__
#define __UIEDITOR_ADD_CONTROL_ACTION_H_INCLUDED__

#include "EditorAction.h"
class UI_ControlContainer;
class UI_ControlBase;

/// Операция создания контрола в контейнере
class CreateControlAction : public EditorAction
{
public:
    CreateControlAction(UI_ControlContainer& _container, int _type_index);

    void act();
    void undo();
    std::string description() const;
    UI_ControlBase* added_control();
protected:
    ShareHandle<UI_ControlBase> added_control_;
    UI_ControlContainer& control_container_;
    int type_index_;
};

#endif
