#include "StdAfx.h"
#include "resource.h"

#include "..\Util\mfc\TreeListCtrl.h"
#include "..\Util\mfc\PopupMenu.h"

#include <functional>

#include "EditArchive.h"
#include "CheckComboBox.h"
#include ".\AttribEditorCtrl.h"
#include "CustomEdit.h"
#include "AttribComboBox.h"
#include "TreeInterface.h"

#include "AttribEditorDrawNotifyListener.h"
#include "Dictionary.h"

// _VISTA_ENGINE_EXTERNAL_ - нужно для перевода external-редактора

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


TreeNodeClipboard CAttribEditorCtrl::clipboard_;

IMPLEMENT_DYNCREATE(CAttribEditorCtrl, CWnd)

inline TreeNode* CAttribEditorCtrl::getNode(ItemType item) const
{
    if (item == 0)
        return 0;
	xassert("BUG: Дерево падает... Скажите admix-у!" && isDataAttached());
    return reinterpret_cast<TreeNode*>(item->GetData());
}

#pragma warning(disable: 4355)
CAttribEditorCtrl::CAttribEditorCtrl()
: drawNotifyListener_(new AttribEditorDrawNotifyListener(*this))
, style_(0)
, rootNode_(0)
, statusLabel_(0)
, tree_(*new CTreeListCtrl())
, popupMenu_(new PopupMenu(200))
, draggedItem_(0)
{
	if(false){
		TRANSLATE("Да");
		TRANSLATE("Нет");
	}

	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, className(), &wndclass) )
	{
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= className();

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}

	ctrlWnd_ = 0;
	editItem_ = 0;
}

CAttribEditorCtrl::~CAttribEditorCtrl()
{
	delete &tree_;
}


BEGIN_MESSAGE_MAP(CAttribEditorCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY(TLN_ITEMEXPANDING,   0, OnItemExpanding)
	ON_NOTIFY(TLN_BEGINLABELEDIT,  0, OnBeginLabelEdit)
	ON_NOTIFY(TLN_SUBITEMUPDATED,  0, OnSubItemUpdated)
	ON_NOTIFY(TLN_BEGINCONTROL,    0, OnBeginControl)
	ON_NOTIFY(TLN_ENDCONTROL,      0, OnEndControl)
	ON_NOTIFY(TLN_REQUESTCTRLTYPE, 0, OnRequestCtrlType)
	ON_NOTIFY(TLN_SELCHANGED,      0, OnSelChanged)
	ON_NOTIFY(NM_RCLICK,           0, OnRClick)
	ON_NOTIFY(TLN_MCLICK,          0, OnClick)
	ON_NOTIFY(NM_CLICK,            0, OnClick)
	ON_NOTIFY(TLN_KEYDOWN,         0, OnKeyDown)

    ON_NOTIFY(TLN_BEGINDRAG,       0, OnBeginDrag)
    ON_NOTIFY(TLN_DRAGENTER,       0, OnDragEnter)
    ON_NOTIFY(TLN_DRAGLEAVE,       0, OnDragLeave)
    ON_NOTIFY(TLN_DRAGOVER,        0, OnDragOver)
    ON_NOTIFY(TLN_DROP,            0, OnDrop)

	ON_WM_HSCROLL()
END_MESSAGE_MAP()


int CAttribEditorCtrl::initControl()
{
	CRect rcClientRect;
	GetClientRect (&rcClientRect);

    if (! defaultFont_.CreateFont (13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma"))
		return -1;

	if(!tree_.Create(WS_VISIBLE | WS_CHILD | WS_BORDER, rcClientRect, this, 0))
		return -1;

	if(!imageList_.Create(IDB_ATTRIB_EDITOR_CTRL, 16, 3, RGB (255, 0, 255)))
		return -1;
	
	tree_.SetImageList (&imageList_);
	tree_.SetFont (&defaultFont_);

	DWORD treeStyle = 
		TLC_TREELIST		                    // TreeList or List
		| TLC_DOUBLECOLOR	                    // double color background
		| TLC_MULTIPLESELECT                    // single or multiple select
		| TLC_SHOWSELACTIVE	                    // show active column of selected item
		| TLC_SHOWSELALWAYS	                    // show selected item always
		| TLC_HGRID			                    // show horizonal grid lines
		| TLC_VGRID			                    // show vertical grid lines
		| TLC_TGRID			                    // show tree horizonal grid lines ( when HGRID & VGRID )
		| TLC_HGRID_EXT		                    // show extention horizonal grid lines
		| TLC_VGRID_EXT		                    // show extention vertical grid lines
		| TLC_HGRID_FULL	                    // show full horizonal grid lines
		
		| TLC_IMAGE			                    // show image
		
		| TLC_HOTTRACK		                    // show hover text 
		| TLC_DRAG			                    // drag support
		| TLC_DROP			                    // drop support
		| TLC_DROPTHIS		                    // drop on this support
//		| TLC_HEADDRAGDROP	                    // head drag drop
		;
	
	if (!(style_ & COMPACT)) {
		treeStyle |= TLC_TREELINE;				// show tree line
		treeStyle |= TLC_ROOTLINE;   			// show root line
		treeStyle |= TLC_BUTTON;				// show expand/collapse button [+]
	}
	if (!(style_ & NO_HEADER)) {
		treeStyle |= TLC_HEADER;		        // show header
	}

	tree_.SetStyle (treeStyle);
	int totalWidth = (rcClientRect.Width () - GetSystemMetrics (SM_CXHSCROLL) - 2);

	tree_.InsertColumn(TRANSLATE("Параметр"), TLF_DEFAULT_LEFT, totalWidth / 2);
	tree_.InsertColumn(TRANSLATE("Значение"), TLF_DEFAULT_LEFT, totalWidth - totalWidth / 2);
	tree_.SetColumnModify (1, TLM_REQUEST);
	tree_.SetCustomDrawNotifyListener (drawNotifyListener_.get ());
	return 0;
}

int CAttribEditorCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return initControl ();
}

BOOL CAttribEditorCtrl::Create (DWORD style, const CRect& rect, CWnd* parent_wnd, UINT id)
{
	DWORD dwExStyle = 0;

    if(!CWnd::Create(className(), 0, style | WS_TABSTOP, rect, parent_wnd, id, 0))
		return FALSE;

	return TRUE;
}

void CAttribEditorCtrl::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();
}

void CAttribEditorCtrl::OnPaint()
{
	CPaintDC dc(this);
}

void CAttribEditorCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	tree_.MoveWindow (0, 0, cx, cy, 0);
	int fullWidth = cx - GetSystemMetrics (SM_CXHSCROLL) - 2;
	if (style_ & AUTO_SIZE) {
		tree_.SetColumnWidth (0, fullWidth / 2);
		tree_.SetColumnWidth (1, fullWidth - fullWidth / 2);
	}
}

void CAttribEditorCtrl::OnDestroy()
{
	detachData();
	defaultFont_.DeleteObject();
	CWnd::OnDestroy();
}

void CAttribEditorCtrl::detachData ()
{
	if (IsWindow (tree_.GetSafeHwnd ())){
		tree_.CancelModify ();
		tree_.DeleteAllItems ();
	}
	//rootNode_ = 0;

	DataHolders::iterator it;
	FOR_EACH(dataHolders_, it)
		it->release ();

	dataHolders_.clear ();
}

TreeNode* getNode(CAttribEditorCtrl::ItemType item){
	return item == 0 ? 0 : reinterpret_cast<TreeNode*>(item->GetData());
}

void CAttribEditorCtrl::setItemImage(TreeNode* node, ItemType new_item)
{
	int iconCollapsed = TreeEditor::ICON_ELEMENT;
	int iconExpanded = TreeEditor::ICON_ELEMENT;

	if(node->editor() && node->editor()->hideContent()){
	}
	else if(node->editType() == TreeNode::COMBO){
        //iconCollapsed = TreeEditor::ICON_DROPDOWN;
        //iconExpanded  = TreeEditor::ICON_DROPDOWN;
    }
	else if (node->editType() == TreeNode::COMBO_MULTI){
        //iconCollapsed = TreeEditor::ICON_CHECKBOX;
        //iconExpanded  = TreeEditor::ICON_CHECKBOX;
    }
	else if (node->editType() == TreeNode::POLYMORPHIC){
        iconCollapsed = TreeEditor::ICON_POINTER;
        iconExpanded  = TreeEditor::ICON_POINTER;
    }
	else if (node->editType() == TreeNode::VECTOR){
        iconCollapsed = TreeEditor::ICON_CONTAINER;
        iconExpanded  = TreeEditor::ICON_CONTAINER_EXPANDED;
    }
	else if(!node->empty ()) {
        iconCollapsed = TreeEditor::ICON_STRUCT;
        iconExpanded  = TreeEditor::ICON_STRUCT_EXPANDED;
	}
    new_item->SetImage(iconCollapsed, iconExpanded, iconCollapsed, iconExpanded);
}

