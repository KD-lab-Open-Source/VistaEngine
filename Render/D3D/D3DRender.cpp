#include "StdAfxRD.h"
#include "Font.h"
#include "FontInternal.h"

#include "VideoMemoryInformation.h"

//#include "..\Render\VertexDeclarationMacro.h"

cD3DRender::VertexDeclarations& cD3DRender::vertexDeclarations(){
	static cD3DRender::VertexDeclarations declarations;
	return declarations;
}
#define D3D_SDK_VERSION_MY D3D_SDK_VERSION
//		lpD3D=Direct3DCreate9(D3D9b_SDK_VERSION);//Временно, когда появится специфичное для 9.0c переправить обратно
extern bool	boolEnablePerfhud;

SAMPLER_DATA sampler_wrap_point;
SAMPLER_DATA sampler_clamp_point;

SAMPLER_DATA sampler_clamp_linear;
SAMPLER_DATA sampler_wrap_linear;

SAMPLER_DATA sampler_clamp_anisotropic;
SAMPLER_DATA sampler_wrap_anisotropic;

class cD3DRender *gb_RenderDevice3D=NULL;
void IsDeleteAllDefaultTextures();

cD3DRender::cD3DRender()
{
	xassert(sizeof(sShort4)==8);
	original_screen_size.set(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
	InitSamplerConstants();
	nSupportTexture=0;
	bActiveScene=false;
	gb_RenderDevice3D=this;
	hWnd=0;
	
	lpD3D=0;
	lpD3DDevice=0;
    lpZBuffer=lpBackBuffer=0;

	for(int i=0;i<SURFMT_NUMBER;i++)
		TexFmtData[i]=D3DFMT_UNKNOWN;

	MaxTextureAspectRatio=0;

	dtFixed=NULL;
	dtAdvance=NULL;
	dtAdvanceOriginal=NULL;
	current_bump_scale=1.0f;
	DrawNode=NULL;
	pShaderLib=NULL;

	shadow_map_size=-1;
	inv_shadow_map_size=1;

	vertex_fog_param.x=0;
	vertex_fog_param.y=1;
	vertex_fog_param.z=0;
	vertex_fog_param.w=0;

	planar_node_size.x=0;
	planar_node_size.y=0;
	planar_node_size.z=1;
	planar_node_size.w=1;

	fog_of_war_color.x=0.5f;
	fog_of_war_color.y=0.5f;
	fog_of_war_color.z=0.5f;
	fog_of_war_color.w=1.0f;

	tilemap_inv_size.x=1;
	tilemap_inv_size.y=1;
	tilemap_inv_size.z=0;
	tilemap_inv_size.w=0;

	is_fog_of_war=false;

	vsStandart=NULL;
	psStandart=NULL;
	psFont=NULL;
	psMiniMap = NULL;
	psMonochrome = NULL;
	psSolidColor = NULL;

	pQueryEndFrame[0]=pQueryEndFrame[1]=NULL;
	initedQueryEndFrame=false;
	pSphereShadow=NULL;
	bSphereShadowLoaded=false;
	multisample_num=1;
	anisotropic_level = 0;

	multisample=D3DMULTISAMPLE_NONE;
	pWhiteTexture=NULL;
}

cD3DRender::~cD3DRender()
{
	RELEASE(gb_VisGeneric);
	Done();
	gb_RenderDevice3D=NULL;
}

bool cD3DRender::Initialize(int xscr,int yscr,int Mode,HWND lphWnd,int RefreshRateInHz)
{ 
	if(lphWnd==0)
		return false;

	flag_restore_shader=true;
	ClampDeviceSize(xscr,yscr,Mode);

	pShaderLib=new cShaderLib;

	memset(CurrentTexture,0,sizeof(CurrentTexture));
	memset(ArrayRenderState,0xEF,sizeof(ArrayRenderState));

	hWnd=lphWnd;

	RenderMode=Mode;

	if(!lpD3D)
		RDERR((lpD3D=Direct3DCreate9(D3D_SDK_VERSION_MY))==0);
		
	if(lpD3D==0)
		return false;

	D3DDISPLAYMODE d3ddm;
	DWORD Adapter=0/*D3DADAPTER_DEFAULT*/;
	if(boolEnablePerfhud)
	{
//		Adapter=lpD3D->GetAdapterCount()-1;
		UINT a;
		for(a=0;a<lpD3D->GetAdapterCount();a++)
		{
			D3DADAPTER_IDENTIFIER9 Indentifier;
			HRESULT Res;
			Res = lpD3D->GetAdapterIdentifier(a,0,&Indentifier);
			if(strstr(Indentifier.Description,"NVPerfHUD")!=0)
			{
				Adapter=a;
				break;
			}
		}

		if(Adapter!=a)
			boolEnablePerfhud=false;
	}

	RDERR(lpD3D->GetAdapterDisplayMode(Adapter,&d3ddm));

//	RenderMode|=RENDERDEVICE_MODE_REF;
//	if(TexFmtData[SURFMT_RENDERMAP].TexFmtD3D==D3DFMT_UNKNOWN||TexFmtData[SURFMT_COLORALPHA].TexFmtD3D==D3DFMT_UNKNOWN||TexFmtData[SURFMT_COLOR].TexFmtD3D==D3DFMT_UNKNOWN)
//		return false; // don't support render to texture
	RDCALL(lpD3D->GetDeviceCaps(Adapter,D3DDEVTYPE_HAL,&DeviceCaps));
	ZeroMemory(&d3dpp,sizeof(d3dpp));
	D3DDEVTYPE devtype=RenderMode&RENDERDEVICE_MODE_REF?D3DDEVTYPE_REF:D3DDEVTYPE_HAL;

	d3dpp.BackBufferWidth			= xScr=xscr;
	d3dpp.BackBufferHeight			= yScr=yscr;
	d3dpp.MultiSampleType			= (D3DMULTISAMPLE_TYPE)multisample;//D3DMULTISAMPLE_NONMASKABLE;//D3DMULTISAMPLE_NONE;//D3DMULTISAMPLE_6_SAMPLES;//
	d3dpp.MultiSampleQuality		= 0;
	d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow				= (HWND)lphWnd;

	d3dpp.EnableAutoDepthStencil	= TRUE;
	d3dpp.Flags						= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	d3dpp.FullScreen_RefreshRateInHz= (Mode&RENDERDEVICE_MODE_WINDOW)?0:RefreshRateInHz;
    d3dpp.PresentationInterval		= D3DPRESENT_INTERVAL_IMMEDIATE;

	UpdateRenderMode();

	DWORD mt=D3DCREATE_FPU_PRESERVE;

	if(gb_VisGeneric->IsMultithread())
		mt|=D3DCREATE_MULTITHREADED;

	if(RenderMode&RENDERDEVICE_MODE_REF)
	{
		RDERR(lpD3D->CreateDevice(Adapter,D3DDEVTYPE_REF,hWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING|mt,&d3dpp,&lpD3DDevice));
	}else
	{
		if(boolEnablePerfhud)
		{
			RDERR(lpD3D->CreateDevice(Adapter,D3DDEVTYPE_REF,hWnd,D3DCREATE_HARDWARE_VERTEXPROCESSING|mt,&d3dpp,&lpD3DDevice));
		}else
		{
			if(lpD3D->CreateDevice(Adapter,D3DDEVTYPE_HAL,hWnd,D3DCREATE_HARDWARE_VERTEXPROCESSING|mt,&d3dpp,&lpD3DDevice))
				if(lpD3D->CreateDevice(Adapter,D3DDEVTYPE_HAL,hWnd,D3DCREATE_MIXED_VERTEXPROCESSING|mt,&d3dpp,&lpD3DDevice))
					RDERR(lpD3D->CreateDevice(Adapter,D3DDEVTYPE_HAL,hWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING|mt,&d3dpp,&lpD3DDevice));
		}
	}

	if(lpD3DDevice==0)
		return false;

	global_render_window=CreateRenderWindow(hWnd);
	if(RenderMode&RENDERDEVICE_MODE_WINDOW)
		global_render_window->SetSizeDummy(xscr,yscr);
	else
		global_render_window->SetSizeConstant(xscr,yscr);

	SelectRenderWindow(global_render_window);

	if(Mode&RENDERDEVICE_MODE_WINDOW)
	{
		D3DDISPLAYMODE mode;
		if(SUCCEEDED(lpD3DDevice->GetDisplayMode(0, &mode)))
		{
			if(!(mode.Format==D3DFMT_X8R8G8B8||mode.Format==D3DFMT_R8G8B8||mode.Format==D3DFMT_A8R8G8B8))
				return false;//Запускаемся только в true color
		}

	}

	SetClipRect(xScrMin=0,yScrMin=0,xScrMax=xScr,yScrMax=yScr);

	RDCALL(lpD3DDevice->GetDeviceCaps(&DeviceCaps));
	dwSuportMaxSizeTextureX=DeviceCaps.MaxTextureWidth;
	dwSuportMaxSizeTextureY=DeviceCaps.MaxTextureHeight;
	bSupportVertexFog=(DeviceCaps.RasterCaps&D3DPRASTERCAPS_FOGVERTEX)?1:0;
	bSupportTableFog=(DeviceCaps.RasterCaps&D3DPRASTERCAPS_FOGTABLE)?1:0;

	MaxTextureAspectRatio=DeviceCaps.MaxTextureAspectRatio;
	nSupportTexture=min(DeviceCaps.MaxTextureBlendStages,TEXTURE_MAX-1);

	SetFocus(false);

	cD3DRender::CreateVertexDeclarations(lpD3DDevice);
	InitVertexBuffers();
	Fill(0,0,0,255);
	BeginScene();
	DrawRectangle(0,0,xScr-1,yScr-1,sColor4c(0,0,0,255));
	EndScene();
	
	VISASSERT(gb_RenderDevice==this);

	Flush();

	dtFixed=new DrawTypeMinimal;

	dtAdvanceID=DT_UNKNOWN;
	if(DeviceCaps.PixelShaderVersion>= D3DPS_VERSION(2,0) && lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_D16)==0)
	{
		dtAdvanceID=DT_GEFORCEFX;
		dtAdvanceOriginal=new DrawTypeGeforceFX;
	}else
	if( DeviceCaps.PixelShaderVersion>= D3DPS_VERSION(2,0) && 
		lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_TEXTURE,D3DFMT_R32F)==0)
	{
		dtAdvanceID=DT_RADEON9700;
		dtAdvanceOriginal=new DrawTypeRadeon9700;
	}
	VISASSERT(occlusion_query.empty());

	SetAdvance(false);

	InitStandartIB();

	if(bSupportTableFog)
	{
		if(!(dtAdvanceOriginal && (dtAdvanceOriginal->GetID()!=DT_GEFORCEFX)))
			bSupportTableFog=false;
	}

	vsStandart=new VSStandart;
	vsStandart->Restore();
	psStandart=new PSStandart;
	psStandart->Restore();

	psFont=new PSFont;
	psFont->Restore();
	psMiniMap = new PSMiniMap;
	psMiniMap->Restore();
	psMonochrome = new PSMonochrome;
	psMonochrome->Restore();
	psSolidColor = new PSSolidColor;
	psSolidColor->Restore();
	CreateQueryEndFrame();

	CalcMultisampleNum();

	xassert(pWhiteTexture==NULL);
	pWhiteTexture=TexLibrary.GetWhileTexture();

	gb_VisGeneric->SetData(this);
	return true;
}

int cD3DRender::GetBackBuffersSize()
{
	return d3dpp.BackBufferWidth*d3dpp.BackBufferHeight*12;
}

void cD3DRender::DeleteQueryEndFrame()
{
	RELEASE(pQueryEndFrame[0]);
	RELEASE(pQueryEndFrame[1]);
	initedQueryEndFrame=false;
}

void cD3DRender::CreateQueryEndFrame()
{
	DeleteQueryEndFrame();
	bool is0=SUCCEEDED(lpD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pQueryEndFrame[0]));
	bool is1=SUCCEEDED(lpD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pQueryEndFrame[1]));
	initedQueryEndFrame=is0 && is1;
	if(initedQueryEndFrame)
	{
		pQueryEndFrame[0]->Issue(D3DISSUE_END);
		pQueryEndFrame[1]->Issue(D3DISSUE_END);
	}
}

D3DFORMAT cD3DRender::GetBackBufferFormat(int Mode)
{
	DWORD Adapter=0;
	D3DFORMAT BackBufferFormat=D3DFMT_X8R8G8B8;
	if(Mode&RENDERDEVICE_MODE_WINDOW)
	{
		D3DDISPLAYMODE d3ddm;
		DWORD Adapter=0;
		RDCALL(lpD3D->GetAdapterDisplayMode(Adapter,&d3ddm));
		BackBufferFormat = d3ddm.Format;
		if(RenderMode&RENDERDEVICE_MODE_STENCIL)
			BackBufferFormat = D3DFMT_A8R8G8B8;
		else
			BackBufferFormat = D3DFMT_X8R8G8B8;
	}else
	{
		if(RenderMode&RENDERDEVICE_MODE_STENCIL)
			BackBufferFormat = D3DFMT_A8R8G8B8;
		else
		{
			if(lpD3D->GetAdapterModeCount(Adapter,D3DFMT_X8R8G8B8)>0)
				BackBufferFormat = D3DFMT_X8R8G8B8;
			else
				BackBufferFormat = D3DFMT_R8G8B8;
		}
	}

	if(Mode&RENDERDEVICE_MODE_ALPHA)
		BackBufferFormat=D3DFMT_A8R8G8B8;
	return BackBufferFormat;
}

