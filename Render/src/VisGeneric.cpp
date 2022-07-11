#include "StdAfxRD.h"
#include "VisGeneric.h"
#include "ObjLibrary.h"
#include "Scene.h"
#include "Font.h"
#include "..\3dx\Lib3dx.h"
#include "FontInternal.h"

void Init3dxshader();
void Done3dxshader();

static void get_string(char*& str,char* s)
{
	while(*str && IsCharAlphaNumeric(*str))
		*s++=*str++;

	*s=0;
}

static int get_int(char* s)
{
	while(*s && (*s=='\t' || *s==' '))
		s++;
	int i=-1;
	sscanf(s,"%i",&i);
	return i;
}
static float get_float(char* s)
{
	while(*s && (*s=='\t' || *s==' '))
		s++;
	float f=-1;
	sscanf(s,"%f",&f);
	return f;
}

// глобальные переменные
cVisGeneric	*gb_VisGeneric=0;

DebugType<int>		Option_MipMapLevel(5);
DebugType<int>		Option_TextureDetailLevel(0);//”ровень детализации текстур, 0 - самый высокий
DebugType<int>		Option_DrawNumberPolygon(0);
DebugType<int>		Option_ShadowSizePower(true);
DebugType<float>	Option_MapLevel(0.8f);
DebugType<int>		Option_ShowRenderTextureDBG(0);
DebugType<int>		Option_ShadowType(false);
DebugType<int>		Option_FavoriteLoadDDS(false);
bool				Option_IsShadowMap=false;
DebugType<int>		Option_EnableBump(true);
DebugType<int>		Option_ShadowMapSelf4x4(true);//only radeon 9700
DebugType<float>	Option_ParticleRate(1);
DebugType<int>		Option_TileMapTypeNormal(0);//0 - vertex shader, 1 - pixel shader
DebugType<int>		Option_ShadowTSM(0);
DebugType<int>		Option_EnablePreload3dx(1);
DebugType<int>		Option_DeleteLod(0);//0 - ничего не удал€ть, 1 - удалить LOD первого уровн€, 2 - второго.
DebugType<int>		Option_EnableOcclusionQuery(1);

bool				Option_DetailTexture=true;
float				Option_HideFactor=40;
float				Option_HideRange = 100;
bool				Option_HideSmoothly = true;

bool				Option_UseTextureCache = false;
bool				Option_UseMeshCache = false;
bool				Option_UseDOF = false;

bool				Option_AccessibleZBuffer=false;
int					Option_FloatZBufferType=0;// 0-disable ZBuffer, 1 - use only terra, 2 - use objects and terra

DebugType<int>	Option_DprintfLoad(0);
DebugType<int>	Option_DprintfFile(0);
FILE* file_dprintf=NULL;

bool cVisGeneric::assertEnabled_ = true;
bool boolEnablePerfhud=false;

bool Option_ShowType[SHOW_MAX];
EffectLibrary2* gb_EffectLibrary=NULL;

__declspec( thread ) DWORD tls_is_graph = MT_GRAPH_THREAD|MT_LOGIC_THREAD;

bool RenderFileReadXstream(const char *fname,char *&buf,int &size);
bool RenderFileReadXZipStream(const char *fname,char *&buf,int &size);
TypeRenderFileRead RenderFileRead=RenderFileReadXZipStream;

float CONVERT_PROCENT(float x,float min,float max)
{
	return x*(max-min)/100.f+min;
}

