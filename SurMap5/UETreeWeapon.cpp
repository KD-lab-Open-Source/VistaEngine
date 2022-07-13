#include "stdafx.h"
#include "SurMap5.h"
#include "UETreeWeapon.h"
#include "UnitEditorDlg.h"
#include "Dictionary.h"
#include "..\Units\UnitAttribute.h"

#include "EditArchive.h"
#include "CreateAttribDlg.h"

extern ShareHandle<TreeNode> attributeClipboard;

IMPLEMENT_DYNAMIC(CUETreeWeapon, CUETreeBase)
CUETreeWeapon::CUETreeWeapon(CAttribEditorCtrl& attribEditor, const CRect& rt, CWnd* parent)
: CUETreeBase(attribEditor)
, clickedItem_(0)
, pasteByDefault_(false)
{
	BOOL result = Create(WS_VISIBLE | WS_CHILD, rt, parent, 0);
	xassert(result);
	rebuildTree();
}

CUETreeWeapon::~CUETreeWeapon()
{
}


BEGIN_MESSAGE_MAP(CUETreeWeapon, CUETreeBase)
	ON_WM_CREATE()
	ON_COMMAND(IDM_WEAPON_PRM_ADD, OnMenuAdd)
	ON_COMMAND(IDM_WEAPON_PRM_DELETE, OnMenuDelete)
END_MESSAGE_MAP()


// CUETreeWeapon message handlers

int CUETreeWeapon::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	GetClientRect(&rect);

	SetStyle(TLC_TREELIST
		| TLC_READONLY
		| TLC_SHOWSELACTIVE
		| TLC_SHOWSELALWAYS
		| TLC_SHOWSELFULLROWS
		
		| TLC_HEADER
		| TLC_BUTTON
		| TLC_TREELINE);

	InsertColumn (dictionary().translate("Оружие"), TLF_DEFAULT_LEFT,
				  rect.Width () - GetSystemMetrics (SM_CXVSCROLL) - GetSystemMetrics (SM_CXBORDER) * 2);

	return 0;
}

// User functinos

void CUETreeWeapon::rebuildTree () 
{
    DeleteAllObjects ();
	
	WeaponPrmLibrary::Map& objects_map = WeaponPrmLibrary::instance().map ();
	WeaponPrmLibrary::Map::iterator it;

	typedef ClassCreatorFactory<WeaponPrm> CF;
	typedef CF::ClassCreatorBase CreatorType;
	int classes_count = CF::instance ().size ();
	for (int i = 0; i < classes_count; ++i) {
		CreatorType& creator = CF::instance ().findByIndex (i);
		std::string nameAlt = dictionary ().translate (creator.nameAlt ());
		BuildTypeNode (AddTreeObject (creator, nameAlt));
	}
}

void CUETreeWeapon::BuildTypeNode (ItemType _handle)
{
    typedef ClassCreatorFactory<WeaponPrm> CF;
    typedef CF::ClassCreatorBase CreatorType;

    CreatorType* creator = GetObjectByHandle (_handle).get<CreatorType*> ();
    xassert (creator);

    DeleteChildItems (_handle);

	WeaponPrmLibrary::Map& objects_map = WeaponPrmLibrary::instance().map ();
	WeaponPrmLibrary::Map::iterator it;

    FOR_EACH (objects_map, it) {
        std::string name = it->first;
        WeaponPrm& weapon = *it->second;

		const char* name1 = typeid (weapon).name ();
		const char* name2 = creator->name ();
		if (strcmp (name1, name2) == 0) {
			AddTreeObject (weapon, name.c_str(), _handle);
		}
    }
}

void CUETreeWeapon::onItemSelected(ItemType item)
{
	attribEditor_.DetachData ();

	if (item == 0 || !GetItemState (item, TLIS_SELECTED)) {
		return;
	}

    if (TreeObjectBase* object = &GetObjectByHandle (item)) {

		if (object->get<WeaponPrm*>()) {
			object->save (attribEditor_);
		} else {
			ItemType i = GetChildItem (TLI_ROOT);
			while (i) {
				if (i != item) {
					Expand (i, TLE_COLLAPSE);
				} else {
					Expand (i, TLE_EXPAND);
					ItemType child_item = GetChildItem (i);
					while (child_item) {
						Expand (child_item, TLE_EXPAND);
						child_item = GetNextSiblingItem(child_item);
					}

				}
				i = GetNextSiblingItem (i);
			}
		}
	}

	attribEditor_.RedrawWindow ();
}

