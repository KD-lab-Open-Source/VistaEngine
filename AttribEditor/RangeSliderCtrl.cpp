#include "StdAfx.h"

#include "RangeSliderCtrl.h"

BEGIN_MESSAGE_MAP(CRangeSliderCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_HSCROLL()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

CRangeSliderCtrl::CRangeSliderCtrl ()
{
    WNDCLASS wndclass;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if( !::GetClassInfo (hInst, RANGE_SLIDER_CTRL_CLASSNAME, &wndclass) )
    {
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
        wndclass.lpfnWndProc	= ::DefWindowProc;
        wndclass.cbClsExtra		= 0;
        wndclass.cbWndExtra		= 0;
        wndclass.hInstance		= hInst;
        wndclass.hIcon			= NULL;
        wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
        wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
        wndclass.lpszMenuName	= NULL;
        wndclass.lpszClassName	= RANGE_SLIDER_CTRL_CLASSNAME;

        if (!AfxRegisterClass (&wndclass))
            AfxThrowResourceException ();
    }
}

int CRangeSliderCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rcClientRect;
	GetClientRect (&rcClientRect);
	//slider_.ScreenToClient (&rcClientRect);
	int edit_width = rcClientRect.Width () / 3;
	CRect rcEdit = rcClientRect;
	rcEdit.right = rcEdit.left + edit_width;
	CRect rcSlider = rcClientRect;
	rcSlider.left += edit_width;

    slider_.Create (WS_VISIBLE, rcSlider, this, 0);

	//Range<int> range = element_.range ();
	///int pos = element_.ranged_value ();
    //
	//slider_.SetRange (range.minimum (), range.maximum ());
	//slider_.SetPos (pos);
	//slider_.Invalidate ();
	//edit_.Create (WS_VISIBLE, rcEdit, this, 0);
    //edit_.SetWindowText (static_cast<std::string> (element_).c_str ());
	return 0;
}

BOOL CRangeSliderCtrl::Create (DWORD style, const CRect& rect, CWnd* parent_wnd, UINT id)
{
    DWORD dwExStyle = 0;

    if (!CWnd::Create (RANGE_SLIDER_CTRL_CLASSNAME, 0,
                       style, rect, parent_wnd, id, 0))
    {
        return FALSE;
    }

    return TRUE;
}

void CRangeSliderCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString str;
	//element_.ranged_value (slider_.GetPos ());
	// serialization_notify_.notify ();
	//str.Format ("%.5g", static_cast<double>(slider_.GetPos ()) * element_.range_step ());
	//edit_.SetWindowText (str);
	switch (nSBCode)
    {
    case SB_THUMBTRACK:
		str = CString ("Значение: ") + str; 
        break;
    case SB_THUMBPOSITION:
        break;
    }

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRangeSliderCtrl::SetFont (CFont* pFont)
{
    edit_.SetFont (pFont);
}

void CRangeSliderCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	edit_.SetFocus ();
	edit_.SetSel (0, -1);
}
