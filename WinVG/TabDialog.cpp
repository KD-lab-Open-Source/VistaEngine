// TabDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WinVG.h"
#include "TabDialog.h"
#include "tabdialog.h"
#include "WinVGDoc.h"
#include "WinVGView.h"
#include ".\tabdialog.h"

void CTabDialog::GetRect(CWnd& wnd,CRect& rc)
{
	wnd.GetWindowRect(rc);
	ScreenToClient(rc);
}
void CTabDialog::SetPos(CWnd& wnd,CRect& rc)
{
	wnd.SetWindowPos(NULL,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,0);
}

// CTabDialog

IMPLEMENT_DYNCREATE(CTabDialog, CFormView)

CTabDialog::CTabDialog()
	: CFormView(CTabDialog::IDD)
{
	logic_obj=NULL;
	is_initialized=false;
	current_lod=0;
}

CTabDialog::~CTabDialog()
{
	ClearRoot();
}

void CTabDialog::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, treeAnimation);
	DDX_Control(pDX, IDC_TREE_VISIBILITY, treeVisibility);
	DDX_Control(pDX, IDC_CHAIN, chains);
	DDX_Control(pDX, IDC_LOD, lods);
	if(!pDX->m_bSaveAndValidate)
	{
		SetHeadVisibilityGroup();
		SetHeadAnimationGroup();
		is_initialized=true;
		TreeUpdate();
		UpdateSize();
	}
}

BEGIN_MESSAGE_MAP(CTabDialog, CFormView)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_CHAIN, OnCbnSelchangeChains)
	ON_CBN_SELCHANGE(IDC_LOD, OnCbnSelchangeLod)
END_MESSAGE_MAP()


// CTabDialog diagnostics

#ifdef _DEBUG
void CTabDialog::AssertValid() const
{
	CFormView::AssertValid();
}

void CTabDialog::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


// CTabDialog message handlers
void CTabDialog::TreeUpdate()
{
	if(!is_initialized)
		return;
	UpdateAnimationGroup();
	UpdateVisibilityGroup();
	UpdateAnimationGroupBox();
	UpdateLods();
}

void CTabDialog::SetRoot(cObject3dx* pUObj,CString _filename)
{
	ClearRoot();

	if(pUObj)
		UObj.push_back(pUObj);
	filename=_filename;
	filename.MakeUpper();
}

void CTabDialog::AddRoot(cObject3dx* pUObj)
{
	if(pUObj)
		UObj.push_back(pUObj);
}

void CTabDialog::SetLogicObj(cObject3dx* pUObj)
{
	RELEASE(logic_obj);
	logic_obj=pUObj;
}

cObject3dx* CTabDialog::GetRoot()
{
	if(UObj.empty())
		return NULL;
	return UObj.front();
}

void CTabDialog::ClearRoot()
{
	vector<cObject3dx*>::iterator it;
	FOR_EACH(UObj,it)
	if(*it)
		(*it)->Release();
	UObj.clear();

	RELEASE(logic_obj);
}

void CTabDialog::OnSize(UINT nType,int cx,int cy)
{
	CFormView::OnSize(nType,cx,cy);
	UpdateSize();
}

void CTabDialog::UpdateSize()
{
	CRect rc_main,rc_list;
	GetClientRect(rc_main);
	if(treeAnimation.m_hWnd)
	{
		GetRect(treeAnimation,rc_list);
		rc_list.left=rc_main.left;
		rc_list.right=rc_main.right;
		rc_list.bottom=rc_main.bottom;
		SetPos(treeAnimation,rc_list);
	}

	if(treeVisibility.m_hWnd)
	{
		GetRect(treeVisibility,rc_list);
		rc_list.left=rc_main.left;
		rc_list.right=rc_main.right;
		SetPos(treeVisibility,rc_list);
	}
}

