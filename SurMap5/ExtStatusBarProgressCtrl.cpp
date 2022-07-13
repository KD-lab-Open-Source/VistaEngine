#include "StdAfx.h"
#include "ExtStatusBarProgressCtrl.h"

LRESULT CExtStatusBarProgressCtrl::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if( uMsg == WM_ERASEBKGND )
		return (!0);
	if( uMsg == WM_PAINT )
	{
		CRect rcClient;
		GetClientRect( &rcClient );
		CPaintDC dcPaint( this );
		CExtMemoryDC dc( &dcPaint, &rcClient );
		if( g_PaintManager->GetCb2DbTransparentMode(this) )
		{
			CExtPaintManager::stat_ExcludeChildAreas(
													 dc,
													 GetSafeHwnd(),
													 CExtPaintManager::stat_DefExcludeChildAreaCallback
													 );
			g_PaintManager->PaintDockerBkgnd(true, dc, this);
		} // if( g_PaintManager->GetCb2DbTransparentMode(this) )
		else
			dc.FillSolidRect( &rcClient, g_PaintManager->GetColor( CExtPaintManager::CLR_3DFACE_OUT ) );
		DefWindowProc( WM_PAINT, WPARAM(dc.GetSafeHdc()), 0L );
		g_PaintManager->OnPaintSessionComplete( this );
		return 0;
	}
	if( uMsg == WM_TIMER && IsWindowEnabled()){
		StepIt();
	}
	if( uMsg == WM_DESTROY ){
		KillTimer(0);
	}
	LRESULT lResult = CProgressCtrl::WindowProc(uMsg, wParam, lParam);
	return lResult;
}
