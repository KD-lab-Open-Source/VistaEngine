// EffectTreeView.cpp : implementation file
//

#include "stdafx.h"
#include "EffectTool.h"
#include "EffectTreeView.h"
#include "Undo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CEffectTreeView

IMPLEMENT_DYNCREATE(CEffectTreeView, CTreeView)

CEffectTreeView::CEffectTreeView()
{
	m_bDragging = false;
	m_hitemDrag = 0;
	m_hitemDrop = 0;
	m_pimagelist = 0;
	user_act = true;
	_pDoc->SetEffectTreeView(this);
}

CEffectTreeView::~CEffectTreeView()
{
}
bool bFillingTree = false;
void CEffectTreeView::PopulateTree()
{
//	return;
	bFillingTree = true;
	CEffectToolDoc* pDoc = GetDocument();
	CTreeCtrl& tree = GetTreeCtrl();

	tree.DeleteAllItems();

	HTREEITEM first = NULL;
	
	for(int i=0; i<pDoc->GroupsSize(); i++)
	{
		CGroupData *group = pDoc->Group(i);
		// add group
		HTREEITEM hGroupItem = tree.InsertItem(group->m_name, 2, 2);
		if(!first)
			first = hGroupItem;
		tree.SetItemData(hGroupItem, (DWORD)group);
		tree.SetItemState(hGroupItem, 0, TVIS_STATEIMAGEMASK); //no checkbox
		pDoc->SetActiveGroup(group);

		for(int i=0;i<group->EffectsSize(); i++)
		{
			CEffectData *eff = group->Effect(i);
			HTREEITEM hEffectItem = tree.InsertItem(eff->name.c_str(), 0, 0, hGroupItem);
			tree.SetItemData(hEffectItem, (DWORD)eff);
			tree.SetItemState(hEffectItem, 0, TVIS_STATEIMAGEMASK); //no checkbox
			GetDocument()->SetActiveEffect(eff);
			
			for(int i=0;i<eff->EmittersSize(); i++)
			{
				CEmitterData *em = eff->Emitter(i);
				HTREEITEM h = NULL;
				h = tree.InsertItem(em->name().c_str(), 1, 1, hEffectItem);

				tree.SetItemData(h, (DWORD)em);
				tree.SetCheck(h);
				GetDocument()->SetActiveEmitter(em);
			}
			if(eff->IsExpand())
				tree.Expand(hEffectItem, TVE_EXPAND);
		}
		if(group->IsExpand())
			tree.Expand(hGroupItem, TVE_EXPAND);
	}
	if(first)
		tree.EnsureVisible(first);
	bFillingTree = false;
	Sort();
}
void CEffectTreeView::GetCheckState()
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItemGroup = tree.GetNextItem(TVI_ROOT, TVGN_CHILD);
	while(hItemGroup)
	{
		HTREEITEM hItemEffect = tree.GetNextItem(hItemGroup, TVGN_CHILD);
		while(hItemEffect)
		{
			HTREEITEM hItemEmitter = tree.GetNextItem(hItemEffect, TVGN_CHILD);
			while(hItemEmitter)
			{
				CEmitterData* p = (CEmitterData*)tree.GetItemData(hItemEmitter);
				p->SetActive(tree.GetCheck(hItemEmitter));

				hItemEmitter = tree.GetNextItem(hItemEmitter, TVGN_NEXT);
			}
			hItemEffect = tree.GetNextItem(hItemEffect, TVGN_NEXT);
		}
		hItemGroup = tree.GetNextItem(hItemGroup, TVGN_NEXT);
	}

	GetDocument()->UpdateAllViews(0);
}
void CEffectTreeView::CloneEffect(HTREEITEM hItemDrag, HTREEITEM hItemDrop)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItemGroup = NULL;
	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItemDrop, nImage, nSelectedImage );

	switch(nImage)
	{
	case 2://group
		hItemGroup = hItemDrop;
		break;
	case 0://effect
		hItemGroup = tree.GetParentItem(hItemDrop);
		break;
	case 1://emitter
		hItemGroup = tree.GetParentItem(tree.GetParentItem(hItemDrop));
		break;
	default:
		MessageBeep(0);
		return;
	}
	if(hItemGroup!=tree.GetParentItem(hItemDrag))
		return;
	CGroupData* group = (CGroupData*)tree.GetItemData(hItemGroup);
	CEffectData* p = (CEffectData*)tree.GetItemData(hItemDrag);
	CEffectData* eff = group->AddEffect(new CEffectData(p));
	//group->CheckEffectName(eff); //next
	UpdateGroup(group);
