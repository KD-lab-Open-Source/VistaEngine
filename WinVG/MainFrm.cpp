// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WinVG.h"

#include "MainFrm.h"

#include "WinVGDoc.h"
#include "WinVGView.h"
#include "TabDialog.h"
#include "DirectoryTree.h"
#include "Hierarhy.h"
#include "mainfrm.h"
#include "afxcmn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern cCamera		*gb_Camera1;
CMainFrame*			gb_FrameWnd=NULL;
extern bool			g_bPressLighing;
//ID_LIGHT_ON
/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_BUTTON_BBOX, OnButtonBbox)
	ON_COMMAND(ID_BUTTON_BOUND, OnButtonBound)
	ON_COMMAND(ID_BUTTON_LOGIC, OnButtonLogic)
	ON_COMMAND(ID_BUTTON_OBJECT, OnButtonObject)
	ON_WM_SIZE()
	ON_COMMAND(ID_BUTTON_LIGHTING, OnButtonLighting)
	ON_COMMAND(ID_MENU_CONTROL_CAMERA, OnMenuControlCamera)
	ON_COMMAND(ID_MENU_CONTROL_OBJECT, OnMenuControlObject)
	ON_COMMAND(ID_MENU_CONTROL_DIRLIGHT, OnMenuControlDirlight)
	ON_COMMAND(ID_BUTTON_COLOR, OnButtonColor)
	ON_COMMAND(ID_BUTTON_BKCOLOR, OnButtonBkcolor)
	ON_COMMAND(ID_DEBUG_MEMORY, OnDebugMemory)
	ON_COMMAND(ID_NUM_OBJECT, OnNumObject)
	ON_UPDATE_COMMAND_UI(ID_NUM_OBJECT, OnUpdateNumObject)
	ON_COMMAND(ID_HOLOGRAM, OnHologram)
	ON_UPDATE_COMMAND_UI(ID_HOLOGRAM, OnUpdateHologram)
	ON_COMMAND(ID_SCALENORMAL, OnScalenormal)
	ON_UPDATE_COMMAND_UI(ID_SCALENORMAL, OnUpdateScalenormal)
	ON_COMMAND(ID_BUTTON_SHADOW, OnButtonShadow)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SHADOW, OnUpdateButtonShadow)
	ON_COMMAND(ID_ENABLE_BUMP, OnEnableBump)
	ON_UPDATE_COMMAND_UI(ID_ENABLE_BUMP, OnUpdateEnableBump)
	ON_COMMAND(ID_EFFECT_DIRECTORY, OnEffectDirectory)
	ON_COMMAND(ID_MODEL_INFO, OnModelInfo)
	ON_COMMAND(ID_ENABLE_ANISOTROPIC, OnEnableAnisotropic)
	ON_UPDATE_COMMAND_UI(ID_ENABLE_ANISOTROPIC, OnUpdateEnableAnisotropic)
	ON_COMMAND(ID_BUTTON_BASEMENT, OnButtonBasement)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_BASEMENT, OnUpdateButtonBasement)
	ON_COMMAND(ID_SCREEN_SHOOT, OnScreenShoot)
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(ID_ENABLE_TILEMAP, OnEnableTilemap)
	ON_UPDATE_COMMAND_UI(ID_ENABLE_TILEMAP, OnUpdateEnableTilemap)
	ON_UPDATE_COMMAND_UI(ID_PIXNORM, OnUpdatePixNorm)
	ON_COMMAND(ID_PIXNORM, OnPixNorm)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_LIGHT_ON, OnLightOn)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_ON,OnUpdateLightOn)
	ON_COMMAND(ID_HIERARHY_OBJECT, OnHierarhyObject)
	ON_COMMAND(ID_HIERARHY_LOGIC, OnHierarhyLogic)

	ON_UPDATE_COMMAND_UI(ID_MODEL_CAMERA, OnUpdateModelCamera)
	ON_COMMAND(ID_MODEL_CAMERA, OnModelCamera)

	ON_UPDATE_COMMAND_UI(ID_CAMERA43, OnUpdateCamera43)
	ON_COMMAND(ID_CAMERA43, OnCamera43)

	ON_UPDATE_COMMAND_UI(ID_ZERO_PLANE, OnUpdateZeroPlane)
	ON_COMMAND(ID_ZERO_PLANE, OnZeroPlane)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_TIME, OnUpDateButtonTime)
	ON_COMMAND(ID_BUTTON_TIME, OnButtonTime)

	ON_UPDATE_COMMAND_UI(ID_DRAW_SCELETON, OnUpdateDrawSceleton)
	ON_COMMAND(ID_DRAW_SCELETON, OnDrawSceleton)

	ON_COMMAND(ID_BUTTON_PAUSE, OnPause)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PAUSE, OnUpdatePause)

	ON_COMMAND(ID_BUTTON_REVERSE, OnReverse)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REVERSE, OnUpdateReverse)

	ON_COMMAND(ID_SHOW_DEBRISES, OnShowDebrises)
	ON_UPDATE_COMMAND_UI(ID_SHOW_DEBRISES, OnUpdateShowDebrises)
	
	
	ON_WM_DESTROY()
	ON_COMMAND(ID_VIEW_AUTOMATICMIPMAP, OnViewAutomaticmipmap)
	ON_UPDATE_COMMAND_UI(ID_VIEW_AUTOMATICMIPMAP, OnUpdateViewAutomaticmipmap)
	ON_COMMAND(ID_VIEW_0MIPLEVEL, OnView0miplevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_0MIPLEVEL, OnUpdateView0miplevel)
	ON_COMMAND(ID_VIEW_1MIPLEVEL, OnView1miplevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_1MIPLEVEL, OnUpdateView1miplevel)
	ON_COMMAND(ID_VIEW_2MIPLEVEL, OnView2miplevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_2MIPLEVEL, OnUpdateView2miplevel)
	ON_COMMAND(ID_VIEW_3MIPLEVEL, OnView3miplevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_3MIPLEVEL, OnUpdateView3miplevel)
	ON_COMMAND(ID_VIEW_FORCEMIPCOLOR, OnViewForcemipcolor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FORCEMIPCOLOR, OnUpdateViewForcemipcolor)

	ON_COMMAND(ID_BUTTON_FOG, OnFog)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_FOG, OnUpdateFog)
	ON_COMMAND(ID_VIEW_NODECONTROL, OnViewNodecontrol)
	ON_COMMAND(ID_PHASE_CONTROL, OnPhaseControl)

	ON_COMMAND(ID_SHOW_NORMALS, OnViewNormal)
	ON_UPDATE_COMMAND_UI(ID_SHOW_NORMALS, OnUpdateViewNormal)
	ON_COMMAND(ID_LOGOOPEN, OnButtonLogoOpen)
	
