// DlgLoadSprite.cpp : implementation file
//

#include <stdio.h>
#include "stdafx.h"
#include "effecttool.h"
#include "DlgLoadSprite.h"
#include ".\dlgloadsprite.h"
#include <dlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static LPCTSTR lpszFilter = "Image files (*.tga,*.bmp,*.avi,*.dds)|*.tga;*.bmp;*.avi;*.dds|All files|*.*||";

HBITMAP CreateBitmapFromTexture(LPCSTR name, bool mula)
{
	cTexture* pTexture = theApp.scene.CreateTexture(name);
	if(!pTexture)
		return 0;

	BITMAPINFO bmi;
	BITMAPINFOHEADER& bmh=bmi.bmiHeader;
	bmh.biSize=sizeof(BITMAPINFOHEADER);
	bmh.biWidth=pTexture->GetWidth();
	bmh.biHeight=-pTexture->GetHeight();
	bmh.biPlanes=1;
	bmh.biBitCount=32;
	bmh.biCompression=BI_RGB;
	bmh.biSizeImage=0;
	bmh.biXPelsPerMeter=100;
	bmh.biYPelsPerMeter=100;
	bmh.biClrUsed=256;
	bmh.biClrImportant=0;

	BYTE *pBits=0;

	HBITMAP hbitmap=CreateDIBSection(GetDC(NULL),(BITMAPINFO*)&bmi,
		DIB_RGB_COLORS,(void**)&pBits,NULL,0);

	int dx=pTexture->GetWidth(),dy=pTexture->GetHeight();
	BYTE *pout=pBits;
	int Pitch;
	BYTE *pin=pTexture->LockTexture(Pitch);
	if(mula)
	{
		const r1=255>>1,g1=230>>1,b1=230>>1;
		const r2=255>>1,g2=179>>1,b2=179>>1;
		for(int y=0;y<dy;y++)
		{
			BYTE* p=pout;
			BYTE* pi=pin;
			for(int x=0;x<dx;x++,p+=4,pi+=4)
			{
				int a=pi[3],a1=255-a;
				int r,g,b;
				if((x+y)&16)
				{
					r=r1;g=g1;b=b1;
				}else
				{
					r=r2;g=g2;b=b2;
				}

				p[0]=(b*a1+pi[0]*a)>>8;
				p[1]=(g*a1+pi[1]*a)>>8;
				p[2]=(r*a1+pi[2]*a)>>8;
				p[3]=a;
			}
			pin+=Pitch;
			pout+=dx*4;
		}
	}else
	{
		for(int y=0;y<dy;y++)
		{
			DWORD* p=(DWORD*)pout;
			DWORD* pi=(DWORD*)pin;
			for(int x=0;x<dx;x++,p++,pi++)
				*p=*pi;
			pin+=Pitch;
			pout+=dx*4;
		}
	}
	pTexture->UnlockTexture();
	pTexture->Release();

	return hbitmap;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgLoadSprite dialog

CDlgLoadSprite::CDlgLoadSprite(CWnd* pParent /*=NULL*/,LPCSTR fileName)
	:CFileDialog(TRUE, NULL, NULL, OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_FILEMUSTEXIST, lpszFilter, pParent)
	, m_FileInfo(_T(""))
{
	m_ofn.hInstance = AfxGetInstanceHandle();
	m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD);
	m_intCurImage = 0;
	m_bPlumeVisible = false;
	m_str[0] = fileName;
	//{{AFX_DATA_INIT(CDlgLoadSprite)
	m_bAlpha = TRUE;
	//}}AFX_DATA_INIT
}
INT_PTR CDlgLoadSprite::DoModal()
{
//	char fn[1024];
//	memcpy(fn,m_str[0],m_str[0].GetLength()+1);
	m_ofn.lpstrTitle = "Загрузить спрайт";
//	m_ofn.lpstrFile = fn;
//	m_ofn.nMaxFile = 512;
	return CFileDialog::DoModal();

}


void CDlgLoadSprite::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgLoadSprite)
	DDX_Control(pDX, IDC_IMAGE, m_Image[0]);
	DDX_Control(pDX, IDC_IMAGE_PLUME, m_Image[1]);
	DDX_Check(pDX, IDC_ALPHA, m_bAlpha);
	DDX_Control(pDX, IDC_STATIC1, m_title[0]);
	DDX_Control(pDX, IDC_STATIC2, m_title[1]);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_INFO, m_FileInfo);
}


