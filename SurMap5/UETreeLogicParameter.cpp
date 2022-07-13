#include "StdAfx.h"

#include <fstream>

#include "UETreeBase.h"
#include "UETreeLogicParameter.h"
#include "..\Units\UnitAttribute.h"
#include "Dictionary.h"


#include "CreateAttribDlg.h"
#include "EditArchive.h"


UETreeLogicParameter::UETreeLogicParameter(CUETreeCtrl& tree)
: UETreeLogicBase(tree)
{

}

UETreeLogicParameter::~UETreeLogicParameter()
{
}

void UETreeLogicParameter::rebuildTree()
{
	const char* comboList = ParameterValueTable::instance().comboList();
	std::map<int, ItemType> groupItems;
	
	static int dummy;
	ItemType root = tree_.AddTreeObject(dummy, Dictionary::instance().translate("Группы"));

	const ParameterGroupTable::Strings& strings = ParameterGroupTable::instance().strings();
	ParameterGroupTable::Strings::const_iterator it;
	int index = 0;
	FOR_EACH(strings, it) {
		ParameterGroupReference ref(it->c_str());
		if (it == strings.begin()) {
			groupItems[ref.key()] = root;
		} else {
			ItemType item = tree_.AddTreeObject(*ref, it->c_str(), root);
			tree_.SetItemImage(item, 0, 1, 0, 1);
			groupItems[ref.key()] = item;
		}
	}

	{
		ParameterValueTable::Strings::iterator it;
		ParameterValueTable::Strings& strings = const_cast<ParameterValueTable::Strings&>(ParameterValueTable::instance().strings());
		
		FOR_EACH(strings, it) {
			ParameterValue& val = *it;

			std::map<int, ItemType>::iterator git = groupItems.find(val.group().key());
			ItemType parent = TLI_ROOT;
			if (git != groupItems.end())
				parent = git->second;
			tree_.AddTreeObject(val, val.c_str(), parent, TLI_SORT);
		}
	}

	tree_.Expand(root, TLE_EXPAND);
}

void UETreeLogicParameter::onItemSelected(ItemType item)
{
	attribEditor().DetachData ();

	if (!tree_.GetItemState (item, TLIS_SELECTED)) {
		return;
	}

    if (TreeObjectBase* object = &tree_.getObjectByHandle (item)) {

		if (object->get<ParameterValue*>()) {
			attribEditor().attachSerializeable(object->getSerializeable());
		} else {
			//expandOnly(item);
		}
	}

	attribEditor().RedrawWindow ();
}

void UETreeLogicParameter::onItemRClick(ItemType item)
{
	TreeObjectBase* object = &tree_.getObjectByHandle (item);

	if (std::string* group = object->get<std::string*> ()) {
		popupItemMenu(IDR_PARAMETER_GROUP_MENU, item);
	} else if (ParameterValue* parameter = object->get<ParameterValue*> ()) {
		popupItemMenu(IDR_DELETE_MENU, item);
	} else {
		popupItemMenu(IDR_ADD_MENU, item);
	}
}

