#ifndef __TOOL_PREVIEW_WINDOW_H_INCLUDED__
#define __TOOL_PREVIEW_WINDOW_H_INCLUDED__

#include "IRenderDevice.h"
#include "IVisGeneric.h"

#define TOOL_PREVIEW_CLASSNAME "VistaEngineToolPreview"

class CToolPreviewWindow : public CWnd
{
	DECLARE_DYNAMIC(CToolPreviewWindow)
public:
	BOOL Create (DWORD dwStyle, const CRect& rect, CWnd* pParentWnd);
	CToolPreviewWindow();
	virtual ~CToolPreviewWindow();

	cScene* getScene () {
		return m_pScene;
	}

	void UpdateCameraFrustum (int width, int height);

	void OnDraw ();

	void InitRenderDevice ();
	void DoneRenderDevice ();

protected:
	Vect2i m_WindowSize;
	cRenderWindow* m_pRenderWindow;
    cCamera* m_pCamera;
	cScene* m_pScene;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif
