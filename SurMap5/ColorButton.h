#if !defined(AFX_COLORBUTTON_H__ED0DCA4B_9D2E_4237_8BCF_8CFAF8768529__INCLUDED_)
#define AFX_COLORBUTTON_H__ED0DCA4B_9D2E_4237_8BCF_8CFAF8768529__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColorButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CColorButton window

class CColorButton : public CButton
{
// Construction
public:
	DECLARE_DYNAMIC(CColorButton);
	CColorButton(void);

// Attributes
public:

// Operations
public:
	enum eButtonType{
		SIMPLY,
		AUTORADIO
	};
	eButtonType m_buttonType;
	// ВНИМАНИЕ !!! в Create не передавать нижних стилей (с BS_PUSHBUTTON по BS_OWNERDRAW)
	BOOL Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, eButtonType butType=SIMPLY);

	bool m_State;
	void setState(bool state){
		if(m_State!=state){
			m_State=state;
			Invalidate(FALSE);
		}
	};
	void SetColor(COLORREF color){
		m_bg=color;
		Invalidate(); 
	};
	COLORREF GetColor(void){
		return m_bg;
	};

	void DrawFrame(CDC *DC, CRect R, int Inset);
	void DrawFilledRect(CDC *DC, CRect R, COLORREF color);
	void DrawLine(CDC *DC, CRect EndPoints, COLORREF color);
	void DrawLine(CDC *DC, long left, long top, long right, long bottom, COLORREF color);
	void DrawButtonText(CDC *DC, CRect R, const char *Buf, COLORREF TextColor);

	COLORREF m_fg, m_bg, m_disabled;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorButton)
	afx_msg BOOL OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORBUTTON_H__ED0DCA4B_9D2E_4237_8BCF_8CFAF8768529__INCLUDED_)
