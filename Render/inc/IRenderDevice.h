#ifndef __I_RENDER_DEVICE_H_INCLUDED__
#define __I_RENDER_DEVICE_H_INCLUDED__

#include "rd.h"
#include "unknown.h"
#include "Render/D3D/renderstates.h"

struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DVertexDeclaration9;

class cRenderWindow;
class Camera;

class cTexture;
class cTextureScale;

namespace FT { class Font; };

enum eModeRenderDevice
{
	RENDERDEVICE_MODE_WINDOW	=	1<<0,	// ����� � ����

	//d3d only
	RENDERDEVICE_MODE_VSYNC		=	1<<11,	// ������������ ������������ �������������
	RENDERDEVICE_MODE_STENCIL	=	1<<12,
	RENDERDEVICE_MODE_REF		=	1<<14,
	RENDERDEVICE_MODE_ALPHA		=   1<<16,  //��� �������� rendertarget ������������ alpha 

	//Internal
	RENDERDEVICE_MODE_RETURNERROR=	1<<17, //������ ��� ChangeSize
	RENDERDEVICE_MODE_ONEBACKBUFFER=1<<18,
};

enum eRenderStateOption //���������� �������.
{
	RS_FILLMODE					= 8,
	RS_ZWRITEENABLE             = 14,   /* TRUE to enable z writes */
    RS_CULLMODE                 = 22,   /* D3DCULL */
    RS_FOGENABLE                = 28,   /* TRUE to enable fog blending */
};

/// �������� ��� RS_FILLMODE
enum eFillMode
{
    FILL_POINT					= 1,
    FILL_WIREFRAME				= 2,
    FILL_SOLID					= 3, 
};

///	����� ��������� ������� ������������� ����� �������� �� �����.
enum eColorMode : int
{
	COLOR_MOD,/// dst=tex1*tex2
	COLOR_ADD,/// dst=tex1+tex2
	COLOR_MOD2,/// dst=tex1*tex2*2
	COLOR_MOD4,/// dst=tex1*tex2*4
};

///	�������� ������ �� ����� �������������� ��������.
enum eBlendMode : int
{   /// dst - ���� ������� �� ������,
	/// src - ���� ������� � �������� ���������� �� ����� ����
	/// alpha - ����� ������� � �������� ���������� �� ����� �����
	ALPHA_NONE,///	dst=src
	ALPHA_TEST,///	if(alpha>alphamax) dst=src
	ALPHA_ADDBLENDALPHA, /// dst=dst+src*alpha
	ALPHA_BLEND,/// dst=dst*(1-alpha)+src*alpha
	ALPHA_ADDBLEND,/// dst=dst+src
	ALPHA_SUBBLEND,/// dst=dst-src
	ALPHA_MUL,///dst=dst*src
	ALPHA_BLEND_NOTUSETEXTUREALPHA,//�� �� ��� � ALPHA_BLEND, �� ����� ����� �������� �� �����������.
	ALPHA_BLEND_INVALPHA,
};

enum PRIMITIVETYPE 
{
    PT_POINTLIST = 1,
    PT_LINELIST = 2,
    PT_LINESTRIP = 3,
    PT_TRIANGLELIST = 4,
    PT_TRIANGLESTRIP = 5,
    PT_TRIANGLEFAN = 6,
};

/// ������ �������������� ������
enum ALIGN_TEXT
{
	ALIGN_TEXT_LEFT=-1,
	ALIGN_TEXT_CENTER=0,
	ALIGN_TEXT_RIGHT=+1,
};

struct sSlotVB
{
	IDirect3DVertexBuffer9* p;
	int	init;
	char dynamic;
	IDirect3DVertexDeclaration9* declaration;
	short VertexSize;
	int	NumberVertex;
};

struct sSlotIB
{
	IDirect3DIndexBuffer9* p;
	int	init;
	int	NumberPolygon;
	int	PolygonSize;
};

struct RENDER_API sPtrVertexBuffer
{
	sSlotVB*	ptr;

	sPtrVertexBuffer()		{ptr=0; }
	~sPtrVertexBuffer()		{Destroy();};
	bool IsInit()	{ return ptr!=0; }
	void Destroy();

	int GetVertexSize()const{return ptr ? ptr->VertexSize : 0;}
	int GetNumberVertex()const{return ptr ? ptr->NumberVertex : 0;}
	void CopyAddRef(const sPtrVertexBuffer& from);
};

struct RENDER_API sPtrIndexBuffer
{
	sSlotIB*	ptr;

	sPtrIndexBuffer()		{ ptr=0; }
	~sPtrIndexBuffer();
	int IsInit()		{ return ptr!=0; }
	int GetNumberPolygon()const {return ptr ? ptr->NumberPolygon : 0;}
	void CopyAddRef(const sPtrIndexBuffer& from);
};

