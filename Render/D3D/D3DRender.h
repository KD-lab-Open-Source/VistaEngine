#ifndef __D3_D_RENDER_H_INCLUDED__
#define __D3_D_RENDER_H_INCLUDED__

#include "SlotManager.h"
#include "Render\shader\shaders.h"
#include "DrawType.h"
#include "Render\Inc\IRenderDevice.h"
#include "Render\Src\TexLibrary.h"
#include "PoolManager.h"
#include "OcclusionQuery.h"
#include "Render\Src\VisError.h"

class cFileImage;

class RENDER_API cD3DRender : public cInterfaceRenderDevice
{
	enum {
		RENDERSTATE_MAX=210,
		TEXTURE_MAX=8,
		TEXTURESTATE_MAX=33,
		SAMPLERSTATE_MAX=14,
	};

public:
	cD3DRender();
	~cD3DRender();

	bool Initialize(int xScr,int yScr,int mode,HWND hWnd,int RefreshRateInHz=0, HWND fallbackWindow = 0);
	bool inited() const { return D3DDevice_ != 0; } 

	virtual bool ChangeSize(int xScr,int yScr,int mode);
	virtual int GetAvailableTextureMem();
	virtual int Done();

	virtual int GetClipRect(int *xmin,int *ymin,int *xmax,int *ymax);
	virtual int SetClipRect(int xmin,int ymin,int xmax,int ymax);

	int GetSizeX();
	int GetSizeY();

	cRenderWindow* createRenderWindow(HWND hwnd);
	void selectRenderWindow(cRenderWindow* window);
	void setGlobalRenderWindow(cRenderWindow* window) { currentRenderWindow_ = globalRenderWindow_ = window; }
	cRenderWindow* currentRenderWindow() { return currentRenderWindow_; }

	virtual int BeginScene();
	virtual int EndScene();
	virtual int Fill(int r,int g,int b,int a=255);
	virtual int Flush();
	virtual int SetGamma(float fGamma,float fStart=0.f,float fFinish=1.f);

	virtual void SetMultisample(DWORD multisample_){multisample=(D3DMULTISAMPLE_TYPE)multisample_;}
	virtual DWORD GetMultisample(){return multisample;}

	virtual void CreateVertexBuffer(struct sPtrVertexBuffer &vb,int NumberVertex,IDirect3DVertexDeclaration9* declaration,int dynamic=0);
	void CreateVertexBufferBySize(struct sPtrVertexBuffer &vb,int NumberVertex,int size,int dynamic);
	virtual void DeleteVertexBuffer(sPtrVertexBuffer &vb);
	virtual void* LockVertexBuffer(sPtrVertexBuffer &vb,bool readonly=false);
	virtual void UnlockVertexBuffer(sPtrVertexBuffer &vb);
	virtual void CreateIndexBuffer(sPtrIndexBuffer& ib,int NumberVertex,int size=sizeof(sPolygon));
	virtual void DeleteIndexBuffer(sPtrIndexBuffer &ib);
	virtual sPolygon* LockIndexBuffer(sPtrIndexBuffer &ib,bool readonly=false);
	virtual void UnlockIndexBuffer(sPtrIndexBuffer &ib);
	virtual int CreateTexture(cTexture *Texture, cFileImage *FileImage,int dxout,int dyout,bool enable_assert=true);
	virtual int DeleteTexture(cTexture *Texture);
	void* LockTexture(cTexture *Texture,int& Pitch);
	void* LockTexture(cTexture *Texture,int& Pitch,Vect2i lock_min,Vect2i lock_size);
	void UnlockTexture(cTexture *Texture);

	virtual int CreateCubeTexture(cTexture *Texture);
	virtual int CreateBumpTexture(cTexture *Texture);

	virtual void setCamera(Camera* camera);
	virtual void SetGlobalFog(const Color4f &color,const Vect2f &v);
	virtual const Vect4f& GetVertexFogParam(){return vertex_fog_param;}

	virtual void SetGlobalLight(Vect3f *vLight,Color4f *Ambient=0,Color4f *Diffuse=0,Color4f *Specular=0);

