#include "StdAfx.h"

#include "EditArchive.h"
#include "Serialization.h"
#include "PropertyChangeAction.h"

#include "..\UserInterface\UI_Types.h"

struct TreeNodeWithName {
	TreeNodeWithName (const std::string& _name)
		: name_ (_name)
	{}
	bool operator() (TreeNode* node) const {
		return (name_ == node->name ());
	}
	std::string name_;
};

void PropertyChangeAction::act ()
{
    EditOArchive oarchive;
    oarchive.serialize(*control_, "control", "Control");
    ShareHandle<TreeNode> root_node (new TreeNode (""));
    *root_node = *oarchive.rootNode ();
    TreeNode* current_node = root_node;
    TreeNode::iterator it;
    int level = 0;
    bool done = false;
    for (;;) {
        if (done) {
            break;
        }
        FOR_EACH (*current_node, it) {
            TreeNode::iterator pit = std::find_if ((*it)->begin (), (*it)->end (), TreeNodeWithName (node_path_ [level]));
            if (pit != (**it).end ()) {
                current_node = *it;
                if (level == node_path_.size () - 1) {
                    current_node = *it;
                    done = true;
                }
                else {
                    ++level;
                }
                break;
            }
        }
        if (it == current_node->end ())
            break;
    }
    EditIArchive iarchive;
    iarchive.setRootNode (root_node);
    oarchive.serialize(*control_, "control", "Control");
    // xassert (element_);
    // old_value_ = *element_;
    // *element_ = new_value_.c_str ();
    // new_value_ = old_value_;
}

void PropertyChangeAction::undo ()
{
    act ();
}

PropertyChangeAction::PropertyChangeAction (UI_ControlBase* _control, const TreeNodePath& _path, const std::string& _new_value)
{
    control_   = _control;
    node_path_ = _path;
    new_value_ = _new_value;
}
