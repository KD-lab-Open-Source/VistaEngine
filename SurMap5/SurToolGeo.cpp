// SurToolGeo.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolGeo.h"
#include "Serialization.h"


// CSurToolGeo dialog

IMPLEMENT_DYNAMIC(CSurToolGeo, CSurToolBase)
CSurToolGeo::CSurToolGeo(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)//CDialog(CSurToolGeo::IDD, pParent)
{
}

CSurToolGeo::~CSurToolGeo()
{
}

void CSurToolGeo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolGeo, CSurToolBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// CSurToolGeo message handlers

void CSurToolGeo::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	int a=7;
}

BOOL CSurToolGeo::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	m_Smpl.Create(this, IDC_SLIDER1, IDC_EDIT1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CSurToolGeo::CallBack_OperationOnMap(int x, int y)
{
	return true;
}

