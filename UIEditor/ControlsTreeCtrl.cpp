#include "StdAfx.h"
#include "ControlUtils.h"
#include ".\ControlsTreeCtrl.h"

#include "UIEditor.h"
#include "EditorView.h"
#include "MainFrame.h"

#include "SelectionManager.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_Controls.h"

#include "UITreeObjects.h"

#include "Dictionary.h"
#include "..\Util\mfc\PopupMenu.h"
#include "UITreeObjects.h"

BEGIN_MESSAGE_MAP(CControlsTreeCtrl, CObjectsTreeCtrl)
    ON_NOTIFY_REFLECT(TLN_BEGINLABELEDIT, OnBeginLabelEdit )
	ON_NOTIFY_REFLECT(TLN_ITEMCHECK, OnItemCheck)

//  ON_NOTIFY_REFLECT(TLN_BEGINDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(TLN_DRAGENTER, OnDragEnter)
    ON_NOTIFY_REFLECT(TLN_DRAGLEAVE, OnDragLeave)
//    ON_NOTIFY_REFLECT(TLN_DRAGOVER, OnDragOver)
//  ON_NOTIFY_REFLECT(TLN_DROP, OnDrop)
END_MESSAGE_MAP()



CControlsTreeCtrl::CControlsTreeCtrl()
: drop_target_(0)
, dragged_control_(0)
, popupMenu_(new PopupMenu(100))
{
}

CControlsTreeCtrl::~CControlsTreeCtrl()
{

}

void CControlsTreeCtrl::buildTree () 
{
	clear();
	UI_Dispatcher& dispatcher = UI_Dispatcher::instance();
	UI_Dispatcher::ScreenContainer& screens = dispatcher.screens ();
	UI_Dispatcher::ScreenContainer::iterator it;
	FOR_EACH (screens, it) {
		UI_Screen& screen = *it;
		UITreeObject* screenObject = safe_cast<UITreeObject*>(rootObject()->add(new UITreeObjectScreen(&screen)));

		screenObject->showCheckBox(true);
		screenObject->setCheck(true);
		buildContainerSubtree(screenObject);
		int image = getControlImage(typeid(UI_Screen));
		screenObject->setImage(image, image, image, image);
	}
	UpdateWindow ();
}

void CControlsTreeCtrl::buildContainerSubtree(UITreeObject* object)
{
	UI_ControlContainer& container = *object->controlContainer();
	object->clear();

	UI_ControlContainer::ControlList::const_iterator it;

	if (!dynamic_cast<UI_Screen*>(&container)) {
		UI_ControlBase& base = static_cast<UI_ControlBase&> (container);
		UI_ControlBase::StateContainer::iterator sit;
		FOR_EACH(base.states (), sit){
			UI_ControlState& state = *sit;

			UITreeObject* addedObject = safe_cast<UITreeObject*>(object->add(new UITreeObjectControlState(&state)));

			addedObject->showCheckBox(false);
			//object->setCheck(true);
			int image = getControlImage(typeid(UI_ControlState));
			addedObject->setImage(image, image, image, image);
		}
	}

	FOR_EACH (container.controlList (), it) {
		UI_ControlBase& control = **it;
		const char* typeName = typeid (control).name ();
		const char* nameAlt = TRANSLATE (ClassCreatorFactory<UI_ControlBase>::instance ().find (typeName).nameAlt ());
		CString strName;
		//xassert(findObjectByData(control) == 0);
		
		UITreeObject* addedObject = safe_cast<UITreeObject*>(object->add(new UITreeObjectControl(&control)));

		buildContainerSubtree(addedObject);
		addedObject->showCheckBox(true);
		addedObject->setCheck(control.visibleInEditor());
		int image = getControlImage(typeid(control));
		addedObject->setImage(image, image, image, image);
	}
}

void CControlsTreeCtrl::buildContainerSubtree(UI_ControlContainer& container)
{
	buildContainerSubtree(static_cast<UITreeObject*>(findObjectByData(container)));
}

int CControlsTreeCtrl::getControlImage (const std::type_info& info)
{
	const static std::type_info* types [] = {
		&typeid(UI_Screen),
		&typeid(UI_ControlButton),
		&typeid(UI_ControlCustom),
		&typeid(UI_ControlSlider),
		&typeid(UI_ControlProgressBar),
		&typeid(UI_ControlTab),
		&typeid(UI_ControlStringList),
		&typeid(UI_ControlEdit),
		&typeid(UI_ControlState),
        0
	};

    for (int i = 0; types [i] != 0; ++i) {
        if (*types [i] == info) {
            return (i + 1);
        }
    }
    return 0;
}