END_MESSAGE_MAP()



static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CHAIN_TIME,
	ID_INDICATOR_FPS,
	ID_INDICATOR_PHASE,
	ID_INDICATOR_MEGA_POLY,
	ID_INDICATOR_POLY,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	extern void InitAllockHook();
	InitAllockHook();
	
	m_bPressBBox=m_bPressBound=m_bPressLogic=TRUE;
	m_bPressObject=FALSE;
	m_bInit=FALSE;
	gb_FrameWnd=this;
	use_shadow=false;
	scale_normal=true;
	effect_directory=GetRegistryString("effect_directory","");

	skin_color.set(1,0,0,1);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		/*| CBRS_GRIPPER*/ | CBRS_TOOLTIPS /*| CBRS_FLYBY | CBRS_SIZE_DYNAMIC*/) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}


	EnableDocking(CBRS_ALIGN_ANY);
//	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
//	DockControlBar(&m_wndToolBar);

	OnButtonBbox();
	OnButtonBound();
	OnButtonLogic();
	OnButtonObject();
	OnButtonLighting();

	CheckControlSwitch();
	ncdlg.Create(IDD_NODE_CONTROL_DIALOG, this);
	phaseDlg.Create(IDD_PHASE_CONTROL_DIALOG,this);

	time_slider_dialog.Create(CSetTimeDialog::IDD, this);

	SetWindowPos(NULL,GetRegistryInt("WindowPosX",0),GetRegistryInt("WindowPosY",0),GetRegistryInt("WindowWidth",800),GetRegistryInt("WindowHeight",600),0);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	if(!m_Splitter0.CreateStatic(this,1,3))
		return FALSE;
	if( !m_Splitter0.CreateView(0,0,RUNTIME_CLASS(CTabDialog),CSize(100,100),pContext)||
		!m_Splitter0.CreateView(0,1,RUNTIME_CLASS(CWinVGView),CSize(100,100),pContext)||
		!m_Splitter0.CreateView(0,2,RUNTIME_CLASS(CDirectoryTree),CSize(100,100),pContext))
	{
		m_Splitter0.DestroyWindow();
		return FALSE;
	}
	return m_bInit=TRUE; //CFrameWnd::OnCreateClient(lpcs, pContext);
}


