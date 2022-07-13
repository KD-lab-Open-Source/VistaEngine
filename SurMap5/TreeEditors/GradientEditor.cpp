#include "StdAfx.h"
#include "Rect.h"
#include "GradientEditor.h"
#include "ColorUtils.h"

#include <algorithm>
#include ".\gradienteditor.h"

BEGIN_MESSAGE_MAP(CGradientEditorView, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CGradientEditorView::CGradientEditorView (CWnd* pParent)
: cycled_(false)
, moving_(false)
, selection_(-1)
, lastMousePosition_(0.0f)
, fixedPointsCount_(false)
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

        if (!AfxRegisterClass (&wndclass))
            AfxThrowResourceException ();
    }
}

CGradientEditorView::~CGradientEditorView ()
{
}


void CGradientEditorView::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientEditorView::setGradient (const CKeyColor& gradient)
{
	gradient_ = gradient;
	std::sort(gradient_.begin(),  gradient_.end(), KeyColorComparer());
	if(!gradient_.empty())
		setSelection(0);
}

CKeyColor& CGradientEditorView::getGradient()
{
	return gradient_;
}

void CGradientEditorView::DrawColorBlend (CDC* dc, const Recti& rect, const sColor4f& color1, const sColor4f& color2)
{
	for (int i = rect.left(); i <= rect.right (); ++i) {
		float pos = static_cast<float>(i - rect.left ()) / static_cast<float>(rect.width ());
		

        sColor4f solidColor, result;
		solidColor.interpolate (color1, color2, pos);

		for (int j = 0; j < 2; ++j) {
			sColor4f backColor = ((i / rect.height () + j) % 2) ? sColor4f (1.0f, 1.0f, 1.0f) : sColor4f (0.0f, 0.0f, 0.0f);
			result.interpolate3 (backColor, solidColor, solidColor.a);
			dc->FillSolidRect (i, rect.top () + j * rect.height () / 2, 
							   1, j ? (rect.height() - rect.height () / 2) : (rect.height () / 2),
							   toColorRef (result));
		}
	}
}

void CGradientEditorView::DrawGradient (CDC* dc, const CRect& rect, const CKeyColor& gradient)
{
	if (gradient.empty())
		return;

    CKeyColor::const_iterator it;
    float last_time = 0.0f;

	Rectf wholeRect(float(rect.left), float(rect.top), float(rect.Width()), float(rect.Height()));

	KeyColor lastKey (gradient.front ());
	if (gradient.size () == 1) {
		DrawColorBlend (dc, wholeRect, lastKey, lastKey);
	} else {
		FOR_EACH (gradient, it) {
			Recti blendRect (round(wholeRect.left() + wholeRect.width() * lastKey.time), int(wholeRect.top()),
							 max (1, round(wholeRect.width() * (it->time - lastKey.time))), int(wholeRect.height()));
			blendRect.right(min(blendRect.right(), rect.right - 1));
			DrawColorBlend(dc, blendRect, lastKey, *it);
			lastKey = *it;
		}
	}
}

void CGradientEditorView::DrawPegs (CDC* dc, const Recti& rect, const CKeyColor& gradient, int selection)
{
	
	//float pos = static_cast<float>(i - rect.left ()) / static_cast<float>(rect.width ());
	CKeyColor::const_iterator it;
	int index = 0;
	FOR_EACH (gradient, it) {
		int posX = rect.left() + it->time * rect.width();
		int pegSize = rect.height () - 4;
		Recti pegRect (posX - pegSize / 2, rect.top () + 2, pegSize, pegSize);
		
		dc->FillSolidRect (pegRect.left() - 1, pegRect.top() - 1,
						   pegRect.width() + 2, pegRect.height() + 2,
						   (index == selection) ? RGB (255, 255, 255) : RGB (100, 100, 100));

		dc->FillSolidRect (pegRect.left(), pegRect.top(),
						   pegRect.width(), pegRect.height(), toColorRef (*it));
		++index;
	}
}