void CControlsTreeCtrl::updateSelected ()
{
	CControlsTreeCtrl::ItemType selected = GetSelectedItem ();
	TreeObjectBase* object = objectByItem(selected);
	std::string title = object->name ();
	SetItemText(selected, title.c_str());
}

void CControlsTreeCtrl::selectState(UI_ControlBase* control, int _state_index)
{
	TreeObject* object = findObjectByData(*control);
	TreeObject::iterator it;
	int state_index = 0;
	FOR_EACH(*object, it){
		TreeObject* child = *it;
		if(UI_ControlState* state = child->get<UI_ControlState*>()){
			if(state_index == _state_index){
				child->focus();
				return;
			}
			else
				++state_index;
		}
	}
}

void CControlsTreeCtrl::selectScreen(UI_Screen* screen)
{
	if(TreeObject* object = findObjectByData(*screen)){
		object->focus();
	}
	else
		xassert(0);
	/*
	CControlsTreeCtrl::ItemType item = GetChildItem (TLI_ROOT);
	while (item != 0) {
		if (reinterpret_cast<void*>(screen) == objectByItem(item)->getPointer ()) {
			expandParents(item);
			Select (item, TVGN_CARET);
			EnsureVisible (item, 0);
			return;
		}
		item = GetNextItem (item, TVGN_NEXT);
	}
	*/
}

void CControlsTreeCtrl::spawnMenu(TreeObject* object)
{
	CRect itemRect;
    GetItemRect(objectItem(object), 0, itemRect, TRUE);
    ClientToScreen(itemRect);
	popupMenu_->spawn(CPoint(itemRect.left, itemRect.bottom), GetSafeHwnd());
}


bool CControlsTreeCtrl::selectControl(UI_ControlBase* control)
{
	TreeObject::iterator it;
	
	FOR_EACH(*rootObject(), it){
		UITreeObject* child = safe_cast<UITreeObject*>(*it);
		if(UITreeObject* object = child->findByControlContainer(control)){
			object->focus();
			return true;
		}
	}

	xassert(0);
	return false;
	/*
	UITreeObject* root = safe_cast<UITreeObject*>(rootObject());

	if(UITreeObject* object = root->findByControlContainer(control)){
		object->focus();
		return true;
	}
	else{
		xassert(0);
		return false;
	}
	*/
}

bool CControlsTreeCtrl::selectControl(UI_ControlBase* control, ItemType parent)
{
	return selectControl(control);
	// TODO
	/*
	CControlsTreeCtrl::ItemType item = GetChildItem (parent);
	while (item != 0) {
		if (ItemHasChildren (item) && selectControl (control, item))
			return true;

		if (reinterpret_cast<void*>(control) == objectByItem(item)->getPointer()) {
			expandParents (item);
			Select (item, TVGN_CARET);
			EnsureVisible (item, 0);
			return true;
		}
		item = GetNextItem (item, TVGN_NEXT);
	}
	*/
}

void CControlsTreeCtrl::selectControlInScreen (UI_ControlBase* control, UI_Screen* screen)
{
	selectControl(control);
	// TODO
	/*
	CControlsTreeCtrl::ItemType item = GetChildItem (TLI_ROOT);
	while (item != 0) {
		if (reinterpret_cast<void*>(screen) == objectByItem(item)->getPointer()) {
			if (selectControl (control, item))
				return;
		}
		item = GetNextItem (item, TVGN_NEXT);
	}
	*/
}

void CControlsTreeCtrl::OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDRAG* pNM = reinterpret_cast<NMTREELISTDRAG*>(pNMHDR);
	ItemType selected_one = GetSelectedItem ();
	if (UI_ControlBase* base = objectByItem(selected_one)->get<UI_ControlBase*>()) {
        dragged_control_ = base;
		*pResult = 0;
	} else {
        dragged_control_ = 0;
		*pResult = 1;
	}
}

void CControlsTreeCtrl::OnDragEnter (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);
    *pResult = 0;
}

