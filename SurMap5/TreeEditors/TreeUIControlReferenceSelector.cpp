#include "StdAfx.h"
#include "EditArchive.h"

#include "..\LibraryBookmark.h"

#include "TreeEditor.h"
#include "TreeSelector.h"

#include "..\..\UserInterface\UI_References.h"
#include "..\..\UserInterface\UI_Types.h"
#include "..\..\UserInterface\UI_Inventory.h"
#include "..\..\UserInterface\UserInterface.h"

typedef UI_ControlReference ReferenceType;

class UIControlReferenceTreeBuilder : public TreeBuilder {
public:
	UIControlReferenceTreeBuilder(ReferenceType& reference)
	: reference_(reference)
	{}
	class ControlObject : public Object{
	public:
		ControlObject(const UI_ControlBase* control, const char* name)
		: Object(name, name)
		, control_(control)
		{
			selectable_ = control != 0;
		}

		const UI_ControlBase* control() { return control_; }

	protected:
		const UI_ControlBase* control_;
	};
protected:
	ReferenceType& reference_;

	Object* buildControlSubtree(Object* parent, const UI_ControlContainer* container, const UI_ControlContainer* selected){
		UI_ControlContainer::ControlList::const_iterator it;
		const char* name = container->name();
		Object* containerObj = parent->add(new ControlObject(0, name));

		Object* result = selected == container ? containerObj : 0;
		FOR_EACH(container->controlList(), it){
			if(Object* o = buildControlSubtree(containerObj, &**it, selected))
				result = o;
		}
		return result;
	}
	Object* buildControlSubtree(Object* parent, const UI_ControlBase* container, const UI_ControlContainer* selected){
		UI_ControlContainer::ControlList::const_iterator it;
		const char* name = container->name();
		Object* containerObj = parent->add(new ControlObject(container, name));

		Object* result = selected == container ? containerObj : 0;
		FOR_EACH(container->controlList(), it){
			if(Object* o = buildControlSubtree(containerObj, &**it, selected))
				result = o;
		}
		return result;
	}

    Object* buildTree(Object* root){
		typedef StaticMap<std::string, Object*> Groups;
		Object* result = 0;
		UI_Dispatcher::ScreenContainer& screens = UI_Dispatcher::instance().screens();
		UI_Dispatcher::ScreenContainer::iterator sit;
		UI_ControlBase* selected = reference_.control();
		FOR_EACH(screens, sit){
			UI_Screen* screen = &*sit;
			if(Object* o = buildControlSubtree(root, screen, selected))
				result = o;
		}
		return result;
    }
	bool select(Object* object){
		ControlObject* controlObject = safe_cast<ControlObject*>(object);
		if(controlObject->selectable()){
			reference_ = UI_ControlReference(controlObject->control());
			return true;
		}
		else
			return false;
	}
};

class TreeUIControlReferenceSelector : public TreeEditorImpl<TreeUIControlReferenceSelector, ReferenceType>{
public:
	bool invokeEditor(ReferenceType& reference, HWND parent);

	bool hideContent() const{ return true; }
	Icon buttonIcon() const{
		return TreeEditor::ICON_REFERENCE;
	}
	std::string nodeValue() const{ 
		return getData().referenceString();
	}

};

bool TreeUIControlReferenceSelector::invokeEditor (ReferenceType& reference, HWND parent)
{
	CTreeSelectorDlg dlg(CWnd::FromHandle(parent));
	UIControlReferenceTreeBuilder treeBuilder(reference);
	dlg.setBuilder(&treeBuilder);
    dlg.DoModal();
    return true;
}

typedef TreeUIControlReferenceSelector TreeUIInventoryReferenceSelector;

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(ReferenceType).name(), TreeUIControlReferenceSelector);
REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(UI_InventoryReference).name(), TreeUIInventoryReferenceSelector);
