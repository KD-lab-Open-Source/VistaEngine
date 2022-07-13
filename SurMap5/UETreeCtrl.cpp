#include "stdafx.h"
#include "UETreeCtrl.h"
#include "UnitEditorDlg.h"
#include "SafeCast.h"
#include "..\Util\mfc\PopupMenu.h"

//////////////////////////////////////////////////////////////////////////////
#include "UnitEditorLogicParameter.h"
#include "UnitEditorLogicProjectile.h"
#include "UnitEditorLogicSource.h"
#include "UnitEditorLogicUnit.h"
#include "UnitEditorLogicWeapon.h"
#include "UnitEditorLogicSearch.h"

REGISTER_CLASS(UnitEditorLogicBase, UnitEditorLogicUnit,       "Юниты");
REGISTER_CLASS(UnitEditorLogicBase, UnitEditorLogicWeapon,     "Оружие");
REGISTER_CLASS(UnitEditorLogicBase, UnitEditorLogicSource,     "Источники");
REGISTER_CLASS(UnitEditorLogicBase, UnitEditorLogicProjectile, "Снаряды");
REGISTER_CLASS(UnitEditorLogicBase, UnitEditorLogicParameter,  "Параметры");
REGISTER_CLASS(UnitEditorLogicBase, UnitEditorLogicSearch,     "Результаты поиска");
//////////////////////////////////////////////////////////////////////////////
CUETreeCtrl* UnitEditorLogicObjectCreatorBase::unitEditorTreeCtrl_ = 0;

IMPLEMENT_DYNAMIC(CUETreeCtrl, CTreeListCtrl)

BEGIN_MESSAGE_MAP(CUETreeCtrl, CTreeListCtrl)
	ON_NOTIFY_REFLECT(TLN_SELCHANGED, OnSelChanged)
	ON_NOTIFY_REFLECT(TLN_SELCHANGING, OnSelChanging)

	ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClick)

    ON_NOTIFY_REFLECT (TLN_BEGINDRAG, OnBeginDrag )
    ON_NOTIFY_REFLECT (TLN_DRAGENTER, OnDragEnter )
    ON_NOTIFY_REFLECT (TLN_DRAGLEAVE, OnDragLeave )
    ON_NOTIFY_REFLECT (TLN_DRAGOVER, OnDragOver )
    ON_NOTIFY_REFLECT (TLN_DROP, OnDrop )
END_MESSAGE_MAP()


CUETreeCtrl::CUETreeCtrl(CAttribEditorCtrl& attribEditor)
: attribEditor_(attribEditor)
, draggedItem_(0)
, logicClassIndex_(0)
, popupMenu_(new PopupMenu(100))
{
}



void CUETreeCtrl::OnSelChanging(NMHDR *pNMHDR, LRESULT *result)
{
	NMTREELIST* nmTreeList = reinterpret_cast<NMTREELIST*>(pNMHDR);
	ItemType item = nmTreeList->pItem;

}

void CUETreeCtrl::OnSelChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTREELIST* pNotify = reinterpret_cast<NMTREELIST*>(pNMHDR);
	ItemType item = pNotify->pItem;
	if(logic_) {
        if (item == 0)
            return;

		//lastSelectedItem_ = item;

        if ((DWORD(item) & 0xFFFF0000) == 0) {
            xassert("Всё плохо :(");
            return; // очень странно...
        }
		//if(currentPosition_)
		//	addToHistory(currentPosition_);
        logic_->clickedItem_ = item;
	    logic_->onItemSelected(item);
	}
}


void CUETreeCtrl::OnDoubleClick(NMHDR *nm, LRESULT *result)
{
	NMTREELIST* pNotify = reinterpret_cast<NMTREELIST*>(nm);
	ItemType item = pNotify->pItem;
    if(logic_) {
        if(item == 0)
            return;
        if((DWORD(item) & 0xFFFF0000) == 0){
            xassert("Всё плохо :(");
            return; // очень странно...
        }
        logic_->clickedItem_ = item;

		CObjectsTreeCtrl::OnItemDblClick(nm, result);
        //logic_->onDoubleClick(item);
    }
}

void CUETreeCtrl::OnRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTREELIST* pNotify = reinterpret_cast<NMTREELIST*>(pNMHDR);
	ItemType item = pNotify->pItem;
    if(logic_) {
        if(item == 0)
            return;
        if((DWORD(item) & 0xFFFF0000) == 0){
            xassert("Всё плохо :(");
            return; // очень странно...
        }
        logic_->clickedItem_ = item;
        logic_->onItemRClick(item);
    }
}

