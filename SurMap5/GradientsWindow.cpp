#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientsWindow.h"

#include "TreeEditors\GradientEditor.h"
#include "TreeEditors\GradientPositionCtrl.h"
#include "TreeEditors\ColorSelector.h"
#include "TreeEditors\ColorUtils.h"

#include "mfc\LayoutMFC.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientsWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientsWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientsWindow::CGradientsWindow(CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
//, gradients_(gradients)
, colorSelector_(new CColorSelector(true, this))
, rulerControl_(new CGradientRulerCtrl(GRADIENT_POSITION_TIME))
, currentGradient_(0)
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

	//setGradientList(gradients);
	positionControl_ = new CGradientPositionCtrl(0, GRADIENT_POSITION_TIME, this);
}

void CGradientsWindow::setGradientList(const GradientList& gradients)
{
	gradients_ = gradients;
	currentGradient_ = 0;
	createGradientViews();
}

CGradientsWindow::~CGradientsWindow ()
{
}


void CGradientsWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientsWindow::createGradientViews()
{
	if(!::IsWindow(GetSafeHwnd()))
		return;
	if(!gradientsBox_){
		xassert(0);
		return;
	}

	GradientViews::iterator vit;
	FOR_EACH(gradientViews_, vit){
		CGradientEditorView* view = *vit;
		if(view && ::IsWindow(view->GetSafeHwnd()))
			view->DestroyWindow();
		delete view;
    }

	gradientsBox_->clear();
	int index = 0;
	GradientList::iterator it;
	FOR_EACH(gradients_, it){
		CGradientEditorView* view = new CGradientEditorView(this);
		VERIFY(view->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		view->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);
		view->signalColorChanged()    = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);

		view->setGradient(*it->gradient);
		gradientsBox_->pack(new LayoutControl(view), true, true);
		gradientViews_.push_back(view);
		++index;
	}
}

int CGradientsWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 20, 20), this, 0, true));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientsWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onPositionChanged), 0);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);

	gradientsBox_ = new LayoutVBox;
   	createGradientViews();
	box->pack(gradientsBox_, true, true);

	box->pack(new LayoutControl(positionControl_), false, false);
	box->pack(new LayoutControl(colorSelector_), false, false);
	
    layout_->add(box);
	return 0;
}



void CGradientsWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientsWindow::onPositionChanged(int index)
{
	if(gradients_.empty())
		return;

	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = positionControl_->getSelection();
    view->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

void CGradientsWindow::onColorChanged()
{
	if(gradients_.empty())
		return;
	CGradientEditorView* view = getViewByIndex(currentGradient_);
	CKeyColor& gradient = view->getGradient();
	int selection = view->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientsWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, currentGradient_);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(currentGradient_);
}

void CGradientsWindow::onGradientViewChanged(int index)
{
	if(gradients_.empty())
		return;

	currentGradient_ = index;
	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = view->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

BOOL CGradientsWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CWnd::OnCommand(wParam, lParam);
}

CGradientEditorView* CGradientsWindow::getViewByIndex(int index)
{
	xassert(index >= 0 && index < gradientViews_.size());
	GradientViews::iterator it = gradientViews_.begin();
	std::advance(it, index);
	return *it;
}
#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientsWindow.h"

#include "TreeEditors\GradientEditor.h"
#include "TreeEditors\GradientPositionCtrl.h"
#include "TreeEditors\ColorSelector.h"
#include "TreeEditors\ColorUtils.h"

#include "mfc\LayoutMFC.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientsWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientsWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientsWindow::CGradientsWindow(CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
//, gradients_(gradients)
, colorSelector_(new CColorSelector(true, this))
, rulerControl_(new CGradientRulerCtrl(GRADIENT_POSITION_TIME))
, currentGradient_(0)
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

	//setGradientList(gradients);
	positionControl_ = new CGradientPositionCtrl(0, GRADIENT_POSITION_TIME, this);
}

void CGradientsWindow::setGradientList(const GradientList& gradients)
{
	gradients_ = gradients;
	currentGradient_ = 0;
	createGradientViews();
}

CGradientsWindow::~CGradientsWindow ()
{
}


void CGradientsWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientsWindow::createGradientViews()
{
	if(!::IsWindow(GetSafeHwnd()))
		return;
	if(!gradientsBox_){
		xassert(0);
		return;
	}

	GradientViews::iterator vit;
	FOR_EACH(gradientViews_, vit){
		CGradientEditorView* view = *vit;
		if(view && ::IsWindow(view->GetSafeHwnd()))
			view->DestroyWindow();
		delete view;
    }

	gradientsBox_->clear();
	int index = 0;
	GradientList::iterator it;
	FOR_EACH(gradients_, it){
		CGradientEditorView* view = new CGradientEditorView(this);
		VERIFY(view->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		view->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);
		view->signalColorChanged()    = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);

		view->setGradient(*it->gradient);
		gradientsBox_->pack(new LayoutControl(view), true, true);
		gradientViews_.push_back(view);
		++index;
	}
}

int CGradientsWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 20, 20), this, 0, true));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientsWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onPositionChanged), 0);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);

	gradientsBox_ = new LayoutVBox;
   	createGradientViews();
	box->pack(gradientsBox_, true, true);

	box->pack(new LayoutControl(positionControl_), false, false);
	box->pack(new LayoutControl(colorSelector_), false, false);
	
    layout_->add(box);
	return 0;
}



void CGradientsWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientsWindow::onPositionChanged(int index)
{
	if(gradients_.empty())
		return;

	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = positionControl_->getSelection();
    view->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

void CGradientsWindow::onColorChanged()
{
	if(gradients_.empty())
		return;
	CGradientEditorView* view = getViewByIndex(currentGradient_);
	CKeyColor& gradient = view->getGradient();
	int selection = view->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientsWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, currentGradient_);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(currentGradient_);
}

void CGradientsWindow::onGradientViewChanged(int index)
{
	if(gradients_.empty())
		return;

	currentGradient_ = index;
	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = view->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

BOOL CGradientsWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CWnd::OnCommand(wParam, lParam);
}

CGradientEditorView* CGradientsWindow::getViewByIndex(int index)
{
	xassert(index >= 0 && index < gradientViews_.size());
	GradientViews::iterator it = gradientViews_.begin();
	std::advance(it, index);
	return *it;
}
#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientsWindow.h"

#include "TreeEditors\GradientEditor.h"
#include "TreeEditors\GradientPositionCtrl.h"
#include "TreeEditors\ColorSelector.h"
#include "TreeEditors\ColorUtils.h"

#include "mfc\LayoutMFC.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientsWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientsWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientsWindow::CGradientsWindow(CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
//, gradients_(gradients)
, colorSelector_(new CColorSelector(true, this))
, rulerControl_(new CGradientRulerCtrl(GRADIENT_POSITION_TIME))
, currentGradient_(0)
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

	//setGradientList(gradients);
	positionControl_ = new CGradientPositionCtrl(0, GRADIENT_POSITION_TIME, this);
}

void CGradientsWindow::setGradientList(const GradientList& gradients)
{
	gradients_ = gradients;
	currentGradient_ = 0;
	createGradientViews();
}

CGradientsWindow::~CGradientsWindow ()
{
}


void CGradientsWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientsWindow::createGradientViews()
{
	if(!::IsWindow(GetSafeHwnd()))
		return;
	if(!gradientsBox_){
		xassert(0);
		return;
	}

	GradientViews::iterator vit;
	FOR_EACH(gradientViews_, vit){
		CGradientEditorView* view = *vit;
		if(view && ::IsWindow(view->GetSafeHwnd()))
			view->DestroyWindow();
		delete view;
    }

	gradientsBox_->clear();
	int index = 0;
	GradientList::iterator it;
	FOR_EACH(gradients_, it){
		CGradientEditorView* view = new CGradientEditorView(this);
		VERIFY(view->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		view->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);
		view->signalColorChanged()    = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);

		view->setGradient(*it->gradient);
		gradientsBox_->pack(new LayoutControl(view), true, true);
		gradientViews_.push_back(view);
		++index;
	}
}

int CGradientsWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 20, 20), this, 0, true));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientsWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onPositionChanged), 0);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);

	gradientsBox_ = new LayoutVBox;
   	createGradientViews();
	box->pack(gradientsBox_, true, true);

	box->pack(new LayoutControl(positionControl_), false, false);
	box->pack(new LayoutControl(colorSelector_), false, false);
	
    layout_->add(box);
	return 0;
}



void CGradientsWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientsWindow::onPositionChanged(int index)
{
	if(gradients_.empty())
		return;

	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = positionControl_->getSelection();
    view->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

void CGradientsWindow::onColorChanged()
{
	if(gradients_.empty())
		return;
	CGradientEditorView* view = getViewByIndex(currentGradient_);
	CKeyColor& gradient = view->getGradient();
	int selection = view->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientsWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, currentGradient_);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(currentGradient_);
}

void CGradientsWindow::onGradientViewChanged(int index)
{
	if(gradients_.empty())
		return;

	currentGradient_ = index;
	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = view->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

BOOL CGradientsWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CWnd::OnCommand(wParam, lParam);
}

CGradientEditorView* CGradientsWindow::getViewByIndex(int index)
{
	xassert(index >= 0 && index < gradientViews_.size());
	GradientViews::iterator it = gradientViews_.begin();
	std::advance(it, index);
	return *it;
}
#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientsWindow.h"

#include "TreeEditors\GradientEditor.h"
#include "TreeEditors\GradientPositionCtrl.h"
#include "TreeEditors\ColorSelector.h"
#include "TreeEditors\ColorUtils.h"

#include "mfc\LayoutMFC.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientsWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientsWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientsWindow::CGradientsWindow(CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
//, gradients_(gradients)
, colorSelector_(new CColorSelector(true, this))
, rulerControl_(new CGradientRulerCtrl(GRADIENT_POSITION_TIME))
, currentGradient_(0)
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

	//setGradientList(gradients);
	positionControl_ = new CGradientPositionCtrl(0, GRADIENT_POSITION_TIME, this);
}

void CGradientsWindow::setGradientList(const GradientList& gradients)
{
	gradients_ = gradients;
	currentGradient_ = 0;
	createGradientViews();
}

CGradientsWindow::~CGradientsWindow ()
{
}


void CGradientsWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientsWindow::createGradientViews()
{
	if(!::IsWindow(GetSafeHwnd()))
		return;
	if(!gradientsBox_){
		xassert(0);
		return;
	}

	GradientViews::iterator vit;
	FOR_EACH(gradientViews_, vit){
		CGradientEditorView* view = *vit;
		if(view && ::IsWindow(view->GetSafeHwnd()))
			view->DestroyWindow();
		delete view;
    }

	gradientsBox_->clear();
	int index = 0;
	GradientList::iterator it;
	FOR_EACH(gradients_, it){
		CGradientEditorView* view = new CGradientEditorView(this);
		VERIFY(view->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		view->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);
		view->signalColorChanged()    = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);

		view->setGradient(*it->gradient);
		gradientsBox_->pack(new LayoutControl(view), true, true);
		gradientViews_.push_back(view);
		++index;
	}
}

int CGradientsWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 20, 20), this, 0, true));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientsWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onPositionChanged), 0);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);

	gradientsBox_ = new LayoutVBox;
   	createGradientViews();
	box->pack(gradientsBox_, true, true);

	box->pack(new LayoutControl(positionControl_), false, false);
	box->pack(new LayoutControl(colorSelector_), false, false);
	
    layout_->add(box);
	return 0;
}



void CGradientsWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientsWindow::onPositionChanged(int index)
{
	if(gradients_.empty())
		return;

	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = positionControl_->getSelection();
    view->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

void CGradientsWindow::onColorChanged()
{
	if(gradients_.empty())
		return;
	CGradientEditorView* view = getViewByIndex(currentGradient_);
	CKeyColor& gradient = view->getGradient();
	int selection = view->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientsWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, currentGradient_);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(currentGradient_);
}

void CGradientsWindow::onGradientViewChanged(int index)
{
	if(gradients_.empty())
		return;

	currentGradient_ = index;
	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = view->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

BOOL CGradientsWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CWnd::OnCommand(wParam, lParam);
}

CGradientEditorView* CGradientsWindow::getViewByIndex(int index)
{
	xassert(index >= 0 && index < gradientViews_.size());
	GradientViews::iterator it = gradientViews_.begin();
	std::advance(it, index);
	return *it;
}
#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientsWindow.h"