/*
	group->m_effects.push_back(p = new CEffectData(p));
	// Check Name

	//
	HTREEITEM hItem = tree.InsertItem(p->name.c_str(), 0, 0, hItemGroup);
	tree.SetItemData(hItem, (DWORD)p);
	tree.SetItemState(hItem, 0, TVIS_STATEIMAGEMASK); //no checkbox
	tree.SelectItem(hItem);

	EmitterListType::iterator i_e;
	FOR_EACH(p->emitters, i_e)
	{
		HTREEITEM h = tree.InsertItem((*i_e)->name().c_str(), 1, 1, hItem);
		tree.SetItemData(h, (DWORD)*i_e);
		tree.SetCheck(h);
	}
	tree.Expand(hItem, TVE_EXPAND);
	Sort();
*/
}
void CEffectTreeView::CopyEmitter(HTREEITEM hItemFrom, HTREEITEM hItemTo)
{
	CTreeCtrl& tree = GetTreeCtrl();

	CEmitterData* pEmitter = (CEmitterData*)tree.GetItemData(hItemFrom);

	HTREEITEM hItemEffect = NULL;
	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItemTo, nImage, nSelectedImage );

	switch(nImage)
	{
	case 2://group
		hItemEffect = tree.GetChildItem(hItemTo);
		break;
	case 0://effect
		hItemEffect = hItemTo;
		break;
	case 1://emitter
		hItemEffect = tree.GetParentItem(hItemTo);
		break;
	default:
		MessageBeep(0);
		return;
		break;
	}
	CGroupData* group = (CGroupData*)tree.GetItemData(tree.GetParentItem(hItemEffect));
	CEffectData* pEffect = (CEffectData*)tree.GetItemData(hItemEffect);
	_pDoc->History().PushEffect(pEffect, group);
	CEmitterData* em = pEffect->add_emitter(pEmitter);
	//pEffect->CheckEmitterName(em); //next
	UpdateGroup(group);
	
/*
	pEffect->emitters.push_back(pEmitter = new CEmitterData(pEmitter->emitter()));

	// Check name
	CString name = pEmitter->name().c_str();

	int i = 1;
	while(!pEffect->CheckName(name, pEmitter))
	{
		CString new_name;
		new_name.Format("%s %d", name, i);
		name = new_name;
		i++;
	}
	pEmitter->name() = name;

	HTREEITEM h = tree.InsertItem(name, 1, 1, hItemEffect);
	tree.SetItemData(h, (DWORD)pEmitter);
	tree.SetCheck(h);
	Sort();
*/
}

void CEffectTreeView::MoveEmitter(HTREEITEM hItemFrom, HTREEITEM hItemTo)
{
	CTreeCtrl& tree = GetTreeCtrl();
//	CEmitterData* pEmitter = (CEmitterData*)tree.GetItemData(hItemFrom);

	HTREEITEM hItemParent = tree.GetParentItem(hItemTo);
	if(hItemParent!=tree.GetParentItem(hItemFrom))
		return;

	CEffectData* pEffect = (CEffectData*)tree.GetItemData(hItemParent);
	_pDoc->History().PushEffect(pEffect);

	CEmitterData* p1 = (CEmitterData*)tree.GetItemData(hItemFrom);
	CEmitterData* p2 = (CEmitterData*)tree.GetItemData(hItemTo);

	//pEffect->swap_emitters(p1, p2);
	//int n1=0,n2=0;
	//for(int i=0; i<pEffect->emitters.size(); i++)
	//{
	//	if (pEffect->emitters[i] == p1)
	//		n1 = i;
	//	if (pEffect->emitters[i] == p2)
	//		n2 = i;
	//}
	tree.DeleteItem(hItemFrom);

	HTREEITEM h = tree.InsertItem(p1->name().c_str(), 1, 1, hItemParent, hItemTo);
	tree.SetItemData(h, (DWORD)p1);
	tree.SetCheck(h);

	pEffect->move_emitters(p1,p2);
	//int n = pEffect->EmitterIndex(p2);
	//pEffect->insert_emitter(n,p1);
	//pEffect->del_emitter(p1);

}


BEGIN_MESSAGE_MAP(CEffectTreeView, CTreeView)
	//{{AFX_MSG_MAP(CEffectTreeView)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
//	ON_UPDATE_COMMAND_UI(ID_EFFECT_DEL, OnUpdateEffectDel)
//	ON_UPDATE_COMMAND_UI(ID_EFFECT_EMITTER_DEL, OnUpdateEffectEmitterDel)
	ON_UPDATE_COMMAND_UI(ID_EFFECT_EMITTER_NEW, OnUpdateEffectEmitterNew)
	ON_UPDATE_COMMAND_UI(ID_EFFECT_NEW, OnUpdateEffectNew)
	ON_COMMAND(ID_EFFECT_NEW, OnEffectNew)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndlabeledit)
//	ON_COMMAND(ID_EFFECT_DEL, OnEffectDel)
	ON_COMMAND(ID_EFFECT_EMITTER_NEW, OnEffectEmitterNew)
