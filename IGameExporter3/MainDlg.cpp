// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MainDlg.h"
#include "maindlg.h"
#include "resource.h"

#include "AnimationGroupDlg.h"
#include "AnimationGroupMatDlg.h"
#include "AnimationChainDlg.h"
#include "VisibleGroupDlg.h"
#include "DebugDlg.h"
#include "LogicNodeDlg.h"
#include "MoveEmblem.h"

// CMainDlg dialog

IMPLEMENT_DYNAMIC(CMainDlg, CDialog)
CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainDlg::IDD, pParent)
	, m_MaxWeight(0)
{
}

CMainDlg::~CMainDlg()
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MAX_WEIGHTS, m_MaxWeight);
	DDX_Control(pDX, IDC_MAX_WEIGHTS_SPIN, m_MaxWeightSpin);
	if(!pDX->m_bSaveAndValidate)
		SetWindowText("3dx Exporter (" __DATE__ ")");

}


BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
	ON_BN_CLICKED(IDC_ANIMATION_GROUP, OnBnClickedAnimationGroup)
	ON_BN_CLICKED(IDC_ANIMATION_CHAIN, OnBnClickedAnimationChain)
	ON_BN_CLICKED(IDC_ANIMATION_GROUP_MAT, OnBnClickedAnimationGroupMat)
	ON_BN_CLICKED(IDC_NONDELETE_NODE, OnBnClickedNondeleteNode)
	ON_BN_CLICKED(IDC_DEBUG, OnBnClickedDebug)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MAX_WEIGHTS_SPIN, OnDeltaposMaxWeightsSpin)
	ON_BN_CLICKED(IDC_VISIBLE_GROUP, OnBnClickedVisibleGroup)
	ON_BN_CLICKED(IDC_LOGIC_NODE, OnBnClickedLogicNode)
	ON_BN_CLICKED(IDC_PLACE_EMBLEM, OnBnClickedPlaceEmblem)
	ON_BN_CLICKED(IDC_BOUND_NODE, OnBnClickedBoundNode)
	ON_BN_CLICKED(ID_EXPORT_LOD1, OnBnClickedLod1)
	ON_BN_CLICKED(ID_EXPORT_LOD2, OnBnClickedLod2)
END_MESSAGE_MAP()


// CMainDlg message handlers

void CMainDlg::OnBnClickedAnimationGroup()
{
	CAnimationGroupDlg dlg;
	dlg.DoModal();
}

void CMainDlg::OnBnClickedAnimationChain()
{
	CAnimationChainDlg dlg;
	dlg.DoModal();
}

void CMainDlg::OnBnClickedAnimationGroupMat()
{
	CAnimationGroupMatDlg dlg;
	dlg.DoModal();
}

void CMainDlg::OnBnClickedDebug()
{
	CDebugDlg dlg;
	dlg.DoModal();
}

BOOL CMainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_MaxWeight = pRootExport->GetMaxWeights();
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMainDlg::OnDeltaposMaxWeightsSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	UpdateData();
	m_MaxWeight -= pNMUpDown->iDelta;
	if (m_MaxWeight > 4)
		m_MaxWeight = 4;
	if (m_MaxWeight < 1)
		m_MaxWeight = 1;
	UpdateData(FALSE);
	*pResult = 0;
}

void CMainDlg::OnOK()
{
	UpdateData();
	pRootExport->SetMaxWeights(m_MaxWeight);
	CDialog::OnOK();
}

void CMainDlg::OnBnClickedLod1()
{
	OnOK();
	EndDialog(ID_EXPORT_LOD1);
}

void CMainDlg::OnBnClickedLod2()
{
	OnOK();
	EndDialog(ID_EXPORT_LOD2);
}


void CMainDlg::OnBnClickedVisibleGroup()
{
	CVisibleGroupDlg dlg;
	dlg.DoModal();
	
}

void CMainDlg::OnBnClickedLogicNode()
{
	CLogicNodeDlg dlg(pRootExport->logic_node,"Логические Node");
	dlg.DoModal();

}

void CMainDlg::OnBnClickedNondeleteNode()
{
	CLogicNodeDlg dlg(pRootExport->nondelete_node,"Не удаляемые при оптимизации Node");
//	CNonDeleteDlg dlg;
	dlg.DoModal();
}

void CMainDlg::OnBnClickedPlaceEmblem()
{
	cMoveEmblem dlg;
	dlg.DoModal();
}

void CMainDlg::OnBnClickedBoundNode()
{
	CLogicNodeDlg dlg(pRootExport->bound_node,"Bound Node");
	dlg.DoModal();
}

