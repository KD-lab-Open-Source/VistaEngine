#ifndef __UIEDITOR_CREATE_STATE_ACTION_H_INCLUDED__
#define __UIEDITOR_CREATE_STATE_ACTION_H_INCLUDED__

#include "EditorAction.h"

class UI_ControlBase;

class CreateStateAction : public EditorAction
{
public:
    CreateStateAction(UI_ControlBase& control);

    // virtuals:
    void act();
    void undo();
    std::string description() const;
    int stateIndex();
protected:
    int state_index_;
    UI_ControlBase& control_;
};

#endif
