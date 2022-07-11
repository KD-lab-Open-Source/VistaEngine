#pragma once
#include "afxcmn.h"


// CMainDlg dialog

class CMainDlg : public CDialog
{
	DECLARE_DYNAMIC(CMainDlg)

public:
	CMainDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMainDlg();

// Dialog Data
	enum { IDD = IDD_MAIN_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAnimationGroup();
	afx_msg void OnBnClickedAnimationChain();
	afx_msg void OnBnClickedAnimationGroupMat();
	afx_msg void OnBnClickedNondeleteNode();
	afx_msg void OnBnClickedDebug();
	afx_msg void OnBnClickedVisibleGroup();
	int m_MaxWeight;
	virtual BOOL OnInitDialog();
	CSpinButtonCtrl m_MaxWeightSpin;
	afx_msg void OnDeltaposMaxWeightsSpin(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	virtual void OnOK();
public:
	afx_msg void OnBnClickedLogicNode();
	afx_msg void OnBnClickedPlaceEmblem();
	afx_msg void OnBnClickedBoundNode();
	afx_msg void OnBnClickedLod1();
	afx_msg void OnBnClickedLod2();
};