cVisGeneric::cVisGeneric(bool multiThread)
{
	maximal_shadow_object=c3dx::OST_SHADOW_REAL;
	is_multithread=multiThread;
	logic_quant=0;
	graph_logic_quant=0;//¬ случае, когда эти переменные вообще не выставл€ютс€, пускай сразу удал€етс€ как только возможно.
	use_logic_quant=false;

	for(int i=0;i<SHOW_MAX;i++)
		Option_ShowType[i]=true;
	Option_ShowType[SHOW_INFO]=false;
	// инициализаци€ глобальных переменых
	shaders=NULL;
	Lib3dx=new cLib3dx;
	LibSimply3dx=new cLibSimply3dx;
	gb_EffectLibrary=new EffectLibrary2;

	interpolation_factor=0;
	logic_time_period_inv=1/100.0f;

	FILE* f=fopen("VisGeneric.cfg","rt");
	if( !f ) return;
	char buf[1024],str[1024];
	str[0]=0;
	DebugType<int>	Option_EnablePerfhud(0);

#define RDI(x) if(stricmp(str,#x)==0) Option_##x.setd(get_int(p)) 
	while(fgets(buf,1023,f))
	{
		char* p=buf;
		get_string(p,str);

		RDI(MipMapLevel);
		RDI(DrawNumberPolygon);
		RDI(ShadowSizePower);
		RDI(ShowRenderTextureDBG);
		RDI(ShadowType);
		RDI(FavoriteLoadDDS);
		RDI(ShadowMapSelf4x4);
		RDI(ShadowTSM);
		RDI(TextureDetailLevel);
		RDI(EnablePreload3dx);
		RDI(DeleteLod);
		RDI(EnableOcclusionQuery);
		RDI(DprintfLoad);
		RDI(DprintfFile);
		RDI(EnablePerfhud);
		else if(stricmp(str,"MapLevel")==0)
		{
			float flt=get_float(p);
			if(1<=flt && flt<=100)
				Option_MapLevel=CONVERT_PROCENT(flt, 0.1f, 0.8f);
		}else if(stricmp(str,"ParticleRate")==0)
		{
			float flt=get_float(p);
			Option_ParticleRate.setd(flt);
		}
	}
	fclose(f);

	silhouettes_enabled = true;
	silhouette_color.set(0, 255, 0);

	boolEnablePerfhud=Option_EnablePerfhud;
	if(Option_DprintfFile)
		file_dprintf=fopen("dprintf.log","wt");

	CalcIsShadowMap();
}

cVisGeneric::~cVisGeneric()
{
	Done3dxshader();
	ReleaseShaders();
	delete Lib3dx;
	delete LibSimply3dx;
	delete gb_EffectLibrary;
	gb_EffectLibrary=NULL;
	ClearData();
	gb_VisGeneric=0;
//	SaveKindObjNotFree();
	if(file_dprintf)
	{
		fclose(file_dprintf);
		file_dprintf=NULL;
	}
}
void cVisGeneric::SetUseTextureCache(bool enable)
{
	Option_UseTextureCache = enable;
}
bool cVisGeneric::GetUseTextureCache()
{
	return Option_UseTextureCache;
}
void cVisGeneric::SetUseMeshCache(bool enable)
{
	Option_UseMeshCache = enable;
}
bool cVisGeneric::GetUseMeshCache()	
{
	return Option_UseMeshCache;
}
void cVisGeneric::SetEnableDOF(bool enable)
{
	if(gb_RenderDevice3D->IsPS20()&&GetFloatZBufferType()==2)
		Option_UseDOF = enable;
	else
		Option_UseDOF = false;
}
bool cVisGeneric::GetEnableDOF()
{
	return Option_UseDOF;
}

void cVisGeneric::SetMipMapLevel(int p)
{
	Option_MipMapLevel=p;
}
void cVisGeneric::SetDrawNumberPolygon(bool p)
{
	Option_DrawNumberPolygon=p;
}
void cVisGeneric::SetMapLevel(float p)
{
	Option_MapLevel=CONVERT_PROCENT(p, 0.1f, 0.8f);
}
void cVisGeneric::SetShowRenderTextureDBG(bool p)
{
	Option_ShowRenderTextureDBG=p;
}

void cVisGeneric::SetFavoriteLoadDDS(bool p)
{
	Option_FavoriteLoadDDS=p;
}

void cVisGeneric::SetShadowType(eShadowType p,int shadow_size)
{
	if(!gb_RenderDevice3D->IsPS20())
	{
		shadow_size=0;
	}
	if(shadow_size==0)
		p=SHADOW_NONE;

	Option_ShadowType=p;
	Option_ShadowSizePower=shadow_size;
	CalcIsShadowMap();
}


//////////////////////////////////////////////////////////////////////////////////////////
cScene* cVisGeneric::CreateScene()
{
	if(file_dprintf)
		fflush(file_dprintf);//„тобы иногда сбрасывалс€.

	cScene *Scene=new cScene;
	return Scene;
}
//////////////////////////////////////////////////////////////////////////////////////////
void cVisGeneric::SetData(cInterfaceRenderDevice *pData)//јнахронизм, надо protected сделать.
{ // функци€ дл€ работы с окном вывода
	cInterfaceRenderDevice *IRenderDevice=pData;
	InitShaders();
	Init3dxshader();
}
void cVisGeneric::ClearData()
{ // функци€ дл€ работы с окном вывода
	vector<cFontInternal*>::iterator it;
	FOR_EACH(fonts,it)
		(*it)->Release();
	fonts.clear();
}
//////////////////////////////////////////////////////////////////////////////////////////
cTexture* cVisGeneric::CreateTexture(const char *TextureName)
{
	return GetTexLibrary()->GetElement2D(TextureName);
}

