#include "StdAfxRD.h"
#include "VisGeneric.h"
#include "Scene.h"
#include "D3DRender.h"
#include "Render\3dx\Lib3dx.h"
#include "Render\Src\VisGeneric.h"
#include "kdw/PropertyEditor.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization\EnumDescriptor.h"

void Init3dxshader();
void Done3dxshader();

// глобальные переменные
RENDER_API cVisGeneric	*gb_VisGeneric=0;

bool Option_VSync = true;
int Option_MipMapLevel = 5;
int Option_TextureDetailLevel(0);//”ровень детализации текстур, 0 - самый высокий
bool Option_DrawNumberPolygon = false;
int Option_ShadowSizePower = 4;
float Option_MapLevel = 0.8f;
int Option_ShowRenderTextureDBG = 0;
bool Option_shadowEnabled = true;
bool Option_FavoriteLoadDDS = false;
bool Option_EnableBump = true;
float Option_ParticleRate = 1;
bool Option_tileMapVertexLight = false;
bool Option_EnablePreload3dx = true;
bool Option_EnableOcclusionQuery = true;

bool Option_DetailTexture=true;
float Option_HideFactor=40;
float Option_HideRange = 100;
bool Option_HideSmoothly = true;

bool Option_UseTextureCache = false;
bool Option_UseMeshCache = false;
bool Option_UseDOF = false;

bool Option_AccessibleZBuffer=false;
int Option_FloatZBufferType=0;// 0-disable ZBuffer, 1 - use only terra, 2 - use objects and terra

bool Option_DprintfLoad = false;
bool Option_DprintfFile = false;

bool Option_EnablePerfhud=false;
bool Option_EnablePureDevice = true;
int Option_cameraDebug = 0;

bool Option_shadowTSM = true;
bool Option_filterShadow = false;
float Option_trapezoidFactor = 0.3f;
bool Option_showDebugTSM = false;

float Option_reflectionOffset = -5;

DebugShowSwitch debugShowSwitch;

FILE* file_dprintf=0;

bool Option_ShowType[SHOW_MAX];

RENDER_API EffectLibrary2* gb_EffectLibrary=0;

__declspec( thread ) DWORD tls_is_graph = MT_GRAPH_THREAD|MT_LOGIC_THREAD;

RENDER_API bool MT_IS_GRAPH() 
{
	return tls_is_graph & MT_GRAPH_THREAD ? true : false;
}

RENDER_API bool MT_IS_LOGIC() 
{
	return tls_is_graph & MT_LOGIC_THREAD ? true : false;
}

RENDER_API void MT_SET_TLS(unsigned int t) 
{ 
	tls_is_graph=t;
}

MTAutoSkipAssert::MTAutoSkipAssert()
{
	real_tls=tls_is_graph;
	tls_is_graph=MT_GRAPH_THREAD|MT_LOGIC_THREAD;
}

MTAutoSkipAssert::~MTAutoSkipAssert()
{
	tls_is_graph=real_tls;
}

float CONVERT_PROCENT(float x,float min,float max)
{
	return x*(max-min)/100.f+min;
}

cVisGeneric::cVisGeneric(bool multiThread)
{
	maximal_shadow_object=OST_SHADOW_REAL;
	is_multithread=multiThread;
	logic_quant=0;
	graph_logic_quant=0;//¬ случае, когда эти переменные вообще не выставл€ютс€, пускай сразу удал€етс€ как только возможно.
	use_logic_quant=false;

	for(int i=0;i<SHOW_MAX;i++)
		Option_ShowType[i]=true;
	Option_ShowType[SHOW_INFO]=false;
	// инициализаци€ глобальных переменых
	shaders=0;
	Lib3dx=new cLib3dx;
	LibSimply3dx=new cLibSimply3dx;
	gb_EffectLibrary=new EffectLibrary2;

	if(check_command_line("VisGeneric"))
		editOption();

	XPrmIArchive ia;
	if(ia.open("VisGeneric.cfg"))
		optionFileName_ = "VisGeneric.cfg";
	else if(ia.open("..\\VisGeneric.cfg"))
		optionFileName_ = "..\\VisGeneric.cfg";
	if(!optionFileName_.empty())
		serialize(ia);
	else
		optionFileName_ = "VisGeneric.cfg";

	silhouettes_enabled = true;
	silhouette_color.set(0, 255, 0);

	if(Option_DprintfFile)
		file_dprintf=fopen("dprintf.log","wt");
}

cVisGeneric::~cVisGeneric()
{
	Done3dxshader();
	ReleaseShaders();
	delete Lib3dx;
	delete LibSimply3dx;
	delete gb_EffectLibrary;
	gb_EffectLibrary=0;
	gb_VisGeneric=0;
//	SaveKindObjNotFree();
	if(file_dprintf)
	{
		fclose(file_dprintf);
		file_dprintf=0;
	}
}

