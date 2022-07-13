#include "StdAfx.h"

#include "SafeCast.h"
#include "LibraryEditorTree.h"
#include "LibraryEditorWindow.h"


#include "mfc\PopupMenu.h"
#include "TreeInterface.h"

IMPLEMENT_DYNAMIC(CLibraryEditorTree, CObjectsTreeCtrl)

BEGIN_MESSAGE_MAP(CLibraryEditorTree, CObjectsTreeCtrl)
	ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

CLibraryEditorTree::CLibraryEditorTree(CAttribEditorCtrl* attribEditor)
: attribEditor_(attribEditor)
, popupMenu_(new PopupMenu(100))
, draggedItem_(0)
{

}

CLibraryEditorTree::~CLibraryEditorTree()
{

}


// ----------------------------------------------------------------------------


int CLibraryEditorTree::OnCreate(LPCREATESTRUCT createStruct)
{
	if(__super::OnCreate(createStruct))
		return -1;

	SetStyle(TLC_TREELIST
			 | TLC_READONLY
			 | TLC_SHOWSELACTIVE
			 | TLC_SHOWSELALWAYS
			 | TLC_SHOWSELFULLROWS
			 | TLC_IMAGE
			 | TLC_DRAG | TLC_DROP | TLC_DROPTHIS
//			 | TLC_HEADER
             | TLC_BUTTON
			 | TLC_TREELINE);
	
	InsertColumn(".", TLF_DEFAULT_LEFT, 100);
	return 0;
}


LibraryTabBase* CLibraryEditorTree::tab()
{
	return safe_cast<CLibraryEditorWindow*>(GetParent())->currentTab();
}

void CLibraryEditorTree::OnSize(UINT nType, int cx, int cy)
{
    SetColumnWidth(0, cx - GetSystemMetrics(SM_CXVSCROLL) - GetSystemMetrics(SM_CXBORDER) * 2);
}

BOOL CLibraryEditorTree::OnCommand(WPARAM wParam, LPARAM lParam)
{
    popupMenu_->onCommand(wParam, lParam);

    return __super::OnCommand(wParam, lParam);
}

void CLibraryEditorTree::spawnMenuAtObject(TreeObject* object)
{
    CRect rt;
    GetItemRect(objectItem(object), 0, &rt, true);
    ClientToScreen(&rt);

    popupMenu_->spawn(CPoint(rt.left, rt.bottom), GetSafeHwnd());
}