#include "VertexFormat.h"
#include "Render/D3D/VertexBuffer.h"

///��������� ��� ������ � ��������� ���� (HWND) �����������.
class RENDER_API cRenderWindow: public UnknownClass
{
public:
	cRenderWindow(HWND hwnd);
	~cRenderWindow();

	HWND GetHwnd(){return hwnd;};
	void ChangeSize();//��������, ����� ���������� ������� ����

	int SizeX(){return size_x;};
	int SizeY(){return size_y;};
protected:
	void CalcSize();
	HWND hwnd;
	bool constant_size;
	int size_x,size_y;

	friend class cD3DRender;
	void SetSizeConstant(int size_x_,int size_y_){size_x=size_x_;size_y=size_y_;constant_size=true;}
	void SetSizeDummy(int size_x_,int size_y_){size_x=size_x_;size_y=size_y_;}
};

class RENDER_API cInterfaceRenderDevice : public UnknownClass
{
public:
	cInterfaceRenderDevice();
	~cInterfaceRenderDevice();

	virtual void SetMultisample(DWORD multisample_)=0;//�������� ����������� ������ ����� Init ��� ChangeSize
	virtual DWORD GetMultisample()=0;
	virtual bool Initialize(int xScr,int yScr,int mode,HWND hWnd,int RefreshRateInHz=0, HWND fallbackWindow = 0)=0;
	virtual bool inited() const = 0;

	virtual int GetAvailableTextureMem()=0;//� ������ ����������

	///ChangeSize � ������������ ������ ��������� �� �����������. �������� ������ ������� ��� ������ �� �����.
	virtual bool ChangeSize(int xScr,int yScr,int mode)=0;

	///�������, ������ ������� ��������� �����������.
	virtual int GetClipRect(int *xmin,int *ymin,int *xmax,int *ymax)=0;
	virtual int SetClipRect(int xmin,int ymin,int xmax,int ymax)=0;

	///������ �������� ���� ��� �������.
	virtual int GetSizeX() = 0;
	virtual int GetSizeY() = 0;

	///���������� ������������ �����. ����� ������� ������� ���� ���������� ������� ���������������.
	virtual cRenderWindow* createRenderWindow(HWND hwnd) = 0;
	virtual void selectRenderWindow(cRenderWindow* window) = 0;
	virtual void setGlobalRenderWindow(cRenderWindow* window) = 0;
	virtual cRenderWindow* currentRenderWindow() = 0;

	/// ���������� ����������� ������� �������������� ��� ������ � 3D ��� ������ ��� ��������. 
	/// cScene::Draw ��� ������ ������ ����.
	virtual void SetDrawTransform(Camera *pDrawNode)=0;

	/// ���������� SetDrawTransform � ����������� ��� ������ ������.
	virtual void setCamera(Camera* camera)=0;
	Camera* camera() { return camera_; }

	virtual bool IsFullScreen()=0;
	virtual Vect2i GetOriginalScreenSize()=0;
	
	///�������� ������������ ������
	virtual int Done()=0;

	/// ��������� ������ BeginScene EndScene
	/// ����������� ������� ������ ����� Fill BeginScene �������� ����� EndScene Flush
	virtual bool IsInBeginEndScene()=0;
	virtual int BeginScene()=0;
	virtual int EndScene()=0;
	virtual int Fill(int r,int g,int b,int a=255)=0;
	virtual int Flush()=0;

	/// ����� ��������.
	/// ������� ������ (fStart+(fFinish-fStart)*pow(i/255.f,fGamma))
	/// �� ������� fStart, fFinish ��� ��������� �����������.
	virtual int SetGamma(float fGamma,float fStart=0.f,float fFinish=1.f)=0;

	/// �������� ��������� ���������.
	virtual void DrawLine(int x1,int y1,int x2,int y2,Color4c color)=0;
	virtual void DrawPixel(int x1,int y1,Color4c color)=0;
	virtual void DrawRectangle(int x,int y,int dx,int dy,Color4c color,bool outline=false)=0;

	/// ��� ��� ����� ��������� ���������� ����������, �� ����� �������� ��� �������, ���� 
	/// ���������� ������ �������� ������ � ����� ������� ���������.
	virtual void FlushPrimitive2D()=0;

	/// �������� ���������� ���������.
	virtual void DrawLine(const Vect3f &v1,const Vect3f &v2,Color4c color)=0;
	virtual void DrawPoint(const Vect3f &v1,Color4c color)=0;
	/// ��� ��� ����� ���������� ���������� ����������, �� ����� �������� ��� �������, ���� 
	/// ���������� ������ �������� ������ � ����� ������� ���������.
	virtual void FlushPrimitive3D()=0;

