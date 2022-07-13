#include "stdafx.h"
#include "SurMap5.h"

#include <direct.h>
#include "DebugUtil.h"

#include "SurToolMiniDetaile.h"
#include "Serialization\Serialization.h"
#include "Game\RenderObjects.h"
#include "Render\src\MultiRegion.h"
#include "Render\src\FileImage.h"
#include "Render\Src\TexLibrary.h"
#include "Render\src\TileMap.h"
#include "Render\src\Scene.h"
#include "Terra\terTools.h"
#include "FileUtils\FileUtils.h"

// CSurToolHardness dialog
//int CSurToolMiniDetail::next_layer = 0;
IMPLEMENT_DYNAMIC(CSurToolMiniDetail, CSurToolBase)
//char* detileName[] =
//{
//	"Зона детализации по умолчанию",
//	"Зона детализации 1",
//	"Песок",
//	"Земля",
//	"Трава",
//	"Трещины",
//	"Дорога",
//	"Камни",
//	"Дополнительная 1",
//	"Дополнительная 2",
//	"Дополнительная 3",
//	"Дополнительная 4",
//	"Дополнительная 5",
//	"Дополнительная 6",
//	"Дополнительная 7",
//	"Дополнительная 8"
//};
CSurToolMiniDetail::CSurToolMiniDetail(int detailTextureLayer, CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	xassert( detailTextureLayer < cTileMap::miniDetailTexturesNumber);
	layer = detailTextureLayer; // next_layer;
	//next_layer++;
	previewTexture_ = NULL;

	if(tileMap)
		setName(extractFileName(tileMap->miniDetailTexture(layer).textureName.c_str()).c_str());
	updateTreeNode();
}

CSurToolMiniDetail::~CSurToolMiniDetail()
{
	RELEASE(previewTexture_);
	//next_layer--;
}

void CSurToolMiniDetail::serialize(Archive& ar) 
{
	//__super::serialize(ar);
	//CString s;
	//if ((UINT)layer<cTileMap::miniDetailTexturesNumber)
	//	s = detileName[layer];
	//else
	//	s.Format("Зона детализации %d",layer);
	//setName(s);
}

void CSurToolMiniDetail::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolMiniDetail, CSurToolBase)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE, OnBnClickedBtnBrowseFile)
	ON_BN_CLICKED(IDC_MINI_DETAIL_ERACE, OnBnClickedMiniDetailErace)
//	ON_WM_ACTIVATE()
ON_BN_CLICKED(IDC_DETAIL_ERACE_ALL, OnBnClickedDetailEraceAll)
END_MESSAGE_MAP()


// CSurToolHardness message handlers

void CSurToolMiniDetail::setPreviewTexture(const char* tgaFName, const char* ddsFName)
{
	dataFileName = ddsFName;
	string new_name = extractFileName(tgaFName);
	GetDlgItem(IDC_EDT_MINITX)->SetWindowText(new_name.c_str());
	setName(new_name.c_str());
	updateTreeNode();

	if (!previewTexture_ || dataFileName!=previewTexture_->name())
	{
		RELEASE(previewTexture_);
		previewTexture_ = GetTexLibrary()->GetElement2D(dataFileName.c_str());
	}
}

BOOL CSurToolMiniDetail::OnInitDialog()
{
	CSurToolBase::OnInitDialog();
	string new_name;
	if(tileMap && layer<cTileMap::miniDetailTexturesNumber)
		setPreviewTexture(tileMap->miniDetailTexture(layer).textureName.c_str(), tileMap->miniDetailTexture(layer).textureName.c_str());
	else{
		GetDlgItem(IDC_BTN_BROWSE_FILE)->ShowWindow(false);
		GetDlgItem(IDC_EDT_MINITX)->ShowWindow(false);
		GetDlgItem(IDC_MINI_DETAIL_ERACE)->ShowWindow(false);
		GetDlgItem(IDC_DETAIL_ERACE_ALL)->ShowWindow(false);
		GetDlgItem(IDC_LAYER_NOT_SUPPORT)->ShowWindow(true);
	}
	return FALSE;
}
bool CSurToolMiniDetail::onOperationOnMap(int x, int y)
{
	if(tileMap && layer<cTileMap::miniDetailTexturesNumber){
		Vect2f point(x, y);
		int radius = getBrushRadius();
		vMap.drawMiniDetailTexture(layer, point, radius);
	}
	return true;
}

void CSurToolMiniDetail::OnDestroy()
{
	RELEASE(previewTexture_);
	CSurToolBase::OnDestroy();
}

bool CSurToolMiniDetail::onDrawAuxData()
{
	if(tileMap && layer < cTileMap::miniDetailTexturesNumber)
		drawCursorCircle();
	return true;
}

bool CSurToolMiniDetail::onDrawPreview(int width, int height)
{
	if (previewTexture_) 
		gb_RenderDevice->DrawSprite (width*0.1f, height*0.1f,
									 width*0.8f, width*0.8f, 0, 0, 1, 1, previewTexture_);
	return true;
}


void CSurToolMiniDetail::OnBnClickedBtnBrowseFile()
{	
	if(tileMap && layer<cTileMap::miniDetailTexturesNumber){
		mkdir("Resource\\TerrainData\\Textures\\Noise");
		string fname = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Textures\\Noise", 
			"*.tga", "bitmap.tga", "Will select location of an file textures");
		if(fname.empty())
			return;
		cTileMap::MiniDetailTexture& miniDetail = tileMap->miniDetailTexture(layer);
		miniDetail.setTexture(fname.c_str());
		setPreviewTexture(miniDetail.textureName.c_str(), miniDetail.textureName.c_str());
	}
}

void CSurToolMiniDetail::OnBnClickedMiniDetailErace()
{
}

void CSurToolMiniDetail::OnBnClickedDetailEraceAll()
{
}

