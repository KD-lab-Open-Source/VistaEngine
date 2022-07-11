#include "StdAfx.h"
#include "MainFrame.h"
#include "UIEditorPanel.h"
#include "EditorView.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization\Serialization.h"
#include "UITreeObjects.h"

#include "ActionManager.h"
#include "SelectionManager.h"
#include "ControlsTreeCtrl.h"

#include "CreateControlAction.h"
#include "CreateStateAction.h"
#include "EraseAction.h"
#include "XTL\SafeCast.h"

#include "UserInterface\UserInterface.h"
#include "AttribEditor\AttribEditorCtrl.h"

#include "Serialization\Dictionary.h"
#include "Serialization\SerializationFactory.h"

#include "kdw/PopupMenu.h"
#include "kdw/ClassMenu.h"


int getControlImage(const std::type_info& info, bool visibleInEditor);

void UITreeRoot::rebuild()
{
	clear();
	UI_Dispatcher& dispatcher = UI_Dispatcher::instance();
	UI_Dispatcher::ScreenContainer& screens = dispatcher.screens ();
	UI_Dispatcher::ScreenContainer::iterator it;
	FOR_EACH(screens, it){
		UI_Screen& screen = *it;
		UITreeObject* screenObject = add(new UITreeObjectScreen(&screen));

		screenObject->showCheckBox(true);
		screenObject->setCheck(true);
		screenObject->rebuild();
		int image = getControlImage(typeid(UI_Screen), true);
		screenObject->setImage(image);
	}
}

// ---------------------------------------------------------------------------

void UITreeObject::onMenuAddControl(int index, ControlsTree* tree)
{
	xassert(index >= 0);
	UI_ControlContainer* container = controlContainer();;
	CreateControlAction* action = new CreateControlAction(*container, index);
	ActionManager::the().act(action);
	rebuild();
	UITreeObject* child = findByControlContainer(action->added_control());
	tree->selectObject(child);
}

UITreeObject* UITreeObject::findByControlContainer(UI_ControlContainer* container, bool recurse, bool onlyCheck)
{
	if(controlContainer() == container)
		return this;
	iterator it;
	FOR_EACH(*this, it){
		UITreeObject* object = safe_cast<UITreeObject*>(&**it);
		if(!onlyCheck){
			if(UITreeObject* result = object->findByControlContainer(container, recurse, recurse == false))
				return result;
		}
	}
	return 0;
}

void UITreeObject::sortControls()
{
	controlContainer()->sortControls();
	if(!controlContainer()->controlList().empty())
		rebuild();
}

UITreeObjectControlState* UITreeObject::stateByIndex(int index)
{
	int i = 0;
	iterator it;
	FOR_EACH(*this, it){
		UITreeObjectControlState* child = dynamic_cast<UITreeObjectControlState*>(&**it);
		if(child){
			if(i == index)
				return child;
			++i;
		}
	}
	return 0;
}

void UITreeObject::onRightClick(kdw::ObjectsTree* tree)
{
	onRightClick(safe_cast<ControlsTree*>(tree));
}

void UITreeObject::rebuild()
{
	UI_ControlContainer* container = controlContainer();
	xassert(container);
	clear();

	UI_ControlContainer::ControlList::const_iterator it;

	if(!dynamic_cast<UI_Screen*>(container)){
		UI_ControlBase& base = static_cast<UI_ControlBase&>(*container);
		UI_ControlBase::StateContainer::iterator sit;
		FOR_EACH(base.states (), sit){
			UI_ControlState& state = *sit;

			UITreeObjectControlState* addedObject = add(new UITreeObjectControlState(&state));
		}
	}

	FOR_EACH(container->controlList(), it){
		UI_ControlBase& control = **it;
		const char* typeName = typeid (control).name ();
		const char* nameAlt = TRANSLATE(FactorySelector<UI_ControlBase>::Factory::instance().find (typeName).nameAlt ());
		
		UITreeObject* addedObject = add(new UITreeObjectControl(&control));

		addedObject->rebuild();
	}
}

UI_ControlContainer* UITreeObject::controlContainer()
{
	return 0;
}

const UI_ControlContainer* UITreeObject::controlContainer() const
{
	return const_cast<UITreeObject*>(this)->controlContainer();
}
CAttribEditorCtrl& UITreeObject::attribEditor()
{
	return uiEditorFrame()->GetPanel()->attribEditor();
}


