#ifndef __SURMAP5_UI_EDITOR_ACTION_MANAGER__
#define __SURMAP5_UI_EDITOR_ACTION_MANAGER__

#include <stack>

#include "EditorAction.h"

class ActionManager
{
	ActionManager ()
	: changed_(false)
	{
	}
public:
	static ActionManager& the () {
		static ActionManager theOnlyOne;
		return theOnlyOne;
	}

	~ActionManager () {
		purge ();
	}
    void act(EditorAction* _new_action, bool _do_action = true){
        if (_new_action == 0)
            return;

        actions_.push(_new_action);
        if(_do_action)
            _new_action->act ();

		changed_ = true;
    }
    void undo(){
		if (actions_.empty ())
			return;

        actions_.top ()->undo ();
        actions_.pop ();

		changed_ = true;
    }
    bool can_be_undone () const {
        return !actions_.empty ();
    }
	std::string last_action_description() const{
		if(actions_.size())
			return actions_.top()->description();
		else
			return "";
	}
	void purge(){
		while(!actions_.empty()){
			actions_.pop ();
		}
		changed_ = false;
	}

	bool haveChanges() const{
		return changed_;
	}

	void setChanged(bool changed = true){ changed_ = changed; }
private:
	bool changed_;
	std::stack< ShareHandle<EditorAction> > actions_;
};

#endif
