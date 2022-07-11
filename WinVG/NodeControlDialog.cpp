// NodeControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WinVG.h"
#include "NodeControlDialog.h"
#include "MainFrm.h"
#include ".\nodecontroldialog.h"
#include "WinVGDoc.h"
#include "TabDialog.h"


// CNodeControlDialog dialog

IMPLEMENT_DYNAMIC(CNodeControlDialog, CDialog)
CNodeControlDialog::CNodeControlDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CNodeControlDialog::IDD, pParent)
{
}

CNodeControlDialog::~CNodeControlDialog()
{
}

void CNodeControlDialog::UpdateInfo()
{

	m_XStatic.Format("X Transform = %d",m_XSlider.GetPos()-180);
	m_YStatic.Format("Y Transform = %d",m_YSlider.GetPos()-180);
	m_ZStatic.Format("Z Transform = %d",m_ZSlider.GetPos()-180);

	UpdateData(FALSE);

/*

	sprintf(bu,"Y Transform = %d\0",m_YSlider.GetPos()-180);
	m_YStatic.SetWindowText((char*)(&bu[0]));

	sprintf(bu,"Z Transform = %d\0",m_ZSlider.GetPos()-180);
	m_ZStatic.SetWindowText((char*)(&bu[0]));*/
}

void CNodeControlDialog::ModelChange(cObject3dx* obj,cObject3dx* logic_obj)
{
	m_NodeList.ResetContent();
	m_LogicNodeList.ResetContent();
	pObject3dx = obj;
	pLogicObject3dx = logic_obj;
	if(obj)
	{
		vector<cStaticNode>::iterator i;
		int pos = 0;
		FOR_EACH(pObject3dx->GetStatic()->nodes, i)
		{
			m_NodeList.AddString(i->name.c_str());
			pos++;
		}
		m_XSlider.SetRange(0,360);
		m_YSlider.SetRange(0,360);
		m_ZSlider.SetRange(0,360);
		m_XSlider.SetPos(180);
		m_YSlider.SetPos(180);
		m_ZSlider.SetPos(180);
		UpdateInfo();
	}
	if(logic_obj)
	{
		vector<cStaticNode>::iterator i;
		int pos = 0;
		FOR_EACH(pLogicObject3dx->GetStatic()->nodes, i)
		{
			m_LogicNodeList.AddString(i->name.c_str());
			pos++;
		}
	}
}

void CNodeControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NODE_LIST, m_NodeList);
	DDX_Control(pDX, IDC_X_SLIDER, m_XSlider);
	DDX_Control(pDX, IDC_Y_SLIDER, m_YSlider);
	DDX_Control(pDX, IDC_Z_SLIDER, m_ZSlider);
	DDX_Text(pDX, IDC_X_STATIC, m_XStatic);
	DDX_Text(pDX, IDC_Y_STATIC, m_YStatic);
	DDX_Text(pDX, IDC_Z_STATIC, m_ZStatic);
	DDX_Control(pDX, IDC_LOGIC_NODE_LIST, m_LogicNodeList);
	DDX_Control(pDX, IDC_CHECK_GRAPH_BY_LOGIC_NODE, check_graph_by_logic);
}


BEGIN_MESSAGE_MAP(CNodeControlDialog, CDialog)
	ON_WM_HSCROLL()
	ON_LBN_SELCHANGE(IDC_NODE_LIST, OnLbnSelchangeNodeList)
	ON_BN_CLICKED(IDC_RESET_BUTTON, OnBnClickedResetButton)
	ON_BN_CLICKED(IDC_RESET_SELECTED_BUTTON, OnBnClickedReselSelectedButton)
	ON_BN_CLICKED(IDC_DESELECT_LOGIC_NODE, OnBnClickedDeselectLogicNode)
	ON_BN_CLICKED(IDC_DESELECT_NODE, OnBnClickedDeselectNode)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_BY_LOGIC_NODE, OnBnClickedCheckGraphByLogicNode)
END_MESSAGE_MAP()


// CNodeControlDialog message handlers

void CNodeControlDialog::OnNMThemeChangedXSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	// This feature requires Windows XP or greater.
	// The symbol _WIN32_WINNT must be >= 0x0501.

	*pResult = 0;
}

Se3f CNodeControlDialog::nodePose_X(float ang)
{
	Se3f R(QuatF(ang, Vect3f::I), Vect3f::ZERO);
	return R;
}

Se3f CNodeControlDialog::nodePose_Y(float ang)
{
	Se3f R(QuatF(ang, Vect3f::J), Vect3f::ZERO);
	return R;
}

Se3f CNodeControlDialog::nodePose_Z(float ang)
{
	Se3f R(QuatF(ang, Vect3f::K), Vect3f::ZERO);
	return R;
}

