#ifndef __I_RENDER_DEVICE_H_INCLUDED__
#define __I_RENDER_DEVICE_H_INCLUDED__

#include "unknown.h"
#include "..\d3d\renderstates.h"
#include "..\src\IDirect3dtextureProxy.h"
inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }
inline FLOAT DW2F( DWORD f ) { return *((FLOAT*)&f); }

enum eModeRenderDevice
{
	RENDERDEVICE_MODE_WINDOW	=	1<<0,	// вывод в окно

	//d3d only
	RENDERDEVICE_MODE_VSYNC		=	1<<11,	// использовать вертикальную синхронизацию
	RENDERDEVICE_MODE_STENCIL	=	1<<12,
	RENDERDEVICE_MODE_REF		=	1<<14,
	RENDERDEVICE_MODE_ALPHA		=   1<<16,  //При создании rendertarget использовать alpha 

	//Internal
	RENDERDEVICE_MODE_RETURNERROR=	1<<17, //Только для ChangeSize
	RENDERDEVICE_MODE_ONEBACKBUFFER=1<<18,
};

enum eRenderStateOption //Постепенно удалить.
{
	RS_FILLMODE					= 8,
	RS_ZWRITEENABLE             = 14,   /* TRUE to enable z writes */
    RS_CULLMODE                 = 22,   /* D3DCULL */
    RS_FOGENABLE                = 28,   /* TRUE to enable fog blending */
};

/// Значения для RS_FILLMODE
enum eFillMode
{
    FILL_POINT					= 1,
    FILL_WIREFRAME				= 2,
    FILL_SOLID					= 3, 
};

///	Когда несколько текстур накладываются одним проходом на экран.
enum eColorMode
{
	COLOR_MOD,/// dst=tex1*tex2
	COLOR_ADD,/// dst=tex1+tex2
	COLOR_MOD2,/// dst=tex1*tex2*2
	COLOR_MOD4,/// dst=tex1*tex2*4
};

///	Варианты вывода на экран полупрозрачных объектов.
enum eBlendMode
{   /// dst - цвет пикселя на экране, 
	/// src - цвет пикселя в текстуре умноженный на общий цвет
	/// alpha - альфа пикселя в текстуре умноженное на общую альфу
	ALPHA_NONE,///	dst=src
	ALPHA_TEST,///	if(alpha>alphamax) dst=src
	ALPHA_ADDBLENDALPHA, /// dst=dst+src*alpha
	ALPHA_BLEND,/// dst=dst*(1-alpha)+src*alpha
	ALPHA_ADDBLEND,/// dst=dst+src
	ALPHA_SUBBLEND,/// dst=dst-src
	ALPHA_MUL,///dst=dst*src
	ALPHA_BLEND_NOTUSETEXTUREALPHA,//То эе что и ALPHA_BLEND, но альфа канал текстуры не учитывается.
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

/// режимы форматирования текста
enum ALIGN_TEXT
{
	ALIGN_TEXT_LEFT=-1,
	ALIGN_TEXT_CENTER=0,
	ALIGN_TEXT_RIGHT=+1,
};

struct sSlotVB
{
	IDirect3DVertexBufferProxy* p;
	int			init;
	char			dynamic;
	struct IDirect3DVertexDeclaration9* declaration;
	short			VertexSize;
	int				NumberVertex;
};
struct sSlotIB
{
	IDirect3DIndexBufferProxy* p;
	int			init;
	int				NumberPolygon;
	int				PolygonSize;
};

struct sPtrVertexBuffer
{
	struct sSlotVB*	ptr;
	sPtrVertexBuffer()		{ptr=NULL; }
	~sPtrVertexBuffer()		{Destroy();};
	inline bool IsInit()	{ return ptr!=NULL; }
	void Destroy();

	inline int GetVertexSize()const{return ptr?ptr->VertexSize:0;}
	inline int GetNumberVertex()const{return ptr?ptr->NumberVertex:0;}
	void CopyAddRef(const sPtrVertexBuffer& from);
};
struct sPtrIndexBuffer
{
	struct sSlotIB*	ptr;
	sPtrIndexBuffer()		{ ptr=NULL; }
	~sPtrIndexBuffer();
	inline int IsInit()		{ return ptr!=NULL; }
	inline int GetNumberPolygon()const {return ptr?ptr->NumberPolygon:0;}
	void CopyAddRef(const sPtrIndexBuffer& from);
};

#include "VertexFormat.h"
#include "..\D3D\VertexBuffer.h"

class cFont;
class cTexture;
class cTextureScale;

///Структура для вывода в несколько окон (HWND) изображения.
class cRenderWindow: public cUnknownClass
{
public:
	cRenderWindow(HWND hwnd);
	~cRenderWindow();