void cD3DRender::UpdateRenderMode()
{
	DWORD Adapter=0;
	D3DDEVTYPE devtype=RenderMode&RENDERDEVICE_MODE_REF?D3DDEVTYPE_REF:D3DDEVTYPE_HAL;

	d3dpp.AutoDepthStencilFormat= (RenderMode&RENDERDEVICE_MODE_STENCIL)?D3DFMT_D24S8:D3DFMT_D24X8;
	d3dpp.BackBufferFormat=GetBackBufferFormat(RenderMode);

	d3dpp.BackBufferCount			= (RenderMode&RENDERDEVICE_MODE_WINDOW)?1:2;
	d3dpp.Windowed					= (RenderMode&(RENDERDEVICE_MODE_WINDOW|RENDERDEVICE_MODE_ONEBACKBUFFER))?TRUE:FALSE;

	if(d3dpp.MultiSampleType==D3DMULTISAMPLE_NONMASKABLE)
	{
		DWORD QualityLevels=0;
		RDCALL(lpD3D->CheckDeviceMultiSampleType(Adapter,
		devtype,
		d3dpp.BackBufferFormat,
		d3dpp.Windowed,
		d3dpp.MultiSampleType,
		&QualityLevels
		));
		d3dpp.MultiSampleQuality=QualityLevels-1;
	}

	D3DDISPLAYMODE d3ddm;
	RDCALL(lpD3D->GetAdapterDisplayMode(Adapter,&d3ddm));
	TexFmtData[SURFMT_COLOR]=D3DFMT_A8R8G8B8;
	TexFmtData[SURFMT_COLORALPHA]=D3DFMT_A8R8G8B8;
	TexFmtData[SURFMT_COLOR32]=D3DFMT_A8R8G8B8;
	TexFmtData[SURFMT_COLORALPHA32]=D3DFMT_A8R8G8B8;

	// render map format
	TexFmtData[SURFMT_RENDERMAP32]=D3DFMT_A8R8G8B8;
	TexFmtData[SURFMT_RENDERMAP16]=D3DFMT_R5G6B5;

	// bump map format
	if(lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_V8U8)==0)
		TexFmtData[SURFMT_BUMP]=D3DFMT_V8U8;
		

	if(lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_TEXTURE,D3DFMT_R32F)==0)
		TexFmtData[SURFMT_RENDERMAP_FLOAT]=D3DFMT_R32F;

	if(lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_L8)==0)
	{
		TexFmtData[SURFMT_L8]=D3DFMT_L8;
	}else
	{
		xassert(0 && "Old videocard");
	}

	if(lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_A8L8)==0)
	{
		TexFmtData[SURFMT_A8L8]=D3DFMT_A8L8;
	}else
	{
		xassert(0 && "Old videocard");
	}

	if(lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_V8U8)==0)
	{
		TexFmtData[SURFMT_UV]=D3DFMT_V8U8;
	}

	if(lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_V16U16)==0)
	{
		TexFmtData[SURFMT_U16V16]=D3DFMT_V16U16;
	}
	if (lpD3D->CheckDeviceFormat(Adapter,devtype,d3ddm.Format,0,D3DRTYPE_TEXTURE,D3DFMT_R32F)==0)
	{
		TexFmtData[SURFMT_R32F]=D3DFMT_R32F;
	}
}

void cD3DRender::ClampDeviceSize(int& x,int& y,int mode)
{
	if(mode&RENDERDEVICE_MODE_WINDOW)
	{
		x=min(x,original_screen_size.x);
		y=min(y,original_screen_size.y);
	}
}

bool cD3DRender::ChangeSize(int xscr,int yscr,int mode)
{
	ClampDeviceSize(xscr,yscr,mode);
	if(all_render_window.size()==1)
	{
		//if(mode&RENDERDEVICE_MODE_WINDOW)
		//{
		//	global_render_window->ChangeSize();
		//}else
			global_render_window->SetSizeConstant(xscr,yscr);
		return ChangeSizeInternal(xscr,yscr,mode);
	}else
	{
		xassert(mode&RENDERDEVICE_MODE_WINDOW);
		vector<cRenderWindow*>::iterator it;
		FOR_EACH(all_render_window,it)
			(*it)->ChangeSize();
		return RecalculateDeviceSize();
	}
}

bool cD3DRender::ChangeSizeInternal(int xscr,int yscr,int mode)
{
	MTAuto reset_lock(gb_VisGeneric->GetLockResetDevice());
	KillFocus();
	int mode_mask=RENDERDEVICE_MODE_ALPHA|RENDERDEVICE_MODE_WINDOW;

	RenderMode&=~mode_mask;
	RenderMode|=mode;
	if(RenderMode&RENDERDEVICE_MODE_WINDOW)
	{
		D3DDISPLAYMODE mode;
		if(SUCCEEDED(lpD3DDevice->GetDisplayMode(0, &mode)))
		{
			if(!(mode.Format==D3DFMT_X8R8G8B8||mode.Format==D3DFMT_R8G8B8||mode.Format==D3DFMT_A8R8G8B8))
				return false;
		}

	}

	d3dpp.BackBufferWidth	= xScr=xscr;
	d3dpp.BackBufferHeight	= yScr=yscr;
	d3dpp.MultiSampleType = multisample;
	UpdateRenderMode();

	bool ret=SetFocus(false,(mode&RENDERDEVICE_MODE_RETURNERROR)?false:true);
	return ret;
}


extern bool in_draw_assert;
void cD3DRender::SetAdvance(bool is_shadow)
{
	xassert(!in_draw_assert);
	bool self_shadow=(Option_ShadowType==SHADOW_MAP_SELF) && is_shadow;
	if(dtAdvanceOriginal)
	{
		if(self_shadow)
			dtAdvance=dtAdvanceOriginal;
		else
			dtAdvance=dtFixed;
	}else
		dtAdvance=dtFixed;
}

bool cD3DRender::IsEnableSelfShadow()
{
	return dtAdvanceOriginal!=NULL;
}


int cD3DRender::Done()
{
	RELEASE(pWhiteTexture);

	RDCALL(lpD3DDevice->SetStreamSource(0,NULL,0,0));
	SetIndices(NULL);
	SetVertexShader(NULL);
	SetPixelShader(NULL);

	RELEASE(pSphereShadow);
	KillFocus();
	DeleteQueryEndFrame();

	if(dtFixed)
	{
		delete dtFixed;
		dtFixed=NULL;
	}

	dtAdvance=NULL;

	if(dtAdvanceOriginal)
	{
		delete dtAdvanceOriginal;
		dtAdvanceOriginal=NULL;
	}

	DeleteIndexBuffer(standart_ib);
	BufferXYZDT1.Destroy();
	BufferXYZDT2.Destroy();
	BufferXYZWD.Destroy();
	BufferXYZD.Destroy();
	BufferXYZWDT1.Destroy();
	BufferXYZWDT2.Destroy();
	BufferXYZWDT3.Destroy();
	QuadBufferXYZDT1.Destroy();
	QuadBufferXYZWDT1.Destroy();
	QuadBufferXYZWDT2.Destroy();
	QuadBufferXYZWDT3.Destroy();
	BufferXYZOcclusion.Destroy();
/*
	for(int i=0;i<LibVB.size();i++)
	{
		sSlotVB& s=LibVB[i];
		VISASSERT(!s.init);
	}

	for(i=0;i<LibIB.size();i++)
	{
		sSlotIB& s=LibIB[i];
		VISASSERT(!s.init);
	}
*/	
	LibVB.clear();
	LibIB.clear();
	TexLibrary.Free();
	
	bActiveScene=0;
	delete pShaderLib;
	pShaderLib=NULL;

	RELEASE(lpD3DDevice);
	RELEASE(lpD3D);
	
	xScr=yScr=xScrMin=yScrMin=xScrMax=yScrMax=0;
	hWnd=0;	RenderMode=0;


	RELEASE(global_render_window);
	cur_render_window=NULL;
	delete vsStandart;
	vsStandart=NULL;
	delete psStandart;
	psStandart=NULL;
	delete psFont;
	psFont=NULL;
	delete psMiniMap;
	psMiniMap = NULL;
	delete psMonochrome;
	psMonochrome = NULL;
	delete psSolidColor;
	psSolidColor = NULL;

	ReleaseVertexDeclarations();
	return 0;
}
int cD3DRender::GetClipRect(int *xmin,int *ymin,int *xmax,int *ymax)
{
	if(lpD3DDevice==0) return -1;
	D3DVIEWPORT9 vp;
	RDCALL(lpD3DDevice->GetViewport(&vp));
	*xmin=xScrMin=vp.X; *xmax=xScrMax=vp.X+vp.Width;
	*ymin=yScrMin=vp.Y; *ymax=yScrMax=vp.Y+vp.Height;
	return 0;
}

int cD3DRender::SetClipRect(int xmin,int ymin,int xmax,int ymax)
{
	if(lpD3DDevice==0) return -1;
	xassert(xmin<=xmax);
	xassert(ymin<=ymax);
	xassert(xmin>=0 && xmax<=GetSizeX());
	xassert(ymin>=0 && ymax<=GetSizeY());
	xScrMin=xmin;
	yScrMin=ymin;
	xScrMax=xmax;
	yScrMax=ymax;

	D3DVIEWPORT9 vp={xScrMin,yScrMin,xScrMax-xScrMin,yScrMax-yScrMin,0.0f,1.0f};
	RDCALL(lpD3DDevice->SetViewport(&vp));
	return 0;
}

int cD3DRender::Fill(int r,int g,int b,int a)
{ 
	xassert(!bActiveScene);
//	if(bActiveScene) EndScene();
	if(lpD3DDevice==0) return -1;
	RestoreDeviceIfLost();
	a=0;//Затычка для ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE 

	D3DVIEWPORT9 vp={0,0,GetSizeX(),GetSizeY(),0.0f,1.0f};
	RDCALL(lpD3DDevice->SetViewport(&vp));
	RDCALL(lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|((RenderMode&RENDERDEVICE_MODE_STENCIL)?D3DCLEAR_STENCIL:0),
		D3DCOLOR_RGBA(r,g,b,a),1,0));

	return 0;
}

void cD3DRender::RestoreDeviceIfLost()
{
	int hr;
	while(FAILED(hr=lpD3DDevice->TestCooperativeLevel()))
    { // Test the cooperative level to see if it's okay to render
        if( D3DERR_DEVICELOST == hr || D3DERR_DEVICENOTRESET == hr)
		{
			MSG msg;
			if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
				if(!GetMessage(&msg, NULL, 0, 0))
					break;
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}else
				WaitMessage();
//			VISASSERT(0 && D3DERR_DEVICELOST);
//          return;
		}

        if(hr==D3DERR_DEVICENOTRESET)
		{
			MTAuto reset_lock(gb_VisGeneric->GetLockResetDevice());
			KillFocus();
			SetFocus(false);
		}
    }
}

void cD3DRender::RestoreDeviceForce()
{
	MTG();
	MTAuto reset_lock(gb_VisGeneric->GetLockResetDevice());
/*
	struct IDirect3DSurface9* pSysSurface=NULL;
	{
		IDirect3DTexture9* pRenderTexture=NULL;
		bool ok=true;
		RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateTexture(xScr,yScr,1,D3DUSAGE_RENDERTARGET,D3DFMT_X8R8G8B8,D3DPOOL_DEFAULT,&pRenderTexture,NULL));
		RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateOffscreenPlainSurface(xScr,yScr,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&pSysSurface,NULL));

		if(pRenderTexture && pSysSurface)
		{
			RECT rc={0,0,xScr,yScr};
			IDirect3DSurface9 *pDestSurface=NULL;
			RDCALL(pRenderTexture->GetSurfaceLevel(0,&pDestSurface));

			RDCALL(gb_RenderDevice3D->lpD3DDevice->StretchRect(
				gb_RenderDevice3D->lpBackBuffer,NULL,pDestSurface,&rc,D3DTEXF_LINEAR));
			RDCALL(gb_RenderDevice3D->lpD3DDevice->GetRenderTargetData(pDestSurface,pSysSurface));

			pDestSurface->Release();
		}
		RELEASE(pRenderTexture);
	}
*/

	KillFocus();
	SetFocus(true);
/*
	if(pSysSurface)
	{
		cTexture* pTexture=TexLibrary.CreateTexture(xScr,yScr,SURFMT_COLORALPHA32,false);
		pTexture->ClearAttribute(TEXTURE_ALPHA_BLEND);

		D3DLOCKED_RECT LockedRect;
		RDCALL(pSysSurface->LockRect(&LockedRect,
			NULL,
			D3DLOCK_READONLY
		));

		int out_pitch;
		BYTE* pOut=pTexture->LockTexture(out_pitch);
		for(int y=0;y<yScr;y++)
		{
			memcpy(pOut+out_pitch*y,LockedRect.Pitch*y+(BYTE*)LockedRect.pBits,xScr*4);

			//sColor4c* in=(sColor4c*)(LockedRect.Pitch*y+(BYTE*)LockedRect.pBits);
			//sColor4c* out=(sColor4c*)(pOut+out_pitch*y);
			//for(int x=0;x<xScr;x++)
			//{
			//	sColor4c c=in[x];
			//	c.a=255;
			//	out[x]=c;
			//}
		}

		pTexture->UnlockTexture();
		pSysSurface->UnlockRect();

		Fill(255,0,0,255);
		BeginScene();
		DrawSprite(0,0,xScr,yScr,0,0,1,1,pTexture);
		EndScene();
		Flush();
		Sleep(1000);

		RELEASE(pTexture);
		RELEASE(pSysSurface);
	}
*/
}

