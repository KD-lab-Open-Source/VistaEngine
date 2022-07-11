#pragma once
#include "afxcmn.h"
#include "MltiTree.h"

// CAnimationGroupDlg dialog

class CAnimationGroupDlg : public CDialog
{
	DECLARE_DYNAMIC(CAnimationGroupDlg)

public:
	CAnimationGroupDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAnimationGroupDlg();

// Dialog Data
	enum { IDD = IDD_ANIMATION_GRUP_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	void BuildTreeObject();
	void BuildTreeGroup();
	void AddTreeNode(IGameNode* cur,IGameNode* parent,HTREEITEM hparent);
	HTREEITEM  AddGroup(string sGroupName);
	void DeleteAGFromNodeMap(string ag);
	void UpdateObjectTree(HTREEITEM hobject=TVI_ROOT);
	void AddObjectInGroup(HTREEITEM hgroup,HTREEITEM hobject);
	void AddObjectInGroup(HTREEITEM hgroup,IGameNode* pGameNode);
	bool DeleteGroupObject(IGameNode* pGameNode,string group_name_);
	void ExpandAll(CTreeCtrl* tree,HTREEITEM hItem = TVI_ROOT);
protected:
	vector<string> animation_group;
	typedef map<IGameNode*,string> NODE_MAP;
	NODE_MAP node_map;

public:
	afx_msg void OnBnClickedAddGroup();
	afx_msg void OnBnClickedDelGroup();
	virtual BOOL OnInitDialog();
	CMultiTree m_ObjectTree;
	CImageList m_ImageList;
	CTreeCtrl m_GroupTree;
	CString m_NewGroupName;
	afx_msg void OnBnClickedAddObjectIntree();
protected:
	virtual void OnOK();
public:
	afx_msg void OnTvnKeydownTreeGroup(NMHDR *pNMHDR, LRESULT *pResult);
};