	HWND GetHwnd(){return hwnd;};
	void ChangeSize();//Вызывать, когда изменились размеры окна

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

class cInterfaceRenderDevice : public cUnknownClass
{
public:
	cInterfaceRenderDevice();
	~cInterfaceRenderDevice();

	virtual void SetMultisample(DWORD multisample_)=0;//Начинает дейчтвовать только после Init или ChangeSize
	virtual DWORD GetMultisample()=0;
	virtual bool Initialize(int xScr,int yScr,int mode,HWND hWnd,int RefreshRateInHz=0)=0;

	virtual int GetAvailableTextureMem()=0;//В байтах возвращает

	///ChangeSize В многооконном режиме параметры не учитываются. Изменяет размер буферов для вывода на экран.
	virtual bool ChangeSize(int xScr,int yScr,int mode)=0;

	///Область, внутри которой выводится изображение.
	virtual int GetClipRect(int *xmin,int *ymin,int *xmax,int *ymax)=0;
	virtual int SetClipRect(int xmin,int ymin,int xmax,int ymax)=0;

	///Размер текущего окна для рендера.
	virtual int GetSizeX();
	virtual int GetSizeY();

	///Включается многооконный режим. Перед выводом каждого окна необходимо выбрать соответствующее.
	cRenderWindow* CreateRenderWindow(HWND hwnd);
	void SelectRenderWindow(cRenderWindow* window);
	cRenderWindow* GetCurRenderWindow(){return cur_render_window;}

	/// Выставляет необходимые матрицы преобразования для вывода в 3D для случая без шейдеров. 
	/// cScene::Draw это делает внутри себя.
	virtual void SetDrawTransform(class cCamera *pDrawNode)=0;

	/// Выставляет SetDrawTransform и необходимые для вывода буфера.
	virtual void SetDrawNode(class cCamera *DrawNode)=0;
	inline  cCamera *GetDrawNode()							{return DrawNode;}

	virtual bool IsFullScreen()=0;
	virtual Vect2i GetOriginalScreenSize()=0;
	
	///Закрытие графического режима
	virtual int Done()=0;

	/// Находится внутри BeginScene EndScene
	/// Стандартный порядок вызова такой Fill BeginScene рисовать здесь EndScene Flush
	virtual bool IsInBeginEndScene()=0;
	virtual int BeginScene()=0;
	virtual int EndScene()=0;
	virtual int Fill(int r,int g,int b,int a=255)=0;
	virtual int Flush()=0;

	/// Гамма корекция.
	/// Формула такова (fStart+(fFinish-fStart)*pow(i/255.f,fGamma))
	/// не трогать fStart, fFinish без понимания последствий.
	virtual int SetGamma(float fGamma,float fStart=0.f,float fFinish=1.f)=0;

	/// Рисовать двумерные примитивы.
	virtual void DrawLine(int x1,int y1,int x2,int y2,sColor4c color)=0;
	virtual void DrawPixel(int x1,int y1,sColor4c color)=0;
	virtual void DrawRectangle(int x,int y,int dx,int dy,sColor4c color,bool outline=false)=0;

	/// Так как вывод двумерных примитивов кешируется, то нужно вызывать эту функцию, если 
	/// вызываются другие операции вывода и важен порядок отрисовки.
	virtual void FlushPrimitive2D()=0;

	/// Рисовать трехмерные примитивы.
	virtual void DrawLine(const Vect3f &v1,const Vect3f &v2,sColor4c color)=0;
	virtual void DrawPoint(const Vect3f &v1,sColor4c color)=0;
	/// Так как вывод трехмерных примитивов кешируется, то нужно вызывать эту функцию, если 
	/// вызываются другие операции вывода и важен порядок отрисовки.
	virtual void FlushPrimitive3D()=0;

