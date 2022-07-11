// MoveEmblem.cpp : implementation file
//
#include "stdafx.h"
#include "MoveEmblem.h"
#include "../Render/src/FileImage.h"
#include "XFolder/XFolderDialog.h"

char* GetFileName(const char *FullName)
{
	static char fname[255];
	fname[0]=0;
	if(FullName==0||FullName[0]==0) return "";
	int l=strlen(FullName)-1;
	while(l>=0&&FullName[l]!='\\') 
		l--;
	strcpy(fname,&FullName[l+1]);
	strupr(fname);
	return fname;
}

string getStringFromReg(const string& folderName, const string& keyName) {
	string res;
	HKEY hKey;
	const int maxLen = 64;
	char name[maxLen];
	DWORD nameLen = maxLen;
	LONG lRet;

	lRet = RegOpenKeyEx( HKEY_CURRENT_USER, folderName.c_str(), 0, KEY_QUERY_VALUE, &hKey );

	if ( lRet == ERROR_SUCCESS ) {
		lRet = RegQueryValueEx( hKey, keyName.c_str(), NULL, NULL, (LPBYTE) name, &nameLen );

		if ( (lRet == ERROR_SUCCESS) && nameLen && (nameLen <= maxLen) ) {
			res = name;
		}

		RegCloseKey( hKey );
	}
	return res;
}
void putStringToReg(const string& folderName, const string& keyName, const string& value) {
	HKEY hKey;
	DWORD dwDisposition;
	LONG lRet;

	lRet = RegCreateKeyEx( HKEY_CURRENT_USER, folderName.c_str(), 0, "", 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition );

	if ( lRet == ERROR_SUCCESS ) {
		lRet = RegSetValueEx( hKey, keyName.c_str(), 0, REG_SZ, (LPBYTE) (value.c_str()), value.length() );

		RegCloseKey( hKey );
	}
}

// cMoveEmblem dialog

IMPLEMENT_DYNAMIC(cMoveEmblem, CDialog)
cMoveEmblem::cMoveEmblem(CWnd* pParent /*=NULL*/)
	: CDialog(cMoveEmblem::IDD, pParent)
	, m_EnableLogo(FALSE)
	, m_PosX(_T(""))
	, m_PosY(_T(""))
	, m_Angle(_T(""))
	, m_TexturePath(_T(""))
{
}

cMoveEmblem::~cMoveEmblem()
{
}

void cMoveEmblem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE, m_DrawTexture);
	DDX_Control(pDX, IDC_MATERIALS_LIST, m_MaterialsList);
	DDX_Control(pDX, IDC_SCROLLBAR1, m_hScroll);
	DDX_Control(pDX, IDC_SCROLLBAR2, m_vScroll);
	DDX_Text(pDX, IDC_EDIT1, m_Width);
	DDX_Text(pDX, IDC_EDIT2, m_Height);
	DDX_Check(pDX, IDC_USE_LOGO, m_EnableLogo);
	DDX_Text(pDX, IDC_EDIT5, m_Angle);
	DDX_Text(pDX, IDC_EDIT_TEXTURE_PATH, m_TexturePath);
}


BEGIN_MESSAGE_MAP(cMoveEmblem, CDialog)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
//	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickMaterialsList)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_MATERIALS_LIST, OnLvnItemchangedMaterialsList)
ON_BN_CLICKED(IDOK, OnOk)
ON_BN_CLICKED(IDC_ZOOMIN, OnBnClickedZoomin)
ON_BN_CLICKED(IDC_ZOOMOUT, OnBnClickedZoomout)
ON_WM_HSCROLL()
ON_WM_VSCROLL()
ON_BN_CLICKED(IDC_ONEONE, OnBnClickedOneone)
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_BN_CLICKED(IDBOK, OnBnClickedBok)
ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
ON_EN_CHANGE(IDC_EDIT2, OnEnChangeEdit2)
ON_BN_CLICKED(IDC_COLOR, OnBnClickedColor)
ON_WM_MOUSEWHEEL()
ON_BN_CLICKED(IDC_USE_LOGO, OnBnClickedUseLogo)
ON_EN_CHANGE(IDC_EDIT5, OnEnChangeEdit5)
ON_BN_CLICKED(IDC_BUTTON_SELECT_TEXTURE_PATH, OnBnClickedButtonSelectTexturePath)
ON_BN_CLICKED(IDC_BUTTON_COPY, OnBnClickedButtonCopy)
ON_BN_CLICKED(IDC_BUTTON_PASTE, OnBnClickedButtonPaste)
END_MESSAGE_MAP()


