#include "stdafx.h"
#include "PropertyWindow.h"
#include "..\Game\RenderObjects.h"
#include "MainFrm.h"

#include "..\UserInterface\UI_Render.h"
//#include "..\UserInterface\UI_CustomControls.h"


//IMPLEMENT_DYNAMIC(CPropertyWindow, CWnd)
CPropertyWindow::CPropertyWindow()
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, MINIMAP_CLASSNAME, &wndclass) )
	{
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= PROPERTY_WINDOW_CLASSNAME;

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}
}

CPropertyWindow::~CPropertyWindow()
{
}


BEGIN_MESSAGE_MAP(CPropertyWindow, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CPropertyWindow::Create( DWORD dwStyle, const CRect& rect, CWnd* pParentWnd )
{
	return CWnd::Create (MINIMAP_CLASSNAME, 0, dwStyle, rect, pParentWnd, 0);
}

// CPropertyWindow message handlers


void CPropertyWindow::OnPaint()
{
	CPaintDC dc(this);

	OnDraw ();
}

int CPropertyWindow::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) == -1)
		return -1;

	CRect rt(cs->x, cs->y, cs->cx, cs->cy);

	attribEditor_.setStyle(CAttribEditorCtrl::AUTO_SIZE);
	attribEditor_.Create(WS_VISIBLE | WS_CHILD, rt, this);
	return 0;
}

void CPropertyWindow::OnSize(UINT nType, int cx, int cy)
{
	xassert (::IsWindow (GetSafeHwnd ()));
	CWnd::OnSize(nType, cx, cy);

	if(::IsWindowVisible(attribEditor_.GetSafeHwnd()))
		attribEditor_.MoveWindow(0, 0, cx, cy);
    /*
	m_bSizing = true;
	if (::IsWindowVisible (GetSafeHwnd()) && gb_RenderDevice) {
		gb_RenderDevice->ChangeSize (cx, cy, RENDERDEVICE_MODE_RGB32 | RENDERDEVICE_MODE_WINDOW);
		UI_Render::instance().setWindowPosition (Vect2i (cx, cy), false);
        updateCameraFrustum (cx, cy);
		updateMinimapPosition (cx, cy);
	}
	m_WindowSize.set (cx, cy);
	m_bSizing = false;
    */
}

void CPropertyWindow::OnDraw()
{
}

void CPropertyWindow::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();
}

void CPropertyWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
}

void CPropertyWindow::onSelectionChanged()
{
    //forEachSelected();
}
