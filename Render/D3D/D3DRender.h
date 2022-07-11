#ifndef __D3_D_RENDER_H_INCLUDED__
#define __D3_D_RENDER_H_INCLUDED__

#include "SlotManager.h"
#include "..\shader\shaders.h"
#include "DrawType.h"
#include "..\src\IDirect3DTextureProxy.h"

enum
{
	RENDERSTATE_MAX=210,
	TEXTURE_MAX=8,
	TEXTURESTATE_MAX=33,
	SAMPLERSTATE_MAX=14,
};

__forceinline void cD3DRender_SetMatrix(D3DXMATRIX &mat,const MatXf &m)
{
	mat._11=m.rot()[0][0],	mat._12=m.rot()[1][0],	mat._13=m.rot()[2][0],	mat._14=0;
	mat._21=m.rot()[0][1],	mat._22=m.rot()[1][1],	mat._23=m.rot()[2][1],	mat._24=0;
	mat._31=m.rot()[0][2],	mat._32=m.rot()[1][2],	mat._33=m.rot()[2][2],	mat._34=0;
	mat._41=m.trans().x,	mat._42=m.trans().y,	mat._43=m.trans().z,	mat._44=1;
}

__forceinline void cD3DRender_SetMatrixTranspose(D3DXMATRIX &mat,const MatXf &m)
{
	mat._11=m.rot()[0][0],	mat._12=m.rot()[0][1],	mat._13=m.rot()[0][2],	mat._14=m.trans().x;
	mat._21=m.rot()[1][0],	mat._22=m.rot()[1][1],	mat._23=m.rot()[1][2],	mat._24=m.trans().y;
	mat._31=m.rot()[2][0],	mat._32=m.rot()[2][1],	mat._33=m.rot()[2][2],	mat._34=m.trans().z;
	mat._41=0,				mat._42=0,				mat._43=0,				mat._44=1;
}

__forceinline void cD3DRender_SetMatrixTranspose4x3(D3DXMATRIX &mat,const MatXf &m)
{
	mat._11=m.rot()[0][0],	mat._12=m.rot()[0][1],	mat._13=m.rot()[0][2],	mat._14=m.trans().x;
	mat._21=m.rot()[1][0],	mat._22=m.rot()[1][1],	mat._23=m.rot()[1][2],	mat._24=m.trans().y;
	mat._31=m.rot()[2][0],	mat._32=m.rot()[2][1],	mat._33=m.rot()[2][2],	mat._34=m.trans().z;
}

#include "PoolManager.h"
#include "OcclusionQuery.h"


class cD3DRender : public cInterfaceRenderDevice
{
	friend class DrawType;
public:
	cD3DRender();
	~cD3DRender();

	virtual bool Initialize(int xScr,int yScr,int mode,HWND hWnd,int RefreshRateInHz=0);
	virtual bool ChangeSize(int xScr,int yScr,int mode);
	virtual int GetAvailableTextureMem();
	virtual int Done();

	virtual int GetClipRect(int *xmin,int *ymin,int *xmax,int *ymax);
	virtual int SetClipRect(int xmin,int ymin,int xmax,int ymax);

	virtual int BeginScene();
	virtual int EndScene();
	virtual int Fill(int r,int g,int b,int a=255);
	virtual int Flush();
	virtual int SetGamma(float fGamma,float fStart=0.f,float fFinish=1.f);

	virtual void SetMultisample(DWORD multisample_){multisample=(D3DMULTISAMPLE_TYPE)multisample_;}
	virtual DWORD GetMultisample(){return multisample;}

	VertexPoolManager* GetVertexPool(){return &vertex_pool_manager;};
	IndexPoolManager* GetIndexPool(){return &index_pool_manager;};
	