	/// Выбрать фонт, которым будет выводиться текст.
	virtual void SetFont(cFont *pFont);
	/// Выбрать фонт, которым будет выводиться текст при установке SetFont(0)
	virtual void SetDefaultFont(cFont *pFont);

	/// Длинна и ширина соответствующего текста.
	virtual float GetFontLength(const char *string)=0;
	virtual float GetLineLength(const char *string)=0;
	virtual float GetCharLength(const char c)=0;
	virtual float GetFontHeight(const char *string)=0;

	/// Внутренний метод. Использовать с крайней осторожностью.
	/// Перед использованием посмотреть как используется внутри Render.
	/// Реально это затычка для ленивых при простом форматировании текста, 
	/// которое задается токеном &XXXXXX - где X - шестнадцатеричная цифра.
//	virtual void ChangeTextColor(const char* &str,sColor4c& diffuse)=0;

	/// вывод текста на экран
	virtual void OutText(int x,int y,const char *string,const sColor4f& color,ALIGN_TEXT align=ALIGN_TEXT_LEFT,eBlendMode blend_mode=ALPHA_BLEND)=0;
	virtual int OutTextLine(int x, int y, const char *textline, const char* end, const sColor4c& color, eBlendMode blend_mode = ALPHA_BLEND, int xRangeMin = -1, int xRangeMax = -1) = 0;
	virtual void OutTextRect(int x,int y,const char *string,ALIGN_TEXT align,Vect2f& bmin,Vect2f& bmax)=0;
	virtual void OutText(int x,int y,const char *string,const sColor4f& color,ALIGN_TEXT align,eBlendMode blend_mode,
		                 cTexture* pTexture,eColorMode mode,
						 Vect2f uv,//Координаты текстуры в точке x,y
						 Vect2f duv,//du,dv на один логический пиксель 
						//(лог пиксель равен графическому в разрешении 1024x768)
						float phase=0,
						float lerp_factor=1//0..1 Насколько сильно влияет pTexture
						 )=0;

	/// Выводят текс при помощи GDI пользоваться только для дебага.
	virtual void OutText(int x,int y,const char *string,int r=255,int g=255,int b=255)=0;
	virtual void OutText(int x,int y,const char *string,int r,int g,int b,char *FontName/*="Arial"*/,int size=12,int bold=0,int italic=0,int underline=0)=0;

	/// Окно, для которого был создан трехмерный контекст.
	virtual HWND GetWindowHandle()=0;

	/// Записать текущее состояние экрана в файл.
	virtual bool SetScreenShot(const char *fname)=0;

	/// Устанавливает некоторые SetRenderState, не пользоваться не прочитав документацию к DirectX
	virtual void SetRenderState(eRenderStateOption option,int value)=0;
	virtual unsigned int GetRenderState(eRenderStateOption option) = 0;

	/// Выводит ориентированный бокс. Для дебага.
	virtual void DrawBound(const MatXf &Matrix,Vect3f &min,Vect3f &max,bool wireframe=0,sColor4c color=sColor4c(255,255,255,255))=0;

