#include "StdAfx.h"

#include "ColorSelectorWindow.h"
#include "SolidColorCtrl.h"
#include "ColorSelector.h"
#include "ColorRampCtrl.h"

#include "mfc\LayoutMFC.h"

IMPLEMENT_DYNAMIC(CColorSelectorWindow, CWnd)
BEGIN_MESSAGE_MAP(CColorSelectorWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CColorSelectorWindow::CColorSelectorWindow(bool hasAlpha, CWnd* parent)
: layout_(new LayoutWindow(this))
, colorRamp_(new CColorRampCtrl)
, colorSelector_(new CColorSelector(!hasAlpha, false, this))
, modalResult_(false)
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

CColorSelectorWindow::~CColorSelectorWindow()
{

}

BOOL CColorSelectorWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Color Selector", style | WS_TABSTOP, rect, parent, id);
}

int CColorSelectorWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

    LayoutVBox* vbox = new LayoutVBox(5, 5);
	VERIFY(colorRamp_->Create(WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(0, 0, 128 + GetSystemMetrics(SM_CXBORDER) * 2 + 12, 128 + GetSystemMetrics(SM_CYBORDER) * 2), this, 0));
    vbox->pack(new LayoutControl(colorRamp_), true, true);
	colorRamp_->signalColorChanged() = bindArgument(bindMethod(*this, onColorChanged), 0);
	colorRamp_->setColor(color_);

	VERIFY(colorSelector_->Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 200, 100), this, 0));
	colorSelector_->signalColorChanged() = bindArgument(bindMethod(*this, onColorChanged), 1);
	colorSelector_->setColor(color_);
    vbox->pack(new LayoutControl(colorSelector_), false, false);
	{
		LayoutHBox* hbox = new LayoutHBox(5, 0);
		hbox->pack(new LayoutElement(), true, true);
		VERIFY(okButton_.Create("OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, CRect(0, 0, 80, 25), this, IDOK));
        okButton_.SetFont(font);
		hbox->pack(new LayoutControl(&okButton_));
		VERIFY(cancelButton_.Create("Cancel", WS_VISIBLE | WS_CHILD, CRect(0, 0, 80, 25), this, IDCANCEL));
        cancelButton_.SetFont(font);
		hbox->pack(new LayoutControl(&cancelButton_));

		vbox->pack(hbox);
	}
	//VERIFY(solidColorCtrl_->Create(WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(0, 0, 200, 100), this, 0));
	//solidColorCtrl_->signalColorChanged()
    //vbox->pack(new LayoutControl(solidColorCtrl_), true, true);

    //hbox->pack(vbox, true, true);

    layout_->add(vbox);
    return 0;
}

void CColorSelectorWindow::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	layout_->relayout();
}

void CColorSelectorWindow::OnClose()
{
	modalResult_ = false;
	EndModalLoop(IDCANCEL);
}

bool CColorSelectorWindow::doModal(CWnd* parent)
{
	if(parent && ::IsWindow(parent->GetSafeHwnd()))
		parent->EnableWindow(FALSE);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int width = 400;
	int height = 500;
	int left = (screenWidth - width) / 2;
	int top = (screenHeight - height) / 2;

	VERIFY(Create(WS_VISIBLE | WS_OVERLAPPEDWINDOW, CRect(left, top, left + width, top + height), parent, 0));
	RunModalLoop(0);

	ShowWindow(SW_HIDE);
	DestroyWindow();

	if(parent && ::IsWindow(parent->GetSafeHwnd())){
		parent->EnableWindow(TRUE);
		parent->SetActiveWindow();
		parent->SetFocus();
	}  

	return modalResult_;
}

BOOL CColorSelectorWindow::OnEraseBkgnd(CDC* pDC)
{
	return __super::OnEraseBkgnd(pDC);
}

void CColorSelectorWindow::onColorChanged(int index)
{
    std::vector< Functor0<const sColor4f&> > getters;
    std::vector< Functor1<void, const sColor4f&> > setters;

	const CColorRampCtrl* colorRamp = &*colorRamp_;
    getters.push_back(bindMethod(*colorRamp_, &CColorRampCtrl::getColor));
    setters.push_back(bindMethod(*colorRamp_, &CColorRampCtrl::setColor));

    getters.push_back(bindMethod(*colorSelector_, &CColorSelector::getColor));
    setters.push_back(bindMethod(*colorSelector_, &CColorSelector::setColor));

    float alpha = colorSelector_->getColor().a;
    color_ = getters[index]();
    color_.a = alpha;

    for(int i = 0; i < getters.size(); ++i){
        if(i != index)
            setters[i](color_);
    }

    /*(
	switch(index){
    case 0:
        getColor = colorRamp_->getColor();
        break;
    case 1:
        getColor = colorRamp_->getColor();
        break;
	}
    */
}

BOOL CColorSelectorWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(wParam == IDOK){
		modalResult_ = true;
		EndModalLoop(IDOK);
	}
	else if(wParam == IDCANCEL){
		modalResult_ = false;
		EndModalLoop(IDCANCEL);
	}
	return CWnd::OnCommand(wParam, lParam);
}

void CColorSelectorWindow::setColor(const sColor4f& color)
{
    color_ = color;
	if(::IsWindow(GetSafeHwnd())){
		colorSelector_->setColor(color_);
		colorRamp_->setColor(color_);
	}
}

BOOL CColorSelectorWindow::PreTranslateMessage(MSG* msg)
{
	if(msg->message == WM_KEYDOWN){
		if(msg->wParam == VK_RETURN){
			PostMessage(WM_COMMAND, IDOK, 0);
			return TRUE;
		}
		else if(msg->wParam == VK_ESCAPE){
			PostMessage(WM_COMMAND, IDCANCEL, 0);
			return TRUE;
		}
	}
	return FALSE;
	//return CWnd::PreTranslateMessage(msg);
}