static void assignNodeName(std::string& out, TreeNode* node)
{
	out = TRANSLATE(node->name());

	const TreeNode* parent = node->parent();
    if(parent && parent->editType() == TreeNode::VECTOR && out == "@"){
        XBuffer buf;
        int index = std::distance(parent->begin(), std::find(parent->begin(), parent->end(), node));
        buf <= index;
        out = "[";
        out += static_cast<const char*>(buf);
        out += "]";
	}
}

CAttribEditorCtrl::ItemType CAttribEditorCtrl::insertItem(const char* name, const char* type, ItemType parent, ItemType previous, Items* unusedItems)
{
	xassert(parent);
	if(unusedItems) {
		Items::iterator it;
		FOR_EACH(*unusedItems, it){
			ItemType item = *it;
			std::string node_name;
			TreeNode* node = getNode(item);
			if(node){
				assignNodeName(node_name, node);
				if(strcmp(node_name.c_str(), name) == 0 && getNode(item)->type() == type)
					return item;
			}else{
				CString str;
                str = tree_.GetItemText(item, 0);
				xassert(0);
			}
		}
		if(!previous)
			previous = unusedItems->empty() ? TLI_LAST : TLI_FIRST;
	}
	return tree_.InsertItem(name, parent, previous ? previous : TLI_LAST);
}

void fillEditors(TreeNode* node)
{
	if(!node->empty()){
		if(!node->editor()){
			node->setEditor(TreeEditorFactory::instance ().create(node->type(), true));
			if(node->editor())
				node->editor()->onChange(*node);
		}
		
		TreeNode::iterator it;
		FOR_EACH(*node, it){
			fillEditors(*it);
		}
	}
}


CAttribEditorCtrl::ItemType CAttribEditorCtrl::makeItemByNode(TreeNode* node, ItemType parentItem, ItemType previousItem, bool deepExpand, Items* unusedItems)
{
	xassert(node && parentItem);
	
	if(!node->editor())
		node->setEditor (TreeEditorFactory::instance ().create(node->type(), true));
	if(node->editor())
		node->editor()->onChange(*node);

    std::string nodeName;
    assignNodeName(nodeName, node);
    ItemType new_item = insertItem(nodeName.c_str(), node->type(), parentItem, previousItem, unusedItems);

	xassert(new_item);
	tree_.SetItemData(new_item, reinterpret_cast<DWORD>(node));

	bool expanded = false;

	if((parentItem == TLI_ROOT && !(style_ & HIDE_ROOT_NODE)) || ((style_ & EXPAND_ALL) && !node->editor ()) || deepExpand){
		tree_.Expand(new_item, TLE_EXPAND);
		tree_.SetItemState(new_item, TLIS_EXPANDED);
		expanded = true;
	}

	if(tree_.GetItemState(new_item, TLIS_EXPANDED))
		expanded = true;

	new_item->ShowTreeImage(true);
	setItemImage(node, new_item);
	bool isContainer = !(node->empty() || (node->editor() && node->editor()->hideContent()));
	if(isContainer){
		if(!expanded){
			ItemType empty_item = tree_.InsertItem("[ Empty ]", new_item);
			xassert(empty_item);
			tree_.SetItemData(empty_item, 0);

			fillEditors(node);
		}
		else
			fillItem(node, new_item, deepExpand);
	}
	updateItemText(new_item);
	return new_item;
}

void CAttribEditorCtrl::disconnectNodes(ItemType root)
{
	oldRootNode_ = rootNode_;
	rootNode_ = 0;
}

void CAttribEditorCtrl::onAttachData ()
{
	xassert(rootNode_);

	if(oldRootNode_ == 0)
		RemoveChilds(TLI_ROOT);
	if(!(style_ & HIDE_ROOT_NODE)) {
		fillItem(rootNode_, TLI_ROOT);
	} else {
		TreeNode& node = *rootNode_;
		TreeNode::iterator it;
		FOR_EACH (node, it) {
			fillItem(*it, TLI_ROOT);
		}
	}
}

void CAttribEditorCtrl::fillItem(TreeNode* node, ItemType parent, bool deepExpand)
{
	xassert(node && parent);
	if(!node)
		return;
	if(!node->empty()){
		if(!(node->editor() && node->editor()->hideContent())){
			Items unusedItems;
			if(ItemType item = tree_.GetChildItem(parent)){
				do{
					unusedItems.push_back(item);
				}while(item = tree_.GetNextItem(item, TLGN_NEXT));
			}

			TreeNode::iterator node_it;
			ItemType lastItem = 0;
			FOR_EACH(*node, node_it){
				TreeNode* current_node = *node_it;
				if(!current_node->hidden()){
					ItemType new_item = makeItemByNode(current_node, parent, lastItem, deepExpand, &unusedItems);
					xassert(new_item);
					Items::iterator it = std::find(unusedItems.begin(), unusedItems.end(), new_item);
					if(it != unusedItems.end())
						unusedItems.erase(it);

					lastItem = new_item;
				}
			}

			Items::iterator it;
			FOR_EACH(unusedItems, it)
				tree_.DeleteItem(*it);
		}
	}
}

void CAttribEditorCtrl::RemoveChilds (ItemType parent)
{
	if(parent == 0)
		return;

	ItemType cur_item = tree_.GetChildItem (parent);
	ItemType next_item = cur_item;
	while(next_item != 0){
		cur_item = next_item;
		next_item = tree_.GetNextSiblingItem (cur_item);
		tree_.DeleteItem (cur_item);
	}
}

void CAttribEditorCtrl::OnItemExpanding(NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELIST pNMTreeList = reinterpret_cast<LPNMTREELIST> (pNotifyStruct);
	ItemType parent = pNMTreeList->pItem;
	
	if (reinterpret_cast<long> (parent) == 0 ||
		reinterpret_cast<long> (parent) == 1)
	{
		// Запрещаем перемещать колонки
		*plResult = 1;
		return;
	}

	TreeNode* node = getNode (parent);
	
	// Если это не блок, удаляем дочерние ноды
	RemoveChilds (parent);
	
	// Элемент раскрывается
	if (! parent->GetState (TLIS_EXPANDED))	{
		bool deepExpand = (style_ & DEEP_EXPAND) && !node->editor ();
		if (GetAsyncKeyState (VK_SHIFT)) {
			deepExpand = true;
		} else if (GetAsyncKeyState (VK_CONTROL)) {
			deepExpand = false;
		}
		// Заполняем ноду содержимым контейнера
		fillItem(node, parent, deepExpand);
	}
	// Элемент закрывается
	else {
		// Вставляем пустышку
		ItemType empty_item = tree_.InsertItem ("[ Empty ]", parent);
		tree_.SetItemData (empty_item, 0);
	}
	
	*plResult = 0;
}

void CAttribEditorCtrl::OnBeginLabelEdit (NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELIST pNMTreeList = reinterpret_cast<LPNMTREELIST> (pNotifyStruct);

	if (pNMTreeList->iCol == 1)
	{
		// Первая колонка, запрещаем редактировать элементы
		*plResult = 0;
	}
	else
	{
		ItemType item = pNMTreeList->pItem;
		*plResult = 1;
	}
}



