#include "StdAfx.h"
#include ".\CustomEdit.h"
#include "..\Util\MFC\TreeListCtrl.h"

CCustomEdit::CCustomEdit()
: m_ptrTreeCtrl(NULL)
, textHeight_(0)
, minHeight_(0)
{
}

CCustomEdit::~CCustomEdit()
{
}


BEGIN_MESSAGE_MAP(CCustomEdit, CEdit)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

int CCustomEdit::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_ptrTreeCtrl = reinterpret_cast<CTreeListCtrl*>(GetParent());

	minHeight_ = lpCreateStruct->cy;

	CDC* dc = GetDC();
	TEXTMETRIC tm;
	GetTextMetrics(*dc, &tm);
	ReleaseDC(dc);

	textHeight_ = tm.tmHeight;
	return 0;
}

void CCustomEdit::OnKillFocus(CWnd* pNewWnd) 
{
	CEdit::OnKillFocus(pNewWnd);
	
	m_ptrTreeCtrl->SetCtrlFocus( pNewWnd, FALSE );
	
}

void CCustomEdit::SetWindowText(LPCTSTR lpszString)
{
	ASSERT(IsWindow(m_hWnd));
	adjustControlHeight(lpszString);
	CEdit::SetWindowText(lpszString);
}

void CCustomEdit::adjustControlHeight(LPCTSTR lpszStr)
{

	int const iHeight = getCtrlHeight(lpszStr, textHeight_, minHeight_);
	CRect rc;
	GetWindowRect(&rc);
	rc.bottom = rc.top + iHeight;
	GetParent()->ScreenToClient(&rc);
	MoveWindow(&rc, FALSE);
	GetParent()->Invalidate();
}

int CCustomEdit::getCtrlHeight(LPCTSTR lpszStr, int charHeight, int cellHeight)
{
	int iReturnCount = 1;
	while ((lpszStr = _tcschr(lpszStr, _T('\n'))))
	{
		++iReturnCount;
		++lpszStr;
	}
	return round(float(iReturnCount * charHeight) / (cellHeight + 1.0f)) * (cellHeight + 1) - 1;
}

void CCustomEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(nChar == VK_RETURN && bool(GetKeyState(VK_CONTROL))) {
		CRect rc;
		GetWindowRect(&rc);
		rc.bottom = rc.top + round(float(rc.bottom - rc.top + textHeight_) / (minHeight_ + 1.0f)) * (minHeight_ + 1);
		GetParent()->ScreenToClient(&rc);
		MoveWindow(&rc, FALSE);
	}
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CCustomEdit::OnChange() 
{
	CString str;
	GetWindowText(str);
	adjustControlHeight(str);	
}
