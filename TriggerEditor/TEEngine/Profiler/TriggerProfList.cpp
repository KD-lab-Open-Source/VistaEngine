#include "stdafx.h"

#include <prof-uis.h>

#include "resource.h"
#include "triggerproflist.h"
#include "TriggerDbgDlg.h"

TriggerProfList::TriggerProfList(void)
: parentFrame_(0)
, triggerDbgDlg_(*new TriggerDbgDlg())
, controlBar_(*new CExtControlBar())
{
}

TriggerProfList::~TriggerProfList(void)
{
	delete &triggerDbgDlg_;
	delete &controlBar_;
}

void TriggerProfList::setTriggerChainProfiler(ITriggerChainProfiler* ptrProf){
	triggerDbgDlg_.setTriggerChainProfiler(ptrProf);
}

bool TriggerProfList::create(CFrameWnd* pParent, DWORD dwStyle)
{
	parentFrame_ = pParent;

	VERIFY(controlBar_.Create("Debugger", parentFrame_, ID_VIEW_PROFILER));
	controlBar_.EnableDocking(CBRS_ALIGN_ANY);
	controlBar_.SetInitDesiredSizeVertical( CSize(200,400) );
	controlBar_.SetInitDesiredSizeHorizontal( CSize(400,150) );
	controlBar_.SetInitDesiredSizeFloating( CSize(400,200) );

	parentFrame_->DockControlBar(&controlBar_, AFX_IDW_DOCKBAR_BOTTOM);
	
	VERIFY(triggerDbgDlg_.Create(TriggerDbgDlg::IDD, &controlBar_));

	return true;
}

bool TriggerProfList::show() const 
{
	parentFrame_->ShowControlBar(&controlBar_, TRUE, FALSE); 
	triggerDbgDlg_.OnShow();
	return true;
}

bool TriggerProfList::hide() const
{
	parentFrame_->ShowControlBar(&controlBar_, FALSE, FALSE); 
	triggerDbgDlg_.OnHide();
	return true;
}

bool TriggerProfList::isVisible() const {
	return IsWindowVisible(controlBar_.GetSafeHwnd());
}

bool TriggerProfList::load()
{
	return triggerDbgDlg_.Load();
}

bool TriggerProfList::next() const
{
	return triggerDbgDlg_.Next();
}

bool TriggerProfList::prev() const
{
	return triggerDbgDlg_.Prev();
}

bool TriggerProfList::activate()
{
	if (triggerDbgDlg_.Activate())
		return load();
	return false;
}

void TriggerProfList::enableDocking(bool enable)
{
    if(enable)
        controlBar_.EnableDocking(CBRS_ALIGN_ANY);
    else
        controlBar_.EnableDocking(0);
}

void TriggerProfList::dock(UINT uiDocBarId)
{
	parentFrame_->DockControlBar(&controlBar_, uiDocBarId);
}

void TriggerProfList::setTriggerEditorView(ITriggerEditorView* ptrTEView)
{
	triggerDbgDlg_.setTriggerEditorView(ptrTEView);
}

bool TriggerProfList::canBeActivated() const{
	return triggerDbgDlg_.canBeActivated();
}
