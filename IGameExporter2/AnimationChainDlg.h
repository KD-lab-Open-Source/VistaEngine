#pragma once
#include "afxcmn.h"
#include "ReportCtrl.h"


// CAnimationChainDlg dialog

class CAnimationChainDlg : public CDialog
{
	DECLARE_DYNAMIC(CAnimationChainDlg)

public:
	CAnimationChainDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAnimationChainDlg();

// Dialog Data
	enum { IDD = IDD_ANIMATION_CHAIN_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void SetItem(int index,bool insert);

	DECLARE_MESSAGE_MAP()
protected:
	vector<AnimationChain> animation_chain;
	vector<AnimationVisibleGroup> animation_chain_group;

	enum  
	{
		LS_NAME,
		LS_BEGIN,
		LS_END,
		LS_TIME,
		LS_CYCLED
	};
	CImageList m_ImageList;
public:
	virtual BOOL OnInitDialog();
	CReportCtrl m_ListChain;
	afx_msg void OnBnClickedAddChain();
	afx_msg void OnBnClickedDeleteChain();
	afx_msg void OnBnClickedEditChain();
	afx_msg void OnHdnItemdblclickListChains(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListChains(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	virtual void OnOK();
};
