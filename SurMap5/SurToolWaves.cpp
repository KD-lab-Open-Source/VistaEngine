#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolWaves.h"
#include "..\Game\RenderObjects.h"
#include "..\Units\ExternalShow.h"

#include "MainFrame.h"
#include ".\surtoolwaves.h"

IMPLEMENT_DYNAMIC(CSurToolWaves, CSurToolBase)

CSurToolWaves::CSurToolWaves(CWnd* pParent /*=NULL*/)
: CSurToolBase(getIDD(), pParent)
{
	leftButtonDown_ = false;
	waveDlg_ = NULL;
}

CSurToolWaves::~CSurToolWaves()
{
}

void CSurToolWaves::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSurToolWaves, CSurToolBase)
	ON_WM_DESTROY()
END_MESSAGE_MAP()
bool CSurToolWaves::CallBack_LMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	leftButtonDown_ = true;
	waveDlg_->OnMapLBClick(worldCoord);
	return true;
}

bool CSurToolWaves::CallBack_LMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	leftButtonDown_ = false;
	return true;
}
bool CSurToolWaves::CallBack_RMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	waveDlg_->OnMapRBClick(worldCoord);
	return true;
}
bool CSurToolWaves::CallBack_TrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	if (leftButtonDown_)
		waveDlg_->OnMapMouseMove(worldCoord);
	return true;
}

bool CSurToolWaves::CallBack_DrawAuxData(void)
{
	return true;
}
BOOL CSurToolWaves::OnInitDialog()
{
	if(environment){
		CSurToolBase::OnInitDialog();
		
		waveDlg_ = &((CMainFrame*)AfxGetMainWnd())->waveDialog();
		waveDlg_->ShowWindow(SW_RESTORE);
		waveDlg_->UpdateWindow();
		waveDlg_->RefreshList();
	}
	return FALSE;
}
void CSurToolWaves::OnDestroy()
{
	CSurToolBase::OnDestroy();
}

