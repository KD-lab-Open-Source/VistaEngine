#ifndef __CONTROLS_TREE_CTRL_H__INCLUDED__
#define __CONTROLS_TREE_CTRL_H__INCLUDED__

#include "MFC\ObjectsTreeCtrl.h"

class UI_ControlState;
class UI_ControlContainer;
class UI_ControlBase;
class UI_Screen;

class PopupMenu;
class UITreeObject;
class UITreeRoot;

class ControlsTree : public kdw::ObjectsTree{
public:
	ControlsTree();

	UITreeRoot* root();
	const UITreeRoot* root() const;

	bool onRowLMBDown(kdw::TreeRow* row, const Recti& rowRect, Vect2i point);
protected:
	ShareHandle<kdw::ImageStore> imageStore_;
};


/// при переходи на новый интерфейс нужно будет удалить
class CControlsTreeCtrl : public CObjectsTreeCtrl
{
public:
    CControlsTreeCtrl();
    virtual ~CControlsTreeCtrl();

    void buildTree();	

    void buildContainerSubtree(UI_ControlContainer&);

    void updateSelected ();
    bool selectControl (UI_ControlBase* control);
    void selectControlInScreen (UI_ControlBase* control, UI_Screen* screen);
    void selectState (UI_ControlBase* control, int stateIndex);
    void selectScreen (UI_Screen* screen);
	UITreeObject* objectByControl(UI_ControlBase* control);

	void sortControls(UITreeObject* object);

	UITreeRoot* root(){ return tree()->root(); }
	const UITreeRoot* root() const{ return tree()->root(); }

	ControlsTree* tree(){ return safe_cast<ControlsTree*>(__super::tree()); }
	const ControlsTree* tree() const{ return safe_cast<const ControlsTree*>(__super::tree()); }

    DECLARE_MESSAGE_MAP()
private:

    UI_ControlContainer* drop_target_;
    UI_ControlBase* dragged_control_;
};

#endif