int cD3DRender::Flush()
{ 
	if(bActiveScene) EndScene();
	MTG();
	RestoreDeviceIfLost();

	if(initedQueryEndFrame)
	{
		evenQueryEndFrame=!evenQueryEndFrame;
		pQueryEndFrame[evenQueryEndFrame?1:0]->Issue(D3DISSUE_END);
		DWORD data;
		while (pQueryEndFrame[evenQueryEndFrame?0:1]->GetData(&data, sizeof(data), D3DGETDATA_FLUSH) == S_FALSE);
	}

	
//	lpD3DDevice->Present(NULL,NULL,wnd,NULL);
	HRESULT hr;
	if(!IsFullScreen() && (cur_render_window->SizeX()!=xScr || cur_render_window->SizeY()!=yScr))
	{
		RECT rc;
		rc.left=0;
		rc.top=0;
		rc.right=cur_render_window->SizeX();
		rc.bottom=cur_render_window->SizeY();
		hr=lpD3DDevice->Present(&rc,&rc,cur_render_window->GetHwnd(),NULL);
		
	}else
	{
		hr=lpD3DDevice->Present(NULL,NULL,cur_render_window->GetHwnd(),NULL);
	}

	xassert(hr==D3D_OK);

	return 0;
}
int cD3DRender::BeginScene()
{ 
	if(lpD3DDevice==0) return -1;
	if(bActiveScene)
	{	
		xassert(!bActiveScene);
		return 1;
	}

	if(boolEnablePerfhud)
	{
		BufferXYZDT1.Rewind();
		BufferXYZDT2.Rewind();
		BufferXYZWD.Rewind();
		BufferXYZD.Rewind();
		BufferXYZWDT1.Rewind();
		BufferXYZWDT2.Rewind();
		BufferXYZWDT3.Rewind();
		QuadBufferXYZDT1.Rewind();
		QuadBufferXYZWDT1.Rewind();
		QuadBufferXYZWDT2.Rewind();
		QuadBufferXYZWDT3.Rewind();
		BufferXYZOcclusion.Rewind();
	}

	RestoreShaderReal();
	bActiveScene=1;
	DrawNode=NULL;
	HRESULT hr=lpD3DDevice->BeginScene();
	
	NumberPolygon=0;
	NumDrawObject=0;
	NumberTilemapPolygon=0;
	CurrentIndexBuffer=NULL;
	CurrentVertexShader=NULL;;
	CurrentPixelShader=NULL;
	CurrentFVF=0;
	CurrentDeclaration=0;
    for(int i=0;i<RENDERSTATE_MAX;i++)
		ArrayRenderState[i]=0xEFEFEFEF;
    for(int j=0;j<TEXTURE_MAX;j++)
	{
		int i;
	    for(i=0;i<TEXTURESTATE_MAX;i++)
			ArrayTextureStageState[j][i]=0xEFEFEFEF;
		for(i=0;i<SAMPLERSTATE_MAX;i++)
			ArraytSamplerState[j][i]=0xEFEFEFEF;
	}

	FillSamplerState();

	for( int nPasses=0; nPasses<nSupportTexture; nPasses++ ) 
	{
		lpD3DDevice->SetTexture( nPasses, CurrentTexture[nPasses]=0 );
		SetTextureStageState( nPasses, D3DTSS_COLOROP,	 D3DTOP_DISABLE );
		SetTextureStageState( nPasses, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		SetTextureStageState( nPasses, D3DTSS_COLORARG2, nPasses ? D3DTA_CURRENT : D3DTA_DIFFUSE );

		SetTextureStageState( nPasses, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE );
		SetTextureStageState( nPasses, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		SetTextureStageState( nPasses, D3DTSS_ALPHAARG2, nPasses ? D3DTA_CURRENT : D3DTA_DIFFUSE );

  		SetSamplerState( nPasses, D3DSAMP_MINFILTER, sampler_clamp_linear.minfilter );	// D3DTEXF_POINT
		SetSamplerState( nPasses, D3DSAMP_MAGFILTER, sampler_clamp_linear.magfilter );
		SetSamplerState( nPasses, D3DSAMP_MIPFILTER, sampler_clamp_linear.mipfilter );	// trilinear

		SetTextureStageState( nPasses, D3DTSS_BUMPENVMAT00, F2DW(0.5f) );
		SetTextureStageState( nPasses, D3DTSS_BUMPENVMAT01, F2DW(0.0f) );
		SetTextureStageState( nPasses, D3DTSS_BUMPENVMAT10, F2DW(0.0f) );
		SetTextureStageState( nPasses, D3DTSS_BUMPENVMAT11, F2DW(0.5f) );
        SetTextureStageState( nPasses, D3DTSS_BUMPENVLSCALE, F2DW(4.0f) );
        SetTextureStageState( nPasses, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f) );
		SetTextureStageState( nPasses, D3DTSS_TEXCOORDINDEX, nPasses);

		if(GetAnisotropic())
			SetSamplerState( nPasses, D3DSAMP_MAXANISOTROPY, anisotropic_level);
	}

//	SetRenderState(D3DRS_TEXTUREPERSPECTIVE,TRUE);
//	SetRenderState(D3DRS_ANTIALIAS,D3DANTIALIAS_NONE);
	SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
	SetRenderState(D3DRS_FILLMODE,FILL_SOLID);
	SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);
	SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
	SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
	SetRenderState(D3DRS_LASTPIXEL,TRUE);
	SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	SetRenderState(D3DRS_CULLMODE,CurrentCullMode=D3DCULL_CW);
	SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	SetRenderState(D3DRS_ALPHAREF,0);
	SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER); //D3DCMP_ALWAYS

	SetRenderState(D3DRS_DITHERENABLE,FALSE);
	SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	SetRenderState(D3DRS_SPECULARENABLE,FALSE);
	SetRenderState(D3DRS_DEPTHBIAS,0);
	SetRenderState(D3DRS_TEXTUREFACTOR,0xFFFFFFFF);

	SetRenderState(D3DRS_CLIPPING,TRUE);
	SetRenderState(D3DRS_LIGHTING,FALSE);

	SetRenderState(D3DRS_AMBIENT,0xFFFFFFFF);
	SetRenderState(D3DRS_COLORVERTEX,FALSE);
	SetRenderState(D3DRS_CLIPPLANEENABLE,FALSE);

	// FOG RenderState
	SetRenderState(D3DRS_FOGENABLE,FALSE);
	SetRenderState(D3DRS_RANGEFOGENABLE,FALSE);
	SetRenderState(D3DRS_FOGSTART,0);
	SetRenderState(D3DRS_FOGEND,0);
	SetRenderState(D3DRS_FOGDENSITY,0);
	if(bSupportTableFog && (dtAdvance && (dtAdvance->GetID()!=DT_GEFORCEFX)))
		SetRenderState( D3DRS_FOGTABLEMODE,   D3DFOG_LINEAR );
	else 
	if(bSupportVertexFog)
	{
		SetRenderState( D3DRS_FOGVERTEXMODE,  D3DFOG_LINEAR );
	}else
		SetRenderState( D3DRS_FOGTABLEMODE,  D3DFOG_NONE ),
		SetRenderState( D3DRS_FOGVERTEXMODE,  D3DFOG_NONE );

	return hr;
}

int cD3DRender::EndScene()
{ 
	if(lpD3DDevice==0) return -1;
	if(!bActiveScene) return 1; 


	DrawNumberPolygon(32,136);
//	D3DVIEWPORT9 vpall={0,0,xScr,yScr,0.0f,1.0f};
//	RDCALL(lpD3DDevice->SetViewport(&vpall));
	FlushPrimitive2D();
	FlushPrimitive3D();
	bActiveScene=0;
	HRESULT hr=lpD3DDevice->EndScene();
	return hr;
}

void cD3DRender::FlushPrimitive2D()
{
	SetVertexShader(NULL);
	SetPixelShader(NULL);

	FlushPixel();
	FlushLine();
	FlushFilledRect();
}

void cD3DRender::SetGlobalLight(Vect3f *vLight,sColor4f *Ambient,sColor4f *Diffuse,sColor4f *Specular)
{
	if(Ambient==0||Diffuse==0||Specular==0)	
	{
		RDCALL(lpD3DDevice->LightEnable(0,FALSE));
		return;
	}

	D3DLIGHT9 GlobalLight;
	memset(&GlobalLight,0,sizeof(GlobalLight));
	GlobalLight.Type = D3DLIGHT_DIRECTIONAL;
	if(vLight)
		memcpy(&GlobalLight.Direction.x,&vLight->x,sizeof(GlobalLight.Direction));
	else
		memcpy(&GlobalLight.Direction.x,&Vect3f(0,0,1),sizeof(GlobalLight.Direction));
	
	memcpy(&GlobalLight.Ambient.r,&Ambient->r,sizeof(GlobalLight.Ambient));
	memcpy(&GlobalLight.Diffuse.r,&Diffuse->r,sizeof(GlobalLight.Diffuse));
	memcpy(&GlobalLight.Specular.r,&Specular->r,sizeof(GlobalLight.Specular));
	
	GlobalLight.Range=1000.f;
	GlobalLight.Attenuation0=0.0f;
	GlobalLight.Attenuation1=1.0f;
	GlobalLight.Attenuation2=0.0f;

	lpD3DDevice->SetLight(0,&GlobalLight);
	lpD3DDevice->LightEnable(0,TRUE);
}
void cD3DRender::SetRenderState(eRenderStateOption option,int value)
{ 
	switch(option)
	{
		case RS_CULLMODE:
			if(value<0) value=CurrentCullMode;
			break;
	}
	SetRenderState((D3DRENDERSTATETYPE)option,value);
}
unsigned int cD3DRender::GetRenderState(eRenderStateOption option)
{
	return GetRenderState((D3DRENDERSTATETYPE)option);
}

void cD3DRender::DrawLine(int x1,int y1,int x2,int y2,sColor4c color)
{ 
	MTG();
	VISASSERT(bActiveScene);
	if(x1<=x2) { if(x2<xScrMin||x1>xScrMax) return; }
	else if(x1<xScrMin||x2>xScrMax) return;
	if(y1<=y2) { if(y2<yScrMin||y1>yScrMax) return; }
	else if(y1<yScrMin||y2>yScrMax) return;

	PointStruct v;
	v.x=(float)x1;
	v.y=(float)y1;
	v.diffuse=color;
	lines.push_back(v);
	v.x=(float)x2;
	v.y=(float)y2;
	lines.push_back(v);
}

void cD3DRender::DrawRectangle(int x1,int y1,int dx,int dy,sColor4c color,bool outline)
{ 
   	VISASSERT(bActiveScene);
	int x2=x1+dx,y2=y1+dy;
	if(dx>=0) { if(x2<xScrMin||x1>xScrMax) return; }
	else if(x1<xScrMin||x2>xScrMax) return;
	if(dy>=0) { if(y2<yScrMin||y1>yScrMax) return; }
	else if(y1<yScrMin||y2>yScrMax) return;

	if(outline)
	{
		PointStruct v;
		v.diffuse=color;
		v.x=(float)x1; v.y=(float)y1; lines.push_back(v);
		v.x=(float)x2; v.y=(float)y1; lines.push_back(v);

		v.x=(float)x2; v.y=(float)y1; lines.push_back(v);
		v.x=(float)x2; v.y=(float)y2; lines.push_back(v);

		v.x=(float)x2; v.y=(float)y2; lines.push_back(v);
		v.x=(float)x1; v.y=(float)y2; lines.push_back(v);

		v.x=(float)x1; v.y=(float)y2; lines.push_back(v);
		v.x=(float)x1; v.y=(float)y1; lines.push_back(v);
	}else
	{
		RectStruct s;
		s.x1=(float)x1;s.x2=(float)x2;
		s.y1=(float)y1;s.y2=(float)y2;
		s.diffuse=color;
		rectangles.push_back(s);
	}
}

void cD3DRender::FlushPrimitive3D()
{
	SetVertexShader(NULL);
	SetPixelShader(NULL);
	VISASSERT(bActiveScene);
	FlushLine3D();
	FlushPoint3D();
}

void cD3DRender::FlushPrimitive3DWorld()
{
	VISASSERT(bActiveScene);
	FlushLine3D(true);
	FlushPoint3D();
}

