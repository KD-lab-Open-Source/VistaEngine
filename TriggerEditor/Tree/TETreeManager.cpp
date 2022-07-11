#include "StdAfx.h"
#include "../Resource.h"
#include "TETreeManager.h"
#include "TETreeDlg.h"
#include "TETreeNotifyListener.h"

TETreeManager::TETreeManager(void)
{
}

TETreeManager::~TETreeManager(void)
{
}

void TETreeManager::enableDocking(bool enable)
{
    if(enable)
        controlBar_.EnableDocking(CBRS_ALIGN_ANY);
    else
        controlBar_.EnableDocking(0);
}

/*
void TETreeManager::dock()
{
	pParent->DockControlBar(&controlBar_, AFX_IDW_DOCKBAR_LEFT);
}
*/

bool TETreeManager::create(CFrameWnd* pParent, DWORD dwStyle)
{
	parentFrame_ = pParent;

	VERIFY(controlBar_.Create("Actions", parentFrame_, ID_VIEW_PROJECT_TREE));
	controlBar_.SetInitDesiredSizeVertical( CSize(250,250) );
	controlBar_.SetInitDesiredSizeHorizontal( CSize(400,250) );
	controlBar_.SetInitDesiredSizeFloating( CSize(250,250) );


	treeDlg_.reset(new TETreeDlg(pParent));
	if (!treeDlg_)
		return false;
	
	VERIFY(treeDlg_->Create(TETreeDlg::IDD, &controlBar_));

	return true;
}

void TETreeManager::dock(UINT dockBarID)
{
	parentFrame_->DockControlBar(&controlBar_, dockBarID);
}

bool TETreeManager::show(eShowHide e)
{
	if (e == SH_SHOW)
		parentFrame_->ShowControlBar(&controlBar_, TRUE, FALSE); 
	else 
		parentFrame_->ShowControlBar(&controlBar_, FALSE, FALSE); 
	return true;
}

bool TETreeManager::isVisible() const {
	return (IsWindowVisible(controlBar_.GetSafeHwnd()) == TRUE);
}

xTreeListCtrl& TETreeManager::getTreeListCtrl() const
{
	return treeDlg_->getTreeCtrl();
}

void TETreeManager::setTETreeNotifyListener(TETreeNotifyListener* ptl)
{
	treeDlg_->setTETreeNotifyListener(ptl);
}