void CMainFrame::OnButtonBbox()
{
	m_bPressBBox=!m_bPressBBox;
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUTTON_BBOX,m_bPressBBox);
}

void CMainFrame::OnButtonBound() 
{
	m_bPressBound=!m_bPressBound;
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUTTON_BOUND,m_bPressBound);
}
void CMainFrame::OnButtonLogic() 
{
	m_bPressLogic=!m_bPressLogic;
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUTTON_LOGIC,m_bPressLogic);
}
void CMainFrame::OnButtonObject() 
{
	m_bPressObject=!m_bPressObject;
	pView->UpdateIgnore();
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUTTON_OBJECT,m_bPressObject);
}

void CMainFrame::OnButtonLighting() 
{
	g_bPressLighing=!g_bPressLighing;
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUTTON_LIGHTING,g_bPressLighing);

	pView->UpdateObjectLight();
}
void CMainFrame::OnButtonColor() 
{
	CWinVGDoc *Doc=(CWinVGDoc*)GetActiveDocument();
	CColorDialog ColorDlg;
	if(ColorDlg.DoModal()==IDCANCEL) return;
	if(Doc==0||Doc->m_pView==0||Doc->m_pHierarchyObj==0||Doc->m_pHierarchyObj->GetRoot()==0) return;
	int color=ColorDlg.GetColor();
	sColor4c c((color>>0)&0xFF,(color>>8)&0xFF,(color>>16)&0xFF,255);
	skin_color=sColor4f(c);
	pView->SetSkinColor(skin_color,logo_image_name);
}

void CMainFrame::OnButtonLogoOpen() 
{
	CFileDialog dlg(true,NULL,NULL/*effect_directory.c_str()*/,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			"*.tga|*.tga||",this);
	if(dlg.DoModal()!=IDOK)
		return;
	logo_image_name=dlg.GetPathName();
	pView->SetSkinColor(skin_color,logo_image_name);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	if(m_bInit)
	{
		int size0=202,size2=150;

		m_Splitter0.SetColumnInfo(0,202,1);
		m_Splitter0.SetColumnInfo(1,max(cx-size0-size2,10),1);
		m_Splitter0.SetActivePane(0,1);
		m_Splitter0.RecalcLayout();
	}
	CFrameWnd::OnSize(nType, cx, cy);
}

void CMainFrame::CheckControlSwitch()
{
	int s=pView->m_ControlSwitch;
	GetMenu()->CheckMenuItem(ID_MENU_CONTROL_OBJECT,s==0?MF_CHECKED:MF_UNCHECKED);
	GetMenu()->CheckMenuItem(ID_MENU_CONTROL_CAMERA,s==1?MF_CHECKED:MF_UNCHECKED);
	GetMenu()->CheckMenuItem(ID_MENU_CONTROL_DIRLIGHT,s==2?MF_CHECKED:MF_UNCHECKED);
}

void CMainFrame::OnMenuControlObject() 
{
	if(!pView) return;
	pView->m_ControlSwitch=0;
	CheckControlSwitch();
}
void CMainFrame::OnMenuControlCamera() 
{
	if(!pView) return;
	pView->m_ControlSwitch=1;
	CheckControlSwitch();
}
void CMainFrame::OnMenuControlDirlight() 
{
	if(!pView) return;
	pView->m_ControlSwitch=2;
	CheckControlSwitch();
}

void CMainFrame::OnButtonBkcolor() 
{
	CWinVGDoc *Doc=(CWinVGDoc*)GetActiveDocument();
	CColorDialog ColorDlg;
	if(ColorDlg.DoModal()==IDCANCEL) return;
	if(Doc==0||Doc->m_pView==0) return;
	int color=ColorDlg.GetColor();
	Doc->m_pView->Color.set((color>>0)&0xFF,(color>>8)&0xFF,(color>>16)&0xFF,255);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask) 
{
	CFrameWnd::OnActivateApp(bActive, hTask);
	theApp.SetActive(bActive);
}