BEGIN_MESSAGE_MAP(CDlgLoadSprite, CDialog)
	//{{AFX_MSG_MAP(CDlgLoadSprite)
	ON_BN_CLICKED(IDC_ALPHA, OnAlpha)
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDOWN()
	ON_WM_CHILDACTIVATE()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgLoadSprite message handlers

void CDlgLoadSprite::OnAlpha() 
{
	UpdateData();
	m_Image[0].SetFileImage(m_str[0],m_bAlpha);
	if (m_bPlumeVisible)
	{
		m_Image[1].SetFileImage(m_str[1],m_bAlpha);
	}
}
extern  CString ToStr(int);
void CDlgLoadSprite::SetTitleImage()
{
	for(int i=0;i<2;++i)
	{
		std::string title = i==0 ? "Частица" : "Шлейф";
		int n = m_Image[i].GetFramesCount();
		if (n<m_Image[i].GetRealyFramesCount())
		{
			title+= ": только "+ToStr(n)+" кадр";
			if (n!=1)
				if(n<5)title+="а";
				else		title+="ов";
			title += " из "+ToStr(m_Image[i].GetRealyFramesCount());
		}
		m_title[i].SetWindowText(title.c_str());
	}
	m_FileInfo = m_Image[0].GetInfo();
	UpdateData(FALSE);
}

void CDlgLoadSprite::OnFileNameChange()
{
	CListCtrl* list = (CListCtrl*)GetParent()->GetDlgItem(lst1);
	int n = list->GetItemCount();
	list->EnableWindow(FALSE);
	CString s = GetPathName();
	CString& name = m_str[m_intCurImage];
	OFSTRUCT t;
	if (OpenFile(s,&t,OF_EXIST)==HFILE_ERROR)
		return;
	m_Image[m_intCurImage].SetFileImage(s,m_bAlpha);
	if (m_Image[m_intCurImage].Empty()) 
		s = "";
	SetTitleImage();
	m_str[m_intCurImage] = s;	
	__super::OnFileNameChange();
}
const int big_side = 90;
const int sm_side = 80;

int y_big = 340; //y offset
int xl_big = 30; //x offset

int y_sm = y_big + big_side/2 - sm_side/2;
int xl_sm = xl_big + big_side/2 - sm_side/2;
int xr_big = xl_big + big_side;
int xr_sm = xl_sm + big_side;

void CDlgLoadSprite::OnChildActivate()
{
	CFileDialog::OnChildActivate();
	CRect r1,r2;
//	GetWindowRect(r1);
//	m_Image[0].GetWindowRect(r2);
//	y_big = r2.top-r1.top;
//	xl_big = r2.left - r1.left;
//	m_Image[1].GetWindowRect(r2);
//	xr_big = r2.left - r1.left;
//	
//	y_sm = y_big + big_side/2 - sm_side/2;
//	xl_sm = xl_big + big_side/2 - sm_side/2;
////	xr_big = xl_big + big_side;
//	xr_sm = xr_big + big_side/2 - sm_side/2;
//
//	m_Image[0].MoveWindow(xl_big, y_big, big_side, big_side);
//	m_Image[1].MoveWindow(xr_sm, y_sm, sm_side, sm_side);
//
//	m_Image[0].ShowWindow(SW_SHOW);
//	m_title[0].ShowWindow(SW_SHOW);
//	m_Image[0].SetFileImage(m_str[0],m_bAlpha);
//	if (m_bPlumeVisible)
//	{
//		m_Image[1].ShowWindow(SW_SHOW);
//		m_title[1].ShowWindow(SW_SHOW);
//		m_Image[1].SetFileImage(m_str[1],m_bAlpha);
//	}
	SetTitleImage();
}

void CDlgLoadSprite::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPoint pt;
	CRect rc;
	GetCursorPos(&pt);
	m_Image[0].GetWindowRect(&rc);
	if (rc.PtInRect(pt))
	{
		m_intCurImage = 0;
		m_Image[0].MoveWindow(xl_big, y_big, big_side, big_side);
		m_Image[1].MoveWindow(xr_sm, y_sm, sm_side, sm_side);
		m_Image[0].UpDateBitMap();
		m_Image[1].UpDateBitMap();
	}
	else
	{
		m_Image[1].GetWindowRect(&rc);
		if (m_bPlumeVisible && rc.PtInRect(pt))
		{
			m_intCurImage = 1;
			m_Image[0].MoveWindow(xl_sm, y_sm, sm_side, sm_side);
			m_Image[1].MoveWindow(xr_big, y_big, big_side, big_side);
			m_Image[0].UpDateBitMap();
			m_Image[1].UpDateBitMap();
		}
	}
	CFileDialog::OnLButtonDown(nFlags, point);
}