//	ON_COMMAND(ID_EFFECT_EMITTER_DEL, OnEffectEmitterDel)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
	ON_UPDATE_COMMAND_UI(ID_GROUP_NEW, OnUpdateGroupNew)
	ON_COMMAND(ID_GROUP_NEW, OnGroupNew)
	ON_COMMAND(ID_ELEMENT_DELETE, OnElementDelete)
	ON_UPDATE_COMMAND_UI(ID_ELEMENT_DELETE, OnUpdateElementDelete)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnTvnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnTvnKeydown)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//

void CEffectTreeView::OnDraw(CDC* pDC)
{
}
/////////////////////////////////////////////////////////////////////////////
// CEffectTreeView message handlers

void CEffectTreeView::OnDestroy() 
{
	delete GetTreeCtrl().GetImageList(TVSIL_NORMAL);

	CTreeView::OnDestroy();
}

void CEffectTreeView::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();

	GetTreeCtrl().ModifyStyle(0, TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_EDITLABELS|TVS_CHECKBOXES|TVS_SHOWSELALWAYS);

	CImageList* pImageList = new CImageList;
	pImageList->Create(IDB_TREE, 16, 0, RGB(255, 0, 0));
	GetTreeCtrl().SetImageList(pImageList, TVSIL_NORMAL);

	PopulateTree();
}
#include "OptTree.h"
#include ".\effecttreeview.h"
extern COptTree* tr;
void CEffectTreeView::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	*pResult = 1;

	if(pTVDispInfo->item.pszText)
	{
		CTreeCtrl& tree = GetTreeCtrl();

		HTREEITEM hItem = tree.GetSelectedItem();
		int nImage, nSelectedImage;
		int img = tree.GetItemImage( hItem, nImage, nSelectedImage );
		switch(nImage)
		{
		case 2://group
			{
				CGroupData* group = (CGroupData*)tree.GetItemData(hItem);
				if(!GetDocument()->CheckGroupName(pTVDispInfo->item.pszText))
				{
					CString s;
					s.Format("Группа '%s' уже существует", pTVDispInfo->item.pszText);
					AfxMessageBox(s, MB_OK|MB_ICONWARNING);

					*pResult = 0;
				}
				else
				{
					_pDoc->History().PushGroup();
					group->m_name = pTVDispInfo->item.pszText;
				}
			}
			break;
		case 0://effect
			{
				CEffectData* effect = (CEffectData*)tree.GetItemData(hItem);

				if(!_pDoc->ActiveGroup()->CheckEffectName(pTVDispInfo->item.pszText))
				{
					CString s;
					s.Format("Эффект '%s' уже существует", pTVDispInfo->item.pszText);
					AfxMessageBox(s, MB_OK|MB_ICONWARNING);

					*pResult = 0;
				}
				else
				{
					_pDoc->History().PushEffect();
					effect->name = pTVDispInfo->item.pszText;
				}
			}
			break;
		case 1://emitter
			{
				CEmitterData* emitter = (CEmitterData*)tree.GetItemData(hItem);

				hItem = tree.GetParentItem(hItem);
				CEffectData* pEffect = (CEffectData*)tree.GetItemData(hItem);
				if (CString(pTVDispInfo->item.pszText)=="" || CString(pTVDispInfo->item.pszText)=="не связан")
				{
					AfxMessageBox("Не допустимое имя эмиттера", MB_OK|MB_ICONWARNING);
					*pResult = 0;
				}
				else if(pEffect->CheckEmitterName(pTVDispInfo->item.pszText))
				{
					if (emitter->name() != pTVDispInfo->item.pszText)
					{
						CString s;
						s.Format("Эмиттер '%s' уже существует", pTVDispInfo->item.pszText);
						AfxMessageBox(s, MB_OK|MB_ICONWARNING);
						*pResult = 0;
					}
				}
				else
				{
					_pDoc->History().PushEmitter();
					if(emitter->IsBase())
					{
						tr->ChangeEmitterName(emitter->name().c_str(),pTVDispInfo->item.pszText);
						emitter->name() = pTVDispInfo->item.pszText;
					}
					else
						emitter->name() = pTVDispInfo->item.pszText;

					emitter->SetDirty(true);
				}
			}
			break;
		}
	}
	CTreeCtrl& tree = GetTreeCtrl();
	tree.SortChildren(TVI_ROOT);
}

