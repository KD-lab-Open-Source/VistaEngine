#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\ColorSelector.h"
#include "mfc\LayoutMFC.h"
#include "SolidColorCtrl.h"
#include "ColorUtils.h"

#include <algorithm>

BEGIN_MESSAGE_MAP(CColorSelector, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	// ON_WM_PAINT()
	// ON_WM_LBUTTONDOWN()
	// ON_WM_LBUTTONUP()
	// ON_WM_MOUSEMOVE()
	// ON_WM_LBUTTONDBLCLK()
	// ON_WM_KEYDOWN()
	// ON_WM_RBUTTONDOWN()
	// ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

CColorSelector::CColorSelector(bool noAlpha, bool allowPopup, CWnd* parent)
: red_(0)
, green_(0)
, blue_(0)
, alpha_(255)
, layout_(new LayoutWindow(this))
, colorControl_(new CSolidColorCtrl(allowPopup, this))
, noAlpha_(noAlpha)
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

CColorSelector::~CColorSelector ()
{
}


void CColorSelector::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs){
		return lhs.time < rhs.time;
	}
};


int CColorSelector::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(redLabel_.Create("Red", WS_VISIBLE | WS_CHILD | SS_RIGHT, CRect(0, 0, 50, 20), this, 0));
	VERIFY(greenLabel_.Create("Green", WS_VISIBLE | WS_CHILD | SS_RIGHT, CRect(0, 0, 50, 20), this, 0));
	VERIFY(blueLabel_.Create("Blue", WS_VISIBLE | WS_CHILD | SS_RIGHT, CRect(0, 0, 50, 20), this, 0));
	if(!noAlpha_)
		VERIFY(alphaLabel_.Create("Alpha", WS_VISIBLE | WS_CHILD | SS_RIGHT, CRect(0, 0, 50, 20), this, 0));
	
	VERIFY(redSlider_.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_NUMBER | WS_GROUP, CRect(0, 0, 100, 20), this, 0));
	redSlider_.SetRange(0, 255);
	VERIFY(greenSlider_.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP |  ES_NUMBER, CRect(0, 0, 100, 20), this, 0));
	greenSlider_.SetRange(0, 255);
	VERIFY(blueSlider_.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP |  ES_NUMBER, CRect(0, 0, 100, 20), this, 0));
	blueSlider_.SetRange(0, 255);
	if(!noAlpha_){
		VERIFY(alphaSlider_.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP |  ES_NUMBER, CRect(0, 0, 100, 20), this, 0));
		alphaSlider_.SetRange(0, 255);
	}

	VERIFY(redEdit_.CreateEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_GROUP, CRect(0, 0, 60, 20), this, 0));
	VERIFY(greenEdit_.CreateEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, CRect(0, 0, 60, 20), this, 0));
	VERIFY(blueEdit_.CreateEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, CRect(0, 0, 60, 20), this, 0));
	
	if(!noAlpha_)
		VERIFY(alphaEdit_.CreateEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, CRect(0, 0, 60, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
	redLabel_.SetFont(font);
	greenLabel_.SetFont(font);
	blueLabel_.SetFont(font);
	if(!noAlpha_)
		alphaLabel_.SetFont(font);
	redEdit_.SetFont(font);
	greenEdit_.SetFont(font);
	blueEdit_.SetFont(font);
	if(!noAlpha_)
		alphaEdit_.SetFont(font);

	VERIFY(colorControl_->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER, CRect(0, 0, 80, 80), this, 0));
	colorControl_->signalColorChanged() = bindMethod(*this, &CColorSelector::onColorChanged);

	updateEdits();

	LayoutHBox* hBox = new LayoutHBox;

	hBox->pack(new LayoutControl(colorControl_));

	LayoutVBox* slidersBox = new LayoutVBox(1, 1);
	{
		LayoutHBox* box = new LayoutHBox;
		box->pack(new LayoutControl(&redLabel_), false, false);
		box->pack(new LayoutControl(&redSlider_), true, true);
		box->pack(new LayoutControl(&redEdit_), false, false);
		slidersBox->pack(box);
	}
	{
		LayoutHBox* box = new LayoutHBox;
		box->pack(new LayoutControl(&greenLabel_), false, false);
		box->pack(new LayoutControl(&greenSlider_), true, true);
		box->pack(new LayoutControl(&greenEdit_), false, false);
		slidersBox->pack(box);
	}
	{
		LayoutHBox* box = new LayoutHBox;
		box->pack(new LayoutControl(&blueLabel_), false, false);
		box->pack(new LayoutControl(&blueSlider_), true, true);
		box->pack(new LayoutControl(&blueEdit_), false, false);
		slidersBox->pack(box);
	}
	if(!noAlpha_){
		LayoutHBox* box = new LayoutHBox;
		box->pack(new LayoutControl(&alphaLabel_), false, false);
		box->pack(new LayoutControl(&alphaSlider_), true, true);
		box->pack(new LayoutControl(&alphaEdit_), false, false);
		slidersBox->pack(box);
	}
	hBox->pack(slidersBox, true, true);

	layout_->add(hBox);
	return 0;
}