CUETreeCtrl::~CUETreeCtrl()
{
    delete popupMenu_;
	popupMenu_ = 0;
}

BOOL CUETreeCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(logic_){
		popupMenu().onCommand(wParam, lParam);
		logic_->onCommand(wParam);
	}

	return __super::OnCommand(wParam, lParam);
}

CUnitEditorDlg& CUETreeCtrl::unitEditorDialog()
{
	CUnitEditorDlg* dialog = safe_cast<CUnitEditorDlg*>(GetParent());
	xassert(dialog);
	return *dialog;
}

void CUETreeCtrl::setLogicPosition(BookmarkBase* bookmark)
{
	currentPosition_ = bookmark;
}

void CUETreeCtrl::setLogic(int logicClassIndex)
{
	if(!logic_ || logicClassIndex_ != logicClassIndex){
		UnitEditorLogicObjectCreatorBase::unitEditorTreeCtrl_ = const_cast<CUETreeCtrl*>(this);
		unitEditorDialog().setCurrentTab(logicClassIndex);
		UnitEditorLogicBase* logic = ClassCreatorFactory<UnitEditorLogicBase>::instance().createByIndex(logicClassIndex);
		setLogic(logic);
		logicClassIndex_ = logicClassIndex;
		UnitEditorLogicObjectCreatorBase::unitEditorTreeCtrl_ = 0;
	}
}

void CUETreeCtrl::setLogic(UnitEditorLogicBase* _logic)
{
	logic_ = _logic;
	clear();
	attribEditor_.detachData();
	if(logic_)
		logic_->rebuildTree();
}

void CUETreeCtrl::beforeJumping(CAttribEditorCtrl::ItemType item)
{
	logic_->onBeforeJumping(item);
	addToHistory(currentPosition_);
}

void CUETreeCtrl::beforeElementChanged()
{
	logic_->onBeforeElementChanged();
}

void CUETreeCtrl::afterElementChanged()
{
	logic_->onAfterElementChanged();
}

void CUETreeCtrl::addToHistory(BookmarkBase* bookmark)
{
	history_.push_back(bookmark);
	unitEditorDialog().updateControls();
}

void CUETreeCtrl::goBack()
{
	if(!history_.empty()) {
		BookmarkBase* bookmark = history_.back();

		if(logicIndex() != bookmark->logicClassIndex())
			setLogic(bookmark->logicClassIndex());

		bookmark->visit(*this);
	}

	if(!history_.empty())
		history_.pop_back();
	unitEditorDialog().updateControls();
}

UnitEditorLogicBase::UnitEditorLogicBase(CUETreeCtrl& tree)
: tree_(tree)
{
}

void UnitEditorLogicBase::popupItemMenu(PopupMenu& menu, ItemType item)
{
	CRect rt;
	tree_.GetItemRect(item, 0, &rt, TRUE);
	tree_.ClientToScreen(&rt);
	menu.spawn(CPoint(rt.left, rt.bottom), tree_.GetSafeHwnd());
}

void UnitEditorLogicBase::popupItemMenu(UINT menuID, ItemType item)
{
	CRect rt;
	tree_.GetItemRect(item, 0, &rt, TRUE);
	tree_.ClientToScreen(&rt);
	CMenu menu;
	menu.LoadMenu(menuID);
	CMenu* child_menu = menu.GetSubMenu (0);
	child_menu->TrackPopupMenu(TPM_LEFTBUTTON | TPM_LEFTALIGN, rt.left, rt.bottom, &tree_);
}

CString UnitEditorLogicBase::expandStateFilename_;
ShareHandle<TreeNode> UnitEditorLogicBase::attributeClipboard_;

CAttribEditorCtrl& UnitEditorLogicBase::attribEditor()
{
	return tree_.attribEditor();
}

void UnitEditorLogicBase::rebuildTree()
{
	tree_.AllowRedraw(FALSE);
	tree_.clear();
	onTreeRebuild();
	tree_.AllowRedraw(TRUE);
}

void UnitEditorLogicBase::onItemSelected(ItemType item)
{
	saveExpandState();
	attribEditor().detachData ();

	if (!tree_.GetItemState (item, TLIS_SELECTED))
		return;

    if(TreeObject* object = tree_.objectByItem(item)){

		std::string name;
		int index;

		if(isEditable(object, name, index)){
			attribEditor().attachSerializeable(object->getSerializeable());

			if(name != ""){
				loadExpandState(name.c_str(), index);
			}
			else{
				resetExpandState();
			}

			tree_.setLogicPosition(new BookmarkGeneric(ClassCreatorFactory<UnitEditorLogicBase>::instance().typeIndexByName(typeid(*this).name()), tree_, item));
		}
		else
			attribEditor().detachData();
	}
	attribEditor().RedrawWindow ();
}