void cD3DRender::FlushLine3D(bool world,bool check_zbuffer)
{
	if(lines3d.empty())return;

	DWORD prev_zfunc=GetRenderState(D3DRS_ZFUNC);
	DWORD prev_fog=GetRenderState(D3DRS_FOGENABLE);
	if(check_zbuffer)
		SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	else
		SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
	SetRenderState(D3DRS_FOGENABLE,FALSE);
	if(world)
	{
		SetWorldMaterial(ALPHA_BLEND,MatXf::ID);
		//SetWorldMaterial(ALPHA_NONE,MatXf::ID);
	}else
	{
		SetNoMaterial(ALPHA_BLEND,MatXf::ID);
	}

	VISASSERT((lines3d.size()&1)==0);
	int npoint=0;
	sVertexXYZD* v=BufferXYZD.Lock();
	vector<sVertexXYZD>::iterator it;
	FOR_EACH(lines3d,it)
	{
		v[npoint]=*it;
		npoint++;
		if(((npoint&1)==0) && npoint>=BufferXYZD.GetSize()-2)
		{
			BufferXYZD.Unlock(npoint);
			BufferXYZD.DrawPrimitive(PT_LINELIST,npoint/2);
			v=BufferXYZD.Lock();
			npoint=0;
		}
	}

	BufferXYZD.Unlock(npoint);
	if(npoint)
		BufferXYZD.DrawPrimitive(PT_LINELIST,npoint/2);

	lines3d.clear();
	SetRenderState(D3DRS_ZFUNC,prev_zfunc);
	SetRenderState(D3DRS_FOGENABLE,prev_fog);
}

void cD3DRender::FlushPoint3D()
{
	if(points3d.empty())return;

	DWORD prev_zfunc=GetRenderState(D3DRS_ZFUNC);
	DWORD prev_fog=GetRenderState(D3DRS_FOGENABLE);
	SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
	SetRenderState(D3DRS_FOGENABLE,FALSE);
	SetNoMaterial(ALPHA_BLEND,MatXf::ID);

	int npoint=0;
	sVertexXYZD* v=BufferXYZD.Lock();
	vector<sVertexXYZD>::iterator it;
	FOR_EACH(points3d,it)
	{
		v[npoint]=*it;
		npoint++;
		if(npoint>=BufferXYZD.GetSize()-2)
		{
			BufferXYZD.Unlock(npoint);
			BufferXYZD.DrawPrimitive(PT_POINTLIST,npoint);
			v=BufferXYZD.Lock();
			npoint=0;
		}
	}

	BufferXYZD.Unlock(npoint);
	if(npoint)
		BufferXYZD.DrawPrimitive(PT_POINTLIST,npoint);

	points3d.clear();
	SetRenderState(D3DRS_ZFUNC,prev_zfunc);
	SetRenderState(D3DRS_FOGENABLE,prev_fog);
}

void cD3DRender::DrawLine(const Vect3f &v1,const Vect3f &v2,sColor4c color)
{
	MTG();
	VISASSERT(bActiveScene);
	sVertexXYZD v;
	v.pos=v1;
	v.diffuse=color;
	lines3d.push_back(v);
	v.pos=v2;
	lines3d.push_back(v);
}

void cD3DRender::DrawPoint(const Vect3f &v1,sColor4c color)
{
	VISASSERT(bActiveScene);
	sVertexXYZD v;
	v.pos=v1;
	v.diffuse=color;
	points3d.push_back(v);
}

void cD3DRender::DrawPixel(int x1,int y1,sColor4c color)
{ 
    if(!bActiveScene) return;
	if(x1<xScrMin||x1>xScrMax||y1<yScrMin||y1>yScrMax) return;
	PointStruct v;
	v.x=(float)x1;
	v.y=(float)y1;
	v.diffuse=color;
	points.push_back(v);
	return; 
}

void cD3DRender::FlushPixel()
{
	if(points.empty())return;
	SetNoMaterial(ALPHA_BLEND, MatXf::ID);//FIXME
	SetRenderState(D3DRS_ZWRITEENABLE, FALSE); 
	SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
	SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_ALWAYS);
	SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);

	int npoint=0;
	sVertexXYZWD* v=BufferXYZWD.Lock();
	vector<PointStruct>::iterator it;
	FOR_EACH(points,it)
	{
		PointStruct& p=*it;
		sVertexXYZWD& cv=v[npoint];
		cv.x=p.x;
		cv.y=p.y;
		cv.z=0.001f;
		cv.w=0.001f;
		cv.diffuse=p.diffuse;

		npoint++;
		if(npoint>=BufferXYZWD.GetSize())
		{
			BufferXYZWD.Unlock(npoint);
			BufferXYZWD.DrawPrimitive(PT_POINTLIST,npoint);
			v=BufferXYZWD.Lock();
			npoint=0;
		}
	}

	BufferXYZWD.Unlock(npoint);
	if(npoint)
		BufferXYZWD.DrawPrimitive(PT_POINTLIST,npoint);

	points.clear();
}

void cD3DRender::FlushLine()
{
	if(lines.empty())return;
	SetNoMaterial(ALPHA_BLEND,MatXf::ID);

	VISASSERT((lines.size()&1)==0);
	int npoint=0;
	sVertexXYZWD* v=BufferXYZWD.Lock();
	vector<PointStruct>::iterator it;
	FOR_EACH(lines,it)
	{
		PointStruct& p=*it;
		sVertexXYZWD& cv=v[npoint];
		cv.x=p.x;
		cv.y=p.y;
		cv.z=0.001f;
		cv.w=0.001f;
		cv.diffuse=p.diffuse;

		npoint++;
		if(((npoint&1)==0) && npoint>=BufferXYZWD.GetSize()-2)
		{
			BufferXYZWD.Unlock(npoint);
			BufferXYZWD.DrawPrimitive(PT_LINELIST,npoint/2);
			v=BufferXYZWD.Lock();
			npoint=0;
		}
	}

	BufferXYZWD.Unlock(npoint);
	if(npoint)
		BufferXYZWD.DrawPrimitive(PT_LINELIST,npoint/2);

	lines.clear();
}

void cD3DRender::FlushFilledRect()
{
	if(rectangles.empty())return;
	SetNoMaterial(ALPHA_BLEND,MatXf::ID);

	int npoint=0;
	sVertexXYZWD* v=BufferXYZWD.Lock();
	vector<RectStruct>::iterator it;
	FOR_EACH(rectangles,it)
	{
		RectStruct& p=*it;

		sVertexXYZWD* pv=v+npoint;
		pv[0].x=p.x1; pv[0].y=p.y1; pv[0].z=0.001f; pv[0].w=0.001f; pv[0].diffuse=p.diffuse;
		pv[1].x=p.x1; pv[1].y=p.y2; pv[1].z=0.001f; pv[1].w=0.001f; pv[1].diffuse=p.diffuse;
		pv[2].x=p.x2; pv[2].y=p.y1; pv[2].z=0.001f; pv[2].w=0.001f; pv[2].diffuse=p.diffuse;

		pv[3].x=p.x2; pv[3].y=p.y1; pv[3].z=0.001f; pv[3].w=0.001f; pv[3].diffuse=p.diffuse;
		pv[4].x=p.x1; pv[4].y=p.y2; pv[4].z=0.001f; pv[4].w=0.001f; pv[4].diffuse=p.diffuse;
		pv[5].x=p.x2; pv[5].y=p.y2; pv[5].z=0.001f; pv[5].w=0.001f; pv[5].diffuse=p.diffuse;

		npoint+=6;
		if(npoint>=BufferXYZWD.GetSize()-6)
		{
			BufferXYZWD.Unlock(npoint);
			BufferXYZWD.DrawPrimitive(PT_TRIANGLELIST,npoint/3);
			v=BufferXYZWD.Lock();
			npoint=0;
		}
	}

	BufferXYZWD.Unlock(npoint);
	if(npoint)
		BufferXYZWD.DrawPrimitive(PT_TRIANGLELIST,npoint/3);

	rectangles.clear();
}
void cD3DRender::DrawSprite(int x1,int y1,int dx,int dy,float u1,float v1,float du,float dv,
		cTexture *Texture,const sColor4c &ColorMul,float phase,eBlendMode mode, float saturate)
{
    VISASSERT(bActiveScene);
	int x2=x1+dx,y2=y1+dy;
	if(dx>=0) { if(x2<xScrMin||x1>xScrMax) return; }
	else if(x1<xScrMin||x2>xScrMax) return;
	if(dy>=0) { if(y2<yScrMin||y1>yScrMax) return; }
	else if(y1<yScrMin||y2>yScrMax) return;

	bool alpha=ColorMul.a<255 || (Texture && Texture->IsAlpha());
	if(mode<=ALPHA_TEST && alpha)
		mode=ALPHA_BLEND;
	SetNoMaterial(mode,MatXf::ID,phase,Texture);
	if(saturate<1.f-FLT_EPS)
		psMonochrome->Select(1.f-saturate);

	DrawQuad(x1,y1,dx,dy,u1,v1,du,dv,ColorMul);
}
void cD3DRender::DrawSpriteSolid(int x1,int y1,int dx,int dy,float u1,float v1,float du,float dv,
		cTexture *Texture,const sColor4c &ColorMul,float phase,eBlendMode mode)
{
	VISASSERT(bActiveScene);
	int x2=x1+dx,y2=y1+dy;
	if(dx>=0) { if(x2<xScrMin||x1>xScrMax) return; }
	else if(x1<xScrMin||x2>xScrMax) return;
	if(dy>=0) { if(y2<yScrMin||y1>yScrMax) return; }
	else if(y1<yScrMin||y2>yScrMax) return;

	bool alpha=ColorMul.a<255 || (Texture && Texture->IsAlpha());
	if(mode<=ALPHA_TEST && alpha)
		mode=ALPHA_BLEND;
	SetNoMaterial(mode,MatXf::ID,phase,Texture);
	psSolidColor->Select();

	DrawQuad(x1,y1,dx,dy,u1,v1,du,dv,ColorMul);
}

void cD3DRender::DrawQuad(float x1, float y1, float dx, float dy, float u1, float v1, float du, float dv, sColor4c color)
{
	int x2=x1+dx,y2=y1+dy;
	sVertexXYZWDT1* v=BufferXYZWDT1.Lock(4);
	const float w=0.001f;
	float xi=-0.5f+(float)x1,yi=-0.5f+(float)y1,
		  xa=-0.5f+(float)x2,ya=-0.5f+(float)y2; 

	v[0].x=xi;v[0].y=yi;v[0].z=v[0].w=w;v[0].diffuse=color;v[0].u1()=u1;v[0].v1()=v1;
	v[1].x=xi;v[1].y=ya;v[1].z=v[1].w=w;v[1].diffuse=color;v[1].u1()=u1;v[1].v1()=v1+dv;
	v[2].x=xa;v[2].y=yi;v[2].z=v[2].w=w;v[2].diffuse=color;v[2].u1()=u1+du;v[2].v1()=v1;
	v[3].x=xa;v[3].y=ya;v[3].z=v[3].w=w;v[3].diffuse=color;v[3].u1()=u1+du;v[3].v1()=v1+dv;

	BufferXYZWDT1.Unlock(4);
	BufferXYZWDT1.DrawPrimitive(PT_TRIANGLESTRIP,2);
}

void cD3DRender::DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Tex1,cTexture *Tex2,const sColor4c &ColorMul,float phase)
{
	DrawSprite2(x,y,dx,dy,u,v,du,dv,u,v,du,dv,
		Tex1,Tex2,ColorMul,phase);
}

void cD3DRender::DrawSprite2(int x1,int y1,int dx,int dy,
							 float u0,float v0,float du0,float dv0,
							 float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,const sColor4c &ColorMul,float phase,
		eColorMode mode,eBlendMode blend_mode)
{
    VISASSERT(bActiveScene);
	int x2=x1+dx,y2=y1+dy;
	if(dx>=0) { if(x2<xScrMin||x1>xScrMax) return; }
	else if(x1<xScrMin||x2>xScrMax) return;
	if(dy>=0) { if(y2<yScrMin||y1>yScrMax) return; }
	else if(y1<yScrMin||y2>yScrMax) return;

	bool alpha=ColorMul.a<255;
	if(blend_mode==ALPHA_NONE)
		SetNoMaterial(alpha?ALPHA_BLEND:ALPHA_NONE,MatXf::ID,phase,Tex1,Tex2,mode);
	else
		SetNoMaterial(blend_mode,MatXf::ID,phase,Tex1,Tex2,mode);//FO

	sVertexXYZWDT2* v=BufferXYZWDT2.Lock(4);
	v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=ColorMul;
	v[0].x=v[1].x=-0.5f+(float)x1; v[0].y=v[2].y=-0.5f+(float)y1; 
	v[3].x=v[2].x=-0.5f+(float)x2; v[1].y=v[3].y=-0.5f+(float)y2; 

	v[0].u1()=u0;     v[0].v1()=v0;
	v[1].u1()=u0;     v[1].v1()=v0+dv0;
	v[2].u1()=u0+du0; v[2].v1()=v0;
	v[3].u1()=u0+du0; v[3].v1()=v0+dv0;

	v[0].u2()=u1;     v[0].v2()=v1;
	v[1].u2()=u1;     v[1].v2()=v1+dv1;
	v[2].u2()=u1+du1; v[2].v2()=v1;
	v[3].u2()=u1+du1; v[3].v2()=v1+dv1;
	BufferXYZWDT2.Unlock(4);
	
	BufferXYZWDT2.DrawPrimitive(PT_TRIANGLESTRIP,2);
}

void cD3DRender::DrawSprite2(int x1,int y1,int dx,int dy,
							 float u0,float v0,float du0,float dv0,
							 float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,float lerp_factor,float alpha,float phase,
		eColorMode mode,eBlendMode blend_mode)
{
	VISASSERT(bActiveScene);
	int x2=x1+dx,y2=y1+dy;
	if(dx>=0) { if(x2<xScrMin||x1>xScrMax) return; }
	else if(x1<xScrMin||x2>xScrMax) return;
	if(dy>=0) { if(y2<yScrMin||y1>yScrMax) return; }
	else if(y1<yScrMin||y2>yScrMax) return;

	sColor4c ColorMul(sColor4f(1,1,1,alpha));
	if(blend_mode==ALPHA_NONE)
		SetNoMaterial((ColorMul.a<255)?ALPHA_BLEND:ALPHA_NONE,MatXf::ID,phase,Tex2,Tex1,mode);
	else
		SetNoMaterial(blend_mode,MatXf::ID,phase,Tex2,Tex1,mode);//FIXME

	DWORD index1=GetTextureStageState(1,D3DTSS_TEXCOORDINDEX);
	SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
	SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,1);

	sColor4c lerp(round(255.0f*lerp_factor),
				  round(255.0f*lerp_factor),
				  round(255.0f*lerp_factor),
				  round(255.0f*(1.0f-lerp_factor)));

	{
		SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATECOLOR_ADDALPHA);
		SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TFACTOR);
		SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);
		SetRenderState(D3DRS_TEXTUREFACTOR,lerp.RGBA());

		SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
		SetTextureStageState(1,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);

//		SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);
		SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEXTURE);
		SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT);
	}

	sVertexXYZWDT2* v=BufferXYZWDT2.Lock(4);
	v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=ColorMul;
	v[0].x=v[1].x=-0.5f+(float)x1; v[0].y=v[2].y=-0.5f+(float)y1; 
	v[3].x=v[2].x=-0.5f+(float)x2; v[1].y=v[3].y=-0.5f+(float)y2; 

	v[0].u1()=u1;     v[0].v1()=v1;
	v[1].u1()=u1;     v[1].v1()=v1+dv1;
	v[2].u1()=u1+du1; v[2].v1()=v1;
	v[3].u1()=u1+du1; v[3].v1()=v1+dv1;

	v[0].u2()=u0;     v[0].v2()=v0;
	v[1].u2()=u0;     v[1].v2()=v0+dv0;
	v[2].u2()=u0+du0; v[2].v2()=v0;
	v[3].u2()=u0+du0; v[3].v2()=v0+dv0;
	BufferXYZWDT2.Unlock(4);
	
	BufferXYZWDT2.DrawPrimitive(PT_TRIANGLESTRIP,2);

	SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
	SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,index1);

	SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );

	SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );

}


void cD3DRender::OutText(int x,int y,const char *string,int r,int g,int b,char *FontName,int size,int bold,int italic,int underline)
{
	HDC hDC=0;
    HFONT hFont=CreateFont(size,0,0,0,bold?FW_BOLD:FW_NORMAL,italic,underline,0, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH,FontName);
	if(hFont==0) return;
	if(hWnd==0) return;

	if((hDC=GetDC(hWnd))==0) return;
    HFONT hFontOld=(HFONT)SelectObject( hDC, hFont );
    SetTextColor(hDC,RGB(r,g,b));
	SetBkMode(hDC,TRANSPARENT);
	RECT rect={xScrMin,yScrMin,xScrMax,yScrMax};
	RDCALL(!ExtTextOut(hDC,x,y,ETO_CLIPPED,&rect,string,lstrlen(string),0));
	SelectObject(hDC,hFontOld);
	DeleteObject(hFont);
	ReleaseDC(hWnd,hDC);

}

void cD3DRender::OutText(int x,int y,const char *string,int r,int g,int b)
{
	if(hWnd==0) return;
	HDC hDC=GetDC(hWnd);
	if(hDC==0) return;
	SetTextColor(hDC,RGB(r,g,b));
	SetBkMode(hDC,TRANSPARENT);
	RECT rect={xScrMin,yScrMin,xScrMax,yScrMax};
	RDCALL(!ExtTextOut(hDC,x,y,ETO_CLIPPED,&rect,string,lstrlen(string),0));
	ReleaseDC(hWnd,hDC);
}

int cD3DRender::SetGamma(float fGamma,float fStart,float fFinish)
{
	D3DGAMMARAMP gamma;
	fGamma=max(fGamma,0.05f);
	fGamma=1/fGamma;
    for(int i=0;i<256;i++)
    {
		int dwGamma=round(65536*(fStart+(fFinish-fStart)*pow(i/255.f,fGamma)));
		if(dwGamma<0) dwGamma=0; else if(dwGamma>65535) dwGamma=65535;
        gamma.red[i]=gamma.green[i]=gamma.blue[i]=dwGamma;
    }
	lpD3DDevice->SetGammaRamp(0,D3DSGR_NO_CALIBRATION,&gamma);
	return 0;
}

void cD3DRender::CreateVertexBuffer(sPtrVertexBuffer &vb,int NumberVertex, IDirect3DVertexDeclaration9* declaration,int dynamic)
{
	int size = GetSizeFromDeclaration(declaration);
	CreateVertexBufferBySizeFormat(vb,NumberVertex,size,declaration,dynamic);
}

void cD3DRender::CreateVertexBufferBySize(struct sPtrVertexBuffer &vb,int NumberVertex,int size,int dynamic)
{
	CreateVertexBufferBySizeFormat(vb,NumberVertex,size,0,dynamic);
}

void cD3DRender::CreateVertexBufferBySizeFormat(struct sPtrVertexBuffer &vb,int NumberVertex,int size,IDirect3DVertexDeclaration9* declaration,int dynamic)
{
	xassert(NumberVertex>=0 || NumberVertex<=65536);
	vb.ptr=LibVB.NewSlot();

	sSlotVB& s=*vb.ptr;
	s.VertexSize=size;
	s.declaration = declaration;
	s.dynamic = dynamic;
	s.NumberVertex=NumberVertex;
	s.p=NULL;
	IDirect3DVertexBuffer9* buffer=NULL;
	if(dynamic)
		RDCALL(lpD3DDevice->CreateVertexBuffer(NumberVertex*vb.GetVertexSize(),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&buffer,NULL))
	else
	{
		//RDCALL(lpD3DDevice->CreateVertexBuffer(NumberVertex*vb.GetVertexSize(),D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,&buffer,NULL))
		RDCALL(lpD3DDevice->CreateVertexBuffer(NumberVertex*vb.GetVertexSize(),0,0,D3DPOOL_MANAGED,&buffer,NULL))
	}

	s.p=CREATEIDirect3DVertexBufferProxy(buffer);
}

void cD3DRender::DeleteVertexBuffer(sPtrVertexBuffer &vb)
{
	MTG();
	if(!vb.IsInit()) return;
	sSlotVB& s=*vb.ptr;
	xassert(s.init>0);
	s.init--;
	if(s.init==0)
	{
		RELEASE(s.p); 
		LibVB.DeleteSlot(vb.ptr);
	}
	vb.ptr=NULL;
}

void cD3DRender::DeleteDynamicVertexBuffer()
{
	for(int i=0;i<LibVB.size();i++)
	{
		sSlotVB& s=*LibVB[i];
		if(s.init && s.dynamic)
		{
			RELEASE(s.p);
		}
	}
}

void cD3DRender::RestoreDynamicVertexBuffer()
{
	for(int i=0;i<LibVB.size();i++)
	{
		sSlotVB& s=*LibVB[i];
		if(s.init && s.dynamic)
		{
			VISASSERT(s.p==NULL);
			IDirect3DVertexBuffer9* buffer=NULL;
			RDCALL(lpD3DDevice->CreateVertexBuffer(s.NumberVertex*s.VertexSize,D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&buffer,NULL));
			s.p=CREATEIDirect3DVertexBufferProxy(buffer);
		}
	}
}

void* cD3DRender::LockVertexBuffer(sPtrVertexBuffer &vb,bool readonly)
{
	void *p=0;
	VISASSERT( vb.IsInit() && vb.ptr->p );
	RDCALL(vb.ptr->p->Lock(0,0,&p, (vb.ptr->dynamic ? D3DLOCK_NOSYSLOCK|D3DLOCK_DISCARD : 0)| (readonly?D3DLOCK_READONLY:0) ));
	return p;
}
void cD3DRender::UnlockVertexBuffer(sPtrVertexBuffer &vb)
{
	VISASSERT( vb.IsInit() && vb.ptr->p );
	RDCALL(vb.ptr->p->Unlock());
}
void cD3DRender::CreateIndexBuffer(sPtrIndexBuffer& ib,int NumberPolygon,int size)
{
	DeleteIndexBuffer(ib);

	sSlotIB* slot=LibIB.NewSlot();
	IDirect3DIndexBuffer9* buffer=NULL;
	RDCALL(lpD3DDevice->CreateIndexBuffer(NumberPolygon*size,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&buffer,NULL));
	slot->p=CREATEIDirect3DIndexBufferProxy(buffer);
	ib.ptr=slot;
	slot->NumberPolygon = NumberPolygon;
	slot->PolygonSize = size;
}
void cD3DRender::DeleteIndexBuffer(sPtrIndexBuffer &ib)
{
	if(!ib.IsInit()) return;
	sSlotIB& s=*ib.ptr;
	xassert(s.init>0);
	s.init--;
	if(s.init==0)
	{
		RELEASE(s.p);
		LibIB.DeleteSlot(ib.ptr);
	}
	ib.ptr=NULL;
}
sPolygon* cD3DRender::LockIndexBuffer(sPtrIndexBuffer &ib,bool readonly)
{
	void *p=0;
	VISASSERT( ib.IsInit() && ib.ptr->p );
	ib.ptr->p->Lock(0,0,&p,readonly?D3DLOCK_READONLY:0);
	return (sPolygon*)p;
}
void cD3DRender::UnlockIndexBuffer(sPtrIndexBuffer &ib)
{
	VISASSERT( ib.IsInit() && ib.ptr->p );
	ib.ptr->p->Unlock();
}

void cD3DRender::SetGlobalFog(const sColor4f &color,const Vect2f &v_)
{ 
	Vect2f v=v_;
	if(v.x<0 || v.y<0)
	{
		SetRenderState(RS_FOGENABLE,FALSE);
		vertex_fog_param.x=0;
		vertex_fog_param.y=1;
		vertex_fog_param.z=0;
		return;
	}

	SetRenderState(RS_FOGENABLE,TRUE);
	SetRenderState(D3DRS_FOGCOLOR,color.GetRGB());
	SetRenderState(D3DRS_FOGSTART,F2DW(v.x));
	SetRenderState(D3DRS_FOGEND,F2DW(v.y));

	const float min_size=1;
	VISASSERT(v.x+min_size<v.y);
	if(!(v.x+min_size<v.y))
		v.y=v.x+min_size;

	vertex_fog_param.x=v.x;
	vertex_fog_param.y=v.y/(v.y-v.x);
	vertex_fog_param.z=-1/(v.y-v.x);
}
int cD3DRender::KillFocus()
{
	if(lpD3DDevice==0) return 1;

	{
		vector<cOcclusionQuery*>::iterator it;
		FOR_EACH(occlusion_query,it)
			(*it)->Done();
		DeleteQueryEndFrame();
	}

	RELEASE(lpZBuffer);
	RELEASE(lpBackBuffer);
	if(dtFixed)dtFixed->DeleteShadowTexture();
	if(dtAdvance)dtAdvance->DeleteShadowTexture();
	if(dtAdvanceOriginal)
		dtAdvanceOriginal->DeleteShadowTexture();

	TexLibrary.DeleteDefaultTextures();

//	IsDeleteAllDefaultTextures();
	list<cDeleteDefaultResource*>::iterator itd;
	FOR_EACH(delete_default_resource,itd)
		(*itd)->DeleteDefaultResource();

	DeleteDynamicVertexBuffer();
	DeleteShader();
		
	ClearTilemapPool();
	vertex_pool_manager.Clear();
	index_pool_manager.Clear();
	return 0;
}