CWnd* CAttribEditorCtrl::makeCustomEditor(ItemType pItem)
{
	xassert(pItem);
    CRect rcItemRect;
	CWnd* pResult = 0;

	tree_.GetItemRect(pItem, 1, &rcItemRect, true);
	rcItemRect.right -= 1;
	rcItemRect.bottom -= 1;
	TreeNode* node = getNode (pItem);
	xassert(node);

	if(node->editor()){
		node->editor()->onChange (*node);
        if(pResult = node->editor()->beginControl(&tree_, rcItemRect))
			pResult->SetFont(&defaultFont_);
    }
    else{
        std::string value_string = node->value ();

        if(node->editType () == TreeNode::COMBO){
            CAttribComboBox* comboBox = new CAttribComboBox();
            CRect rcComboRect(rcItemRect);
            rcComboRect.bottom += 200;
            if(!comboBox->Create(WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST, rcComboRect, &tree_, 0)){
                delete comboBox;
                return 0;
            }
            comboBox->SetFont(&defaultFont_);

            ComboStrings::iterator it;
            ComboStrings strings;
			splitComboList(strings, node->comboList());
            int index = CB_ERR;
            int selected = 0;
            FOR_EACH (strings, it){
                ++index;
                const char* value = TRANSLATE((*it).c_str());
                const char* translatedNodeValue = TRANSLATE(node->value());
                comboBox->InsertString (-1, value);

                if(strcmp(value, translatedNodeValue) == 0)
                    selected = index;

                comboBox->SetItemData(index, index);
            }
            comboBox->SetCurSel(selected);
            pResult = comboBox;
        } 
        else if(node->editType () == TreeNode::COMBO_MULTI){
            CCheckComboBox* comboBox = new CCheckComboBox ();
            CRect rcComboRect (rcItemRect);
            rcComboRect.bottom += 200;
            if (! comboBox->Create (WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
                                  rcComboRect, &tree_, 0))
            {
                delete comboBox;
                return 0;
            }
            comboBox->SetFont(&defaultFont_);

            ComboStrings::iterator it;
            ComboStrings strings;
			splitComboList(strings, node->comboList());
			FOR_EACH(strings, it)
				*it = TRANSLATE(it->c_str());
            int index = CB_ERR;
            int selected = 0;
            FOR_EACH(strings, it){
                ++index;
                const char* value = it->c_str();
                const char* translatedNodeValue = TRANSLATE(node->value());
                comboBox->InsertString(-1, value);
                if(strcmp(value, translatedNodeValue) == 0)
                    selected = index;
                comboBox->SetItemData(index, index);
            }

            // TODO: Переместить в CheckComboBox (SetWindowText)!
			strings.clear();
			splitComboList(strings, node->value());
            comboBox->SelectAll(FALSE);
            FOR_EACH(strings, it){
                CString currentString(TRANSLATE(it->c_str()));
                currentString.Trim();
                int index = comboBox->FindStringExact(-1, currentString);
                if(index != CB_ERR){
                    comboBox->SetCheck(index, TRUE);
                }
            }
            // End of TODO
            pResult = comboBox;
        }
        else if(node->editType() == TreeNode::EDIT) {
            CCustomEdit* pCustomEdit = new CCustomEdit ();

            if (! pCustomEdit->Create (WS_VISIBLE     | WS_CHILD       |
                                       ES_LEFT        | ES_AUTOHSCROLL |
                                       ES_MULTILINE   | ES_WANTRETURN,
                                       rcItemRect, &tree_, 0))
            {
                delete pCustomEdit;
                return 0;
            }
            pCustomEdit->SetFont(&defaultFont_);
            pCustomEdit->SetWindowText(value_string.c_str ());
            pCustomEdit->SetSel(0, -1);
            pResult = pCustomEdit;
        }
        else {

        }
    }
	return pResult;
}


void CAttribEditorCtrl::OnBeginDrag(NMHDR* nm, LRESULT* cancel)
{
	NMTREELISTDROP* nmTreeListDrop = reinterpret_cast<NMTREELISTDROP*>(nm);
	
	ItemType item = tree_.GetSelectedItem();
	TreeNode* node = getNode(item);

	if(!item || !node || tree_.GetSelectedCount() > 1){
		*cancel = TRUE;
		return;
	}

	if(node->parent() && node->parent()->editType() == TreeNode::VECTOR){
		*cancel = FALSE;
		draggedItem_ = item;
	}
	else{
		*cancel = TRUE;
	}
}
void CAttribEditorCtrl::OnDragEnter(NMHDR* nm, LRESULT* result)
{

}

void CAttribEditorCtrl::OnDragLeave(NMHDR* nm, LRESULT* result)
{
}

void CAttribEditorCtrl::OnDragOver(NMHDR* nm, LRESULT* cancel)
{
	NMTREELISTDROP* nmTreeListDrop = reinterpret_cast<NMTREELISTDROP*>(nm);
	ItemType destItem = nmTreeListDrop->pItem;
	TreeNode* destNode = getNode(destItem);

	ItemType item = draggedItem_;
	TreeNode* node = getNode(draggedItem_);

	if(!destItem || !destNode || !node || !node->parent() || !item){
		*cancel = TRUE;
		tree_.SetTargetItem(0);
		return;
	}

	if(destNode->parent() && destNode->parent()->editType() == TreeNode::VECTOR &&
	   strcmp(destNode->parent()->type(), node->parent()->type()) == 0){
		*cancel = FALSE;
		tree_.SetTargetItem(destItem);
	}
	else if(strcmp(node->parent()->type(), destNode->type()) == 0){
		*cancel = FALSE;
		tree_.SetTargetItem(destItem);
	}
	else{
		*cancel = TRUE;
		tree_.SetTargetItem(0);
	}
}


void CAttribEditorCtrl::updateContainerItem(ItemType containerItem)
{
	if(!containerItem)
		return;
	RemoveChilds(containerItem);
	if(dataHolders_.empty()){
		fillItem(getNode(containerItem), containerItem);
		updateItemText(containerItem);
	}
	onElementChanged(containerItem);

}

void CAttribEditorCtrl::OnDrop(NMHDR* nm, LRESULT* result)
{
	NMTREELISTDROP* nmTreeListDrop = reinterpret_cast<NMTREELISTDROP*>(nm);
	
	CTreeListItem* destItem = nmTreeListDrop->pItem;
	TreeNode* destNode = getNode(destItem);

	ItemType item = draggedItem_;
	TreeNode* node = getNode(item);

	if(!destItem || !destNode || !item || !node)
		return;

    if(!destNode->parent())
		return;

	if(destNode == node)
		return;

	// кидаем на элемента списка - вставляем перед ним
	if(strcmp(destNode->parent()->type(), node->parent()->type()) == 0){
		if(TreeNode* container = const_cast<TreeNode*>(destNode->parent())){

			TreeNode* oldParent = const_cast<TreeNode*>(node->parent());
			ShareHandle<TreeNode> tempNode = node;
			
			oldParent->erase(std::find(oldParent->begin(), oldParent->end(), node));
			TreeNode::iterator it = std::find(container->begin(), container->end(), destNode);
			xassert(it != container->end());
			container->insert(it, node);
			int index = std::distance(container->begin(), std::find(container->begin(), container->end(), node));

			///
			ItemType containerItem = tree_.GetParentItem(destItem);
			if(tree_.GetParentItem(item) != containerItem)
				updateContainerItem(tree_.GetParentItem(item));
			updateContainerItem(containerItem);

			if(ItemType item = childByIndex(containerItem, index))
				tree_.Select(item, CTreeListCtrl::SI_SELECT);
		}
	}
	// кидаем на список - вставляем в конец
	else if(strcmp(destNode->type(), node->parent()->type()) == 0){
		if(TreeNode* container = const_cast<TreeNode*>(destNode)){
			TreeNode* oldParent = const_cast<TreeNode*>(node->parent());
			ShareHandle<TreeNode> tempNode = node;

			oldParent->erase(std::find(oldParent->begin(), oldParent->end(), node));
			container->push_back(node);
			int index = container->size() - 1;

			///
			ItemType containerItem = destItem;
			if(tree_.GetParentItem(item) != containerItem)
				updateContainerItem(tree_.GetParentItem(item));
			updateContainerItem(containerItem);

			if(ItemType item = childByIndex(containerItem, index))
				tree_.Select(item, CTreeListCtrl::SI_SELECT);
		}
	}
}


void CAttribEditorCtrl::OnBeginControl(NMHDR* pNotifyStruct, LRESULT* allow)
{
	LPNMTREELISTCTRL pNM = reinterpret_cast<LPNMTREELISTCTRL> (pNotifyStruct);

	if(pNM->iCol != 1){
		// Не вторая колонка, запрещаем редактировать элементы
		*allow = 0;
        return;		
	}

	editItem_ = pNM->pItem;
    TreeNode* node = getNode(editItem_);

	if(node){
		pNM->pEditControl = ctrlWnd_ = makeCustomEditor(editItem_);
		if(ctrlWnd_)
			*allow = 1;
		else
			*allow = 0;
	}
}

void CAttribEditorCtrl::OnEndControl (NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELISTCTRL pNM = reinterpret_cast<LPNMTREELISTCTRL>(pNotifyStruct);
	ItemType item = pNM->pItem;
	ShareHandle<TreeNode> node = 0;

    if(isDataAttached()) {
        if(item && (DWORD)(item) != 0xFFFFFFFF) {
            node = getNode (item);
            if(node->editor()) {
                node->editor()->endControl (*node, ctrlWnd_);
                node->editor()->controlEnded (*node);

                RemoveChilds(item);
                fillItem(node, item);
                onElementChanged(item);
                //FIXME: updateItemText (pItem);
                Invalidate(FALSE);
            }
        }
    }

	xassert(ctrlWnd_ == pNM->pEditControl);
	if(pNM->pEditControl){
		pNM->pEditControl->DestroyWindow();
		delete pNM->pEditControl;
		ctrlWnd_ = pNM->pEditControl = 0;
		editItem_ = 0;
	}
}

void CAttribEditorCtrl::OnSelChanged(NMHDR* notifyStruct, LRESULT* result)
{
	LPNMTREELIST notifyTreeList = reinterpret_cast<LPNMTREELIST>(notifyStruct);

	ItemType item = notifyTreeList->pItem;
	
	if(!item)
		return;
	if(reinterpret_cast<unsigned long>(item) < tree_.GetColumnCount()){
		return; 
	}

	bool cancel = false;

	if(tree_.GetItemState(item, TLIS_SELECTED)){
		if(tree_.GetSelectedCount() > 1){
			POSITION pos = tree_.GetFirstSelectedItemPosition();
			while(ItemType i = tree_.GetNextSelectedItem(pos)){
				if(i != item){
					if(getNode(i)->parent() != getNode(item)->parent()){
						cancel = true;
						break;
					}
				}
			}
		}

		if(cancel){
			tree_.SelectItem(item, 0, CTreeListCtrl::SI_DESELECT, 0);
		}
		else{
			tree_.EnsureVisible(item, notifyTreeList->iCol);
			//updateClickDetails(clickDetails_, item);

			onElementSelected();
		}
	}
}