void CDlgLoadSprite::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint pt;
	CRect rc;
	GetCursorPos(&pt);
	for(int i=0;i<2;++i)
	{
		m_Image[i].GetWindowRect(&rc);
		if (rc.PtInRect(pt))
		{
			m_str[i] = "";
			m_Image[i].SetFileImage("",0);
			break;
		}
	}
	CFileDialog::OnLButtonDblClk(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// CTextureWnd

CTextureWnd::CTextureWnd()
{
	m_hBmp = 0;
	m_CurFrame = -1;
	m_CountFrame = 0;
	FileImage = NULL;
	szx = 0;
	szy = 0;
}

CTextureWnd::~CTextureWnd()
{
	if(m_hBmp)
		DeleteObject(m_hBmp);
}

void CTextureWnd::SetBitmap(HBITMAP hBmp)
{
	if(m_hBmp)
		DeleteObject(m_hBmp);
	m_hBmp = hBmp;
	Invalidate();
}

BEGIN_MESSAGE_MAP(CTextureWnd, CWnd)
	//{{AFX_MSG_MAP(CTextureWnd)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextureWnd message handlers

void CTextureWnd::OnPaint() 
{
	HBITMAP hb = m_hBmp;
	if(!hb)
	{
		static CBitmap bmp;
		static bool need_load = true;
		if (need_load)
		{
			bmp.LoadBitmap(IDB_EMPTY_TEXTURE);
			need_load = false;
		}
		hb = bmp;
		szx = 86;
		szy = 86;
	}
	
	CPaintDC dc(this);
	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	mdc.SelectObject(CBitmap::FromHandle(hb));
	dc.SelectStockObject(DKGRAY_BRUSH);

	CRect rc;
	GetClientRect(rc);
	dc.Rectangle(0,0,rc.Width(),rc.Height());
	dc.BitBlt((rc.Width()-szx)/2, (rc.Height()-szy)/2, rc.Width(), rc.Height(), &mdc, 0, 0, SRCCOPY);
}
CString CTextureWnd::GetInfo()
{
	CString str;
	if (FileImage)
	{
		str.Format("Width: %d\nHeight: %d\nBPP: %d",FileImage->GetX(),FileImage->GetY(),FileImage->GetBitPerPixel());
	}
	return str;
}

int CTextureWnd::SetFileImage(CString fName, bool alpha)
{
	if (FileImage)
		delete FileImage;
	char name[1024];
	fName.MakeUpper();
	strcpy(name,fName);
	FileImage = cFileImage::Create(name);
	m_bAlpha = alpha;
	if (FileImage)
	{
		bool err = FileImage->load(fName)!=0;
		if (!err)
		{
			int dx = FileImage->GetX()+2;
			int dy = FileImage->GetY()+2;
			m_CountFrame = FileImage->GetLength();
			GetDimTexture(dx,dy,m_CountFrame);
			if (m_CountFrame>0)
			{
				m_CurFrame = 0;
				UpDateBitMap();
				SetTimer(WM_TIMER,1000,NULL);
			}
			else 
				err =  true;

		}
		if (err)
		{
			delete FileImage;
			FileImage = NULL;
		}
	}
	if (!FileImage)
	{
		SetBitmap(NULL);
		m_CountFrame = 0;
		m_CurFrame = -1;
	}
	return m_CountFrame;
}

inline int CTextureWnd::GetFramesCount()
{
	return m_CountFrame;
}

int CTextureWnd::GetRealyFramesCount()
{
	if (FileImage==0)
		return 0;
	return FileImage->GetLength();
}
extern int ResourceFileRead(const char *fname,void *&buf,int &size);

void CTextureWnd::UpDateBitMap()
{
	if (FileImage==NULL || (UINT)m_CurFrame >= m_CountFrame)
		return;
	int dx = FileImage->GetX();
	int dy = FileImage->GetY();
	
	CRect rc;
	GetClientRect(&rc);
	if (dx<dy)
	{
		float delta = float(dx)/float(dy);
		dy = rc.Height();
		dx = dy*delta;
	}else
	{
		float delta = float(dy)/float(dx);
		dx = rc.Width();
		dy = dx*delta;
	}
	szx = dx;
	szy = dy;

	BITMAPINFO bmi;
	BITMAPINFOHEADER& bmh=bmi.bmiHeader;
	bmh.biSize=sizeof(BITMAPINFOHEADER);
	bmh.biWidth=dx;
	bmh.biHeight=dy;
	bmh.biPlanes=1;
	bmh.biBitCount=32;
	bmh.biCompression=BI_RGB;
	bmh.biSizeImage=0;
	bmh.biXPelsPerMeter=100;
	bmh.biYPelsPerMeter=100;
	bmh.biClrUsed=256;
	bmh.biClrImportant=0;

	UCHAR *pBits=NULL;

	HBITMAP hbitmap=CreateDIBSection(::GetDC(NULL),(BITMAPINFO*)&bmi,
		DIB_RGB_COLORS,(void**)&pBits,NULL,0);
	xassert(pBits);
	//memset(pBits,255,dx*dy*4);
	//FileImage->GetTexture(pBits,m_CurFrame,4,4*dx,
	//		8,8,8,8, 16,8,0,24, dx, dy );
	FileImage->GetTexture(pBits,m_CurFrame,dx,dy);
	if (FileImage->GetBitPerPixel()==32&&m_bAlpha)
	{
		//FileImage->GetTextureAlpha(pBits,m_CurFrame,4,4*dx,
		//			8, 24, dx, dy );

		const r1=255>>1,g1=230>>1,b1=230>>1;
		const r2=255>>1,g2=179>>1,b2=179>>1;
		UCHAR* p = pBits;
		UCHAR* lm = pBits + dx*dy*4;
		int x = 0;
		int y = 0;
		while(p<lm)
		{
			int a=p[3],a1=255-a;
			int r,g,b;
			if((x+y)&16)
			{
				r=r1;g=g1;b=b1;
			}else
			{
				r=r2;g=g2;b=b2;
			}
			p[0]=(b*a1+p[0]*a)>>8;
			p[1]=(g*a1+p[1]*a)>>8;
			p[2]=(r*a1+p[2]*a)>>8;
			if (++x>=dx)
			{
				++y; x=0;
			}
			p+=4;
		}
	}
	SetBitmap(hbitmap);
	OnPaint();
}

void CTextureWnd::NextFrame()
{
	if (m_CountFrame==1&&m_CurFrame==0)
		return;
	if (++m_CurFrame>=m_CountFrame)
		m_CurFrame = 0;
	UpDateBitMap();
}


void CTextureWnd::OnTimer(UINT nIDEvent)
{
	NextFrame();
}




BOOL CDlgLoadSprite::OnInitDialog()
{
	CFileDialog::OnInitDialog();
	if (!m_str[m_intCurImage].IsEmpty())
		m_Image[m_intCurImage].SetFileImage(m_str[m_intCurImage],m_bAlpha);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgLoadSprite::OnFolderChange()
{
	CFileDialog::OnFolderChange();
	CWnd* p = FindWindowEx(GetParent()->m_hWnd,NULL,"SHELLDLL_DefView",NULL);

	CListCtrl* list = (CListCtrl*)FindWindowEx(p->m_hWnd,NULL,"SysListView32",NULL);
	if (list)
	{
		list->ModifyStyle(0,LVS_SHOWSELALWAYS);
		LVFINDINFO info;
		int nIndex;

		info.flags = LVFI_STRING;
		CString str = m_str[0];
		int i = str.ReverseFind('\\');
		CString ss = str.Right(str.GetLength()-i-1);
		info.psz = ss;

		list->SetFocus();
		// Delete all of the items that begin with the string lpszmyString.
		nIndex=list->FindItem(&info);
		if (nIndex != -1)
		{
			list->EnsureVisible(nIndex,TRUE);
			list->SetItemState(nIndex,LVIS_SELECTED,LVIS_SELECTED);
			list->SetItemState(nIndex,LVIS_FOCUSED,LVIS_FOCUSED);
			//list->SetSelectionMark(nIndex);
		}
		
	}
}

int CTextureWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rc;
	GetClientRect(&rc);
	szx = rc.Width();
	szy = rc.Height();
	return 0;
}