void UnitEditorLogicBase::onItemSelecting(ItemType item, ItemType oldItem)
{
	//if(oldItem != 0)
		//tree_.addToHistory(new BookmarkGeneric(tree_.logicIndex(), tree_, oldItem));
}

void UnitEditorLogicBase::onBeforeJumping(CAttribEditorCtrl::ItemType item)
{
	ShareHandle<BookmarkBase> bookmark = new BookmarkGeneric(ClassCreatorFactory<UnitEditorLogicBase>::instance().typeIndexByName(typeid(*this).name()), tree_, tree_.GetSelectedItem());
	tree_.setLogicPosition(bookmark);
}

bool UnitEditorLogicBase::isEditable(TreeObject* object, std::string& name, int& index) const
{
	return false;
}

void UnitEditorLogicBase::saveExpandState()
{
	if(expandStateFilename_ != "" && attribEditor().treeControl().GetVisibleCount() > 0) {
		XStream file(expandStateFilename_, XS_OUT);
		attribEditor().saveExpandState (file);
	}
}

void UnitEditorLogicBase::loadExpandState(const char* name, int index)
{
	expandStateFilename_.Format("Scripts\\TreeControlSetups\\UnitEditor_%s%i", name, index);
	XStream file(0);
    if(file.open(expandStateFilename_, XS_IN))
        attribEditor().loadExpandState(file);
}

void UnitEditorLogicBase::resetExpandState()
{
    expandStateFilename_ = "";
}

namespace{
bool isDigit(char c)
{
	static bool initialized = false;
	static bool table[256];
	if(!initialized){
		for(int i = 0; i < 256; ++i)
			table[i] = false;
        table['0'] = table['1'] = table['2'] = table['3'] = table['4']
          = table['5'] = table['6'] = table['7'] = table['8'] = table['9'] = true;
        initialized = true;
	}
	return table[c];
}
};

std::string UnitEditorLogicBase::makeName(const char* nameBase) const
{
	if(isNameValid(nameBase))
		return std::string(nameBase);
    std::string name_base = nameBase;
    
    // обрезаем циферки
	std::string::iterator ptr = name_base.end() - 1;
    if(name_base.size() > 1){
        while(ptr != name_base.begin() && isDigit(*ptr))
            ptr--;
        name_base = std::string(name_base.begin(), ptr + 1);
    }

    int index = 1;
    std::string default_name = name_base;
    while(!isNameValid(default_name.c_str())){
        CString str;
        str.Format ("%s%i", name_base.c_str(), index);
        default_name = static_cast<const char*>(str);
        ++index;
    }
    return default_name;
}

void UnitEditorLogicBase::expandOnly(ItemType item)
{
	ItemType i = tree_.GetChildItem(TLI_ROOT);
	while (i) {
		if (i != item) {
			tree_.Expand(i, TLE_COLLAPSE);
		} else {
			tree_.Expand (i, TLE_EXPAND);
			ItemType child_item = tree_.GetChildItem (i);
			while (child_item) {
				tree_.Expand (child_item, TLE_EXPAND);
				child_item = tree_.GetNextSiblingItem(child_item);
			}

		}
		i = tree_.GetNextSiblingItem (i);
	}
}

void CUETreeCtrl::OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDRAG* pNM = reinterpret_cast<NMTREELISTDRAG*>(pNMHDR);
	ItemType item = GetSelectedItem ();
	if(!logic_ || !logic_->enableDragAndDrop()){
		*pResult = 1;
        return;
	}

	if(logic_->allowDrag(item)) {
		*pResult = 0;
	} else {
		*pResult = 1;
	}
}

void CUETreeCtrl::OnDragEnter (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);
    *pResult = 0;
}

void CUETreeCtrl::OnDragLeave (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);
    *pResult = 0;
	SetTargetItem (0);
}

void CUETreeCtrl::OnDragOver (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);
	if(!logic_ || !logic_->enableDragAndDrop()) {
		*pResult = 1;
		return;
	}

	ItemType dest = pNM->pItem;
	ItemType src = GetSelectedItem();
	if(logic_->allowDropOn(dest, src)) {
		SetTargetItem(dest);
		*pResult = 0;
	} else {
		SetTargetItem(0);
		*pResult = 1;
	}
	return;
}

