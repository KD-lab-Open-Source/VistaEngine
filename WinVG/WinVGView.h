// WinVGView.h : interface of the CWinVGView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINVGVIEW_H__999B7B91_AFA9_4CC9_BD5A_2C3A5EE406AF__INCLUDED_)
#define AFX_WINVGVIEW_H__999B7B91_AFA9_4CC9_BD5A_2C3A5EE406AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWinVGView : public CView
{
protected: // create from serialization only
	CWinVGView();
	DECLARE_DYNCREATE(CWinVGView)

// Attributes
public:
	CWinVGDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinVGView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	//}}AFX_VIRTUAL

	void LightUpdate();
// Implementation
public:
	friend class CTabDialog;
	double m_ScaleTime,m_OldScaleTime;
	int m_NumberAnimChain,dwScrX,dwScrY,m_ControlSwitch;
	int bWireFrame	: 1;
	int bMouseLDown	: 1;
	int bMouseRDown	: 1;
	bool show_basement;
	CPoint	PointMouseLDown;
	CPoint	PointMouseRDown;
	sColor4c Color;
	bool light_on;
	bool show_sceleton;
	bool set_fog;
	bool view_normal;

	int selected_logic_node;
	int selected_graph_node;

	sColor4f GetSkinColor(){return SkinColor;}
	void SetSkinColor(sColor4f color,const char* logo_image_name);

	Vect3f target_pos;
	Se3f camera_pos;
	cTexture* pLightMap;
	cObject3dx* pLogicObj;

	bool b_viewHologram;
	bool enable_tilemap;
	bool enable_zeroplane;
	void DoneRenderDevice();
	int InitRenderDevice(int xScr,int yScr);
	void LoadObject(LPCSTR fname);
	void SetSunDirection(const Vect3f &dAngle);
	void ObjectControl(Vect3f &dPos,Vect3f &dAngle,float dScale);
	virtual ~CWinVGView();

	void UpdateIgnore();

	void SetScale(bool normal);

	void UpdateObjectLight();
	void SetEffectDirectory(const char* dir);

	void ResetCameraPosition();
	void SetCameraPosition(float du,float dv,float dscale);

	void ModelInfo();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	afx_msg void OnNumObject();
	afx_msg void OnUpdateNumObject(CCmdUI* pCmdUI);
	afx_msg void OnHologram();
	afx_msg void OnUpdateHologram(CCmdUI* pCmdUI);
	afx_msg void OnPause();
	afx_msg void OnUpdatePause(CCmdUI* pCmdUI);
	afx_msg void OnReverse();
	afx_msg void OnUpdateReverse(CCmdUI* pCmdUI);

	void DrawLocalBorder(cObject3dx* root);
	void OnScreenShoot();
	void SynhronizeObjAndLogic(cObject3dx *UObj,cObject3dx *ULogic);
	void UpdateFramePeriod();

	void UpdateCameraFrustum();
	void SetDirectLight(float time);
	cFrame* GetFrame(){return &FramePhase;}
	void AnimationEnable(bool enable){animationEnable = enable;}
	bool IsReverse(){return reverseAnimation;}

	void OnShowDebrises();
	void OnUpdateShowDebrises(CCmdUI* pCmdUI);
protected:
	sColor4f SkinColor;
	cFrame	FramePhase;
	float frame_period;
	bool pauseAnimation;
	bool animationEnable;
	bool reverseAnimation;

	vector<cObject3dx*> bound_spheres;
	void BuildBoundSpheres();
	void MoveBoundSperes();
	void DeleteSpheres();

	cFont* pFont;

	void TestObject(cObject3dx* UObj);
	void Draw();

	void DrawSpline();
	void DrawZeroPlane();
	void InitFont();

	cObject3dx* GetRoot();
	vector<cSimply3dx*> debrises;
	void DestroyDebrises();
	void SetDebrisPosition();
// Generated message map functions
protected:
	//{{AFX_MSG(CWinVGView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnViewNumObject2();
};

#ifndef _DEBUG  // debug version in WinVGView.cpp
inline CWinVGDoc* CWinVGView::GetDocument()
   { return (CWinVGDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
extern CWinVGView*		pView;
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINVGVIEW_H__999B7B91_AFA9_4CC9_BD5A_2C3A5EE406AF__INCLUDED_)
