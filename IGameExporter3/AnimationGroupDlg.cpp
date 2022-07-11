// AnimationGroupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AnimationGroupDlg.h"
#include "resource.h"
//===================================================================================================================
bool TestAnimationGroupN(vector<AnimationGroup>& animation_group)
{
	typedef map<IVisNode*,int> NODE_MAP;
	NODE_MAP node_map;
	bool found=false;
	for(int iag=0;iag<animation_group.size();iag++)
	{
		AnimationGroup& ag=animation_group[iag];
		for(int inode=0;inode<ag.groups.size();inode++)
		{
			IVisNode* node=ag.groups[inode];
			NODE_MAP::iterator it=node_map.find(node);
			if(it!=node_map.end())
			{
				IVisNode* cur_node=it->first;
				const char* name=cur_node->GetName();
				int group=it->second;
				//xassert(0);
				found=true;
				Msg("Есть несколько Node с одинаковым именем: %s\n",name);
			}
			node_map[node]=iag;
		}
	}
	return found;
}
//===================================================================================================================
bool TestAnimationGroupM(vector<AnimationGroup>& animation_group)
{
	typedef map<IVisMaterial*,int> NODE_MAP;
	NODE_MAP node_map;
	bool found=false;
	for(int iag=0;iag<animation_group.size();iag++)
	{
		AnimationGroup& ag=animation_group[iag];
		for(int inode=0;inode<ag.materials.size();inode++)
		{
			IVisMaterial* node=ag.materials[inode];
			NODE_MAP::iterator it=node_map.find(node);
			if(it!=node_map.end())
			{
				IVisMaterial* cur_node=it->first;
				const char* name=cur_node->GetName();
				int group=it->second;
				//xassert(0);
				found=true;
				Msg("Есть несколько материалов с одинаковым именем: %s\n",name);
			}
			node_map[node]=iag;
		}
	}

	return found;
}
//===================================================================================================================
void TestAnimationGroup(vector<AnimationGroup>& animation_group,HWND m_hWnd)
{
	bool f1=TestAnimationGroupN(animation_group);
	bool f2=TestAnimationGroupM(animation_group);
	if(f1 || f2)
		ShowConsole(m_hWnd);
}
//===================================================================================================================
IMPLEMENT_DYNAMIC(CAnimationGroupDlg, CDialog)
CAnimationGroupDlg::CAnimationGroupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAnimationGroupDlg::IDD, pParent)
	, m_NewGroupName(_T(""))
{
}
//===================================================================================================================
CAnimationGroupDlg::~CAnimationGroupDlg()
{
}
//===================================================================================================================
void CAnimationGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OBJECT_TREE, m_ObjectTree);
	DDX_Control(pDX, IDC_TREE_GROUP, m_GroupTree);
	DDX_Text(pDX, IDC_NEW_GROUP_NAME, m_NewGroupName);
}
//===================================================================================================================
BEGIN_MESSAGE_MAP(CAnimationGroupDlg, CDialog)
	ON_BN_CLICKED(IDC_ADD_GROUP, OnBnClickedAddGroup)
	ON_BN_CLICKED(IDC_DEL_GROUP, OnBnClickedDelGroup)
	ON_BN_CLICKED(IDC_ADD_OBJECT_INTREE, OnBnClickedAddObjectIntree)
	ON_NOTIFY(TVN_KEYDOWN, IDC_TREE_GROUP, OnTvnKeydownTreeGroup)
	ON_WM_DESTROY()
