#pragma once


// CBaseTreeDlg dialog

class CBaseTreeDlg : public CDialog
{
	DECLARE_DYNAMIC(CBaseTreeDlg)

public:
	CBaseTreeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBaseTreeDlg();
	bool flag_init_dialog;

// Dialog Data
	enum { IDD = IDD_BASE_TREE_VIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
};