void CEffectTreeView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	if (m_bDragging)
	{
		HTREEITEM hitem = GetTreeCtrl().GetDropHilightItem();
		if (GetTreeCtrl().GetItemState(hitem,0xffffffff)&TVIS_EXPANDED)
			GetTreeCtrl().Expand(hitem,TVE_COLLAPSE);
		else 
			GetTreeCtrl().Expand(hitem,TVE_EXPAND);
	}else
	{
		CPoint     pt, ptScreen;
		UINT       flags;
		CTreeCtrl& tree = GetTreeCtrl();

		GetCursorPos(&ptScreen); pt = ptScreen;
		tree.ScreenToClient(&pt);
		
		HTREEITEM hItem = tree.HitTest(pt, &flags);

		if(hItem){
			tree.SelectItem(hItem);
			SelChanged(hItem);
		}

		CMenu menu;
		menu.LoadMenu(IDR_MENU_POPUP);

		if (theApp.scene.m_pModel)
		{
			int visCount = theApp.scene.m_pModel->GetVisibilityGroupNumber();
			if (visCount > 0)
			{
				CMenu* visMenu;
				visMenu = new CMenu();
				visMenu->CreatePopupMenu();
				menu.GetSubMenu(0)->InsertMenu(6,MF_BYPOSITION | MF_POPUP, (UINT)visMenu->m_hMenu,"Visible Groups");
				
				for (int i = 0; i<visCount; i++)
				{
					visMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_VISGROUP_NUM+i, theApp.scene.m_pModel->GetVisibilityGroupName(C3dxVisibilityGroup(i)));
					if (C3dxVisibilityGroup(i) == theApp.scene.m_pModel->GetVisibilityGroupIndex(theApp.scene.m_pModel->GetVisibilityGroup()->name.c_str()))
					{
						visMenu->CheckMenuItem(ID_VISGROUP_NUM+i,MF_CHECKED);
					}
				}
			}
			int nodeCount = theApp.scene.m_pModel->GetNodeNumber();
			if (nodeCount > 0)
			{
				CMenu* nodeMenu;
				nodeMenu = new CMenu();
				nodeMenu->CreatePopupMenu();
				menu.GetSubMenu(0)->InsertMenu(6,MF_BYPOSITION | MF_POPUP, (UINT)nodeMenu->m_hMenu,"Nodes");

				nodeMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_NODE_NUM,"Не выбрано");
				if (GetDocument()->m_nCurrentNode == 0)
				{
					nodeMenu->CheckMenuItem(ID_NODE_NUM,MF_CHECKED);
				}
				for(int i=0; i<nodeCount; i++)
				{
					nodeMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_NODE_NUM+i+1, theApp.scene.m_pModel->GetNodeName(i));
					if (GetDocument()->m_nCurrentNode == i+1)
					{
							nodeMenu->CheckMenuItem(ID_NODE_NUM+i+1,MF_CHECKED);
					}
					//if (i == theApp.scene.m_pModel->GetVisibilityGroupIndex0(theApp.scene.m_pModel->GetVisibilityGroup()->name.c_str()))
					//{
					//	visMenu->CheckMenuItem(ID_VISGROUP_NUM+i,MF_CHECKED);
					//}
				}
			}
			int chainNum = theApp.scene.m_pModel->GetChainNumber();
			if(chainNum>0)
			{
				CMenu* chainMenu;
				chainMenu = new CMenu();
				chainMenu->CreatePopupMenu();
				menu.GetSubMenu(0)->InsertMenu(7,MF_BYPOSITION | MF_POPUP, (UINT)chainMenu->m_hMenu,"Animation chains");
				for(int i=0; i<chainNum; i++)
				{
					chainMenu->AppendMenu(MF_STRING | MF_ENABLED, ID_CHAIN_NUM+i, theApp.scene.m_pModel->GetChain(i)->name.c_str());
					if (GetDocument()->m_nCurrentChain == i)
					{
						chainMenu->CheckMenuItem(ID_CHAIN_NUM+i,MF_CHECKED);
					}
				}
			}
		}
		menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN, ptScreen.x, ptScreen.y, AfxGetMainWnd());
	}
}

void CEffectTreeView::OnGroupNew()
{

	CTreeCtrl& tree = GetTreeCtrl();
	_pDoc->SetActiveGroup(_pDoc->AddGroup());
	_pDoc->SetActiveEffect(_pDoc->ActiveGroup()->Effect(0));
	_pDoc->SetActiveEmitter(_pDoc->ActiveEffect()->Emitter(0));
	_pDoc->History().PushGroup(_pDoc->ActiveGroup());//additing
	UpdateGroup(_pDoc->ActiveGroup());
/*	string name = pDoc->ActiveGroup()->m_name;
	UINT i=0;
	HTREEITEM item = ;
	tree.GetParentItem(item);
	for(;i<tree.GetCount();i++)
	{
		if (tree.GetFirstVisibleItem()
	}
	tree.SortChildren(tree.GetRootItem());
*/	
/*	HTREEITEM hItem = tree.InsertItem(_pDoc->ActiveGroup()->m_name, 2, 2);
	tree.SetItemData(hItem, (DWORD)_pDoc->ActiveGroup());
	tree.SetItemState(hItem, 0, TVIS_STATEIMAGEMASK); //no checkbox
	tree.SelectItem(hItem);
	Sort();
	OnEffectNew();
*/
}
/*
HTREEITEM CEffectTreeView::FindItemDat(DWORD dat, HTREEITEM item)
{
	CTreeCtrl& tree = GetTreeCtrl();
	if (!item) item = tree.GetRootItem();
	if (!item) return NULL;
	do
	{
		if (tree.GetItemData(item))==dat) 
			return item;
		HTREEITEM chit = tree.GetNextItem(item,TLGN_CHILD);
		if (chit&&(chit=FindItem(dat,chit)))
			return chit;

	}while((item = tree.GetNextItem(item,TLGN_NEXT)));
	return NULL;
}*/
HTREEITEM CEffectTreeView::FindGroupItem(CGroupData* group)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM item = tree.GetRootItem();
	if (!item) return NULL;
	do
	{
		CGroupData * groupItem = (CGroupData*)(void*)tree.GetItemData(item);
		if (groupItem && group->GetID() == groupItem->GetID()) 
			return item;
	}
	while(item=tree.GetNextItem(item,TVGN_NEXT));
	return NULL;
}
/*
void CEffectTreeView::GroupDeleted(CGroupData* group)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = FindItem((DWORD)group);
	ASSERT(hItem);
	tree.DeleteItem(hItem);
}
*/
void CEffectTreeView::Sort()
{
	CTreeCtrl& tree = GetTreeCtrl();
	tree.SortChildren(TVI_ROOT);
}