UITreeObjectScreen* findScreenByChild(UITreeObject* object) {
    while(object){
        if(UITreeObjectScreen* screen = dynamic_cast<UITreeObjectScreen*>(object))
            return screen;
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
	updateText();
}

UI_ControlContainer* UITreeObjectControl::controlContainer()
{
	return control_;
}

struct ControlsMenuItemAdder : public kdw::ClassMenuItemAdder{
	ControlsMenuItemAdder(UITreeObject* object, ControlsTree* tree)
	: tree_(tree)
	, object_(object) {}

	UITreeObject* object_;
	ControlsTree* tree_;

	kdw::PopupMenuItem& operator()(kdw::PopupMenuItem& root, const char* text){
		return root.add(text);
	}
	void operator()(kdw::PopupMenuItem& root, int index, const char* text){
		root.add(text, index, tree_).connect(object_, &UITreeObject::onMenuAddControl);
	}
};

void UITreeObjectControl::onRightClick(ControlsTree* tree)
{
	kdw::PopupMenu menu(100);
	menu.clear();
	kdw::PopupMenuItem& root = menu.root();
	kdw::PopupMenuItem& addedItem = root.add(TRANSLATE("Добавить контрол"));
	
	ControlsMenuItemAdder(this, tree).generateMenu(addedItem, FactorySelector<UI_ControlBase>::Factory::instance().comboStringsAlt(), false);
	if(addedItem.empty())
		addedItem.enable(false);

	root.add(TRANSLATE("Добавить состояние"), tree).connect(this, &UITreeObjectControl::onMenuAddState);
	root.addSeparator();
	root.add(TRANSLATE("Сортировать по имени")).connect(static_cast<UITreeObject*>(this), &UITreeObject::sortControls);
	root.addSeparator();
	root.add(TRANSLATE("Удалить")).connect(this, &UITreeObjectControl::onMenuDelete);
	root.addSeparator();
	root.add(TRANSLATE("Клонировать"), tree).connect(this, &UITreeObjectControl::onMenuDuplicate);

	menu.spawn(tree);
}

void UITreeObjectControl::onMenuAddState(ControlsTree* tree)
{
    if(control_){
        CreateStateAction* action = new CreateStateAction(*control_);
        ActionManager::the().act(action);
        rebuild();
		UITreeObjectControlState* child = stateByIndex(action->stateIndex());
		ActionManager::the().setChanged();
		tree->selectObject(child);
		tree->update();
    }
}


void UITreeObjectControl::onMenuDelete()
{
	UI_ControlBase* control = control_;
	UI_ControlContainer* container = parent()->controlContainer();

	attribEditor().detachData();
    SelectionManager::the().deselectAll();
    SelectionManager::the().select(control);

	remove();
	ActionManager::the().act(new EraseControlAction(SelectionManager::the().selection(), *container));
}

void UITreeObjectControl::onMenuDuplicate(ControlsTree* tree)
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

	UITreeObject* parent = this->parent();
	parent->rebuild();
	if(UITreeObject* object = parent->findByControlContainer(new_control))
		tree->selectObject(object);
	
	ActionManager::the().setChanged();
}

void selectionFromTree(Selection& selection, UI_ControlBase* control)
{
	UI_ControlBase::ControlList& controlList = const_cast<UI_ControlBase::ControlList&>(control->controlList());
	UI_ControlBase::ControlList::iterator it;
	FOR_EACH(controlList, it){
		selection.push_back(*it);
		selectionFromTree(selection, *it);
	}
}


void UITreeObjectControl::onSelect(kdw::ObjectsTree* tree)
{
	if(UITreeObjectScreen* screenObject = findScreenByChild(this)){
		if(UI_Screen* screen = screenObject->screen())
			uiEditorFrame()->view().setCurrentScreen(screen);
	}
	if(GetAsyncKeyState(VK_SHIFT) >> 15 && !control_->controlList().empty()){
		Selection selection;
		selection.push_back(control_);
		selectionFromTree(selection, control_);
		SelectionManager::the().select(selection);
	}
	else
		SelectionManager::the().select(control_);
	uiEditorFrame()->view().updateProperties();
}

void UITreeObjectControl::updateText()
{
	setText(name().c_str());
	xassert(control_);
	int image = getControlImage(typeid(*control_), control_->visibleInEditor());
	setImage(image);
}

std::string UITreeObjectControl::name() const
{
	std::string str  = "(";
    str += control_->name();
    str += ")";
    return str;
}


Serializer UITreeObjectControl::getSerializer(const char* name, const char* nameAlt)
{
    return Serializer(*control_, name, nameAlt);
}

const type_info& UITreeObjectControl::getTypeInfo() const {
    return typeid(UI_ControlBase);
}

bool UITreeObjectControl::canBeDroppedOn(const kdw::TreeRow* _row, const kdw::TreeRow* _beforeChild, const kdw::Tree* _tree, bool direct) const
{
	const UITreeObject* row = safe_cast<const UITreeObject*>(_row);
	if(_beforeChild && dynamic_cast<const UITreeObjectControl*>(_beforeChild) == 0)
		return false;
	if(row == this)
		return false;
	if(row->isChildOf(this))
		return false;
	if(row->controlContainer() != 0)
		return true;
	return false;
}