bool cD3DRender::SetFocus(bool wait,bool focus_error)
{
	{
		HRESULT hres=D3D_OK;
		int imax=wait?10:1;
		for(int i=0;i<imax;i++)
		{
			hres=lpD3DDevice->Reset(&d3dpp);
			if(hres==D3D_OK)
				break;
//			dprintf("Sleep(1000)wait reset device\n");
			Sleep(1000);
		}
		if(hres!=D3D_OK)
		{
			if(focus_error)
			{
				VisError<<"Internal error. Cannot reinitialize graphics"<<VERR_END;
			}
			return false;
		}
	}

	RDCALL(lpD3DDevice->EvictManagedResources());
	 
	//int i;
	//for(i=0;i<TexLibrary.GetNumberTexture();i++)
	//{
	//	cTexture* pTexture=TexLibrary.GetTexture(i);
	//	if(pTexture && pTexture->GetAttribute(TEXTURE_D3DPOOL_DEFAULT))
	//	{
	//		xassert(0);
	//		if(pTexture->GetRef()>1)
	//			CreateTexture(pTexture,0,-1,-1);
	//	}
	//}

	//for(i=0;i<TexLibrary.default_texture.size();i++)
	//{
	//	cTexture* pTexture=TexLibrary.default_texture[i];
	//	xassert(pTexture->GetAttribute(TEXTURE_D3DPOOL_DEFAULT));
	//	CreateTexture(pTexture,0,-1,-1);
	//}

	TexLibrary.CreateDefaultTextures();

	list<cDeleteDefaultResource*>::iterator itd;
	FOR_EACH(delete_default_resource,itd)
		(*itd)->RestoreDefaultResource();

	RELEASE(lpZBuffer);
	RELEASE(lpBackBuffer);

	RDCALL(lpD3DDevice->GetDepthStencilSurface(&lpZBuffer));
	RDCALL(lpD3DDevice->GetRenderTarget(0,&lpBackBuffer)); 

	RestoreShader();
	RestoreDynamicVertexBuffer();
	RestoreTilemapPool();

	ReinitOcclusion();
	CreateQueryEndFrame();
	return true;
}

void cD3DRender::DeleteShader()
{
	vector<cShader*>::iterator it;
	FOR_EACH(cShader::all_shader,it)
		(*it)->Delete();
}

void cD3DRender::RestoreShader()
{
	flag_restore_shader=true;
}

void cD3DRender::RestoreShaderReal()
{
	if(!flag_restore_shader)
		return;
	flag_restore_shader=false;
	vector<cShader*>::iterator it;
	FOR_EACH(cShader::all_shader,it)
		(*it)->Restore();
}

void cD3DRender::SetBlendState(eBlendMode blend)
{
	if(blend==ALPHA_BLEND_INVALPHA)
	{
		SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
		SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
	}else
	{
		SetRenderState(D3DRS_ALPHATESTENABLE,blend!=ALPHA_NONE);
		SetRenderState(D3DRS_ALPHABLENDENABLE,blend>ALPHA_TEST);
	}

	switch(blend)
	{
	case ALPHA_SUBBLEND:
		SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_REVSUBTRACT);
		break;
	case ALPHA_ADDBLENDALPHA:
		SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		break;
	case ALPHA_ADDBLEND:
		SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
		SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		break;
	case ALPHA_BLEND:
		SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);

		//ot.rgb*=ot.a;
		//SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
		//SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
		//SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_MAX);
		break;
	case ALPHA_MUL:
		SetRenderState(D3DRS_SRCBLEND,D3DBLEND_DESTCOLOR);
		SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
		SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		break;
	case ALPHA_BLEND_INVALPHA:
		SetRenderState(D3DRS_SRCBLEND,D3DBLEND_INVSRCALPHA);
		SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCALPHA);
		SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		break;
	}
}

void cD3DRender::SetBlendStateAlphaRef(eBlendMode blend)
{
	SetRenderState(D3DRS_ALPHAREF,blend==ALPHA_TEST?BLEND_STATE_ALPHA_REF:0);
	SetBlendState(blend);
}


void cD3DRender::SetNoMaterial(eBlendMode blend,const MatXf& mat,float Phase,cTexture *Texture0,
							   cTexture *Texture1,eColorMode color_mode)
{
	SetVertexShader(NULL);
	SetPixelShader(NULL);
	SetTexturePhase(0,Texture0,Phase);
	SetTexturePhase(1,Texture1,Phase);
	SetTextureBase(2,NULL);

	if(Texture0)
	{
		if(blend==ALPHA_NONE && Texture0->IsAlphaTest())
			blend=ALPHA_TEST;
		if(!(blend>ALPHA_TEST) && Texture0->IsAlpha())
			blend=ALPHA_BLEND;
	}
	SetBlendStateAlphaRef(blend);
	
	SetRenderState(D3DRS_SPECULARENABLE,FALSE);
	SetRenderState(D3DRS_NORMALIZENORMALS,FALSE);


	SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );

	SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );

	SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	if(blend>ALPHA_NONE)
	{
		if(blend==ALPHA_BLEND_NOTUSETEXTUREALPHA)
			SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
		else
			SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	}else
		SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	if(Texture1)
	{
		SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1 ),
		SetTextureStageState( 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );

		switch(color_mode)
		{
		case COLOR_ADD:
			SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_ADD);
			break;
		case COLOR_MOD2:
			SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE2X );
			break;
		case COLOR_MOD4:
			SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE4X );
			break;
		case COLOR_MOD:
			SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
			break;
		default:
			VISASSERT(0);
			break;
		}
	}
	else 
		SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
	SetWorldMatrix(mat);
}

void cD3DRender::CreateVertexDeclarations(LPDIRECT3DDEVICE9 pDevice)
{
	for(int i = 0; i < vertexDeclarations().size(); ++i)
		RDCALL(pDevice->CreateVertexDeclaration(vertexDeclarations()[i].second, vertexDeclarations()[i].first));
}

void cD3DRender::ReleaseVertexDeclarations()
{
	for(int i = 0; i < vertexDeclarations().size(); ++i){
		if(vertexDeclarations()[i].first && *vertexDeclarations()[i].first){
			(*vertexDeclarations()[i].first)->Release();
			(*vertexDeclarations()[i].first) = 0;
		}
	}
}

void cD3DRender::InitVertexBuffers()
{
	const int size=8192;
	const int sizemin=1024;
	BufferXYZDT1.CreateVertexNum(size);
	BufferXYZDT2.CreateVertexNum(sizemin);
	BufferXYZWD.CreateVertexNum(sizemin);
	BufferXYZD.CreateVertexNum(sizemin);
	QuadBufferXYZDT1.Create();
	QuadBufferXYZWDT1.Create();
	QuadBufferXYZWDT2.Create();
	QuadBufferXYZWDT3.Create();

	BufferXYZWDT1.CreateVertexNum(sizemin);
	BufferXYZWDT2.CreateVertexNum(sizemin);
	BufferXYZWDT3.CreateVertexNum(sizemin);
	BufferXYZOcclusion.CreateVertexNum(sizemin);
}
////////////////////////////////////////////////////////////

void sPtrVertexBuffer::Destroy()
{
	if(ptr)
		gb_RenderDevice->DeleteVertexBuffer(*this);
	ptr=NULL;
}

sPtrIndexBuffer::~sPtrIndexBuffer()
{
	if(ptr)
		gb_RenderDevice->DeleteIndexBuffer(*this);
	ptr=NULL;
}

void sPtrVertexBuffer::CopyAddRef(const sPtrVertexBuffer& from)
{
	xassert(ptr==0);
	ptr=from.ptr;
	ptr->init++;
}

void sPtrIndexBuffer::CopyAddRef(const sPtrIndexBuffer& from)
{
	xassert(ptr==0);
	ptr=from.ptr;
	ptr->init++;
}

////////////////////////////////////////////////////////////
cVertexBufferInternal::cVertexBufferInternal()
{
	MTG();
	numvertex=0;
	cur_min_vertex=0;
	start_vertex=0;
}

void cVertexBufferInternal::Destroy()
{
	MTG();
	vb.Destroy();
}

cVertexBufferInternal::~cVertexBufferInternal()
{
}

void cVertexBufferInternal::Rewind()
{
	if(cur_min_vertex)
	{
		IDirect3DVertexBufferProxy* pvb=gb_RenderDevice3D->GetVB(vb);
		void* min_index=NULL;
		RDCALL(pvb->Lock(0,0,&min_index,D3DLOCK_DISCARD));
		RDCALL(pvb->Unlock());
	}
	cur_min_vertex=0;
	start_vertex=0;
}


BYTE* cVertexBufferInternal::Lock(int minvertex)
{
	MTG();
	void* min_index=NULL;

	IDirect3DVertexBufferProxy* pvb=gb_RenderDevice3D->GetVB(vb);
	if(GetSize()>minvertex)
	{
		RDCALL(pvb->Lock(cur_min_vertex*vb.GetVertexSize(),GetSize()*vb.GetVertexSize(),
			&min_index,D3DLOCK_NOOVERWRITE));
	}else
	{
		RDCALL(pvb->Lock(0,0,&min_index,D3DLOCK_DISCARD));
		cur_min_vertex=0;
	}

	return (BYTE*)min_index;
}

void cVertexBufferInternal::Unlock(int num_write_vertex)
{
	MTG();
	cD3DRender* rd=gb_RenderDevice3D;
	RDCALL(rd->GetVB(vb)->Unlock());

	start_vertex=cur_min_vertex;
	cur_min_vertex+=num_write_vertex;
	VISASSERT(cur_min_vertex<=numvertex);
}

void cVertexBufferInternal::Create(int bytesize,int vertexsize, IDirect3DVertexDeclaration9* _declaration)
{
	MTG();
	numvertex=bytesize/vertexsize;
	declaration=_declaration;
	gb_RenderDevice3D->CreateVertexBuffer(vb,numvertex, declaration,TRUE);
	xassert(vertexsize==vb.GetVertexSize());
}

void cVertexBufferInternal::DrawPrimitive(PRIMITIVETYPE Type,UINT Count)
{
	MTG();
	cD3DRender* rd=gb_RenderDevice3D;
	rd->SetVertexDeclaration(declaration);
	rd->SetStreamSource(vb);
	RDCALL(rd->lpD3DDevice->DrawPrimitive((D3DPRIMITIVETYPE)Type,start_vertex,Count));
	*rd->PtrNumberPolygon+=Count;
}

void cVertexBufferInternal::DrawIndexedPrimitive(UINT Count)
{
	MTG();
	cD3DRender* rd=gb_RenderDevice3D;
	rd->SetVertexDeclaration(declaration);
	rd->SetStreamSource(vb);
	rd->SetIndices(rd->GetStandartIB());
	RDCALL(rd->lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
		0, start_vertex, Count*2, (start_vertex>>2)*6, Count ));
	*rd->PtrNumberPolygon+=Count;
}

/////////////////////////cQuadBufferInternal///////////////////////////////

cQuadBufferInternal::cQuadBufferInternal()
{
	start_vertex=NULL;
}

cQuadBufferInternal::~cQuadBufferInternal()
{
}

void cQuadBufferInternal::Destroy()
{
	cVertexBufferInternal::Destroy();
}

void cQuadBufferInternal::Create(int vertexsize, IDirect3DVertexDeclaration9* declaration)
{
	cVertexBufferInternal::Create(vertexsize*POLYGONMAX*2,vertexsize,declaration);
}

void cQuadBufferInternal::SetMatrix(const MatXf &m)
{
	gb_RenderDevice3D->SetMatrix(D3DTS_WORLD,m);
}

void cQuadBufferInternal::BeginDraw()
{
	VISASSERT(start_vertex==NULL);
	start_vertex=cVertexBufferInternal::Lock(4);
	vertex_index=0;
}

void* cQuadBufferInternal::Get()
{
	if(vertex_index+4>=GetSize())
	{
		EndDraw();
		BeginDraw();
	}

	BYTE* cur=start_vertex+vertex_index*vb.GetVertexSize();
	vertex_index+=4;
	return cur;
}

void cQuadBufferInternal::EndDraw()
{
	Unlock(vertex_index);
	if(vertex_index>0)
		DrawIndexedPrimitive(vertex_index>>1);
	start_vertex=NULL;
}

void DrawStrip::Begin()
{
	gb_RenderDevice3D->SetMatrix(D3DTS_WORLD,MatXf::ID);
	buf=gb_RenderDevice->GetBufferXYZDT1();
	pointer=buf->Lock(8);
	num=0;
}

void DrawStrip::End()
{
	buf->Unlock(num);
	if(num>=4)
	{
		buf->DrawPrimitive(PT_TRIANGLESTRIP,num-2);
	}
}

void DrawStripT2::Begin(const MatXf& mat)
{
	gb_RenderDevice3D->SetMatrix(D3DTS_WORLD,mat);
	buf=gb_RenderDevice->GetBufferXYZDT2();
	pointer=buf->Lock(8);
	num=0;
}

void DrawStripT2::End()
{
	buf->Unlock(num);
	if(num>=4)
	{
		buf->DrawPrimitive(PT_TRIANGLESTRIP,num-2);
	}
}

void cD3DRender::InitStandartIB()
{
	CreateIndexBuffer(standart_ib,POLYGONMAX);
	sPolygon* polygon=LockIndexBuffer(standart_ib);

	for(int nPolygon=0; nPolygon<POLYGONMAX; nPolygon+=2 )
	{
		polygon[nPolygon+0].set(2*nPolygon+0,2*nPolygon+1,2*nPolygon+2);
		polygon[nPolygon+1].set(2*nPolygon+2,2*nPolygon+1,2*nPolygon+3);
	}

	UnlockIndexBuffer(standart_ib);
}