void CAttribEditorCtrl::OnRequestCtrlType (NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELIST pNMTreeList = reinterpret_cast<LPNMTREELIST>(pNotifyStruct);

	if(pNMTreeList->iCol == 1){
		// Не вторая колонка, запрещаем редактировать элементы
		ItemType item = pNMTreeList->pItem;
		*plResult = TLM_CUSTOM;
	}
}

void CAttribEditorCtrl::onBeforeJumping(ItemType item)
{

}

static void assignNodeValue(std::string& out, const TreeNode* node)
{
    if(node->editor())
        out = node->editor()->nodeValue();
    else{
        if(node->editType() == TreeNode::COMBO){
            out = TRANSLATE(node->value());
        }
		else if(node->editType() == TreeNode::POLYMORPHIC){
            out = TRANSLATE(node->value());
		}
        else if(node->editType() == TreeNode::COMBO_MULTI){
            ComboStrings values;
            splitComboList(values, node->value());
            ComboStrings::iterator it;
            FOR_EACH(values, it){
                *it = TRANSLATE(it->c_str());
            }
            joinComboList(out, values);
        }
		else if(node->editType() == TreeNode::VECTOR){
			XBuffer buf;
			buf < "[ ";
			buf <= node->size();
			buf < " ]";
			out = buf;
		}
		else
            out = node->value();
    }
}

void CAttribEditorCtrl::onElementChanged(ItemType item)
{
	int column = 1;
	DataHolders::iterator it;
	FOR_EACH(dataHolders_, it){
		load(*it);
	}
	
	std::string value;
    assignNodeValue(value, getNode(item));
	item->SetText(value.c_str(), column);
	if(ItemType parentItem = tree_.GetParentItem(item))
		updateItemText(parentItem);
	setStatusText("");
	beforeResave();
	if(dataHolders_.size() == 1)
		FOR_EACH(dataHolders_, it) {
			save(*it);
		}
}

static bool untranslate(std::string& out, const char* translatedValue, const ComboStrings& untranslatedCombo)
{
	ComboStrings::const_iterator it;
	FOR_EACH(untranslatedCombo, it){
		if(strcmp(TRANSLATE(it->c_str()), translatedValue) == 0){
			out = it->c_str();
			return true;
		}
	}
	out = translatedValue;
	return false;
}

void CAttribEditorCtrl::OnSubItemUpdated (NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELIST pNM = reinterpret_cast<LPNMTREELIST> (pNotifyStruct);

	xassert(pNM->pItem);
	TreeNode* node = getNode(pNM->pItem);
	xassert(node);
	
	const char* translatedValue = pNM->pItem->GetText(1);

	ComboStrings combo;
	splitComboList(combo, node->comboList(), '|');
	if(node->editType() == TreeNode::COMBO){
		std::string value;
		if(untranslate(value, translatedValue, combo))
			node->setValue(value.c_str());
		else
			xassert(0);
	}
	else if(node->editType() == TreeNode::COMBO_MULTI){
		ComboStrings values;
		splitComboList(values, translatedValue, '|');
		ComboStrings::iterator it;
		FOR_EACH(values, it){
			std::string str = *it;
			if(!untranslate(str, it->c_str(), combo))
				xassert(0);
			else
				*it = str;
		}
		std::string value;
		joinComboList(value, values);
		node->setValue(value.c_str());
	}
	else
		node->setValue(translatedValue);

	node->setUnedited(false);
	onElementChanged (pNM->pItem);
    *plResult = 0;
}

bool CAttribEditorCtrl::isEditableContainerElement(ItemType item) const
{
	xassert(item);
	if (item == TLI_ROOT)
		return false;

    if(!getNode(item))
        return false;

    if(!item->m_pParent)
        return false;

	return isEditableContainer(item->m_pParent);
}

bool CAttribEditorCtrl::isEditableContainer(ItemType pItem) const
{
	if(!pItem)
		return false;
	if (pItem == TLI_ROOT)
		return false;

    TreeNode* node = getNode(pItem);
    if(!node)
        return false;

    if(node->editType() == TreeNode::VECTOR)
        return true;

    return false;
}

bool CAttribEditorCtrl::isPointer(ItemType pItem) const
{
	xassert(pItem);
	if(pItem == TLI_ROOT)
		return false;

    TreeNode* node = getNode (pItem);
    if (! node)
        return false;

    if (node->editType () == TreeNode::POLYMORPHIC)
        return true;
    return false;
}


void CAttribEditorCtrl::spawnRootMenu()
{
	if(style_ & DISABLE_MENU)
		return;

	tree_.SelectItem(0);

	popupMenu_->clear();
    PopupMenuItem& root = popupMenu_->root();

	popupMenu_->root().add(TRANSLATE("Копировать\tCtrl+C")).connect(bindMethod(*this, &CAttribEditorCtrl::onMenuCopy));
	popupMenu_->root().add(TRANSLATE("Вставить\tCtrl+V")).connect(bindMethod(*this, &CAttribEditorCtrl::onMenuPaste));

	CPoint point;
	GetCursorPos(&point);
	popupMenu_->spawn(point, GetSafeHwnd());
}



