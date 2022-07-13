#include "stdafx.h"
#include "SurMap5.h"
#include "SToolBar.h"
#include "MainFrame.h"

IMPLEMENT_DYNAMIC(CSToolBar, CExtToolControlBar)
CSToolBar::CSToolBar()
{
}

CSToolBar::~CSToolBar()
{
}


BEGIN_MESSAGE_MAP(CSToolBar, CExtToolControlBar)
	ON_CBN_SELCHANGE(ID_BRUSH_COMBO_PLACE, OnBrushRadiusComboSelChanged)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_COMBO_PLACE, OnUpdateBrushRadius)
END_MESSAGE_MAP()

bool CSToolBar::CreateExt(CWnd* pParentWnd)
{
	if(!Create("Main Toolbar", pParentWnd, AFX_IDW_TOOLBAR) || !LoadToolBar(ID()))
	{
		TRACE0("Failed to create toolbar\n");
		return 0;
	}
	return 1;
}

void CSToolBar::OnBrushRadiusComboSelChanged ()
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd ());
	if(CSurToolBase* currentToolzer = mainFrame->m_wndView.getCurCtrl())
		currentToolzer->CallBack_BrushRadiusChanged ();
	
}

void CSToolBar::OnUpdateBrushRadius(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}
