#ifndef __BG_FOCUS_CHANGE_ACTION_H_INCLUDED__
#define __BG_FOCUS_CHANGE_ACTION_H_INCLUDED__

#include "EditorAction.h"
#include <string>

/// Операция изменения свойства объекта
class BgFocusChangeAction : public EditorAction
{
public:
	BgFocusChangeAction(float oldFocus, float newFocus)
		: oldFocus_(oldFocus), newFocus_(newFocus) {}

	virtual ~BgFocusChangeAction() {}

	virtual void act();
	virtual void undo();

	virtual std::string description() const {
		return "Focus Change";
	}

private:

	float oldFocus_;
	float newFocus_;
};

#endif //__BG_FOCUS_CHANGE_ACTION_H_INCLUDED__