void CAttribEditorCtrl::spawnContextMenu()
{
	if(style_ & DISABLE_MENU)
		return;

 	CPoint point;
 	GetCursorPos(&point);

	Items selectedItems;
	POSITION pos = tree_.GetFirstSelectedItemPosition();
	while(ItemType item = tree_.GetNextSelectedItem(pos))
		selectedItems.push_back(item);
	
	if(selectedItems.empty())
		return;

	TreeNode* node = getNode(selectedItems.front());

	bool isEditable = (node->editType() != TreeNode::STATIC && node->editType() != TreeNode::VECTOR && node->editType() != TreeNode::POLYMORPHIC) || node->editor();
	bool isPointer = true;
	bool isContainer = true;
	bool isElement = true;
	bool isReference = (node->editor() && node->editor()->hasLibrary());
	bool isSingle = selectedItems.size() == 1;
	Items::iterator it;
	FOR_EACH(selectedItems, it){
		if(!this->isPointer(*it))
			isPointer = false;
		if(!this->isEditableContainer(*it))
			isContainer = false;
		if(!this->isEditableContainerElement(*it))
			isElement = false;
	}
	TreeEditor* editor = isSingle ? node->editor() : 0;

	popupMenu_->clear();
    PopupMenuItem& root = popupMenu_->root();

	if(isPointer && isSingle){
		PopupMenuItem& createItem = root.add(TRANSLATE("Создать"));
		ComboStrings strings;
		node->generateComboList();

		splitComboList(strings, node->comboList());
		xassertStr(!strings.empty(), node->comboList());
		ComboStrings::iterator it;
		int index = 0;
		FOR_EACH(strings, it){
			ComboStrings path;
			splitComboList(path, TRANSLATE(it->c_str()), '\\');
			int path_position = 0;


			PopupMenuItem* item = &createItem;
			for(int path_position = 0; path_position < path.size(); ++path_position){
				const char* leaf = path[path_position].c_str();
				if(path_position + 1 == path.size()){
					item->add(leaf).connect(bindArgument(bindMethod(*this, &CAttribEditorCtrl::onMenuPointerCreate),  index++));
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

	if(isPointer || (editor && editor->canBeCleared()))
		root.add(TRANSLATE("Очистить содержимое\tShift+Del"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuPointerDelete));

	if(isEditable && isSingle)
		root.add(TRANSLATE("Редактировать\tSpace")).connect(bindMethod(*this, &CAttribEditorCtrl::onMenuEdit));
	if(isContainer && isSingle){
        if(!root.empty())
            root.addSeparator();

		root.add(TRANSLATE("Добавить элемент в конец\tShift+Insert"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuAppendElement));
	}

	if(isElement && isSingle){
        if(!root.empty())
            root.addSeparator();
		root.add(TRANSLATE("Вставить элемент\tInsert"))
			.connect(bindArgument(bindMethod(*this, &CAttribEditorCtrl::onMenuInsertElement), selectedItems.front()));

		root.add(TRANSLATE("Передвинуть выше\tCtrl+Up"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuMoveUp));
		root.add(TRANSLATE("Передвинуть ниже\tCtrl+Down"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuMoveDown));
	}

	if(isSingle)
		onMenuConstruction(selectedItems.front(), root);

	{
        if(!root.empty())
            root.addSeparator();
		popupMenu_->root().add(TRANSLATE("Копировать\tCtrl+C"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuCopy));
		popupMenu_->root().add(TRANSLATE("Вставить\tCtrl+V"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuPaste));
	}

	if(isElement){
		root.addSeparator();
		root.add(isSingle ?  TRANSLATE("Удалить элемент\tDel") : TRANSLATE("Удалить элементы\tDel"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuRemoveSelectedElements));
	}

	if(isContainer && isSingle){
		root.addSeparator();
		root.add(TRANSLATE("Очистить список"))
			.connect(bindMethod(*this, &CAttribEditorCtrl::onMenuClearContainer));
	}

	popupMenu_->spawn(point, GetSafeHwnd());
}

void CAttribEditorCtrl::OnRClick (NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELIST pNM = reinterpret_cast<LPNMTREELIST> (pNotifyStruct);

	ItemType item = pNM->pItem;
	if(item != 0){
		//updateClickDetails(clickDetails_, item);
		spawnContextMenu();
	}
	else{
		spawnRootMenu();
	}
	*plResult = 1;
}


void CAttribEditorCtrl::updateItemText(ItemType item)
{
	if(TreeNode* node = getNode(item)){
		std::string label;
		assignNodeName(label, node);
		if(label[0] == '&')
			label.erase(label.begin());
		if(node->editor())
			node->editor()->onChange(*node);
		std::string value;
		assignNodeValue(value, node);

		if(!node->empty() && !(style_ & NO_DIGEST)){
			bool first_one = true;

		    TreeNode::iterator it;
			FOR_EACH(*node, it){
				const char* sub_name = (**it).name();
				
				if(sub_name[0] == '&'){
					if (first_one) {
						label += ": {";
						first_one = false;
					}
					else {
						label += ", ";
					}
					if ((*it)->editor ()) {
						(*it)->editor ()->onChange (**it);
						label += (*it)->editor ()->nodeValue ();
					} else {
						label += (*it)->value ();
					}
				}
			}

			if (first_one == false)
				label += "}";
		}
		item->SetText(label.c_str(), COLUMN_NAME);
		item->SetText(value.c_str(), COLUMN_VALUE);
	}
}

void CAttribEditorCtrl::rebuildClickedItem()
{
	ItemType item = tree_.GetSelectedItem();
    RemoveChilds(item);
	TreeNode* node = getNode(item);
	if(node)
		fillItem(node, item);

	updateItemText(item);
}

void CAttribEditorCtrl::InsertIntoContainer (ItemType containerItem, TreeNode* beforeNode, TreeNode* insertWhat)
{
    TreeNode& container = *getNode(containerItem);
    container.insert(std::find(container.begin(), container.end(), beforeNode), insertWhat);

	RemoveChilds(containerItem);
	fillItem(&container, containerItem);
    updateItemText(containerItem);
}

CAttribEditorCtrl::ItemType CAttribEditorCtrl::childByIndex(ItemType root, int index)
{
	if(ItemType item = tree_.GetChildItem(root)){
		int i = 0;
		do{
			if(i++ == index)
				return item;
		}while(item = tree_.GetNextItem(item, TLGN_NEXT));
	}
	return 0;
}

void CAttribEditorCtrl::swapContainerItems(ItemType containerItem, TreeNode* firstNode, TreeNode* secondNode)
{
	TreeNode& container = *getNode(containerItem);

	std::swap(*firstNode, *secondNode);
	int index = std::distance(container.begin(), std::find(container.begin(), container.end(), secondNode));
    	
	RemoveChilds(containerItem);
	fillItem(&container, containerItem);
	if(ItemType item = tree_.GetChildItem(containerItem)){
		do {
			updateItemText(tree_.GetParentItem(containerItem));
		} while(item = tree_.GetNextItem(item, TLGN_NEXT));
	}
	
	RemoveChilds(containerItem);
	if(dataHolders_.empty()){
		fillItem(&container, containerItem);
	    updateItemText(containerItem);
	}
	onElementChanged(containerItem);

	if(ItemType item = childByIndex(containerItem, index))
		tree_.Select(item, CTreeListCtrl::SI_SELECT);
}

void CAttribEditorCtrl::removeFromContainer(ItemType containterItem, TreeNode* node)
{
    TreeNode& container = *getNode(containterItem);
	TreeNode::iterator it = std::find (container.begin (), container.end (), node);
	
	xassert(it != container.end ());

	container.erase(it);

	RemoveChilds(containterItem);
	fillItem(&container, containterItem);
    updateItemText(containterItem);
}

void CAttribEditorCtrl::AppendContainer (ItemType pItem, TreeNode* appendWhat)
{
	TreeNode* container = getNode (pItem);

	container->push_back (appendWhat);

	RemoveChilds (pItem);
	fillItem (container, pItem);
    updateItemText (pItem);
}

void CAttribEditorCtrl::ClearContainer (ItemType pItem)
{
    TreeNode& container = *getNode (pItem);

    container.clear ();
    RemoveChilds (pItem);
    fillItem (&container, pItem);
    updateItemText (pItem);
}

void CAttribEditorCtrl::onMenuInsertElement(ItemType beforeItem)
{
	ItemType item = beforeItem;
	if(isEditableContainerElement(item)){
		ItemType containerItem = tree_.GetParentItem(item);


		TreeNode* node = getNode(item);
		TreeNode& container = *getNode(containerItem);

		TreeNode::iterator it = std::find(container.begin(), container.end(), node);
		xassert(it != container.end());
		int index = max(0, min(container.size() - 1, std::distance(container.begin(), it)));
		///
		const TreeNode* defaultTreeNode = attribEditorInterface().treeNodeStorage().defaultTreeNode(container.value());
		xassert(defaultTreeNode);
		if(defaultTreeNode){
			ShareHandle<TreeNode> new_node = new TreeNode("");
			*new_node = *defaultTreeNode;

			InsertIntoContainer(containerItem, node, new_node);
			onElementChanged(containerItem);

			TreeNode& container = *getNode(containerItem);
			ItemType focusItem = containerItem;
			if(index >= 0){
				it = container.begin();
				std::advance(it, index);
				focusItem = findItemByNode(*it, containerItem);
				xassert(item);
			}
			tree_.SelectItem(focusItem, 0);
		}
	}
}

void CAttribEditorCtrl::onMenuRemoveSelectedElements()
{
	Items selectedItems;
	POSITION pos = tree_.GetFirstSelectedItemPosition();
	while(ItemType item = tree_.GetNextSelectedItem(pos))
		selectedItems.push_back(item);


	typedef std::vector<TreeNode*> Nodes;
	Nodes selectedNodes;

	ItemType containerItem = 0;
	Items::iterator i;
	FOR_EACH(selectedItems, i){
		if(*i)
			containerItem = tree_.GetParentItem(*i);
		if(*i && containerItem && isEditableContainer(containerItem) && isEditableContainerElement(*i))
			selectedNodes.push_back(getNode(*i));
	}
	TreeNode* container = getNode(containerItem);

	if(containerItem && !selectedItems.empty()){
		Nodes::iterator it;
		int maxIndex = 0;
		FOR_EACH(selectedNodes, it){
			TreeNode* node = *it;

			TreeNode::iterator it = std::find(container->begin(), container->end(), node);
			xassert(it != container->end());
			int index = min(int(container->size()) - 2, int(std::distance(container->begin(), it)));

			TreeNode::iterator nit = std::find(container->begin(), container->end(), node);
			xassert(nit != container->end ());
			container->erase(nit);

			ItemType item = containerItem;
			maxIndex = max(index, maxIndex - 1);
		}

		
		RemoveChilds(containerItem);
		fillItem(container, containerItem);
		updateItemText(containerItem);
		onElementChanged(containerItem);

		container = getNode(containerItem);
		if(maxIndex >= 0){
			TreeNode::iterator it = container->begin();
			std::advance(it, maxIndex);
			ItemType focusItem = findItemByNode(*it, containerItem);
			xassert(focusItem);
			tree_.SelectItem(focusItem, 0);
		}
	}
}


CAttribEditorCtrl::ItemType CAttribEditorCtrl::findItemByNode(const TreeNode* node, ItemType root)
{
	if(getNode(root) == node) 
		return root;

	if(ItemType item = tree_.GetChildItem(root)){
		do{
			if(ItemType temp = findItemByNode(node, item)){
				return temp;
			}
		}while(item = tree_.GetNextItem(item, TLGN_NEXT));
	}
	return 0;
}



void CAttribEditorCtrl::onMenuMoveUp()
{
	ItemType item = tree_.GetSelectedItem();
	ItemType containerItem = tree_.GetParentItem(item);
	TreeNode* container = getNode(containerItem);
	TreeNode* node = getNode(item);
	if(container) {
		TreeNode* firstNode = node;
		TreeNode* secondNode = firstNode;
		
		TreeNode::iterator it = std::find(container->begin(), container->end(), firstNode);
		if(it != container->begin() && it != container->end()) {
			--it;
			secondNode = *it;
		}
		swapContainerItems(containerItem, firstNode, secondNode);
	}
}

void CAttribEditorCtrl::onMenuMoveDown()
{
	ItemType item = tree_.GetSelectedItem();
	ItemType containerItem = tree_.GetParentItem(item);
	TreeNode* container = getNode(containerItem);
	TreeNode* node = getNode(item);
	if(container) {
		TreeNode* firstNode = node;
		TreeNode* secondNode = firstNode;
		
		TreeNode::iterator it = std::find(container->begin(), container->end(), firstNode);
		if(std::distance(it, container->end()) != 1 && it != container->end()) {
			++it;
			secondNode = *it;
		}
		swapContainerItems(containerItem, firstNode, secondNode);
	}
}

void CAttribEditorCtrl::onMenuAppendElement()
{
	ItemType item = tree_.GetSelectedItem();
	TreeNode* node = getNode(item);

	if(isEditableContainer(item)){
		ShareHandle<TreeNode> new_node(new TreeNode (""));
		const TreeNode* defaultTreeNode = attribEditorInterface().treeNodeStorage().defaultTreeNode(node->value());
		if(defaultTreeNode){
			*new_node = *defaultTreeNode;
			AppendContainer(item, new_node);
			onElementChanged(item);
			///	
			tree_.Expand(item, TLE_EXPAND);
			if(item = tree_.GetChildItem(item)){
				ItemType lastItem = item;
				while(item = tree_.GetNextItem(item, TVGN_NEXT)){
					lastItem = item;
				}
				tree_.SelectItem(lastItem, 0);
			}
		}
		else{
			xassert(0);
		}
	}
}

void CAttribEditorCtrl::onMenuClearContainer()
{
	ItemType item = tree_.GetSelectedItem();
	TreeNode* node = getNode(item);

	xassert(isEditableContainer(item));

	ClearContainer(item);
	onElementChanged(item);
}

void CAttribEditorCtrl::onMenuPointerDelete()
{
	ItemType item = tree_.GetSelectedItem();
	TreeNode* node = getNode(item);

	if(node->editType() == TreeNode::POLYMORPHIC){
		node->clear();
		node->setValue("");
	}
	else{
		if(TreeEditor* editor = node->editor()){
			xassert(editor->canBeCleared());
			editor->onClear(*node);
		}
	}
    rebuildClickedItem();
    onElementChanged(item);
}

void CAttribEditorCtrl::onMenuEdit()
{
	ItemType item = tree_.GetSelectedItem();
	TreeNode* node = getNode(item);

	if(item){
		OnClick(item, 1, false);
	}
}

void CAttribEditorCtrl::onMenuPointerCreate(int type_index)
{
	ItemType item = tree_.GetSelectedItem();
	TreeNode* node = getNode(item);

	TreeNode& dest = *node;
	dest.clear();

	TreeNode src(*node->defaultTreeNode(type_index));

	TreeNode::const_iterator tit;
	FOR_EACH(src, tit){
		TreeNode* new_node = new TreeNode("");
		*new_node = **tit;
		dest.push_back(new_node);
	}
	dest.setValue(src.value());
	dest.setType(src.type());
	rebuildClickedItem();
	onElementChanged(item);
}

void CAttribEditorCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

bool CAttribEditorCtrl::OnClick(ItemType item, int column, bool middleButton)
{
	TreeNode* node = getNode(item);

	GetAsyncKeyState(VK_SHIFT);
    bool shift = (GetAsyncKeyState(VK_SHIFT) >> 15);

	if(node && column == 1){
		tree_.SelectItem(item, column);

		if(!beforeElementEdit(item, middleButton)){

		}
		else{
			bool beginModify = true;
			if(TreeEditor* editor = node->editor()){
				TreeNode nodeCopy(*node);
				if(editor->invokeEditor(nodeCopy, GetSafeHwnd())){
					RemoveChilds(item);
					*node = nodeCopy;
					TreeEditor* newEditor = TreeEditorFactory::instance().create(node->type(), true);
					node->setEditor(newEditor);
					newEditor->onChange(*node);

					fillItem(node, item);
					onElementChanged(item);
					updateItemText(item);
					beginModify = false;
					Invalidate(FALSE);
				}
			}
			if(beginModify)
				tree_.BeginModify (item, 1);
		}
		
		return true;
	}
	return false;
}

void CAttribEditorCtrl::OnClick(NMHDR* pNotifyStruct, LRESULT* plResult)
{
	LPNMTREELIST pNM = reinterpret_cast<LPNMTREELIST> (pNotifyStruct);
    CTreeListItem* pItem = pNM->pItem;
	if(OnClick(pItem, pNM->iCol, pNM->iMouseButton == MK_MBUTTON))
		*plResult = 1;
	else
		*plResult = 0;
}

void CAttribEditorCtrl::OnKeyDown (NMHDR* pNotifyStruct, LRESULT* plResult)
{
	NMTREELISTKEYDOWN* pNM = reinterpret_cast<NMTREELISTKEYDOWN*>(pNotifyStruct);
	
    CTreeListItem* item = tree_.GetSelectedItem ();
	int column = tree_.GetSelectColumn ();
    *plResult = 0;

	if (!item)
		return;

    bool shift = bool(GetAsyncKeyState(VK_SHIFT) >> 15);
    bool control = bool(GetAsyncKeyState(VK_CONTROL) >> 15);
    UINT key = pNM->wVKey;

    if(key == VK_SPACE || key == VK_RETURN) {
        OnClick(item, 1, false);
    } 
    if(shift && !control && key == VK_DELETE) {
        TreeNode* node = getNode(item);
        if(node){
            if(node->editType() == TreeNode::POLYMORPHIC){
                onMenuPointerDelete();
            }
            else if(node->editor()){
                node->editor()->onClear(*node);
            }
            updateItemText(item);
            onElementChanged(item);
            Invalidate(FALSE);
        }
    }

	if(control) {
		if(key == 'C' || key == VK_INSERT)
			onMenuCopy();
		if(key == 'V')
			onMenuPaste();
	}
	
	if(shift && !control){
		if(key == VK_INSERT)
			onMenuAppendElement();
	}

    if(!shift && !control){
		if(key == VK_INSERT)
			onMenuInsertElement(tree_.GetSelectedItem());
		if(key == VK_DELETE)
			onMenuRemoveSelectedElements();
    }


	if(control && !shift){
		if(key == VK_UP){
			onMenuMoveUp();
			*plResult = TRUE;
		}
		if(key == VK_DOWN){
			onMenuMoveDown();
			*plResult = TRUE;
		}
	}
}

void CAttribEditorCtrl::copyToClipboard(const TreeNode* sourceNode)
{
	/*
	xassert(::IsWindow(GetSafeHwnd()));
	TreeNode node("");
	TreeNode* child = new TreeNode("");
	*child = *sourceNode;
	node.push_back(new TreeNode(*sourceNode));
	*/
	clipboard().set(sourceNode, GetSafeHwnd());
}

TreeNodeClipboard& CAttribEditorCtrl::clipboard()
{
	return TreeNodeClipboard::instance();
}


void CAttribEditorCtrl::onMenuCopy()
{
	TreeNode clip;

	if(tree_.GetSelectedCount() == 0){
		TreeNode* root = (style_ & HIDE_ROOT_NODE) ? rootNode_->front() : rootNode_;
		copyToClipboard(root);
        return;
	}
	if(tree_.GetSelectedCount() == 1){
		clip.push_back(new TreeNode(*getNode(tree_.GetSelectedItem())));
		setStatusText(TRANSLATE("Элемент скопирован..."));
	}
	else{
		POSITION pos = tree_.GetFirstSelectedItemPosition();
		int count = 0;
		while(ItemType item = tree_.GetNextSelectedItem(pos)){
			TreeNode* sourceNode = getNode(item);
			xassert(sourceNode);
			clip.push_back(new TreeNode(*sourceNode));
			if(sourceNode->parent())
				clip.setType(sourceNode->parent()->type());
			++count;
		}
		CString str;
		str.Format(TRANSLATE("%i элемент(а/ов) скопировано"), count);
		setStatusText(str);
	}
	if(!clip.empty())
		copyToClipboard(&clip);
}

bool CAttribEditorCtrl::CanBePastedOn(const TreeNode& dest, const TreeNode& src) const
{
    return strcmp(dest.type(), src.type()) == 0;
}

bool CAttribEditorCtrl::CanBePastedInto (const TreeNode& dest, const TreeNode& src) const
{
	if(dest.editType () != TreeNode::VECTOR)
		return false;

	if(dest.defaultTreeNode() && strcmp(dest.defaultTreeNode()->type(), src.type()) == 0)
		return true;
	return false;
}


void CAttribEditorCtrl::PasteSingleNode (TreeNode& dest, const TreeNode& source)
{
	const char* oldName = dest.name();
	TreeNode* oldParent = dest.parent();
	dest = source;
	dest.setName(oldName);
	dest.setParent(oldParent);
	dest.setEditor(TreeEditorFactory::instance().create(dest.type(), true));
	if(dest.editor())
		dest.editor()->onChange(dest);
}

bool CAttribEditorCtrl::CanBePastedOnSingleTypeSelection (const TreeNode& source)
{
	if (tree_.GetSelectedCount ()) {
		POSITION pos = tree_.GetFirstSelectedItemPosition();
		while(ItemType pItem = tree_.GetNextSelectedItem(pos)){
			if (!CanBePastedOn (*getNode(pItem), source))
				return false;
		}
		return true;
	}
	else {
		return false;
	}
}

void CAttribEditorCtrl::onMenuPaste()
{
	xassert(::IsWindow(GetSafeHwnd()));
	ShareHandle<TreeNode> clipboard = new TreeNode("");
	*clipboard = *this->clipboard().get(GetSafeHwnd(), strings_);
	if(!clipboard)
		return;
    if(clipboard->empty())
        return;

	TreeNode* singleNode = 0;
	ItemType singleItem = 0;
	if(tree_.GetSelectedCount() == 1){
		singleItem = tree_.GetSelectedItem();
		singleNode = getNode(singleItem);
	}
	else if(tree_.GetSelectedCount() == 0){
		singleItem = (style_ & HIDE_ROOT_NODE) ? TLI_ROOT : tree_.GetChildItem(TLI_ROOT);
		singleNode = (style_ & HIDE_ROOT_NODE) ? rootNode_->front() : rootNode_;
	}

	if(CanBePastedOnSingleTypeSelection(*clipboard->front())){
		POSITION pos = tree_.GetFirstSelectedItemPosition();
		while(ItemType pItem = tree_.GetNextSelectedItem(pos)) {
			PasteSingleNode(*getNode(pItem), *clipboard->front());
			RemoveChilds(pItem);
			fillItem(getNode(pItem), pItem);
			updateItemText(pItem);
			onElementChanged(pItem);
		}
		tree_.Invalidate (FALSE);
	}
	else if(singleNode && CanBePastedInto(*singleNode, *clipboard->front())){
		ShareHandle<TreeNode> newNode(new TreeNode (""));
		PasteSingleNode(*newNode, *clipboard->front ());
		newNode->setName("[+]");
		AppendContainer(singleItem, newNode);
		onElementChanged(singleItem);
	}
	else{
		tree_.AllowRedraw(FALSE);
		TreeNode::iterator it;

		int counter = 0;
		FOR_EACH(*clipboard, it){
			const TreeNode* source = *it;
			if(!source->parent())
				return;
			POSITION pos = tree_.GetFirstSelectedItemPosition();
			while(ItemType pItem = tree_.GetNextSelectedItem(pos)){
				TreeNode* node = getNode(pItem); 
				TreeNode* dest = node->findChild(source->name(), source->type(), source->parent()->type());
				if(dest){
					PasteSingleNode(*dest, *source);
					counter++;
					RemoveChilds(pItem);
					fillItem(getNode(pItem), pItem);
					updateItemText(pItem);
				}
				onElementChanged (pItem);
			}
		}

		if(counter){
			tree_.Invalidate (FALSE);
			CString str;
			str.Format("Smart-paste: Найдено и вставлено %i элемент(а/ов)", counter);
			setStatusText(str);
		}
		tree_.AllowRedraw();
	}
}

void CAttribEditorCtrl::showMix()
{
#ifndef _USRDLL
	DataHolders::iterator it;
	ShareHandle<TreeNode> resultRoot(new TreeNode(""));

	FOR_EACH (dataHolders_, it) {
		EditOArchive ar;
		it->serialize (ar);

		if (it == dataHolders_.begin ()) {
			*resultRoot = *ar.rootNode ();
		} else {
			resultRoot->front()->intersect (ar.rootNode ()->front ());
		}
	}
	setRootNode (resultRoot);
#else
	xassert (0 && "showMix(): Not implemented for DLL version!");
#endif
}

LRESULT CAttribEditorCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_USER + 1) {
		int a = 3;
	}

	return CWnd::WindowProc(message, wParam, lParam);
}

void CAttribEditorCtrl::saveExpandState(XStream& file)
{
	XBuffer buf(64, 1);
	saveExpandState(buf);
	file.write(buf.buffer(), buf.size());
}

void CAttribEditorCtrl::loadExpandState(XStream& file)
{
	long len = file.size() - file.tell();
	XBuffer buf(len, 0); 
	file.read(buf.buffer(), len);
	loadExpandState(buf);
}

void CAttribEditorCtrl::saveExpandState(XBuffer& buffer)
{
	const int visibleCount = tree_.GetVisibleCount ();
	buffer < visibleCount;
	if(!visibleCount)
		return;

	ItemType pItem = tree_.GetFirstVisibleItem ();
	if (!pItem)
		return;
	do{
		const char expandState = static_cast<unsigned char>(pItem->GetState(TLIS_EXPANDED));
		buffer < expandState;
	} while(pItem = tree_.GetNextVisibleItem (pItem));
}

void CAttribEditorCtrl::loadExpandState(XBuffer& buffer)
{
	int visibleCount = 0;
	
	if(buffer.tell() + sizeof(visibleCount) >= buffer.size())
		return;
	buffer > visibleCount;

	if (!visibleCount)
		return;

	if(ItemType pItem = tree_.GetFirstVisibleItem()){
		int i = 0;
		do{
			if (i++ >= visibleCount)
				return;

			char expanded = '\0';

			if(buffer.tell() + sizeof(expanded) >= buffer.size())
				return;
			buffer > expanded;

			if(expanded)
				tree_.Expand(pItem, TLE_EXPAND);
			else 
				tree_.Expand(pItem, TLE_COLLAPSE);

		}while(pItem = tree_.GetNextVisibleItem(pItem));
	}
}

void CAttribEditorCtrl::expandNodes(int levels, ItemType parent)
{
    if(levels == 0)
        return;

    ItemType item = tree_.GetChildItem (parent);
    while(item != 0){
        expandNodes(levels - 1, item);
		tree_.Expand(item, TLE_EXPAND);
        item = tree_.GetNextSiblingItem(item);
    }
}

void CAttribEditorCtrl::setStatusText(const char* text)
{
	if (statusLabel_) {
		statusLabel_->SetWindowText(text);
	}
}

void CAttribEditorCtrl::attachSerializeable(Serializeable& holder)
{
	detachData ();

	dataHolders_.clear ();
	dataHolders_.push_back(holder);
    save(dataHolders_.back());
}

void CAttribEditorCtrl::resave()
{
    if(dataHolders_.size() == 1)
		save(dataHolders_.front());
	else
		showMix();
}

void CAttribEditorCtrl::load(Serializeable& holder)
{
	EditIArchive iarchive;
	iarchive.setRootNode(rootNode_);
	holder.serialize(iarchive);		
}

void CAttribEditorCtrl::save(Serializeable& holder)
{
	EditOArchive oarchive;
	if(holder.serialize(oarchive))
	    setRootNode(oarchive.rootNode());
	else
		setRootNode(0);
}

void CAttribEditorCtrl::mixIn(Serializeable& holder)
{
	dataHolders_.push_back(holder);
}

void CAttribEditorCtrl::setRootNode(const TreeNode* rootNode)
{
	oldRootNode_ = rootNode_;
    rootNode_ = 0;
	rootNode_ = new TreeNode("");
	if(rootNode){
		*rootNode_ = *rootNode;
		onAttachData();
	}
	else{
		RemoveChilds(TLI_ROOT);
	}
}

const TreeNode* CAttribEditorCtrl::tree()
{
	return rootNode_;
}

CAttribEditorCtrl::ItemType CAttribEditorCtrl::rootItem()
{
	return TLI_ROOT;
}

bool CAttribEditorCtrl::isDataAttached() const{
	if(::IsWindow(tree_.GetSafeHwnd()) && rootNode_ != 0)
		return true;
	else
		return false;
}

BOOL CAttribEditorCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	BOOL result = popupMenu_->onCommand(wParam, lParam);
	if(!result)
		return CWnd::OnCommand(wParam, lParam);
	else
		return result;
}

bool CAttribEditorCtrl::getItemPath(ComboStrings& path, ItemType item)
{
	std::vector<ItemType> items;
	path.clear();
	while(item && item != TLI_ROOT){
		items.push_back(item);
		item = tree_.GetParentItem(item);
	}
	if(!items.empty()){
		if(style_ & HIDE_ROOT_NODE)
			path.push_back("");
		path.reserve(path.size() + items.size());
		for(int i = items.size() - 1; i >= 0; --i){
			TreeNode* node = getNode(items[i]);
			bool isElement = node->parent() && node->parent()->editType() == TreeNode::VECTOR;
			if(isElement){
				XBuffer buf;
				int index = std::distance(node->parent()->begin(), std::find(node->parent()->begin(), node->parent()->end(), node));
				buf <= index;
				path.push_back(static_cast<const char*>(buf));
			}
			else{
				const char* text = getNode(items[i])->name();
				path.push_back(text);
			}
		}
	}
	return !path.empty();
}

bool CAttribEditorCtrl::selectItemByPath(const ComboStrings& itemPath)
{
	ItemType root = TLI_ROOT;
	ItemType item = root;
	if(itemPath.empty())
		return 0;

	ComboStrings::const_iterator it = itemPath.begin();
	if(it != itemPath.end() && style_ & HIDE_ROOT_NODE)
		++it;
	while(it != itemPath.end()){
		bool isVector = false;
		if(root != TLI_ROOT){
			tree_.Expand(root, TLE_EXPAND);
			isVector = getNode(root)->editType() == TreeNode::VECTOR;
		}
		
		if(!(item = tree_.GetChildItem(root)))
			break;
		
		int child_index = 0;

		do{
			const char* text = getNode(item)->name();
			int index = isVector ? index = atoi(it->c_str()) : -1;
			if((child_index == index) || *it == text){
				++it;
				root = item;
				if(it == itemPath.end())
					goto break_loop;
				else
					break;
			}
			++child_index;
		}while(item = tree_.GetNextItem(item, TLGN_NEXT));
		if(item == 0)
			break;
	}
break_loop:
	if(root == TLI_ROOT)
		return false;
	else{
		tree_.Select(root, CTreeListCtrl::SI_SELECT);
		return true;
	}
}

// ---------------------------------------------------------------------------

static void save(XBuffer& buf, const char* str)
{
	int len = strlen(str);
	buf < len;
	if(len)
		buf < str;
}

template<class T>
static void save(XBuffer& buf, T& value)
{
	buf < value;
}

template<class T>
static void load(XBuffer& buf, T& value)
{
	buf > value;
}

static void load(XBuffer& buf, std::string& str)
{
	int len;
    buf > len;
	if(len){
		char* buffer = new char[len + 1];
		buf.read(buffer, len);
		buffer[len] = '\0';
		str = buffer;
		delete[] buffer;
	}
}

void TreeNodeStrings::clear()
{
	strings_.clear();
}

const char* TreeNodeStrings::load(XBuffer& buffer)
{
	std::string str;
	::load(buffer, str);
	if(strlen(str.c_str())){
		Strings::iterator it = std::find(strings_.begin(), strings_.end(), str);
		if(it == strings_.end()){
			strings_.push_back(str);
			return strings_.back().c_str();
		}
		else
			return it->c_str();
	}
	else
		return "";
}

// ---------------------------------------------------------------------------


TreeNodeClipboard::TreeNodeClipboard()
: clipboardFormat_(0)
, rootNode_(new TreeNode(""))
, currentContentTicks_(0)
{
	clipboardFormat_ = RegisterClipboardFormat("VistaEngineTreeNode");
	xassert(clipboardFormat_);
}

TreeNodeClipboard::~TreeNodeClipboard()
{	
}

const TreeNode* TreeNodeClipboard::get(HWND obtainerWnd, TreeNodeStrings& strings)
{
	if(retrieve(obtainerWnd, strings))
		return rootNode_;
	else
		return rootNode_;
}

void TreeNodeClipboard::set(const TreeNode* node, HWND publisherWnd)
{
	clear();
	*rootNode_ = *node;
	publish(publisherWnd);
}

void TreeNodeClipboard::get(Serializeable serializeable, HWND obtainerWnd)
{
	TreeNodeStrings strings;
	const TreeNode* clipboard = get(obtainerWnd, strings);

    EditIArchive ia(clipboard);
	serializeable.serialize(ia);
}

void TreeNodeClipboard::set(Serializeable serializeable, HWND publisherWnd)
{
	EditOArchive oa;
	serializeable.serialize(oa);
	set(oa.rootNode(), publisherWnd);
}

int TreeNodeClipboard::smartPaste(Serializeable serializeable, HWND obtainerWnd)
{
	TreeNodeStrings strings;
	TreeNode clipboard(*get(obtainerWnd, strings));
	if(clipboard.empty())
		return 0;

	EditOArchive oar;
	serializeable.serialize(oar);
	TreeNode rootNode = *oar.rootNode();
	if(rootNode.empty())
		return 0;
	TreeNode& dest = *rootNode.front();

	TreeNode::iterator it;
	int result = 0;
	FOR_EACH(clipboard, it){
		TreeNode* source = *it;
		if(TreeNode* node = dest.find(source->name(), source->type())){
			CAttribEditorCtrl::PasteSingleNode(*node, *source);
			++result;
		}
	}
	if(result){
		EditIArchive iar(&rootNode);
		serializeable.serialize(iar);
	}
	return result;
}


void TreeNodeClipboard::clear()
{
	xassert(rootNode_);
	rootNode_->clear();
}

TreeNode* TreeNodeClipboard::loadTreeNode(XBuffer& buf, TreeNodeStrings& strings)
{
	const char* name = strings.load(buf);
	TreeNode* node = new TreeNode(name);
	const char* type = strings.load(buf);
	node->setType(type);

	int editType;
	load(buf, editType);
	node->setEditType(TreeNode::EditType(editType));

	std::string value;
	load(buf, value);
	node->setValue(value.c_str());

	std::string comboList;
	load(buf, comboList);
	node->setComboList(node->editType(), comboList.c_str());

	bool hidden;
	load(buf, hidden);
	if(hidden)
		node->setHidden();

	bool expanded;
	load(buf, expanded);
	node->setExpanded(expanded);

	int childrenCount = 0;
	load(buf, childrenCount);
	xassert(childrenCount < 0xFFFF);

	TreeNode::iterator it;
	node->clear();
	for(int i = 0; i < childrenCount; ++i){
		TreeNode* child = loadTreeNode(buf, strings);
		xassert(child);
		if(child)
			node->push_back(child);
	}
	return node;
}

void TreeNodeClipboard::saveTreeNode(XBuffer& buf, const TreeNode* node)
{
	save(buf, node->name());
	save(buf, node->type());
	int editType = int(node->editType());
	save(buf, editType);
	save(buf, node->value());
	save(buf, node->comboList());
	bool hidden = node->hidden();
	save(buf, hidden);
	bool expanded = node->expanded();
	save(buf, expanded);
	int childrenCount = node->size();
	save(buf, childrenCount);

	TreeNode::const_iterator it;
	FOR_EACH(node->children(), it){
		saveTreeNode(buf, *it);
	}
}

static const DWORD CLIPBOARD_HEADER = 'VEBC';

bool TreeNodeClipboard::retrieve(HWND wnd, TreeNodeStrings& strings)
{
	if(::OpenClipboard(wnd)){
		HGLOBAL memoryHandle = (HGLOBAL)(::GetClipboardData(clipboardFormat_));
		if(memoryHandle){
			if(void* mem = GlobalLock(memoryHandle)){
				xassert(mem);
				XBuffer buffer(mem, GlobalSize(memoryHandle));
				DWORD header;
				buffer > header;
				DWORD ticks;
				buffer > ticks;
				if(header == CLIPBOARD_HEADER/* && ticks != currentContentTicks_*/){
					ShareHandle<TreeNode> node = loadTreeNode(buffer, strings);
					if(node){
						rootNode_ = node;
					}
				}
				GlobalUnlock(memoryHandle);
			}
			else
				xassert(0);

		
		}
		::CloseClipboard();
		return true;
	}
	return false;
}

TreeNodeClipboard& TreeNodeClipboard::instance()
{
	static TreeNodeClipboard treeNodeClipboard;
	return treeNodeClipboard;
}

bool TreeNodeClipboard::publish(HWND wnd)
{

	if(::OpenClipboard(wnd)){
		XBuffer buffer;
		currentContentTicks_ = ::GetTickCount();
		buffer < CLIPBOARD_HEADER;
		buffer < currentContentTicks_;
		saveTreeNode(buffer, rootNode_);

		HGLOBAL memoryHandle = GlobalAlloc(GPTR, buffer.tell());
		xassert(memoryHandle);
		if(!memoryHandle)
			return false;
		

		void* mem = GlobalLock(memoryHandle);
		xassert(mem);
		if(!mem)
			return false;
		CopyMemory(mem, buffer.buffer(), buffer.tell());
		GlobalUnlock(memoryHandle);

		VERIFY(::SetClipboardData(clipboardFormat_, memoryHandle));

		::CloseClipboard();
		return true;
	}
	return false;
}

bool TreeNodeClipboard::typeNameInClipboard(const char* typeName, HWND obtainerWnd)
{
	bool result = false;
	TreeNodeStrings strings;
	const TreeNode* node = get(obtainerWnd, strings);
	if(node->size() == 1){
		result = strcmp(node->front()->type(), typeName) == 0;
	}
	rootNode_->clear();
	return result;
}
