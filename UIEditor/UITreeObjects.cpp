#include "StdAfx.h"
#include "MainFrame.h"
#include "UIEditorPanel.h"
#include "EditorView.h"
#include "XPrmArchive.h"
#include "Serialization.h"
#include "UITreeObjects.h"

#include "ActionManager.h"
#include "SelectionManager.h"
#include "ControlsTreeCtrl.h"

#include "CreateControlAction.h"
#include "CreateStateAction.h"
#include "EraseAction.h"
#include "SafeCast.h"

#include "..\UserInterface\UserInterface.h"
#include "..\AttribEditor\AttribEditorCtrl.h"

#include "Dictionary.h"
#include "..\Util\mfc\PopupMenu.h"


UITreeObject* UITreeObject::findByControlContainer(UI_ControlContainer* container)
{
	if(controlContainer() == container)
		return this;
	iterator it;
	FOR_EACH(*this, it){
		UITreeObject* object = safe_cast<UITreeObject*>(*it);
		if(UITreeObject* result = object->findByControlContainer(container))
			return result;
	}
	return 0;
}

CControlsTreeCtrl& UITreeObject::tree()
{
	return static_cast<CControlsTreeCtrl&>(*tree_);
}

UI_ControlContainer* UITreeObject::controlContainer()
{
	return 0;
}

CAttribEditorCtrl& UITreeObject::attribEditor()
{
	return uiEditorFrame()->GetPanel()->attribEditor();
}


