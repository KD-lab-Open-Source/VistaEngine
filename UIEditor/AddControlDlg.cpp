// AddControlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "UIEditor.h"
#include "AddControlDlg.h"

#include "..\UserInterface\UI_Types.h"
#include "ClassCreatorFactory.h"
#include "..\AttribEditor\AttribEditorCtrl.h"


// CAddControlDlg dialog

IMPLEMENT_DYNAMIC(CAddControlDlg, CDialog)
CAddControlDlg::CAddControlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddControlDlg::IDD, pParent)
{
}

CAddControlDlg::~CAddControlDlg()
{
}

void CAddControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TYPE_COMBO, m_ctlTypeCombo);
}


BEGIN_MESSAGE_MAP(CAddControlDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CAddControlDlg message handlers

BOOL CAddControlDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	const char* comboListAlt = ClassCreatorFactory<UI_ControlBase>::instance ().comboListAlt ();
	for (;;) {
		std::string token = getComboToken (comboListAlt);
		if (token.empty ())
			break;
		m_ctlTypeCombo.InsertString (-1, token.c_str ());
	}
	m_ctlTypeCombo.SetCurSel (0);

	return TRUE;
}

void CAddControlDlg::OnBnClickedOk()
{
	OnOK();
}

void CAddControlDlg::OnOK()
{
	typeIndex_ = m_ctlTypeCombo.GetCurSel ();
	CDialog::OnOK();
}
