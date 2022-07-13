#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientPositionCtrl.h"
#include "mfc\LayoutMFC.h"
#include "ColorUtils.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientPositionCtrl, CWnd)

BEGIN_MESSAGE_MAP(CGradientPositionCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientPositionCtrl::CGradientPositionCtrl(CKeyColor& gradient, GradientPositionMode positionMode, CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
, gradient_(&gradient)
, position_("0")
, positionMode_(positionMode)
, selection_(-1)
, updatingControls_(false)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!::GetClassInfo(hInst, className(), &wndclass)){
		wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= 0;
		wndclass.hCursor		= AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH>(COLOR_WINDOW);
		wndclass.lpszMenuName	= 0;
		wndclass.lpszClassName	= className();

		if(!AfxRegisterClass(&wndclass))
			AfxThrowResourceException();
	}

}

CGradientPositionCtrl::~CGradientPositionCtrl ()
{
}


void CGradientPositionCtrl::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

int CGradientPositionCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	VERIFY(prevButton_.Create("<", WS_VISIBLE | WS_CHILD, CRect(0, 0, 25, 20), this, 100));
	prevButton_.SetFont(font);
	VERIFY(nextButton_.Create(">", WS_VISIBLE | WS_CHILD, CRect(0, 0, 25, 20), this, 101));
	nextButton_.SetFont(font);

	VERIFY(positionLabel_.Create("Position:", WS_VISIBLE | WS_CHILD, CRect(0, 0, 60, 20), this, 0));
	positionLabel_.SetFont(font);
	VERIFY(pegLabel_.Create("Peg:", WS_VISIBLE | WS_CHILD, CRect(0, 0, 60, 25), this, 0));
	pegLabel_.SetFont(font);

	VERIFY(currentPegLabel_.Create("-/-", WS_VISIBLE | WS_CHILD, CRect(0, 0, 25, 20), this, 0));
	currentPegLabel_.SetFont(font);
	VERIFY(positionEdit_.Create(WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(0, 0, 25, 20), this, 0));
	positionEdit_.SetFont(font);
	positionEdit_.SetWindowText("-");

	LayoutVBox* box = new LayoutVBox(4);
	{
		LayoutHBox* buttonsBox = new LayoutHBox(4);
		buttonsBox->pack(new LayoutControl(&pegLabel_));
		buttonsBox->pack(new LayoutElement(), true, true);
		buttonsBox->pack(new LayoutControl(&prevButton_));
		buttonsBox->pack(new LayoutControl(&currentPegLabel_), true, true);
		buttonsBox->pack(new LayoutControl(&nextButton_));
		box->pack(buttonsBox);
	}

	{
		LayoutHBox* buttonsBox = new LayoutHBox(4);
		buttonsBox->pack(new LayoutControl(&positionLabel_));
		buttonsBox->pack(new LayoutElement(), true, true);
		buttonsBox->pack(new LayoutControl(&positionEdit_), true, true);
		box->pack(buttonsBox);
	}
	layout_->add(box);
	return 0;
}

void CGradientPositionCtrl::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

BOOL CGradientPositionCtrl::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT,  className(), "", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientPositionCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(HIWORD(wParam) == EN_CHANGE && HWND(lParam) == positionEdit_.GetSafeHwnd())
		onEditChange();
	if(gradient_ && !gradient_->empty()){
		if(HWND(lParam) == prevButton_.GetSafeHwnd())
		{
			if(selection_ == -1){
				selection_ = gradient_->size() - 1;
				setPosition((*gradient_)[selection_].time);
				signalPositionChanged_();
				updateControls();
			}
			else
			if(selection_ > 0){
				--selection_;
				setPosition((*gradient_)[selection_].time);
				signalPositionChanged_();
				updateControls();
			}
		}
		if(HWND(lParam) == nextButton_.GetSafeHwnd()){
			if(selection_ == -1){
				selection_ = 0;
				setPosition((*gradient_)[selection_].time);
				signalPositionChanged_();
				updateControls();
			}
			else
			if(selection_ < gradient_->size() - 1){
				++selection_;
				setPosition((*gradient_)[selection_].time);
				signalPositionChanged_();
				updateControls();
			}
		}
	}
	return CWnd::OnCommand(wParam, lParam);
}

void CGradientPositionCtrl::setGradient(CKeyColor* gradient)
{
	gradient_ = gradient;
}

void CGradientPositionCtrl::setSelection(int selection)
{
	selection_ = selection;

	if(gradient_ && selection_ >= 0)
		setPosition((*gradient_)[selection].time);
	updateControls();
}

int CGradientPositionCtrl::getSelection() const
{
	return selection_;
}