void CMainFrame::OnDebugMemory() 
{
	extern void DebugMemInfo();
	DebugMemInfo();
}

void CMainFrame::OnNumObject() 
{
	pView->OnNumObject();
}

void CMainFrame::OnUpdateNumObject(CCmdUI* pCmdUI) 
{
	pView->OnUpdateNumObject(pCmdUI);
}

void CMainFrame::OnHologram() 
{
	pView->OnHologram();
}

void CMainFrame::OnUpdateHologram(CCmdUI* pCmdUI) 
{
	pView->OnUpdateHologram(pCmdUI);
}

void CMainFrame::OnPause()
{
	pView->OnPause();
}
void CMainFrame::OnUpdatePause(CCmdUI* pCmdUI)
{
	pView->OnUpdatePause(pCmdUI);
}
bool CMainFrame::IsReverse()
{
	return pView->IsReverse();
}

void CMainFrame::OnReverse()
{
	pView->OnReverse();
}
void CMainFrame::OnUpdateReverse(CCmdUI* pCmdUI)
{
	pView->OnUpdateReverse(pCmdUI);
}

void CMainFrame::OnShowDebrises()
{
	pView->OnShowDebrises();
}
void CMainFrame::OnUpdateShowDebrises(CCmdUI* pCmdUI)
{
	pView->OnUpdateShowDebrises(pCmdUI);
}

void CMainFrame::OnScalenormal() 
{
	scale_normal=!scale_normal;
	pView->SetScale(scale_normal);
}

void CMainFrame::OnUpdateScalenormal(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(!scale_normal);
}

void SetUseShadow(bool use);
void CMainFrame::OnButtonShadow() 
{
	use_shadow=!use_shadow;
	SetUseShadow(use_shadow);
}

void CMainFrame::OnUpdateButtonShadow(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(use_shadow?1:0);
}

void CMainFrame::OnEnableBump() 
{
	gb_VisGeneric->SetEnableBump(!gb_VisGeneric->GetEnableBump());
}

void CMainFrame::OnUpdateEnableBump(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(gb_VisGeneric->GetEnableBump()?1:0);
}

void CMainFrame::LoadRegistry()
{
	use_shadow=GetRegistryInt("use_shadow", false)?true:false;
	scale_normal=GetRegistryInt("scale_normal", true)?true:false;
	gb_VisGeneric->SetEnableBump(GetRegistryInt("enable_bump",true));
	SetUseShadow(use_shadow);
	pView->SetScale(scale_normal);

	pView->m_ControlSwitch=GetRegistryInt("m_ControlSwitch",0);
	CheckControlSwitch();
	pView->enable_tilemap=GetRegistryInt("enable_tilemap", false)?true:false;
}

void CMainFrame::SaveRegistry()
{
	WriteRegistryInt("use_shadow",use_shadow);
	WriteRegistryInt("scale_normal",scale_normal);
	WriteRegistryInt("enable_bump",gb_VisGeneric->GetEnableBump());
	WriteRegistryInt("m_ControlSwitch",pView->m_ControlSwitch);
	WriteRegistryInt("enable_tilemap",pView->enable_tilemap);
}

void CMainFrame::OnEffectDirectory() 
{
	CFileDialog dlg(true,NULL,NULL/*effect_directory.c_str()*/,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			"*.effect|*.effect||",this);
	if(dlg.DoModal()!=IDOK)
		return;

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	CString fin=dlg.GetPathName();
	_splitpath(fin,drive,dir,fname,ext);
	effect_directory=drive+string(dir);
	WriteRegistryString("effect_directory",effect_directory.c_str());
	pView->SetEffectDirectory(effect_directory.c_str());
}

void CMainFrame::OnModelInfo() 
{
	pView->ModelInfo();
}

void CMainFrame::OnEnableAnisotropic() 
{
	gb_VisGeneric->SetAnisotropic(gb_VisGeneric->GetAnisotropic()?0:3);
	
}

void CMainFrame::OnUpdateEnableAnisotropic(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(gb_VisGeneric->GetAnisotropic());
}

