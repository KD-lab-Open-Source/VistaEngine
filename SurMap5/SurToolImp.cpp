// SurToolImp.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolImp.h"


// CSurToolImp dialog

IMPLEMENT_DYNAMIC(CSurToolImp, CSurToolBase)
CSurToolImp::CSurToolImp(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
}

CSurToolImp::~CSurToolImp()
{
}

void CSurToolImp::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolImp, CSurToolBase)
END_MESSAGE_MAP()


// CSurToolImp message handlers

BOOL CSurToolImp::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	m_Width.Create(this, IDC_SLIDER_WIDTH, IDC_EDIT_WIDTH);
	m_Height.Create(this, IDC_SLIDER_HEIGHT, IDC_EDIT_HEIGHT);
	m_H.Create(this, IDC_SLIDER_H, IDC_EDIT_H);

	return FALSE;
}

bool CSurToolImp::CallBack_OperationOnMap(int x, int y)
{
	return true;
}

