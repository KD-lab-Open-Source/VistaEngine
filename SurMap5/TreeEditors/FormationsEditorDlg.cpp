#include "stdafx.h"
#include "FormationsEditorDlg.h"
#include "EditArchive.h"
#include "XPrmArchive.h"
#include ".\formationseditordlg.h"
#include "TypeLibraryImpl.h"

IMPLEMENT_DYNAMIC(CFormationsEditorDlg, CDialog)
CFormationsEditorDlg::CFormationsEditorDlg(CWnd* pParent)
	: CDialog(CFormationsEditorDlg::IDD, pParent)
	, m_strName(_T(""))
{
}

CFormationsEditorDlg::~CFormationsEditorDlg()
{
}

void CFormationsEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FORMATION_EDITOR, m_ctlFormationEditor);
	DDX_Control(pDX, IDC_FORMATIONS_TREE, m_ctlFormationsTree);
	DDX_Text(pDX, IDC_NAME_EDIT, m_strName);
}


BEGIN_MESSAGE_MAP(CFormationsEditorDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_FORMATIONS_TREE, OnFormationsTreeSelChanged)
	ON_EN_CHANGE(IDC_NAME_EDIT, OnNameEditChange)
	ON_BN_CLICKED(IDC_TYPES_BUTTON, OnTypesButtonClicked)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButtonClicked)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButtonClicked)
END_MESSAGE_MAP()

void CFormationsEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	m_Layout.onSize (cx, cy);
	RedrawWindow (0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
}

void CFormationsEditorDlg::updateFormationsList ()
{
	if (m_ctlFormationsTree.GetCount ()) {
		m_ctlFormationsTree.DeleteAllItems ();
	}

	const FormationPatterns::Strings& strings = FormationPatterns::instance().strings();
	FormationPatterns::Strings::const_iterator it;
	
	FOR_EACH (strings, it) {
		m_ctlFormationsTree.InsertItem (it->c_str());
	}
	/// TODO: жуткий хинт, выяснить отчего глюки с прорисовкой!!!
	CRect rt;
	m_ctlFormationsTree.GetWindowRect (&rt);
	ScreenToClient (&rt);
	rt.bottom += 1;
	m_ctlFormationsTree.MoveWindow (&rt, TRUE);
	rt.bottom -= 1;
	m_ctlFormationsTree.MoveWindow (&rt, TRUE);
}

BOOL CFormationsEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	updateFormationsList ();

	CRect rt;
	GetClientRect (&rt);

	if (!m_Layout.isInitialized ()) {
		m_Layout.init(this);
		m_Layout.add(1, 1, 1, 0, IDC_NAME_EDIT);
		m_Layout.add(0, 1, 1, 0, IDC_TYPES_BUTTON);
		m_Layout.add(1, 1, 1, 1, IDC_FORMATION_EDITOR);
		m_Layout.add(1, 1, 0, 1, IDC_FORMATIONS_TREE);
		m_Layout.add(1, 0, 1, 1, IDC_H_LINE);
		m_Layout.add(1, 0, 0, 1, IDC_ADD_BUTTON);
		m_Layout.add(1, 0, 0, 1, IDC_REMOVE_BUTTON);
		m_Layout.add(0, 0, 1, 1, IDOK);
	} 
	return TRUE;
}

void CFormationsEditorDlg::OnFormationsTreeSelChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM new_item = pNMTreeView->itemNew.hItem;
	HTREEITEM old_item = pNMTreeView->itemOld.hItem;

	if (old_item) {
		FormationPattern& pattern = const_cast<FormationPattern&>(*FormationPatterns::instance().find (m_ctlFormationsTree.GetItemText (old_item)));
		pattern = m_ctlFormationEditor.pattern();
	}
	if (new_item) {
		CString text = m_ctlFormationsTree.GetItemText (new_item);
		const FormationPattern& pattern = *FormationPatterns::instance().find(text);
		m_ctlFormationEditor.setPattern (pattern);
		m_strName = text;
		m_ctlFormationEditor.setPatternName (text);
		m_ctlFormationEditor.Invalidate ();
		UpdateData (FALSE);
	}
	*pResult = 0;
}

void CFormationsEditorDlg::OnNameEditChange()
{
	UpdateData (TRUE);
	m_ctlFormationEditor.setPatternName (m_strName);
	HTREEITEM item = m_ctlFormationsTree.GetSelectedItem();
	const_cast<FormationPattern&>(*FormationPatterns::instance().find (m_ctlFormationsTree.GetItemText (item))) = m_ctlFormationEditor.pattern();
	if (item) {
		m_ctlFormationsTree.SetItemText (item, m_strName);
	}
}

void CFormationsEditorDlg::OnOK()
{
	storePattern ();
	CDialog::OnOK();
}

void CFormationsEditorDlg::OnTypesButtonClicked()
{
	SingletonPrm<UnitFormationTypes>::edit();
	m_ctlFormationEditor.Invalidate ();
}

void CFormationsEditorDlg::OnRemoveButtonClicked()
{
	if (HTREEITEM item = m_ctlFormationsTree.GetSelectedItem()) {
		FormationPatterns::instance().remove (m_ctlFormationsTree.GetItemText (item));
		updateFormationsList ();
	}
}

void CFormationsEditorDlg::storePattern ()
{
	HTREEITEM selection = m_ctlFormationsTree.GetSelectedItem();
	if (selection) {
		CString str = m_ctlFormationsTree.GetItemText (selection);
		FormationPattern& pattern = const_cast<FormationPattern&>(*FormationPatterns::instance().find (str));
		pattern = m_ctlFormationEditor.pattern();
	}
}

void CFormationsEditorDlg::OnAddButtonClicked()
{
	storePattern ();

	CString name_base = Dictionary::instance().translate("Новая формация");
	CString name = name_base;

	int index = 1;
	while (FormationPatterns::instance().find (name) != FormationPatterns::Reference()) {
		name.Format ("%s%i", name_base, index);
		index++;
	}
	FormationPatterns::instance().add (name);

    updateFormationsList ();

	m_ctlFormationEditor.Invalidate ();
	UpdateData (FALSE);
}