void CEffectTreeView::UpdateGroup(CGroupData* group)
{
	user_act = false;
	CTreeCtrl& tree = GetTreeCtrl();
	
	HTREEITEM hItem = FindGroupItem(group);
	if (hItem)
		tree.DeleteItem(hItem);
	hItem = tree.InsertItem(group->m_name, 2, 2);

	tree.SetItemData(hItem, (DWORD)group);
	tree.SetItemState(hItem, 0, TVIS_STATEIMAGEMASK); 
	HTREEITEM cur_eff = NULL;
	HTREEITEM cur_em = NULL;
	tree.Expand(hItem, group->IsExpand() ? TVE_EXPAND : TVE_COLLAPSE);
	vector<HTREEITEM> Hit;
	for(int i=0;i<group->EffectsSize(); i++)
	{
		CEffectData* Eff = group->Effect(i);
		HTREEITEM hEff = tree.InsertItem(Eff->name.c_str(), 0, 0, hItem);
		ASSERT(hEff);
		tree.SetItemData(hEff, (DWORD)Eff);
		tree.SetItemState(hEff, 0, TVIS_STATEIMAGEMASK); 
		Hit.push_back(hEff);
		if (Eff==_pDoc->ActiveEffect())
			cur_eff = hEff;
		for(int j=0;j<Eff->EmittersSize(); j++)
		{
			CEmitterData* em = Eff->Emitter(j);
			HTREEITEM hEm = tree.InsertItem(em->name().c_str(), 1, 1, hEff);
			tree.SetItemData(hEm, (DWORD)em);
			tree.SetCheck(hEm, em->IsActive());
			if (em==_pDoc->ActiveEmitter())
				cur_em = hEm;
		}
		tree.Expand(hEff, Eff->IsExpand() ? TVE_EXPAND : TVE_COLLAPSE);
	}
	if (group==_pDoc->ActiveGroup())
	{
		tree.SelectItem(hItem);
		tree.SelectItem(cur_eff);
		tree.SelectItem(cur_em);
	}
	tree.Expand(hItem, group->IsExpand() ? TVE_EXPAND : TVE_COLLAPSE);
	for(int i=0;i<group->EffectsSize(); i++)
	{
		CEffectData* Eff = group->Effect(i);
		tree.Expand(Hit[i], Eff->IsExpand() ? TVE_EXPAND : TVE_COLLAPSE);
	}
	Sort();
	user_act = true;
}
void CEffectTreeView::DeleteGroupView(CGroupData* group)
{
	HTREEITEM hItem = FindGroupItem(group);
	if (hItem)
	{
		_pDoc->DeleteGroup(group);
		GetTreeCtrl().DeleteItem(hItem);
	}
}

void CEffectTreeView::OnElementDelete()
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();
	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItem, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		_pDoc->History().PushGroup(true);
		GetDocument()->DeleteGroup(_pDoc->ActiveGroup());
		GetTreeCtrl().DeleteItem(hItem);
		break;
	case 0://effect
		_pDoc->History().PushGroup();
		_pDoc->ActiveGroup()->DeleteEffect(_pDoc->ActiveEffect());
		UpdateGroup(_pDoc->ActiveGroup());
		break;
	case 1://emitter
		_pDoc->History().PushEffect();
		_pDoc->ActiveEffect()->del_emitter(_pDoc->ActiveEmitter());
		_pDoc->SetActiveEmitter(_pDoc->ActiveEffect()->Emitter(0));
		UpdateGroup(_pDoc->ActiveGroup());
		break;
	default: ASSERT(0);
	}
}

