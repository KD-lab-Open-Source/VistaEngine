#include "stdafx.h"
#include "ConditionList.h"
#include "Dictionary.h"
#include "..\TriggerEditor\TriggerExport.h"
#include "ConditionEditor.h"

BEGIN_MESSAGE_MAP(CConditionList, CTreeListCtrl)
    ON_NOTIFY_REFLECT (TLN_BEGINDRAG, OnBeginDrag )
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

CConditionList::CConditionList (IConditionDroppable* condition_editor)
: draging_ (false)
, condition_editor_ (condition_editor)
{
}

namespace{
CTreeListItem* findSubNode(CTreeListCtrl& tree, CTreeListItem* parent, const char* text)
{
	if(CTreeListItem* item = tree.GetChildItem(parent)){
		do{
			if(strcmp(tree.GetItemText(item, 0), text) == 0)
				return item;
		}while(item = tree.GetNextItem(item, TLGN_NEXT));
	}
	return 0;
}
}

void CConditionList::initControl ()
{
	DWORD dwTreeStyle = 
		TLC_TREELIST		                    // TreeList or List
		| TLC_READONLY
		| TLC_DOUBLECOLOR	                    // double color background
//		| TLC_MULTIPLESELECT                    // single or multiple select
		| TLC_SHOWSELACTIVE	                    // show active column of selected item
		| TLC_SHOWSELALWAYS	                    // show selected item always
		| TLC_SHOWSELFULLROWS                   // show selected item in fullrow mode
		
		| TLC_HEADER		                    // show header
		| TLC_HGRID			                    // show horizonal grid lines
		| TLC_VGRID			                    // show vertical grid lines
		| TLC_TGRID			                    // show tree horizonal grid lines ( when HGRID & VGRID )
//		| TLC_HGRID_EXT		                    // show extention horizonal grid lines
//		| TLC_VGRID_EXT		                    // show extention vertical grid lines
//		| TLC_HGRID_FULL	                    // show full horizonal grid lines
		
//		| TLC_IMAGE			                    // show image
		| TLC_BUTTON
		| TLC_TREELINE
		
//		| TLC_HOTTRACK		                    // show hover text 
		| TLC_DRAG			                    // drag support
//		| TLC_DROP			                    // drop support
//		| TLC_DROPTHIS		                    // drop on this support
//		| TLC_HEADDRAGDROP	                    // head drag drop
		;

	CRect rect;

    SetStyle (dwTreeStyle);
    GetClientRect (&rect);
    InsertColumn (TRANSLATE("Условия"), TLF_DEFAULT_LEFT,
                  rect.Width () - GetSystemMetrics (SM_CXVSCROLL));

	ComboStrings strings;
	splitComboList(strings, triggerInterface().conditionComboList());
	ComboStrings::const_iterator it;
	int index = 0;
	CTreeListItem* addedItem = 0;
	FOR_EACH(strings, it) {
		ComboStrings path;
		::splitComboList(path, TRANSLATE(it->c_str()), '\\');
		CTreeListItem* item = TLI_ROOT;
		for(int path_position = 0; path_position < path.size(); ++path_position){
			const char* leaf = path[path_position].c_str();
			if(path_position + 1 == path.size()){
				addedItem = InsertItem(leaf, item);
				SetItemData(addedItem, index++);
				//setTreeListItem(tree.InsertItem(leaf, item));
			}
			else{
				if(CTreeListItem* folder = findSubNode(*this, item, leaf))
					item = folder;
				else{
					folder = InsertItem(leaf, item);
					SetItemData(folder, DWORD_PTR(-1));
					item = folder;
				}
			}
		}
	}
}


void CConditionList::OnBeginDrag (NMHDR* pNMHDR, LRESULT* cancel)
{
	NMTREELISTDRAG* pNM = reinterpret_cast<NMTREELISTDRAG*>(pNMHDR);
	CTreeListItem* selected_one = GetSelectedItem ();

	int conditionIndex = GetItemData(selected_one);
	if(conditionIndex >= 0){
		condition_editor_->beginExternalDrag(CPoint(0, 0), conditionIndex);
		*cancel = TRUE;
	}
	else{
		*cancel = TRUE;
	}
}

void CConditionList::OnMouseMove(UINT nFlags, CPoint point)
{
    CTreeListCtrl::OnMouseMove(nFlags, point);
}

void CConditionList::OnLButtonUp(UINT nFlags, CPoint point)
{
	CTreeListCtrl::OnLButtonUp(nFlags, point);
}

BOOL CConditionList::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	/*
	if(condition_editor_) {
		CPoint screenPoint;
		GetCursorPos(&screenPoint);
		bool canBeDropped = condition_editor_->canBeDropped(screenPoint);

		if(canBeDropped)
			SetCursor(::LoadCursor (0, MAKEINTRESOURCE(IDC_ARROW)));
		else
			SetCursor(::LoadCursor(0, MAKEINTRESOURCE(IDC_NO)));
			*/
	if (draging_) {
		SetCursor (::LoadCursor (0, MAKEINTRESOURCE(IDC_NO)));
		return TRUE;
	} else {
		SetCursor (::LoadCursor (0, MAKEINTRESOURCE(IDC_ARROW)));
		return TRUE;
	}
}
