#pragma once
#include "afxcmn.h"


// CAddChainDlg dialog

class CAddChainDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddChainDlg)

public:
	CAddChainDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddChainDlg();

// Dialog Data
	enum { IDD = IDD_ADD_CHAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Name;
	int m_nBegin;
	int m_nEnd;
	int m_nMax;
	float m_fTime;
	BOOL m_bCycled;
	virtual BOOL OnInitDialog();
	CSpinButtonCtrl m_BeginSpin;
	CSpinButtonCtrl m_EndSpin;
	CSpinButtonCtrl m_TimeSpin;
	afx_msg void OnDeltaposBeginSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposEndSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposTimeSpin(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	virtual void OnOK();
};