void CEffectTreeView::OnUpdateElementDelete(CCmdUI *pCmdUI)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();
	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItem, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		pCmdUI->Enable(_pDoc->GroupsSize()>1);
		break;
	case 0://effect
		pCmdUI->Enable(_pDoc->ActiveGroup()->EffectsSize()>1);
		break;
	case 1://emitter
		pCmdUI->Enable(_pDoc->ActiveEffect()->EmittersSize()>1);
		break;
	}
}

void CEffectTreeView::OnEffectNew() 
{
	CTreeCtrl& tree = GetTreeCtrl();
	_pDoc->History().PushGroup();
	_pDoc->SetActiveEffect(_pDoc->ActiveGroup()->AddEffect());
//	_pDoc->ActiveEffect()->name = "effect";
//	_pDoc->ActiveEffect()->SetExpand(true);
	_pDoc->ActiveEffect()->add_emitter();
	_pDoc->SetActiveEmitter(NULL);
//	_pDoc->History().PushEffect(NULL, NULL, true);
	UpdateGroup(GetDocument()->ActiveGroup());
/*
	CEffectData* pNewEffect;
	GetDocument()->ActiveGroup()->m_effects.push_back(pNewEffect = new CEffectData);

	HTREEITEM hItemGroup = tree.GetSelectedItem();

	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItemGroup, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		break;
	case 0://effect
		hItemGroup = tree.GetParentItem(hItemGroup);
		break;
	case 1://emitter
		hItemGroup = tree.GetParentItem(tree.GetParentItem(hItemGroup));
		break;
	}

	HTREEITEM hItem = tree.InsertItem(pNewEffect->name.c_str(), 0, 0, hItemGroup);
	tree.SetItemData(hItem, (DWORD)pNewEffect);
	tree.SetItemState(hItem, 0, TVIS_STATEIMAGEMASK); //no checkbox
	tree.SelectItem(hItem);

	OnEffectEmitterNew();
*/
}
void CEffectTreeView::OnEffectDel() 
{
	_pDoc->ActiveGroup()->DeleteEffect(_pDoc->ActiveEffect());
	UpdateGroup(_pDoc->ActiveGroup());
/*
	CTreeCtrl& tree = GetTreeCtrl();


	HTREEITEM hItem = tree.GetSelectedItem();
	CEffectData* pEffect = (CEffectData*)tree.GetItemData(hItem);
	
	EffectStorageType& effects = GetDocument()->ActiveGroup()->m_effects;
	EffectStorageType::iterator it = find(effects.begin(), effects.end(), pEffect);
	ASSERT(it != effects.end());

	GetDocument()->Release3DModel();

	delete pEffect;
	effects.erase(it);
	GetTreeCtrl().DeleteItem(hItem);
*/
}
void CEffectTreeView::Clear()
{
	user_act = false;
	CTreeCtrl& tree = GetTreeCtrl();
	tree.DeleteAllItems();
	user_act = true;
}	

void CEffectTreeView::OnEffectEmitterNew() 
{
	CTreeCtrl& tree = GetTreeCtrl();
	_pDoc->History().PushEffect();
	_pDoc->SetActiveEmitter(_pDoc->ActiveEffect()->add_emitter());
//	_pDoc->History().PushEmitter(true);
	UpdateGroup(_pDoc->ActiveGroup());
/*
	HTREEITEM hItem = tree.GetSelectedItem();
//	if(tree.GetParentItem(hItem) != 0)
//		hItem = tree.GetParentItem(hItem);
///
	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItem, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		hItem = tree.GetChildItem(hItem);
		break;
	case 0://effect
		break;
	case 1://emitter
		hItem = tree.GetParentItem(hItem);
		break;
	}
///
	CEffectData* pEffect = (CEffectData*)tree.GetItemData(hItem);

	CEmitterData* pEmitter = pEffect->add_emitter();

	tree.SetItemData(hItem, (DWORD)pEmitter);
	tree.SetCheck(hItem);
	tree.SelectItem(hItem);
*/
}

void CEffectTreeView::OnEffectEmitterDel() 
{
	ASSERT(0);
	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hItem = tree.GetSelectedItem();
	CEmitterData* pEmitter = (CEmitterData*)tree.GetItemData(hItem);
	CEffectData* pEffect = (CEffectData*)tree.GetItemData(tree.GetParentItem(hItem));

	GetDocument()->Release3DModel();

	pEffect->del_emitter(pEmitter);

	tree.DeleteItem(hItem);
}

void CEffectTreeView::OnUpdateEffectNew(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetTreeCtrl().GetSelectedItem() != 0 );
}
void CEffectTreeView::OnUpdateEffectDel(CCmdUI* pCmdUI) 
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();

	pCmdUI->Enable(hItem && GetTreeCtrl().GetParentItem(hItem) == 0);
}
void CEffectTreeView::OnUpdateEffectEmitterDel(CCmdUI* pCmdUI) 
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();

	pCmdUI->Enable(hItem && GetTreeCtrl().GetParentItem(hItem) != 0);
}
void CEffectTreeView::OnUpdateEffectEmitterNew(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetTreeCtrl().GetSelectedItem() != 0);
}
void CEffectTreeView::OnUpdateGroupNew(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}

