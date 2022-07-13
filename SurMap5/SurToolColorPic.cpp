#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolColorPic.h"

#include "..\Units\ExternalShow.h"

const int MIN_FILTER_H=0;
const int MAX_FILTER_H=256;
int CSurToolColorPic::filterMinHValue=0;
int CSurToolColorPic::filterMaxHValue=256;
bool CSurToolColorPic::flag_EnableFilterH=false;

IMPLEMENT_DYNAMIC(CSurToolColorPic, CSurToolBase)
CSurToolColorPic::CSurToolColorPic(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	popUpMenuRestriction=PUMR_PermissionDelete;
	pBitmap=0;
	m_CenterAlpha.SetRange(0,255);
	state_RButton_DrawErase=0;
	previewTexture_ = 0;
	m_FilterMinH.SetRange(MIN_FILTER_H, MAX_FILTER_H);
	m_FilterMaxH.SetRange(MIN_FILTER_H, MAX_FILTER_H);
	m_FilterMinH.value=filterMinHValue;
	m_FilterMaxH.value=filterMaxHValue;
}

CSurToolColorPic::~CSurToolColorPic()
{
}

void CSurToolColorPic::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolColorPic, CSurToolBase)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE, OnBnClickedBtnBrowseBitmap)
	ON_WM_HSCROLL()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RBUT_DRAWERASE1, OnBnClickedRbutDrawerase1)
	ON_BN_CLICKED(IDC_RBUT_DRAWERASE2, OnBnClickedRbutDrawerase2)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_H_FILTER, OnBnClickedCheckEnableHFilter)
END_MESSAGE_MAP()

void CSurToolColorPic::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(dataFileName, "dataFileName", 0);
	ar.serialize(m_CenterAlpha.value, "m_CenterAlpha", 0);
	ar.serialize(state_RButton_DrawErase, "state_RButton_DrawErase", 0);
	//if(ar.isInput())
	//	bitmapIndex = bitmapDispatcher.getIndex(dataFileName.c_str());
}
void CSurToolColorPic::staticSerialize(Archive& ar)
{
	ar.serialize(filterMinHValue, "filterMinHValue", 0);
	ar.serialize(filterMaxHValue, "filterMaxHValue", 0);
	ar.serialize(flag_EnableFilterH, "flag_EnableFilterH", 0);
}


// CSurToolColorPic message handlers


BOOL CSurToolColorPic::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	m_CenterAlpha.Create(this, IDC_SLDR_CENTERALPHA, IDC_EDT_CENTERALPHA);

	CheckRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2, IDC_RBUT_DRAWERASE1+state_RButton_DrawErase);//
	state_RButton_DrawErase=GetCheckedRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2)-IDC_RBUT_DRAWERASE1;//

	m_FilterMinH.Create(this, IDC_SLD_MINH, IDC_EDT_MINH);
	m_FilterMaxH.Create(this, IDC_SLD_MAXH, IDC_EDT_MAXH);
	m_FilterMinH.SetPos(filterMinHValue);
	m_FilterMaxH.SetPos(filterMaxHValue);

	//Инициализация чек-бокса Enable 
	CButton* chB = (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	chB->SetCheck(flag_EnableFilterH);



	///IDC_EDT_BITMAP
	///IDC_BTN_BROWSE_FILE
	ReLoadBitmap();
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CSurToolColorPic::ReLoadBitmap()
{
	pBitmap=0;
	if(!dataFileName.empty()){
		CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAP);
		ew->SetWindowText(dataFileName.c_str());
		const BitmapDispatcher::Bitmap* pbmp = bitmapDispatcher.getBitmap(dataFileName.c_str());
		if(pbmp){
			pBitmap=pbmp->bitmap;
			UpdateTexture();
			return true;
		}
	}
	UpdateTexture();
	return false;
}

void CSurToolColorPic::CallBack_CreateScene(void)
{
	ReLoadBitmap();
}

void CSurToolColorPic::CallBack_ReleaseScene(void)
{
}
//#include "..\terra\terTools.h"
bool CSurToolColorPic::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		int rad = getBrushRadius();
		//flag_EnableFilterH

		int minfh=0, maxfh=256<<VX_FRACTION;
		if(flag_EnableFilterH){
			minfh=m_FilterMinH.value<<VX_FRACTION;
			maxfh=m_FilterMaxH.value<<VX_FRACTION;
		}
		if(state_RButton_DrawErase!=0) //Стерка
			vMap.drawBitmapCircle(x, y, rad, m_CenterAlpha.value, -1, minfh, maxfh); //-1 erase
		else if(pBitmap)
			vMap.drawBitmapCircle(x, y, rad, m_CenterAlpha.value, bitmapDispatcher.getUID(dataFileName.c_str()), minfh, maxfh);

//		terToolsDispatcher.test(x, y, rad);

	}
	return true;
}