void CNodeControlDialog::Update()
{
	int cur_sel = m_NodeList.GetCurSel();
	int logic_cur_sel = m_LogicNodeList.GetCurSel();
	r_x = (m_XSlider.GetPos()*M_PI/180.0f)-M_PI;
	r_y = (m_YSlider.GetPos()*M_PI/180.0f)-M_PI;
	r_z = (m_ZSlider.GetPos()*M_PI/180.0f)-M_PI;

	/*
	Если есть и логическая и графическая ноды, то вращать графическую по осям логической.
	*/
	bool is_logic_transform=false;
	Mats logic_transform;
	logic_transform.Identify();
	
	if(check_graph_by_logic.GetCheck()==BST_CHECKED)
	if (cur_sel > -1)
	{
		cObject3dx* obj=pDoc->m_pHierarchyObj->GetRoot();
		string name=obj->GetNodeName(cur_sel);
		name+="_logic";
		int cur_sel_additional=obj->FindNode(name.c_str());
		if(cur_sel_additional>-1)
		{
			int parent;
			obj->GetNodeOffset(cur_sel_additional,logic_transform,parent);
			if(parent==cur_sel)
				is_logic_transform=true;
			else
			{
				xassert(0 && "Неправильно прилинкована нода _logic");
			}
		}
	}


	Se3f rotation=nodePose_X(r_x)*nodePose_Y(r_y)*nodePose_Z(r_z);

	Se3f logic_rotation=Se3f::ID;

	if (cur_sel > -1)
	{
		vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
		vector<cObject3dx*>::iterator it;
		//graph*additional_rotation*rot*inv_additional_rotation

		if(is_logic_transform)
		{
			QuatF inv_logic_rotation;
			inv_logic_rotation.invert(logic_transform.rot());
			logic_rotation.rot().mult(logic_transform.rot(),rotation.rot());
			logic_rotation.rot().postmult(inv_logic_rotation);
			logic_rotation.trans()=Vect3f::ZERO;

			FOR_EACH(obj,it)
			{ 
				cObject3dx* Obj=*it;
				Obj->SetUserTransform(cur_sel,logic_rotation);
			}
		}else
		{
			FOR_EACH(obj,it)
			{ 
				cObject3dx* Obj=*it;
				Obj->SetUserTransform(cur_sel,rotation);
			}
		}
	}
		
	if (logic_cur_sel > -1 && pLogicObject3dx)
	{
		if(is_logic_transform)
		{
			pLogicObject3dx->SetUserTransform(logic_cur_sel,logic_rotation);
		}else
		{
			pLogicObject3dx->SetUserTransform(logic_cur_sel,rotation);
		}
	}

	UpdateInfo();
}

void CNodeControlDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	DWORD id = pScrollBar->GetDlgCtrlID();
	Update();
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CNodeControlDialog::OnLbnSelchangeNodeList()
{
	m_XSlider.SetPos(180);
	m_YSlider.SetPos(180);
	m_ZSlider.SetPos(180);
	UpdateInfo();
}

void CNodeControlDialog::OnBnClickedResetButton()
{
	vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;

	FOR_EACH(obj,it)
	{ 
		cObject3dx* Obj=*it;
		if(Obj)
		for(int i = 0; i<Obj->GetStatic()->nodes.size(); i++)
			Obj->RestoreUserTransform(i);
	}

	if(pLogicObject3dx)
	{
		for(int i = 0; i<pLogicObject3dx->GetStatic()->nodes.size(); i++)
			pLogicObject3dx->RestoreUserTransform(i);
	}
	m_XSlider.SetPos(180);
	m_YSlider.SetPos(180);
	m_ZSlider.SetPos(180);
	UpdateInfo();
}

void CNodeControlDialog::OnBnClickedReselSelectedButton()
{
	int cur_sel = m_NodeList.GetCurSel();
	int logic_cur_sel = m_LogicNodeList.GetCurSel();
	if (cur_sel > -1)
	{
		vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
		vector<cObject3dx*>::iterator it;

		FOR_EACH(obj,it)
		{ 
			cObject3dx* Obj=*it;
			if(Obj)
				Obj->RestoreUserTransform(cur_sel);
		}
	}
	if (logic_cur_sel > -1)
	{
		if(pLogicObject3dx)
		{
			pLogicObject3dx->RestoreUserTransform(logic_cur_sel);
		}
	}
	m_XSlider.SetPos(180);
	m_YSlider.SetPos(180);
	m_ZSlider.SetPos(180);
	UpdateInfo();
}

void CNodeControlDialog::OnBnClickedDeselectLogicNode()
{
	m_LogicNodeList.SetCurSel(-1);
}

void CNodeControlDialog::OnBnClickedDeselectNode()
{
	m_NodeList.SetCurSel(-1);
}

void CNodeControlDialog::OnBnClickedCheckGraphByLogicNode()
{
	Update();
}

//BOOL CNodeControlDialog::OnInitDialog()
//{
//	CDialog::OnInitDialog();
//
//	phaseSlider.
//	return TRUE;  // return TRUE unless you set the focus to a control
//	// EXCEPTION: OCX Property Pages should return FALSE
//}