	virtual void CreateVertexBuffer(struct sPtrVertexBuffer &vb,int NumberVertex,IDirect3DVertexDeclaration9* declaration,int dynamic=0);
	void CreateVertexBufferBySize(struct sPtrVertexBuffer &vb,int NumberVertex,int size,int dynamic);
	virtual void DeleteVertexBuffer(sPtrVertexBuffer &vb);
	virtual void* LockVertexBuffer(sPtrVertexBuffer &vb,bool readonly=false);
	virtual void UnlockVertexBuffer(sPtrVertexBuffer &vb);
	virtual void CreateIndexBuffer(sPtrIndexBuffer& ib,int NumberVertex,int size=sizeof(sPolygon));
	virtual void DeleteIndexBuffer(sPtrIndexBuffer &ib);
	virtual sPolygon* LockIndexBuffer(sPtrIndexBuffer &ib,bool readonly=false);
	virtual void UnlockIndexBuffer(sPtrIndexBuffer &ib);
	virtual int CreateTexture(class cTexture *Texture,class cFileImage *FileImage,int dxout,int dyout,bool enable_assert=true);
	virtual int DeleteTexture(class cTexture *Texture);
	void* LockTexture(class cTexture *Texture,int& Pitch);
	void* LockTexture(class cTexture *Texture,int& Pitch,Vect2i lock_min,Vect2i lock_size);
	void UnlockTexture(class cTexture *Texture);

	virtual int CreateCubeTexture(class cTexture *Texture);
	virtual int CreateBumpTexture(class cTexture *Texture);

	virtual void SetDrawNode(class cCamera *DrawNode);
	virtual void SetGlobalFog(const sColor4f &color,const Vect2f &v);
	virtual const D3DXVECTOR4& GetVertexFogParam(){return vertex_fog_param;}

	virtual void SetGlobalLight(Vect3f *vLight,sColor4f *Ambient=0,sColor4f *Diffuse=0,sColor4f *Specular=0);

	virtual void Draw(class cScene *Scene, bool objects = false);

	virtual void DrawBound(const MatXf &Matrix,Vect3f &min,Vect3f &max,bool wireframe=0,sColor4c diffuse=sColor4c(255,255,255,255));

	virtual int Create(class cTileMap *TileMap);

	virtual void PreDraw(cTileMap *TileMap);
	virtual void Draw(cTileMap *TileMap,eBlendMode MatMode,bool shadow,bool zbuffer=false);
	virtual int Delete(class cTileMap *TileMap);

	virtual void SetRenderState(eRenderStateOption option,int value);
	virtual unsigned int GetRenderState(eRenderStateOption option);

	// вспомогательные функции, могут быть не реализованы
	virtual void DrawLine(int x1,int y1,int x2,int y2,sColor4c color);
	virtual void DrawPixel(int x1,int y1,sColor4c color);
	virtual void DrawRectangle(int x,int y,int dx,int dy,sColor4c color,bool outline=false);
	virtual void FlushPrimitive2D();

	virtual void OutText(int x,int y,const char *string,const sColor4f& color,ALIGN_TEXT align=ALIGN_TEXT_LEFT,eBlendMode blend_mode=ALPHA_BLEND);
	virtual int OutTextLine(int x, int y, const char *textline, const char* end, const sColor4c& color, eBlendMode blend_mode = ALPHA_BLEND, int xRangeMin = -1, int xRangeMax = -1);
	virtual void OutTextRect(int x,int y,const char *string,ALIGN_TEXT align,Vect2f& bmin,Vect2f& bmax);
	virtual void OutText(int x,int y,const char *string,const sColor4f& color,ALIGN_TEXT align,eBlendMode blend_mode,
				cTexture* pTexture,eColorMode mode,Vect2f uv,Vect2f duv,float phase=0,float lerp_factor=1);
	virtual float GetFontLength(const char *string);
	virtual float GetFontHeight(const char *string);
	virtual float GetLineLength(const char *string);
	virtual float GetCharLength(const char c);

