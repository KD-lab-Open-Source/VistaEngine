#include "stdafx.h"
#include ".\HotKeySelectorDlg.h"
#include "Dictionary.h"


IMPLEMENT_DYNAMIC(CHotKeySelectorDlg, CDialog)
CHotKeySelectorDlg::CHotKeySelectorDlg(const sKey& key, CWnd* parent)
: CDialog(CHotKeySelectorDlg::IDD, parent)
, key_(key)
, pressed_(false)
{
}

CHotKeySelectorDlg::~CHotKeySelectorDlg()
{
}

void CHotKeySelectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CHotKeySelectorDlg, CDialog)
	ON_WM_KEYDOWN()
	ON_WM_SYSKEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SYSKEYUP()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()

	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()

	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()

	ON_WM_TIMER()
END_MESSAGE_MAP()

void CHotKeySelectorDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if((nFlags >> 14) % 2 == 0){ // кнопка не была нажата
		key_ = sKey(nChar, true);
		pressed_ = true;
		CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
		if(nChar != VK_SHIFT && nChar != VK_CONTROL && nChar != VK_MENU)
			EndModalLoop(IDOK);
	}
}

void CHotKeySelectorDlg::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	key_ = sKey(nChar, true);
	pressed_ = true;

	if(nChar == VK_CANCEL)
		EndModalLoop(IDCANCEL);

	if(nChar != VK_SHIFT && nChar != VK_CONTROL && nChar != VK_MENU)
		EndModalLoop(IDOK);
}

void CHotKeySelectorDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(pressed_){
		key_ = sKey(nChar, true);
		CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
		EndModalLoop(IDOK);
	}
}

void CHotKeySelectorDlg::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(pressed_){
		key_ = sKey(nChar, true);
		EndModalLoop(IDOK);
	}
	// CDialog::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

void CHotKeySelectorDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	dc.SelectObject(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	dc.SetBkMode(TRANSPARENT);
	CString str(TRANSLATE("Пожалуйста, нажмите кнопку..."));
	CRect rt;
	GetClientRect(&rt);
	dc.DrawText(str, &rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}

void CHotKeySelectorDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	key_ = sKey(VK_LDBL, true);
	EndModalLoop(IDOK);

	CDialog::OnLButtonDblClk(nFlags, point);
}

void CHotKeySelectorDlg::OnMButtonDown(UINT nFlags, CPoint point)
{
	pressed_ = true;
	CDialog::OnMButtonDown(nFlags, point);
}

void CHotKeySelectorDlg::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	CDialog::OnMButtonDblClk(nFlags, point);
}

BOOL CHotKeySelectorDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	key_ = sKey(zDelta > 0 ? VK_WHEELUP : VK_WHEELDN, true);
	pressed_ = true;
	EndModalLoop(IDOK);

	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}

void CHotKeySelectorDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	pressed_ = true;
	CDialog::OnLButtonDown(nFlags, point);
}

void CHotKeySelectorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(pressed_){
		key_ = sKey(VK_LBUTTON, true);
		ticksLeft_ = 30;
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CHotKeySelectorDlg::OnMButtonUp(UINT nFlags, CPoint point)
{
	if(pressed_){
		key_ = sKey(VK_MBUTTON, true);
		ticksLeft_ = 30;
	}
	CDialog::OnMButtonUp(nFlags, point);
}

void CHotKeySelectorDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	pressed_ = true;
	CDialog::OnRButtonDown(nFlags, point);
}

void CHotKeySelectorDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	if(pressed_){
		key_ = sKey(VK_RBUTTON, true);
		ticksLeft_ = 30;
	}
	CDialog::OnRButtonUp(nFlags, point);
}

void CHotKeySelectorDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	key_ = sKey(VK_RDBL, true);
	EndModalLoop(IDOK);

	CDialog::OnRButtonDblClk(nFlags, point);
}


BOOL CHotKeySelectorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	this->SetTimer(100, 10, 0);
	ticksLeft_ = -1;
	pressed_ = false;

	return TRUE;
}

void CHotKeySelectorDlg::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == 100){
		if(ticksLeft_ > 0){
			--ticksLeft_;
			if(ticksLeft_ == 0 && (key_.key == VK_LBUTTON || key_.key == VK_RBUTTON || key_.key == VK_MBUTTON))
				EndModalLoop(IDOK);
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CHotKeySelectorDlg::OnCancel()
{
	key_ = sKey(VK_ESCAPE, true);
	EndModalLoop(IDOK);
}

void CHotKeySelectorDlg::OnOK()
{
	key_ = sKey(VK_RETURN, true);
	CDialog::OnOK();
}