TreeObject* findScreenByChild(TreeObject* object) {
    while(object){
        if(object->get<UI_Screen*>())
            return object;
		else
			object = object->parent();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

UITreeObjectControl::UITreeObjectControl(UI_ControlBase* control)
: control_(control)
{
    xassert(control_);
}

UI_ControlContainer* UITreeObjectControl::controlContainer()
{
	return control_;
}

template<class T>
void makeControlsSubmenu(UI_ControlContainer *const container, PopupMenuItem& root, T callback)
{
    const ComboStrings& strings = ClassCreatorFactory<UI_ControlBase>::instance().comboStringsAlt();
    ComboStrings::const_iterator it;
    int index = 0;
	FOR_EACH(strings, it){
		const char* text = TRANSLATE(it->c_str());
		ComboStrings path;
		splitComboList(path, text, '\\');
		PopupMenuItem* item = &root;
		for(int path_position = 0; path_position < path.size(); ++path_position){
			const char* leaf = path[path_position].c_str();
			if(path_position + 1 == path.size()){
				item->add(leaf).connect(bindArgument(callback, index++));
			}
			else{
				if(PopupMenuItem* subItem = item->find(leaf))
					item = subItem;
				else
					item = &item->add(leaf);
			}
		}
	}
}

void UITreeObjectControl::onRightClick()
{
	PopupMenu& menu = tree().popupMenu();
	menu.clear();
    PopupMenuItem& root = menu.root();
	PopupMenuItem& addedItem = root.add(TRANSLATE("Добавить контрол"));
	makeControlsSubmenu(control_, addedItem, bindMethod(*this, onMenuAddControl));
	if(addedItem.empty())
		addedItem.enable(false);

	root.add(TRANSLATE("Добавить состояние")).connect(bindMethod(*this, onMenuAddState));
	root.addSeparator();
	root.add(TRANSLATE("Сортировать по имени")).connect(bindArgument(bindMethod(tree(), &CControlsTreeCtrl::sortControls), this));
	root.addSeparator();
	root.add(TRANSLATE("Удалить")).connect(bindMethod(*this, onMenuDelete));
	root.addSeparator();
	root.add(TRANSLATE("Клонировать")).connect(bindMethod(*this, onMenuDuplicate));
	tree().spawnMenu(this);
}

void UITreeObjectControl::onMenuAddState()
{
    tree().SelectItem(0);

    if(control_){
        CreateStateAction* action = new CreateStateAction(*control_);
        ActionManager::the().act(action);
        tree().buildContainerSubtree(this);
        tree().selectState(control_, action->stateIndex());
		ActionManager::the().setChanged();
    }
}

void UITreeObjectControl::onMenuAddControl(int index)
{
	tree().SelectItem (0);

	UI_ControlContainer* container = control_;
	CreateControlAction* action = new CreateControlAction(*container, index);
	ActionManager::the().act(action);
	tree().buildContainerSubtree(this);
	tree().selectControl (action->added_control());
}

void UITreeObjectControl::onMenuDelete()
{
	UI_ControlBase* control = control_;
	UI_ControlContainer* container = parent()->controlContainer();

	attribEditor().detachData();
    SelectionManager::the().deselectAll();
    SelectionManager::the().select(control);

	tree().deleteObject(this);
	ActionManager::the().act(new EraseControlAction(SelectionManager::the().selection(), *container));
}

void UITreeObjectControl::onMenuDuplicate()
{
	const char* clipboard_filename = "Scripts\\TreeControlSetups\\UIEditorClipboard";
	{
		XPrmOArchive oarchive (clipboard_filename);
		oarchive.serializePolymorphic(control_, "control", "control");
	}
	UI_ControlBase* new_control = 0;
	{
		XPrmIArchive iarchive (clipboard_filename);
		iarchive.serializePolymorphic(new_control, "control", "control");
	}
	xassert(new_control);
	new_control->updateControlOwners();

	parent()->controlContainer()->addControl(new_control);

	CControlsTreeCtrl& tree = this->tree();
	tree.buildContainerSubtree(parent());
	tree.selectControl(new_control);
	ActionManager::the().setChanged();
}

void UITreeObjectControl::onSelect()
{
	if(TreeObject* screenObject = findScreenByChild(this)){
		if(UI_Screen* screen = screenObject->get<UI_Screen*>()){
			uiEditorFrame()->view().setCurrentScreen(screen);
		}
	}
    SelectionManager::the().select(control_);
	uiEditorFrame()->view().updateProperties();
}

const char* UITreeObjectControl::name() const
{
    name_ = "(";
    name_ += control_->name();
    name_ += ")";
    return name_.c_str();
}


void* UITreeObjectControl::getPointer() const
{
    return static_cast<void*>(control_);
}

Serializeable UITreeObjectControl::getSerializeable(const char* name, const char* nameAlt)
{
    return Serializeable(*control_, name, nameAlt);
}

const type_info& UITreeObjectControl::getTypeInfo() const {
    return typeid(UI_ControlBase);
}

bool UITreeObjectControl::canBeDropped(TreeObject* object)
{
	if(object == this)
		return false;

	TreeObject* parent = this;
	while(parent){
		if(parent == object)
			return false;
        parent = parent->parent();
	}
	return true;
}

bool UITreeObjectControl::onDragOver(const TreeObjects& objects)
{
	UITreeObjectControl* source = safe_cast<UITreeObjectControl*>(objects.front());
	return canBeDropped(objects.front());
}

void UITreeObjectControl::onDrop(const TreeObjects& objects)
{
	TreeObject* object = objects.front();
	UITreeObjectControl* source = safe_cast<UITreeObjectControl*>(object);
	xassert(canBeDropped(source));

	UI_ControlContainer* sourceParent = source->parent()->controlContainer();
	ShareHandle<UI_ControlBase> control = source->control();
	tree().deleteObject(source);
	source = 0;

	controlContainer()->addControl(control);
	sourceParent->removeControl(control);
	tree().buildContainerSubtree(this);
	
	iterator it;
	FOR_EACH(*this, it){
		UITreeObject* object = safe_cast<UITreeObject*>(*it);
		if(object->controlContainer() == control){
			object->focus();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

UITreeObjectScreen::UITreeObjectScreen(UI_Screen* screen)
: screen_(screen)
{
    xassert(screen_);
}

UI_ControlContainer* UITreeObjectScreen::controlContainer()
{
	return screen_;
}

void UITreeObjectScreen::onDrop(const TreeObjects& objects)
{
	TreeObject* object = objects.front();
	UITreeObjectControl* source = safe_cast<UITreeObjectControl*>(object);
	
	UI_ControlContainer* sourceParent = source->parent()->controlContainer();
	ShareHandle<UI_ControlBase> control = source->control();
	tree().deleteObject(source);
	source = 0;

	controlContainer()->addControl(control);
	sourceParent->removeControl(control);
	tree().buildContainerSubtree(this);
	
	iterator it;
	FOR_EACH(*this, it){
		UITreeObject* object = safe_cast<UITreeObject*>(*it);
		if(object->controlContainer() == control){
			object->focus();
		}
	}
}

void UITreeObjectScreen::onSelect()
{
	/*
	if (screen != view.currentScreen()) {
		view.setCurrentScreen(screen);

		ItemType item = GetChildItem(TLI_ROOT);
		while (item) {
			if (item != pNM->pItem){
				Expand (item, TLE_COLLAPSE);
			}
            else{
				Expand (item, TLE_EXPAND);
				ItemType child_item = GetChildItem (item);
				while(child_item){
					Expand (child_item, TLE_EXPAND);
					child_item = GetNextSiblingItem(child_item);
				}

			}
			item = GetNextSiblingItem (item);
		}
	}
	*/
	xassert(screen_);
    uiEditorFrame()->view().setCurrentScreen(screen_);
	uiEditorFrame()->view().updateProperties();
}

void UITreeObjectScreen::onRightClick()
{
	PopupMenu& menu = tree().popupMenu();
	menu.clear();
    PopupMenuItem& root = menu.root();

	PopupMenuItem& addedItem = root.add(TRANSLATE("Добавить контрол"));
	makeControlsSubmenu(screen_, addedItem, bindMethod(*this, onMenuAddControl));
	if(addedItem.empty())
		addedItem.enable(false);

	root.addSeparator();
	root.add(TRANSLATE("Сортировать по имени")).connect(bindArgument(bindMethod(tree(), &CControlsTreeCtrl::sortControls), this));
	root.addSeparator();
	root.add(TRANSLATE("Удалить")).connect(bindMethod(*this, onMenuDelete));
	root.addSeparator();
	root.add(TRANSLATE("Клонировать")).connect(bindMethod(*this, onMenuDuplicate));
	tree().spawnMenu(this);
}

const char* UITreeObjectScreen::name() const
{
    xassert(screen_);
    name_ = "[";
    name_ += screen_->name();
    name_ += "]";
    return name_.c_str();
}

void* UITreeObjectScreen::getPointer() const
{
    return static_cast<void*>(screen_);
}

Serializeable UITreeObjectScreen::getSerializeable(const char* name, const char* nameAlt)
{
    return Serializeable(*screen_, name, nameAlt);
}
const type_info& UITreeObjectScreen::getTypeInfo() const
{
    return typeid(*screen_); 
}

void UITreeObjectScreen::onMenuAddControl(int index)
{
	tree().SelectItem (0);

	UI_ControlContainer* container = screen_;
	CreateControlAction* action = new CreateControlAction(*container, index);
	ActionManager::the().act(action);
	tree().buildContainerSubtree(this);
	tree().selectControl (action->added_control());
}

void UITreeObjectScreen::onMenuDelete()
{
	// TODO: сделать Action
    attribEditor().detachData();
    UI_Dispatcher::instance().removeScreen(screen_->name());
    UI_Dispatcher::instance().init();
    uiEditorFrame()->view().setCurrentScreen(0);
    tree().buildTree();
	ActionManager::the().setChanged();
}

void UITreeObjectScreen::onMenuDuplicate()
{
	const char* clipboard_filename = "Scripts\\TreeControlSetups\\UIEditorClipboard";
	std::string name = "Copy of ";
	name += screen_->name();
	bool screenAdded = UI_Dispatcher::instance().addScreen(name.c_str());
	xassert(screenAdded);
	UI_Screen* newScreen = UI_Dispatcher::instance().screen(name.c_str());
	xassert(newScreen);
	{
		XPrmOArchive oarchive (clipboard_filename);
		oarchive.serialize(*screen_, "screen", "screen");
	}
	{
		XPrmIArchive iarchive (clipboard_filename);
		iarchive.serialize(*newScreen, "screen", "screen");
	}
	newScreen->updateControlOwners();
	newScreen->setName(name.c_str());
	CControlsTreeCtrl& tree = this->tree();
	tree.buildTree();
	tree.selectScreen(newScreen);
	ActionManager::the().setChanged();
}

//////////////////////////////////////////////////////////////////////////////

UITreeObjectControlState::UITreeObjectControlState(UI_ControlState* state)
: state_(state)
{
    xassert(state_);
}

const char* UITreeObjectControlState::name() const
{
    xassert(state_);
    name_ = "[";
    name_ += state_->name();
    name_ += "]";
    return name_.c_str();
}

void* UITreeObjectControlState::getPointer() const
{
    return static_cast<void*>(state_);
}

Serializeable UITreeObjectControlState::getSerializeable(const char* name, const char* nameAlt)
{
    return Serializeable(*state_, name, nameAlt);
}

const type_info& UITreeObjectControlState::getTypeInfo() const
{
    return typeid(*state_);
}

void UITreeObjectControlState::onSelect()
{
	attribEditor().attachSerializeable(Serializeable(*state_));
}

void UITreeObjectControlState::onRightClick()
{
	PopupMenu& menu = tree().popupMenu();
	menu.clear();
    PopupMenuItem& root = menu.root();
	root.add(TRANSLATE("Удалить")).connect(bindMethod(*this, onMenuDelete));
	tree().spawnMenu(this);
}

void UITreeObjectControlState::onMenuDelete()
{
	UI_ControlBase* control = safe_cast<UI_ControlBase*>(parent()->controlContainer());
    // TODO: сделать RemoveStateAction
    UI_ControlBase::StateContainer::iterator it;
    FOR_EACH(control->states(), it){
        if(&*it == state_){
            control->states().erase (it);
            control->setState(0);
            control->init();
            UI_Dispatcher::instance().init();
            tree().buildContainerSubtree(parent());
			ActionManager::the().setChanged();
            return;
        }
    }
}

