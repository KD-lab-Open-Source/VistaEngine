// LogicNodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "IGameExporter.h"
#include "LogicNodeDlg.h"


// CLogicNodeDlg dialog

IMPLEMENT_DYNAMIC(CLogicNodeDlg, CDialog)
CLogicNodeDlg::CLogicNodeDlg(MAP_NODE& map_node_,string dialog_name_,CWnd* pParent)
	: CDialog(CLogicNodeDlg::IDD, pParent),map_node(map_node_),dialog_name(dialog_name_)
{
}

CLogicNodeDlg::~CLogicNodeDlg()
{
}

void CLogicNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_VISIBLE, m_TreeVisible);
	if(!pDX->m_bSaveAndValidate)
	{
		SetWindowText(dialog_name.c_str());
	}
}


BEGIN_MESSAGE_MAP(CLogicNodeDlg, CDialog)
END_MESSAGE_MAP()


// CLogicNodeDlg message handlers
void CLogicNodeDlg::BuildTreeObject()
{
	IGameScene * pIgame=pRootExport->pIgame;
	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
		AddTreeNode(pGameNode,NULL,TVI_ROOT);
	}
	ExpandAll(&m_TreeVisible);
}
void CLogicNodeDlg::AddTreeNode(IGameNode* current,IGameNode* parent,HTREEITEM hparent)
{
	if(current->IsTarget())
		return;
	char* name=current->GetName();
	HTREEITEM hcurrent = m_TreeVisible.InsertItem(name,1,1,hparent);
	m_TreeVisible.SetItemData(hcurrent,(DWORD)current);

	for(int i=0;i<current->GetChildCount();i++)
	{
		IGameNode* node=current->GetNodeChild(i);
		AddTreeNode(node,current,hcurrent);
	}

	//TreeView_Expand(hTreeObject,hcurrent,TVE_EXPAND);

	MAP_NODE::iterator it=map_node.find(current);
	bool is_visible=it!=map_node.end();
	m_TreeVisible.SetCheck(hcurrent,is_visible);
}
void CLogicNodeDlg::SaveObjectTree(HTREEITEM hparent)
{
	if(hparent!=TVI_ROOT)
	{
		IGameNode* pGameNode=(IGameNode*)m_TreeVisible.GetItemData(hparent);
		string name=pGameNode->GetName();
		int checked = m_TreeVisible.GetCheck(hparent);

		if(checked)
			map_node[pGameNode]=1;
	}

	for(HTREEITEM hcur = m_TreeVisible.GetChildItem(hparent);hcur;
		hcur = m_TreeVisible.GetNextItem(hcur,TVGN_NEXT))
	{
		SaveObjectTree(hcur);
	}
}

void CLogicNodeDlg::OnOK()
{
	map_node.clear();
	SaveObjectTree();
	CDialog::OnOK();
}

BOOL CLogicNodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ImageList.Create(IDB_TREE_IMAGE,16,4,RGB(255,255,255));
	m_TreeVisible.SetImageList(&m_ImageList,TVSIL_NORMAL);
	m_TreeVisible.SetBkColor(RGB(255,255,255));

	BuildTreeObject();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogicNodeDlg::ExpandAll(CTreeCtrl* tree,HTREEITEM hItem)
{
	for(HTREEITEM hcur = tree->GetChildItem(hItem);hcur;
		hcur = tree->GetNextItem(hcur,TVGN_NEXT))
	{
		ExpandAll(tree,hcur);
	}
	tree->Expand(hItem,TVE_EXPAND);
}