void cVisGeneric::serialize(Archive& ar)
{
	ar.serialize(Option_VSync, "Option_VSync", "Option_VSync");
	ar.serialize(Option_MipMapLevel, "Option_MipMapLevel", "Option_MipMapLevel");
	ar.serialize(Option_TextureDetailLevel, "Option_TextureDetailLevel", "Option_TextureDetailLevel");
	ar.serialize(Option_DrawNumberPolygon, "Option_DrawNumberPolygon", "Option_DrawNumberPolygon");
	ar.serialize(Option_ShadowSizePower, "Option_ShadowSizePower", "Option_ShadowSizePower");
	ar.serialize(Option_MapLevel, "Option_MapLevel", "Option_MapLevel");
	ar.serialize(Option_ShowRenderTextureDBG, "Option_ShowRenderTextureDBG", "Option_ShowRenderTextureDBG");
	ar.serialize(Option_shadowEnabled, "Option_shadowEnabled", "Option_shadowEnabled");
	ar.serialize(Option_FavoriteLoadDDS, "Option_FavoriteLoadDDS", "Option_FavoriteLoadDDS");
	ar.serialize(Option_EnableBump, "Option_EnableBump", "Option_EnableBump");
	ar.serialize(Option_ParticleRate, "Option_ParticleRate", "Option_ParticleRate");
	ar.serialize(Option_tileMapVertexLight, "Option_tileMapVertexLight", "Option_tileMapVertexLight");
	ar.serialize(Option_EnablePreload3dx, "Option_EnablePreload3dx", "Option_EnablePreload3dx");
	ar.serialize(Option_EnableOcclusionQuery, "Option_EnableOcclusionQuery", "Option_EnableOcclusionQuery");

	ar.serialize(Option_DetailTexture, "Option_DetailTexture", "Option_DetailTexture");
	ar.serialize(Option_HideFactor, "Option_HideFactor", "Option_HideFactor");
	ar.serialize(Option_HideRange, "Option_HideRange", "Option_HideRange");
	ar.serialize(Option_HideSmoothly, "Option_HideSmoothly", "Option_HideSmoothly");

	ar.serialize(Option_UseTextureCache, "Option_UseTextureCache", "Option_UseTextureCache");
	ar.serialize(Option_UseMeshCache, "Option_UseMeshCache", "Option_UseMeshCache");
	ar.serialize(Option_UseDOF, "Option_UseDOF", "Option_UseDOF");

	ar.serialize(Option_AccessibleZBuffer, "Option_AccessibleZBuffer", "Option_AccessibleZBuffer");
	ar.serialize(Option_FloatZBufferType, "Option_FloatZBufferType", "Option_FloatZBufferType");

	ar.serialize(Option_DprintfLoad, "Option_DprintfLoad", "Option_DprintfLoad");
	ar.serialize(Option_DprintfFile, "Option_DprintfFile", "Option_DprintfFile");

	ar.serializeArray(Option_ShowType, "Option_ShowType", "Option_ShowType");

	ar.serialize(Option_EnablePerfhud, "Option_EnablePerfhud", "Option_EnablePerfhud");
	ar.serialize(Option_EnablePureDevice, "Option_EnablePureDevice", "Option_EnablePureDevice");
	ar.serialize(Option_cameraDebug, "Option_cameraDebug", "Option_cameraDebug");

	ar.serialize(Option_filterShadow, "Option_FilterShadow", "Option_filterShadow");
	ar.serialize(Option_shadowTSM, "Option_shadowTSM", "Option_shadowTSM");
	ar.serialize(Option_trapezoidFactor, "Option_TrapezoidFactor", "Option_trapezoidFactor");
	ar.serialize(Option_showDebugTSM, "Option_showDebugTSM", "Option_showDebugTSM");
	ar.serialize(Option_reflectionOffset, "Option_reflectionOffset", "Option_reflectionOffset");

	ar.serialize(AlphaMaxiumBlend, "AlphaMaxiumBlend", "AlphaMaxiumBlend");
	ar.serialize(AlphaMiniumShadow, "AlphaMiniumShadow", "AlphaMiniumShadow");

	ar.serialize(debugShowSwitch, "debugShowSwitch", "debugShowSwitch");
}

