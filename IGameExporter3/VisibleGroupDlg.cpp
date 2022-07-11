// VisibleGroupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VisibleGroupDlg.h"
#include ".\visiblegroupdlg.h"
#include "AddGroup.h"


bool IsNodeMesh(IVisNode* node);

// CVisibleGroupDlg dialog

IMPLEMENT_DYNAMIC(CVisibleGroupDlg, CDialog)
CVisibleGroupDlg::CVisibleGroupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVisibleGroupDlg::IDD, pParent)
{
}

CVisibleGroupDlg::~CVisibleGroupDlg()
{
}

void CVisibleGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_SETS, m_SetsTree);
	DDX_Control(pDX, IDC_LIST_VISIBLE, m_VisibleList);
}


BEGIN_MESSAGE_MAP(CVisibleGroupDlg, CDialog)
	ON_BN_CLICKED(IDC_ADD_GROUP, OnBnClickedAddGroup)
	ON_BN_CLICKED(IDC_DELETE_GROUP, OnBnClickedDeleteGroup)
	ON_BN_CLICKED(IDC_ADD_SET, OnBnClickedAddSet)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_SETS, OnTvnSelchangedTreeSets)
	ON_NOTIFY(NM_CLICK, IDC_LIST_VISIBLE, OnNMClickListVisible)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE_SETS, OnTvnBeginlabeleditTreeSets)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE_SETS, OnTvnEndlabeleditTreeSets)
	ON_BN_CLICKED(IDC_DELETE_SET, OnBnClickedDeleteSet)
END_MESSAGE_MAP()


// CVisibleGroupDlg message handlers

static int CALLBACK 
SortingProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// lParamSort contains a pointer to the list view control.
	CListCtrl* pListCtrl = (CListCtrl*) lParamSort;
	CString    strItem1 = pListCtrl->GetItemText(lParam1, 0);
	CString    strItem2 = pListCtrl->GetItemText(lParam2, 0);

	return strcmp(strItem2, strItem1);
}
void CVisibleGroupDlg::SortingList()
{
	//m_VisibleList.SortItems(SortingProc,(LPARAM)&m_VisibleList);
}

void CVisibleGroupDlg::OnBnClickedAddGroup()
{
	HTREEITEM hitem = m_SetsTree.GetSelectedItem();
	if (!hitem)
	{
		MessageBox("Не выбрано множество в которое необходимо добавить группу","Error!!!",MB_OK|MB_ICONSTOP);
		return;
	}
	
	if(m_SetsTree.GetParentItem(hitem) != NULL)
		hitem = m_SetsTree.GetParentItem(hitem);
	
	CString Name = m_SetsTree.GetItemText(hitem);
	int i;
	for(i=0; i<animation_sets.size();i++)
	{
		if (animation_sets[i].name == (LPCSTR)Name)
		{
			break;
		}
	}

	if(i >= animation_sets.size())
		return;

	AnimationVisibleSet& avs = animation_sets[i];

	CAddGroup dlg;
	dlg.m_Name="NewGroup";
	if(dlg.DoModal()!=IDOK)
		return;
	
	string name = TrimString((LPCSTR)dlg.m_Name);
	for(i=0;i<avs.animation_visible_groups.size();i++)
	{
		AnimationVisibleGroup& ag=avs.animation_visible_groups[i];
		if(ag.name == name)
		{
			MessageBox("Такая группа уже существует","Error",MB_OK|MB_ICONSTOP);
			return;
		}
	}

	AnimationVisibleGroup ag;
	ag.name = name;
	avs.animation_visible_groups.push_back(ag);
	hitem = m_SetsTree.InsertItem(ag.name.c_str(),4,4,hitem);
	m_SetsTree.EnsureVisible(hitem);
}


void CVisibleGroupDlg::OnBnClickedDeleteGroup()
{
	HTREEITEM hitem = m_SetsTree.GetSelectedItem();
	if(!hitem)
		return;
	if (m_SetsTree.GetParentItem(hitem) == NULL)
		return;

	if (MessageBox("Вы уверены, что хотите удалить группу?","Warning!!!",MB_ICONQUESTION|MB_YESNO) == IDNO)
		return;

	string sname = (LPCSTR)m_SetsTree.GetItemText(m_SetsTree.GetParentItem(hitem));
	string name = (LPCSTR)m_SetsTree.GetItemText(hitem);

	for(int i=0; i<animation_sets.size(); i++)
	{
		if (animation_sets[i].name == sname)
		{
			for(int j=0; j<animation_sets[i].animation_visible_groups.size(); j++)
			{
				if (animation_sets[i].animation_visible_groups[j].name == name) 
				{
					animation_sets[i].animation_visible_groups.erase(animation_sets[i].animation_visible_groups.begin()+j);
					break;
				}
			}
		}
	}

	m_SetsTree.DeleteItem(hitem);
	if(m_SetsTree.GetCount() == 0)
		m_VisibleList.DeleteAllItems();
}