void CUETreeWeapon::onItemRClick(ItemType item)
{
	if (item == 0)
		return;

	if ((DWORD(item) & 0xFFFF0000) == 0) {
		xassert("Всё плохо :(");
		return; // очень странно...
	}
	
	clickedItem_ = item;
	
	TreeObjectBase* object = &GetObjectByHandle (item);
	typedef ClassCreatorFactory<WeaponPrm> CF;
    typedef CF::ClassCreatorBase CreatorType;

	if (CreatorType* creator = object->get<CreatorType*> ()) {
#ifndef _VISTA_ENGINE_EXTERNAL_
		CRect rt;
		GetItemRect (item, 0, &rt, TRUE);
		ClientToScreen (&rt);
		CMenu menu;
		menu.LoadMenu (IDR_WEAPON_TYPE_MENU);
		CMenu* child_menu = menu.GetSubMenu (0);
		child_menu->TrackPopupMenu (TPM_LEFTBUTTON | TPM_LEFTALIGN, rt.left, rt.bottom, this);
#endif
	}
	else if (WeaponPrm* attribute_base = object->get<WeaponPrm*> ())	{
		CRect rt;
		GetItemRect (item, 0, &rt, TRUE);
		ClientToScreen (&rt);
		CMenu menu;
		menu.LoadMenu (IDR_WEAPON_PRM_MENU);
		CMenu* child_menu = menu.GetSubMenu (0);
		child_menu->TrackPopupMenu (TPM_LEFTBUTTON | TPM_LEFTALIGN, rt.left, rt.bottom, this);
	}
}

void CUETreeWeapon::OnMenuAdd()
{
	typedef ClassCreatorFactory<WeaponPrm> CF;
    typedef CF::ClassCreatorBase CreatorType;
	CUETreeWeapon::ItemType item = clickedItem_;
	if (CreatorType* creator = GetObjectByHandle (item).get<CreatorType*> ()) {
		bool allowPaste = (attributeClipboard.get() != 0) &&
			(attributeClipboard->front ()->value () == dictionary().translate(creator->nameAlt()));

		std::string name_base = dictionary().translate("Новое оружие");
		std::string default_name = name_base;
		int index = 1;
		while (WeaponPrmLibrary::instance().find (default_name.c_str())) {
			CString str;
			str.Format ("%s%i", name_base.c_str(), index);
			default_name = static_cast<const char*>(str);
			++index;
		}

		CCreateAttribDlg create_dlg (allowPaste, pasteByDefault_, default_name.c_str());
        int result = create_dlg.DoModal ();
		if (result == IDOK) {
			std::string attrib_name = create_dlg.getAttribName ().c_str ();
            WeaponPrm* new_attribute = creator->create ();
			if (allowPaste && create_dlg.getPaste ()) {
				EditIArchive iarchive (attributeClipboard);
				iarchive & TRANSLATE_NAME (new_attribute, "buffer", "buffer");
			}
			WeaponPrmLibrary::instance ().add (attrib_name, new_attribute);
			BuildTypeNode (item);
		}
		pasteByDefault_ = false;
		UpdateWindow ();
	}
}

void CUETreeWeapon::OnMenuDelete()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	WeaponPrm* attributeBase = GetObjectByHandle (clickedItem_).get<WeaponPrm*> ();
	xassert (attributeBase != 0);

	WeaponPrmLibrary::Map& objects_map = WeaponPrmLibrary::instance ().map ();
	WeaponPrmLibrary::Map::iterator it;

	FOR_EACH (objects_map, it) {
		if (it->second == attributeBase) {
			WeaponPrmLibrary::instance().remove (it->first);
			BuildTypeNode (GetParentItem (clickedItem_));
			return;
		}
	}
#endif
}
