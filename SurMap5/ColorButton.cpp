// ColorButton.cpp : implementation file
//

#include "stdafx.h"
#include "ColorButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorButton
IMPLEMENT_DYNAMIC(CColorButton, CButton);

CColorButton::CColorButton()
{
	m_bg=RGB(255,0,0);
	m_State=0;
	m_buttonType=SIMPLY;
}

CColorButton::~CColorButton()
{
}

BOOL CColorButton::Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, eButtonType butType)
{
	switch(butType){
	case SIMPLY:
		m_buttonType=SIMPLY;
		break;
	case AUTORADIO:
		m_buttonType=AUTORADIO;
		break;
	};
	return CButton::Create(lpszCaption, dwStyle|BS_OWNERDRAW, rect, pParentWnd, nID);
}

// ON_CONTROL_REFLECT_EX необходим, чтобы Owner получал BN_CLICKED  (в OnClicked надо вставить return FALSE)
BEGIN_MESSAGE_MAP(CColorButton, CButton)
	//{{AFX_MSG_MAP(CColorButton)
	ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorButton message handlers

void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// TODO: Add your code to draw the specified item
/*	RECT r;
	GetClientRect(&r);
	CBrush * pBrush;
	pBrush = new CBrush(RGB(255,0,255));
	CClientDC * CDC;
	CDC= new CClientDC(this);
	CDC->FillRect(&r,pBrush);
	CDC->SetPixel(r.top,r.left,RGB(255,0,0));
	CDC->LineTo(r.right, r.bottom);
	delete CDC;
	delete pBrush;*/

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	CRect focusRect, btnRect;
	focusRect.CopyRect(&lpDrawItemStruct->rcItem); 
	btnRect.CopyRect(&lpDrawItemStruct->rcItem); 
	//
	// Set the focus rectangle to just past the border decoration
	//
	focusRect.left += 4;
    focusRect.right -= 4;
    focusRect.top += 4;
    focusRect.bottom -= 4;
      
	//
	// Draw and label the button using draw methods 
	//
    DrawFilledRect(pDC, btnRect, m_bg); 
    DrawFrame(pDC, btnRect, 2);

	//
	// Now, depending upon the state, redraw the button (down image) if it is selected,
	// place a focus rectangle on it, or redisplay the caption if it is disabled
	//
	if (m_State==1) {
	    DrawFilledRect(pDC, btnRect, m_bg); 
    	DrawFrame(pDC, btnRect, -1);
	}
	else{
		UINT state = lpDrawItemStruct->itemState; 
		if (state & ODS_FOCUS) {
			//DrawFocusRect(lpDrawItemStruct->hDC, (LPRECT)&focusRect);
			if (state & ODS_SELECTED){
	    		DrawFilledRect(pDC, btnRect, m_bg); 
    			DrawFrame(pDC, btnRect, -1);
				//DrawFocusRect(lpDrawItemStruct->hDC, (LPRECT)&focusRect);
			}
		}
		else if (state & ODS_DISABLED) {
    		//COLORREF disabledColor = bg ^ 0xFFFFFF; // contrasting color
		}
	}
}
void CColorButton::DrawFrame(CDC *DC, CRect R, int Inset)
{ 
	COLORREF dark, light, tlColor, brColor;
	int i, m, width;
	width = (Inset < 0)? -Inset : Inset;
	
	for (i = 0; i < width; i += 1) {
		m = 255 / (i + 2);
		dark = PALETTERGB(m, m, m);
		m = 192 + (63 / (i + 1));
		light = PALETTERGB(m, m, m);
		  
	  	if ( width == 1 ) {
			light = RGB(255, 255, 255);
			dark = RGB(128, 128, 128);
		}
		
		if ( Inset < 0 ) {
			tlColor = dark;
			brColor = light;
		}
		else {
			tlColor = light;
			brColor = dark;
		}
		
		DrawLine(DC, R.left, R.top, R.right, R.top, tlColor);							// Across top
		DrawLine(DC, R.left, R.top, R.left, R.bottom, tlColor);							// Down left
	  
		if ( (Inset < 0) && (i == width - 1) && (width > 1) ) {
			DrawLine(DC, R.left + 1, R.bottom - 1, R.right, R.bottom - 1, RGB(1, 1, 1));// Across bottom
			DrawLine(DC, R.right - 1, R.top + 1, R.right - 1, R.bottom, RGB(1, 1, 1));	// Down right
		}
	  	else {
			DrawLine(DC, R.left + 1, R.bottom - 1, R.right, R.bottom - 1, brColor);		// Across bottom
			DrawLine(DC, R.right - 1, R.top + 1, R.right - 1, R.bottom, brColor);		// Down right
		}
	  	InflateRect(R, -1, -1);
	}
}

void CColorButton::DrawFilledRect(CDC *DC, CRect R, COLORREF color)
{ 
	CBrush B;
	B.CreateSolidBrush(color);
	DC->FillRect(R, &B);
}
 

void CColorButton::DrawLine(CDC *DC, CRect EndPoints, COLORREF color)
{ 
	CPen newPen;
	newPen.CreatePen(PS_SOLID, 1, color);
	CPen *oldPen = DC->SelectObject(&newPen);
	DC->MoveTo(EndPoints.left, EndPoints.top);
	DC->LineTo(EndPoints.right, EndPoints.bottom);
	DC->SelectObject(oldPen);
    newPen.DeleteObject();
}

void CColorButton::DrawLine(CDC *DC, long left, long top, long right, long bottom, COLORREF color)
{ 
	CPen newPen;
	newPen.CreatePen(PS_SOLID, 1, color);
	CPen *oldPen = DC->SelectObject(&newPen);
	DC->MoveTo(left, top);
	DC->LineTo(right, bottom);
	DC->SelectObject(oldPen);
    newPen.DeleteObject();
}

BOOL CColorButton::OnClicked() 
{
	// TODO: Add your control notification handler code here
	if(m_buttonType==AUTORADIO){
		CWnd * cwnd=GetOwner();
		CColorButton * cbp=this;
		while((cbp=(CColorButton*)cwnd->GetNextDlgGroupItem(cbp))!=this){
			ASSERT_KINDOF(CColorButton, cbp);
			cbp->setState(0);
		}
		setState(1);
	}
	return FALSE;
}


