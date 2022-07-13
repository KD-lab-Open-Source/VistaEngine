#ifndef __FORMULA_EDITOR_DLG_H_INCLUDED__
#define __FORMULA_EDITOR_DLG_H_INCLUDED__
#include "..\Util\FormulaString.h"
#include "..\Util\MFC\SizeLayoutManager.h"
#include "ParameterComboBox.h"

class CFormulaEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CFormulaEditorDlg)

public:
	CFormulaEditorDlg(const FormulaString& formula, FormulaString::LookupFunction lookupFunction, float x = 0.0f, CWnd* pParent = 0);
	virtual ~CFormulaEditorDlg();

	void updateSyntaxHighlighting ();
	void onParameterInserted(ParameterValueReference ref);

	const FormulaString& formula() const {
		return formula_;
	}

	enum { IDD = IDD_DLG_FORMULA_EDITOR };

	FormulaString::LookupFunction lookupFunction_;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
	CSizeLayoutManager layout_;
	FormulaString formula_;
	float x_value_;
public:
	virtual BOOL OnInitDialog();
	CRichEditCtrl m_ctlEdit;
	CParameterComboBox paramCombo_;
	//CComboBox m_ctlAddCombo;
	afx_msg void OnFormulaEditChange();
	afx_msg void OnInsertButtonClicked();
	afx_msg void OnInsertVariable (UINT nID);
protected:
	virtual void OnOK();
public:
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif
