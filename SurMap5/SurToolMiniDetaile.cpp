#include "stdafx.h"
#include "SurMap5.h"

#include <direct.h>
#include "DebugUtil.h"

#include ".\SurToolMiniDetaile.h"
#include "..\Game\Region.h"
#include "Serialization.h"
#include "..\Game\RenderObjects.h"
#include "..\game\NumDetailTexture.h"
#include "NormalizeTGA.h"
#include "..\Render\src\FileImage.h"
#include "..\Render\inc\IVisD3D.h"
#include "..\Render\src\TileMap.h"
#include "..\Render\src\Scene.h"

extern void DrawOnRegion(int layer, const Vect2i& point, float radius);

// CSurToolHardness dialog
int CSurToolMiniDetail::next_layer = 0;
IMPLEMENT_DYNAMIC(CSurToolMiniDetail, CSurToolBase)
char* detileName[] =
{
	"Зона детализации по умолчанию",
	"Зона детализации 1",
	"Песок",
	"Земля",
	"Трава",
	"Трещины",
	"Дорога",
	"Камни",
	"Кратер"
};
CSurToolMiniDetail::CSurToolMiniDetail(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	xassert(NumDetailTextures+1==sizeof(detileName)/sizeof(detileName[0]));
	layer = next_layer;
	next_layer++;
	previewTexture_ = NULL;

	CString s;
	if ((UINT)layer<NumDetailTextures)
		s = detileName[layer];
	else
		s.Format("Зона детализации %d",layer);
	setName(s);
	updateTreeNode();
}

CSurToolMiniDetail::~CSurToolMiniDetail()
{
	RELEASE(previewTexture_);
	next_layer--;
}

