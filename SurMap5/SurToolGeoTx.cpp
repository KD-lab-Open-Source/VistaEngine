// SurToolGeoTx.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolGeoTx.h"

#include "Serialization\Serialization.h"
#include "surtoolgeotx.h"

// CSurToolGeoTx dialog

IMPLEMENT_DYNAMIC(CSurToolGeoTx, CSurToolBase)
CSurToolGeoTx::CSurToolGeoTx(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	flag_changedGeoViewOption=false;
}

CSurToolGeoTx::~CSurToolGeoTx()
{
}

void CSurToolGeoTx::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(dataFileName, "dataFileName", 0);
	ar.serialize(dataFileName2, "dataFileName2", 0);
}

void CSurToolGeoTx::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolGeoTx, CSurToolBase)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE, OnBnClickedBtnBrowseFile)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE2, OnBnClickedBtnBrowseFile2)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CSurToolGeoTx message handlers

void CSurToolGeoTx::OnBnClickedBtnBrowseFile()
{
	//dataFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\GeoTx", 
	//	"*.xml", "defaulr.xml",
	//	"Will select location of an file geotextures");
	dataFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Textures", 
		"*.tga", "default.tga",
		"Will select location of an file geotextures");

	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_GEOTX);
	ew->SetWindowText(dataFileName.c_str());

	//dataFileName = 
	bool result=true;
	if(!result){
		AfxMessageBox(" Error texture loading");
	}
}
void CSurToolGeoTx::OnBnClickedBtnBrowseFile2()
{
	dataFileName2 = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Textures", 
		"*.tga", "default.tga",
		"Will select location of an file geotextures");

	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_GEOTX2);
	ew->SetWindowText(dataFileName2.c_str());

	bool result=true;
	if(!result){
		AfxMessageBox(" Error texture loading");
	}
}

bool CSurToolGeoTx::onOperationOnMap(int x, int y)
{
	if( vMap.isWorldLoaded() ) { //&& !dataFileName.empty() 
		CWaitCursor wait;
		//vMap.putNewGeoTexture(dataFileName.c_str(), dataFileName2.c_str());
		vMap.WorldRender();
	}
	return true;
}

BOOL CSurToolGeoTx::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_GEOTX);
	ew->SetWindowText(dataFileName.c_str());

	ew=(CEdit*)GetDlgItem(IDC_EDT_GEOTX2);
	ew->SetWindowText(dataFileName2.c_str());

	//if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowAllGeo){
	//	if(vMap.isWorldLoaded()) {
	//		CWaitCursor wait;
	//		vMap.toShowSpecialInfo(vrtMap::SSI_ShowAllGeo);
	//		vMap.WorldRender();
	//		flag_changedGeoViewOption=true;
	//	}
	//}
	//else 
	//	flag_changedGeoViewOption=false;

	return FALSE;
}


void CSurToolGeoTx::OnDestroy()
{
	CSurToolBase::OnDestroy();
	if(flag_changedGeoViewOption){
		if(vMap.isWorldLoaded()) {
			CWaitCursor wait;
			//vMap.toShowSpecialInfo(vrtMap::SSI_NoShow);
			vMap.WorldRender();
		}
	}
}
