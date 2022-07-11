#include "stdafx.h"
#include "resource.h"


#include "..\Util\MFC\MemDC.h"

#include "TEMiniMap.h"
#include "TriggerExport.h"
#include "TriggerEditor.h"
#include "TEEngine\TriggerEditorLogic.h"
#include "TEEngine\TriggerEditorView.h"

IMPLEMENT_DYNAMIC(TEMiniMap, CWnd)

TEMiniMap::TEMiniMap(TriggerEditor* triggerEditor)
: triggerEditor_(triggerEditor)
, triggerEditorView_(0)
{
	registerWindowClass();

	scaleX_ = 0.05f;
	scaleY_ = 0.05f;
}

void TEMiniMap::registerWindowClass()
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, className(), &wndclass) )
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
		wndclass.lpszClassName	= className();

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}
}

TEMiniMap::TEMiniMap()
: triggerEditor_(0)
, triggerEditorView_(0)
{
	assert(triggerEditor_);//его необходимо задать в другом месте
}

TEMiniMap::~TEMiniMap()
{
}

BEGIN_MESSAGE_MAP(TEMiniMap, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TEFrame message handlers


int TEMiniMap::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	const int SCALE_TOOLBAR_ID	= 2;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	
	return 0;
}

BOOL TEMiniMap::Create(CWnd* parent)
{
	return CreateEx(WS_EX_TOOLWINDOW, className(), "", WS_VISIBLE | WS_CHILD, CRect(0, 0, 200, 200), parent, 0);
}

void TEMiniMap::OnPaint()
{
	CPaintDC paintDC(this);

	COLORREF backgroundColor  (RGB(255, 255, 255));
	COLORREF triggerColor     (RGB(128, 128, 128));
	COLORREF startTriggerColor(RGB(255, 128, 128));

	CRect rt;
	GetClientRect(&rt);

	CMemDC dc(&paintDC);
	dc.FillSolidRect(&rt, backgroundColor);

    TriggerEditorView* view = triggerEditorView_;
	if(!view)
		return;

	TriggerEditorLogic* logic = view->getTriggerEditorLogic();
	if(!logic)
		return;

	CSize oldSize = dc.SetViewportExt(1000, 1000);

	TriggerChain& chain = *logic->getTriggerChain();
	TriggerList& triggers = chain.triggers;
	TriggerList::iterator it;

	updateView();

	FOR_EACH(triggers, it) {
		COLORREF color = triggerColor;
		if(it == triggers.begin()){
			color = startTriggerColor;
		}
		Trigger& trigger = *it;

		CRectSerialized triggerRect = trigger.boundingRect();
		
		rectToMinimap(triggerRect);
		dc.FillSolidRect(&triggerRect, color);
	}

	CPoint pt = view->getViewportOrg();

	CRect viewRect;
	view->GetClientRect(&viewRect);

    CRect frameRect(view->descaleX(-pt.x),
                    view->descaleY(-pt.y),
                    view->descaleX(viewRect.Width() - pt.x),
                    view->descaleY(viewRect.Height() - pt.y));

	rectToMinimap(frameRect);
	dc.DrawFocusRect(&frameRect);

}
void TEMiniMap::setView(TriggerEditorView* view)
{
	triggerEditorView_ = view;
	updateView();
}

void TEMiniMap::updateView()
{
	TriggerEditorView* view = triggerEditorView_;
	if(!view || !view->getTriggerEditorLogic())
		return;

	scaleX_ = float(windowSize_.cx) / max(1.0f, float(view->getWorkArea().Width()));
	float scaleY = float(windowSize_.cy) / max(1.0f, float(view->getWorkArea().Height()));
	float cellHeight = float(view->getGrid().getCellHeight()) * scaleY;
	scaleY_ = scaleY * float(max(1, int(cellHeight))) / cellHeight;

	viewOrigin_.x = view->getWorkArea().left * scaleX_;
	viewOrigin_.y = view->getWorkArea().top * scaleY_ - 0.5f * view->getWorkArea().Height() * (scaleY - scaleY_);
}

void TEMiniMap::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	windowSize_.SetSize(cx, cy);
	updateView();
}


BOOL TEMiniMap::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void TEMiniMap::rectToMinimap(CRect& rect)
{
	if(static_cast<CRectSerialized&>(rect).valid()){
		rect.left = round(rect.left * scaleX_)     - viewOrigin_.x;
		rect.top  =   round(rect.top * scaleY_)    - viewOrigin_.y;
		rect.right =  round(rect.right * scaleX_)  - viewOrigin_.x;
		rect.bottom = round(rect.bottom * scaleY_) - viewOrigin_.y;
	}
}

void TEMiniMap::pointFromMinimap(CPoint& point)
{
	if(static_cast<CPointSerialized&>(point).valid()){
		point.x =  round((point.x + viewOrigin_.x) / scaleX_);
		point.y =  round((point.y + viewOrigin_.y) / scaleY_);
	}
}

void TEMiniMap::OnLButtonDown(UINT nFlags, CPoint point)
{
    TriggerEditorView* view = triggerEditorView_;
	if(!view)
		return;
	
	pointFromMinimap(point);

	CRect viewRect;
	view->GetClientRect(&viewRect);
	view->scalePoint(&point);
	point.x -= viewRect.Width() / 2;
	point.y -= viewRect.Height() / 2;
    view->setViewportOrg(-point.x, -point.y);
	view->updateScrollers();
	view->Invalidate();
	CWnd::OnLButtonDown(nFlags, point);
}

void TEMiniMap::OnMouseMove(UINT nFlags, CPoint point)
{
    TriggerEditorView* view = triggerEditorView_;
	if(!view || !(nFlags & MK_LBUTTON))
		return;
	
	pointFromMinimap(point);

	CRect viewRect;
	view->GetClientRect(&viewRect);
	view->scalePoint(&point);
	point.x -= viewRect.Width() / 2;
	point.y -= viewRect.Height() / 2;
    view->setViewportOrg(-point.x, -point.y);
	view->updateScrollers();
	view->Invalidate();

	CWnd::OnMouseMove(nFlags, point);
}