void CGradientEditorView::OnDraw(CDC* pDC)
{
	if (!pDC || !pDC->m_hDC)
		return;
	CRect rt;
	GetClientRect (&rt);
	CSize rectSize = rt.Size ();
	CDC dc;
	CBitmap bitmap;
	dc.CreateCompatibleDC (pDC);

	if(!dc.m_hDC)
		return;

	bitmap.CreateCompatibleBitmap (pDC, rectSize.cx, rectSize.cy);
	dc.SelectObject (bitmap);
	dc.FillSolidRect (&rt, RGB (0, 0, 0));
	
	rt.DeflateRect (0, 8, 0, 8);
	DrawPegs (&dc, Recti (rt.left, rt.top - 8, rt.Width(), 8), gradient_, selection_);
	DrawPegs (&dc, Recti (rt.left, rt.bottom, rt.Width(), 8), gradient_, selection_);

	DrawGradient (&dc, rt, gradient_);
	pDC->BitBlt (0, 0, rectSize.cx, rectSize.cy, &dc, 0, 0, SRCCOPY);
	bitmap.DeleteObject();
	dc.DeleteDC();
}

void CGradientEditorView::OnPaint()
{
	CPaintDC paintDC(this);
	OnDraw (&paintDC);
}

int CGradientEditorView::GetPegAt (float pos, float precision)
{
	for (int i = 1; i < gradient_.size (); ++i) {
        if (fabs (gradient_[i].time - pos) <= precision) {
			return i;
        }
	}
    if (!gradient_.empty() && fabs (gradient_[0].time - pos) <= precision) {
		return 0;
    }
	return -1;
}

void CGradientEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rt;
	GetClientRect (&rt);

	SetFocus ();

    float pos = static_cast<float>(point.x) / static_cast<float>(rt.Width());
	float precision = 4.0f / static_cast<float>(rt.Width());

	lastMousePosition_ = pos;
	int peg = GetPegAt (pos, precision);

	if(peg != -1){
		setSelection (peg);
		if(signalPositionChanged_)
			signalPositionChanged_();
	}
	else{
		if(!fixedPointsCount_){
			KeyColor* newKey = &gradient_.InsertKey (pos);
			int index = -1;
			for(int i = 0; i < gradient_.size(); ++i){
				if(&gradient_[i] == newKey){
					index = i;
					break;
				}
			}
			if(gradient_.size() == 2){
				newKey->time = 1.0f;
				*static_cast<sColor4f*>(newKey) = gradient_ [0];
			}
			else{
				xassert(index != -1 && index > 0 && index < gradient_.size() - 1);
				float scale = (newKey->time - gradient_[index-1].time) / (gradient_[index+1].time - gradient_[index-1].time);
				static_cast<sColor4f*>(newKey)->interpolate (gradient_[index-1], gradient_[index+1], scale);
			}
			setSelection(index);
			if(signalPositionChanged_)
				signalPositionChanged_();
		}
	}

	if(selection_ != 0 && selection_ != gradient_.size () - 1){
		SetCapture();
		moving_ = true;
	}
	else{
		moving_ = false;
	}

	OnDraw (GetDC());
	CWnd::OnLButtonDown(nFlags, point);
}

void CGradientEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (moving_) {
		moving_ = false;
		if (GetCapture() == this) {
			ReleaseCapture ();
		}
		OnDraw (GetDC ());
	}

	CWnd::OnLButtonUp(nFlags, point);
}

BOOL CGradientEditorView::OnMouseWheel(UINT flags, short delta, CPoint pt)
{
	if(delta > 0)
		previousPeg();
	else
		nextPeg();
	return TRUE;
}

void CGradientEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (moving_ && selection_ != -1) {
		xassert (selection_ > 0 && selection_ < gradient_.size () - 1);
		CRect rt;
		GetClientRect (&rt);
		float pos = static_cast<float>(point.x) / static_cast<float>(rt.Width());
		float old_pos = gradient_[selection_].time;
		
		float new_pos = old_pos;
		if ((nFlags & MK_SHIFT) && (nFlags & MK_RBUTTON))
			new_pos += (pos - lastMousePosition_) / 10.0f;
		else
			new_pos += pos - lastMousePosition_;

		if (new_pos < 0.0f)
			new_pos = 0.0f;
		if (new_pos > 1.0f)
			new_pos = 1.0f;

		if (old_pos < new_pos) {
			for (int i = 0; i < gradient_.size() - 1; ++i) {
				if (old_pos < gradient_[i].time && gradient_[i].time <= new_pos) {
					std::swap (gradient_[i], gradient_[selection_]);
					selection_ = i;
				}
			}
		} else if (old_pos > new_pos) {
			for (int i = gradient_.size() - 1; i > 0; --i) {
				if (old_pos > gradient_[i].time && gradient_[i].time >= new_pos) {
					std::swap (gradient_[i], gradient_[selection_]);
					selection_ = i;
				}
			}
		}
		xassert (selection_ >= 0 && selection_ < gradient_.size ());

		/*
		if (nFlags & MK_RBUTTON) {
			SetCursorPos (lastMousePoint_.x, lastMousePoint_.y);
		} else {
			lastMousePosition_ = pos;
		}
		*/
		lastMousePosition_ = pos;

		gradient_[selection_].time = new_pos;
		setSelection (selection_);
		if(signalPositionChanged_)
			signalPositionChanged_();
		OnDraw(GetDC ());
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CGradientEditorView::OnLButtonDblClk(UINT nFlags, CPoint point)
{

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CGradientEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(VK_DELETE == nChar && selection_ != -1){
		if(!fixedPointsCount_){
			if(selection_ > 0 && ((selection_ < gradient_.size() - 1) || (gradient_.size () == 2 && selection_ == 1))){
				gradient_.erase(gradient_.begin () + selection_);
				setSelection(-1);
				if(signalPositionChanged_)
					signalPositionChanged_();
				OnDraw(GetDC());
			}
		}
	}

	if(VK_NEXT == nChar)
		previousPeg();

	if(VK_PRIOR == nChar)
		nextPeg();

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CGradientEditorView::setCurrentColor(const sColor4f& color)
{
	if(selection_ >= 0 && selection_ < gradient_.size()){
		gradient_[selection_].set(color.r, color.g, color.b, color.a);
		if(cycled_ && selection_ == 0)
			gradient_.back().set (color.r, color.g, color.b, color.a);
		if(cycled_ && selection_ == gradient_.size() - 1)
			gradient_.front().set (color.r, color.g, color.b, color.a);
		OnDraw(GetDC());
	}
}

void CGradientEditorView::setSelection (int index)
{
	selection_ = index;
}

int CGradientEditorView::getSelection() const
{
	return selection_;
}

void CGradientEditorView::moveSelection (float position)
{
	if (selection_ > 0 && selection_ < gradient_.size()-1) {
		sColor4f oldValue = gradient_[selection_];
		gradient_.erase (gradient_.begin () + selection_);
		KeyColor* newKey = &gradient_.InsertKey (position);
		static_cast<sColor4f&>(*newKey) = oldValue;
		for (int i = 0; i < gradient_.size(); ++i) {
			if (&gradient_[i] == newKey) {
				selection_ = i;
				break;
			}
		}
	}
	OnDraw (GetDC ());
}

void CGradientEditorView::nextPeg ()
{
	if(getGradient ().empty())
		return;

	if(selection_ == -1)
		setSelection(getGradient().size() - 1);

	if(selection_ < getGradient().size() - 1)
		setSelection(selection_ + 1);

	if(signalPositionChanged_)
		signalPositionChanged_();
}

void CGradientEditorView::previousPeg ()
{
	if(getGradient().empty())
		return;

	if(selection_ == -1)
		setSelection (0);
	if(selection_ > 0)
		setSelection(selection_ - 1);
	if(signalPositionChanged_)
		signalPositionChanged_();
}

void CGradientEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CRect rt;
	GetClientRect (&rt);

	SetFocus ();

    float pos = static_cast<float>(point.x) / static_cast<float>(rt.Width());
	float precision = 4.0f / static_cast<float>(rt.Width());

    if (selection_ == -1 || selection_ == 0 || selection_ >= getGradient().size() - 1)
        return;

	lastMousePosition_ = pos;
	CPoint screenPoint = point;
	ClientToScreen (&screenPoint);
	lastMousePoint_.set (screenPoint.x, screenPoint.y);
    moving_ = true;

    SetCapture ();
	//ShowCursor (FALSE);

	OnDraw (GetDC());

	CWnd::OnRButtonDown(nFlags, point);
}

void CGradientEditorView::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (moving_) {
		moving_ = false;
		if (GetCapture() == this) {
			ReleaseCapture ();
		}
		//ShowCursor (TRUE);
		OnDraw (GetDC ());
	}

	CWnd::OnRButtonUp(nFlags, point);
}

BOOL CGradientEditorView::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CLIENTEDGE, className(), "", style, rect, parent, id, 0);
}