void cD3DRender::SaveStates(const char* fname)
{
	FILE* f=fopen(fname,"wt");
	fprintf(f,"Render state\n");
#define W(s) {DWORD d;RDCALL(lpD3DDevice->GetRenderState(s,&d));fprintf(f,"%s=%x\n",#s,d); }
	W(D3DRS_ZENABLE);
    W(D3DRS_FILLMODE);
    W(D3DRS_SHADEMODE);
    W(D3DRS_ZWRITEENABLE);
    W(D3DRS_ALPHATESTENABLE);
    W(D3DRS_LASTPIXEL);
    W(D3DRS_SRCBLEND);
    W(D3DRS_DESTBLEND);
    W(D3DRS_CULLMODE);
    W(D3DRS_ZFUNC);
    W(D3DRS_ALPHAREF);
    W(D3DRS_ALPHAFUNC);
    W(D3DRS_DITHERENABLE);
    W(D3DRS_ALPHABLENDENABLE);
    W(D3DRS_FOGENABLE);
    W(D3DRS_SPECULARENABLE);
    W(D3DRS_FOGCOLOR);
    W(D3DRS_FOGTABLEMODE);
    W(D3DRS_FOGSTART);
    W(D3DRS_FOGEND);
    W(D3DRS_FOGDENSITY);
    W(D3DRS_RANGEFOGENABLE);
    W(D3DRS_STENCILENABLE);
    W(D3DRS_STENCILFAIL);
    W(D3DRS_STENCILZFAIL);
    W(D3DRS_STENCILPASS);
    W(D3DRS_STENCILFUNC);
    W(D3DRS_STENCILREF);
    W(D3DRS_STENCILMASK);
    W(D3DRS_STENCILWRITEMASK);
    W(D3DRS_TEXTUREFACTOR);
    W(D3DRS_WRAP0);
    W(D3DRS_WRAP1);
    W(D3DRS_WRAP2);
    W(D3DRS_WRAP3);
    W(D3DRS_WRAP4);
    W(D3DRS_WRAP5);
    W(D3DRS_WRAP6);
    W(D3DRS_WRAP7);
    W(D3DRS_CLIPPING);
    W(D3DRS_LIGHTING);
    W(D3DRS_AMBIENT);
    W(D3DRS_FOGVERTEXMODE);
    W(D3DRS_COLORVERTEX);
    W(D3DRS_LOCALVIEWER);
    W(D3DRS_NORMALIZENORMALS);
    W(D3DRS_DIFFUSEMATERIALSOURCE);
    W(D3DRS_SPECULARMATERIALSOURCE);
    W(D3DRS_AMBIENTMATERIALSOURCE);
    W(D3DRS_EMISSIVEMATERIALSOURCE);
    W(D3DRS_VERTEXBLEND);
    W(D3DRS_CLIPPLANEENABLE);
    W(D3DRS_POINTSIZE);
    W(D3DRS_POINTSIZE_MIN);
    W(D3DRS_POINTSPRITEENABLE);
    W(D3DRS_POINTSCALEENABLE);
    W(D3DRS_POINTSCALE_A);
    W(D3DRS_POINTSCALE_B);
    W(D3DRS_POINTSCALE_C);
    W(D3DRS_MULTISAMPLEANTIALIAS);
    W(D3DRS_MULTISAMPLEMASK);
    W(D3DRS_PATCHEDGESTYLE);
    W(D3DRS_DEBUGMONITORTOKEN);
    W(D3DRS_POINTSIZE_MAX);
    W(D3DRS_INDEXEDVERTEXBLENDENABLE);
    W(D3DRS_COLORWRITEENABLE);
    W(D3DRS_TWEENFACTOR);
    W(D3DRS_BLENDOP);
    W(D3DRS_POSITIONDEGREE);
    W(D3DRS_NORMALDEGREE);
    W(D3DRS_SCISSORTESTENABLE);
    W(D3DRS_SLOPESCALEDEPTHBIAS);
    W(D3DRS_ANTIALIASEDLINEENABLE);
    W(D3DRS_MINTESSELLATIONLEVEL);
    W(D3DRS_MAXTESSELLATIONLEVEL);
    W(D3DRS_ADAPTIVETESS_X);
    W(D3DRS_ADAPTIVETESS_Y);
    W(D3DRS_ADAPTIVETESS_Z);
    W(D3DRS_ADAPTIVETESS_W);
//    W(D3DRS_ENABLEADAPTIVETESSELATION);
    W(D3DRS_TWOSIDEDSTENCILMODE);
    W(D3DRS_CCW_STENCILFAIL);
    W(D3DRS_CCW_STENCILZFAIL);
    W(D3DRS_CCW_STENCILPASS);
    W(D3DRS_CCW_STENCILFUNC);
    W(D3DRS_COLORWRITEENABLE1);
    W(D3DRS_COLORWRITEENABLE2);
    W(D3DRS_COLORWRITEENABLE3);
    W(D3DRS_BLENDFACTOR);
    W(D3DRS_SRGBWRITEENABLE);
    W(D3DRS_DEPTHBIAS);
    W(D3DRS_WRAP8);
    W(D3DRS_WRAP9);
    W(D3DRS_WRAP10);
    W(D3DRS_WRAP11);
    W(D3DRS_WRAP12);
    W(D3DRS_WRAP13);
    W(D3DRS_WRAP14);
    W(D3DRS_WRAP15);
    W(D3DRS_SEPARATEALPHABLENDENABLE);
    W(D3DRS_SRCBLENDALPHA);
    W(D3DRS_DESTBLENDALPHA);
    W(D3DRS_BLENDOPALPHA);
#undef W

	fprintf(f,"\nSampler state\n");
#define W(i,s) {DWORD d;RDCALL(lpD3DDevice->GetSamplerState(i,s,&d));fprintf(f,"[%i]%s=%x\n",i,#s,d); }
	int i;
	for(i=0;i<4;i++)
	{
		W(i,D3DSAMP_ADDRESSU);
		W(i,D3DSAMP_ADDRESSV);
		W(i,D3DSAMP_ADDRESSW);
		W(i,D3DSAMP_BORDERCOLOR);
		W(i,D3DSAMP_MAGFILTER);
		W(i,D3DSAMP_MINFILTER);
		W(i,D3DSAMP_MIPFILTER);
		W(i,D3DSAMP_MIPMAPLODBIAS);
		W(i,D3DSAMP_MAXMIPLEVEL);
		W(i,D3DSAMP_MAXANISOTROPY);
		W(i,D3DSAMP_SRGBTEXTURE);
		W(i,D3DSAMP_ELEMENTINDEX);
		W(i,D3DSAMP_DMAPOFFSET);
		fprintf(f,"\n");
	}
#undef W

	fprintf(f,"\nTexture stage state\n");
#define W(i,s) {DWORD d;RDCALL(lpD3DDevice->GetTextureStageState(i,s,&d));fprintf(f,"[%i]%s=%x\n",i,#s,d); }
	for(i=0;i<4;i++)
	{
		W(i,D3DTSS_COLOROP);
		W(i,D3DTSS_COLORARG1);
		W(i,D3DTSS_COLORARG2);
		W(i,D3DTSS_ALPHAOP);
		W(i,D3DTSS_ALPHAARG1);
		W(i,D3DTSS_ALPHAARG2);
		W(i,D3DTSS_BUMPENVMAT00);
		W(i,D3DTSS_BUMPENVMAT01);
		W(i,D3DTSS_BUMPENVMAT10);
		W(i,D3DTSS_BUMPENVMAT11);
		W(i,D3DTSS_TEXCOORDINDEX);
		W(i,D3DTSS_BUMPENVLSCALE);
		W(i,D3DTSS_BUMPENVLOFFSET);
		W(i,D3DTSS_TEXTURETRANSFORMFLAGS);
		W(i,D3DTSS_COLORARG0);
		W(i,D3DTSS_ALPHAARG0);
		W(i,D3DTSS_RESULTARG);
		W(i,D3DTSS_CONSTANT);
		fprintf(f,"\n");
	}
#undef W
	fclose(f);
}


void cD3DRender::DrawSpriteScale(int x,int y,int dx,int dy,float u,float v,
	cTextureScale *Texture,const sColor4c &ColorMul,float phase,eBlendMode mode)
{
	float du,dv;
	Texture->ConvertUV(u,v);
	Texture->DXY2DUV(dx,dy,du,dv);
	DrawSprite(x,y,dx,dy,u,v,du,dv,
		Texture,ColorMul,phase,mode);
}

void cD3DRender::DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Tex1,cTextureScale *Tex2,const sColor4c &ColorMul,float phase)
{
	DrawSpriteScale2(x,y,dx,dy,u,v,u,v,
		Tex1,Tex2,ColorMul,phase);
}

void cD3DRender::DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,float u1,float v1,
		cTextureScale *Tex1,cTextureScale *Tex2,const sColor4c &ColorMul,float phase,eColorMode mode)
{
	float du,dv,du1,dv1;
	Tex1->ConvertUV(u,v);
	Tex2->ConvertUV(u1,v1);
	Tex1->DXY2DUV(dx,dy,du,dv);
	Tex2->DXY2DUV(dx,dy,du1,dv1);

	DrawSprite2(x,y,dx,dy,u,v,du,dv,u1,v1,du1,dv1,
		Tex1,Tex2,ColorMul,phase,mode);

}

bool cD3DRender::PossibleAnisotropic()
{
	D3DCAPS9 caps;
	RDCALL(lpD3DDevice->GetDeviceCaps(&caps));
	return caps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC;
}
int cD3DRender::GetMaxAnisotropicLevels()
{
	D3DCAPS9 caps;
	RDCALL(lpD3DDevice->GetDeviceCaps(&caps));
	return caps.MaxAnisotropy;
}

void cD3DRender::SetAnisotropic(int level)
{
	D3DTEXTUREFILTERTYPE texture_interpolation;
	if(level>1&&PossibleAnisotropic()&&level<=GetMaxAnisotropicLevels())
	{
		texture_interpolation=D3DTEXF_ANISOTROPIC;
		anisotropic_level = level;
	}else
	{
		texture_interpolation=D3DTEXF_LINEAR;
		anisotropic_level = 0;
	}

	sampler_clamp_anisotropic=sampler_clamp_linear;
	sampler_wrap_anisotropic=sampler_wrap_linear;

	sampler_clamp_anisotropic.magfilter=
	sampler_clamp_anisotropic.minfilter=
	sampler_clamp_anisotropic.mipfilter=
	sampler_wrap_anisotropic.magfilter=
	sampler_wrap_anisotropic.minfilter=
	sampler_wrap_anisotropic.mipfilter=texture_interpolation;
}

int cD3DRender::GetAnisotropic()
{
	return anisotropic_level;
}

bool cD3DRender::ReinitOcclusion()
{
	bool ok=true;
	vector<cOcclusionQuery*>::iterator it;
	FOR_EACH(occlusion_query,it)
	{
		bool b=(*it)->Init();
		ok = ok && b;
	}

	return ok;
}

bool cD3DRender::PossibilityOcclusion()
{
	HRESULT hr;
	IDirect3DQuery9* pQuery=NULL;
	hr=gb_RenderDevice3D->lpD3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION,&pQuery);
	RELEASE(pQuery);
	return SUCCEEDED(hr);
}

#ifndef _FINAL_VERSION_
void IsDeleteAllDefaultTextures()
{
	int num_undeleted=0,num_uncorrect=0;
	for(cCheckDelete* cur=cCheckDelete::GetDebugRoot();cur;cur=cur->GetDebugNext())
	{
		cTexture* p=dynamic_cast<cTexture*>(cur);
		if(p)
		{
			if(p->GetAttribute(TEXTURE_D3DPOOL_DEFAULT))
			{
				for(int nFrame=0;nFrame<p->GetNumberFrame();nFrame++)
				if(p->GetDDSurface(nFrame)) 
				{
					dprintf("D3DPOOL_DEFAULT size=(%i,%i)",p->GetWidth(),p->GetHeight());
					num_undeleted++;
				}
			}else
			{
				for(int nFrame=0;nFrame<p->GetNumberFrame();nFrame++)
				if(p->GetDDSurface(nFrame)) 
				{
					IDirect3DTextureProxy* surface=p->GetDDSurface(nFrame);
					D3DSURFACE_DESC desc;
					RDCALL(surface->GetLevelDesc(0,&desc));
					if(desc.Pool==D3DPOOL_DEFAULT)
					{
						xassert(0);
						dprintf("D3DPOOL_DEFAULT size=(%i,%i)",p->GetWidth(),p->GetHeight());
						num_uncorrect++;
					}
				}
			}
		}
	}

	if(num_undeleted)
		dprintf("D3DPOOL_DEFAULT not delete=%i",num_undeleted);
	if(num_uncorrect)
		dprintf("D3DPOOL_DEFAULT not correct=%i",num_uncorrect);
}
#endif 

void cD3DRender::SetDialogBoxMode(bool enable)
{
	if(RenderMode&RENDERDEVICE_MODE_WINDOW)
		return;
	if(enable)
		RenderMode|=RENDERDEVICE_MODE_ONEBACKBUFFER;
	else
		RenderMode&=~(DWORD)RENDERDEVICE_MODE_ONEBACKBUFFER;

	KillFocus();
	SetFocus(false);
}


Vect2i GetSize(IDirect3DSurface9 *pTexture)
{
	D3DSURFACE_DESC desc;
	RDCALL(pTexture->GetDesc(&desc));
	return Vect2i((int)desc.Width,(int)desc.Height);
}