void CMainFrame::OnButtonBasement() 
{
	pView->show_basement=!pView->show_basement;
	
}

void CMainFrame::OnUpdateButtonBasement(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pView->show_basement);
}

void CMainFrame::OnScreenShoot() 
{
	pView->OnScreenShoot();
}

void CMainFrame::OnEnableTilemap() 
{
	pView->enable_tilemap=!pView->enable_tilemap;
	gb_VisGeneric->SetShowType(SHOW_TILEMAP,pView->enable_tilemap);
}

void CMainFrame::OnUpdateEnableTilemap(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pView->enable_tilemap);
	
}

void CMainFrame::OnUpdatePixNorm(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((bool)pDoc->m_pHierarchyObj && pDoc->m_pHierarchyObj->GetRoot());
}

void CMainFrame::OnPixNorm()
{
}

void CMainFrame::OnCloseBumpDialog()
{
}


void CMainFrame::OnLightOn()
{
	pView->light_on=!pView->light_on;
	pView->LightUpdate();
}

void CMainFrame::OnUpdateLightOn(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pView->light_on);
}

void CMainFrame::OnHierarhyObject()
{
	if(!pDoc->m_pHierarchyObj)
		return;
	cObject3dx* pObject=pDoc->m_pHierarchyObj->GetRoot();
	if(pObject==NULL)
		return;
	CHierarhy dlg(pObject,this);
	dlg.selected_object=pView->selected_graph_node;
	dlg.DoModal();
	pView->selected_graph_node=dlg.selected_object;
}

void CMainFrame::OnHierarhyLogic()
{
	if(!pDoc->m_pHierarchyObj)
		return;
	cObject3dx* pObject=pDoc->m_pHierarchyObj->GetLogicObj();
	if(pObject==NULL)
		return;
	CHierarhy dlg(pObject,this);
	dlg.selected_object=pView->selected_logic_node;
	dlg.DoModal();
	pView->selected_logic_node=dlg.selected_object;
}

void CMainFrame::OnUpdateCamera43(CCmdUI* pCmdUI)
{
	cObject3dx* pObject = pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
	bool enable = false;
	if (pObject)
	{
		if (pObject->FindNode("Camera01")!=-1)
			enable = true;
		if (!enable && pDoc && pDoc->IsCamera43())
			OnCamera43();
	}
	pCmdUI->Enable(enable);
}
void CMainFrame::OnCamera43()
{
	cObject3dx* pObject = pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
	if (pObject)
	{
		pDoc->IsCamera43() = !pDoc->IsCamera43();
		pDoc->m_pView->UpdateCameraFrustum();
		m_wndToolBar.GetToolBarCtrl().CheckButton(ID_CAMERA43, pDoc->IsCamera43());
		if (!pDoc->IsCamera43())
			pDoc->m_pView->SetCameraPosition(0,0,1);
	}
}

void CMainFrame::OnUpdateModelCamera(CCmdUI* pCmdUI)
{
	cObject3dx* pObject = pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
	bool enable = false;
	if (pObject)
	{
		if (pObject->FindNode("Camera01")!=-1)
			enable = true;
		if (!enable && pDoc && pDoc->IsModelCamera())
			OnModelCamera();
	}
	pCmdUI->Enable(enable);
}
void CMainFrame::OnModelCamera()
{
	cObject3dx* pObject = pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
	if (pObject)
	{
		pDoc->IsModelCamera() = !pDoc->IsModelCamera();
		pDoc->m_pView->UpdateCameraFrustum();
		m_wndToolBar.GetToolBarCtrl().CheckButton(ID_MODEL_CAMERA, pDoc->IsModelCamera());
		if (!pDoc->IsModelCamera())
			pDoc->m_pView->SetCameraPosition(0,0,1);
	}
}

void CMainFrame::OnUpdateZeroPlane(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pView->enable_zeroplane);
}

void CMainFrame::OnZeroPlane()
{
	pView->enable_zeroplane=!pView->enable_zeroplane;
}



IMPLEMENT_DYNAMIC(CSetTimeDialog, CDialog)