// cMoveEmblem message handlers

IMPLEMENT_DYNAMIC(cMyPicture, CStatic)
cMyPicture::cMyPicture()
{
	parentDlg = 0;
}

cMyPicture::~cMyPicture()
{
}

BEGIN_MESSAGE_MAP(cMyPicture, CStatic)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

//:((((
float x_pos = 0;
float y_pos = 0;
float height_pos = 0;
float width_pos = 0;
int current_logo = -1;
bool mouse_moving = false;
bool Rmouse_moving = false;
int xx;
int yy;
int hpos = 0;
int vpos = 0;
float zoom=1;
float angle=0;

COLORREF rectColor = RGB(255,0,0);

CPoint old_point;


CBitmap BitMap;
int sizex,sizey;

int cMoveEmblem::GetCurrentMaterial(const char* fname)
{
	//for (int i = 0; i<logos.size(); i++)
	//	if (strcmp(logos[i].TextureName.c_str(),GetFileName(fname)) == 0) return i;
	//
	//sLogo logo;

	//logo.rect.min.set(0,0);
	//logo.rect.max.set(0,0);
	//logo.TextureName = GetFileName(fname);

	//logos.push_back(logo);

	//return logos.size()-1;
	return 0;
}

void cMyPicture::OnPaint()
{
	CPaintDC dc(this);
	if(current_logo<0)
		return;

	//BITMAP pbmp;

   // if (BitMap.GetBitmap(&pbmp))
	{
	CDC dcMemory;
	
    dcMemory.CreateCompatibleDC(&dc);
	dcMemory.SelectObject(&BitMap);
	RECT rect;
	GetClientRect(&rect);
	dc.SetStretchBltMode(STRETCH_DELETESCANS);
	dc.StretchBlt(0,0,rect.right,rect.bottom,&dcMemory,hpos,vpos,rect.right*zoom,rect.bottom*zoom,SRCCOPY);

	if(x_pos<0) x_pos = 0;
	if(y_pos<0) y_pos = 0;
	if (x_pos<width_pos)
	{
		rect.left = x_pos;
		rect.right = width_pos;
	}
	else
	{
		rect.left = width_pos;
		rect.right = x_pos;
	}

	if (y_pos<height_pos)
	{
		rect.top = y_pos;
		rect.bottom = height_pos;
	}
	else
	{
		rect.top = height_pos;
		rect.bottom = y_pos;
	}


	if (current_logo>-1)
	{
		sRectangle4f r;
		r.min.x=rect.left/float(sizex);
		r.min.y=rect.top/float(sizey);
		r.max.x=rect.right/float(sizex);
		r.max.y=rect.bottom/float(sizey);
		((cMoveEmblem*)GetParent())->logos_[current_logo].rect = r;
		((cMoveEmblem*)GetParent())->logos_[current_logo].angle = angle;
	}

		CPen pen;
		CBrush brush;
		pen.CreatePen(PS_SOLID, 1, rectColor);
		brush.CreateSolidBrush(RGB(0,255,0));
		dc.SelectObject(&pen);
		dc.SelectObject(&brush);
		//dc.Rectangle(&rect);
		//RECT selRect;
		//float left = (-hpos+rect.left)/zoom;
		//float right = (-hpos+rect.right)/zoom;
		//float top = (-vpos+rect.top)/zoom;
		//float bottom = (-vpos+rect.bottom)/zoom;
		float inv_zoom = 1.f/zoom;
		float left = rect.left;
		float right = rect.right;
		float top = rect.top;
		float bottom = rect.bottom;

		GetClientRect(&rect);

		float posx = left+(right-left)/2;
		float posy = top+(bottom-top)/2;
		float w = (right-left);
		float h = (bottom-top);
		float w2 = w/2;
		float h2 = h/2;
		float w4 = w/4;
		float h4 = h/4;

		float cosa = cos(angle);
		float sina = sin(angle);

		float x1 = posx-w2*cosa-h2*sina;
		float y1 = posy+w2*sina-h2*cosa;

		float x2 = posx+w2*cosa-h2*sina;
		float y2 = posy-w2*sina-h2*cosa;

		float x3 = posx+w2*cosa+h2*sina;
		float y3 = posy-w2*sina+h2*cosa;

		float x4 = posx-w2*cosa+h2*sina;
		float y4 = posy+w2*sina+h2*cosa;

		float x11 = posx-0-h4*sina;
		float y11 = posy+0-h4*cosa;

		float x22 = posx-w4*cosa;
		float y22 = posy+w4*sina;

		float x33 = posx+w4*cosa;
		float y33 = posy-w4*sina;

		float x44 = posx+h4*sina;
		float y44 = posy+h4*cosa;

		CRgn rgn;
		rgn.CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
		dc.SelectClipRgn(&rgn);
		
		if(((cMoveEmblem*)GetParent())->m_EnableLogo)
		{
			dc.MoveTo((x1-hpos)*inv_zoom,(y1-vpos)*inv_zoom);
			dc.LineTo((x2-hpos)*inv_zoom,(y2-vpos)*inv_zoom);
			dc.LineTo((x3-hpos)*inv_zoom,(y3-vpos)*inv_zoom);
			dc.LineTo((x4-hpos)*inv_zoom,(y4-vpos)*inv_zoom);
			dc.LineTo((x1-hpos)*inv_zoom,(y1-vpos)*inv_zoom);

			dc.MoveTo((x11-hpos)*inv_zoom,(y11-vpos)*inv_zoom);
			dc.LineTo((x22-hpos)*inv_zoom,(y22-vpos)*inv_zoom);
			dc.LineTo((x33-hpos)*inv_zoom,(y33-vpos)*inv_zoom);
			dc.LineTo((x11-hpos)*inv_zoom,(y11-vpos)*inv_zoom);
			dc.LineTo((x44-hpos)*inv_zoom,(y44-vpos)*inv_zoom);
			//dc.MoveTo(left+(right-left)/2,top+(bottom-top)/4);
			//dc.LineTo(left+(right-left)/4,top+(bottom-top)/2);
			//dc.LineTo(right-(right-left)/4,top+(bottom-top)/2);
			//dc.LineTo(left+(right-left)/2,top+(bottom-top)/4);
			//dc.LineTo(left+(right-left)/2,bottom-(bottom-top)/4);
		}
		

		//dc.DrawFocusRect(&rect);
		//GetClientRect(&rect);
		//dc.MoveTo(rect.left,rect.top);
		//dc.LineTo(rect.right,rect.top);
		//dc.LineTo(rect.right,rect.bottom);
		//dc.LineTo(rect.left,rect.bottom);
		//dc.LineTo(rect.left,rect.top);
	}
}

