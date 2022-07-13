#include "stdafx.h"

#include "SurMap5.h"
#include "SurToolColorPic.h"

#include "Render\Src\TexLibrary.h"
#include "Terra\worldFileDispatcher.h"

const int MIN_FILTER_H=0;
const int MAX_FILTER_H=MAX_VX_HEIGHT;//_WHOLE;
int CSurToolColorPic::filterMinHValue=0;
int CSurToolColorPic::filterMaxHValue=MAX_VX_HEIGHT;//_WHOLE;
bool CSurToolColorPic::flag_EnableFilterH=false;

IMPLEMENT_DYNAMIC(CSurToolColorPic, CSurToolBase)
CSurToolColorPic::CSurToolColorPic(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	popUpMenuRestriction=PUMR_PermissionDelete;
	pBitmap=0;
	m_CenterAlpha.SetRange(0,255);
	m_CenterAlpha.value=0;
	m_KColor.SetRange(0,200);
	m_KColor.value=0;
	m_Saturation.SetRange(0, 200);
	m_Saturation.value=100;
	m_Brightness.SetRange(-0, 200);
	m_Brightness.value=100;

	state_RButton_DrawErase=0;
	previewTexture_ = 0;
	m_FilterMinH.SetRange(MIN_FILTER_H, MAX_FILTER_H);
	m_FilterMaxH.SetRange(MIN_FILTER_H, MAX_FILTER_H);
	m_FilterMinH.value=filterMinHValue;
	m_FilterMaxH.value=filterMaxHValue;
	txColor=Color4c::WHITE;
}

CSurToolColorPic::~CSurToolColorPic()
{
}

void CSurToolColorPic::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_COLOR, btnSelectColor);
}


BEGIN_MESSAGE_MAP(CSurToolColorPic, CSurToolBase)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE, OnBnClickedBtnBrowseBitmap)
	ON_BN_CLICKED(IDC_BTN_COLOR, OnBnClickedBtnSelectColor)
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RBUT_DRAWERASE1, OnBnClickedRbutDrawerase1)
	ON_BN_CLICKED(IDC_RBUT_DRAWERASE2, OnBnClickedRbutDrawerase2)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_H_FILTER, OnBnClickedCheckEnableHFilter)
	ON_BN_CLICKED(IDCBTN_PUT2ALLWORLD, OnBnClicked_Put2World)
END_MESSAGE_MAP()

void CSurToolColorPic::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(dataFileName, "dataFileName", 0);
	ar.serialize(m_CenterAlpha.value, "m_CenterAlpha", 0);
	ar.serialize(m_KColor.value, "m_KColor", 0);
	ar.serialize(m_Saturation.value, "m_Saturation", 0);
	ar.serialize(m_Brightness.value, "m_Brightness", 0);
	ar.serialize(state_RButton_DrawErase, "state_RButton_DrawErase", 0);
	ar.serialize(txColor, "txColor", 0);
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

	m_CenterAlpha.Create(this, IDC_SLDR_CENTERALPHA, IDC_EDT_CENTERALPHA);
	m_KColor.Create(this, IDC_SLD_KCOLOR, IDC_EDT_KCOLOR);
	m_Saturation.Create(this, IDC_SLD_SATURATION, IDC_EDT_SATURATION);
	m_Brightness.Create(this, IDC_SLD_BRIGHTNESS, IDC_EDT_BRIGHTNESS);

	CheckRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2, IDC_RBUT_DRAWERASE1+state_RButton_DrawErase);//
	state_RButton_DrawErase=GetCheckedRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2)-IDC_RBUT_DRAWERASE1;//

	m_FilterMinH.Create(this, IDC_SLD_MINH, IDC_EDT_MINH);
	m_FilterMaxH.Create(this, IDC_SLD_MAXH, IDC_EDT_MAXH);
	m_FilterMinH.SetPos(filterMinHValue);
	m_FilterMaxH.SetPos(filterMaxHValue);

	//»нициализаци€ чек-бокса Enable 
	CButton* chB = (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	chB->SetCheck(flag_EnableFilterH);

	btnSelectColor.SetColor(RGB(txColor.r, txColor.g, txColor.b));

	layout_.init(this);
	layout_.add(1, 1, 1, 0, IDC_SLD_MINH);
	layout_.add(1, 1, 1, 0, IDC_SLD_MAXH);

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

void CSurToolColorPic::onCreateScene(void)
{
	ReLoadBitmap();
}

void CSurToolColorPic::onReleaseScene(void)
{
}

bool CSurToolColorPic::onOperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		int rad = getBrushRadius();
		int minfh=0, maxfh=MAX_VX_HEIGHT;
		if(flag_EnableFilterH){
			minfh=clamp(m_FilterMinH.value, MIN_VX_HEIGHT, MAX_VX_HEIGHT);//<<VX_FRACTION
			maxfh=clamp(m_FilterMaxH.value, MIN_VX_HEIGHT, MAX_VX_HEIGHT);//<<VX_FRACTION
		}
		if(pBitmap)
			vMap.drawBitmapCircle(x, y, rad, m_CenterAlpha.value, bitmapDispatcher.getUID(dataFileName.c_str()), minfh, maxfh, 
				ColorModificator(txColor, (float)m_KColor.value/100.f, (float)m_Saturation.value/100.f, (float)m_Brightness.value/100.f));
	}
	return true;
}

bool CSurToolColorPic::onDrawAuxData()
{
	drawCursorCircle();
	return true;
}