	virtual void OutText(int x,int y,const char *string,int r=255,int g=255,int b=255);
	virtual void OutText(int x,int y,const char *string,int r,int g,int b,char *FontName="Arial",int size=12,int bold=0,int italic=0,int underline=0);
	virtual HWND GetWindowHandle()												{ return hWnd;}
	virtual bool SetScreenShot(const char *fname);
	virtual void DrawSprite(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE,float saturate=1.f);
	virtual void DrawSpriteSolid(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE);
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Tex1,cTexture *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0);
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE);
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,float lerp_factor,float alpha=1,float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE);

	void DrawQuad(float x1, float y1, float dx, float dy, float u1, float v1, float du, float dv, sColor4c color=sColor4c(255,255,255,255));

	virtual void DrawSpriteScale(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Texture,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE);
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Tex1,cTextureScale *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0);
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,float u1,float v1,
		cTextureScale *Tex1,cTextureScale *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD);

	virtual cVertexBuffer<sVertexXYZDT1>* GetBufferXYZDT1(){return &BufferXYZDT1;};
	virtual cVertexBuffer<sVertexXYZDT2>* GetBufferXYZDT2(){return &BufferXYZDT2;};
	virtual cVertexBuffer<sVertexXYZD>* GetBufferXYZD(){return &BufferXYZD;};
	virtual cVertexBuffer<sVertexXYZWD>* GetBufferXYZWD(){return &BufferXYZWD;};
	virtual cQuadBuffer<sVertexXYZDT1>* GetQuadBufferXYZDT1(){return &QuadBufferXYZDT1;}
	virtual cQuadBuffer<sVertexXYZWDT1>* GetQuadBufferXYZWDT1(){return &QuadBufferXYZWDT1;}
	virtual cQuadBuffer<sVertexXYZWDT3>* GetQuadBufferXYZWDT3(){return &QuadBufferXYZWDT3;}
	virtual cVertexBuffer<sVertexXYZWDT1>* GetBufferXYZWDT1(){return &BufferXYZWDT1;};
	virtual cVertexBuffer<sVertexXYZWDT2>* GetBufferXYZWDT2(){return &BufferXYZWDT2;};
	virtual cVertexBuffer<sVertexXYZWDT3>* GetBufferXYZWDT3(){return &BufferXYZWDT3;};

	virtual bool IsFullScreen() {return (RenderMode&RENDERDEVICE_MODE_WINDOW)?false:true;}
	virtual DWORD GetRenderMode()							{return RenderMode;}
	virtual Vect2i GetOriginalScreenSize(){return original_screen_size;}

	virtual int GetSizeFromDeclaration(IDirect3DVertexDeclaration9* declaration);
	virtual int GetSizeFromFmt(int fmt);

	void SetWorldMatrix(const MatXf& pos)
	{
		SetMatrix(D3DTS_WORLD,pos);
	}

	__forceinline void SetMatrix(int type,const MatXf &m)
	{
		D3DXMATRIX mat;
		cD3DRender_SetMatrix(mat,m);
		RDCALL(lpD3DDevice->SetTransform((D3DTRANSFORMSTATETYPE)type,&mat));
	}

	int GetZDepth()
	{
		int f=d3dpp.AutoDepthStencilFormat;
		switch(f)
		{
		case D3DFMT_D16:
			return 16;
		case D3DFMT_D24S8:
			return 24;
		}
		VISASSERT(0);
		return 16;
	}

	int GetBackBuffersSize();

	virtual void SaveStates(const char* fname="states.txt");
	D3DCAPS9					DeviceCaps;

	bool IsPS20(){return DeviceCaps.PixelShaderVersion>= D3DPS_VERSION(2,0);};
	bool IsVS20(){return DeviceCaps.VertexShaderVersion>= D3DVS_VERSION(2,0);};


	//параметры для самозаттенения
	int GetShadowMapSize(){return shadow_map_size;};
	float GetInvShadowMapSize(){return inv_shadow_map_size;}
	const D3DXMATRIX& GetShadowMatViewProj(){return shadow_mat_view_proj;};
	const D3DXMATRIX& GetFloatZBufferMatViewProj(){return floatZBufferMatViewProj;};
	void SetShadowMapSize(int i){shadow_map_size=i;inv_shadow_map_size=1.0f/shadow_map_size;};
	void SetShadowMatViewProj(const D3DXMATRIX* m){shadow_mat_view_proj=*m;};
	void SetFloatZBufferMatViewProj(const D3DXMATRIX* m){floatZBufferMatViewProj=*m;};

	Vect2f& GetInvFloatZBufferSize(){return inv_floatZBufferSize;};
	void SetFloatZBufferSize(int width, int height){
		inv_floatZBufferSize.x = 1.f/width;
		inv_floatZBufferSize.y = 1.f/height;
	}
	//////////
	DrawType*	dtAdvance;

	DrawType*	dtFixed;
	DrawType*	dtAdvanceOriginal;
	eDrawID		dtAdvanceID;

	static void RegisterVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9& declaration, D3DVERTEXELEMENT9* elements);