BOOL cMoveEmblem::OnInitDialog()
{
	CDialog::OnInitDialog();
	current_logo = -1;
	//logos = pRootExport->logos;
	//SetWindowPos(0,0,0,250,592,SWP_SHOWWINDOW);
	m_MaterialsList.DeleteAllItems();
	m_TexturePath = getStringFromReg("Software\\3DXExporter","TexturePath").c_str();
	int ItemsCount = pRootExport->GetMaterialNum();
	vector<string> unkNames;
	for(int i=0;i<ItemsCount;i++)
	{
		
		IVisMaterial* material=pRootExport->GetMaterial(i);
		int  num_maps=material->GetTexmapCount();
		if(num_maps==0)
			continue;
		const char* fname = material->GetTexmap(0)->GetBitmapFileName(); 
		bool found = false;
		for(int j=0; j<unkNames.size(); j++)
		{
			if(unkNames[j] == GetFileName(fname))
			{
				found = true;
				break;
			}
		}
		if(found)
			continue;
		unkNames.push_back(GetFileName(fname));
		int n = m_MaterialsList.InsertItem(i,GetFileName(fname));
		m_MaterialsList.SetItemData(n,i);


		sLogo logo(GetFileName(fname));
		for(int j=0; j<pRootExport->logos.size(); j++)
		{
			if(logo.TextureName == pRootExport->logos[j].TextureName)
			{
				logo.enabled = true;
				logo.rect = pRootExport->logos[j].rect;
				logo.angle = pRootExport->logos[j].angle;
			}
		}
		logos_.push_back(logo);
	}
	hpos = vpos = 0;
	zoom = 1;
	m_DrawTexture.parentDlg = this;
	RECT rect;
	m_DrawTexture.GetWindowRect(&rect);
	ScreenToClient(&rect);
	//rect.left = 0;
	//rect.top = 0;
	rect.right = rect.left + 514;
	rect.bottom = rect.top + 514;
	m_DrawTexture.MoveWindow(&rect);
	long bottom = rect.bottom;
	long right = rect.right;
	m_hScroll.GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.top = bottom;
	rect.bottom = rect.top + 16;
	rect.right = rect.left + 514;
	m_hScroll.MoveWindow(&rect);
	m_vScroll.GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.bottom = rect.top + 514;
	rect.left = right;
	rect.right = rect.left + 16;
	m_vScroll.MoveWindow(&rect);
	return TRUE;
}
 