cTextureScale* cVisGeneric::CreateTextureScale(const char *TextureName,Vect2f scale)
{
	//return GetTexLibrary()->GetElementScale(TextureName,scale);
	return (cTextureScale*)GetTexLibrary()->GetElement2DScale(TextureName,scale);
}

cTexture* cVisGeneric::CreateRenderTexture(int width,int height,int attr,bool enable_assert)
{
	return GetTexLibrary()->CreateRenderTexture(width,height,attr,enable_assert);
}

cTexture* cVisGeneric::CreateTexture(int sizex,int sizey,bool alpha)
{
	return GetTexLibrary()->CreateTexture(sizex,sizey,alpha);
}

cTexture* cVisGeneric::CreateTextureDefaultPool(int sizex,int sizey,bool alpha)
{
	return GetTexLibrary()->CreateTextureDefaultPool(sizex,sizey,alpha);
}

cTexture* cVisGeneric::CreateBumpTexture(int sizex,int sizey)
{
	cTexture *Texture=new cTexture;

	Texture->BitMap.resize(1);
	Texture->BitMap[0]=0;
	Texture->SetWidth(sizex);
	Texture->SetHeight(sizey);

	if(gb_RenderDevice3D->CreateBumpTexture(Texture))
	{
		delete Texture;
		return NULL;
	}

	return Texture;
}

//////////////////////////////////////////////////////////////////////////////////////////
void cVisGeneric::CalcIsShadowMap()
{
	Option_IsShadowMap=(Option_ShadowType==SHADOW_MAP ||
		Option_ShadowType==SHADOW_MAP_SELF) && Option_ShadowSizePower;
}

//////////////////////////////////////////////////////////////////////////////////////////

cInterfaceRenderDevice* cVisGeneric::GetRenderDevice()
{
	return gb_RenderDevice;
}

eShadowType cVisGeneric::GetShadowType()
{
	return (eShadowType)(int)Option_ShadowType;
}

////////////////////DebugMemInfo
#include <crtdbg.h>
struct _CrtMemBlockHeader{
// Pointer to the block allocated just before this one:
   struct _CrtMemBlockHeader *pBlockHeaderNext; 
// Pointer to the block allocated just after this one:
   struct _CrtMemBlockHeader *pBlockHeaderPrev; 
   char *szFileName;   // File name   
   int nLine;          // Line number
   size_t nDataSize;   // Size of user block
   int nBlockUse;      // Type of block
   long lRequest;      // Allocation number
// Buffer just before (lower than) the user's memory:
//   unsigned char gap[nNoMansLandSize];
};


int __cdecl MyAllocHook(
   int      nAllocType,
   void   * pvData,
   size_t   nSize,
   int      nBlockUse,
   long     lRequest,
   const unsigned char * szFileName,
   int      nLine
   )
{
	if(nSize==3096)
	{
		int k=0;
	}
	
	return true;
}

void InitAllockHook()
{
	_CrtSetAllocHook(MyAllocHook);
}

void DebugMemInfo()
{
#ifdef _DEBUG
	_CrtMemState pc;
	_CrtMemCheckpoint(&pc);
	_CrtMemBlockHeader* pm=pc.pBlockHeader;

	const int block_size=12;
	const int min_block_shl=4;
	struct INFO
	{
		int num;
		int size;
	};

	INFO num[block_size];
	int i;
	for(i=0;i<block_size;i++)
	{
		num[i].num=0;
		num[i].size=0;
	}

	while(pm)
	{
		if(pm->nBlockUse==_FREE_BLOCK)continue;

		int sz=pm->nDataSize>>min_block_shl;
		int out=0;
		while(sz)
		{
			out++;
			sz=sz>>1;
		}

		if(out>block_size)
			out=block_size-1;

		num[out].num++;
		num[out].size+=pm->nDataSize;

		pm=pm->pBlockHeaderNext;
	}

	static char text[2048];
	char* p=text;

	int summary_size=0,summary_num=0;
	for(i=0;i<block_size;i++)
	{
		INFO& n=num[i];
		p+=sprintf(p,"% 6i num=% 5i, size=%i.\n",(1<<(min_block_shl+i)),n.num,n.size);
		summary_size+=n.size;
		summary_num+=n.num;
	}

	p+=sprintf(p,"summary size=%i.\n",summary_size);
	p+=sprintf(p,"summary num=%i.\n",summary_num);
	MessageBox(gb_RenderDevice->GetWindowHandle(),text,"Memory info",MB_OK);
#endif _DEBUG
}


void dprintf(char *format, ...)
{
  va_list args;
  char    buffer[512];

  va_start(args,format);
  _vsnprintf(buffer, 511, format, args);

  OutputDebugString(buffer);

  if(file_dprintf)
	fputs(buffer,file_dprintf);
}


void cVisGeneric::SetShowType(eShowType type,bool show)
{
	Option_ShowType[type]=show;
}

void cVisGeneric::XorShowType(eShowType type)
{
	Option_ShowType[type]=!Option_ShowType[type];
}

bool cVisGeneric::GetShowType(eShowType type)
{
	return Option_ShowType[type];
}
void cVisGeneric::SetFloatZBufferType(int type)
{
	Option_FloatZBufferType = type;
}
int cVisGeneric::GetFloatZBufferType()
{
	return Option_FloatZBufferType;
};
bool cVisGeneric::PossibilityBump()
{
	return gb_RenderDevice3D->PossibilityBump();
}

void cVisGeneric::SetEnableBump(bool enable)
{
	if(gb_RenderDevice3D->IsPS20())
		Option_EnableBump=enable;
	else
		Option_EnableBump=false;
}

bool cVisGeneric::GetEnableBump()
{
	return Option_EnableBump;
}

void cVisGeneric::SetEffectLibraryPath(const char* effect_path_,const char* texture_path)
{
	effect_path=effect_path_;
	if(!effect_path.empty())
	{
		char c=effect_path[effect_path.size()-1];
		if(c!='\\' && c!='/')
		{
			effect_path+='\\';
		}
	}

	effect_texture_path=texture_path;
}

void cVisGeneric::SetAnisotropic(int b)
{
	gb_RenderDevice3D->SetAnisotropic(b+1);
}

int cVisGeneric::GetAnisotropic()
{
	return gb_RenderDevice3D->GetAnisotropic();
}
int cVisGeneric::GetMaxAnisotropyLevel()
{
	return gb_RenderDevice3D->GetMaxAnisotropicLevels();
}
void cVisGeneric::EnableSilhouettes(bool enable)
{
	if(gb_RenderDevice3D->IsPS20())
		silhouettes_enabled = enable;
	else
		silhouettes_enabled =false;
}

void cVisGeneric::EnableOcclusion(bool b)
{
	if(gb_RenderDevice3D){
		gb_RenderDevice3D->ReinitOcclusion();
	}
}
bool cVisGeneric::PossibilityOcclusion()
{
	return gb_RenderDevice3D->PossibilityOcclusion();
}

cTexture* cVisGeneric::CreateTextureScreen()
{
	LPDIRECT3DDEVICE9 device=gb_RenderDevice3D->lpD3DDevice;
	HRESULT hr;
	int dx=gb_RenderDevice3D->GetSizeX();
	int dy=gb_RenderDevice3D->GetSizeY();

	int dx_plain_surface=dx;
	int dy_plain_surface=dy;
	if(!gb_RenderDevice3D->IsFullScreen())
	{
		dx_plain_surface=GetSystemMetrics(SM_CXSCREEN);
		dy_plain_surface=GetSystemMetrics(SM_CYSCREEN);
	}

	IDirect3DSurface9* sys_surface=NULL;
	hr=device->CreateOffscreenPlainSurface(
		dx_plain_surface,
		dy_plain_surface,
		D3DFMT_A8R8G8B8,
		D3DPOOL_SYSTEMMEM,
		&sys_surface,
		NULL
	);
	if(FAILED(hr))
	{
		return NULL;
	}

	hr=device->GetFrontBufferData(0,sys_surface);
	if(FAILED(hr))
	{
		RELEASE(sys_surface);
		return NULL;
	}

	int dx_real=Power2up(dx);
	int dy_real=Power2up(dy);

	cTexture* pTextureBackground=CreateTexture(dx_real,dy_real,false);

	D3DLOCKED_RECT lock_in;
	hr=sys_surface->LockRect(&lock_in,NULL,0);
	VISASSERT(SUCCEEDED(hr));
	int out_pitch=0;
	BYTE* out_data=pTextureBackground->LockTexture(out_pitch);

	for(int y=0;y<dy;y++)
	{
		BYTE* in=lock_in.Pitch*y+(BYTE*)lock_in.pBits;
		BYTE* out=out_pitch*y+out_data;
		memcpy(out,in,dx*4);
	}

	sys_surface->UnlockRect();
	pTextureBackground->UnlockTexture();

	RELEASE(sys_surface);

	return pTextureBackground;
}