public:
	virtual bool IsEnableSelfShadow();
	void SetAdvance(bool is_shadow);

	bool PossibleAnisotropic();
	void SetAnisotropic(int level);
	int GetAnisotropic();
	int GetMaxAnisotropicLevels();

	bool PossibilityBump(){	return dtAdvanceOriginal!=NULL;}

	virtual void SetDialogBoxMode(bool enable);

	D3DXVECTOR4 planar_node_size;
	D3DXVECTOR4 fog_of_war_color;
	D3DXVECTOR4 tilemap_inv_size;

	//Применяется ли туман войны в шейдере.
	void SetFogOfWar(bool b){is_fog_of_war=b;};
	bool GetFogOfWar(){return is_fog_of_war;}

	LPDIRECT3DSURFACE9	GetZBuffer() { return lpZBuffer; }
protected:
	bool is_fog_of_war;
public:

//private:
	HWND						hWnd;

	LPDIRECT3D9					lpD3D;
	LPDIRECT3DDEVICE9			lpD3DDevice;
    LPDIRECT3DSURFACE9			lpBackBuffer,lpZBuffer;
	D3DPRESENT_PARAMETERS		d3dpp;
	IDirect3DBaseTexture9*		CurrentTexture[TEXTURE_MAX];
	bool						bSupportVertexFog;
	bool						bSupportTableFog;
	int							dwSuportMaxSizeTextureX,dwSuportMaxSizeTextureY;
	int							MaxTextureAspectRatio;

	bool SetFocus(bool wait,bool focus_error=true);
	int KillFocus();
	LPDIRECT3DTEXTURE9 CreateSurface(int x,int y,eSurfaceFormat TextureFormat,int MipMap,bool enable_assert,DWORD attribute);
	LPDIRECT3DTEXTURE9 CreateUnpackedSurface(int x,int y,eSurfaceFormat TextureFormat,int MipMap,bool enable_assert,DWORD attribute);

	inline IDirect3DBaseTexture9* GetTexture(int dwStage)
	{
		VISASSERT( dwStage<nSupportTexture );
		return CurrentTexture[dwStage];
	}

	inline void SetTexturePhase(int dwStage,cTexture *Texture,float Phase)
	{
		if(Texture==0) 
		{ 
			SetTextureBase(dwStage,NULL);
			return; 
		}
		VISASSERT( dwStage<nSupportTexture );

		int nAllFrame=Texture->GetNumberFrame();
		int nFrame ;
		if(nAllFrame>1)
			nFrame= (int)( 0.999f*Phase*nAllFrame);
		else
			nFrame=0;

		VISASSERT(0<=nFrame&&nFrame<Texture->GetNumberFrame()&&bActiveScene);
		SetTextureBaseP(dwStage,Texture->GetDDSurface(nFrame));
	}

	inline void SetTexture(int dwStage,cTexture *Texture)
	{
		if(Texture==0) 
		{ 
			SetTextureBase(dwStage,NULL);
			return; 
		}
		VISASSERT( dwStage<nSupportTexture );
		VISASSERT( bActiveScene );
		SetTextureBaseP(dwStage,Texture->GetDDSurface(0));
	}

	inline void SetTextureBaseP(DWORD dwStage,IDirect3DTextureProxy *pTexture)
	{
		VISASSERT(dwStage<TEXTURE_MAX);
		if(CurrentTexture[dwStage]!=GETIDirect3DTexture(pTexture))
			RDCALL(lpD3DDevice->SetTexture(dwStage,CurrentTexture[dwStage]=GETIDirect3DTexture(pTexture)));
	}

	inline void SetTextureBase(DWORD dwStage,IDirect3DBaseTexture9 *pTexture)
	{
		VISASSERT(dwStage<TEXTURE_MAX);
		if(CurrentTexture[dwStage]!=pTexture)
			RDCALL(lpD3DDevice->SetTexture(dwStage,CurrentTexture[dwStage]=pTexture));
	}

	inline void SetVertexShader(IDirect3DVertexShader9 * Handle)
	{
		if(Handle!=CurrentVertexShader)
		{
			RDCALL(lpD3DDevice->SetVertexShader(CurrentVertexShader=Handle));
		}
	}

	inline void SetPixelShader(IDirect3DPixelShader9 * Handle)
	{
		if(Handle!=CurrentPixelShader)
		{
			RDCALL(lpD3DDevice->SetPixelShader(CurrentPixelShader=Handle));
		}
	}


	inline void SetVertexDeclaration(IDirect3DVertexDeclaration9* declaration)
	{
        xassert(declaration);
		if(declaration != CurrentDeclaration)
		{
			RDCALL(lpD3DDevice->SetVertexDeclaration(CurrentDeclaration = declaration));
		}
	}

	inline void SetVertexDeclaration(sPtrVertexBuffer& vb)
	{
		SetVertexDeclaration(vb.ptr->declaration);
	}
	__forceinline void SetRenderState(D3DRENDERSTATETYPE State,unsigned int Value)
	{
		VISASSERT(0<=State&&State<RENDERSTATE_MAX);
//		DWORD value;
//		RDCALL(lpD3DDevice->GetRenderState((D3DRENDERSTATETYPE)State,&value));
//		VISASSERT(ArrayRenderState[State]==value || ArrayRenderState[State]==0xefefefef);

		if(ArrayRenderState[State]!=Value) 
		{
			lpD3DDevice->SetRenderState(State,ArrayRenderState[State]=Value);
		}
	}
	inline unsigned int GetRenderState(D3DRENDERSTATETYPE State)
	{
		VISASSERT(0<=State && State<RENDERSTATE_MAX);
		return ArrayRenderState[State];
	}
	__forceinline void SetTextureStageState(unsigned int Stage,D3DTEXTURESTAGESTATETYPE Type,unsigned int Value)
	{
		VISASSERT(Stage<TEXTURE_MAX);
		VISASSERT(0<=Type && Type<TEXTURESTATE_MAX);

//		DWORD value;
///		RDCALL(lpD3DDevice->GetTextureStageState(Stage,(D3DTEXTURESTAGESTATETYPE)Type,&value));
//		VISASSERT(ArrayTextureStageState[Stage][Type]==value || ArrayTextureStageState[Stage][Type]==0xefefefef);

		if(ArrayTextureStageState[Stage][Type]!=Value) 
		{
			RDCALL(lpD3DDevice->SetTextureStageState(Stage,Type,ArrayTextureStageState[Stage][Type]=Value));
		}
	}

	__forceinline DWORD GetTextureStageState(unsigned int Stage,D3DTEXTURESTAGESTATETYPE Type)
	{
		return ArrayTextureStageState[Stage][Type];
	}

	virtual void SetNoMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD);
	virtual void SetWorldMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD,bool useZBuffer=false, bool zreflection=false);

	void SetBlendState(eBlendMode blend);
	void SetBlendStateAlphaRef(eBlendMode blend);

	void SetVertexShaderConstant(int StartRegister,const D3DXMATRIX *pMat);
	void SetVertexShaderConstant(int StartRegister,const D3DXVECTOR4 *pVect);
	void SetPixelShaderConstant(int StartRegister,const D3DXVECTOR4 *pVect);

	virtual void DrawIndexedPrimitive(sPtrVertexBuffer &vb,int OfsVertex,int nVertex,const sPtrIndexBuffer& ib,int nOfsPolygon,int nPolygon)
	{
		SetVertexDeclaration(vb);
		SetIndices(ib);
		SetStreamSource(vb);
		RDCALL(lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,OfsVertex,nVertex,3*nOfsPolygon,nPolygon));
		*PtrNumberPolygon+=nPolygon;
		NumDrawObject++;
	}
	inline void DrawPrimitiveUP(D3DPRIMITIVETYPE Type,UINT Count,void* pVertex,UINT Size)
	{
		RDCALL(lpD3DDevice->DrawPrimitiveUP(Type,Count,pVertex,Size));
	}

	__forceinline IDirect3DVertexBufferProxy* GetVB(const sPtrVertexBuffer& vb)
	{
		return vb.ptr->p;
	}

	inline void SetIndices(IDirect3DIndexBuffer9 *pIndexData)
	{
		if(CurrentIndexBuffer!=pIndexData)
		{
			CurrentIndexBuffer=pIndexData;
			RDCALL(lpD3DDevice->SetIndices(pIndexData));
		}
	}

	inline void SetIndices(const sPtrIndexBuffer& ib)
	{
		SetIndices(GETIDirect3DVertexBuffer(ib.ptr->p));
	}

	inline void SetStreamSource(const sPtrVertexBuffer& vb)
	{
		RDCALL(lpD3DDevice->SetStreamSource(0,GETIDirect3DVertexBuffer(vb.ptr->p),0,vb.GetVertexSize()));
	}

	void SetRenderTarget(IDirect3DSurface9* target,IDirect3DSurface9* pZBuffer);
	void SetRenderTarget(cTexture* target,IDirect3DSurface9* pZBuffer);
	void SetRenderTarget1(cTexture* target1);
	void RestoreRenderTarget();
	void SetDrawTransform(class cCamera *DrawNode);
	cCamera* GetDrawNode(){return DrawNode;};

	virtual void DrawLine(const Vect3f &v1,const Vect3f &v2,sColor4c color);
	virtual void DrawPoint(const Vect3f &v1,sColor4c color);
	void FlushPrimitive3D();
	void FlushPrimitive3DWorld();//Дебаговая функция

	sPtrIndexBuffer& GetStandartIB(){return standart_ib;}

	LPDIRECT3DTEXTURE9 CreateTextureFromMemory(void* pSrcData, UINT SrcDataSize,bool build_lod)
	{
		LPDIRECT3DTEXTURE9 pTexture=NULL;

		HRESULT hr=D3DXCreateTextureFromFileInMemoryEx(lpD3DDevice, pSrcData, SrcDataSize, 
			D3DX_DEFAULT, D3DX_DEFAULT, build_lod?D3DX_DEFAULT:1, 0, D3DFMT_UNKNOWN, 
			D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pTexture);


		if(FAILED(hr))
			return NULL;
		return pTexture;
	}

	int CreateTextureU16V16(class cTexture *Texture,bool defaultpool);

	void SetCurrentConvertDot3Mul(float scale){current_bump_scale=scale;};
	bool ReinitOcclusion();
	bool PossibilityOcclusion();

	void RestoreShader();
	void BuildNormalMap(cTexture *Texture,Vect3f* normals);

	cShaderLib* GetShaderLib(){return pShaderLib;}

	cTexLibrary			TexLibrary;
	D3DFORMAT			TexFmtData[SURFMT_NUMBER];

	bool IsInBeginEndScene(){return bActiveScene;};
	class PSFont* psFont;
	class PSMiniMap* psMiniMap;
	class PSMonochrome* psMonochrome;
	class PSSolidColor* psSolidColor;

	cSlotManagerInit<sSlotVB>& GetSlotVB(){return LibVB;}
	cSlotManagerInit<sSlotIB>& GetSlotIB(){return LibIB;}
	void GetTilemapTextureMemory(int& total,int& free);

	void FlushLine3D(bool world=false,bool check_zbuffer=false);

	//SetSamplerData - высокоуровневая замена SetSamplerState
	//Обычно хватает sampler_clamp_linear..sampler_clamp_point
	void SetSamplerData(DWORD stage,SAMPLER_DATA& data)
	{
		VISASSERT(stage<TEXTURE_MAX);
		SAMPLER_DATA& s=ArraytSamplerData[stage];
		if(s.dword[0]==data.dword[0] && s.dword[1]==data.dword[1])
			return;
		s=data;
		SetSamplerDataReal(stage,s);
	}

	void SetSamplerDataVirtual(DWORD stage,SAMPLER_DATA& data);

	int GetMultisampleNum(){return multisample_num;}

	cTexture* GetWhiteTexture(){return pWhiteTexture;}