	void drawCircle(const Vect3f& vc, float radius, Color4c color);
	void DrawBound(const MatXf &Matrix,Vect3f &min,Vect3f &max,bool wireframe=0,Color4c diffuse=Color4c(255,255,255,255));

	virtual void SetRenderState(eRenderStateOption option,int value);
	virtual unsigned int GetRenderState(eRenderStateOption option);

	// вспомогательные функции, могут быть не реализованы
	virtual void DrawLine(int x1,int y1,int x2,int y2,Color4c color);
	virtual void DrawPixel(int x1,int y1,Color4c color);
	virtual void DrawRectangle(int x,int y,int dx,int dy,Color4c color,bool outline=false);
	virtual void FlushPrimitive2D();

	virtual void OutText(int x,int y,const char *outtext,const Color4f& color,ALIGN_TEXT align=ALIGN_TEXT_LEFT,eBlendMode blend_mode=ALPHA_BLEND, Vect2f scale = Vect2f::ID);
	virtual int OutTextLine(int x, int y, const FT::Font& font, const wchar_t *textline, const wchar_t* end, const Color4c& color, eBlendMode blend_mode = ALPHA_BLEND, int xRangeMin = -1, int xRangeMax = -1);

	virtual void OutText(int x,int y,const char *string,int r=255,int g=255,int b=255);
	virtual void OutText(int x,int y,const char *string,int r,int g,int b,char *FontName="Arial",int size=12,int bold=0,int italic=0,int underline=0);
	virtual HWND GetWindowHandle(){ return globalRenderWindow_ ? globalRenderWindow_->GetHwnd() : 0; }
	virtual bool SetScreenShot(const char *fname);
	virtual void DrawSprite(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE,float saturate=1.f);
	virtual void DrawSpriteSolid(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE);
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Tex1,cTexture *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0);
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE);
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,float lerp_factor,float alpha=1,float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE);

	void DrawQuad(float x1, float y1, float dx, float dy, float u1, float v1, float du, float dv, Color4c color=Color4c(255,255,255,255));

	virtual void DrawSpriteScale(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Texture,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE);
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Tex1,cTextureScale *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0);
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,float u1,float v1,
		cTextureScale *Tex1,cTextureScale *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD);

	virtual cVertexBuffer<sVertexXYZDT1>* GetBufferXYZDT1(){return &BufferXYZDT1;};
	virtual cVertexBuffer<sVertexXYZDT2>* GetBufferXYZDT2(){return &BufferXYZDT2;};
	virtual cVertexBuffer<sVertexXYZD>* GetBufferXYZD(){return &BufferXYZD;};
	virtual cVertexBuffer<sVertexXYZWD>* GetBufferXYZWD(){return &BufferXYZWD;};
	virtual cQuadBuffer<sVertexXYZD>* GetQuadBufferXYZD(){return &QuadBufferXYZD;}
	virtual cQuadBuffer<sVertexXYZDT1>* GetQuadBufferXYZDT1(){return &QuadBufferXYZDT1;}
	virtual cQuadBuffer<sVertexXYZWDT1>* GetQuadBufferXYZWDT1(){return &QuadBufferXYZWDT1;}
	virtual cQuadBuffer<sVertexXYZWDT3>* GetQuadBufferXYZWDT3(){return &QuadBufferXYZWDT3;}
	virtual cVertexBuffer<sVertexXYZWDT1>* GetBufferXYZWDT1(){return &BufferXYZWDT1;};
	virtual cVertexBuffer<sVertexXYZWDT2>* GetBufferXYZWDT2(){return &BufferXYZWDT2;};
	virtual cVertexBuffer<sVertexXYZWDT3>* GetBufferXYZWDT3(){return &BufferXYZWDT3;};
	virtual cVertexBuffer<sVertexXYZWDT4>* GetBufferXYZWDT4(){return &BufferXYZWDT4;};

	virtual bool IsFullScreen() {return (RenderMode&RENDERDEVICE_MODE_WINDOW)?false:true;}
	virtual DWORD GetRenderMode()							{return RenderMode;}
	virtual Vect2i GetOriginalScreenSize(){return original_screen_size;}

	virtual int GetSizeFromDeclaration(IDirect3DVertexDeclaration9* declaration);
	virtual int GetSizeFromFmt(int fmt);

	void setWorldMatrix(const MatXf& mat);

	int GetBackBuffersSize();

	virtual void SaveStates(const char* fname="states.txt");
	D3DCAPS9					DeviceCaps;

	bool IsPS20(){return DeviceCaps.PixelShaderVersion>= D3DPS_VERSION(2,0);};
	bool IsVS20(){return DeviceCaps.VertexShaderVersion>= D3DVS_VERSION(2,0);};

	//параметры для самозаттенения
	int GetShadowMapSize(){return shadow_map_size;}
	void SetShadowMapSize(int i){shadow_map_size=i;inv_shadow_map_size=1.0f/shadow_map_size;}
	float GetInvShadowMapSize(){return inv_shadow_map_size;}
	const Mat4f& shadowMatViewProj() const {return shadow_mat_view_proj;}
	Mat4f shadowMatBias() const;
	void SetShadowMatViewProj(const Mat4f& m){shadow_mat_view_proj=m;}
	const Mat4f& GetFloatZBufferMatViewProj(){return floatZBufferMatViewProj;}
	void SetFloatZBufferMatViewProj(const Mat4f& m){floatZBufferMatViewProj=m;}
	
	void setPlanarTransform(const Vect4f& transform) { planarTransform_ = transform; }
	const Vect4f& planarTransform() const { return planarTransform_; }

	Vect2f& GetInvFloatZBufferSize() {return inv_floatZBufferSize; }
	void SetFloatZBufferSize(int width, int height){ inv_floatZBufferSize.x = 1.f/width; inv_floatZBufferSize.y = 1.f/height; }

	bool createRenderTargets(int xysize);
	void deleteRenderTargets();
	bool CreateFloatTexture(int width, int height);
	void CreateMirageMap(int x, int y, bool recreate=false);
	
	cTexture* GetShadowMap() { return shadowMap_; }
	cTexture* GetLightMap()	{ return lightMap_; }
	cTexture* GetLightMapObjects() { return lightMapObjects_; }
	cTexture* GetFloatMap() { return floatZBuffer_;	}
	IDirect3DSurface9* GetZBufferShadowMap() { return zBufferShadowMap_; }
	IDirect3DSurface9* GetFloatZBuffer() { return floatZBufferSurface_; }
	cTexture* GetAccessibleZBuffer() { return accessibleZBuffer_; }
	IDirect3DTexture9* GetTZBuffer() { return tZBuffer_; }
	IDirect3DSurface9* GetMirageMap() { return mirageBuffer_; }

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

	bool PossibilityBump(){	return dtAdvanceOriginal!=0;}

	virtual void SetDialogBoxMode(bool enable);

	Vect4f fog_of_war_color;
	Vect4f tilemap_inv_size;

	//Применяется ли туман войны в шейдере.
	void SetFogOfWar(bool b){is_fog_of_war=b;};
	bool GetFogOfWar(){return is_fog_of_war;}

	IDirect3DSurface9*	GetZBuffer() { return zBuffer_; }

	MTSection& resetDeviceLock() { return resetDeviceLock_; }