void CSurToolMiniDetail::serialize(Archive& ar) 
{
	__super::serialize(ar);
	CString s;
	if ((UINT)layer<NumDetailTextures)
		s = detileName[layer];
	else
		s.Format("Зона детализации %d",layer);
	setName(s);
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

void CSurToolMiniDetail::SetPreTexture(const char* fname)
{
	dataFileName = fname;
	CString new_name = fname;//toTGA(fname).c_str();
	int ix = new_name.ReverseFind('\\');
	if (ix!=-1)
		new_name = new_name.Right(new_name.GetLength() - ix-1);
	GetDlgItem(IDC_EDT_MINITX)->SetWindowText(new_name);
	if (!previewTexture_ || dataFileName!=previewTexture_->GetName())
	{
		RELEASE(previewTexture_);
		previewTexture_ = GetTexLibrary()->GetElement2D(fname);
	}
}

BOOL CSurToolMiniDetail::OnInitDialog()
{
	CSurToolBase::OnInitDialog();
	string new_name;
	if (terMapPoint && layer<terMapPoint->GetZeroplastNumber())
	{
		SetPreTexture(terScene->GetTilemapDetailTextures().TexNames()[layer].c_str());
	}else
	{
		GetDlgItem(IDC_BTN_BROWSE_FILE)->ShowWindow(false);
		GetDlgItem(IDC_EDT_MINITX)->ShowWindow(false);
		GetDlgItem(IDC_MINI_DETAIL_ERACE)->ShowWindow(false);
		GetDlgItem(IDC_DETAIL_ERACE_ALL)->ShowWindow(false);
		GetDlgItem(IDC_LAYER_NOT_SUPPORT)->ShowWindow(true);
	}
	return FALSE;
}
bool CSurToolMiniDetail::CallBack_OperationOnMap(int x, int y)
{
	int num = terMapPoint->GetZeroplastNumber();
	if (terMapPoint && layer<num)
	{
		Vect2f point(x, y);
		int radius = getBrushRadius();
		DrawOnRegion(layer, point, radius);
		terMapPoint->UpdateMap(point, radius);
	}
	return true;
}

void CSurToolMiniDetail::OnDestroy()
{
	RELEASE(previewTexture_);
	CSurToolBase::OnDestroy();
}

bool CSurToolMiniDetail::CallBack_DrawAuxData()
{
	if(terMapPoint && layer < terMapPoint->GetZeroplastNumber())
		drawCursorCircle();
	return true;
}

bool CSurToolMiniDetail::CallBack_DrawPreview(int width, int height)
{
	if (previewTexture_) 
		gb_RenderDevice->DrawSprite (width*0.1f, height*0.1f,
									 width*0.8f, width*0.8f, 0, 0, 1, 1, previewTexture_);
	return true;
}

bool MakeNoisedTexture(const char* tga_name,const char* dds_name,int tile_size)
{
	static CString destination_path = "Resource\\TerrainData\\Textures\\Noise\\";
	const char* attr = "noise";

	strupr((char*)tga_name);
	cFileImage* image=cFileImage::Create(tga_name);
	if (!image || image->load(tga_name))
	{
		XBuffer strerr;
		strerr < "Cannot load tga file - " < tga_name;
		AfxMessageBox(strerr);
		return false;
	}

	sColor4c* buf=new sColor4c[image->GetX()*image->GetY()];
	image->GetTexture(buf,0,image->GetX(),image->GetY());
	NormalizeTga n;
	if(!n.Normalize(buf,image->GetX(),image->GetY(),tile_size))
	{
		delete image;
		delete buf;
		return false;
	}

	if(!n.SaveDDS(gb_RenderDevice3D->lpD3DDevice,dds_name))
	{
		delete image;
		delete buf;
		return false;
	}

	delete image;
	delete buf;
	return true;
}

void CSurToolMiniDetail::OnBnClickedBtnBrowseFile()
{
	if (terMapPoint && layer<terMapPoint->GetZeroplastNumber())
	{

		mkdir("Resource\\TerrainData\\Textures\\Noise");
		string fname = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Textures\\Noise", 
			"*.tga", "bitmap.tga", "Will select location of an file textures");

		if (fname.empty())
			return;
		string dds_name=toDDS(fname);
		string tga_name=toTGA(fname);
		if(MakeNoisedTexture(tga_name.c_str(),dds_name.c_str(),terMapPoint->GetMiniDetailRes()))
			fname = dds_name;
		else
			fname.clear();

		
/*		dataFileName  = fname;

		std::string::size_type pos = dataFileName.rfind ("\\");
		std::string::size_type end_pos = 0;//dataFileName.rfind (".");
		if (pos != std::string::npos) 
		{
			std::string new_name(dataFileName.begin() + pos + 1, end_pos == std::string::npos ? dataFileName.end() : (dataFileName.begin() + end_pos));
			GetDlgItem(IDC_EDT_MINITX)->SetWindowText(new_name.c_str());
		} 
*/
		terScene->GetTilemapDetailTextures().SetTexture(layer, fname.c_str());
		SetPreTexture(fname.c_str());
	}
}

void CSurToolMiniDetail::OnBnClickedMiniDetailErace()
{
}

void CSurToolMiniDetail::OnBnClickedDetailEraceAll()
{
}

string CSurToolMiniDetail::toTGA(string file_name)
{
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(file_name.c_str(),drive,dir,fname,ext);
	if(stricmp(".tga",ext)==0)
		return file_name;

	for(int i=strlen(fname);i>=0;i--)
	{
		if(fname[i]=='_')
		{
			fname[i]=0;
			break;
		}
	}

	_makepath(path_buffer,drive,dir,fname,".tga");
	return path_buffer;
}

string CSurToolMiniDetail::toDDS(string file_name)
{
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(file_name.c_str(),drive,dir,fname,ext);
	if(stricmp(".dds",ext)==0)
		return file_name;

	sprintf(fname+strlen(fname),"_n%i",terMapPoint->GetMiniDetailRes());
	_makepath(path_buffer,drive,dir,fname,".dds");
	return path_buffer;
}