void CEffectTreeView::SelChanged(HTREEITEM hItem)
{
	if (!user_act)
		return;
	CTreeCtrl& tree = GetTreeCtrl();

	CEffectToolDoc* pDoc = GetDocument();
	int nImage, nSelectedImage;
	int img = tree.GetItemImage( hItem, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		{
			CGroupData* group = (CGroupData*)tree.GetItemData(hItem);
			if (group!=pDoc->ActiveGroup())
			{
				pDoc->SetActiveGroup(group);
				pDoc->SetActiveEffect(pDoc->ActiveGroup()->Effect(0));
			}
			pDoc->SetActiveEmitter(NULL);
		}
		break;
	case 0://effect
		{
			pDoc->SetActiveGroup((CGroupData*)tree.GetItemData(tree.GetParentItem(hItem)));
			CEffectData * eff = (CEffectData*)tree.GetItemData(hItem);
			if (eff != _pDoc->ActiveEffect())
			{
				pDoc->SetActiveEffect((CEffectData*)tree.GetItemData(hItem));
			}
			pDoc->SetActiveEmitter(NULL);
			break;
		}
	case 1://emitter
		pDoc->SetActiveGroup((CGroupData*)tree.GetItemData(tree.GetParentItem(tree.GetParentItem(hItem))));
		pDoc->SetActiveEffect((CEffectData*)tree.GetItemData(tree.GetParentItem(hItem)));
		pDoc->SetActiveEmitter((CEmitterData*)tree.GetItemData(hItem));
		break;
	}
	tree.Invalidate();
}
void CEffectTreeView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CEffectToolDoc* pDoc = GetDocument();
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	if(bFillingTree)
		return;

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hItem = tree.GetSelectedItem();
////
	SelChanged(hItem);
////	
	pDoc->m_nCurrentGenerationPoint = 0;
	pDoc->m_nCurrentParticlePoint = 0;
	pDoc->UpdateAllViews(0);
	AfxGetMainWnd()->SendMessage(WM_UPDATE_BAR);

	theApp.scene.InitEmitters();
}

void CEffectTreeView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTreeView::OnLButtonDown(nFlags, point);

	CTreeCtrl& tree = GetTreeCtrl();

	UINT uFlags;
	if(tree.HitTest(point, &uFlags))
	{
		GetCheckState();
		theApp.scene.InitEmitters();
	}
}

void CEffectTreeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
//	if(lHint == HINT_UPDATE_TREE ||!GetDocument()->ActiveGroup())
//		PopulateTree();
	if(!GetDocument()->m_StorePath.IsEmpty())
		GetDocument()->SetWorkingDir();

	GetDocument()->Load3DModel(MODE_FIND, TYPE_3DMODEL);
	GetDocument()->Load3DModel(MODE_FIND, TYPE_3DBACK);

	GetDocument()->SetWorkingDir();
	Sort();
}

void CEffectTreeView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;


	CPoint      ptAction = pNMTreeView->ptDrag;
	//UINT        nFlags;

	m_hitemDrag = pNMTreeView->itemNew.hItem;//GetTreeCtrl().HitTest(ptAction, &nFlags);

	//////
	int nImage, nSelectedImage;
	int img = GetTreeCtrl().GetItemImage( m_hitemDrag, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		return;
		break;
	case 0://effect
		break;
	case 1://emitter
		break;
	}
	//////

	//GetCursorPos(&ptAction);
	//ScreenToClient(&ptAction);
	ASSERT(!m_bDragging);
	m_bDragging = true;
	//m_hitemDrag = GetTreeCtrl().HitTest(ptAction, &nFlags);
	m_hitemDrop = 0;

	ASSERT(m_pimagelist == NULL);
	m_pimagelist = GetTreeCtrl().CreateDragImage(m_hitemDrag);  // get the image list for dragging
	m_pimagelist->DragShowNolock(TRUE);
	m_pimagelist->SetDragCursorImage(0, CPoint(0, 0));
	m_pimagelist->BeginDrag(0, CPoint(0,0));
	//m_pimagelist->DragMove(ptAction);
	ClientToScreen(&ptAction);
	m_pimagelist->DragEnter(NULL, ptAction);
	SetCapture();
}