void cVisGeneric::editOption()
{
	// TODO: переделать через filter
	if(kdw::edit(Serializer(*this), "Scripts\\TreeControlSetups\\cVisGenericState")){
		XPrmOArchive oa(optionFileName_.c_str());
		serialize(oa);
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

void cVisGeneric::SetShadowType(bool shadowEnabled, int shadow_size)
{
	if(!gb_RenderDevice3D->IsPS20() || !gb_RenderDevice->IsEnableSelfShadow())
		shadow_size = 0;
	if(shadow_size==0)
		shadowEnabled = false;

	Option_shadowEnabled = shadowEnabled;
	Option_ShadowSizePower = shadow_size;
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

//////////////////////////////////////////////////////////////////////////////////////////
cTexture* cVisGeneric::CreateTexture(const char *TextureName)
{
	return GetTexLibrary()->GetElement2D(TextureName);
}

cTexture* cVisGeneric::CreateRenderTexture(int width,int height,int attr,bool enable_assert)
{
	return GetTexLibrary()->CreateRenderTexture(width,height,attr,enable_assert);
}

cTexture* cVisGeneric::CreateTexture(int sizex,int sizey,bool alpha)
{
	return GetTexLibrary()->CreateTexture(sizex,sizey,alpha);
}

//////////////////////////////////////////////////////////////////////////////////////////

cInterfaceRenderDevice* cVisGeneric::GetRenderDevice()
{
	return gb_RenderDevice;
}

bool cVisGeneric::shadowEnabled() const 
{
	return Option_shadowEnabled;
}

RENDER_API void dprintf(char *format, ...)
{
  va_list args;
  char    buffer[512];

  va_start(args,format);
  _vsnprintf(buffer, 511, format, args);

  OutputDebugString(buffer);

  if(file_dprintf)
	fputs(buffer,file_dprintf);
}

void cVisGeneric::SetFloatZBufferType(int type)
{
	Option_FloatZBufferType = type;
}

int cVisGeneric::GetFloatZBufferType()
{
	return Option_FloatZBufferType;
}

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
	IDirect3DDevice9* device=gb_RenderDevice3D->D3DDevice_;
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

	IDirect3DSurface9* sys_surface=0;
	hr=device->CreateOffscreenPlainSurface(
		dx_plain_surface,
		dy_plain_surface,
		D3DFMT_A8R8G8B8,
		D3DPOOL_SYSTEMMEM,
		&sys_surface,
		0
	);
	if(FAILED(hr))
	{
		return 0;
	}

	hr=device->GetFrontBufferData(0,sys_surface);
	if(FAILED(hr))
	{
		RELEASE(sys_surface);
		return 0;
	}

	int dx_real=Power2up(dx);
	int dy_real=Power2up(dy);

	cTexture* pTextureBackground=CreateTexture(dx_real,dy_real,false);

	D3DLOCKED_RECT lock_in;
	hr=sys_surface->LockRect(&lock_in,0,0);
	xassert(SUCCEEDED(hr));
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
	Option_filterShadow=b4x4;
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

void cVisGeneric::setTileMapVertexLight(bool vertexLight)
{
	if(gb_RenderDevice3D->IsPS20())
	{
		Option_tileMapVertexLight=vertexLight;
		gb_RenderDevice3D->RestoreShader();
	}
	else
		Option_tileMapVertexLight = true;
}

bool cVisGeneric::PossibilityBumpChaos()
{
	return gb_RenderDevice3D->DeviceCaps.TextureOpCaps|D3DTEXOPCAPS_BUMPENVMAP;
}

RENDER_API bool GetAllTriangle3dx(const char* filename,vector<Vect3f>& point,vector<sPolygon>& polygon)
{
	xassert(gb_VisGeneric);
	cStatic3dx* pStatic=pLibrary3dx->GetElement(filename,0,false);
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

RENDER_API bool GetAllTextureNames(const char* filename, vector<string>& names)
{
	xassert(gb_VisGeneric);
	string fn = filename;
	strlwr((char*)fn.c_str());
	if(strstr(fn.c_str(), ".effect"))
		return GetAllTextureNamesEffectLibrary(filename, names);
	cStatic3dx* pStatic = pLibrary3dx->GetElement(filename,0,false);
	if(pStatic==0)
        return false;
    
	pStatic->GetTextureNames(reinterpret_cast<TextureNames&>(names));
	return true;
}

void cVisGeneric::SetSilhouetteColor(int index, Color4c color)
{
	if(silhouette_colors.size() <= index) {
		silhouette_colors.resize(index + 1);
	}
	silhouette_colors[index]=color;
	silhouette_colors[index].a=255;
}

Color4c cVisGeneric::GetSilhouetteColor(int index) const
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
	return Option_shadowTSM;
}

void cVisGeneric::SetShadowTSM(bool enable)
{
	Option_shadowTSM = enable;
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


void cVisGeneric::SetMaximalShadowObject(ObjectShadowType type)
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

DebugShowSwitch::DebugShowSwitch()
{
	tilemap = 0;
	water = 0;
	objects = 0;
	simplyObjects = 0;
	grass = 0;
	effects = 0;
}

void DebugShowSwitch::serialize(Archive& ar)
{
	ar.serialize(tilemap, "tilemap", "tilemap");
	ar.serialize(water, "water", "water");
	ar.serialize(objects, "objects", "objects");
	ar.serialize(simplyObjects, "simplyObjects", "simplyObjects");
	ar.serialize(grass, "grass", "grass");
	ar.serialize(effects, "effects", "effects");
}
