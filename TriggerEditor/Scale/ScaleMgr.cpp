#include "stdafx.h"
#include "resource.h"
#include "scalemgr.h"
#include "ScaleBar.h"


ScaleMgr::ScaleMgr(void)
: parentFrame_(0)
, scaleBar_(*new CScaleBar())
{
}

ScaleMgr::~ScaleMgr(void)
{
	delete &scaleBar_;
}

void ScaleMgr::SetScalable(IScalable* pscalable)
{
	scaleBar_.SetScalable(pscalable);
}
//обновляем информацию о масштабе
void ScaleMgr::UpdateScaleInfo()
{
	scaleBar_.UpdateScaleInfo();
}

bool ScaleMgr::Create(CFrameWnd* pParent, DWORD id)
{
	parentFrame_ = pParent;

	if (!scaleBar_.Create(parentFrame_, id))
		return false;
	return true;
}

void ScaleMgr::enableDocking(bool enable)
{
    if(enable)
        scaleBar_.EnableDocking(CBRS_ALIGN_ANY);
    else
        scaleBar_.EnableDocking(0);
}

void ScaleMgr::dock(UINT dockBarID)
{
	parentFrame_->DockControlBar(&scaleBar_, dockBarID);
}

bool ScaleMgr::IsVisible() const{
	return (IsWindowVisible(scaleBar_.GetSafeHwnd()) == TRUE);
}

void ScaleMgr::Show() const
{
	parentFrame_->ShowControlBar(&scaleBar_, TRUE, FALSE); 
}

void ScaleMgr::Hide() const
{
	parentFrame_->ShowControlBar(&scaleBar_, FALSE, FALSE); 
}

//возвращает указатель на окно
CWnd* ScaleMgr::getWindow()
{
	return &scaleBar_;
}

CScaleBar& ScaleMgr::controlBar()
{
	return scaleBar_;
}