void CUETreeCtrl::invokeSearch(const TreeNode& node)
{
	CUnitEditorDlg* dlg = safe_cast<CUnitEditorDlg*>(GetParent());
	dlg->invokeSearch(node);
}

void CUETreeCtrl::OnDrop (NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREELISTDROP* pNM = reinterpret_cast<NMTREELISTDROP*>(pNMHDR);
	if(!logic_ || !logic_->enableDragAndDrop()) {
		*pResult = 1;
		return;
	}

	ItemType dest = pNM->pItem;
	ItemType src = GetSelectedItem();

	logic_->onDrop(dest, src);
}

void getItemPath(CTreeListCtrl& tree, ComboStrings& path, CTreeListItem* item)
{
	std::vector<CTreeListItem*> items;
	path.clear();
	while(item && item != TLI_ROOT){
		items.push_back(item);
		item = tree.GetParentItem(item);
	}
	if(!items.empty()){
		path.reserve(items.size());
		for(int i = items.size() - 1; i >= 0; --i){
			const char* text = tree.GetItemText(items[i]);
			path.push_back(text);
		}
	}
}


CTreeListItem* selectItemByPath(CTreeListCtrl& tree, const ComboStrings& itemPath)
{
	CTreeListItem* root = TLI_ROOT;
	CTreeListItem* item = root;
	if(itemPath.empty())
		return 0;

	ComboStrings::const_iterator it = itemPath.begin();
	while(it != itemPath.end()){
		if(root != TLI_ROOT)
			tree.Expand(root, TLE_EXPAND);
		item = tree.GetChildItem(root);
        if(!item)
			break;

		do{
			const char* text = tree.GetItemText(item, 0);
			if(*it == text){
				++it;
				root = item;
				if(it == itemPath.end())
					goto break_loop;
				else
					break;
			}
		}while(item = tree.GetNextItem(item, TLGN_NEXT));
		if(item == 0)
			break;
	}
break_loop:
	if(root == TLI_ROOT)
		return 0;
	else
		return root;
}

CTreeListItem* getItemByPath(CTreeListCtrl& tree, const ComboStrings& itemPath)
{
	CTreeListItem* root = TLI_ROOT;
	CTreeListItem* item = root;
	if(itemPath.empty())
		return 0;

	ComboStrings::const_iterator it = itemPath.begin();
	while(it != itemPath.end()){
		item = tree.GetChildItem(root);
        if(!item)
			break;

		do{
			const char* text = tree.GetItemText(item, 0);
			if(*it == text){
				++it;
				root = item;
				if(it == itemPath.end())
					goto break_loop;
				else
					break;
			}
		}while(item = tree.GetNextItem(item, TLGN_NEXT));
		if(item == 0)
			break;
	}
break_loop:
	if(root == TLI_ROOT)
		return 0;
	else
		return root;
}


BookmarkGeneric::BookmarkGeneric(int logicClassIndex, CUETreeCtrl& treeControl, CUETreeCtrl::ItemType item)
: logicClassIndex_(logicClassIndex)
{
	getItemPath(treeControl, treePath_, item);
	CAttribEditorCtrl::ItemType selected = treeControl.attribEditor().treeControl().GetSelectedItem();
	if(selected){
		getItemPath(treeControl.attribEditor().treeControl(), attribEditorPath_, selected);;
	}
}

BookmarkGeneric::~BookmarkGeneric()
{
}

int BookmarkGeneric::logicClassIndex() const
{
	return logicClassIndex_;
}


void BookmarkGeneric::visit(CUETreeCtrl& treeControl)
{
	if(CUETreeCtrl::ItemType item = getItemByPath(treeControl, treePath_))
		treeControl.focusItem(item);
	else
		; //xassert(0 && "Wrong item path");

	if(!attribEditorPath_.empty()){
		if(CAttribEditorCtrl::ItemType item = selectItemByPath(treeControl.attribEditor().treeControl(), attribEditorPath_))
			treeControl.attribEditor().treeControl().Select(item, CTreeListCtrl::SI_SELECT);
		else
			xassert(0 && "Wrong item path");
	}
}

void BookmarkGeneric::serialize(Archive& ar)
{
	ar.serialize(treePath_, "treePath", 0);
	ar.serialize(attribEditorPath_, "attribEditorPath", 0);
	ar.serialize(logicClassIndex_, "logicClassIndex", 0);
}