bool CSurToolColorPic::CallBack_DrawAuxData()
{
	drawCursorCircle();
	return true;
}

void CSurToolColorPic::UpdateTexture ()
{
	RELEASE (previewTexture_);

	if(const BitmapDispatcher::Bitmap* bitmap = bitmapDispatcher.getBitmap(dataFileName.c_str())) {
		if (bitmap->size.x < 4 || bitmap->size.y < 4)
			return;

		int width = Power2up (bitmap->size.x);
		int height = Power2up (bitmap->size.y);

		bool old_error = GetTexLibrary()->EnableError (false);
		if (width == bitmap->size.x && height == bitmap->size.y) {
			if (previewTexture_ = GetTexLibrary ()->CreateTexture (width, height, false)) {
				int pitch = sizeof(unsigned long) * width;
				BYTE* bits = previewTexture_->LockTexture (pitch);
				memcpy (bits, bitmap->bitmap, sizeof(unsigned long) * width * height);
				
				previewTexture_->UnlockTexture ();
				previewTextureSize_.set (bitmap->size.x, bitmap->size.y);
			}
		} else {
			if (previewTexture_ = GetTexLibrary ()->CreateTexture (width, height, false)) {
				int pitch = sizeof(unsigned long) * width;
				BYTE* bits = previewTexture_->LockTexture (pitch);
				for (int row = 0; row < bitmap->size.y; ++row) {
					memcpy (bits + row * width * sizeof(unsigned long),
							bitmap->bitmap + row * bitmap->size.x, sizeof(unsigned long) * bitmap->size.x);
				}
				previewTexture_->UnlockTexture ();
				previewTextureSize_.set (bitmap->size.x, bitmap->size.y);
			}
		}
		GetTexLibrary()->EnableError (old_error);
	}
}


bool CSurToolColorPic::CallBack_DrawPreview(int width, int height)
{
	if(previewTexture_){
		sRectangle4f rect(0.0f, 0.0f,
						  float(previewTextureSize_.x) / float(previewTexture_->GetWidth()),
						  float(previewTextureSize_.y) / float(previewTexture_->GetHeight()));
		drawPreviewTexture(previewTexture_, width, height, rect);
	}
	return true;
}

void CSurToolColorPic::OnBnClickedBtnBrowseBitmap()
{
	dataFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");

	bool result=ReLoadBitmap();
	if(!result){
		AfxMessageBox(" Error bitmap loading ");
	} else {
		std::string::size_type pos = dataFileName.rfind ("\\");
		std::string::size_type end_pos = dataFileName.rfind (".");
		if (pos != std::string::npos) {
			std::string new_name(dataFileName.begin() + pos + 1, end_pos == std::string::npos ? dataFileName.end() : (dataFileName.begin() + end_pos));
			setName(new_name.c_str());
			updateTreeNode();
		} else {
		}
	}
}

void CSurToolColorPic::OnDestroy()
{
	RELEASE (previewTexture_);
	CSurToolBase::OnDestroy();
}


void CSurToolColorPic::OnBnClickedRbutDrawerase1()
{
	// TODO: Add your control notification handler code here
	state_RButton_DrawErase=GetCheckedRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2)-IDC_RBUT_DRAWERASE1;//
}

void CSurToolColorPic::OnBnClickedRbutDrawerase2()
{
	// TODO: Add your control notification handler code here
	state_RButton_DrawErase=GetCheckedRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2)-IDC_RBUT_DRAWERASE1;//
}

void CSurToolColorPic::OnBnClickedCheckEnableHFilter()
{
	// TODO: Add your control notification handler code here
	CButton* chB= (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	flag_EnableFilterH=chB->GetCheck();
}

void CSurToolColorPic::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ctrlID = pScrollBar->GetDlgCtrlID();

	static char fl_Recursion=0;
	if(fl_Recursion==0){//предотвращает рекурсию
		fl_Recursion=1;
		switch(ctrlID){
		case IDC_SLD_MINH:
			if(m_FilterMinH.value > m_FilterMaxH.value )
				m_FilterMaxH.SetPos(m_FilterMinH.value);
			//Invalidate(FALSE);
			filterMinHValue=m_FilterMinH.value;
			break;
		case IDC_SLD_MAXH:
			if(m_FilterMaxH.value < m_FilterMinH.value )
				m_FilterMinH.SetPos(m_FilterMaxH.value);
			//Invalidate(FALSE);
			filterMaxHValue=m_FilterMaxH.value;
			break;
		}
		fl_Recursion=0;
	}

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}
void CSurToolColorPic::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);
	// TODO: Add your message handler code here
	//if(bShow){
	//	m_FilterMinH.SetPos(filterMinHValue);
	//	m_FilterMaxH.SetPos(filterMaxHValue);
	//}
 }