protected:
	void SetSamplerDataReal(DWORD stage,SAMPLER_DATA& data);
	//Пользоваться SetSamplerData
	__forceinline void SetSamplerState(DWORD stage,D3DSAMPLERSTATETYPE type,DWORD value)
	{
		VISASSERT(stage<TEXTURE_MAX);
		VISASSERT(0<=type && type<SAMPLERSTATE_MAX);
		if(ArraytSamplerState[stage][type]!=value)
		{
			ArraytSamplerState[stage][type]=value;
			lpD3DDevice->SetSamplerState(stage,type,value);
		}
	}

	inline DWORD GetSamplerState(DWORD Stage,D3DSAMPLERSTATETYPE Type)
	{
		VISASSERT(Stage<TEXTURE_MAX);
		VISASSERT(0<=Type && Type<SAMPLERSTATE_MAX);
		return ArraytSamplerState[Stage][Type];
	}

	typedef std::vector< std::pair<IDirect3DVertexDeclaration9**, D3DVERTEXELEMENT9*> > VertexDeclarations;
	static VertexDeclarations& vertexDeclarations();
	void FlushPoint3D();
	void CreateVertexBufferBySizeFormat(struct sPtrVertexBuffer &vb,int NumberVertex,int size,IDirect3DVertexDeclaration9* declaration,int dynamic);

	DWORD				nSupportTexture;
	bool				bActiveScene;

	IDirect3DIndexBuffer9 *			CurrentIndexBuffer;
	IDirect3DVertexShader9 *		CurrentVertexShader;	// vertex shader
	IDirect3DPixelShader9 *			CurrentPixelShader;
	DWORD							CurrentFVF;
	IDirect3DVertexDeclaration9 *	CurrentDeclaration;
	int								CurrentCullMode;

	cSlotManagerInit<sSlotVB>	LibVB;
	cSlotManagerInit<sSlotIB>	LibIB;
	
	DWORD				ArrayRenderState[RENDERSTATE_MAX];
	DWORD				ArrayTextureStageState[TEXTURE_MAX][TEXTURESTATE_MAX];
	DWORD				ArraytSamplerState[TEXTURE_MAX][SAMPLERSTATE_MAX];
	SAMPLER_DATA		ArraytSamplerData[TEXTURE_MAX];

	D3DMULTISAMPLE_TYPE multisample;

	void FillSamplerState();
	
	VertexPoolManager vertex_pool_manager;
	IndexPoolManager index_pool_manager;

	struct PointStruct
	{
		float x,y;
		sColor4c diffuse;
	};
	struct RectStruct
	{
		float x1,y1;
		float x2,y2;
		sColor4c diffuse;
	};
	vector<PointStruct> points;
	vector<PointStruct> lines;
	vector<RectStruct>	rectangles;

	vector<sVertexXYZD> lines3d;
	vector<sVertexXYZD> points3d;

	cVertexBuffer<sVertexXYZDT1> BufferXYZDT1;
	cVertexBuffer<sVertexXYZDT2> BufferXYZDT2;
	cVertexBuffer<sVertexXYZWD>	 BufferXYZWD;
	cVertexBuffer<sVertexXYZD>	 BufferXYZD;
	cVertexBuffer<sVertexXYZWDT1>	 BufferXYZWDT1;
	cVertexBuffer<sVertexXYZWDT2>	 BufferXYZWDT2;
	cVertexBuffer<sVertexXYZWDT3>	 BufferXYZWDT3;
	cQuadBuffer<sVertexXYZDT1>	 QuadBufferXYZDT1;
	cQuadBuffer<sVertexXYZWDT1>	 QuadBufferXYZWDT1;
	cQuadBuffer<sVertexXYZWDT2>	 QuadBufferXYZWDT2;
	cQuadBuffer<sVertexXYZWDT3>	 QuadBufferXYZWDT3;
	cVertexBuffer<sVertexXYZ>	 BufferXYZOcclusion;

	sPtrIndexBuffer standart_ib;
	float current_bump_scale;

	vector<cOcclusionQuery*> occlusion_query;
	cTexture* pWhiteTexture;

	static void CreateVertexDeclarations(LPDIRECT3DDEVICE9 pDevice);
	static void ReleaseVertexDeclarations();

	void InitVertexBuffers();

	void FlushPixel();
	void FlushLine();
	void FlushFilledRect();

	void DeleteShader();

	void DeleteDynamicVertexBuffer();
	void RestoreDynamicVertexBuffer();
	void RestoreDeviceIfLost();
	void RestoreDeviceForce();

	vector<class cTileMapRender*> tilemaps;
	void ClearTilemapPool();
	void RestoreTilemapPool();

	void InitStandartIB();

	void ConvertDot3(unsigned int* buf,int dx,int dy,float h_mul);
	void ConvertBumpRGB_UVW(unsigned int* buf,int dx,int dy);
	void FixSpecularPower(unsigned int* buf,int dx,int dy);

	void UpdateRenderMode();

	D3DFORMAT GetBackBufferFormat(int Mode);

	cShaderLib* pShaderLib;

	list<cDeleteDefaultResource*> delete_default_resource;

	int multisample_num;
	int anisotropic_level;
	int shadow_map_size;
	float inv_shadow_map_size;
	D3DXMATRIX  shadow_mat_view_proj;
	D3DXMATRIX  floatZBufferMatViewProj;
	D3DXVECTOR4 vertex_fog_param;

	Vect2f inv_floatZBufferSize;

	bool ChangeSizeInternal(int xScr,int yScr,int mode);
	bool RecalculateDeviceSize();

	VSStandart* vsStandart;
	PSStandart* psStandart;

	IDirect3DQuery9 *pQueryEndFrame[2];
	bool evenQueryEndFrame;
	bool initedQueryEndFrame;
	void DeleteQueryEndFrame();
	void CreateQueryEndFrame();

	friend class cTileMapRender;
	friend class cOcclusionQuery;
	friend class cDeleteDefaultResource;

	cTexture* pSphereShadow;
	bool bSphereShadowLoaded;

	void DrawNumberPolygon(int x,int y);
	void CalcMultisampleNum();
	void InitSamplerConstants();

	Vect2i original_screen_size;
	void ClampDeviceSize(int& x,int& y,int mode);//Не давать выставить разрешение в оконном режиме больше чем разрешение экрана.

	bool flag_restore_shader;
	void RestoreShaderReal();
};

__forceinline int VectorToRGBA(Vect3f &v,int a=255)
{
    int r=round(127.0f*v.x)+128,g=round(127.0f*v.y)+128,b=round(127.0f*v.z)+128;
    return (a<<24)+(r<<16)+(g<<8)+(b<<0);
}

int GetTextureFormatSize(D3DFORMAT f);
bool DeclarationHasEntry(IDirect3DVertexDeclaration9* declaration, BYTE type, BYTE usage);

Vect2i GetSize(IDirect3DSurface9 *pTexture);

extern class cD3DRender *gb_RenderDevice3D;
extern bool is_sse_instructions;

const int BLEND_STATE_ALPHA_REF=80;

#endif
