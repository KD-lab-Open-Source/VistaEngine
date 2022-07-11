#ifndef __UIEDITOR_CONTROL_UTILS_H_INCLUDED__
#define __UIEDITOR_CONTROL_UTILS_H_INCLUDED__

#include "type_name.h"
#include "UserInterface\UserInterface.h"
#include "XTL\Rect.h"
#include "ControlsTreeCtrl.h"

static bool is_erased (const UI_ControlBase& control){
    return false;
}

inline Rectf get_rect (const UI_ControlBase& control){
	return control.position ();
}

inline void set_rect (UI_ControlBase& control, const Rectf& rect){
	control.setPosition (rect);
}


inline bool isControlVisibleInEditor(const UI_ControlBase& _control){
	if(!_control.visibleInEditor()) {
		return false;
	} else {
		const UI_ControlContainer* owner = _control.owner();
		while (owner) {
			if(owner->visibleInEditor() == false)
				return false;
			owner = owner->owner();
		}
	}
	return true;
}

struct ControlUnderPoint {
    ControlUnderPoint (const Vect2f& _point) {
        point_ = _point;
    }
    inline bool operator() (const UI_ControlBase& _control) {
		if (::is_erased(_control))
            return false;

		if(!::isControlVisibleInEditor(_control))
			return false;

		return ::get_rect (_control).point_inside (point_);
    }
	inline bool operator() (const UI_ControlBase* _control) {
		xassert(_control);
		return operator()(*_control);
    }
    Vect2f point_;
};

#endif
