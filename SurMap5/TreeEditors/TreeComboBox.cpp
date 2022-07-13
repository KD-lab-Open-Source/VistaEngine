#include "stdafx.h"

#include "TreeComboBox.h"
////////////////////////////////////

IMPLEMENT_DYNAMIC(CPopupTreeCtrl, CTreeCtrl)
BEGIN_MESSAGE_MAP(CPopupTreeCtrl, CTreeCtrl)
//	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


////////////////////////////////////
IMPLEMENT_DYNAMIC(CTreeComboBox, CButton)
BEGIN_MESSAGE_MAP(CTreeComboBox, CButton)
	ON_WM_PAINT()	
	ON_WM_SETFOCUS()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CTreeComboBox* CTreeComboBox::m_pActiveComboBox = 0;
WNDPROC CTreeComboBox::m_parentWndProc = 0;


////////////////////////////////////
void CTreeComboBox::onItemSelected(HTREEITEM item)
{
	hideDropDown();
}

void CTreeComboBox::OnLButtonDown(UINT nFlags, CPoint point) 
{
	hideDropDown();
}

BOOL CTreeComboBox::Create(DWORD style, const CRect& rect, CWnd* parent, UINT id)
{
	return __super::CreateEx(0, className(), "", style, rect, parent, id);
}

void CTreeComboBox::registerWindowClass ()
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

CTreeComboBox::CTreeComboBox()
{
	registerWindowClass();
}

void CTreeComboBox::OnPaint()
{
	CPaintDC dc(this);
    CRect rt;
	GetClientRect(&rt);
	dc.FillSolidRect(&rt, RGB(255, 255, 255));
	CFont* oldFont = dc.SelectObject(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	dc.DrawText(text_.c_str(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	CRect buttonRect(rt.right - 20, rt.top, rt.right, rt.bottom);
	dc.SelectObject(oldFont);
}

void CTreeComboBox::showDropDown()
{
	CRect windowRect;
    GetWindowRect(&windowRect);
    CRect rt(windowRect.left,  windowRect.bottom,
             min(GetSystemMetrics(SM_CXSCREEN), windowRect.right + windowRect.Width()), windowRect.bottom + 400);

	tree_.SetWindowPos(0, rt.left, rt.top, rt.Width(), rt.Height(), SWP_SHOWWINDOW);
	tree_.SetFocus();
	m_pActiveComboBox = this;
}

void CTreeComboBox::hideDropDown(bool byLostFocus)
{
	tree_.ShowWindow(SW_HIDE);
	xassert (::IsWindow (GetSafeHwnd ()));
}

int CTreeComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
    
    CRect rt(0, 0, 0, 0);
	GetParent()->ClientToScreen(&rt);
	rt.right += rt.right - rt.left;

	static_cast<CWnd&>(tree_).CreateEx (0, WC_TREEVIEW, "", 
		TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT | WS_BORDER | WS_POPUP, rt, this, 0);
	tree_.SetFont(GetFont());
	tree_.setParent(this);
	return 0;
}

void CTreeComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	if(pNewWnd != &tree_)
		hideDropDown(true);
}

void CTreeComboBox::OnSetFocus(CWnd* pOldWnd)
{
//	CWnd::OnSetFocus(pOldWnd);

	tree_.SetFocus();
}

void CTreeComboBox::interceptParentWndProc()
{
	// ASSERT
	CWnd *pwndParent = GetParent();
	if (!pwndParent)
		return;
	if (!pwndParent->GetSafeHwnd())
		return;

	// GET Parent WinProc & SET our function
	if (m_parentWndProc)
		return;
	m_parentWndProc = (WNDPROC)::SetWindowLong(
		pwndParent->GetSafeHwnd(), 
		GWL_WNDPROC, 
		(long)(WNDPROC)ParentWindowProc);
}

void CTreeComboBox::unInterceptParentWndProc()
{
	// ASSERT
	CWnd *pwndParent = GetParent();
	if (!pwndParent) return;
	if (!pwndParent->GetSafeHwnd()) return;

	// SET Parent WinProc = UNINTERCEPT
	(WNDPROC)::SetWindowLong(
		pwndParent->GetSafeHwnd(), 
		GWL_WNDPROC, 
		(long)(WNDPROC)m_parentWndProc);
}

////////////////////
void CPopupTreeCtrl::setParent(CTreeComboBox* menu)
{
    parent_ = menu;
}

void CPopupTreeCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	UINT flags ;
	HTREEITEM item = HitTest(point, &flags);
	if (item != NULL)
		CTreeCtrl::SelectItem(item);	
	CTreeCtrl::OnMouseMove(nFlags, point);
}

void CPopupTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	UINT flags;
	HitTest(point, &flags);
	if(flags != TVHT_ONITEMBUTTON){
		HTREEITEM item = GetSelectedItem();
		CString str = "";
		if(!ItemHasChildren(item)){
			xassert(parent_);
			parent_->onItemSelected(item);
			return;
		}	
	}
	CTreeCtrl::OnLButtonDown(nFlags, point);
}


void CPopupTreeCtrl::OnSelChanged(NMHDR* nm, LRESULT* result)
{
	NMTREEVIEW* notify = (NMTREEVIEW*)(nm);
	HTREEITEM item = notify->itemNew.hItem;
	
	*result = 0;
}

CPopupTreeCtrl::CPopupTreeCtrl()
: parent_(0)
{
}

void CPopupTreeCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CTreeCtrl::OnKillFocus(pNewWnd);
    
	parent_->hideDropDown(true);
}

void CTreeComboBox::PreSubclassWindow()
{
	__super::PreSubclassWindow();

	interceptParentWndProc();
}

void CTreeComboBox::OnDestroy()
{
    unInterceptParentWndProc();
	__super::OnDestroy();
}

LRESULT CALLBACK CTreeComboBox::ParentWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// CHECK Message
	if (nMsg==WM_COMMAND || 
        nMsg==WM_SYSCOMMAND ||
		nMsg==WM_SYSKEYDOWN ||
		nMsg==WM_LBUTTONDOWN ||
		nMsg==WM_NCLBUTTONDOWN)
	{
		// FILTER Messages
		if (!IsMsgOK(hWnd, nMsg, lParam))
			m_pActiveComboBox->hideDropDown();
	}
	
	// CALL Parent
	if (!m_parentWndProc) 
		return NULL;
	
	return CallWindowProc( m_parentWndProc, hWnd, nMsg, wParam, lParam );
}

BOOL CTreeComboBox::IsMsgOK(HWND hWnd, UINT nMsg, /*WPARAM wParam,*/ LPARAM lParam)
{
	// ASSERT
	if (!hWnd) 
		return FALSE;
	if (nMsg != WM_COMMAND) 
		return FALSE;
	if (!m_pActiveComboBox) 
		return FALSE;
	if ((HWND)lParam != m_pActiveComboBox->GetSafeHwnd()) 
		return FALSE;

	//check if mouse pos is in drop down rect
	CRect rc;
	m_pActiveComboBox->GetWindowRect(rc);
	CPoint pt;
	GetCursorPos(&pt);
	if (!rc.PtInRect(pt))
		return FALSE;
	
	return TRUE;
}