void CControlsTreeCtrl::OnDragLeave (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);
    *pResult = 0;
	SetTargetItem (0);
}

bool CControlsTreeCtrl::canBeDroppedIn (ItemType dest, ItemType item)
{
	if (dest == item)
		return false;

	ItemType parent = dest;
	while (parent != TLI_ROOT && parent) {
		if (parent == item)
			return false;
        parent = GetParentItem(parent);
	}
	return true;
}

void CControlsTreeCtrl::OnDragOver (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);

	ItemType src = GetSelectedItem();

	if (!src)
		return;

	if (UI_ControlBase* base = objectByItem(pNM->pItem)->get<UI_ControlBase*>()) {
		if (canBeDroppedIn (pNM->pItem, src)) {
			SetTargetItem (pNM->pItem);
			drop_target_ = base;
		} else {
			SetTargetItem (0);
			*pResult = 1;
		}
	} else if (UI_Screen* screen = objectByItem(pNM->pItem)->get<UI_Screen*>()) {
		SetTargetItem (pNM->pItem);
        drop_target_ = screen;
	} else {
		SetTargetItem (0);
		*pResult = 1;
        drop_target_ = 0;
	}
}

void CControlsTreeCtrl::OnDrop (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);

	ShareHandle<UI_ControlBase> dragged_control = dragged_control_;
	SelectItem (0);

    if (drop_target_ && drop_target_ != dragged_control_) {
		
		ItemType draggedControlHandle = objectItem(findObjectByData(*dragged_control));
		xassert(draggedControlHandle && draggedControlHandle != TLI_ROOT);
		
		ItemType parentItem = GetParentItem(draggedControlHandle);
		xassert(parentItem && parentItem != TLI_ROOT);
        
		TreeObjectBase* treeObject = objectByItem(parentItem);
		xassert(treeObject);

		if (UI_ControlBase* parent_control = treeObject->get<UI_ControlBase*>()) {
			deleteObject(findObjectByData(*dragged_control));
			parent_control->removeControl (dragged_control);
		} else if (UI_Screen* parent_control = treeObject->get<UI_Screen*>()) {
			deleteObject(findObjectByData(*dragged_control));
			parent_control->removeControl (dragged_control);
		} else {
			xassert (0);
		}

		drop_target_->addControl (dragged_control);
		xassert(findObjectByData(*dragged_control) == 0);
		/*
		if(getObjectHandle(*dragged_control) != 0) {
			DeleteTreeObject (*dragged_control);
		}
		*/
		buildContainerSubtree (*drop_target_);
		selectControl (dragged_control);
    }
    *pResult = 0;
}

void CControlsTreeCtrl::OnBeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 1;
}
void CControlsTreeCtrl::OnItemCheck (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELIST* pNM = reinterpret_cast<NMTREELIST*>(pNMHDR);
	if (UI_ControlBase* base = objectByItem(pNM->pItem)->get<UI_ControlBase*>()) {
		base->setVisibleInEditor (!bool(pNM->pItem->GetCheck ()));
		*pResult = 0;
	} else {
		*pResult = 1;
	}
	//pNM->pItem->SetCheck(!pNM->pItem->GetCheck());
}

void CControlsTreeCtrl::onRightClick()
{
	if(selection().empty()){
		popupMenu().clear();
		PopupMenuItem& root = popupMenu().root();
		CPoint mousePt;
		GetCursorPos(&mousePt);
		root.add(TRANSLATE("Создать экран")).connect(bindMethod(*uiEditorFrame(), &CUIMainFrame::OnEditAddScreen));
		popupMenu().spawn(mousePt, GetSafeHwnd());
	}
}

BOOL CControlsTreeCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(popupMenu_)
		popupMenu_->onCommand(wParam, lParam);
	
	return CObjectsTreeCtrl::OnCommand(wParam, lParam);
}

void CControlsTreeCtrl::sortControls(UITreeObject* object)
{
	UI_ControlContainer* controlContainer = object->controlContainer();
    controlContainer->sortControls();
	if(!controlContainer->controlList().empty())
		buildContainerSubtree(*controlContainer);
}

// --------------------------------------------------------------------------------------------
#include "EditorAction.h"

CControlsTreeCtrl& EditorAction::controlsTree()
{
	CControlsTreeCtrl* control = uiEditorFrame()->GetControlsTree();
	xassert(control);
	return *control;
}