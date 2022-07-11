#ifndef __POSITION_CHANGE_ACTION_H_INCLUDED__
#define __POSITION_CHANGE_ACTION_H_INCLUDED__

#include "EditorAction.h"
#include "Selection.h"

/// ќпераци€ изменени€ положени€/размера котрола
class PositionChangeAction : public EditorAction
{
public:
	PositionChangeAction(const Selection& selection)
    : selection_ (selection)
    {
		if (!selection_.empty()) { 
			newRect_ = oldRect_ = selection_.calculateBounds ();
		}
    }
	PositionChangeAction(const Selection& selection, const Vect2f& delta)
    : selection_ (selection)
    {
		if (!selection_.empty()) { 
			oldRect_ = selection_.calculateBounds ();
			newRect_ = oldRect_ + delta;
		}
    }
	virtual ~PositionChangeAction() {}

	void setNewRect (const Rectf& new_rect) {
		newRect_ = new_rect;
	}

	virtual void act () {
		if (!selection_.empty()) {
			selection_.setRect (newRect_);
		}
	}
	virtual void undo () {
		if (!selection_.empty()) { 
			selection_.setRect (oldRect_);
		}
	}
	virtual std::string description () const {
		return "Position Change";
	}
private:
	Rectf oldRect_;
	Rectf newRect_;
    Selection selection_;
};

#endif