void UITreeObjectControl::dropInto(kdw::TreeRow* _destination, kdw::TreeRow* _beforeChild, kdw::Tree* _tree)
{	
	UITreeObject* destination = safe_cast<UITreeObject*>(_destination);
	UITreeObjectControl* beforeChild = dynamic_cast<UITreeObjectControl*>(_beforeChild);
	ControlsTree* tree = safe_cast<ControlsTree*>(_tree);

	UI_ControlContainer* sourceParent = parent()->controlContainer();
	ShareHandle<UI_ControlBase> control = this->control();
	remove(); // suicide!
	sourceParent->removeControl(control);

	xassert(destination->controlContainer());
	destination->controlContainer()->addControl(control, beforeChild ? beforeChild->control() : 0);
	destination->rebuild();
	UITreeObject* object = destination->findByControlContainer(control);
	if(object)
		tree->selectObject(object);
}


//////////////////////////////////////////////////////////////////////////////

UITreeObjectScreen::UITreeObjectScreen(UI_Screen* screen)
: screen_(screen)
{
    xassert(screen_);
	updateText();
}

UI_ControlContainer* UITreeObjectScreen::controlContainer()
{
	return screen_;
}

void UITreeObjectScreen::onSelect(kdw::ObjectsTree* tree)
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

void UITreeObjectScreen::onRightClick(ControlsTree* tree)
{
	kdw::PopupMenu menu(100);

	kdw::PopupMenuItem& root = menu.root();

	kdw::PopupMenuItem& addedItem = root.add(TRANSLATE("Добавить контрол"));

	ControlsMenuItemAdder(this, tree).generateMenu(addedItem, FactorySelector<UI_ControlBase>::Factory::instance().comboStringsAlt());

	if(addedItem.empty())
		addedItem.enable(false);

	root.addSeparator();
	root.add(TRANSLATE("Сортировать по имени")).connect(static_cast<UITreeObject*>(this), &UITreeObject::sortControls);
	root.addSeparator();
	root.add(TRANSLATE("Удалить"), tree).connect(this, onMenuDelete);
	root.addSeparator();
	root.add(TRANSLATE("Клонировать"), tree).connect(this, onMenuDuplicate);
	menu.spawn(tree);
}

void UITreeObjectScreen::updateText()
{
	setText(name().c_str());
}

std::string UITreeObjectScreen::name() const
{
    xassert(screen_);
	std::string str;
    str = "[";
    str += screen_->name();
    str += "]";
    return str.c_str();
}


Serializer UITreeObjectScreen::getSerializer(const char* name, const char* nameAlt)
{
    return Serializer(*screen_, name, nameAlt);
}
const type_info& UITreeObjectScreen::getTypeInfo() const
{
    return typeid(*screen_); 
}

void UITreeObjectScreen::onMenuDelete(ControlsTree* tree)
{
	// TODO: сделать Action
    attribEditor().detachData();
    UI_Dispatcher::instance().removeScreen(screen_->name());
    UI_Dispatcher::instance().init();
    uiEditorFrame()->view().setCurrentScreen(0);
    tree->root()->rebuild();
	ActionManager::the().setChanged();
}

void UITreeObjectScreen::onMenuDuplicate(ControlsTree* tree)
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
	tree->root()->rebuild();
	if(UITreeObject* object = tree->root()->findByControlContainer(newScreen))
		tree->selectObject(object);
	ActionManager::the().setChanged();
}

//////////////////////////////////////////////////////////////////////////////

UITreeObjectControlState::UITreeObjectControlState(UI_ControlState* state)
: state_(state)
{
    xassert(state_);
	updateText();
}

void UITreeObjectControlState::updateText()
{
	setText(name().c_str());
	int image = getControlImage(typeid(UI_ControlState), true);
	setImage(image);
}

std::string UITreeObjectControlState::name() const
{
    xassert(state_);
	std::string str;
    str = "[";
    str += state_->name();
    str += "]";
    return str;
}

Serializer UITreeObjectControlState::getSerializer(const char* name, const char* nameAlt)
{
    return Serializer(*state_, name, nameAlt);
}

const type_info& UITreeObjectControlState::getTypeInfo() const
{
    return typeid(*state_);
}

void UITreeObjectControlState::onSelect(kdw::ObjectsTree* tree)
{
	attribEditor().attachSerializer(Serializer(*state_));
	UI_ControlBase* control = safe_cast<UI_ControlBase*>(parent()->controlContainer());
	UI_ControlBase::StateContainer::iterator it;
	FOR_EACH(control->states(), it)
		if(&*it == state_)
			control->setState(distance(control->states().begin(), it));
}

void UITreeObjectControlState::onRightClick(ControlsTree* tree)
{
	kdw::PopupMenu menu(100);
	
	kdw::PopupMenuItem& root = menu.root();
	root.add(TRANSLATE("Удалить"), tree).connect(this, onMenuDelete);
	menu.spawn(tree);
}

void UITreeObjectControlState::onMenuDelete(ControlsTree* tree)
{
	UI_ControlBase* control = safe_cast<UI_ControlBase*>(parent()->controlContainer());
    // TODO: сделать RemoveStateAction
	control->removeState(state_);
    UI_Dispatcher::instance().init();
    parent()->rebuild();
	ActionManager::the().setChanged();
	tree->update();
}

