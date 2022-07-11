#include "StdAfx.h"
#include "ObjectsTreeCtrl.h"

class TreeRootObject : public TreeObject{
public:
	TreeRootObject(CObjectsTreeCtrl* tree)
	{
		tree_ = tree;
		item_ = TLI_ROOT;
	}
	virtual const std::type_info& getTypeInfo () const{ return typeid(TreeRootObject); }
	virtual void* getPointer() const{ return 0; }
#ifndef __DISABLE_SERIALIZEABLE__
	virtual Serializeable getSerializeable(const char* name = "", const char* nameAlt = ""){
		return Serializeable();
	}
#endif
};


IMPLEMENT_DYNAMIC(CObjectsTreeCtrl, CTreeListCtrl)

BEGIN_MESSAGE_MAP(CObjectsTreeCtrl, CTreeListCtrl)
    ON_NOTIFY_REFLECT(TLN_BEGINLABELEDIT, OnBeginLabelEdit )
	ON_NOTIFY_REFLECT(TLN_ITEMCHECK, OnItemCheck)

    ON_NOTIFY_REFLECT(TLN_BEGINDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(TLN_DRAGENTER, OnDragEnter)
    ON_NOTIFY_REFLECT(TLN_DRAGLEAVE, OnDragLeave)
    ON_NOTIFY_REFLECT(TLN_DRAGOVER, OnDragOver)
    ON_NOTIFY_REFLECT(TLN_DROP, OnDrop)

    ON_NOTIFY_REFLECT(NM_RCLICK, OnItemRClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnItemDblClick)

	ON_NOTIFY_REFLECT(TLN_SELCHANGED, OnSelChanged)
END_MESSAGE_MAP()


CObjectsTreeCtrl::CObjectsTreeCtrl()
: rootObject_(new TreeRootObject(this))
{
    registerWindowClass ();
}

bool CObjectsTreeCtrl::initControl(DWORD dwStyle)
{
    if (::IsWindow (GetSafeHwnd ())) {

    } else {
        CRect rcClientRect;
        GetClientRect (&rcClientRect);
        if (! Create (WS_VISIBLE | WS_CHILD, rcClientRect, this, 0))
            return false;
    }
    SetStyle (dwStyle);
    return true;
}

void CObjectsTreeCtrl::registerWindowClass ()
{
    WNDCLASS wndclass;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if(!::GetClassInfo(hInst, className(), &wndclass)){
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

		if(!::AfxRegisterClass (&wndclass))
			::AfxThrowResourceException();
    }
}

TreeObject* CObjectsTreeCtrl::addObject(TreeObject* object, TreeObject* rootObject, TreeObject* afterObject)
{
	ItemType afterItem = afterObject ? afterObject->item() : TLI_LAST;
	ItemType item = InsertItem(object->name(), rootObject->item(), afterItem);
	SetItemData(item, reinterpret_cast<DWORD_PTR>(object));
    object->addedToTree(this, item);
	objects_.push_back(object);
	return item ? object : 0;
}

CObjectsTreeCtrl::ItemType CObjectsTreeCtrl::addObject(TreeObject* object, ItemType rootItem, ItemType afterItem)
{
	ItemType item = InsertItem(object->name(), rootItem, afterItem);
	SetItemData(item, reinterpret_cast<DWORD_PTR>(object));
    object->addedToTree(this, item);
	objects_.push_back(object);
    return item;
}

void CObjectsTreeCtrl::expandParents(ItemType item)
{
	std::vector<ItemType> items;
    ItemType parent = GetParentItem(item);
    while(parent && parent != TLI_ROOT) {
        items.push_back(parent);
		if(reinterpret_cast<DWORD>(parent) < 0x0000FFFF){
			xassert(0);
            continue;
        }
        parent = GetParentItem (parent);
    }
	for(int i = items.size() - 1; i >= 0; --i) {
		Expand(items[i], TLE_EXPAND);
	}
}

void CObjectsTreeCtrl::clear()
{
    SelectItem(0);
    DeleteAllItems ();
    objects_.clear ();
}

void CObjectsTreeCtrl::deleteChildItems(ItemType _item)
{
	if(_item == TLI_ROOT)
		clear();
	else{
		ItemType selected_item = GetSelectedItem ();
		ItemType current_item = GetChildItem (_item);
		while (current_item != 0)  {
			ItemType prev_item = current_item;
			current_item = GetNextSiblingItem (current_item); 
			if(GetItemState (prev_item, TLIS_SELECTED))
				SelectItem(_item);
			deleteObject(objectByItem(prev_item));
		}
	}
}
void CObjectsTreeCtrl::deleteObject(TreeObject* object)
{
	if(object){
		ItemType item = object->item();
		object->item_ = 0;
		xassert(object->numRef() == 1);
		Objects::iterator it = std::find(objects_.begin(), objects_.end(), object);
		if(it != objects_.end())
			objects_.erase(it);
		else
			xassert(0);
		deleteChildItems(item);
		DeleteItem(item);
	}
	else
		xassert(0);
}

CObjectsTreeCtrl::ItemType CObjectsTreeCtrl::objectItem(TreeObject* object)
{
    return object->item();
}

TreeObject* CObjectsTreeCtrl::objectByItem(ItemType item)
{
	if(item == 0)
		return 0;
	if(item == TLI_ROOT)
		return rootObject_;

    TreeObject* object = reinterpret_cast<TreeObject*>(GetItemData(item));
    xassert(object);
    return object;
}


void CObjectsTreeCtrl::insertColumn(const char* name, int width)
{
	InsertColumn(name, TLF_DEFAULT_LEFT, width);
}

void CObjectsTreeCtrl::focusItem(ItemType item)
{
	expandParents(item);
	Select(item, CObjectsTreeCtrl::SI_SELECT);
	EnsureVisible(item, 0);
}

void CObjectsTreeCtrl::OnBeginDrag(NMHDR* nm, LRESULT* result)
{
	NMTREELISTDROP* nmTreeListDrop = reinterpret_cast<NMTREELISTDROP*>(nm);
	draggedObjects_.clear();
	TreeObjects objects = selection();
	if(!objects.empty()){
		*result = 0;
		TreeObjects::iterator it;
		FOR_EACH(objects, it){
			if(!(*it)->onBeginDrag()){
				*result = 1;
				return;
			}
		}
		draggedObjects_ = objects;
	}
	else
		*result = 1;
}

void CObjectsTreeCtrl::OnDragEnter(NMHDR* nm, LRESULT* result)
{
}

void CObjectsTreeCtrl::OnDragLeave(NMHDR* nm, LRESULT* result)
{
}

void CObjectsTreeCtrl::OnDragOver(NMHDR* nm, LRESULT* result)
{
	NMTREELISTDROP* nmTreeListDrop = reinterpret_cast<NMTREELISTDROP*>(nm);

	TreeObject* object = objectByItem(nmTreeListDrop->pItem);
	if(object && !draggedObjects_.empty()){
		if(object->onDragOver(draggedObjects_)){
			SetTargetItem(nmTreeListDrop->pItem);
			*result = 0;
			return;
		}
	}
	SetTargetItem(0);
	*result = 1;
}

void CObjectsTreeCtrl::OnDrop(NMHDR* nm, LRESULT* result)
{
	NMTREELISTDROP* nmTreeListDrop = reinterpret_cast<NMTREELISTDROP*>(nm);
	
	CTreeListItem* item = nmTreeListDrop->pItem;

	if(item && !draggedObjects_.empty()){
		if(item != TLI_ROOT){
			TreeObject* destObject = objectByItem(item);
			destObject->onDrop(draggedObjects_);
		}
		else
			rootObject()->onDrop(draggedObjects_);
	}
}

void CObjectsTreeCtrl::OnBeginLabelEdit(NMHDR* nm, LRESULT* result)
{
}

void CObjectsTreeCtrl::OnItemCheck(NMHDR* nm, LRESULT* result)
{
}

void CObjectsTreeCtrl::OnItemDblClick(NMHDR* nm, LRESULT* result)
{
	NMTREELIST* nmTreeList = reinterpret_cast<NMTREELIST*>(nm);

	ItemType item = nmTreeList->pItem;
	if(item == 0){
		//onRClick();
		return;
	}
	else{
		if(TreeObject* object = objectByItem(item))
			*result = object->onDoubleClick();
	}
}

void CObjectsTreeCtrl::OnItemRClick(NMHDR* nm, LRESULT* result)
{
	NMTREELIST* nmTreeList = reinterpret_cast<NMTREELIST*>(nm);

	ItemType item = nmTreeList->pItem;
	onRightClick();
	if(item == 0){
		return;
	}
	else{
		if(TreeObject* object = objectByItem(item))
			object->onRightClick();
	}
}

void CObjectsTreeCtrl::OnSelChanged(NMHDR* nm, LRESULT* result)
{
    NMTREELIST* nmTreeList = reinterpret_cast<NMTREELIST*>(nm);
    if(nmTreeList->pItem && GetItemState(nmTreeList->pItem, TLIS_SELECTED)){
		if(TreeObject* object = objectByItem(nmTreeList->pItem))
			object->onSelect();
    }
}

TreeObjects CObjectsTreeCtrl::selection()
{
	TreeObjects result;
	POSITION pos = GetFirstSelectedItemPosition ();
	while(ItemType current_item = GetNextSelectedItem (pos)) {
		TreeObject* object = objectByItem(current_item);
		xassert(object);
		result.push_back(object);
	}
    return result;
}

TreeObject* CObjectsTreeCtrl::selected()
{
	if(ItemType item = GetSelectedItem())
		return objectByItem(item);
	else
		return 0;
}

void CObjectsTreeCtrl::selectObject(TreeObject* object)
{
	xassert(object && object->item_);
    Select(object->item_, SI_SELECT);
}

void CObjectsTreeCtrl::overrideRoot(TreeObject* newRoot)
{
	rootObject_ = newRoot;
	rootObject_->item_ = TLI_ROOT;
	rootObject_->tree_ = this;
}


std::size_t TreeObject::size() const
{
	xassert(0);
	return 0;
}

bool TreeObject::empty() const
{
	return tree_->GetChildItem(item_) == 0;
}

void TreeObject::erase()
{
	tree_->deleteObject(this);
}

// --------------------------------------------------------------------------

void TreeObject::showCheckBox(bool show)
{
	xassert(tree_ && item_);
	if(show)
		tree_->SetItemState(item_, TLIS_SHOWCHECKBOX, 0);
	else
		tree_->SetItemState(item_, 0, TLIS_SHOWCHECKBOX);
}

void TreeObject::setImage(int defaultImage, int expandedImage, int selectedImage, int expandedSelectedImage)
{
	xassert(tree_ && item_);
	tree_->SetItemImage(item_, defaultImage, expandedImage, selectedImage, expandedSelectedImage);
}

void TreeObject::setCheck(bool checked)
{
	xassert(item_);
	item_->SetCheck(checked);
}

TreeObject* TreeObject::parent()
{
	if(!tree_ || !item_)
		return 0;

	if(item_ == TLI_ROOT)
		return 0;
	else
		return tree_->objectByItem(tree_->GetParentItem(item_));
}

const TreeObject* TreeObject::parent() const
{
	if(!tree_ || !item_)
		return 0;

	if(item_ == TLI_ROOT)
		return 0;
	else
		return tree_->objectByItem(tree_->GetParentItem(item_));
}

void TreeObject::updateLabel()
{
	xassert(tree_ && item_);
	tree_->SetItemText(item_, 0, name());
}

void TreeObject::expandParents()
{
	xassert(tree_ && item_);
	tree_->expandParents(item_);
}

void TreeObject::focus()
{
	xassert(tree_ && item_);
	tree_->focusItem(item_);
}

bool TreeObject::selected() const
{
	return bool(tree_->GetItemState(item_, TLIS_SELECTED));
}
