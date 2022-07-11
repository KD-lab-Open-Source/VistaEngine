#include "StdAfx.h"

#include ".\UIEditorPanel.h"
#include "ActionManager.h"

#include "UIEditor.h"
#include "MainFrame.h"
#include "EditorView.h"
#include "ControlsTreeCtrl.h"
#include "..\AttribEditor\AttribEditorCtrl.h"

#include "..\UserInterface\UserInterface.h"
#include "TreeInterface.h"

class CUIAttribEditorCtrl : public CAttribEditorCtrl{
	void onElementChanged(CTreeListItem* pElement);
};

IMPLEMENT_DYNCREATE(CUIEditorPanel, CWnd)

CUIEditorPanel::CUIEditorPanel()
: controlsTree_(new CControlsTreeCtrl())
, attribEditor_(new CUIAttribEditorCtrl())
{
}

CUIEditorPanel::~CUIEditorPanel()
{
    delete controlsTree_;
    controlsTree_ = 0;
    delete attribEditor_;
    attribEditor_ = 0;
}


BEGIN_MESSAGE_MAP(CUIEditorPanel, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

int CUIEditorPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rt;
	GetClientRect (&rt);
	CRect upper_rect = rt;
	upper_rect.bottom /= 2;
	CRect lower_rect = rt;
	lower_rect.top = upper_rect.bottom;
	lower_rect.bottom -= 24;

	controlsTreeImages_.Create (IDB_CONTROLS_TREE, 16, 2, RGB (255, 0, 255));
	controlsTree_->Create (WS_CHILD | WS_VISIBLE, upper_rect, this, 0);
	controlsTree_->SetFont (CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	controlsTree_->SetImageList (&controlsTreeImages_);
                 
	controlsTree_->SetStyle (TLC_TREELIST | TLC_TREELINE | TLC_BUTTON | TLC_IMAGE | TLC_SHOWSELALWAYS |
				     		    TLC_READONLY | TLC_HOTTRACK | TLC_DRAG | TLC_DROP | TLC_DROPTHIS | TLC_CHECKBOX | TLC_NOAUTOCHECK);
	controlsTree_->InsertColumn ("Controls", TLF_DEFAULT_LEFT, lower_rect.Width () - GetSystemMetrics (SM_CXVSCROLL) - 3);
                 
	attribEditor_->setStyle(CAttribEditorCtrl::HIDE_ROOT_NODE |
							CAttribEditorCtrl::COMPACT |
							CAttribEditorCtrl::AUTO_SIZE |
							CAttribEditorCtrl::DEEP_EXPAND |
							CAttribEditorCtrl::EXPAND_ALL);

	attribEditor_->Create (WS_VISIBLE | WS_CHILD, lower_rect, this, 0);
	return 0;
}

void CUIEditorPanel::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (IsWindow (controlsTree_->GetSafeHwnd ())) {
		controlsTree_->MoveWindow (0, 0, cx, cy / 2, TRUE);
		controlsTree_->SetColumnWidth (0, cx - GetSystemMetrics (SM_CXVSCROLL) - 3);
	}
	if (IsWindow(attribEditor_->GetSafeHwnd ()))
		attribEditor_->MoveWindow (0, cy / 2, cx, cy - cy / 2, TRUE);
}


void CUIAttribEditorCtrl::onElementChanged (CTreeListItem* item)
{
	if(item){
		TreeNode* node = getNode(item);
		std::string type = node->type();
		// if(type == typeid(UI_Sprite).name())
		// 	UI_Dispatcher::instance().init();
	}
	
	ActionManager::the().setChanged(true);
	__super::onElementChanged(item);
	uiEditorFrame()->GetControlsTree()->updateSelected ();
}