#include "TreeEditors\GradientEditor.h"
#include "TreeEditors\GradientPositionCtrl.h"
#include "TreeEditors\ColorSelector.h"
#include "TreeEditors\ColorUtils.h"

#include "mfc\LayoutMFC.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientsWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientsWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientsWindow::CGradientsWindow(CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
//, gradients_(gradients)
, colorSelector_(new CColorSelector(true, this))
, rulerControl_(new CGradientRulerCtrl(GRADIENT_POSITION_TIME))
, currentGradient_(0)
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

	//setGradientList(gradients);
	positionControl_ = new CGradientPositionCtrl(0, GRADIENT_POSITION_TIME, this);
}

void CGradientsWindow::setGradientList(const GradientList& gradients)
{
	gradients_ = gradients;
	currentGradient_ = 0;
	createGradientViews();
}

CGradientsWindow::~CGradientsWindow ()
{
}


void CGradientsWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientsWindow::createGradientViews()
{
	if(!::IsWindow(GetSafeHwnd()))
		return;
	if(!gradientsBox_){
		xassert(0);
		return;
	}

	GradientViews::iterator vit;
	FOR_EACH(gradientViews_, vit){
		CGradientEditorView* view = *vit;
		if(view && ::IsWindow(view->GetSafeHwnd()))
			view->DestroyWindow();
		delete view;
    }

	gradientsBox_->clear();
	int index = 0;
	GradientList::iterator it;
	FOR_EACH(gradients_, it){
		CGradientEditorView* view = new CGradientEditorView(this);
		VERIFY(view->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		view->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);
		view->signalColorChanged()    = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);

		view->setGradient(*it->gradient);
		gradientsBox_->pack(new LayoutControl(view), true, true);
		gradientViews_.push_back(view);
		++index;
	}
}

int CGradientsWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 20, 20), this, 0, true));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientsWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onPositionChanged), 0);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);

	gradientsBox_ = new LayoutVBox;
   	createGradientViews();
	box->pack(gradientsBox_, true, true);

	box->pack(new LayoutControl(positionControl_), false, false);
	box->pack(new LayoutControl(colorSelector_), false, false);
	
    layout_->add(box);
	return 0;
}



void CGradientsWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientsWindow::onPositionChanged(int index)
{
	if(gradients_.empty())
		return;

	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = positionControl_->getSelection();
    view->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

void CGradientsWindow::onColorChanged()
{
	if(gradients_.empty())
		return;
	CGradientEditorView* view = getViewByIndex(currentGradient_);
	CKeyColor& gradient = view->getGradient();
	int selection = view->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientsWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, currentGradient_);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(currentGradient_);
}

void CGradientsWindow::onGradientViewChanged(int index)
{
	if(gradients_.empty())
		return;

	currentGradient_ = index;
	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = view->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

BOOL CGradientsWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CWnd::OnCommand(wParam, lParam);
}

CGradientEditorView* CGradientsWindow::getViewByIndex(int index)
{
	xassert(index >= 0 && index < gradientViews_.size());
	GradientViews::iterator it = gradientViews_.begin();
	std::advance(it, index);
	return *it;
}
#include "StdAfx.h"
#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"
#include ".\GradientsWindow.h"

#include "TreeEditors\GradientEditor.h"
#include "TreeEditors\GradientPositionCtrl.h"
#include "TreeEditors\ColorSelector.h"
#include "TreeEditors\ColorUtils.h"

#include "mfc\LayoutMFC.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CGradientsWindow, CWnd)

BEGIN_MESSAGE_MAP(CGradientsWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CGradientsWindow::CGradientsWindow(CWnd* parent)
: layout_(new LayoutWindow(this))
, parent_(parent)
//, gradients_(gradients)
, colorSelector_(new CColorSelector(true, this))
, rulerControl_(new CGradientRulerCtrl(GRADIENT_POSITION_TIME))
, currentGradient_(0)
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

	//setGradientList(gradients);
	positionControl_ = new CGradientPositionCtrl(0, GRADIENT_POSITION_TIME, this);
}

