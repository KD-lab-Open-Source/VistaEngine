#pragma once

#include "FormationEditor.h"
#include "..\Util\MFC\SizeLayoutManager.h"
#include "afxcmn.h"

class CFormationsEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CFormationsEditorDlg)

public:
	CFormationsEditorDlg(CWnd* pParent = NULL);
	virtual ~CFormationsEditorDlg();

	void updateFormationsList ();
	void storePattern ();

	enum { IDD = IDD_DLG_FORMATIONS_EDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
	CFormationEditor m_ctlFormationEditor;
	CSizeLayoutManager m_Layout;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	CTreeCtrl m_ctlFormationsTree;
	afx_msg void OnFormationsTreeSelChanged(NMHDR *pNMHDR, LRESULT *pResult);
	CString m_strName;
	afx_msg void OnNameEditChange();
protected:
	virtual void OnOK();
public:
	afx_msg void OnTypesButtonClicked();
	afx_msg void OnAddButtonClicked();
	afx_msg void OnRemoveButtonClicked();
};