void cVisGeneric::SetGlobalParticleRate(float r)
{
	xassert(r>=0 && r<=1);
	Option_ParticleRate=r;
}

bool cVisGeneric::PossibilityShadowMapSelf4x4()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		eDrawID id=gb_RenderDevice3D->dtAdvanceOriginal->GetID();
		return id==DT_RADEON9700 || id==DT_GEFORCEFX;
	}
	return false;
}

void cVisGeneric::SetShadowMapSelf4x4(bool b4x4)
{
	Option_ShadowMapSelf4x4=b4x4;
	gb_RenderDevice3D->RestoreShader();
}

void cVisGeneric::SetTilemapDetail(bool b)
{
	if(gb_RenderDevice3D->IsPS20())
	{
		Option_DetailTexture=b;
		gb_RenderDevice3D->RestoreShader();
	}else
	{
		Option_DetailTexture=false;
	}
}

bool cVisGeneric::GetTilemapDetail()
{
	return Option_DetailTexture;
}

void cVisGeneric::SetTileMapTypeNormal(bool pixel_normal)
{
	if(gb_RenderDevice3D->IsPS20())
	{
		Option_TileMapTypeNormal=pixel_normal;
		gb_RenderDevice3D->RestoreShader();
	}else
	{
		Option_TileMapTypeNormal=false;
	}
}

bool cVisGeneric::PossibilityBumpChaos()
{
	return gb_RenderDevice3D->DeviceCaps.TextureOpCaps|D3DTEXOPCAPS_BUMPENVMAP;
}

void cVisGeneric::ReloadAllFont()
{
	MTG();
	for(int i=0;i<fonts.size();i++)
	{
		fonts[i]->Reload();
	}
}

cFont* cVisGeneric::CreateFont(const char *TextureFileName,int h,bool silentErr)
{
	if(TextureFileName==0||TextureFileName[0]==0) return NULL;

	vector<cFontInternal*>::iterator it;
	FOR_EACH(fonts,it)
	{
		cFontInternal* f=*it;
		if(_stricmp(f->font_name.c_str(),TextureFileName)==0 && 
			f->GetStatementHeight()==h)
		{
			return new cFont(f);
		}
	}

	cFontInternal *UObj=new cFontInternal;
	if(!UObj->Create(TextureFileName,h,silentErr))
	{
		delete UObj;
		return NULL;
	}

	fonts.push_back(UObj);
	return new cFont(UObj);
}

cFont* cVisGeneric::CreateFontMem(void* pFontData,int size,int h,bool silentErr)
{
	if(!pFontData || size==0) return NULL;

	cFontInternal *UObj=new cFontInternal;
	if(!UObj->Create(pFontData,size,h,silentErr))
	{
		delete UObj;
		return NULL;
	}

	fonts.push_back(UObj);
	return new cFont(UObj);
}

cFont* cVisGeneric::CreateDebugFont()
{
	if(fonts.empty())
		return NULL;

	int ioptimal=0;
	float dhoptimal=1000;
	for(int i=0;i<fonts.size();i++)
	{
		float h=fonts[i]->GetHeight();
		float dh=fabsf(h-15);
		if(dh<dhoptimal)
		{
			dhoptimal=dh;
			ioptimal=i;
		}
	}

	return new cFont(fonts[ioptimal]);
}

MTTexObjAutoLock::MTTexObjAutoLock()
:object_lock(gb_VisGeneric->GetObjLib()->GetLock()),
 texture_lock(GetTexLibrary()->GetLock())
{
}