END_MESSAGE_MAP()
//===================================================================================================================
void CAnimationGroupDlg::OnBnClickedAddGroup()
{
	UpdateData();
	if(m_NewGroupName.IsEmpty())
		return;

	AddGroup(TrimString((LPCSTR)m_NewGroupName));
	m_NewGroupName.Empty();
	UpdateData(FALSE);
}
//===================================================================================================================
HTREEITEM  CAnimationGroupDlg::AddGroup(string sGroupName)
{
	for(int i=0;i<animation_group.size();i++)
	{
		string& s=animation_group[i];
		if(s==sGroupName)
		{
			MessageBox("Такое имя группы уже существует","Error",MB_OK|MB_ICONSTOP);
			return NULL;
		}
	}

	string group;
	group=sGroupName;
	animation_group.push_back(group);
	return m_GroupTree.InsertItem(sGroupName.c_str(),0,0);
}
//===================================================================================================================
void CAnimationGroupDlg::OnBnClickedDelGroup()
{
	HTREEITEM hGroup = m_GroupTree.GetSelectedItem();
	if(hGroup == NULL)
		return;

	CString name = m_GroupTree.GetItemText(hGroup);
	for(int igroup=0;igroup<animation_group.size();igroup++)
	{
		string& ag=animation_group[igroup];
		if((LPCSTR)name==ag)
		{
			m_GroupTree.DeleteItem(hGroup);
			animation_group.erase(animation_group.begin()+igroup);
			DeleteAGFromNodeMap((LPCSTR)name);
			UpdateObjectTree();
			return;
		}
	}

}
//===================================================================================================================
void CAnimationGroupDlg::DeleteAGFromNodeMap(string ag)
{
	NODE_MAP new_node_map;
	NODE_MAP::iterator it;
	FOR_EACH(node_map,it)
	{
		if(it->second!=ag)
		{
			new_node_map[it->first]=it->second;
		}
	}

	node_map=new_node_map;
}
//===================================================================================================================
BOOL CAnimationGroupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ImageList.Create(IDB_TREE_IMAGE,16,4,RGB(255,255,255));
	m_ObjectTree.SetImageList(&m_ImageList,TVSIL_NORMAL);
	m_ObjectTree.SetBkColor(RGB(255,255,255));
	m_GroupTree.SetImageList(&m_ImageList,TVSIL_NORMAL);
	m_GroupTree.SetBkColor(RGB(255,255,255));

	animation_group.resize(pRootExport->animation_data.animation_group.size());
	for(int iag=0;iag<animation_group.size();iag++)
	{
		AnimationGroup& ag=pRootExport->animation_data.animation_group[iag];
		animation_group[iag]=ag.name;
		for(int inode=0;inode<ag.groups.size();inode++)
		{
			IVisNode* node=ag.groups[inode];
			NODE_MAP::iterator it=node_map.find(node);
			if(it!=node_map.end())
			{
				IVisNode* cur_node=it->first;
				const char* name=cur_node->GetName();
				string group=it->second;
				xassert(0);
			}
			node_map[node]=ag.name;
		}
	}

	TestAnimationGroup(pRootExport->animation_data.animation_group,NULL);
	BuildTreeObject();
	BuildTreeGroup();

	return TRUE;  
}
//===================================================================================================================
void CAnimationGroupDlg::BuildTreeGroup()
{
	vector<AnimationGroup>& animation_group=pRootExport->animation_data.animation_group;
	for(int igroup=0;igroup<animation_group.size();igroup++)
	{
		AnimationGroup& ag=animation_group[igroup];
		HTREEITEM hgroup = m_GroupTree.InsertItem(ag.name.c_str(),0,0,TVI_ROOT);
		if(hgroup)
		{
			for(int iobject=0;iobject<ag.groups.size();iobject++)
			{
				string object_name=ag.groups[iobject]->GetName();

				m_GroupTree.InsertItem(object_name.c_str(),1,1,hgroup);
				m_GroupTree.Expand(hgroup,TVE_EXPAND);
				//TreeView_Expand(hTreeGroup,hgroup,TVE_EXPAND);
			}
		}
	}
	UpdateObjectTree();
}
//===================================================================================================================
void CAnimationGroupDlg::BuildTreeObject()
{
	IVisExporter * pIgame=pRootExport->pIgame;
	for(int loop = 0; loop <pIgame->GetRootNodeCount();loop++)
	{
		IVisNode * pGameNode = pIgame->GetRootNode(loop);
		AddTreeNode(pGameNode,NULL,TVI_ROOT);
	}
	ExpandAll(&m_ObjectTree,TVI_ROOT);
}
//===================================================================================================================
void CAnimationGroupDlg::AddTreeNode(IVisNode* current,IVisNode* parent,HTREEITEM hparent)
{
	if(current->IsTarget())
		return;
	const char* name=current->GetName();
	HTREEITEM hcurrent=m_ObjectTree.InsertItem(name,1,1,hparent);
	m_ObjectTree.SetItemData(hcurrent,(DWORD)current);
	for(int i=0;i<current->GetChildNodeCount();i++)
	{
		IVisNode* node=current->GetChildNode(i);
		AddTreeNode(node,current,hcurrent);
	}

	//if (m_GroupTree.ItemHasChildren(hcurrent))
	//	m_GroupTree.Expand(hcurrent,TVE_EXPAND);
}
//===================================================================================================================
void CAnimationGroupDlg::OnBnClickedAddObjectIntree()
{
	HTREEITEM hObject = m_ObjectTree.GetSelectedItem();
	if(hObject==NULL)
	{
		MessageBox("Объект не выделен","Error",MB_OK|MB_ICONSTOP);
		return;
	}


	HTREEITEM hGroup = m_GroupTree.GetSelectedItem();
	if(hGroup==NULL)
		return;

	CTreeItemList itemList;
	POSITION pos;
	m_ObjectTree.GetSelectedList(itemList);

	for (pos = itemList.GetHeadPosition(); pos != NULL;)
	{
		AddObjectInGroup(hGroup,itemList.GetNext(pos));
		
	}

	UpdateObjectTree();
}
//===================================================================================================================
void CAnimationGroupDlg::AddObjectInGroup(HTREEITEM hGroup,HTREEITEM hObject)
{
	IVisNode* pGameNode=(IVisNode*)m_ObjectTree.GetItemData(hObject);
	AddObjectInGroup(hGroup,pGameNode);
}
//===================================================================================================================
void CAnimationGroupDlg::AddObjectInGroup(HTREEITEM hGroup,IVisNode* pGameNode)
{
	{//Добраться до root
		HTREEITEM hParent=hGroup;
		do
		{
			hGroup = hParent;
			hParent = m_GroupTree.GetParentItem(hParent);
		}while(hParent!=NULL);
	}

	//
	string group_name = (LPCSTR)m_GroupTree.GetItemText(hGroup);
	string object_name = pGameNode->GetName();

	NODE_MAP::iterator it=node_map.find(pGameNode);
	bool found=it!=node_map.end();
	if(found)
	{
		string group=it->second;
		found=group_name==group;
		if(!found)
		{
			DeleteGroupObject(pGameNode,group);
		}
	}

	if(!found)
	{
		m_GroupTree.InsertItem(object_name.c_str(),1,1,hGroup);
		node_map[pGameNode]=group_name;
	}
	ExpandAll(&m_GroupTree);
}
//===================================================================================================================
bool CAnimationGroupDlg::DeleteGroupObject(IVisNode* pGameNode,string group_name_)
{
	string object_name = pGameNode->GetName();
	for(HTREEITEM hgroup = m_GroupTree.GetChildItem(TVI_ROOT);hgroup;
		hgroup = m_GroupTree.GetNextItem(hgroup,TVGN_NEXT))
	{
		string group_name = (LPCSTR)m_GroupTree.GetItemText(hgroup);
		if(group_name == group_name_)
		{
			for(HTREEITEM hobject = m_GroupTree.GetChildItem(hgroup);hobject;
				hobject = m_GroupTree.GetNextItem(hobject,TVGN_NEXT))
			{
				string obj_name = (LPCSTR)m_GroupTree.GetItemText(hobject);
				if(object_name == obj_name)
				{
					BOOL b = m_GroupTree.DeleteItem(hobject);
					xassert(b);
					return b;
				}
			}
			break;
		}
	}

	return false;
}
//===================================================================================================================
void CAnimationGroupDlg::UpdateObjectTree(HTREEITEM hparent)
{
	if(hparent!=TVI_ROOT)
	{
		IVisNode* pGameNode=(IVisNode*)m_ObjectTree.GetItemData(hparent);
		string group_name;
		NODE_MAP::iterator it=node_map.find(pGameNode);
		if(it!=node_map.end())
			group_name=it->second;


		string name=pGameNode->GetName();

		if(!group_name.empty())
		{
			name+=" (";
			name+=group_name;
			name+=")";
		}

		TVITEM item;
		item.mask=TVIF_HANDLE|TVIF_TEXT;
		item.hItem=hparent;
		item.pszText=(char*)name.c_str();
		item.cchTextMax =name.size();
		m_ObjectTree.SetItem(&item);
	}

	for(HTREEITEM hcur = m_ObjectTree.GetChildItem(hparent);hcur;
		hcur = m_ObjectTree.GetNextItem(hcur,TVGN_NEXT))
	{
		UpdateObjectTree(hcur);
	}
}
//===================================================================================================================
void CAnimationGroupDlg::OnOK()
{
	vector<AnimationGroup> animation_group_store=pRootExport->animation_data.animation_group;
	pRootExport->animation_data.animation_group.resize(animation_group.size());
	for(int iag=0;iag<animation_group.size();iag++)
	{
		AnimationGroup& ag=pRootExport->animation_data.animation_group[iag];
		ag.name=animation_group[iag];
		ag.groups.clear();

		for(int istore=0;istore<animation_group_store.size();istore++)
		{
			AnimationGroup& s=animation_group_store[istore];
			if(s.name==ag.name)
			{
				ag.materials=s.materials;
				break;
			}
		}
	}

	NODE_MAP::iterator it;
	FOR_EACH(node_map,it)
	{
		IVisNode* cur_node=it->first;
		string group=it->second;
		for(int iag=0;iag<animation_group.size();iag++)
		{
			AnimationGroup& ag=pRootExport->animation_data.animation_group[iag];
			if(animation_group[iag]==group)
			{
				ag.groups.push_back(cur_node);
			}
		}
	}

	TestAnimationGroup(pRootExport->animation_data.animation_group,NULL);

	CDialog::OnOK();
}
//===================================================================================================================
void CAnimationGroupDlg::ExpandAll(CTreeCtrl* tree,HTREEITEM hItem)
{
	for(HTREEITEM hcur = tree->GetChildItem(hItem);hcur;
		hcur = tree->GetNextItem(hcur,TVGN_NEXT))
	{
		ExpandAll(tree,hcur);
	}
	tree->Expand(hItem,TVE_EXPAND);
}
//===================================================================================================================

void CAnimationGroupDlg::OnTvnKeydownTreeGroup(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	//if(pTVKeyDown->wVKey == VK_DELETE)
	//{
	//	HTREEITEM hItem = m_GroupTree.GetSelectedItem();
	//	if(hItem == NULL)
	//		return;

	//	HTREEITEM hGroup = m_GroupTree.GetParentItem(hItem);
	//	if (hGroup == NULL)
	//	{
	//		hGroup == hItem;

	//		CString name = m_GroupTree.GetItemText(hGroup);
	//		for(int igroup=0;igroup<animation_group.size();igroup++)
	//		{
	//			string& ag=animation_group[igroup];
	//			if((LPCSTR)name==ag)
	//			{
	//				m_GroupTree.DeleteItem(hGroup);
	//				animation_group.erase(animation_group.begin()+igroup);
	//				DeleteAGFromNodeMap((LPCSTR)name);
	//				UpdateObjectTree();
	//				return;
	//			}
	//		}
	//	}
	//}
	*pResult = 0;
}

void CAnimationGroupDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here
}