void CColorSelector::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CColorSelector::onColorChanged()
{
	setColor(colorControl_->getColor());
	signalColorChanged_();
}

BOOL CColorSelector::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "", style | WS_TABSTOP, rect, parent, id, 0);
}

void CColorSelector::setColor(const sColor4f& color)
{
	red_ = round(color.r * 255.0f);
	green_ = round(color.g * 255.0f);
	blue_ = round(color.b * 255.0f);
	alpha_ = round(color.a * 255.0f);

	if(::IsWindow(GetSafeHwnd())){
		updateColor();
		updateEdits();
		updateSliders();
	}
}

const sColor4f& CColorSelector::getColor() const
{
	color_ = sColor4f(float(red_) / 255.0f, float(green_) / 255.0f, float(blue_) / 255.0f, float(alpha_) / 255.0f);
	return color_;
}

void CColorSelector::OnHScroll(UINT sbCode, UINT pos, CScrollBar* scrollBar)
{
	red_ = BYTE(redSlider_.GetPos());
	green_ = BYTE(greenSlider_.GetPos());
	blue_ = BYTE(blueSlider_.GetPos());
	alpha_ = noAlpha_ ? 255 : BYTE(alphaSlider_.GetPos());
	
	updateEdits();
	updateColor();

	onColorChanged();
}

void CColorSelector::updateColor()
{
	colorControl_->setColor(getColor());
}

void CColorSelector::updateEdits()
{
	CString str;
	CString temp;
    
	str.Format("%i", red_);
	redEdit_.GetWindowText(temp);
	if(temp != str)
		redEdit_.SetWindowText(str);

	str.Format("%i", green_);
	greenEdit_.GetWindowText(temp);
	if(temp != str)
		greenEdit_.SetWindowText(str);

	str.Format("%i", blue_);
	blueEdit_.GetWindowText(temp);
	if(temp != str)
		blueEdit_.SetWindowText(str);

	if(!noAlpha_){
		str.Format("%i", alpha_);
		alphaEdit_.GetWindowText(temp);
		if(temp != str)
			alphaEdit_.SetWindowText(str);
	}
}
void CColorSelector::updateSliders()
{
	redSlider_.SetPos(red_);
	greenSlider_.SetPos(green_);
	blueSlider_.SetPos(blue_);
	if(!noAlpha_)
		alphaSlider_.SetPos(alpha_);
}

void CColorSelector::onEditChange(CEdit* edit)
{
	CString str;

	if(edit == &redEdit_){
		redEdit_.GetWindowText(str);
		red_ = BYTE(atoi(str));
	}

	if(edit == &greenEdit_){
		greenEdit_.GetWindowText(str);
		green_ = BYTE(atoi(str));
	}
	
	if(edit == &blueEdit_){
		blueEdit_.GetWindowText(str);
		blue_ = BYTE(atoi(str));
	}

	if(edit == &alphaEdit_){
		alphaEdit_.GetWindowText(str);
		alpha_ = BYTE(atoi(str));
	}

	updateSliders();
	updateColor();
	if(signalColorChanged_)
		signalColorChanged_();
}

BOOL CColorSelector::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(HIWORD(wParam) == EN_CHANGE && 
		(HWND(lParam) == redEdit_.GetSafeHwnd() ||
		 HWND(lParam) == greenEdit_.GetSafeHwnd() ||
		 HWND(lParam) == blueEdit_.GetSafeHwnd() ||
		 HWND(lParam) == alphaEdit_.GetSafeHwnd()))
		 onEditChange((CEdit*)CWnd::FromHandle(HWND(lParam)));
	return __super::OnCommand(wParam, lParam);
}
