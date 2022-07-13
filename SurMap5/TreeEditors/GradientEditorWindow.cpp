#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientEditorWindow.h"

#include "GradientEditor.h"
#include "GradientPositionCtrl.h"

#include "ColorSelector.h"
#include "mfc\LayoutMFC.h"
#include "ColorUtils.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientEditorWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientEditorWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

CGradientEditorWindow::CGradientEditorWindow(GradientPositionMode positionMode, int style, const CKeyColor& gradient, CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
, colorSelector_(new CColorSelector(style & STYLE_NO_ALPHA, true, this))
, gradientView_(new CGradientEditorView(this))
, positionControl_(0)
, rulerControl_(new CGradientRulerCtrl(positionMode))
, modalResult_(false)
, style_(style)
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

	gradientView_->setGradient(gradient);
	positionControl_ = new CGradientPositionCtrl(gradientView_->getGradient(), positionMode, this);
}

CGradientEditorWindow::~CGradientEditorWindow ()
{
}


void CGradientEditorWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

int CGradientEditorWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(gradientView_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 48), this, 0));
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
	VERIFY(okButton_.Create("OK", BS_DEFPUSHBUTTON | WS_VISIBLE | WS_CHILD, CRect(0, 0, 80, 25), this, IDOK));
	okButton_.SetFont(font);
	VERIFY(cancelButton_.Create("Cancel", WS_VISIBLE | WS_CHILD, CRect(0, 0, 80, 25), this, IDCANCEL));
	cancelButton_.SetFont(font);

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientEditorWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindMethod(*this, &CGradientEditorWindow::onPositionChanged);
	gradientView_->signalPositionChanged() = bindMethod(*this, &CGradientEditorWindow::onGradientViewChanged);
	gradientView_->signalColorChanged() = bindMethod(*this, &CGradientEditorWindow::onGradientViewChanged);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);
	box->pack(new LayoutControl(gradientView_), true, true);
	{
		LayoutHBox* hbox = new LayoutHBox(5);
		hbox->pack(new LayoutControl(colorSelector_), true, true);
		hbox->pack(new LayoutControl(positionControl_));
		box->pack(hbox);
	}

	{
		LayoutHBox* buttonsBox = new LayoutHBox(5);
        buttonsBox->pack(new LayoutElement(), true, true);
        buttonsBox->pack(new LayoutControl(&okButton_));
        buttonsBox->pack(new LayoutControl(&cancelButton_));
		box->pack(buttonsBox);
	}

	layout_->add(box);
	positionControl_->setSelection(0);
	onPositionChanged();
	return 0;
}

void CGradientEditorWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientEditorWindow::onPositionChanged()
{
	CKeyColor& gradient = gradientView_->getGradient();

	int selection = positionControl_->getSelection();
    gradientView_->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	gradientView_->Invalidate(FALSE);
}

void CGradientEditorWindow::onColorChanged()
{
	CKeyColor& gradient = gradientView_->getGradient();
	int selection = gradientView_->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientEditorWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}

	gradientView_->Invalidate(FALSE);
}

void CGradientEditorWindow::onGradientViewChanged()
{
	CKeyColor& gradient = gradientView_->getGradient();
	int selection = gradientView_->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);
}

const CKeyColor& CGradientEditorWindow::getGradient() const
{
	return gradientView_->getGradient();
}

BOOL CGradientEditorWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

bool CGradientEditorWindow::doModal()
{
	if(parent_ && ::IsWindow(parent_->GetSafeHwnd()))
		parent_->EnableWindow(FALSE);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int width = 600;
	int height = 240;
	int left = (screenWidth - width) / 2;
	int top = (screenHeight - height) / 2;

	VERIFY(Create(WS_VISIBLE | WS_OVERLAPPEDWINDOW, CRect(left, top, left + width, top + height), parent_, 0));
	RunModalLoop(0);

	ShowWindow(SW_HIDE);
	DestroyWindow();

	if(parent_ && ::IsWindow(parent_->GetSafeHwnd())){
		parent_->EnableWindow(TRUE);
		parent_->SetActiveWindow();
		parent_->SetFocus();
	}  

	return modalResult_;
}

BOOL CGradientEditorWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(wParam == IDOK){
		modalResult_ = true;
		EndModalLoop(IDOK);
	}
	if(wParam == IDCANCEL){
			modalResult_ = false;
		EndModalLoop(IDCANCEL);
	}
	return CWnd::OnCommand(wParam, lParam);
}

void CGradientEditorWindow::OnClose()
{
	modalResult_ = false;
	EndModalLoop(IDCANCEL);
}
