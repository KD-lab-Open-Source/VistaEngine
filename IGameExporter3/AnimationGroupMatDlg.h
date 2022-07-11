#pragma once
#include "afxcmn.h"


// CAnimationGroupMatDlg dialog

class CAnimationGroupMatDlg : public CDialog
{
	DECLARE_DYNAMIC(CAnimationGroupMatDlg)

public:
	CAnimationGroupMatDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAnimationGroupMatDlg();

// Dialog Data
	enum { IDD = IDD_ANIMATION_GRUP_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	void BuildTreeObject();
	void BuildTreeGroup();
	void AddTreeNode(IVisMaterial* current,IVisMaterial* parent,HTREEITEM hparent);
	HTREEITEM  AddGroup(string sGroupName);
	void DeleteAGFromNodeMap(string ag);
	void UpdateObjectTree(HTREEITEM hobject=TVI_ROOT);
	void AddObjectInGroup(HTREEITEM hgroup,HTREEITEM hobject);
	void AddObjectInGroup(HTREEITEM hGroup,IVisMaterial* pGameNode);
	bool DeleteGroupObject(IVisMaterial* pGameNode,string group_name_);
	void ExpandAll(CTreeCtrl* tree,HTREEITEM hItem = TVI_ROOT);
protected:
	vector<string> animation_group;
	typedef map<IVisMaterial*,string> NODE_MAP;
	NODE_MAP node_map;

public:
	afx_msg void OnBnClickedAddGroup();
	afx_msg void OnBnClickedDelGroup();
	virtual BOOL OnInitDialog();
	CTreeCtrl m_ObjectTree;
	CImageList m_ImageList;
	CTreeCtrl m_GroupTree;
	CString m_NewGroupName;
	afx_msg void OnBnClickedAddObjectIntree();
protected:
	virtual void OnOK();
};