	/// ������� ����, ������� ����� ���������� �����.
	virtual void SetFont(FT::Font *pFont);
	/// ������� ����, ������� ����� ���������� ����� ��� ��������� SetFont(0)
	virtual void SetDefaultFont(FT::Font *pFont);

	/// ���������� �����. ������������ � ������� �������������.
	/// ����� �������������� ���������� ��� ������������ ������ Render.
	/// ������� ��� ������� ��� ������� ��� ������� �������������� ������, 
	/// ������� �������� ������� &XXXXXX - ��� X - ����������������� �����.
//	virtual void ChangeTextColor(const char* &str,Color4c& diffuse)=0;

	/// ����� ������ �� �����
	virtual void OutText(int x,int y,const char *string,const Color4f& color,ALIGN_TEXT align=ALIGN_TEXT::ALIGN_TEXT_LEFT,eBlendMode blend_mode=eBlendMode::ALPHA_BLEND, Vect2f scale = Vect2f::ID)=0;
	virtual int OutTextLine(int x, int y, const FT::Font& font, const wchar_t *textline, const wchar_t* end, const Color4c& color, eBlendMode blend_mode = eBlendMode::ALPHA_BLEND, int xRangeMin = -1, int xRangeMax = -1) = 0;

	/// ������� ���� ��� ������ GDI ������������ ������ ��� ������.
	virtual void OutText(int x,int y,const char *string,int r=255,int g=255,int b=255)=0;
	virtual void OutText(int x,int y,const char *string,int r,int g,int b,char *FontName/*="Arial"*/,int size=12,int bold=0,int italic=0,int underline=0)=0;

	/// ����, ��� �������� ��� ������ ���������� ��������.
	virtual HWND GetWindowHandle()=0;

	/// �������� ������� ��������� ������ � ����.
	virtual bool SetScreenShot(const char *fname)=0;

	/// ������������� ��������� SetRenderState, �� ������������ �� �������� ������������ � DirectX
	virtual void SetRenderState(eRenderStateOption option,int value)=0;
	virtual unsigned int GetRenderState(eRenderStateOption option) = 0;

	virtual void drawCircle(const Vect3f& vc, float radius, Color4c color) = 0;
	virtual void DrawBound(const MatXf &Matrix,Vect3f &min,Vect3f &max,bool wireframe=0,Color4c color=Color4c(255,255,255,255))=0;

	/// ����� ���������� ������� �� �����.
	/// x,y - ��������� �� ������, dx,dy - ������ �� ������, 
	/// u,v,du,dv - ����� ����� �������� ��������� �� ����� (0,0,1,1) - ��� ��������
	/// phase=0..1 ����� ����� � ������������� Texture
	virtual void DrawSprite(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE, float saturate=1.f)=0;
	virtual void DrawSpriteSolid(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE) = 0;
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Tex1,cTexture *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0)=0;
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE)=0;
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,float lerp_factor,float alpha=1,float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE)=0;
	
	/// cTextureScale - ����������� ��������, ������� ������� ��������� ���������� (����� ������������ �������).
	/// ������ - ��� ������ ���������� ��� ������ ����������
	virtual void DrawSpriteScale(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Texture,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE)=0;
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Tex1,cTextureScale *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0)=0;
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,float u1,float v1,
		cTextureScale *Tex1,cTextureScale *Tex2,const Color4c &ColorMul=Color4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD)=0;

	/// ���������� ��������� ���������� ������ ��������� BeginScene EndScene
	virtual int GetDrawNumberPolygon()											{return NumberPolygon+NumberTilemapPolygon;}
	/// ���������� ��������� �� ����� ��������� ������.
	virtual int GetDrawNumberTilemapPolygon()									{return NumberTilemapPolygon;}

	/// ���������� DIP ���������� ������ ��������� BeginScene EndScene
	virtual int GetDrawNumberObjects()											{return NumDrawObject;}

	/// ���������� ��������� ������. color ���� ������, range - ������� � ������� ������� ������.
	virtual void SetGlobalFog(const Color4f &color,const Vect2f &range)=0;

	virtual cVertexBuffer<sVertexXYZDT1>* GetBufferXYZDT1()=0;
	virtual cVertexBuffer<sVertexXYZD>* GetBufferXYZD()=0;
	virtual cVertexBuffer<sVertexXYZWD>* GetBufferXYZWD()=0;
	virtual cQuadBuffer<sVertexXYZD>* GetQuadBufferXYZD()=0;
	virtual cQuadBuffer<sVertexXYZDT1>* GetQuadBufferXYZDT1()=0;
	virtual cVertexBuffer<sVertexXYZDT2>* GetBufferXYZDT2()=0;
	virtual cVertexBuffer<sVertexXYZWDT1>* GetBufferXYZWDT1()=0;
	virtual cQuadBuffer<sVertexXYZWDT1>* GetQuadBufferXYZWDT1()=0;
	virtual cVertexBuffer<sVertexXYZWDT2>* GetBufferXYZWDT2()=0;

	/// ��������� ������� ������������� �� ����������� � ������� ��������� � �������� ��� ��������.
	virtual void SetNoMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=eColorMode::COLOR_MOD)=0;//FFP
	virtual void SetWorldMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=eColorMode::COLOR_MOD,bool useZBuffer=false, bool zreflection=false)=0;//�� �������
	/// ������������� ��������� ��� ����������� ����������� ������������� (������ ���� �� ������������ �������)
	virtual void setWorldMatrix(const MatXf& pos)=0;

	/// ����� ������������� �� �������/������ �������. ��� �������������� ��. ������������ � DirectX
	virtual void DrawIndexedPrimitive(sPtrVertexBuffer &vb,int OfsVertex,int nVertex,const sPtrIndexBuffer& ib,int nOfsPolygon,int nPolygon)=0;

	/// ��������, �������� � ��������� � ������� � ������ �������.
	virtual void CreateVertexBuffer(struct sPtrVertexBuffer &vb,int NumberVertex,struct IDirect3DVertexDeclaration9* declaration,int dynamic=0)=0;
	virtual void DeleteVertexBuffer(struct sPtrVertexBuffer &vb)=0;
	virtual void* LockVertexBuffer(struct sPtrVertexBuffer &vb,bool readonly=false)=0;
	virtual void UnlockVertexBuffer(struct sPtrVertexBuffer &vb)=0;
	virtual void CreateIndexBuffer(sPtrIndexBuffer& ib,int NumberIndex,int size=sizeof(sPolygon))=0;
	virtual void DeleteIndexBuffer(struct sPtrIndexBuffer &ib)=0;
	virtual sPolygon* LockIndexBuffer(struct sPtrIndexBuffer &ib,bool readonly=false)=0;
	virtual void UnlockIndexBuffer(struct sPtrIndexBuffer &ib)=0;

	void AddNumPolygonToTilemap(){PtrNumberPolygon=&NumberTilemapPolygon;};
	void AddNumPolygonToNormal(){PtrNumberPolygon=&NumberPolygon;};

	virtual bool IsEnableSelfShadow()=0;
	virtual void FlushLine3D(bool world=false,bool check_zbuffer=false)=0;

	virtual void SetSamplerDataVirtual(DWORD stage,SAMPLER_DATA& data)=0;

	//������� ��� �������� �� video memory, ������ Reset �������.
	virtual void RestoreDeviceForce()=0;