void cMyPicture::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(current_logo<0)
		return;
	CStatic::OnLButtonDown(nFlags, point);
	ScreenToClient(&point);
	x_pos = point.x*zoom+hpos;
	y_pos = point.y*zoom+vpos;
	mouse_moving = true;
	//SetCapture();
}

void cMoveEmblem::OnMouseMove(UINT nFlags, CPoint point)
{
	CDialog::OnMouseMove(nFlags, point);
	ClientToScreen(&point);
	m_DrawTexture.OnMouseMove(nFlags,point);
}

void cMyPicture::OnMouseMove(UINT nFlags, CPoint point)
{
	if(current_logo<0)
		return;
	CStatic::OnMouseMove(nFlags, point);
	ScreenToClient(&point);
	if (mouse_moving)
	{
		width_pos = point.x*zoom+hpos;
		height_pos = point.y*zoom+vpos;
		if (x_pos>sizex) x_pos = sizex;
		if (y_pos>sizex) y_pos = sizey;
		if (width_pos>sizex) width_pos = sizex;
		if (height_pos>sizey) height_pos = sizey;
		Invalidate();
	}
	if(Rmouse_moving)
	{
		CPoint newpoint = point-old_point;
		x_pos += (float)newpoint.x*zoom;
		y_pos += (float)newpoint.y*zoom;
		width_pos += (float)newpoint.x*zoom;
		height_pos += (float)newpoint.y*zoom;
		old_point = point;
		Invalidate();
	}
	
	parentDlg->m_Width.Format("%d",int(width_pos-x_pos));
	parentDlg->m_Height.Format("%d",int(height_pos-y_pos));
	parentDlg->UpdateData(FALSE);
}

void cMyPicture::OnLButtonUp(UINT nFlags, CPoint point)
{
	CStatic::OnLButtonUp(nFlags, point);
	mouse_moving = false;
	//ReleaseCapture();
}

void cMoveEmblem::OnLButtonDown(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonDown(nFlags, point);
	ClientToScreen(&point);
	m_DrawTexture.OnLButtonDown(nFlags,point);
}

void cMoveEmblem::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonUp(nFlags, point);
	m_DrawTexture.OnLButtonUp(nFlags,point);
}