void CTabDialog::SetHeadAnimationGroup()
{
	treeAnimation.SetStyle
		( 0
//		| TLC_TREELIST								// TreeList or List
		//		| TLC_BKGNDIMAGE							// image background
		//		| TLC_BKGNDCOLOR							// colored background ( not client area )
		| TLC_DOUBLECOLOR							// double color background

		| TLC_MULTIPLESELECT						// single or multiple select
		| TLC_SHOWSELACTIVE							// show active column of selected item
		| TLC_SHOWSELALWAYS							// show selected item always
		//		| TLC_SHOWSELFULLROWS						// show selected item in fullrow mode

		| TLC_HEADER								// show header
		| TLC_HGRID									// show horizonal grid lines
		| TLC_VGRID									// show vertical grid lines
		| TLC_TGRID									// show tree horizonal grid lines ( when HGRID & VGRID )
		| TLC_HGRID_EXT								// show extention horizonal grid lines
		| TLC_VGRID_EXT								// show extention vertical grid lines
		| TLC_HGRID_FULL							// show full horizonal grid lines
		//		| TLC_READONLY								// read only

		| TLC_TREELINE								// show tree line
		| TLC_ROOTLINE								// show root line
		| TLC_BUTTON								// show expand/collapse button [+]
		//		| TLC_CHECKBOX								// show check box
		//		| TLC_LOCKBOX								// show lock box
		| TLC_HOTTRACK								// show hover text 

		| TLC_DROPTHIS								// drop self support
		| TLC_DROPROOT								// drop on root support

		//		| TLC_HEADDRAGDROP							// head drag drop
		//		| TLC_HEADFULLDRAG							// head funn drag
		| TLC_NOAUTOCHECK							// do NOT auto set checkbox of parent & child
		//		| TLC_NOAUTOLOCK							// do NOT auto set lockbox of parent & child
		);

	if (treeAnimation.GetColumnCount() == 0)
	{
		treeAnimation.InsertColumn("Animation group", TLF_DEFAULT_LEFT, 90);
		treeAnimation.InsertColumn("chain", TLF_DEFAULT_LEFT, 100);
	}
	treeAnimation.SetColumnModify(1, TLM_COMBO);
	treeAnimation.setChangeItemLabelNotifyListener(this);
	treeAnimation.setItemChangeNotifyListener(this);

}

void CTabDialog::SetHeadVisibilityGroup()
{
	treeVisibility.SetStyle
		( 0
		| TLC_DOUBLECOLOR							// double color background

		| TLC_MULTIPLESELECT						// single or multiple select
		| TLC_SHOWSELACTIVE							// show active column of selected item
		| TLC_SHOWSELALWAYS							// show selected item always

		| TLC_HEADER								// show header
		| TLC_HGRID									// show horizonal grid lines
		| TLC_VGRID									// show vertical grid lines
		| TLC_TGRID									// show tree horizonal grid lines ( when HGRID & VGRID )
		| TLC_HGRID_EXT								// show extention horizonal grid lines
		| TLC_VGRID_EXT								// show extention vertical grid lines
		| TLC_HGRID_FULL							// show full horizonal grid lines

		| TLC_TREELINE								// show tree line
		| TLC_ROOTLINE								// show root line
		| TLC_BUTTON								// show expand/collapse button [+]
		| TLC_HOTTRACK								// show hover text 

		| TLC_DROPTHIS								// drop self support
		| TLC_DROPROOT								// drop on root support

		| TLC_NOAUTOCHECK							// do NOT auto set checkbox of parent & child
		);

	if (treeVisibility.GetColumnCount() == 0)
	{
		treeVisibility.InsertColumn("visibility set", TLF_DEFAULT_LEFT, 90);
		treeVisibility.InsertColumn("group", TLF_DEFAULT_LEFT, 100);
	}
	treeVisibility.SetColumnModify(1, TLM_COMBO);
	treeVisibility.setChangeItemLabelNotifyListener(this);
	treeVisibility.setItemChangeNotifyListener(this);
}