void CGradientsWindow::setGradientList(const GradientList& gradients)
{
	gradients_ = gradients;
	currentGradient_ = 0;
	createGradientViews();
}

CGradientsWindow::~CGradientsWindow ()
{
}


void CGradientsWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

struct KeyColorComparer {
	bool operator() (const KeyColor& lhs, const KeyColor& rhs) {
		return lhs.time < rhs.time;
	}
};

void CGradientsWindow::createGradientViews()
{
	if(!::IsWindow(GetSafeHwnd()))
		return;
	if(!gradientsBox_){
		xassert(0);
		return;
	}

	GradientViews::iterator vit;
	FOR_EACH(gradientViews_, vit){
		CGradientEditorView* view = *vit;
		if(view && ::IsWindow(view->GetSafeHwnd()))
			view->DestroyWindow();
		delete view;
    }

	gradientsBox_->clear();
	int index = 0;
	GradientList::iterator it;
	FOR_EACH(gradients_, it){
		CGradientEditorView* view = new CGradientEditorView(this);
		VERIFY(view->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		view->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);
		view->signalColorChanged()    = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onGradientViewChanged), index);

		view->setGradient(*it->gradient);
		gradientsBox_->pack(new LayoutControl(view), true, true);
		gradientViews_.push_back(view);
		++index;
	}
}

int CGradientsWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 60), this, 0));
	VERIFY(positionControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 20, 20), this, 0, true));
	VERIFY(rulerControl_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 20), this, 0));

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	colorSelector_->signalColorChanged() = bindMethod(*this, &CGradientsWindow::onColorChanged);
	positionControl_->signalPositionChanged() = bindArgument<void>(bindMethod(*this, &CGradientsWindow::onPositionChanged), 0);

	LayoutVBox* box = new LayoutVBox(5, 5);

	box->pack(new LayoutControl(rulerControl_), false, false);

	gradientsBox_ = new LayoutVBox;
   	createGradientViews();
	box->pack(gradientsBox_, true, true);

	box->pack(new LayoutControl(positionControl_), false, false);
	box->pack(new LayoutControl(colorSelector_), false, false);
	
    layout_->add(box);
	return 0;
}



void CGradientsWindow::OnSize(UINT nType, int cx, int cy)
{
	layout_->relayout();
}

void CGradientsWindow::onPositionChanged(int index)
{
	if(gradients_.empty())
		return;

	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = positionControl_->getSelection();
    view->setSelection(selection);
	if(selection != -1){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

void CGradientsWindow::onColorChanged()
{
	if(gradients_.empty())
		return;
	CGradientEditorView* view = getViewByIndex(currentGradient_);
	CKeyColor& gradient = view->getGradient();
	int selection = view->getSelection();
	if(selection == -1)
		return;

	sColor4f& color = gradient[selection];

	color = colorSelector_->getColor();

	if(style_ & CGradientsWindow::STYLE_CYCLED){
		if(selection == 0)
			gradient[gradient.size() - 1].set(color.r, color.g, color.b, color.a);
		else if(selection == gradient.size() - 1)
			gradient[0].set(color.r, color.g, color.b, color.a);
	}
	view->Invalidate(FALSE);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, currentGradient_);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(currentGradient_);
}

void CGradientsWindow::onGradientViewChanged(int index)
{
	if(gradients_.empty())
		return;

	currentGradient_ = index;
	CGradientEditorView* view = getViewByIndex(index);
	CKeyColor& gradient = view->getGradient();

	int selection = view->getSelection();
	if(selection >= 0){
		sColor4f& color = gradient[selection];
		colorSelector_->setColor(color);
	}
	positionControl_->setSelection(selection);

	//
	GradientList::iterator it = gradients_.begin();
	std::advance(it, index);
	*it->gradient = gradient;

	if(signalGradientChanged_)
		signalGradientChanged_(index);
}

BOOL CGradientsWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Gradient Editor", style | WS_TABSTOP, rect, parent, id);
}

BOOL CGradientsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CWnd::OnCommand(wParam, lParam);
}

CGradientEditorView* CGradientsWindow::getViewByIndex(int index)
{
	xassert(index >= 0 && index < gradientViews_.size());
	GradientViews::iterator it = gradientViews_.begin();
	std::advance(it, index);
	return *it;
}
