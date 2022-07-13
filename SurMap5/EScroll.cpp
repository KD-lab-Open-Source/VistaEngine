// EScroll.cpp : implementation file
//

#include "stdafx.h"
#include "EScroll.h"
#include "..\terra\terra.h"

#include "xmath.h"
#include ".\escroll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEScroll
LPCSTR CEScroll::CLASSNAME=NULL;
//UINT CEScroll::MESSAGE_ID=0;

COLORREF CEScroll::BACKGROUND=::GetSysColor(COLOR_BTNFACE);//RGB(180, 200, 180);//RGB(148, 148, 148);//
COLORREF CEScroll::CURSOR_CONTUR_BLACK=RGB(0,0,0);
COLORREF CEScroll::CURSOR_CONTUR_FILL=RGB(148, 148, 148);
COLORREF CEScroll::CURSOR_PATCHTOLIGHT=RGB(235, 235, 235);
CPen CEScroll::PEN_BLACK,
	CEScroll::PEN_GRAY,
	CEScroll::PEN_WHITE;

CEScroll::CEScroll()
{
	editWin.init(this);// Для обратного интерфейса

	if(CLASSNAME==NULL)
	{
		CLASSNAME=AfxRegisterWndClass(0); 
		PEN_BLACK.CreatePen(PS_SOLID, 1, CURSOR_CONTUR_BLACK);
		PEN_GRAY.CreatePen(PS_SOLID, 1, CURSOR_CONTUR_FILL);
		PEN_WHITE.CreatePen(PS_SOLID, 1, CURSOR_PATCHTOLIGHT);
	}
	MIN=0; MAX=100; value=0;
	xBeg=yBeg=xLenght=0;
}

CEScroll::~CEScroll()
{
}
/*
//Для связи через уникальное сообщение (не через HSCROLL)
UINT CEScroll::GetMessageID()
{
	if(MESSAGE_ID==0)
		MESSAGE_ID=RegisterWindowMessage("CInternalScroll");
	return MESSAGE_ID;
}
//В модуле где используется слайдер
const UINT nMessageInternalScroll=CInternalScroll::GetMessageID();
BEGIN_MESSAGE_MAP(CXxxDlg, CDialog)
	ON_REGISTERED_MESSAGE( nMessageInternalScroll, InternalScroll)
END_MESSAGE_MAP()
*/



BEGIN_MESSAGE_MAP(CEScroll, CWnd)
	//{{AFX_MSG_MAP(CEScroll)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
//	ON_WM_SIZE()
//	ON_WM_MOVE()
//	ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEScroll message handlers

bool CEScroll::Create(CWnd* parent, int nIDScroller, int nIDEdit)
{
	CRect rectScroll;
	CWnd* p;
	p=parent->GetDlgItem(nIDScroller);
	if(p==NULL)return false;
	//Получение координат окна и создание вместо него своего
//	p->GetClientRect(&rectScroll);
//	p->MapWindowPoints(parent, &rectScroll);
	p->GetWindowRect(&rectScroll); 
	parent->ScreenToClient(&rectScroll);
	p->DestroyWindow();
	//Сейчас это считается при отрисовке
	//cursor_DXY05=rectScroll.Height()-CESCROLL_CURSOR_BEG_POINT_DY-2*CESCROLL_BORDER_SIZE_Y-2;
	//xLenght=rectScroll.Width() - 2*CESCROLL_BORDER_SIZE_X- 2*cursor_DXY05;
	//if(xLenght<0) xLenght=0;
	//xBeg=CESCROLL_BORDER_SIZE_X + cursor_DXY05;
	//yBeg=CESCROLL_BORDER_SIZE_Y;

	CRect rectEdit;
	p=parent->GetDlgItem(nIDEdit);
	if(p==NULL)return false;
//	p->GetClientRect(&rectEdit);
//	p->MapWindowPoints(parent, &rectEdit);
	p->GetWindowRect(&rectEdit);
	parent->ScreenToClient(&rectEdit);
	p->DestroyWindow();

	if(!CWnd::Create(CLASSNAME, "InternalScroll", WS_CHILD|WS_VISIBLE, rectScroll, parent, nIDScroller))
		return false;

	CFont* font=parent->GetFont();
	//editWin.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_CHILD|WS_VISIBLE|WS_TABSTOP, rc, parent, 0);
	editWin.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_CHILD|WS_VISIBLE|WS_TABSTOP, rectEdit, parent,  0);//CRect(100,0,130,20), this,
	editWin.SetFont(font);
	putToEditWin(GetPos());//Udate текстового окна

	return true;
}
void CEScroll::ShowControl(bool flag)
{
	if(flag){
		editWin.ShowWindow(SW_SHOW);
		this->ShowWindow(SW_SHOW);
	}
	else {
		editWin.ShowWindow(SW_HIDE);
		this->ShowWindow(SW_HIDE);
	}
}


