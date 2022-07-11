// AddGroup.cpp : implementation file
//

#include "stdafx.h"
#include "AddGroup.h"


// CAddGroup dialog

IMPLEMENT_DYNAMIC(CAddGroup, CDialog)
CAddGroup::CAddGroup(CWnd* pParent /*=NULL*/)
	: CDialog(CAddGroup::IDD, pParent)
	, m_Name(_T(""))
{
}

CAddGroup::~CAddGroup()
{
}

void CAddGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_Name);
}


BEGIN_MESSAGE_MAP(CAddGroup, CDialog)
END_MESSAGE_MAP()


// CAddGroup message handlers

void CAddGroup::OnOK()
{
	UpdateData();
	CDialog::OnOK();
}