void UETreeLogicParameter::onCommand(UINT nID)
{
	TreeObjectBase* object = &tree_.getObjectByHandle (clickedItem_);

    switch(nID){
    case IDM_ADD:
		{
			if(std::string* group = object->get<std::string*>()){
				CCreateAttribDlg dlg(false, false, Dictionary::instance().translate("Новый параметр"), tree_.GetParent());
				if(dlg.DoModal() == IDOK) {
					ParameterValue& value = const_cast<ParameterValue&>(*ParameterValueTable::instance().add(dlg.getAttribName().c_str()));
					value.setGroup(ParameterGroupReference(group->c_str()));
					ParameterValueTable::instance().buildComboList();
					
					tree_.AllowRedraw(FALSE);
					tree_.DeleteAllObjects();
					rebuildTree();
					tree_.AllowRedraw(TRUE);
					tree_.SelectItem(tree_.getObjectHandle(value));
				}
			}
			else{
				CCreateAttribDlg dlg(false, false, Dictionary::instance().translate("Новая группа"), tree_.GetParent());
				if(dlg.DoModal() == IDOK) {
					ParameterGroupReference ref = ParameterGroupTable::instance().add(dlg.getAttribName().c_str());
					ParameterGroupTable::instance().buildComboList();
					tree_.AllowRedraw(FALSE);
					tree_.DeleteAllObjects();
					rebuildTree();
					tree_.AllowRedraw(TRUE);
					tree_.SelectItem(tree_.getObjectHandle(const_cast<std::string&>(*ref)));
				}
			}
		}
        break;
    case IDM_DELETE:
		{
			if (ParameterValue* parameter = object->get<ParameterValue*> ()) {
				DWORD start = GetTickCount();
				ParameterValueTable::instance().remove(parameter->c_str());
				ParameterValueTable::instance().buildComboList();
				tree_.AllowRedraw(FALSE);
				tree_.DeleteAllObjects();
				rebuildTree();
				DWORD end = GetTickCount();
				int duration = int(end - start);
				tree_.AllowRedraw(TRUE);
			} else if (std::string* group = object->get<std::string*> ()) {
				ParameterGroupTable::instance().remove(group->c_str());
				ParameterGroupTable::instance().buildComboList();
				tree_.AllowRedraw(FALSE);
				tree_.DeleteAllObjects();
				rebuildTree();
				tree_.AllowRedraw(TRUE);
			}
		}
        break;
    }
}

bool UETreeLogicParameter::allowDrag(ItemType item)
{
    if(TreeObjectBase* object = &tree_.getObjectByHandle(item)) {
		if(object->get<ParameterValue*>() != 0){
			return true;
		} else if(object->get<std::string*>() != 0) {
			return true;
		}
	}
	return false;
}

bool UETreeLogicParameter::allowDropOn(ItemType dest, ItemType src)
{
	if(tree_.getObjectByHandle(src).get<ParameterValue*>()) {
		if(tree_.GetParentItem(dest) == 0) {
			return true;
		}
		if (tree_.getObjectByHandle(dest).get<std::string*>() ){
			return true;
		}
	} else if (tree_.getObjectByHandle(src).get<std::string*>()) {
		if(tree_.getObjectByHandle(dest).get<std::string*>()) {
			return true;
		}
	}
	return false;
}

void UETreeLogicParameter::onDrop(ItemType dest, ItemType src)
{
	if(src == dest)
		return;

	ParameterValue* parameter = tree_.getObjectByHandle(src).get<ParameterValue*>();
	std::string* group = tree_.getObjectByHandle(dest).get<std::string*>();
	if (parameter) {
		if (group == 0)
			group = const_cast<std::string*>(&*ParameterGroupReference());
		ParameterGroupReference groupRef(group->c_str());
        parameter->setGroup(groupRef);
		//tree_.DeleteItem(src);
		tree_.DeleteTreeObject(*parameter);
		tree_.AddTreeObject(*parameter, parameter->c_str(), dest);
	} else if (group) {
		if(std::string* src_group = tree_.getObjectByHandle(src).get<std::string*>()) {
			ParameterGroupTable::StringIndex sourceGroupIndex;
			
			std::string sourceGroup = *src_group;
			std::string beforeGroup = *group;
			
			ParameterGroupTable::Strings& strings = const_cast<ParameterGroupTable::Strings&>(ParameterGroupTable::instance().strings());
			ParameterGroupTable::Strings::iterator it;
			
			FOR_EACH(strings, it) {
				ParameterGroupTable::StringIndex& index = *it;
				if(sourceGroup == index.c_str()) {
					sourceGroupIndex = index;
					strings.erase(it);
					--it;
				}		
			}			

			FOR_EACH(strings, it) {
				ParameterGroupTable::StringIndex& index = *it;
				if(beforeGroup == index.c_str()) {
					strings.insert(it, sourceGroupIndex);
					break;
				}
			}
			ParameterGroupTable::instance().buildComboList();

			tree_.DeleteAllObjects();
			rebuildTree();
		}        
	}
}