void cMoveEmblem::OnLvnItemchangedMaterialsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int ItemIndex = pNMLV->iItem;
	//SetWindowPos(0,0,0,260,592,SWP_SHOWWINDOW);
	//current_logo = -1;
	if (ItemIndex>-1 && pNMLV->uNewState == 3)
	{
		int n = m_MaterialsList.GetItemData(ItemIndex);	
		IVisMaterial* material=pRootExport->GetMaterial(n);
			int  num_maps=material->GetTexmapCount();
			if(num_maps==0)
				return;
			const char* fname = GetFileName(material->GetTexmap(0)->GetBitmapFileName()); 

			current_logo = ItemIndex;//GetCurrentMaterial(fname);

			m_EnableLogo = logos_[current_logo].enabled;
			UpdateData(FALSE);

			cFileImage* FileImage;

			string file_name = (LPCSTR)m_TexturePath;
			file_name += "\\";
			file_name += fname;
			char fN[1024];
			strcpy(fN,fname);
			strupr(fN);

			FileImage = cFileImage::Create(file_name.c_str());
			BitMap.DeleteObject();
			if(!FileImage)
			{
				Msg("Unknown image type %s\n",file_name.c_str());
				ShowConsole(NULL);
				return;
			}

			if(FileImage->load(file_name.c_str()))
			{
				Msg("Cannot load %s\n",file_name.c_str());
				ShowConsole(NULL);
				return;
			}
			sizex = FileImage->GetX();
			sizey = FileImage->GetY();
			//sizex=min(sizex,512);
			//sizey=min(sizey,512);

			RECT rect;
			m_DrawTexture.GetClientRect(&rect);
			xx = sizex - rect.right;
			yy = sizey - rect.bottom;

			m_hScroll.SetScrollRange(0,xx);
			m_vScroll.SetScrollRange(0,yy);

			angle = logos_[current_logo].angle;
			
			x_pos = logos_[current_logo].rect.min.x*sizex;
			y_pos = logos_[current_logo].rect.min.y*sizey;
			width_pos = logos_[current_logo].rect.max.x*sizex;
			height_pos = logos_[current_logo].rect.max.y*sizey;

			m_Width.Format("%d",int(width_pos-x_pos));
			m_Height.Format("%d",int(height_pos-y_pos));
			m_Angle.Format("%d",(int)R2G(-angle));

			void *ImageData = new char[sizex*sizey*4];
			memset(ImageData,0xFF,sizex*sizey*4);
			FileImage->GetTexture(ImageData,0,sizex, sizey );
			delete FileImage;

		{

			BitMap.CreateBitmap(sizex,sizey,1,32,ImageData);
			delete ImageData;
			RECT rect;
			GetClientRect(&rect);
			//if (sizey>512)
			//	SetWindowPos(0,rect.left,rect.top,260+sizex+40,sizey+80,SWP_SHOWWINDOW);
			//else
			//	SetWindowPos(0,rect.left,rect.top,260+sizex+40,592,SWP_SHOWWINDOW);
			//m_DrawTexture.SetWindowPos(0,260,0,sizex,sizey,SWP_SHOWWINDOW);
		}
	}
	UpdateData(FALSE);
	Invalidate();
	*pResult = 0;
}

void cMoveEmblem::OnOk()
{
	//OnOK();
}
void cMoveEmblem::OnBnClickedBok()
{
	pRootExport->logos.clear();
	for(int i=0; i<logos_.size(); i++)
	{
		if(logos_[i].enabled)
		{
			sLogo logo;
			logo.rect = logos_[i].rect;
			logo.TextureName = logos_[i].TextureName;
			logo.angle = logos_[i].angle;
			pRootExport->logos.push_back(logo);
		}
	}
	CDialog::OnOK();
}

void cMoveEmblem::OnBnClickedZoomin()
{
	zoom -= 0.1f;
	if(zoom < 0.1f)
		zoom = 0.1f;
	RECT rect;
	m_DrawTexture.GetClientRect(&rect);
	xx = sizex - rect.right*zoom;
	yy = sizey - rect.bottom*zoom;

	m_hScroll.SetScrollRange(0,xx);
	m_vScroll.SetScrollRange(0,yy);
	Invalidate();
}