protected:
	bool is_fog_of_war;

public:
//private:
	IDirect3D9*					D3D_;
	IDirect3DDevice9*			D3DDevice_;
    IDirect3DSurface9*			backBuffer_;
    IDirect3DSurface9*			zBuffer_;
	D3DPRESENT_PARAMETERS		d3dpp;
	IDirect3DBaseTexture9*		CurrentTexture[TEXTURE_MAX];
	bool						bSupportVertexFog;
	bool						bSupportTableFog;
	int							dwSuportMaxSizeTextureX,dwSuportMaxSizeTextureY;
	int							MaxTextureAspectRatio;

	bool SetFocus(bool wait,bool focus_error=true);
	int KillFocus();
	IDirect3DTexture9* CreateSurface(int x,int y,eSurfaceFormat TextureFormat,int MipMap,bool enable_assert,DWORD attribute);
	IDirect3DTexture9* CreateUnpackedSurface(int x,int y,eSurfaceFormat TextureFormat,int MipMap,bool enable_assert,DWORD attribute);

	IDirect3DBaseTexture9* GetTexture(int dwStage)
	{
		xassert( dwStage<int(nSupportTexture) );
		return CurrentTexture[dwStage];
	}

	void SetTexturePhase(int dwStage,cTexture *Texture,float Phase)
	{
		if(Texture==0){ 
			SetTextureBase(dwStage,0);
			return; 
		}
		xassert( dwStage<int(nSupportTexture) );

		int nAllFrame=Texture->frameNumber();
		int nFrame ;
		if(nAllFrame>1)
			nFrame= (int)( 0.999f*Phase*nAllFrame);
		else
			nFrame=0;

		xassert(0<=nFrame&&nFrame<Texture->frameNumber()&&bActiveScene);
		SetTextureBase(dwStage,Texture->GetDDSurface(nFrame));
	}

	void SetTexture(int dwStage,cTexture *Texture)
	{
		if(Texture==0) 
		{ 
			SetTextureBase(dwStage,0);
			return; 
		}
		xassert( dwStage<int(nSupportTexture) );
		xassert( bActiveScene );
		SetTextureBase(dwStage,Texture->GetDDSurface(0));
	}

	void SetTextureBase(DWORD dwStage,IDirect3DBaseTexture9 *pTexture)
	{
		xassert(dwStage<TEXTURE_MAX);
		if(CurrentTexture[dwStage]!=pTexture)
			RDCALL(D3DDevice_->SetTexture(dwStage,CurrentTexture[dwStage]=pTexture));
	}

	void SetVertexShader(IDirect3DVertexShader9 * Handle)
	{
		if(Handle!=CurrentVertexShader)
			RDCALL(D3DDevice_->SetVertexShader(CurrentVertexShader=Handle));
	}
	void SetPixelShader(IDirect3DPixelShader9 * Handle)
	{
		if(Handle!=CurrentPixelShader)
			RDCALL(D3DDevice_->SetPixelShader(CurrentPixelShader=Handle));
	}
	void SetVertexDeclaration(IDirect3DVertexDeclaration9* declaration)
	{
        xassert(declaration);
		if(declaration != CurrentDeclaration)
			RDCALL(D3DDevice_->SetVertexDeclaration(CurrentDeclaration = declaration));
	}

	void SetVertexDeclaration(sPtrVertexBuffer& vb)
	{
		SetVertexDeclaration(vb.ptr->declaration);
	}
	void SetRenderState(D3DRENDERSTATETYPE state,unsigned int value)
	{
		xassert(0<=state && state<RENDERSTATE_MAX);
		if(renderStates_[state] != value) 
			D3DDevice_->SetRenderState(state, renderStates_[state] = value);
	}
	unsigned int GetRenderState(D3DRENDERSTATETYPE state)
	{
		xassert(0<=state && state<RENDERSTATE_MAX);
		return renderStates_[state];
	}
	void SetTextureStageState(unsigned int Stage,D3DTEXTURESTAGESTATETYPE Type,unsigned int Value)
	{
		xassert(Stage<TEXTURE_MAX);
		xassert(0<=Type && Type<TEXTURESTATE_MAX);
		if(textureStageStates_[Stage][Type]!=Value) 
			RDCALL(D3DDevice_->SetTextureStageState(Stage,Type,textureStageStates_[Stage][Type]=Value));
	}
	DWORD GetTextureStageState(unsigned int Stage,D3DTEXTURESTAGESTATETYPE Type)
	{
		return textureStageStates_[Stage][Type];
	}

	virtual void SetNoMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD);
	virtual void SetWorldMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD,bool useZBuffer=false, bool zreflection=false);

	void SetBlendState(eBlendMode blend);
	void SetBlendStateAlphaRef(eBlendMode blend);

	void SetVertexShaderConstant(int StartRegister,const Mat4f& mat);
	void SetVertexShaderConstant(int StartRegister,const Vect4f& vect);
	void SetPixelShaderConstant(int StartRegister,const Vect4f& vect);

	void DrawIndexedPrimitive(sPtrVertexBuffer &vb,int OfsVertex,int nVertex,const sPtrIndexBuffer& ib,int nOfsPolygon,int nPolygon)
	{
		SetVertexDeclaration(vb);
		SetIndices(ib);
		SetStreamSource(vb);
		RDCALL(D3DDevice_->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,OfsVertex,nVertex,3*nOfsPolygon,nPolygon));
		*PtrNumberPolygon+=nPolygon;
		NumDrawObject++;
	}
	void DrawPrimitiveUP(D3DPRIMITIVETYPE Type,UINT Count,void* pVertex,UINT Size)
	{
		RDCALL(D3DDevice_->DrawPrimitiveUP(Type,Count,pVertex,Size));
	}

	IDirect3DVertexBuffer9* GetVB(const sPtrVertexBuffer& vb)
	{
		return vb.ptr->p;
	}

	void SetIndices(IDirect3DIndexBuffer9 *pIndexData)
	{
		if(CurrentIndexBuffer!=pIndexData){
			CurrentIndexBuffer=pIndexData;
			RDCALL(D3DDevice_->SetIndices(pIndexData));
		}
	}

	void SetIndices(const sPtrIndexBuffer& ib)
	{
		SetIndices(ib.ptr->p);
	}

	void SetStreamSource(const sPtrVertexBuffer& vb)
	{
		RDCALL(D3DDevice_->SetStreamSource(0, vb.ptr->p, 0, vb.GetVertexSize()));
	}

	void SetRenderTarget(IDirect3DSurface9* target,IDirect3DSurface9* pZBuffer);
	void SetRenderTarget(cTexture* target,IDirect3DSurface9* pZBuffer);
	void SetRenderTarget1(cTexture* target1);
	void RestoreRenderTarget();
	void SetDrawTransform(Camera* camera);
	Camera* camera() { return camera_; }

	virtual void DrawLine(const Vect3f &v1,const Vect3f &v2,Color4c color);
	virtual void DrawPoint(const Vect3f &v1,Color4c color);
	void FlushPrimitive3D();
	void FlushPrimitive3DWorld();//Дебаговая функция

	sPtrIndexBuffer& GetStandartIB(){return standart_ib;}

	IDirect3DTexture9* CreateTextureFromMemory(void* pSrcData, UINT SrcDataSize,bool build_lod)
	{
		IDirect3DTexture9* pTexture=0;

		HRESULT hr=D3DXCreateTextureFromFileInMemoryEx(D3DDevice_, pSrcData, SrcDataSize, 
			D3DX_DEFAULT, D3DX_DEFAULT, build_lod?D3DX_DEFAULT:1, 0, D3DFMT_UNKNOWN, 
			D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &pTexture);


		if(FAILED(hr))
			return 0;
		return pTexture;
	}

	int CreateTextureU16V16(cTexture *Texture,bool defaultpool);

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
	class PSMiniMapBorder* psMiniMapBorder;
	class PSMonochrome* psMonochrome;
	class PSSolidColor* psSolidColor;

	cSlotManagerInit<sSlotVB>& GetSlotVB(){return LibVB;}
	cSlotManagerInit<sSlotIB>& GetSlotIB(){return LibIB;}

	void FlushLine3D(bool world=false,bool check_zbuffer=false);

	//SetSamplerData - высокоуровневая замена SetSamplerState
	//Обычно хватает sampler_clamp_linear..sampler_clamp_point
	void SetSamplerData(DWORD stage,SAMPLER_DATA& data)
	{
		xassert(stage<TEXTURE_MAX);
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
	void SetSamplerState(DWORD stage,D3DSAMPLERSTATETYPE type,DWORD value)
	{
		xassert(stage<TEXTURE_MAX);
		xassert(0<=type && type<SAMPLERSTATE_MAX);
		if(ArraytSamplerState[stage][type]!=value)
		{
			ArraytSamplerState[stage][type]=value;
			D3DDevice_->SetSamplerState(stage,type,value);
		}
	}

	DWORD GetSamplerState(DWORD Stage,D3DSAMPLERSTATETYPE Type)
	{
		xassert(Stage<TEXTURE_MAX);
		xassert(0<=Type && Type<SAMPLERSTATE_MAX);
		return ArraytSamplerState[Stage][Type];
	}

	typedef std::vector< std::pair<IDirect3DVertexDeclaration9**, D3DVERTEXELEMENT9*> > VertexDeclarations;
	static VertexDeclarations& vertexDeclarations();
	void FlushPoint3D();
	void CreateVertexBufferBySizeFormat(struct sPtrVertexBuffer &vb,int NumberVertex,int size,IDirect3DVertexDeclaration9* declaration,int dynamic);

	DWORD				nSupportTexture;
	bool				bActiveScene;

	IDirect3DIndexBuffer9*			CurrentIndexBuffer;
	IDirect3DVertexShader9*			CurrentVertexShader;	// vertex shader
	IDirect3DPixelShader9*			CurrentPixelShader;
	DWORD							CurrentFVF;
	IDirect3DVertexDeclaration9*	CurrentDeclaration;
	int								CurrentCullMode;

	MTSection resetDeviceLock_;

	cRenderWindow*				currentRenderWindow_;
	cRenderWindow*				globalRenderWindow_;
	vector<cRenderWindow*>		all_render_window;

	cSlotManagerInit<sSlotVB>	LibVB;
	cSlotManagerInit<sSlotIB>	LibIB;
	
	DWORD				renderStates_[RENDERSTATE_MAX];
	DWORD				textureStageStates_[TEXTURE_MAX][TEXTURESTATE_MAX];
	DWORD				ArraytSamplerState[TEXTURE_MAX][SAMPLERSTATE_MAX];
	SAMPLER_DATA		ArraytSamplerData[TEXTURE_MAX];

	D3DMULTISAMPLE_TYPE multisample;

	cTexture*		shadowMap_;
	IDirect3DSurface9* mirageBuffer_;
	IDirect3DSurface9*	zBufferShadowMap_;
	cTexture*		floatZBuffer_;
	IDirect3DSurface9* floatZBufferSurface_;
	cTexture*		lightMap_;
	cTexture*		lightMapObjects_;
	cTexture* accessibleZBuffer_;
	IDirect3DTexture9* tZBuffer_;
	Vect4f planarTransform_;

	void FillSamplerState();
	
	struct PointStruct
	{
		float x, y;
		Color4c diffuse;
	};
	struct RectStruct
	{
		float x1, y1;
		float x2, y2;
		Color4c diffuse;
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
	cVertexBuffer<sVertexXYZWDT4>	 BufferXYZWDT4;
	cQuadBuffer<sVertexXYZD>	 QuadBufferXYZD;
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

	void InitStandartIB();

	void ConvertDot3(unsigned int* buf,int dx,int dy,float h_mul);
	void ConvertBumpRGB_UVW(unsigned int* buf,int dx,int dy);
	void FixSpecularPower(unsigned int* buf,int dx,int dy);

	void UpdateRenderMode();

	D3DFORMAT GetBackBufferFormat(int Mode);

	cShaderLib* pShaderLib;

	typedef vector<ManagedResource*> ManagedResources;
	ManagedResources managedResources_;

	int multisample_num;
	int anisotropic_level;
	int shadow_map_size;
	float inv_shadow_map_size;
	Mat4f shadow_mat_view_proj;
	Mat4f floatZBufferMatViewProj;
	Vect4f vertex_fog_param;

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
	friend class ManagedResource;

	void DrawNumberPolygon(int x,int y);
	void CalcMultisampleNum();
	void InitSamplerConstants();

	Vect2i original_screen_size;
	void ClampDeviceSize(int& x,int& y,int mode);//Не давать выставить разрешение в оконном режиме больше чем разрешение экрана.

	bool flag_restore_shader;
	void RestoreShaderReal();

	void DeleteRenderWindow(cRenderWindow* wnd);

	friend class DrawType;
};

int GetTextureFormatSize(D3DFORMAT f);

Vect2i GetSize(IDirect3DSurface9 *pTexture);

void BuildMipMap(int x,int y,int bpp,int bplSrc,void *pSrc,int bplDst,void *pDst,int Blur=0);
void BuildDot3Map(int x,int y,void *pSrc,void *pDst);
void BuildBumpMap(int x,int y,void *pSrc,void *pDst,int fmtBumpMap);

extern RENDER_API cD3DRender *gb_RenderDevice3D;
extern bool is_sse_instructions;

const int BLEND_STATE_ALPHA_REF=80;

#endif