void CEScroll::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here

	//Расчет скроллера
	CRect rectScroll;
	GetClientRect(&rectScroll);
	cursor_DXY05=rectScroll.Height()-CESCROLL_CURSOR_BEG_POINT_DY-2*CESCROLL_BORDER_SIZE_Y-2;
	xLenght=rectScroll.Width() - 2*CESCROLL_BORDER_SIZE_X- 2*cursor_DXY05;
	if(xLenght<0) xLenght=0;
	xBeg=CESCROLL_BORDER_SIZE_X + cursor_DXY05;
	yBeg=CESCROLL_BORDER_SIZE_Y;

	CDC MemDC;
	MemDC.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, rectScroll.Width(), rectScroll.Height());
	CBitmap* pOldBmp=MemDC.SelectObject(&bmp);
////////////////////////////
	MemDC.FillSolidRect(0, 0, rectScroll.Width(), rectScroll.Height(), BACKGROUND);


	CPen* pOldPen;
	pOldPen=MemDC.SelectObject(&PEN_GRAY);
	int i;
	for(i=0; i<4; i++){
		MemDC.MoveTo(xBeg-1, yBeg+i);
		MemDC.LineTo(xBeg+xLenght+1, yBeg+i);
	}

	MemDC.SelectObject(&PEN_BLACK);
	MemDC.MoveTo(xBeg, yBeg+CESCROLL_MAIN_LINE_DY);
	MemDC.LineTo(xBeg+xLenght, yBeg+CESCROLL_MAIN_LINE_DY);


	if(MAX-MIN>0){
		int dx=round((float)(value-MIN)*((float)xLenght/(float)(MAX-MIN)));
		MemDC.MoveTo(xBeg+dx, yBeg+CESCROLL_CURSOR_BEG_POINT_DY);
		MemDC.LineTo(xBeg+dx+cursor_DXY05, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+cursor_DXY05);
		MemDC.LineTo(xBeg+dx-cursor_DXY05, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+cursor_DXY05);
		MemDC.LineTo(xBeg+dx, yBeg+CESCROLL_CURSOR_BEG_POINT_DY);

		CBrush fillCursor(CURSOR_CONTUR_FILL);
		CBrush * pOldBrush=MemDC.SelectObject(&fillCursor);
		MemDC.FloodFill(xBeg+dx, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+1, CURSOR_CONTUR_BLACK);
		MemDC.SelectObject(pOldBrush);

		MemDC.SelectObject(&PEN_WHITE);
		MemDC.MoveTo(xBeg+dx-1, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+2);
		MemDC.LineTo(xBeg+dx+0, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+3);
		MemDC.LineTo(xBeg+dx-2, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+3);
		MemDC.LineTo(xBeg+dx-1, yBeg+CESCROLL_CURSOR_BEG_POINT_DY+2);

	}


////////////////////////////	
	dc.BitBlt(0,0,rectScroll.Width(), rectScroll.Height(), &MemDC, 0, 0, SRCCOPY);

	MemDC.SelectObject(pOldPen);
	MemDC.SelectObject(pOldBmp); //DeleteDC не нужен он вызывается автоматически в деструкторе
	bmp.DeleteObject();
	
	// Do not call CWnd::OnPaint() for painting messages
}

void CEScroll::interSetPos(int x)
{
	int newValue= MIN + round( (float)(x-xBeg)/((float)xLenght/(float)(MAX-MIN)) );
	if(newValue!=GetPos()){
		interSetValue(newValue);
		putToEditWin(GetPos());
	}
}
void CEScroll::interSetValue(int newValue)
{
	if(newValue < MIN) newValue=MIN;
	if(newValue > MAX) newValue=MAX;
	if(newValue!=value){
		value=newValue;
		if(::IsWindow(m_hWnd)){
			Invalidate(FALSE);
			UpdateWindow(); //Для немедленной отрисовки
			//SendHScroll();
			PostMessageToParent(TB_THUMBTRACK);
		}
	}
}
void CEScroll::PostMessageToParent(const int nTBCode) const
{
	CWnd* pWnd = GetParent();
	if(pWnd) pWnd->SendMessage(WM_HSCROLL, (WPARAM)((GetPos() << 16) | nTBCode), (LPARAM)GetSafeHwnd());
}


void CEScroll::SetPos(int newValue)
{
	if(newValue!=GetPos()){
		interSetValue(newValue);
		putToEditWin(GetPos());
	}
}
int CEScroll::GetPos(void) const
{
	return value;
}
void CEScroll::SetRange(int _min, int _max)
{
	MIN=_min; MAX=_max;
	if(value < MIN) SetPos(MIN);
	else if( value > MAX ) SetPos(MAX);
}

int CEScroll::getInEditWin(void)
{
	const int tb_str_lenght=20;
	char txtBuf[tb_str_lenght];
	int cntChar=editWin.GetLine(0,txtBuf,tb_str_lenght-1);
	int result=0;
	if(cntChar!=0){ //Если что-то введено то
		txtBuf[cntChar]='\x0';
		result=atoi(txtBuf);
	}
	return result;
};

