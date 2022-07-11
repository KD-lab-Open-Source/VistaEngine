#ifndef __UI_TREE_OBJECTS_H_INCLUDED__
#define __UI_TREE_OBJECTS_H_INCLUDED__

#include "kdw/ObjectsTree.h"

class UI_ControlBase;
class UI_Screen;
class UI_ControlState;
class UI_ControlContainer;
class CAttribEditorCtrl;
class ControlsTree;

class UITreeObjectControlState;
class UITreeObject : public kdw::TreeObject, public sigslot::has_slots{
public:
	void onSelect(kdw::ObjectsTree* tree){}
    void onShow(bool visible) {}

	UITreeObject* findByControlContainer(UI_ControlContainer* container, bool recurse = true, bool onlyCheck = false);

	virtual void updateText(){}

	virtual void rebuild();
	UITreeObject* parent() { return static_cast<UITreeObject*>(TreeObject::parent()); }
	void onRightClick(kdw::ObjectsTree* tree);
	virtual void onRightClick(ControlsTree* tree) {}

	virtual UI_ControlContainer* controlContainer();
	const UI_ControlContainer* controlContainer() const;
	UITreeObjectControlState* stateByIndex(int index);
	void onMenuAddControl(int index, ControlsTree* tree);
	void sortControls();

    CAttribEditorCtrl& attribEditor();
};

class UITreeRoot : public UITreeObject{
public:
	void rebuild();
};

class UITreeObjectControl : public UITreeObject{
public:
    UITreeObjectControl(UI_ControlBase* control);

	bool canBeDragged() const{ return true; }

	bool canBeDroppedOn(const kdw::TreeRow* row, const kdw::TreeRow* beforeChild, const kdw::Tree* tree, bool direct) const;
	void dropInto(kdw::TreeRow* destination, kdw::TreeRow* beforeChild, kdw::Tree* tree);

	bool onDragOver(const kdw::TreeObjects& objects);
	void onDrop(const kdw::TreeObjects& objects);
	void onRightClick(ControlsTree* tree);
    void onSelect(kdw::ObjectsTree* tree);

	UI_ControlContainer* controlContainer();
	std::string name() const;
	void updateText();
	Serializer getSerializer(const char* name = "", const char* nameAlt = "");
	const type_info& getTypeInfo() const;

	UI_ControlBase* control() { return control_; }

	void onMenuAddControl(int index, ControlsTree* tree);
private:
	void onMenuAddState(ControlsTree* tree);
	void onMenuDelete();
	void onMenuDuplicate(ControlsTree* tree);

	void sortControls(UITreeObject* object); 
private:
    UI_ControlBase* control_;
};

class UITreeObjectScreen : public UITreeObject{
public:
    UITreeObjectScreen(UI_Screen* screen);

	void onSelect(kdw::ObjectsTree* tree);
	void onRightClick(ControlsTree* tree);

	UI_ControlContainer* controlContainer();
	UI_Screen* screen(){ return screen_; }

	std::string name() const;
	void updateText();
	Serializer getSerializer(const char* name = "", const char* nameAlt = "");
	const type_info& getTypeInfo() const;
private:
	void onMenuDelete(ControlsTree* tree);
	void onMenuDuplicate(ControlsTree* tree);
private:
    UI_Screen* screen_;
};

class UITreeObjectControlState : public UITreeObject{
public:
    UITreeObjectControlState(UI_ControlState* state);

	void onSelect(kdw::ObjectsTree* tree);
	void onRightClick(ControlsTree* tree);
    
	void updateText();
	std::string name() const;
	Serializer getSerializer(const char* name = "", const char* nameAlt = "");
	const type_info& getTypeInfo() const;
protected:
	void onMenuDelete(ControlsTree* tree);
    UI_ControlState* state_;
};

#endif