void CTabDialog::UpdateAnimationGroup()
{
	treeAnimation.DeleteAllItems();
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return;

	int ag_size=p3dx->GetAnimationGroupNumber();
	for(int iag=0;iag<ag_size;iag++)
	{
		const char* ag_name=p3dx->GetAnimationGroupName(iag);
		treeAnimation.InsertItem(iag,ag_name);
		int ichain=p3dx->GetAnimationGroupChain(iag);
		cAnimationChain* chain=p3dx->GetChain(ichain);
		treeAnimation.SetItemText( iag,1,chain->name.c_str());
	}
}

bool CTabDialog::onBeginControl(CTreeListCtrl& source, 
						CHANGE_LABEL_NOTIFY_INFO* pclns)
{
	if(&source==&treeAnimation)
		return onBeginControlAnimation(source,pclns);
	if(&source==&treeVisibility)
		return onBeginControlVisibility(source,pclns);
	return true;
}

bool CTabDialog::onBeginControlAnimation(CTreeListCtrl& source, 
					CHANGE_LABEL_NOTIFY_INFO* pclns)
{
	CComboBox* combo=(CComboBox*)pclns->pEditControl;
	if(!combo)
		return true;
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return false;

	combo->ResetContent();
	int chain_size=p3dx->GetChainNumber();
	for(int ichain=0;ichain<chain_size;ichain++)
	{
		cAnimationChain* chain=p3dx->GetChain(ichain);
		combo->AddString(chain->name.c_str());

	}

	int ag_size=p3dx->GetAnimationGroupNumber();
	CString group_name=pclns->pItem->GetText(0);
	for(int iag=0;iag<ag_size;iag++)
	{
		const char* ag_name=p3dx->GetAnimationGroupName(iag);
		if(strcmp(ag_name,(LPCSTR)group_name)==0)
		{
			int ichain=p3dx->GetAnimationGroupChain(iag);
			combo->SetCurSel(ichain);
		}
	}

	return true;
}

bool CTabDialog::onBeginControlVisibility(CTreeListCtrl& source, 
					CHANGE_LABEL_NOTIFY_INFO* pclns)
{
	CComboBox* combo=(CComboBox*)pclns->pEditControl;
	if(!combo)
		return true;
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return false;

	combo->ResetContent();
	C3dxVisibilitySet iset=C3dxVisibilitySet((int)pclns->pItem->GetData());

	cStaticVisibilitySet* pset=p3dx->GetVisibilitySet(iset);
	int vg_size=pset->visibility_groups[0].size();
	for(int ivg=0;ivg<vg_size;ivg++)
	{
		cStaticVisibilityChainGroup* pvg=pset->visibility_groups[0][ivg];
		combo->AddString(pvg->name.c_str());
	}

	C3dxVisibilityGroup vgi=p3dx->GetVisibilityGroupIndex(iset);
	combo->SetCurSel(vgi.igroup);
	return true;
}

bool CTabDialog::onEndLabelEdit(CTreeListCtrl& source, CHANGE_LABEL_NOTIFY_INFO* pclns)
{
	if(&source==&treeAnimation)
		return onEndLabelEditAnimation(source,pclns);
	if(&source==&treeVisibility)
		return onEndLabelEditVisibility(source,pclns);
	return true;
}

bool CTabDialog::onEndLabelEditAnimation(CTreeListCtrl& source, CHANGE_LABEL_NOTIFY_INFO* pclns)
{
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return true;

	if(pclns->text.IsEmpty())
		return true;

	int ag_size=p3dx->GetAnimationGroupNumber();
	CString group_name=pclns->pItem->GetText(0);
	for(int iag=0;iag<ag_size;iag++)
	{
		const char* ag_name=p3dx->GetAnimationGroupName(iag);
		if(strcmp(ag_name,(LPCSTR)group_name)==0)
		{
			vector<cObject3dx*>::iterator ito;
			FOR_EACH(UObj,ito)
				(*ito)->SetAnimationGroupChain(iag,pclns->text);
			if(logic_obj)
				logic_obj->SetAnimationGroupChain(iag,pclns->text);
		}
	}
	return true;
}

