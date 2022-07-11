#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "ReportCtrl.h"


// CVisibleGroupDlg dialog

class CVisibleGroupDlg : public CDialog
{
	DECLARE_DYNAMIC(CVisibleGroupDlg)

public:
	CVisibleGroupDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CVisibleGroupDlg();

// Dialog Data
	enum { IDD = IDD_VISIBLE_GROUP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	vector<AnimationVisibleGroup> animation_chain_group;
	vector<AnimationVisibleSet> animation_sets;
	AnimationVisibleGroup chain_group;
	void AddTreeNode(IVisNode* current,int aset = -1,int ag = -1);
	void AddNode(IVisNode* current,vector<IVisNode*> &groups);
	void ExpandAll(CTreeCtrl* tree,HTREEITEM hItem = TVI_ROOT);

	DECLARE_MESSAGE_MAP()
public:
	CImageList m_ImageList;

	afx_msg void OnBnClickedAddGroup();
	afx_msg void OnBnClickedDeleteGroup();
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
	void SortingList();
	CString oldName;
public:
	CTreeCtrl m_SetsTree;
	CReportCtrl m_VisibleList;
	afx_msg void OnBnClickedAddSet();
	afx_msg void OnTvnSelchangedTreeSets(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickListVisible(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBeginlabeleditTreeSets(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnEndlabeleditTreeSets(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedDeleteSet();
};