BOOL CVisibleGroupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ImageList.Create(IDB_TREE_IMAGE,16,4,RGB(255,255,255));
	DWORD style = m_VisibleList.GetExtendedStyle();
	style |= LVS_EX_FULLROWSELECT; 
	m_VisibleList.SetExtendedStyle(style);
	m_VisibleList.SetImageList(&m_ImageList);
	m_VisibleList.InsertColumn(0,"Name",LVCFMT_LEFT,150);
	m_VisibleList.InsertColumn(1,"Set",LVCFMT_LEFT,75);
	m_VisibleList.InsertColumn(2,"Group",LVCFMT_LEFT,75);

	m_SetsTree.SetImageList(&m_ImageList,TVSIL_NORMAL);
	m_SetsTree.SetBkColor(RGB(255,255,255));

	m_VisibleList.SetEditable(FALSE);

	animation_chain_group = pRootExport->animation_data.animation_chain_group;
	if(animation_chain_group.size() > 0)
	{
		vector<IVisNode*> groups;
		// Заносим все меши в список
		IVisExporter * pIgame=pRootExport->pIgame;
		for(int loop = 0; loop <pIgame->GetRootNodeCount();loop++)
		{
			IVisNode * pGameNode = pIgame->GetRootNode(loop);
			AddNode(pGameNode,groups);
		}
		
		AnimationVisibleSet avs;
		avs.name = "PrevFormat";
		for (int i=0; i<animation_chain_group.size();i++) 
		{
			AnimationVisibleGroup avg;
			avg.name = animation_chain_group[i].name;
			avg.invisible_object = groups;
			for(int j=0; j<animation_chain_group[i].invisible_object.size(); j++)
			{
				for (int k=0; k<avg.invisible_object.size(); k++)
				{
					if (avg.invisible_object[k] == animation_chain_group[i].invisible_object[j])
					{
						avg.invisible_object.erase(avg.invisible_object.begin()+k);
						break;
					}
				}
			}
			avs.animation_visible_groups.push_back(avg);
		}
		avs.objects = groups;
		animation_sets.push_back(avs);
		pRootExport->animation_data.animation_chain_group.clear();
	}else
		animation_sets=pRootExport->animation_data.animation_visible_sets;

	for (int i=0; i<animation_sets.size(); i++)
	{
		HTREEITEM hitem = m_SetsTree.InsertItem(animation_sets[i].name.c_str(),0,0);
		for(int j=0; j<animation_sets[i].animation_visible_groups.size();j++)
		{
			m_SetsTree.InsertItem(animation_sets[i].animation_visible_groups[j].name.c_str(),4,4,hitem);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CVisibleGroupDlg::AddNode(IVisNode* current,vector<IVisNode*> &groups)
{
	if (IsNodeMesh(current) || IsNodeLight(current))
	{
		groups.push_back(current);
	}
	
	for(int i=0;i<current->GetChildNodeCount();i++)
	{
		IVisNode* node=current->GetChildNode(i);
		AddNode(node,groups);
	}
}

void CVisibleGroupDlg::AddTreeNode(IVisNode* current,int aset,int ag)
{
	if(current->IsTarget())
		return;
	const char* name = current->GetName();

	if (aset == -1)
		return;

	LVITEM item;
	item.mask=LVIF_TEXT | LVIF_IMAGE;
	item.state=0;
	item.stateMask=0;
	item.iItem=m_VisibleList.GetItemCount();
	item.iSubItem=0;
	item.pszText=(LPSTR)name;
	item.iImage = 5;
	if (IsNodeLight(current)) {
		item.iImage = 6;
	}

	if (IsNodeMesh(current) || IsNodeLight(current))
	{
		if (ag == -1)
		{
			int id = m_VisibleList.InsertItem(&item);
			m_VisibleList.SetItemData(id,(DWORD)current);

			CString sname;
			for(int i=0;i<animation_sets.size();i++)
			{
				AnimationVisibleSet& avs=animation_sets[i];
				for(int j=0;j<avs.objects.size();j++)
				{
					if (avs.objects[j] == current) 
					{
						sname = avs.name.c_str();
						break;
					}
				}
			}

			m_VisibleList.SetItemText(id,1,sname);
		}else
		{
			for(int i=0;i<animation_sets[aset].objects.size();i++)
			{
				if(animation_sets[aset].objects[i] == current)
				{
					int id = m_VisibleList.InsertItem(&item);
					m_VisibleList.SetItemData(id,(DWORD)current);
					m_VisibleList.SetItemText(id,1,animation_sets[aset].name.c_str());
					//for(int j=0; j<animation_sets[aset].animation_visible_groups.size();j++)
					//{
						AnimationVisibleGroup& avg = animation_sets[aset].animation_visible_groups[ag];
						for (int k=0; k<avg.invisible_object.size();k++)
						{
							if(avg.invisible_object[k] == current)
							{
								m_VisibleList.SetItemText(id,2,avg.name.c_str());
								break;
							}
						}
					//}
				}
			}
		}

	}
	for(int i=0;i<current->GetChildNodeCount();i++)
	{
		IVisNode* node=current->GetChildNode(i);
		AddTreeNode(node,aset,ag);
	}
}

void CVisibleGroupDlg::OnOK()
{
	pRootExport->animation_data.animation_visible_sets = animation_sets;
	CDialog::OnOK();
}
void CVisibleGroupDlg::ExpandAll(CTreeCtrl* tree,HTREEITEM hItem)
{

	for(HTREEITEM hcur = tree->GetChildItem(hItem);hcur;
		hcur = tree->GetNextItem(hcur,TVGN_NEXT))
	{
		ExpandAll(tree,hcur);
	}
	tree->Expand(hItem,TVE_EXPAND);

}

void CVisibleGroupDlg::OnBnClickedAddSet()
{
	CAddGroup dlg;
	dlg.m_Name="NewSet";
	if(dlg.DoModal()!=IDOK)
		return;

	string name = TrimString((LPCSTR)dlg.m_Name);
	for(int i=0;i<animation_sets.size();i++)
	{
		if(animation_sets[i].name == name)
		{
			MessageBox("Такое множество уже существует","Error",MB_OK|MB_ICONSTOP);
			return;
		}
	}
	AnimationVisibleSet av;
	av.name = name;
	animation_sets.push_back(av);

	HTREEITEM hitem = m_SetsTree.InsertItem(name.c_str(),0,0);
}

void CVisibleGroupDlg::OnTvnSelchangedTreeSets(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	m_VisibleList.DeleteAllItems();
	
	HTREEITEM hitem = m_SetsTree.GetSelectedItem();
	if(!hitem)
		return;
	string sname,gname;

	if(m_SetsTree.GetParentItem(hitem) == NULL)
	{
		sname = (LPCSTR)m_SetsTree.GetItemText(hitem);
		gname.clear();
	}else
	{
		gname = (LPCSTR)m_SetsTree.GetItemText(hitem);
		hitem = m_SetsTree.GetParentItem(hitem);
		sname = (LPCSTR)m_SetsTree.GetItemText(hitem);
	}

	int aset=-1, ag=-1;
	bool f = false;
	for(int i=0;i<animation_sets.size();i++)
	{
		AnimationVisibleSet& avs=animation_sets[i];
		if(avs.name == sname)
		{
			aset = i;
			for(int j=0;j<avs.animation_visible_groups.size();j++)
			{
				if(avs.animation_visible_groups[j].name == gname)
				{
					ag = j;
					f = true;
					break;
				}
			}
			if (f)
				break;
		}
	}

	IVisExporter * pIgame=pRootExport->pIgame;
	for(int loop = 0; loop <pIgame->GetRootNodeCount();loop++)
	{
		IVisNode * pGameNode = pIgame->GetRootNode(loop);
		AddTreeNode(pGameNode,aset,ag);
	}
	SortingList();
	*pResult = 0;
}

void CVisibleGroupDlg::OnNMClickListVisible(NMHDR *pNMHDR, LRESULT *pResult)
{
	int nItem = m_VisibleList.GetSelectionMark();
	if (nItem == -1)
		return;

	HTREEITEM hitem = m_SetsTree.GetSelectedItem();
	if(!hitem)
		return;

	IVisNode* current = (IVisNode*)m_VisibleList.GetItemData(nItem);
	if (!current)
		return;

	bool bChecked = false;

	string name = (LPCSTR)m_SetsTree.GetItemText(hitem);
	if (m_SetsTree.GetParentItem(hitem) == NULL) 
	{
		CString lname = m_VisibleList.GetItemText(nItem,1);
		CString tname = m_SetsTree.GetItemText(hitem);
		if(!lname.IsEmpty())
		{
			if (lname != tname)
			{
				m_VisibleList.SetItemState(nItem,0,LVIS_SELECTED);
				return;
			}
		}else
		{
			bChecked = true;
		}

		for(int i=0;i<animation_sets.size();i++)
		{
			if(animation_sets[i].name == name)
				break;
		}
		if (i >= animation_sets.size())
			return;

		if (bChecked) 
		{
			animation_sets[i].objects.push_back(current);
			m_VisibleList.SetItemText(nItem,1,animation_sets[i].name.c_str());
		}else
		{
			for(int j=0;j<animation_sets[i].objects.size();j++)
			{
				if(animation_sets[i].objects[j] == current)
				{
					animation_sets[i].objects.erase(animation_sets[i].objects.begin()+j);
					m_VisibleList.SetItemText(nItem,1,"");
					break;
				}
			}
		}

	}else
	{
		string sname = (LPCSTR)m_SetsTree.GetItemText(m_SetsTree.GetParentItem(hitem));
		
		CString lname = m_VisibleList.GetItemText(nItem,2);
		CString tname = m_SetsTree.GetItemText(hitem);
		if(!lname.IsEmpty())
		{
			//if (lname != tname)
			//{
			//	m_VisibleList.SetItemState(nItem,0,LVIS_SELECTED);
			//	return;
			//}
		}else
		{
			bChecked = true;
		}

		for(int i=0;i<animation_sets.size();i++)
		{
			if(animation_sets[i].name == sname)
				break;
		}
		if (i >= animation_sets.size())
			return;

		for(int j=0;j<animation_sets[i].animation_visible_groups.size();j++)
		{
			if(animation_sets[i].animation_visible_groups[j].name == name)
				break;
		}

		if (j >= animation_sets[i].animation_visible_groups.size())
			return;

		AnimationVisibleGroup& ag = animation_sets[i].animation_visible_groups[j];
		if (bChecked)
		{
			int sz = ag.invisible_object.size();
			ag.invisible_object.push_back(current);
			m_VisibleList.SetItemText(nItem,2,ag.name.c_str());

		}else
		{
			int sz = ag.invisible_object.size();
			for(int j=0;j<ag.invisible_object.size();j++)
			{
				if(ag.invisible_object[j] == current)
				{
					ag.invisible_object.erase(ag.invisible_object.begin()+j);
					m_VisibleList.SetItemText(nItem,2,"");
					break;
				}
			}

		}

	}

	m_VisibleList.SetSelectionMark(-1);

	*pResult = 0;
}

void CVisibleGroupDlg::OnTvnBeginlabeleditTreeSets(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	oldName = pTVDispInfo->item.pszText;
	*pResult = 0;
}

void CVisibleGroupDlg::OnTvnEndlabeleditTreeSets(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	*pResult = 1;
	if(!pTVDispInfo->item.pszText)
	{
		*pResult = 0;
		return;
	}
	
	
	int iset = 0;
	if(m_SetsTree.GetParentItem(pTVDispInfo->item.hItem) == NULL)
	{
		for(int i=0;i<animation_sets.size();i++)
		{
			if(animation_sets[i].name == pTVDispInfo->item.pszText)
			{
				MessageBox("Такое множество уже существует","Error",MB_OK|MB_ICONSTOP);
				*pResult = 0;
				return;
			}
			if (animation_sets[i].name == (LPCSTR)oldName) 
			{
				iset = i;
			}
		}
		animation_sets[iset].name = pTVDispInfo->item.pszText;

	}else
	{
		for(int i=0;i<animation_sets.size();i++)
		{
			CString sname = m_SetsTree.GetItemText(m_SetsTree.GetParentItem(pTVDispInfo->item.hItem));
			if(animation_sets[i].name == (LPCSTR)sname)
			{
				break;
			}
		}

		for (int j=0; j<animation_sets[i].animation_visible_groups.size(); j++)
		{
			if(animation_sets[i].animation_visible_groups[j].name == pTVDispInfo->item.pszText)
			{
				MessageBox("Такая группа уже существует","Error",MB_OK|MB_ICONSTOP);
				*pResult = 0;
				return;
			}
			if (animation_sets[i].animation_visible_groups[j].name == (LPCSTR)oldName) 
			{
				iset = j;
			}
		}
		animation_sets[i].animation_visible_groups[iset].name = pTVDispInfo->item.pszText;
	}
}

void CVisibleGroupDlg::OnBnClickedDeleteSet()
{
	HTREEITEM hitem = m_SetsTree.GetSelectedItem();
	if(hitem==TVI_ROOT)
		return;
	if (!hitem)
		return;
	if (m_SetsTree.GetParentItem(hitem) != NULL)
		return;

	if (MessageBox("Вы уверены, что хотите удалить множество?\nВсе группы в множестве, также будут удалены.","Warning!!!",MB_ICONQUESTION|MB_YESNO) == IDNO)
		return;

	string name = (LPCSTR)m_SetsTree.GetItemText(hitem);

	for(int i=0; i<animation_sets.size(); i++)
	{
		if (animation_sets[i].name == name)
		{
			animation_sets.erase(animation_sets.begin()+i);
			break;
		}
	}

	m_SetsTree.DeleteItem(hitem);
	if(m_SetsTree.GetCount() == 0)
		m_VisibleList.DeleteAllItems();
}
