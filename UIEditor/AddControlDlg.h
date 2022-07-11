#pragma once
#include "afxwin.h"


// CAddControlDlg dialog

class CAddControlDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddControlDlg)

	int typeIndex_;
public:
	CAddControlDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddControlDlg();

	int getTypeIndex () {
		return typeIndex_;
	}

// Dialog Data
	enum { IDD = IDD_ADD_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CComboBox m_ctlTypeCombo;
	afx_msg void OnBnClickedOk();
protected:
	virtual void OnOK();
};
