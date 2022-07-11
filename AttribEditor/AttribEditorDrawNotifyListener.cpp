#include "StdAfx.h"
#include "mfc\TreeListCtrl.h"
#include "AttribEditorCtrl.h"
#include "AttribEditorDrawNotifyListener.h"
#include "TreeInterface.h"
#include "TreeEditor.h"

#include "TreeInterface.h"
#include "Serialization\TreeEditor.h"

AttribEditorDrawNotifyListener::AttribEditorDrawNotifyListener(CAttribEditorCtrl& ctrl)
: attribEditor_(ctrl)
{
}

AttribEditorDrawNotifyListener::~AttribEditorDrawNotifyListener()
{
}

DWORD AttribEditorDrawNotifyListener::onPrepaint(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onPostpaint(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onPreerase(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onPosterase(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onItemPrepaint(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onItemPostpaint(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onItemPreerase(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

DWORD AttribEditorDrawNotifyListener::onItemPosterase(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi)
{
	return CDRF_DODEFAULT;
}

TreeEditor::Icon getNodeButtonIcon(TreeNode* node)
{
	TreeEditor::Icon icon = TreeEditor::ICON_NONE;
	if(node->editor()){
		icon = node->editor()->buttonIcon();
	}
	else{
		switch(node->editType()){
			case TreeNode::COMBO:
			case TreeNode::COMBO_MULTI:
				icon = TreeEditor::ICON_DROPDOWN;
		}
	}
	return icon;
}


DWORD AttribEditorDrawNotifyListener::onSubitemPrepaint(CTreeListCtrl& source, SUBITEM_CUSTOM_DRAW_INFO* pscdi)
{
	if(pscdi->iSubItem == 1){
		CTreeListItem* item = pscdi->item;
		CString title;
		title = item->GetText();
		if(TreeNode* node = getNode(item)){
			if(getNodeButtonIcon(node) != TreeEditor::ICON_NONE){
				pscdi->rcText.right -= 16;
				pscdi->rc.right -= 16;
			}
			if (node->editor ()) {
				if(node->editor ()->prePaint(pscdi->hdc, pscdi->rc))
					return CDRF_SKIPDEFAULT;
			}
		}
	}
	return CDRF_DODEFAULT;
}
DWORD AttribEditorDrawNotifyListener::onSubitemPostpaint(CTreeListCtrl& source, SUBITEM_CUSTOM_DRAW_INFO* pscdi)
{
	if(pscdi->iSubItem == 1){
		CTreeListItem* item = pscdi->item;
		if(TreeNode* node = getNode(item)){
			TreeEditor::Icon icon = getNodeButtonIcon(node);
			
			if (node->editor())
				if(node->editor()->postPaint(pscdi->hdc, pscdi->rc))
					return CDRF_SKIPDEFAULT;

			if(icon != TreeEditor::ICON_NONE){
				CDC dc;
				dc.Attach(pscdi->hdc);
				attribEditor_.imageList().Draw(&dc, icon, CPoint(pscdi->rc.right - 1, pscdi->rc.top + 1), ILD_TRANSPARENT);
				dc.Detach();
			}
		}
	}
	return CDRF_DODEFAULT;
}
