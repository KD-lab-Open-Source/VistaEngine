#ifndef __E_SCROLL_H_INCLUDED__
#define __E_SCROLL_H_INCLUDED__

class CEScroll;

class CEdit4CEScroll : public CEdit
{
	CEScroll * ownerClass;
// Construction
public:
	CEdit4CEScroll();
	void init(CEScroll * ownerClass);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEdit4CEScroll)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEdit4CEScroll();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEdit4CEScroll)
	afx_msg void OnKillfocus();
	afx_msg void OnChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CEScroll window
const int CESCROLL_BORDER_SIZE_X=2;//Рамка на которой не рисуется ничего
const int CESCROLL_BORDER_SIZE_Y=0;//Рамка на которой не рисуется ничего
const int CESCROLL_MAIN_LINE_DY=1; //Смещение для главной линии- линейки
const int CESCROLL_CURSOR_BEG_POINT_DY=2; //Смещение для начала отрисовки треугольного курсора

class CEScroll : public CWnd {
	static LPCSTR CLASSNAME;
//	static UINT MESSAGE_ID;
	static CPen PEN_BLACK, PEN_GRAY, PEN_WHITE;
	static COLORREF BACKGROUND, CURSOR_CONTUR_BLACK, CURSOR_CONTUR_FILL, CURSOR_PATCHTOLIGHT;
	int xBeg, yBeg, xLenght;
	int cursor_DXY05;

	void interSetPos(int x);
	void interSetValue(int newValue);
	virtual int getInEditWin(void);
	virtual void putToEditWin(int value);
	void PostMessageToParent(const int nTBCode) const;

// Construction
public:
	CEScroll();

// Attributes
public:
	int MIN, MAX, value;
	CEdit4CEScroll editWin;

// Operations
public:
	bool Create(CWnd* parent, int nIDScroller, int nIDEdit);
//	UINT GetMessageID(void);
	void SetPos(int newValue);
	int GetPos(void) const;
	void SetRange(int _min, int _max);

	void ShowControl(bool flag);

	void EditWin_EN_CHANGE(void);
	void EditWin_EN_KILLFOCUS(void);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEScroll)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEScroll();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEScroll)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
};

// CButton4EScroll2
class CButton4EScroll2 : public CButton
{
	DECLARE_DYNAMIC(CButton4EScroll2)

	class CEScroll2 * ownerClass;
public:
	CButton4EScroll2();
	virtual ~CButton4EScroll2();
	void init(CEScroll2 * _ownerClass);

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClicked();
};


class CEScroll2 : public CEScroll {
	CRect rectButtonIsLess, rectButtonIsBigger;
	CButton4EScroll2 buttonIsLess, buttonIsBigger;
	int stepButton;
public:
	CEScroll2();
	bool Create(CWnd* parent, int nIDScroller, int nIDEdit, int nIDButtonIsLess, int nIDButtonIsBigger, int _stepButton=1);

	void ButtonWin_BN_CLICKED(int nIDButton);
};



class CEScrollVx : public CEScroll
{
	int getInEditWin(void);
	void putToEditWin(int value);
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
