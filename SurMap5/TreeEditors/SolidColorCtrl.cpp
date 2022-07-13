#include "StdAfx.h"
#include "Rect.h"
#include "..\..\AttribEditor\AttribEditorCtrl.h" // для TreeNodeClipboard
#include "mfc\PopupMenu.h"
#include "ColorSelectorWindow.h"
#include "SolidColorCtrl.h"
#include "ColorUtils.h"
#include "Dictionary.h"

CSolidColorCtrl::CSolidColorCtrl(bool allowPopup, CWnd* pParent) 
: color_(0.0f, 0.0f, 0.0f, 1.0f)
, allowPopup_(allowPopup)
, popupMenu_(new PopupMenu(100))
{
    WNDCLASS wndclass;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if(!::GetClassInfo (hInst, className(), &wndclass)){
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
        wndclass.lpfnWndProc	= ::DefWindowProc;
        wndclass.cbClsExtra		= 0;
        wndclass.cbWndExtra		= 0;
        wndclass.hInstance		= hInst;
        wndclass.hIcon			= NULL;
        wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
        wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
        wndclass.lpszMenuName	= NULL;
        wndclass.lpszClassName	= className();

        if(!AfxRegisterClass(&wndclass))
            AfxThrowResourceException ();
    }
}

CSolidColorCtrl::~CSolidColorCtrl ()
{
}

BEGIN_MESSAGE_MAP(CSolidColorCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

void CSolidColorCtrl::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

void CSolidColorCtrl::OnDraw(CDC* pDC)
{
	if (!pDC || !pDC->m_hDC)
		return;

	CRect rt;
	GetClientRect (&rt);
	CSize rectSize = rt.Size ();
	CDC dc;
	CBitmap bitmap;
	if(!dc.CreateCompatibleDC(pDC))
		return;
	if(!bitmap.CreateCompatibleBitmap(pDC, rectSize.cx, rectSize.cy))
		return;

	dc.SelectObject(&bitmap);

	sColor4f black(0.0f, 0.0f, 0.0f, 1.0f);
	sColor4f white(1.0f, 1.0f, 1.0f, 1.0f);
	float w = rt.Width () * 0.25f;
	float h = rt.Height () * 0.25f;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 2; ++j) {
			CRect rect (round(i * w), round(j * h + h * 2), round((i + 1) * w), round ((j + 1) * h + h * 2));
			sColor4f color;
			color.interpolate3 (((i + j) % 2) ? black : white, color_, color_.a);
			dc.FillSolidRect (&rect, toColorRef (color));
		}
	}

	dc.FillSolidRect (0, 0, round(w * 4), round(h * 2), toColorRef (color_));

	pDC->BitBlt (0, 0, rectSize.cx, rectSize.cy, &dc, 0, 0, SRCCOPY);
	dc.DeleteDC();
	bitmap.DeleteObject();
}

void CSolidColorCtrl::OnPaint()
{
	CPaintDC paintDC(this);
	OnDraw (&paintDC);
}

void CSolidColorCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	popupMenu_->clear();

	PopupMenuItem& root = popupMenu_->root();
	root.add(TRANSLATE("Копировать"))
		.connect(bindMethod(*this, &CSolidColorCtrl::onMenuCopy));
	root.add(TRANSLATE("Вставить"))
		.connect(bindMethod(*this, &CSolidColorCtrl::onMenuPaste));;
    
	POINT pt = point;
	ClientToScreen(&pt);
	popupMenu_->spawn(pt, GetSafeHwnd());

	CWnd::OnRButtonDown(nFlags, point);
}

void CSolidColorCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	OnDraw(GetDC());
	if(allowPopup_){
		CColorSelectorWindow colorSelector;
		colorSelector.setColor(color_);
		if(colorSelector.doModal(this)){
			color_ = colorSelector.getColor();
			signalColorChanged_();
			Invalidate(FALSE);
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CSolidColorCtrl::setColor (const sColor4f& color)
{
	color_ = color;
	if(::IsWindow(GetSafeHwnd()))
		OnDraw(GetDC());
}

const sColor4f& CSolidColorCtrl::getColor() const
{
	return color_;
}

BOOL CSolidColorCtrl::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CLIENTEDGE, className(), "", style, rect, parent, id, 0);
}

void CSolidColorCtrl::onMenuCopy()
{
	Serializeable ser(color_, "", "");
	TreeNodeClipboard::instance().set(ser, GetSafeHwnd());
}

void CSolidColorCtrl::onMenuPaste()
{
	Serializeable ser(color_, "", "");
	TreeNodeClipboard::instance().get(ser, GetSafeHwnd());
	Invalidate(FALSE);
	signalColorChanged_();
}

BOOL CSolidColorCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	popupMenu_->onCommand(wParam, lParam);
	return __super::OnCommand(wParam, lParam);
}
