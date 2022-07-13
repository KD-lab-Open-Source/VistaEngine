// SurToolLighting.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolLighting.h"

#include "MainFrm.h"
#include "..\water\WaterShell.h"
#include "..\water\ZoneSource.h"

IMPLEMENT_DYNAMIC(CSurToolLighting, CSurToolBase)
CSurToolLighting::CSurToolLighting(CWnd* pParent /*=NULL*/)
	: CSurToolBase(CSurToolLighting::IDD, pParent)
{
}

CSurToolLighting::~CSurToolLighting()
{
}


void CSurToolLighting::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolLighting, CSurToolBase)
END_MESSAGE_MAP()


bool CSurToolLighting::DrawAuxData()
{
	drawCursorCircle();
	return true;
}

bool CSurToolLighting::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd ();
		float rad = pMainFrame->m_wndToolBar.getBrushRadius();
		environment->addSource(&SourceLighting(Vect3f((float)x, (float)y, 0.f), rad));
		
		CObjectsManagerDlg::ObjectChangeNotify(CObjectsManagerDlg::TAB_SOURCES);
	}
	return true;
}