bool GetAllTriangle3dx(const char* filename,vector<Vect3f>& point,vector<sPolygon>& polygon)
{
	VISASSERT(gb_VisGeneric);
	cStatic3dx* pStatic=pLibrary3dx->GetElement(filename,NULL,false);
	if(pStatic==0)
		return false;

	cObject3dx *pObject=new cObject3dx(pStatic,false);

	TriangleInfo all;
	pObject->GetTriangleInfo(all,TIF_TRIANGLES|TIF_POSITIONS|TIF_ZERO_POS|TIF_ONE_SCALE);
	point.swap(all.positions);
	polygon.swap(all.triangles);
	pObject->Release();
	return true;
}

bool GetAllTextureNames(const char* filename, vector<string>& names)
{
	xassert(gb_VisGeneric);
	string fn = filename;
	strlwr((char*)fn.c_str());
	if (strstr(fn.c_str(), ".effect"))
		return GetAllTextureNamesEffectLibrary(filename, names);
	cStatic3dx* pStatic=pLibrary3dx->GetElement(filename,NULL,false);
	if(pStatic==0)
        return false;
    
	pStatic->GetTextureNames(names);
	/*
	cObject3dx *pObject=new cObject3dx(pStatic,false);
	pObject->GetAllTextureName(names);
	pObject->Release();
	*/
	return true;
}

void cVisGeneric::SetSilhouetteColor(int index, sColor4c color)
{
	if(silhouette_colors.size() <= index) {
		silhouette_colors.resize(index + 1);
	}
	silhouette_colors[index]=color;
	silhouette_colors[index].a=255;
}

sColor4c cVisGeneric::GetSilhouetteColor(int index) const
{
	xassert(index >= 0 && index < silhouette_colors.size());
	return silhouette_colors[index];
}

int cVisGeneric::GetSilhouetteColorCount() const
{
	return silhouette_colors.size();
}

float cVisGeneric::GetHideFactor()
{
	return Option_HideFactor;
}

float cVisGeneric::GetHideRange()
{
	return Option_HideRange;
}
void cVisGeneric::SetHideRange(float hide_range)
{
	Option_HideRange = hide_range;
}
void cVisGeneric::SetHideFactor(float hide_factor)
{
	Option_HideFactor=hide_factor;
}
bool cVisGeneric::GetHideSmoothly()
{
	return Option_HideSmoothly;
}
void cVisGeneric::SetHideSmoothly(bool hideSmoothly)
{
	Option_HideSmoothly = hideSmoothly;
}

bool cVisGeneric::GetShadowTSM()
{
	return Option_ShadowTSM;
}

void cVisGeneric::SetShadowTSM(bool enable)
{
	Option_ShadowTSM=enable?1:0;
}


void cVisGeneric::SetTextureDetailLevel(int level)
{
	if (Option_TextureDetailLevel != level)
	{
		Option_TextureDetailLevel=level;
		//GetTexLibrary()->ReloadAllTexture();
	}
}

int cVisGeneric::GetTextureDetailLevel()
{
	return Option_TextureDetailLevel;
}


void cVisGeneric::SetMaximalShadowObject(c3dx::OBJECT_SHADOW_TYPE type)
{
	maximal_shadow_object=type;
	{
		cLib3dx::ObjectsList p3dx;
		pLibrary3dx->GetAllElements(p3dx);
		cLib3dx::ObjectsList::iterator it;
		FOR_EACH(p3dx,it)
			(*it)->circle_shadow_enable_min=min(maximal_shadow_object,(*it)->circle_shadow_enable);
	}

	{
		vector<cStaticSimply3dx*> p3dx;
		pLibrarySimply3dx->GetAllElements(p3dx);
		vector<cStaticSimply3dx*>::iterator it;
		FOR_EACH(p3dx,it)
			(*it)->circle_shadow_enable_min=min(maximal_shadow_object,(*it)->circle_shadow_enable);
	}
}

void cVisGeneric::SetRestrictionLOD(int delete_lod)
{
	xassert(0 && "¬ данный момент эта возможность отключена, всв€зи с SetIfNotExistLoadFromCache.");
	xassert(pLibrary3dx->IsEmpty());
	Option_DeleteLod=delete_lod;
}

int cVisGeneric::GetRestrictionLOD()
{
	return Option_DeleteLod;
}
	
void cVisGeneric::SetWorkCacheDir(const char* dir)
{
	GetTexLibrary()->SetWorkCacheDir(dir);
	pLibrary3dx->SetWorkCacheDir(dir);
}
void cVisGeneric::SetBaseCacheDir(const char* dir)
{
	GetTexLibrary()->SetBaseCacheDir(dir);
	pLibrary3dx->SetBaseCacheDir(dir);
}
