#include "stdafx.h"
#include "SurMap5.h"
#include "NameComboDlg.h"

#include "Serialization.h"

IMPLEMENT_DYNAMIC(CNameComboDlg, CDialog)

CNameComboDlg::CNameComboDlg(const char* title, const char* name, const char* combo)
: CDialog(CNameComboDlg::IDD, 0)
, m_strName(name)
, m_strTitle(title)
, m_strCombo(combo)
{
}

CNameComboDlg::~CNameComboDlg()
{
}

void CNameComboDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CNameComboDlg, CDialog)
END_MESSAGE_MAP()

BOOL CNameComboDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText (m_strTitle);

	UpdateData (FALSE);

	CComboBox* combo = ((CComboBox*)GetDlgItem(IDC_NAME_COMBO));
    xassert(combo);

	ComboStrings strings;
	splitComboList(strings, static_cast<const char*>(m_strCombo));
	ComboStrings::iterator it;
	FOR_EACH(strings, it) {
		combo->InsertString (-1, it->c_str());
	}
	combo->SetWindowText(m_strName);
	combo->SetEditSel (0, -1);
	combo->SetFocus ();
	return FALSE;
}

void CNameComboDlg::OnOK()
{
    CComboBox* combo = ((CComboBox*)GetDlgItem(IDC_NAME_COMBO));
	xassert (combo);
	combo->GetWindowText(m_strName);
	CDialog::OnOK();
}
