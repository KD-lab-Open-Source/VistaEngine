#include "stdafx.h"
#include "TreeEditor.h"
#include "..\AttribEditor\AttribEditorDlg.h"

#include "EditArchive.h"

#include "TypeLibrary.h"
#include "..\Units\UnitAttribute.h"
#include "TreeSelector.h"

typedef ParameterValueReference ReferenceType;
typedef ParameterValueTable StringTableType;

class ParameterTreeBuilder : public TreeBuilder {
public:
	ParameterTreeBuilder(ParameterValueReference& reference)
	: reference_(reference)
	, result_(0)
	{}
protected:
	ParameterValueReference& reference_;
	Object* result_;

    Object* buildTree(Object* root){
		std::map<int, Object*> groupItems;
		std::map<int, Object*>::iterator groupIterator;

		const ParameterGroupTable::Strings& strings = ParameterGroupTable::instance().strings();
		ParameterGroupTable::Strings::const_iterator it;
		int index = 0;
		FOR_EACH(strings, it) {
			ParameterGroupReference ref(it->c_str());
			if (it == strings.begin()) {
				groupItems[ref.key()] = root;
			} else {
				groupItems[ref.key()] = root->add(*ref, it->c_str());
			}
		}

		Object* selectedItem = 0;
		{
			ParameterValueTable::Strings::iterator it;
			ParameterValueTable::Strings& strings = const_cast<ParameterValueTable::Strings&>(ParameterValueTable::instance().strings());
			
			FOR_EACH(strings, it) {
				ParameterValue& val = *it;

				std::map<int, Object*>::iterator git = groupItems.find(val.group().key());
				Object* parent = root;
				if (git != groupItems.end())
					parent = git->second;
				Object* item = parent->add(val, val.c_str());
				if(&val == &*reference_)
					selectedItem = item;
			}
		}
		FOR_EACH(groupItems, groupIterator)
			if(groupIterator->second != root && groupIterator->second->empty() == 0){
				groupIterator->second->erase();
				groupIterator->second = 0;
			}
				//tree.deleteItem(groupIterator->second);
		return selectedItem;
    }
	bool select(Object* object){
		if(ParameterValue* val = object->get<ParameterValue*>()){
			reference_ = ParameterValueReference(val->c_str());
			return true;
		}
		else{
			return false;
		}
	}
private:
};

class TreeParameterSelector : public TreeEditorImpl<TreeParameterSelector, ReferenceType> {
public:
	bool invokeEditor(ReferenceType& reference, HWND parent)
	{
		CTreeSelectorDlg dlg(CWnd::FromHandle(parent));
		ParameterTreeBuilder treeBuilder(reference);
		dlg.setBuilder(&treeBuilder);
		dlg.DoModal();
		return true;
	}
	bool hideContent () const{
		return true;
	}
	Icon buttonIcon() const{
		return TreeEditor::ICON_REFERENCE;
	}
	virtual std::string nodeValue () const { 
		return getData().c_str();
	}
	bool hasLibrary() const{
		return true;
	}
};


//REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(ParameterValueReference).name (), TreeParameterSelector);