BEGIN_MESSAGE_MAP(CSetTimeDialog, CDialog)
	ON_WM_DESTROY()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CSetTimeDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString str;
	str.Format("%d", (int)pDoc->time_light());
	GetDlgItem(IDC_HOUR_EDIT)->SetWindowText(str);
	return TRUE; 
}
void CSetTimeDialog::OnDestroy()
{
	CDialog::OnDestroy();
	CString str;
	GetDlgItem(IDC_HOUR_EDIT)->GetWindowText(str);
	time = atof(str);
}

void CSetTimeDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_TIME)
	{
		pDoc->time_light()=((float)slider.GetPos())/MAX_SLIDER*24;
		CString str;
		str.Format("%.1f", pDoc->time_light());
		GetDlgItem(IDC_HOUR_EDIT)->SetWindowText(str);
	//	phaseEdit.Format("%f",((float)phaseSlider.GetPos())/1000.f);
		UpdateData(FALSE);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainFrame::OnUpDateButtonTime(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pDoc->NeedTimeLight());
}
void CMainFrame::OnButtonTime()
{
	pDoc->NeedTimeLight()=!pDoc->NeedTimeLight();
	time_slider_dialog.ShowWindow(pDoc->NeedTimeLight());
	if(!pDoc->NeedTimeLight())
	{
		pView->SetSunDirection(Vect3f::ZERO);
	}
}


void CMainFrame::OnUpdateDrawSceleton(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pView->show_sceleton);
}

void CMainFrame::OnDrawSceleton()
{
	pView->show_sceleton=!pView->show_sceleton;
}


void CMainFrame::OnDestroy()
{
	RECT rect;
	GetWindowRect(&rect);
	WriteRegistryInt("WindowPosX",rect.left);
	WriteRegistryInt("WindowPosY",rect.top);
	WriteRegistryInt("WindowWidth",rect.right-rect.left);
	WriteRegistryInt("WindowHeight",rect.bottom-rect.top);
	CFrameWnd::OnDestroy();
}

void CMainFrame::OnViewAutomaticmipmap()
{
	GetTexLibrary()->DebugMipMapForce(0);
}

void CMainFrame::OnUpdateViewAutomaticmipmap(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetTexLibrary()->GetDebugMipMapForce()==0);
}

void CMainFrame::OnView0miplevel()
{
	GetTexLibrary()->DebugMipMapForce(1);
}

void CMainFrame::OnUpdateView0miplevel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetTexLibrary()->GetDebugMipMapForce()==1);
}

void CMainFrame::OnView1miplevel()
{
	GetTexLibrary()->DebugMipMapForce(2);
}

void CMainFrame::OnUpdateView1miplevel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetTexLibrary()->GetDebugMipMapForce()==2);
}

void CMainFrame::OnView2miplevel()
{
	GetTexLibrary()->DebugMipMapForce(3);
}

void CMainFrame::OnUpdateView2miplevel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetTexLibrary()->GetDebugMipMapForce()==3);
}

void CMainFrame::OnView3miplevel()
{
	GetTexLibrary()->DebugMipMapForce(4);
}

void CMainFrame::OnUpdateView3miplevel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetTexLibrary()->GetDebugMipMapForce()==4);
}

void CMainFrame::OnViewForcemipcolor()
{
	GetTexLibrary()->DebugMipMapColor(!GetTexLibrary()->IsDebugMipMapColor());
}

void CMainFrame::OnUpdateViewForcemipcolor(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetTexLibrary()->IsDebugMipMapColor());
}

void CMainFrame::OnFog()
{
	pView->set_fog=!pView->set_fog;
}

void CMainFrame::OnUpdateFog(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pView->set_fog);
}

void CMainFrame::OnViewNodecontrol()
{
	if(pDoc->m_pHierarchyObj)
	{
		ncdlg.ModelChange(pDoc->m_pHierarchyObj->GetRoot(),pDoc->m_pHierarchyObj->GetLogicObj());
		ncdlg.ShowWindow(true);
	}
}

void CMainFrame::OnPhaseControl()
{
	phaseDlg.SetFrame(pDoc->m_pView->GetFrame(),this);
	phaseDlg.ShowWindow(true);
}

void CMainFrame::OnViewNormal()
{
	pView->view_normal=!pView->view_normal;
}

void CMainFrame::OnUpdateViewNormal(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pView->view_normal);
}
void CMainFrame::AnimationEnable(bool enable)
{
	pView->AnimationEnable(enable);
}