void CEScroll::putToEditWin(int value)
{
	const int tb_str_lenght=20;
	char txtBuf[tb_str_lenght];
	itoa(value, txtBuf, 10);
	if(::IsWindow(editWin.GetSafeHwnd())){
		editWin.SetWindowText(txtBuf);
		editWin.UpdateWindow();//Для немедленного апдейта окна
	}
};

void CEScroll::EditWin_EN_CHANGE(void)
{
	interSetValue(getInEditWin());
}
void CEScroll::EditWin_EN_KILLFOCUS(void)
{
	putToEditWin(GetPos());
}

void CEScroll::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	SetCapture();
	interSetPos(point.x);
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CEScroll::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if(this==GetCapture()){
		interSetPos(point.x);
		::ReleaseCapture();
	}
	CWnd::OnLButtonUp(nFlags, point);
}

void CEScroll::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if(this==GetCapture()){
		interSetPos(point.x);
	}
	CWnd::OnMouseMove(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// CEScroll2
CEScroll2::CEScroll2()
{
	buttonIsLess.init(this);
	buttonIsBigger.init(this);
	stepButton=1;
}

bool CEScroll2::Create(CWnd* parent, int nIDScroller, int nIDEdit, int nIDButtonIsLess, int nIDButtonIsBigger, int _stepButton)
{
	CEScroll::Create(parent, nIDScroller, nIDEdit);

	CWnd* p;
	p=parent->GetDlgItem(nIDButtonIsLess);
	if(p==NULL)return false;
	p->GetWindowRect(&rectButtonIsLess);
	parent->ScreenToClient(&rectButtonIsLess);
	p->DestroyWindow();

	buttonIsLess.Create("<", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, rectButtonIsLess, parent, nIDButtonIsLess);
	
	p=parent->GetDlgItem(nIDButtonIsBigger);
	if(p==NULL)return false;
	p->GetWindowRect(&rectButtonIsBigger);
	parent->ScreenToClient(&rectButtonIsBigger);
	p->DestroyWindow();
	buttonIsBigger.Create(">", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, rectButtonIsBigger, parent, nIDButtonIsBigger);

	stepButton=_stepButton;
	return true;
}

void CEScroll2::ButtonWin_BN_CLICKED(int nIDButton)
{
	if(nIDButton==buttonIsLess.GetDlgCtrlID()){
		SetPos(GetPos()-stepButton);
	}
	else if(nIDButton==buttonIsBigger.GetDlgCtrlID()){
		SetPos(GetPos()+stepButton);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CEdit4CEScroll

CEdit4CEScroll::CEdit4CEScroll()
{
}

CEdit4CEScroll::~CEdit4CEScroll()
{
}
void CEdit4CEScroll::init(CEScroll * _ownerClass)
{
	ownerClass=_ownerClass;
}


BEGIN_MESSAGE_MAP(CEdit4CEScroll, CEdit)
	//{{AFX_MSG_MAP(CEdit4CEScroll)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEdit4CEScroll message handlers

void CEdit4CEScroll::OnChange() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CEdit::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	ownerClass->EditWin_EN_CHANGE();
}

void CEdit4CEScroll::OnKillfocus() 
{
	// TODO: Add your control notification handler code here
	ownerClass->EditWin_EN_KILLFOCUS();
}



////////////////////////////////////////////////////////////////////////////////////////////////
///unsigned short convert_vid2vox(const char*){return 0;}
///void convert_vox2vid(unsigned short, char*){}
int CEScrollVx::getInEditWin(void)
{
	const int tb_str_lenght=20;
	char txtBuf[tb_str_lenght];
	int cntChar=editWin.GetLine(0,txtBuf,tb_str_lenght-1);
	int result=0;
	if(cntChar!=0){ //Если что-то введено то
		txtBuf[cntChar]='\x0';
		//result=atoi(txtBuf);
		result=convert_vid2vox(txtBuf);
	}
	return result;
};


void CEScrollVx::putToEditWin(int value)
{
	const int tb_str_lenght=20;
	char txtBuf[tb_str_lenght];
	//itoa(value, txtBuf, 10);
	convert_vox2vid(value, txtBuf);
	editWin.SetWindowText(txtBuf);
	editWin.UpdateWindow();//Для немедленного апдейта окна
};

// D:\IVN\Perimeter2\SurMap5\EScroll.cpp : implementation file
//


/////////////////////////////////////////////////////////////////////////////
// CButton4EScroll2

IMPLEMENT_DYNAMIC(CButton4EScroll2, CButton)
CButton4EScroll2::CButton4EScroll2()
{
}

CButton4EScroll2::~CButton4EScroll2()
{
}

void CButton4EScroll2::init(CEScroll2 * _ownerClass)
{
	ownerClass=_ownerClass;
}

BEGIN_MESSAGE_MAP(CButton4EScroll2, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
END_MESSAGE_MAP()



// CButton4EScroll2 message handlers


void CButton4EScroll2::OnBnClicked()
{
	// TODO: Add your control notification handler code here
	ownerClass->ButtonWin_BN_CLICKED(GetDlgCtrlID());
}


