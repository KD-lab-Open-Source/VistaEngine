#ifndef __FORMATION_EDITOR_DLG_H_INCLUDED__
#define __FORMATION_EDITOR_DLG_H_INCLUDED__

#include "..\Util\MFC\SizeLayoutManager.h"

class CFormationEditor;
class FormationPattern;

class CFormationEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CFormationEditorDlg)

public:
	CFormationEditorDlg(const FormationPattern& pattern, CWnd* pParent = NULL);
	virtual ~CFormationEditorDlg();

	const class FormationPattern& pattern();

	enum { IDD = IDD_DLG_FORMATION_EDITOR };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
	CSizeLayoutManager layout_;
	CFormationEditor* formationEditor_;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	CEdit m_ctlNameEdit;
protected:
	virtual void OnOK();
public:
	afx_msg void OnFormationTypesButton();
	afx_msg void OnShowTypeNamesCheck();
};

#endif