bool CTabDialog::onEndLabelEditVisibility(CTreeListCtrl& source, CHANGE_LABEL_NOTIFY_INFO* pclns)
{
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return true;
	if(pclns->text[0]==0)
		return true;
	C3dxVisibilitySet iset=C3dxVisibilitySet((int)pclns->pItem->GetData());
	cStaticVisibilitySet* pset=p3dx->GetVisibilitySet(iset);
	C3dxVisibilityGroup vgi=pset->GetVisibilityGroupIndex(pclns->text);
	if(vgi==C3dxVisibilityGroup::BAD)
		vgi.igroup=0;
	xassert(vgi.igroup>=0);
	vector<cObject3dx*>::iterator ito;
	FOR_EACH(UObj,ito)
		(*ito)->SetVisibilityGroup(vgi,iset);
	return true;
}

void CTabDialog::UpdateVisibilityGroup()
{
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return;

	treeVisibility.DeleteAllItems();
	int ag_size=p3dx->GetVisibilitySetNumber();
	for(int iag=0;iag<ag_size;iag++)
	{
		const char* ag_name=p3dx->GetVisibilitySetName(C3dxVisibilitySet(iag));
		CTreeListItem* pItem=treeVisibility.InsertItem(ag_name);
		treeVisibility.SetItemData(pItem, (DWORD_PTR)iag);
		cStaticVisibilityChainGroup* pgroup=p3dx->GetVisibilityGroup(C3dxVisibilitySet(iag));
		treeVisibility.SetItemText( pItem,1,pgroup->name.c_str());
	}
}
void CTabDialog::OnCbnSelchangeChains()
{
	if(GetRoot()==NULL)
		return;

	int ichain=chains.GetCurSel();
	if(ichain<0)
	{
		xassert(0);
		return;
	}

	vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;

	FOR_EACH(obj,it)
	{ 
		cObject3dx* Obj=*it;
		int ag_size=Obj->GetAnimationGroupNumber();
		for(int iag=0;iag<ag_size;iag++)
		{
			Obj->SetAnimationGroupChain(iag,ichain);
			if(logic_obj)
			{
				cAnimationChain* chain = Obj->GetChain(ichain);
				if(chain)
				{
					logic_obj->SetAnimationGroupChain(iag,chain->name.c_str());
				}
			}
		}
	}

	cObject3dx* Obj=pDoc->m_pHierarchyObj->GetRoot();
	int ag_size=Obj->GetAnimationGroupNumber();
	cAnimationChain* chain = Obj->GetChain(ichain);
	if(logic_obj && chain)
	for(int iag=0;iag<ag_size;iag++)
	{
		logic_obj->SetAnimationGroupChain(iag,chain->name.c_str());
	}

	UpdateAnimationGroup();
	pView->UpdateFramePeriod();
}

void CTabDialog::UpdateAnimationGroupBox()
{
	chains.ResetContent();
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return;

	int ch_size=p3dx->GetChainNumber();
	for(int ich=0;ich<ch_size;ich++)
	{
		cAnimationChain* cur=p3dx->GetChain(ich);
		chains.AddString(cur->name.c_str());
	}

	if(p3dx->GetAnimationGroupNumber()>0)
	{
		int ichain=p3dx->GetAnimationGroupChain(0);
		chains.SetCurSel(ichain);
	}
}

void CTabDialog::UpdateLods()
{
	lods.ResetContent();
	lods.AddString("lod01");
	lods.AddString("lod02");
	lods.AddString("lod03");
	lods.SetCurSel(0);
	current_lod=0;
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return;
	lods.EnableWindow(p3dx->GetStatic()->is_lod);
}

void CTabDialog::OnCbnSelchangeLod()
{
	cObject3dx* p3dx=GetRoot();
	if(p3dx==NULL)
		return;
	int ilod=lods.GetCurSel();
	if(ilod<0)
		return;
	current_lod=ilod;
	ReselectLod();
}

void CTabDialog::ReselectLod()
{
	xassert(current_lod>=0 && current_lod<3);
	vector<cObject3dx*>::iterator ito;
	FOR_EACH(UObj,ito)
		(*ito)->SetLod(current_lod);
}