void cMoveEmblem::OnBnClickedZoomout()
{
	zoom += 0.1f;
	RECT rect;
	m_DrawTexture.GetClientRect(&rect);
	if(sizex - int(rect.right*zoom) < 0 || 
	   sizey - int(rect.bottom*zoom) < 0)
	   zoom -= 0.1f;
	xx = sizex - rect.right*zoom;
	yy = sizey - rect.bottom*zoom;

	m_hScroll.SetScrollRange(0,xx);
	m_vScroll.SetScrollRange(0,yy);
	hpos = m_hScroll.GetScrollPos();
	if(hpos <0) hpos =0;
	if(hpos>xx) hpos = xx;
	m_hScroll.SetScrollPos(hpos);
	vpos = m_vScroll.GetScrollPos();
	if(vpos <0) vpos =0;
	if(vpos>yy) vpos = yy;
	m_vScroll.SetScrollPos(vpos);
	Invalidate(false);
}

void cMoveEmblem::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(pScrollBar->GetDlgCtrlID() == m_hScroll.GetDlgCtrlID())
	{
		hpos = m_hScroll.GetScrollPos();
		if(nSBCode == SB_LINELEFT)
		{
			hpos--;
		}else
		if(nSBCode == SB_LINERIGHT)
		{
			hpos++;
		}else
		if(nSBCode == SB_THUMBPOSITION)
		{
			hpos = nPos;
		}else
		if(nSBCode == SB_PAGELEFT)
		{
			hpos-=10;
		}else
		if(nSBCode == SB_PAGERIGHT)
		{
			hpos+=10;
		}else
		if(nSBCode == SB_THUMBTRACK)
		{
			hpos = nPos;
		}
		if(hpos <0) hpos =0;
		if(hpos>xx) hpos = xx;
		m_hScroll.SetScrollPos(hpos);
		Invalidate(false);
	}
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void cMoveEmblem::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(pScrollBar->GetDlgCtrlID() == m_vScroll.GetDlgCtrlID())
	{
		vpos = m_vScroll.GetScrollPos();
		if(nSBCode == SB_LINELEFT)
		{
			vpos--;
		}else
		if(nSBCode == SB_LINERIGHT)
		{
			vpos++;
		}else
		if(nSBCode == SB_THUMBPOSITION)
		{
			vpos = nPos;
		}else
		if(nSBCode == SB_PAGELEFT)
		{
			vpos-=10;
		}else
		if(nSBCode == SB_PAGERIGHT)
		{
			vpos+=10;
		}else
		if(nSBCode == SB_THUMBTRACK)
		{
			vpos = nPos;
		}
		if(vpos <0) vpos =0;
		if(vpos>yy) vpos = yy;
		m_vScroll.SetScrollPos(vpos);
		Invalidate(false);
	}
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void cMoveEmblem::OnBnClickedOneone()
{
	zoom = 1.f;
	RECT rect;
	m_DrawTexture.GetClientRect(&rect);
	xx = sizex - rect.right*zoom;
	yy = sizey - rect.bottom*zoom;

	m_hScroll.SetScrollRange(0,xx);
	m_vScroll.SetScrollRange(0,yy);
	Invalidate(false);
}

void cMyPicture::OnRButtonDown(UINT nFlags, CPoint point)
{
	ScreenToClient(&point);
	old_point = point;
	Rmouse_moving = true;
	CStatic::OnRButtonDown(nFlags, point);
}

void cMyPicture::OnRButtonUp(UINT nFlags, CPoint point)
{
	ScreenToClient(&point);
	CPoint newpoint = point-old_point;
	x_pos += newpoint.x*zoom;
	y_pos += newpoint.y*zoom;
	width_pos += newpoint.x*zoom;
	height_pos += newpoint.y*zoom;
	Rmouse_moving = false;
	Invalidate();
	CStatic::OnRButtonUp(nFlags, point);
}

void cMoveEmblem::OnRButtonDown(UINT nFlags, CPoint point)
{
	CDialog::OnRButtonDown(nFlags, point);
	ClientToScreen(&point);
	m_DrawTexture.OnRButtonDown(nFlags,point);
}

void cMoveEmblem::OnRButtonUp(UINT nFlags, CPoint point)
{
	CDialog::OnRButtonUp(nFlags, point);
	ClientToScreen(&point);
	m_DrawTexture.OnRButtonUp(nFlags,point);
}


