// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__378C9D5C_3E28_40E8_84E9_C1FE6F876BEA__INCLUDED_)
#define AFX_MAINFRM_H__378C9D5C_3E28_40E8_84E9_C1FE6F876BEA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "NodeControlDialog.h"
#include "PhaseControlDialog.h"

class CSetTimeDialog : public CDialog
{
	DECLARE_DYNAMIC(CSetTimeDialog)
public:
	float time, vel_time;
	CSetTimeDialog(CWnd* pParent = NULL):CDialog(IDD, pParent){}
	virtual ~CSetTimeDialog(){}
	enum { IDD = IDD_SET_TIME_DIALOG };
	enum { MAX_SLIDER=240 };
protected:
	virtual void DoDataExchange(CDataExchange* pDX){
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_SLIDER_TIME, slider);
		slider.SetRange(0,MAX_SLIDER);

	}
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	CSliderCtrl slider;
};


class CMainFrame : public CFrameWnd
{
	friend class CWinVGView;
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	string effect_directory;
// Operations
public:
	void AnimationEnable(bool enable);
	void LoadRegistry();
	void SaveRegistry();
	void OnCloseBumpDialog();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL
	
// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	bool	m_bPressBBox;
	bool	m_bPressBound;
	bool	m_bPressLogic;
	bool	m_bPressObject;
	bool	m_bInit;

	CStatusBar  m_wndStatusBar;
	bool scale_normal;

	CString logo_image_name;
	sColor4f skin_color;
	bool IsReverse();
protected:
	CNodeControlDialog ncdlg;
	CPhaseControlDialog phaseDlg;
	CToolBar    m_wndToolBar;

	CSplitterWnd m_Splitter0;

	bool use_shadow;
	void CheckControlSwitch();

	CComboBox directory_box;
// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtonBbox();
	afx_msg void OnButtonBound();
	afx_msg void OnButtonLogic();
	afx_msg void OnButtonObject();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonLighting();
	afx_msg void OnMenuControlCamera();
	afx_msg void OnMenuControlObject();
	afx_msg void OnMenuControlDirlight();
	afx_msg void OnButtonColor();
	afx_msg void OnButtonBkcolor();
	afx_msg void OnDebugMemory();
	afx_msg void OnNumObject();
	afx_msg void OnUpdateNumObject(CCmdUI* pCmdUI);
	afx_msg void OnHologram();
	afx_msg void OnUpdateHologram(CCmdUI* pCmdUI);
	afx_msg void OnScalenormal();
	afx_msg void OnUpdateScalenormal(CCmdUI* pCmdUI);
	afx_msg void OnButtonShadow();
	afx_msg void OnUpdateButtonShadow(CCmdUI* pCmdUI);
	afx_msg void OnEnableBump();
	afx_msg void OnUpdateEnableBump(CCmdUI* pCmdUI);
	afx_msg void OnEffectDirectory();
	afx_msg void OnModelInfo();
	afx_msg void OnEnableAnisotropic();
	afx_msg void OnUpdateEnableAnisotropic(CCmdUI* pCmdUI);
	afx_msg void OnButtonBasement();
	afx_msg void OnUpdateButtonBasement(CCmdUI* pCmdUI);
	afx_msg void OnScreenShoot();
	afx_msg void OnEnableTilemap();
	afx_msg void OnUpdateEnableTilemap(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePixNorm(CCmdUI* pCmdUI);
	afx_msg void OnPixNorm();
	//}}AFX_MSG
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);

	afx_msg void OnLightOn();
	afx_msg void OnUpdateLightOn(CCmdUI* pCmdUI);

	afx_msg void OnHierarhyObject();
	afx_msg void OnHierarhyLogic();
	afx_msg void OnUpdateModelCamera(CCmdUI* pCmdUI);
	afx_msg void OnModelCamera();

	afx_msg void OnUpdateCamera43(CCmdUI* pCmdUI);
	afx_msg void OnCamera43();

	afx_msg void OnUpdateZeroPlane(CCmdUI* pCmdUI);
	afx_msg void OnZeroPlane();
	afx_msg void OnUpDateButtonTime(CCmdUI* pCmdUI);
	afx_msg void OnButtonTime();

	afx_msg void OnUpdateDrawSceleton(CCmdUI* pCmdUI);
	afx_msg void OnDrawSceleton();

	afx_msg void OnPause();
	afx_msg void OnUpdatePause(CCmdUI* pCmdUI);

	afx_msg void OnReverse();
	afx_msg void OnUpdateReverse(CCmdUI* pCmdUI);

	afx_msg void OnFog();
	afx_msg void OnUpdateFog(CCmdUI* pCmdUI);

	afx_msg void OnShowDebrises();
	afx_msg void OnUpdateShowDebrises(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnViewAutomaticmipmap();
	afx_msg void OnUpdateViewAutomaticmipmap(CCmdUI *pCmdUI);
	afx_msg void OnView0miplevel();
	afx_msg void OnUpdateView0miplevel(CCmdUI *pCmdUI);
	afx_msg void OnView1miplevel();
	afx_msg void OnUpdateView1miplevel(CCmdUI *pCmdUI);
	afx_msg void OnView2miplevel();
	afx_msg void OnUpdateView2miplevel(CCmdUI *pCmdUI);
	afx_msg void OnView3miplevel();
	afx_msg void OnUpdateView3miplevel(CCmdUI *pCmdUI);
	afx_msg void OnViewForcemipcolor();
	afx_msg void OnUpdateViewForcemipcolor(CCmdUI *pCmdUI);
	afx_msg void OnViewNodecontrol();
	afx_msg void OnPhaseControl();

	afx_msg void OnViewNormal();
	afx_msg void OnUpdateViewNormal(CCmdUI *pCmdUI);

	afx_msg void OnButtonLogoOpen();

	CSetTimeDialog time_slider_dialog;
};

/////////////////////////////////////////////////////////////////////////////
extern CMainFrame* gb_FrameWnd;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__378C9D5C_3E28_40E8_84E9_C1FE6F876BEA__INCLUDED_)
