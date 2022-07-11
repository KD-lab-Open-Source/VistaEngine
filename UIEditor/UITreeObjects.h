#ifndef __UI_TREE_OBJECTS_H_INCLUDED__
#define __UI_TREE_OBJECTS_H_INCLUDED__

#include "..\Util\MFC\ObjectsTreeCtrl.h"

class UI_ControlBase;
class UI_Screen;
class UI_ControlState;
class UI_ControlContainer;
class CAttribEditorCtrl;

class UITreeObject : public TreeObjectBase{
public:
    void onSelect(){}
    void onShow(bool visible) {}

	UITreeObject* findByControlContainer(UI_ControlContainer* container);
    const char* name() const = 0;

	UITreeObject* parent() { return static_cast<UITreeObject*>(TreeObject::parent()); }
	virtual UI_ControlContainer* controlContainer();

	CControlsTreeCtrl& tree();
    CAttribEditorCtrl& attribEditor();
};

class UITreeObjectControl : public UITreeObject{
public:
    UITreeObjectControl(UI_ControlBase* control);

	bool onBeginDrag(){ return true; }
	bool onDragOver(const TreeObjects& objects);
	void onDrop(const TreeObjects& objects);
	void onRightClick();
    void onSelect();

	UI_ControlContainer* controlContainer();
	const char* name() const;
    void* getPointer() const;
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = "");
	const type_info& getTypeInfo() const;

	UI_ControlBase* control() { return control_; }
private:
	bool canBeDropped(TreeObject* object);
	void onMenuAddControl(int index);
	void onMenuAddState();
	void onMenuDelete();
	void onMenuDuplicate();

	void sortControls(UITreeObject* object); 
private:
    mutable std::string name_;
    UI_ControlBase* control_;
};

class UITreeObjectScreen : public UITreeObject{
public:
    UITreeObjectScreen(UI_Screen* screen);

	bool onBeginDrag() { return false; }
	bool onDragOver(const TreeObjects& object){ return true; }
	void onDrop(const TreeObjects& object);
	void onSelect();
	void onRightClick();

	UI_ControlContainer* controlContainer();

    const char* name() const;
    void* getPointer() const;
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = "");
	const type_info& getTypeInfo() const;
private:
	void onMenuAddControl(int index);
	void onMenuDelete();
	void onMenuDuplicate();
private:
    mutable std::string name_;
    UI_Screen* screen_;
};

class UITreeObjectControlState : public UITreeObject{
public:
    UITreeObjectControlState(UI_ControlState* state);

	void onSelect();
	void onRightClick();
    
    const char* name() const;
    void* getPointer() const;
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = "");
	const type_info& getTypeInfo() const;
private:
	void onMenuDelete();
private:
    mutable std::string name_;
    UI_ControlState* state_;
};

#endif