void cMoveEmblem::OnEnChangeEdit1()
{
	UpdateData();
	width_pos = x_pos + atoi(m_Width);
	m_DrawTexture.Invalidate();
}

void cMoveEmblem::OnEnChangeEdit2()
{
	UpdateData();
	height_pos = y_pos + atoi(m_Height);
	m_DrawTexture.Invalidate();
}

void cMoveEmblem::OnBnClickedColor()
{
	CColorDialog dlg(rectColor,CC_FULLOPEN);
	if (dlg.DoModal()==IDOK)
	{
		rectColor = dlg.GetColor();
	}
	m_DrawTexture.Invalidate();
}

BOOL cMoveEmblem::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if(zDelta<0)
		OnBnClickedZoomin();
	else
		OnBnClickedZoomout();
	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}

BOOL cMyPicture::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}

void cMoveEmblem::OnBnClickedUseLogo()
{
	UpdateData();
	if(current_logo>-1)
	logos_[current_logo].enabled = m_EnableLogo;
	m_DrawTexture.Invalidate();
}

void cMoveEmblem::OnEnChangeEdit5()
{
	UpdateData();
	angle = G2R(-atoi(m_Angle));
	m_DrawTexture.Invalidate();
}

void cMoveEmblem::OnBnClickedButtonSelectTexturePath()
{
	UpdateData();
	CXFolderDialog dlg(m_TexturePath);
	if(dlg.DoModal()!=IDOK)
		return;
	m_TexturePath = dlg.GetPath();
	string path = (LPCSTR)m_TexturePath;
	putStringToReg("Software\\3DXExporter","TexturePath",path);
	UpdateData(FALSE);
}

void cMoveEmblem::OnBnClickedButtonCopy()
{
	::OpenClipboard(NULL);
	::EmptyClipboard();
	CString strData;
	strData.Format("%f %f %f %f %f",x_pos,y_pos,width_pos,height_pos,angle);
	HGLOBAL hClipboardData;
	hClipboardData = GlobalAlloc(GMEM_DDESHARE, 
		strData.GetLength()+1);
	char * pchData;
	pchData = (char*)GlobalLock(hClipboardData);
	strcpy(pchData, LPCSTR(strData));

	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT,hClipboardData);	

	CloseClipboard();
}

void cMoveEmblem::OnBnClickedButtonPaste()
{
	// Test to see if we can open the clipboard first.
	if (OpenClipboard()) 
	{
		// Retrieve the Clipboard data (specifying that 
		// we want ANSI text (via the CF_TEXT value).
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);

		// Call GlobalLock so that to retrieve a pointer
		// to the data associated with the handle returned
		// from GetClipboardData.
		char *pchData = (char*)GlobalLock(hClipboardData);

		// Set a local CString variable to the data
		// and then update the dialog with the Clipboard data
		CString strFromClipboard = pchData;
		//m_edtFromClipboard.SetWindowText(strFromClipboard);

		// Unlock the global memory.
		GlobalUnlock(hClipboardData);
		int curPos= 0;

		CString resToken= strFromClipboard.Tokenize(" ",curPos);
		x_pos = atof(resToken);
		resToken= strFromClipboard.Tokenize(" ",curPos);
		y_pos = atof(resToken);
		resToken= strFromClipboard.Tokenize(" ",curPos);
		width_pos = atof(resToken);
		resToken= strFromClipboard.Tokenize(" ",curPos);
		height_pos = atof(resToken);
		resToken= strFromClipboard.Tokenize(" ",curPos);
		angle = atof(resToken);
		m_Width.Format("%d",int(width_pos-x_pos));
		m_Height.Format("%d",int(height_pos-y_pos));
		m_Angle.Format("%d",(int)R2G(-angle));
		UpdateData(FALSE);
		// Finally, when finished I simply close the Clipboard
		// which has the effect of unlocking it so that other
		// applications can examine or modify its contents.
		CloseClipboard();
	}

	Invalidate();
}
