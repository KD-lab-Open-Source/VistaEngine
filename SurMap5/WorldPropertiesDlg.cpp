// WorldPropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "WorldPropertiesDlg.h"
#include "..\Render\inc\TerraInterface.inl"

#include "Serialization.h"

// CWorldPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CWorldPropertiesDlg, CDialog)
CWorldPropertiesDlg::CWorldPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWorldPropertiesDlg::IDD, pParent)
	, m_strMapSize(_T(""))
{
}

CWorldPropertiesDlg::~CWorldPropertiesDlg()
{
}

void CWorldPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MAP_SIZE_EDIT, m_strMapSize);
}


BEGIN_MESSAGE_MAP(CWorldPropertiesDlg, CDialog)
END_MESSAGE_MAP()


// CWorldPropertiesDlg message handlers

BOOL CWorldPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_strMapSize.Format ("%sx%s", getEnumNameAlt (vMap.H_SIZE_POWER), getEnumNameAlt (vMap.V_SIZE_POWER));

	UpdateData (FALSE);
	return TRUE;
}