void CEffectTreeView::OnMouseMove(UINT nFlags, CPoint point) 
{
	HTREEITEM           hitem;
	UINT                flags;
	CTreeCtrl& tree = GetTreeCtrl();
	if (m_bDragging)
	{
		POINT pt = point;
		ClientToScreen(&pt);
		ASSERT(m_pimagelist != NULL);
		m_pimagelist->DragMove(pt);
		m_pimagelist->DragLeave(NULL);


		hitem = GetTreeCtrl().HitTest(point, &flags);

		m_hitemDrop = NULL;
		if(nFlags & MK_CONTROL)
		{
			//if(tree.GetParentItem(m_hitemDrag)==tree.GetParentItem(m_hitemDrop)) // тот же эффект
			GetTreeCtrl().SelectDropTarget(hitem);
			m_hitemDrop = hitem;
		}else
		{
			if(tree.GetParentItem(m_hitemDrag)==tree.GetParentItem(hitem))
			{
				GetTreeCtrl().SelectDropTarget(hitem);
				m_hitemDrop = hitem;
			}
		}
		CRect rc;
		GetClientRect(rc);
		if (point.y>rc.bottom-10)
		{
			SendMessage(WM_VSCROLL,SB_LINEDOWN,0);
			//Invalidate();
		}
		else if (point.y<10)
		{
			SendMessage(WM_VSCROLL,SB_LINEUP,0);
			//Invalidate();
		}
		m_pimagelist->DragEnter(NULL, pt);
	}

	CTreeView::OnMouseMove(nFlags, point);
}

void CEffectTreeView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bDragging)
	{
		ASSERT(m_pimagelist != NULL);
		m_pimagelist->EndDrag();
		delete m_pimagelist;
		m_pimagelist = NULL;

		int nImage, nSelectedImage;
		int img = GetTreeCtrl().GetItemImage( m_hitemDrag, nImage, nSelectedImage );

		switch(nImage)
		{
		case 2://group
			MessageBeep(0);
			break;
		case 0://effect
//			if(nFlags & MK_CONTROL)
//				CloneEffect(m_hitemDrag, m_hitemDrop);
			break;
		case 1://emitter
			if(m_hitemDrop && m_hitemDrag != m_hitemDrop)
			{
				//if(nFlags & MK_CONTROL)
				if (GetKeyState(VK_CONTROL) & 0x8000)
					CopyEmitter(m_hitemDrag, m_hitemDrop);
				else
					MoveEmitter(m_hitemDrag, m_hitemDrop);
			}
			else
				MessageBeep(0);
			break;
		}

		ReleaseCapture();
		m_bDragging = FALSE;
		GetTreeCtrl().SelectDropTarget(0);
		theApp.scene.InitEmitters();
	}
	
	CTreeView::OnLButtonUp(nFlags, point);
}

DROPEFFECT CEffectTreeView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return CTreeView::OnDragOver(pDataObject, dwKeyState, point);
}

void CEffectTreeView::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult) 
{
    LPNMTVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMTVCUSTOMDRAW>(pNMHDR);
        
	CEffectToolDoc* pDoc = GetDocument();
	*pResult = 0;
	switch(pNMCD->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
	case CDDS_ITEMPREPAINT:
			HTREEITEM hItemSpec = reinterpret_cast<HTREEITEM>(pNMCD->nmcd.dwItemSpec);
			TV_ITEM ti;
			ti.hItem = hItemSpec;
			ti.mask = TVIF_IMAGE;

			CTreeCtrl& tree = GetTreeCtrl();
			tree.GetItem(&ti);

			switch(ti.iImage)
			{
			case 2://group
				{
					CGroupData* group = (CGroupData*)tree.GetItemData(hItemSpec);
					if(pDoc->ActiveGroup()==group)
						pNMCD->clrTextBk = RGB(255, 150, 150);
					else
						pNMCD->clrTextBk = RGB(200, 150, 150);
				}
				break;
			case 0://effect
				{
					CEffectData* effect = (CEffectData*)tree.GetItemData(hItemSpec);
					if(pDoc->ActiveEffect()==effect)
						pNMCD->clrTextBk = RGB(150, 255, 150);
					else
						pNMCD->clrTextBk = RGB(150, 200, 150);
				}
				break;
			case 1://emitter
				{
					CEmitterData* emitter = (CEmitterData*)tree.GetItemData(hItemSpec);
					if(pDoc->ActiveEmitter()==emitter)
						pNMCD->clrTextBk = RGB(150, 150, 255);
					else
						pNMCD->clrTextBk = RGB(150, 150, 200);
				}
				break;
			}
			break;
	} 
}

void CEffectTreeView::OnTvnItemexpanded(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	CTreeCtrl& tree = GetTreeCtrl();	
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	int action = pNMTreeView->action;

	//////
	int nImage, nSelectedImage;
	int img = GetTreeCtrl().GetItemImage( hItem, nImage, nSelectedImage );
	switch(nImage)
	{
	case 2://group
		{
			CGroupData* group = (CGroupData*)tree.GetItemData(hItem);
			group->SetExpand( action==TVE_EXPAND);
		}
		break;
	case 0://effect
		{
			CEffectData* p = (CEffectData*)tree.GetItemData(hItem);
			p->SetExpand(action==TVE_EXPAND);
		}
		break;
	}
	//////

	*pResult = 0;
}

void CEffectTreeView::OnTvnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	if (pTVKeyDown->wVKey==VK_DELETE)
		OnElementDelete();
	*pResult = 0;
}
