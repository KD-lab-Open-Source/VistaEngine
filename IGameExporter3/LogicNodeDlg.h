#pragma once


// CLogicNodeDlg dialog

class CLogicNodeDlg : public CDialog
{
	DECLARE_DYNAMIC(CLogicNodeDlg)

public:
	CLogicNodeDlg(MAP_NODE& map_node,string dialog_name,CWnd* pParent = NULL);
	virtual ~CLogicNodeDlg();

// Dialog Data
	enum { IDD = IDD_LOGIC_NODE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void BuildTreeObject();
	void AddTreeNode(IVisNode* current,IVisNode* parent,HTREEITEM hparent);
	void SaveObjectTree(HTREEITEM hparent=TVI_ROOT);
	void ExpandAll(CTreeCtrl* tree,HTREEITEM hItem = TVI_ROOT);

	DECLARE_MESSAGE_MAP()
public:
	CTreeCtrl m_TreeVisible;
	CImageList m_ImageList;

	MAP_NODE& map_node;
	string dialog_name;

protected:
	virtual void OnOK();
public:
	virtual BOOL OnInitDialog();
};
