#ifndef __PROPERTY_WINDOW_H_INCLUDED__
#define __PROPERTY_WINDOW_H_INCLUDED__

#include "IRenderDevice.h"
#include "IVisGeneric.h"
#include "..\AttribEditor\AttribEditorCtrl.h"

#define PROPERTY_WINDOW_CLASSNAME "VistaEnginePropertyWindow"

class CPropertyWindow : public CWnd
{
//	DECLARE_DYNAMIC(CPropertyWindow)
public:
	BOOL Create (DWORD dwStyle, const CRect& rect, CWnd* pParentWnd);
	CPropertyWindow();
	virtual ~CPropertyWindow();

	CAttribEditorCtrl attribEditor_;

	void onSelectionChanged();
	void OnDraw ();

protected:
	DECLARE_MESSAGE_MAP()
protected:
	virtual void PreSubclassWindow();
public:
	afx_msg void OnPaint();
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

#endif