void CGradientPositionCtrl::setPosition(float pos)
{
	xassert(pos > -10e5f && pos < 10e5f);
	pos = clamp(pos, 0.0f, 1.0f);
	switch(positionMode_){
	case GRADIENT_POSITION_FLOAT:
		position_.Format("%f", pos);
		break;
	case GRADIENT_POSITION_BYTE:
		position_.Format("%f", pos * 255.0f);
		break;
	case GRADIENT_POSITION_TIME:
		position_.Format("%f", pos * 24.0f);
		break;
	};
}

float CGradientPositionCtrl::getPosition() const
{
	switch(positionMode_){
	case GRADIENT_POSITION_FLOAT:
		return clamp(float(atof(position_)), 0.0f, 1.0f);
		break;
	case GRADIENT_POSITION_BYTE:
		return clamp(float(atof(position_)) / 255.0f, 0.0f, 1.0f);
		break;
	case GRADIENT_POSITION_TIME:
		return clamp(float(atof(position_)) / 24.0f, 0.0f, 1.0f);
		break;
	default:
		return 0.0f;
	};
}

void CGradientPositionCtrl::updateControls(bool updateEdit)
{
	if(!gradient_ || updatingControls_)
		return;	

	updatingControls_ = true;

	CString str;
	if(selection_ == -1)
		str.Format("-/%i", gradient_->size());
	else
		str.Format("%i/%i", selection_ + 1, gradient_->size());

	currentPegLabel_.SetWindowText(str);
	if(updateEdit)
		positionEdit_.SetWindowText(position_);

	if(selection_ == 0)
		prevButton_.EnableWindow(FALSE);
	else
		prevButton_.EnableWindow(TRUE);

	if(selection_ == gradient_->size() - 1)
		nextButton_.EnableWindow(FALSE);
	else
		nextButton_.EnableWindow(TRUE);

	updatingControls_ = false;
}

void CGradientPositionCtrl::onEditChange()
{
	positionEdit_.GetWindowText(position_);
	float position = getPosition();
	setPosition(position);
	if(selection_ >= 0)
		(*gradient_)[selection_].time = position;
	updateControls(false);
	if(signalPositionChanged_)
		signalPositionChanged_();
}

IMPLEMENT_DYNAMIC(CGradientRulerCtrl, CWnd)

BEGIN_MESSAGE_MAP(CGradientRulerCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

CGradientRulerCtrl::CGradientRulerCtrl(GradientPositionMode positionMode)
: positionMode_(positionMode)
{
    WNDCLASS wndclass;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if(!::GetClassInfo(hInst, className(), &wndclass)){
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
        wndclass.lpfnWndProc	= ::DefWindowProc;
        wndclass.cbClsExtra		= 0;
        wndclass.cbWndExtra		= 0;
        wndclass.hInstance		= hInst;
        wndclass.hIcon			= 0;
        wndclass.hCursor		= AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndclass.hbrBackground	= reinterpret_cast<HBRUSH>(COLOR_WINDOW);
        wndclass.lpszMenuName	= 0;
        wndclass.lpszClassName	= className();

        if (!AfxRegisterClass (&wndclass))
            AfxThrowResourceException ();
    }
}

CGradientRulerCtrl::~CGradientRulerCtrl()
{

}

BOOL CGradientRulerCtrl::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CLIENTEDGE, className(), "", style, rect, parent, id);
}

int CGradientRulerCtrl::OnCreate(LPCREATESTRUCT createStruct)
{
	font_ = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
	return 0;
}

void CGradientRulerCtrl::OnPaint()
{
	CPaintDC dc(this);
	CRect rt;
	GetClientRect(&rt);
	dc.FillSolidRect(&rt, RGB(0, 0, 0));

	float scale;
	switch(positionMode_){
		case GRADIENT_POSITION_FLOAT:
			scale = 1.0f;
			break;
		case GRADIENT_POSITION_BYTE:
			scale = 256.0f;
			break;
		case GRADIENT_POSITION_TIME:
			scale = 24.0f;
			break;
		default:
			scale = 1.0f;
	}

	for(int i = 0; i < 4; ++i){
		float pos = float(i) * 0.25f;
		CString str;
		str.Format("%.2f", pos * scale);

		int left = round(pos * float(rt.Width()));
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(255, 255, 255));
		dc.FillSolidRect(left, 0, 1, rt.Height(), RGB(255, 255, 255));
		dc.SelectObject(font_);
		dc.TextOut(left + 3, 1, str);
	}

}

void CGradientRulerCtrl::OnSize(UINT type, int cx, int cy)
{
	__super::OnSize(type, cx, cy);
}