void CSurToolColorPic::UpdateTexture()
{
	RELEASE (previewTexture_);

	if(const BitmapDispatcher::Bitmap* bitmap = bitmapDispatcher.getBitmap(dataFileName.c_str())) {
		if (bitmap->size.x < 4 || bitmap->size.y < 4)
			return;
		ColorModificator cMod(txColor, (float)m_KColor.value/100.f, (float)m_Saturation.value/100.f, (float)m_Brightness.value/100.f);

		int width = Power2up(bitmap->size.x);
		int height = Power2up(bitmap->size.y);

		bool old_error = GetTexLibrary()->EnableError(false);
		//if (width == bitmap->size.x && height == bitmap->size.y) {
		//	if (previewTexture_ = GetTexLibrary()->CreateTexture(width, height, false)) {
		//		int pitch = sizeof(unsigned long) * width;
		//		BYTE* bits = previewTexture_->LockTexture(pitch);
		//		memcpy(bits, bitmap->bitmap, sizeof(unsigned long) * width * height);
		//		previewTexture_->UnlockTexture();
		//		previewTextureSize_.set(bitmap->size.x, bitmap->size.y);
		//	}
		//} else {
		//	if (previewTexture_ = GetTexLibrary()->CreateTexture(width, height, false)) {
		//		int pitch = sizeof(unsigned long) * width;
		//		BYTE* bits = previewTexture_->LockTexture(pitch);
		//		for (int row = 0; row < bitmap->size.y; ++row)
		//			memcpy(bits + row * width * sizeof(unsigned long),
		//					bitmap->bitmap + row * bitmap->size.x, sizeof(unsigned long) * bitmap->size.x);
		//		previewTexture_->UnlockTexture();
		//		previewTextureSize_.set(bitmap->size.x, bitmap->size.y);
		//	}
		//}
		if(previewTexture_ = GetTexLibrary()->CreateTexture(width, height, false)) {
			int pitch = sizeof(unsigned long) * width;
			BYTE* bits = previewTexture_->LockTexture(pitch);
			for(int row = 0; row < bitmap->size.y; ++row){
				memcpy(bits + row * width * sizeof(unsigned long),
						bitmap->bitmap + row * bitmap->size.x, sizeof(unsigned long) * bitmap->size.x);
				for(int x=0; x < bitmap->size.x; ++x){
					*((unsigned long*)(bits + (row*width + x)*sizeof(unsigned long))) = 
						cMod.get( Color4c(*(bitmap->bitmap + (row*bitmap->size.x + x))) ).argb;
				}
			}

			previewTexture_->UnlockTexture();
			previewTextureSize_.set(bitmap->size.x, bitmap->size.y);
		}

		GetTexLibrary()->EnableError(old_error);
	}
}


bool CSurToolColorPic::onDrawPreview(int width, int height)
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

void CSurToolColorPic::OnBnClicked_Put2World()
{
	if(!vMap.isWorldLoaded()) return;
	int result=AfxMessageBox("A you sure? (all world texture loost!)", MB_OKCANCEL);
	if(result==IDCANCEL) return;

	int minfh=0, maxfh=MAX_VX_HEIGHT;
	if(flag_EnableFilterH){
		minfh=clamp(m_FilterMinH.value, MIN_VX_HEIGHT, MAX_VX_HEIGHT);//<<VX_FRACTION
		maxfh=clamp(m_FilterMaxH.value, MIN_VX_HEIGHT, MAX_VX_HEIGHT);//<<VX_FRACTION
	}
	if(pBitmap)
		vMap.putBitmap2AllWorld(bitmapDispatcher.getUID(dataFileName.c_str()), minfh, maxfh, 
			ColorModificator(txColor, (float)m_KColor.value/100.f, (float)m_Saturation.value/100.f, (float)m_Brightness.value/100.f));
}


void CSurToolColorPic::OnBnClickedBtnSelectColor()
{
	CColorDialog dlg(RGB(txColor.r, txColor.g, txColor.b), CC_FULLOPEN|CC_RGBINIT);
	dlg.DoModal();
	txColor.r=GetRValue(dlg.m_cc.rgbResult);
	txColor.g=GetGValue(dlg.m_cc.rgbResult);
	txColor.b=GetBValue(dlg.m_cc.rgbResult);
	btnSelectColor.SetColor(RGB(txColor.r, txColor.g, txColor.b));
	Invalidate(FALSE);
	if(pBitmap)
		UpdateTexture();
}

void CSurToolColorPic::OnDestroy()
{
	RELEASE (previewTexture_);
	CSurToolBase::OnDestroy();
}


void CSurToolColorPic::OnBnClickedRbutDrawerase1()
{
	state_RButton_DrawErase=GetCheckedRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2)-IDC_RBUT_DRAWERASE1;//
}

void CSurToolColorPic::OnBnClickedRbutDrawerase2()
{
	state_RButton_DrawErase=GetCheckedRadioButton(IDC_RBUT_DRAWERASE1, IDC_RBUT_DRAWERASE2)-IDC_RBUT_DRAWERASE1;//
}

void CSurToolColorPic::OnBnClickedCheckEnableHFilter()
{
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
		case IDC_SLD_KCOLOR:
		case IDC_SLD_SATURATION:
		case IDC_SLD_BRIGHTNESS:
			if(pBitmap)
				UpdateTexture();
			break;
		}

		fl_Recursion=0;
	}

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}
void CSurToolColorPic::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);
	//if(bShow){
	//	m_FilterMinH.SetPos(filterMinHValue);
	//	m_FilterMaxH.SetPos(filterMaxHValue);
	//}
 }

void CSurToolColorPic::OnSize(UINT type, int cx, int cy)
{
	layout_.onSize(cx, cy);
	__super::OnSize(type, cx, cy);
}
