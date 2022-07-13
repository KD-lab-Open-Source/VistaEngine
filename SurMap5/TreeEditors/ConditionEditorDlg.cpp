#include "stdafx.h"
#include "ConditionEditorDlg.h"

IMPLEMENT_DYNAMIC(CConditionEditorDlg, CDialog)
CConditionEditorDlg::CConditionEditorDlg(EditableCondition& condition, CWnd* pParent)
	: CDialog(CConditionEditorDlg::IDD, pParent)
    , m_ctlConditionList (&m_ctlEditor)
	, condition_ (condition)
{
}

CConditionEditorDlg::~CConditionEditorDlg()
{
}

void CConditionEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONDITION_EDITOR, m_ctlEditor);
	DDX_Control(pDX, IDC_CONDITION_LIST, m_ctlConditionList);
}


BEGIN_MESSAGE_MAP(CConditionEditorDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CConditionEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	layout_.init (this);
	layout_.add (1, 0, 1, 1, IDC_HLINE);
	layout_.add (1, 1, 0, 1, IDC_CONDITION_LIST);
	layout_.add (1, 1, 1, 1, IDC_CONDITION_EDITOR);
	layout_.add (0, 0, 1, 1, IDOK);
	layout_.add (0, 0, 1, 1, IDCANCEL);

	m_ctlEditor.setCondition(condition_);
    m_ctlConditionList.initControl ();

	return TRUE;
}

void CConditionEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	layout_.onSize(cx, cy);
}
