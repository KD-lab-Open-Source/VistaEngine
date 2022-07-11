#include "StdAfx.h"

#include "TreeEditor.h" // дл€ TreeEditorFactory
#include "Serialization.h"
#include "TreeEditor.h"

#include "UI_References.h"
#include "UserInterface.h"
#include "UI_Controls.h"

#include "AttribEditorInterface.h"
AttribEditorInterface& attribEditorInterface();

UI_ControlMapReference ui_ControlMapReference;
UI_ControlMapBackReference ui_ControlMapBackReference;

UI_ControlBase* UI_ControlReference::getControlByID(int id)
{
#ifndef _FINAL_VERSION_
	if(!ui_ControlMapReference.exists(id)){
		xxassert(0, XBuffer() < " нопка с ID=" <= id < " не найдена");
		return 0;
	}
#endif	
	return UI_ControlReference(ui_ControlMapReference[id]).control();
}

int UI_ControlReference::getIdByControl(const UI_ControlBase* control)
{
	return ui_ControlMapBackReference[UI_ControlReference(control).reference()];
}

// ------------------- UI_ControlReferenceBase

bool UI_ControlReferenceBase::init(const UI_ControlBase* control)
{
	clear();

	if(!control)
		return true;
	
	reference_.reserve(128);
	control->referenceString(reference_);
	return true;
}

void UI_ControlReferenceBase::clear()
{
	reference_.clear();
}

UI_ControlBase* UI_ControlReferenceBase::control() const
{
	if(isEmpty()) return 0;

	std::string str = reference_.c_str();

	int pos = str.find(".",0);

	if(pos == std::string::npos) return 0;

	UI_Screen* screen = UI_Dispatcher::instance().screen(str.substr(0, pos).c_str());
	if(!screen) return 0;

	int pos1 = str.find(".", pos + 1);
	int len = (pos1 == std::string::npos) ? str.size() - pos - 1 : pos1 - pos - 1; 

	UI_ControlBase* control = screen->getControlByName(str.substr(pos + 1, len).c_str());
	if(!control) return 0;

	pos = pos1;

	while(pos != std::string::npos){
		int pos1 = str.find(".", pos + 1);
		int len = (pos1 == std::string::npos) ? str.size() - pos - 1 : pos1 - pos - 1; 

		control = control->getControlByName(str.substr(pos + 1, len).c_str());
		if(!control) return 0;

		pos = pos1;
	}
	
	return control;
}

                                                                                                                                                                    
bool UI_ControlReferenceBase::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit()){
		static const char* typeName = typeid(UI_ControlReference).name();
		static bool editorRegistered = attribEditorInterface().isTreeEditorRegistered(typeName);	
		if(editorRegistered){
			bool result = ar.openStruct(name, nameAlt, typeName);
			ar.serialize(reference_, "reference", 0);
			ar.closeStruct(name);
			return result;
		}
		else{
			ComboListString combo_str(comboList(), reference_.c_str());
			bool nodeExists = ar.serialize(combo_str, name, nameAlt);

			if(ar.isInput())
				reference_ = combo_str;
			return nodeExists;
		}
	}
	else
		return ar.serialize(reference_, name, nameAlt);
}

const char* UI_ControlReference::comboList() const
{
	static std::string combo_list;
	static unsigned int combo_list_index = 0;

	if(UI_ControlContainer::changeIndex() > combo_list_index){
		UI_Dispatcher::instance().updateControlOwners();
		UI_Dispatcher::instance().controlComboListBuild(combo_list);
		combo_list_index = UI_ControlContainer::changeIndex();
	}

	return combo_list.c_str();
}

const char* UI_ControlReferenceRefinedBase::comboList() const
{
	static std::string combo_list;
	static unsigned int combo_list_index = 0;

	if(UI_ControlContainer::changeIndex() > combo_list_index){
		UI_Dispatcher::instance().updateControlOwners();
		UI_Dispatcher::instance().controlComboListBuild(combo_list, filter());
		combo_list_index = UI_ControlContainer::changeIndex();
	}

	return combo_list.c_str();
}

// ------------------- UI_ScreenReference

UI_ScreenReference::UI_ScreenReference(const UI_Screen* screen)
{
	if(screen)
		init(screen);
}

UI_ScreenReference::UI_ScreenReference(const std::string& screen_name)
{
	screenName_ = screen_name;
}

bool UI_ScreenReference::init(const UI_Screen* screen)
{
	screenName_ = screen->name();
	return true;
}

void UI_ScreenReference::clear()
{
	screenName_.clear();
}

UI_Screen* UI_ScreenReference::screen() const
{
	if(isEmpty()) return 0;
	return UI_Dispatcher::instance().screen(screenName_.c_str());
}

bool UI_ScreenReference::serialize(Archive& ar, const char* name, const char* nameAlt)
{
    if(ar.isEdit()){
		ComboListString combo_str(UI_Dispatcher::instance().screenComboList(), screenName_.c_str());
		bool nodeExists = ar.serialize(combo_str, name, nameAlt);

		if(ar.isInput())
			screenName_ = combo_str;
		return nodeExists;
	}
	else
		return ar.serialize(screenName_, name, nameAlt);
}