public:
	class Camera				*camera_;
	FT::Font					*CurrentFont;
	FT::Font					*DefaultFont;
	int							NumberPolygon,NumDrawObject;
	int							NumberTilemapPolygon;
	int*						PtrNumberPolygon;
	int							RenderMode,xScrMin,yScrMin,xScrMax,yScrMax;
	int							xScr,yScr;//�������� �������� ��� ������

	virtual void DeleteRenderWindow(cRenderWindow* wnd) = 0;

	virtual bool RecalculateDeviceSize()=0;
};

RENDER_API cInterfaceRenderDevice* CreateIRenderDevice(bool multiThread);

extern RENDER_API SAMPLER_DATA sampler_wrap_point;
extern RENDER_API SAMPLER_DATA sampler_clamp_point;

extern RENDER_API SAMPLER_DATA sampler_clamp_linear;
extern RENDER_API SAMPLER_DATA sampler_wrap_linear;

extern RENDER_API SAMPLER_DATA sampler_clamp_anisotropic;//���� SetAnisotropic(0) �� ��������, ����� - ������������ ����������.
extern RENDER_API SAMPLER_DATA sampler_wrap_anisotropic;

enum eSurfaceFormat : int
{
	SURFMT_BAD			= -1,
	SURFMT_COLOR		=	0,
	SURFMT_COLORALPHA,
	SURFMT_RENDERMAP16,
	SURFMT_RENDERMAP32,
	SURFMT_BUMP,
	SURFMT_COLOR32,
	SURFMT_COLORALPHA32,
	SURFMT_RENDERMAP_FLOAT,
	SURFMT_L8,
	SURFMT_A8L8,
	SURFMT_UV,
	SURFMT_U16V16,
	SURFMT_R32F,
	SURFMT_NUMBER
};

RENDER_API bool GetAllTriangle3dx(const char* filename,vector<Vect3f>& point,vector<sPolygon>& polygon);
RENDER_API bool GetAllTextureNames(const char* filename, vector<string>& names);

class RENDER_API ManagedResource
{
public:
	ManagedResource();
	virtual ~ManagedResource();
	virtual void deleteManagedResource()=0;
	virtual void restoreManagedResource()=0;
	virtual void dumpManagedResource(XBuffer& buffer){}
};

extern RENDER_API class cInterfaceRenderDevice *gb_RenderDevice;

#endif