cDeleteDefaultResource::cDeleteDefaultResource()
{
	gb_RenderDevice3D->delete_default_resource.push_back(this);
}

cDeleteDefaultResource::~cDeleteDefaultResource()
{
	list<cDeleteDefaultResource*>::iterator it=find(gb_RenderDevice3D->delete_default_resource.begin(),gb_RenderDevice3D->delete_default_resource.end(),this);
	if(it!=gb_RenderDevice3D->delete_default_resource.end())
	{
		gb_RenderDevice3D->delete_default_resource.erase(it);
	}else
	{
		xassert(0);
	}
}

bool cD3DRender::RecalculateDeviceSize()
{
	vector<cRenderWindow*>::iterator it;
	int sx=0,sy=0;
	FOR_EACH(all_render_window,it)
	{
		sx=max(sx,(*it)->SizeX());
		sy=max(sy,(*it)->SizeY());
	}

	bool must=false;
	if(all_render_window.size()>1)
	{
		//must=(sx>xScr || sy>yScr);
		must=(sx!=xScr || sy!=yScr);
	}else
	{
		must=(sx!=xScr || sy!=yScr);
	}

	if(must && (sx>=4 && sy>=4))
	{
		bool b=ChangeSizeInternal(sx,sy,RENDERDEVICE_MODE_WINDOW);
		xassert(b && "Cannot RecalculateDeviceSize");
		return b;
	}

	return true;
}

void cD3DRender::SetWorldMaterial(eBlendMode blend,const MatXf& mat,float Phase,cTexture *Texture0,cTexture *Texture1,eColorMode color_mode,bool useZBuffer,bool zreflection)
{
	if(Texture0)
	{
		SetTexturePhase(0,Texture0,Phase);
		if(blend==ALPHA_NONE && Texture0->IsAlphaTest())
			blend=ALPHA_TEST;
		if(!(blend>ALPHA_TEST) && Texture0->IsAlpha())
			blend=ALPHA_BLEND;
	}else
		SetTexturePhase(0,pWhiteTexture,Phase);

	SetBlendStateAlphaRef(blend);
	SetTexturePhase(1,Texture1,Phase);

	if (gb_RenderDevice3D->dtFixed->GetFloatMap()&&gb_VisGeneric->GetFloatZBufferType()&&useZBuffer)
		SetTexture(3,gb_RenderDevice3D->dtFixed->GetFloatMap());
	else
		useZBuffer=false;

//	if(gb_RenderDevice3D->GetFogOfWar())
//	{
//		gb_RenderDevice3D->SetTexture(gb_RenderDevice3D->GetFogOfWar()->GetTexture(),0,2);
//	}

	int color_operation=0;
	if(Texture1)
	{
		switch(color_mode)
		{
		case COLOR_MOD:
			color_operation=2;
			break;
		case COLOR_ADD:
			color_operation=1;
			break;
		case COLOR_MOD2:
			color_operation=3;
			break;
		case COLOR_MOD4:
			color_operation=4;
			break;
		default:
			xassert(0);
		}
	}

	psStandart->SetColorOperation(color_operation);
	vsStandart->SetFixFogAddBlend((blend==ALPHA_ADDBLENDALPHA || blend==ALPHA_ADDBLEND) && GetRenderState(D3DRS_FOGENABLE));
	vsStandart->SetColorOperation(color_operation);
	
	psStandart->EnableFloatZBuffer(useZBuffer);
	vsStandart->EnableFloatZBuffer(useZBuffer);

	psStandart->SetReflectionZ(zreflection);
	vsStandart->SetReflectionZ(zreflection);

	vsStandart->Select(mat);
	psStandart->Select();
}

void cD3DRender::DrawNumberPolygon(int x,int y)
{
	if(!(Option_DrawNumberPolygon && DefaultFont))
		return;
	SetFont(NULL);

	int height=round(GetFontHeight("A"));
	char str[256];
	sColor4f color(1,1,1,1);
	sprintf(str,"poly=%i, drawobj=%i",NumberPolygon,NumDrawObject);
	OutText(x,y,str,color);
	y+=height;

	sprintf(str,"tilepolygon=%i",NumberTilemapPolygon);
	OutText(x,y,str,color);
	y+=height;
	
	int dbg_MemVertexBuffer=0,dbg_NumVertexBuffer=0;

	int i;
	for(i=0; i<LibVB.size(); i++ )
		if( LibVB[i]->p )
		{
			D3DVERTEXBUFFER_DESC dsc;
			LibVB[i]->p->GetDesc( &dsc );
			dbg_MemVertexBuffer += dsc.Size;
			dbg_NumVertexBuffer++;
		}
	sprintf(str,"vb=%i,nvb=%i",dbg_MemVertexBuffer,dbg_NumVertexBuffer);
	OutText(x,y,str,color);
	y+=height;

	int dbg_MemIndexBuffer=0,dbg_NumIndexBuffer=0;
	for(i=0; i<LibIB.size(); i++ )
		if( LibIB[i]->p )
		{
			D3DINDEXBUFFER_DESC dsc;
			LibIB[i]->p->GetDesc( &dsc );
			dbg_MemIndexBuffer += dsc.Size;
			dbg_NumIndexBuffer++;
		}
	sprintf(str,"ib=%i,nib=%i",dbg_MemIndexBuffer,dbg_NumIndexBuffer);
	OutText(x,y,str,color);
	y+=height;

	int dbg_MemTexture=0;
	for( i=0; i<TexLibrary.GetNumberTexture(); i++ )
	{
		cTexture* pTexture=TexLibrary.GetTexture(i);
		if(pTexture)
			dbg_MemTexture +=pTexture->CalcTextureSize();
	}
	sprintf(str,"tex=%i",dbg_MemTexture);
	OutText(x,y,str,color);
	y+=height;

	dbg_MemTexture=0;
	vector<cFontInternal*>::iterator it;
	FOR_EACH(gb_VisGeneric->fonts,it)
	{
		cTexture* pTexture=(*it)->GetTexture();
		if(pTexture)
		for(int nFrame=0;nFrame<pTexture->GetNumberFrame();nFrame++)
		if( pTexture->GetDDSurface(nFrame)) 
		{	
			D3DSURFACE_DESC dsc;
			pTexture->GetDDSurface(nFrame)->GetLevelDesc( 0, &dsc );
			int sz=GetTextureFormatSize(dsc.Format);
			int size=dsc.Width*dsc.Height*sz;
			dbg_MemTexture += (size/8);
		}
	}

	sprintf(str,"font_mem=%i",dbg_MemTexture);
	OutText(x,y,str,color);
	y+=height;

	int FreeTileMemoty,TotalTileMemory;
	vertex_pool_manager.GetUsedMemory(TotalTileMemory,FreeTileMemoty);
	
	if(TotalTileMemory)
		sprintf(str,"TileMemoty=%i byte, slack=%.2f%%",TotalTileMemory,FreeTileMemoty*100.0f/TotalTileMemory);
	else
		sprintf(str,"TileMemoty=0 byte, slack=0%%");
	OutText(x,y,str,color);
	y+=height;

	{
		int total=0,free=0;
		GetTilemapTextureMemory(total,free);

		if(total)
			sprintf(str,"TileTexture=%i byte, slack=%.2f%%",total,free*100.0f/total);
		else
			sprintf(str,"TileTexture=0 byte, slack=0%%");
		OutText(x,y,str,color);
		y+=height;
	}

	index_pool_manager.GetUsedMemory(TotalTileMemory,FreeTileMemoty);
	if(TotalTileMemory)
		sprintf(str,"TileIndex=%i byte, slack=%.2f%%",TotalTileMemory,FreeTileMemoty*100.0f/TotalTileMemory);
	else
		sprintf(str,"TileIndex=0 byte, slack=0%%");
	OutText(x,y,str,color);
	y+=height;
/*
	extern void CalcNumParticleClass(int& num_object,int& num_particle);
	int num_particle_obj,num_particle;
	CalcNumParticleClass(num_particle_obj,num_particle);

	sprintf(str,"PObj=%i, Particles=%i",num_particle_obj,num_particle);
	OutText(x,y,str,color);
	y+=height;
/**/
}

void cD3DRender::RegisterVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9& declaration, D3DVERTEXELEMENT9* elements)
{
	int old_size = vertexDeclarations().size();
	vertexDeclarations().push_back(std::make_pair(&declaration, elements));
}

void cD3DRender::FillSamplerState()
{
	xassert(sizeof(SAMPLER_DATA)==8);
	for(int i=0;i<TEXTURE_MAX;i++)
	{
		SAMPLER_DATA& s=ArraytSamplerData[i];
		memset(&s,0,sizeof(s));
		s.addressu=(DX_TEXTUREADDRESS)GetSamplerState(i,D3DSAMP_ADDRESSU);
		s.addressv=(DX_TEXTUREADDRESS)GetSamplerState(i,D3DSAMP_ADDRESSV);
		s.addressw=(DX_TEXTUREADDRESS)GetSamplerState(i,D3DSAMP_ADDRESSW);

		s.minfilter=(DX_TEXTUREFILTERTYPE)GetSamplerState(i,D3DSAMP_MINFILTER);
		s.magfilter=(DX_TEXTUREFILTERTYPE)GetSamplerState(i,D3DSAMP_MAGFILTER);
		s.mipfilter=(DX_TEXTUREFILTERTYPE)GetSamplerState(i,D3DSAMP_MIPFILTER);
		s.bordercolor=GetSamplerState(i,D3DSAMP_BORDERCOLOR);
	}

}

void cD3DRender::InitSamplerConstants()
{
	memset(&sampler_clamp_linear,0,sizeof(SAMPLER_DATA));
	sampler_clamp_linear.addressu=DX_TADDRESS_CLAMP;
	sampler_clamp_linear.addressv=DX_TADDRESS_CLAMP;
	sampler_clamp_linear.addressw=DX_TADDRESS_CLAMP;

	sampler_clamp_linear.minfilter=DX_TEXF_LINEAR;
	sampler_clamp_linear.magfilter=DX_TEXF_LINEAR;
	sampler_clamp_linear.mipfilter=DX_TEXF_LINEAR;
	sampler_clamp_linear.bordercolor=0;

	sampler_wrap_linear=sampler_clamp_linear;
	sampler_wrap_linear.addressu=DX_TADDRESS_WRAP;
	sampler_wrap_linear.addressv=DX_TADDRESS_WRAP;
	sampler_wrap_linear.addressw=DX_TADDRESS_WRAP;

	sampler_wrap_point=sampler_wrap_linear;
	sampler_wrap_point.minfilter=DX_TEXF_POINT;
	sampler_wrap_point.magfilter=DX_TEXF_POINT;
	sampler_wrap_point.mipfilter=DX_TEXF_POINT;

	sampler_clamp_point=sampler_wrap_point;
	sampler_clamp_point.addressu=DX_TADDRESS_CLAMP;
	sampler_clamp_point.addressv=DX_TADDRESS_CLAMP;
	sampler_clamp_point.addressw=DX_TADDRESS_CLAMP;

	SetAnisotropic(0);
}

void cD3DRender::SetSamplerDataVirtual(DWORD stage,SAMPLER_DATA& data)
{
	SetSamplerData(stage,data);
}

void cD3DRender::SetSamplerDataReal(DWORD stage,SAMPLER_DATA& s)
{

	SetSamplerState(stage,D3DSAMP_ADDRESSU,s.addressu);
	SetSamplerState(stage,D3DSAMP_ADDRESSV,s.addressv);
	SetSamplerState(stage,D3DSAMP_ADDRESSW,s.addressw);

	SetSamplerState(stage,D3DSAMP_MINFILTER,s.minfilter);
	SetSamplerState(stage,D3DSAMP_MAGFILTER,s.magfilter);
	SetSamplerState(stage,D3DSAMP_MIPFILTER,s.mipfilter);
	SetSamplerState(stage,D3DSAMP_BORDERCOLOR,s.bordercolor);
}

void cD3DRender::CalcMultisampleNum()
{
	if(d3dpp.MultiSampleType==D3DMULTISAMPLE_NONMASKABLE)
	{
		multisample_num=1;
		xassert(0);
		return;
	}

	if(d3dpp.MultiSampleType==D3DMULTISAMPLE_NONE)
	{
		multisample_num=1;
		return;
	}

	multisample_num=(int)d3dpp.MultiSampleType;
}

int cD3DRender::GetAvailableTextureMem()
{
	return lpD3DDevice->GetAvailableTextureMem();
}


#define _MMX_FEATURE_BIT 0x00800000 
#define _SSE_FEATURE_BIT 0x02000000 
#define _SSE2_FEATURE_BIT 0x04000000 
static bool IsSSE() 
{ 
	DWORD dwStandard = 0; 
	DWORD dwFeature = 0; 

	_asm { 
		push ebx 
		push ecx 
		push edx 

		// get the Standard bits 
		mov eax, 1 
		cpuid 
		mov dwStandard, eax 
		mov dwFeature, edx 

		pop ecx 
		pop ebx 
		pop edx 
	} 

	if (dwFeature & _SSE_FEATURE_BIT)
		return true;
	return false;
} 

bool is_sse_instructions=IsSSE();