	/// Вывод двумерного спрайта на экран.
	/// x,y - положение на экране, dx,dy - размер на экране, 
	/// u,v,du,dv - какая часть текстуры выводится на экран (0,0,1,1) - вся текстура
	/// phase=0..1 выбор кадра в многокадровой Texture
	virtual void DrawSprite(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE, float saturate=1.f)=0;
	virtual void DrawSpriteSolid(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Texture,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE) = 0;
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,
		cTexture *Tex1,cTexture *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0)=0;
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE)=0;
	virtual void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
		cTexture *Tex1,cTexture *Tex2,float lerp_factor,float alpha=1,float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE)=0;
	
	/// cTextureScale - специальная текстура, размеры которой измененны софтварным (более качественным методом).
	/// Пример - для вывода интерфейса под разные разрешения
	virtual void DrawSpriteScale(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Texture,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eBlendMode mode=ALPHA_NONE)=0;
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,
		cTextureScale *Tex1,cTextureScale *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0)=0;
	virtual void DrawSpriteScale2(int x,int y,int dx,int dy,float u,float v,float u1,float v1,
		cTextureScale *Tex1,cTextureScale *Tex2,const sColor4c &ColorMul=sColor4c(255,255,255,255),float phase=0,eColorMode mode=COLOR_MOD)=0;

	/// Количество полигонов выведенное внутри последних BeginScene EndScene
	virtual int GetDrawNumberPolygon()											{return NumberPolygon+NumberTilemapPolygon;}
	/// Количество полигонов на карте впределах экрана.
	virtual int GetDrawNumberTilemapPolygon()									{return NumberTilemapPolygon;}

	/// Установить параметры тумана. color цвет тумана, range - ближняя и дальняя граница тумана.
	virtual void SetGlobalFog(const sColor4f &color,const Vect2f &range)=0;

	virtual cVertexBuffer<sVertexXYZDT1>* GetBufferXYZDT1()=0;
	virtual cVertexBuffer<sVertexXYZD>* GetBufferXYZD()=0;
	virtual cVertexBuffer<sVertexXYZWD>* GetBufferXYZWD()=0;
	virtual cQuadBuffer<sVertexXYZDT1>* GetQuadBufferXYZDT1()=0;
	virtual cVertexBuffer<sVertexXYZDT2>* GetBufferXYZDT2()=0;
	virtual cVertexBuffer<sVertexXYZWDT1>* GetBufferXYZWDT1()=0;
	virtual cQuadBuffer<sVertexXYZWDT1>* GetQuadBufferXYZWDT1()=0;
	virtual cVertexBuffer<sVertexXYZWDT2>* GetBufferXYZWDT2()=0;

	/// Установка текстур накладываемых на треугольник и способа блендинга в варианте без шейдеров.
	virtual void SetNoMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD)=0;//FFP
	virtual void SetWorldMaterial(eBlendMode blend,const MatXf& mat,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD,bool useZBuffer=false, bool zreflection=false)=0;//на шейдере
	/// Устанявливает положение для последующих выводящихся треугольников (только если не используются шейдеры)
	virtual void SetWorldMatrix(const MatXf& pos)=0;


	/// Вывод треугольников из вертекс/индекс буверов. Про индексирование см. документацию к DirectX
	virtual void DrawIndexedPrimitive(sPtrVertexBuffer &vb,int OfsVertex,int nVertex,const sPtrIndexBuffer& ib,int nOfsPolygon,int nPolygon)=0;

	/// Создание, удаление и обращение к вертекс и индекс буферам.
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

	//Удаляет все текстуры из video memory, делает Reset девайсу.
	virtual void RestoreDeviceForce()=0;
public:
	class cCamera				*DrawNode;
	class cFont					*CurrentFont;
	class cFont					*DefaultFont;
	int							NumberPolygon,NumDrawObject;
	int							NumberTilemapPolygon;
	int*						PtrNumberPolygon;
	int							RenderMode,xScrMin,yScrMin,xScrMax,yScrMax;
	int							xScr,yScr;//Реальная величина бэк буфера
	cRenderWindow*				cur_render_window;
	cRenderWindow*				global_render_window;
	vector<cRenderWindow*>		all_render_window;
	void DeleteRenderWindow(cRenderWindow* wnd);

	virtual bool RecalculateDeviceSize()=0;
	friend cRenderWindow;
};

cInterfaceRenderDevice* CreateIRenderDevice(bool multiThread);

extern SAMPLER_DATA sampler_wrap_point;
extern SAMPLER_DATA sampler_clamp_point;

extern SAMPLER_DATA sampler_clamp_linear;
extern SAMPLER_DATA sampler_wrap_linear;

extern SAMPLER_DATA sampler_clamp_anisotropic;//Если SetAnisotropic(0) то линейная, иначе - анизотропная фильтрация.
extern SAMPLER_DATA sampler_wrap_anisotropic;

//Из RenderDevice.h
enum eSurfaceFormat
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

bool GetAllTriangle3dx(const char* filename,vector<Vect3f>& point,vector<sPolygon>& polygon);
bool GetAllTextureNames(const char* filename, vector<string>& names);

class cDeleteDefaultResource
{
public:
	cDeleteDefaultResource();
	virtual ~cDeleteDefaultResource();
	virtual void DeleteDefaultResource()=0;
	virtual void RestoreDefaultResource()=0;
};

extern class cInterfaceRenderDevice *gb_RenderDevice;

#endif
