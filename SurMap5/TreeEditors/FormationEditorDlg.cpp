#include "stdafx.h"
#include ".\FormationEditorDlg.h"
#include "FormationEditor.h"
#include "AttributeSquad.h"

// CFormationEditorDlg dialog

IMPLEMENT_DYNAMIC(CFormationEditorDlg, CDialog)
CFormationEditorDlg::CFormationEditorDlg(const FormationPattern& pattern, CWnd* pParent /*=NULL*/)
: CDialog(CFormationEditorDlg::IDD, pParent)
, formationEditor_(new CFormationEditor)
{
	formationEditor_->setPattern (pattern);
}

CFormationEditorDlg::~CFormationEditorDlg()
{
	delete formationEditor_;
}

void CFormationEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FORMATION_EDITOR, *formationEditor_);
	DDX_Control(pDX, IDC_NAME_EDIT, m_ctlNameEdit);
}

const FormationPattern& CFormationEditorDlg::pattern ()
{
	return formationEditor_->pattern();
}

BEGIN_MESSAGE_MAP(CFormationEditorDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_TYPES_BUTTON, OnFormationTypesButton)
	ON_BN_CLICKED(IDC_SHOW_TYPE_NAMES, OnShowTypeNamesCheck)
END_MESSAGE_MAP()


// CFormationEditorDlg message handlers

void CFormationEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (!layout_.isInitialized ()) {
		layout_.init(this);
		layout_.add(1, 1, 1, 0, IDC_NAME_EDIT);
		layout_.add(0, 1, 1, 0, IDC_TYPES_BUTTON);
		layout_.add(1, 1, 1, 1, IDC_FORMATION_EDITOR);
		layout_.add(1, 0, 1, 1, IDC_H_LINE);
		layout_.add(1, 0, 0, 1, IDC_SHOW_TYPE_NAMES);
		layout_.add(0, 0, 1, 1, IDOK);
		layout_.add(0, 0, 1, 1, IDCANCEL);
	}
	layout_.onSize (cx, cy);
	((CButton*)GetDlgItem(IDC_SHOW_TYPE_NAMES))->SetCheck(formationEditor_->showTypeNames());
	RedrawWindow (0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
}

BOOL CFormationEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctlNameEdit.SetWindowText (pattern().c_str());
	m_ctlNameEdit.SetFocus ();
	m_ctlNameEdit.SetSel (0, -1);

	return FALSE;
}

void CFormationEditorDlg::OnOK()
{
	CString str;
	m_ctlNameEdit.GetWindowText(str);
	formationEditor_->setPatternName (str);

	FormationPatterns::instance().saveLibrary();

	CDialog::OnOK();
}

void CFormationEditorDlg::OnFormationTypesButton()
{
	if(UnitFormationTypes::instance().editLibrary()) 
		formationEditor_->Invalidate ();
}

void CFormationEditorDlg::OnShowTypeNamesCheck()
{
	bool show = ((CButton*)GetDlgItem(IDC_SHOW_TYPE_NAMES))->GetCheck();
	formationEditor_->setShowTypeNames(show);
	formationEditor_->Invalidate(FALSE);
}
