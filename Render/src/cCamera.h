#ifndef __C_CAMERA_H_INCLUDED__
#define __C_CAMERA_H_INCLUDED__

#include "Render\Inc\IVisGenericInternal.h"
#include "XMath\Plane.h"
#include "XMath\Mat4f.h"
#include "XMath\Rectangle4f.h"

struct IDirect3DSurface9;

enum SceneNode
{
	SCENENODE_OBJECTFIRST	=	0, // cCloudShadow, FieldOfView, FogOfWar, Lights
	SCENENODE_OBJECT_TILEMAP, // cTileMap
	SCENENODE_OBJECT_NOZ_BEFORE_GRASS, // cEffect, CircleManager
	SCENENODE_OBJECT_NOZ_AFTER_GRASS, // cEffect, CircleManager
	SCENENODE_OBJECT		,
	SCENENODE_FLAT_SILHOUETTE,
	SCENENODE_OBJECTSPECIAL	,
	SCENENODE_OBJECT_2PASS,
	SCENENODE_UNDERWATER	,
	SCENENODE_OBJECT_NOZ,
	MAXSCENENODE			,
	SCENENODE_OBJECTSORT	,
	SCENENODE_ZPASS
};

struct sViewPort
{
    int		X;
    int		Y;
    int		Width;
    int		Height;
    float	MinZ;
    float	MaxZ;
};

enum eTestVisible
{
	VISIBLE_OUTSIDE=0,
	VISIBLE_INTERSECT=1,
	VISIBLE_INSIDE=2,
};

class RENDER_API Camera : public UnknownClass, public sAttribute
{
protected:
	friend cScene;
	Camera(cScene *UClass);
	virtual ~Camera();
public:
	//Преобразование из координат мира в экранные координаты.
	//pv - без перспективной коррекции.
	//pe - с коррекцией по перспективе.
	void ConvertorWorldToViewPort(const Vect3f *pw,Vect3f *pv,Vect3f *pe) const;
	void ConvertorWorldToViewPort(const Vect3f& pw, float WorldRadius, Vect3f& pe, float& ScreenRadius) const;

	//screen_pos - положение на экране (x=-0.5 - левый угол, +0.5 правый угол, у=-0.5 - верх, +0.5 - низ)
	void ConvertorCameraToWorld(const Vect2f& screen_pos, Vect3f& pw) const;

	//По точке на экране возвращает луч в глобальных координатах
	virtual void GetWorldRay(const Vect2f& screen_pos,Vect3f& pos,Vect3f& dir) const;
	// функции позиционирования камеры и изменения ее матрицы

	///SetPosition принимает матрицу в координатах камеры.
	///Эта матрица является инвертированной по сравнению с матрией для установки объектов.
	virtual void SetPosition(const MatXf& matrix);
	virtual const MatXf& GetPosition(){ return GetMatrix(); }
	//Положение камеры на мире.
	Vect3f& GetPos()									{ return Pos; }
	const Vect3f& GetPos() const						{ return Pos; }

	const MatXf& GetMatrix() const { return GlobalMatrix; }

	//Делает быстро операцию (GetMatrix()*p).z
	float xformZ(const Vect3f& p){return  p.x*GlobalMatrix.R.zx+p.y*GlobalMatrix.R.zy+p.z*GlobalMatrix.R.zz+GlobalMatrix.d.z;}


	/*  SetFrustum  Перевод Center Clipping в координаты на экране.
		vp.X = round((Center.x+Clipping.xmin())*RenderSize.x);
		vp.Y = round((Center.y+Clipping.ymin())*RenderSize.y);
		vp.Width = round((Center.x+Clipping.xmax())*RenderSize.x)-vp.X;
		vp.Height = round((Center.y+Clipping.ymax())*RenderSize.y)-vp.Y;
	*/
	virtual void SetFrustum(const Vect2f *Center,const sRectangle4f *Clipping,const Vect2f *Focus,
						const Vect2f *zPlane//Near and far z plane
						);
	virtual void GetFrustum(Vect2f *Center,sRectangle4f *Clipping,Vect2f *Focus,Vect2f *zPlane);

	//camera_position - (0,0) -левый верхний угол, (1,1) - правый нижний угол.
	//Камера клипуется по этим координатам а также автоматически выставляет центр и видимую область 
	//внутри этой области. Заменяет установку  Focus, Center и Clipping в SetFrustum
	void SetFrustumPositionAutoCenter(sRectangle4f& camera_position,float focusx);

	//Плоскости, которыми отсекается видимая область мира.
	//Rect - положение на экране, при котором определяются плоскости отсечения.
	//0 - near z, 1 - left, 2 - right, 3 - top, 4 - bottom.
	//Координаты определяются по типу screen_pos в ConvertorCameraToWorld
	//
	void GetPlaneClip(Plane PlaneClip[5],const sRectangle4f *Rect);

	cScene* scene() const { return scene_; }
	void setScene(cScene* scene) { scene_ = scene; }

	void GetLighting(Vect3f& l);
	
	Camera* GetRoot()	{return RootCamera;}
	Camera* GetParent(){return Parent;}
	void ClearParent(){Parent=0;}
	void AttachChild(Camera *child);
	Camera* FindChildCamera(int AttributeCamera);

	// функции для работы с пирамидой видимости
	virtual void SetClip(const sRectangle4f &clip);
	const sRectangle4f& GetClip()			{ return Clip; }
	const Vect2f& GetZPlane()				{ return zPlane; }
	void SetZPlaneTemp(Vect2f zp)			{ zPlane=zp;Update(); }

	float GetFocusX() const					{ return Focus.x; }
	float GetFocusY() const					{ return Focus.y; }
	float GetCenterX() const				{ return Center.x; }
	float GetCenterY() const				{ return Center.y; }

	SceneNode GetCameraPass() const			{ return camerapass; }
	void SetCameraPass(SceneNode node)		{ camerapass=node; }

	// функции теста видимости
	eTestVisible TestVisible(const Vect3f &min,const Vect3f &max);
	bool TestVisible(int x,int y);	
	eTestVisible TestVisible(const MatXf &matrix,const Vect3f &min,const Vect3f &max);
	eTestVisible TestVisible(const Vect3f &center,float radius=0);

	void Attach(SceneNode pos, BaseGraphObject* object);
	void AttachNoRecursive(SceneNode pos, BaseGraphObject* object);

	// инлайновые функции доступа к полям класса
	BaseGraphObject*& GetDraw(int pos,int number)				{ return DrawArray[pos][number]; }
	int GetNumberDraw(int pos)							{ return DrawArray[pos].size(); }

	Plane& GetPlaneClip3d(int number)					{ return PlaneClip3d[number]; }
	int GetNumberPlaneClip3d()							{ return PlaneClip3d_size; }

	const Vect2f& GetFocusViewPort() const					{ return FocusViewPort; }
	const Vect2f& GetScaleViewPort() const					{ return ScaleViewPort; }
	// функции для работы с отрисовкой
	const Vect2f& GetRenderSize() const					{ return RenderSize; }
	const Vect3f& GetWorldI() const						{ return WorldI; }
	const Vect3f& GetWorldJ() const						{ return WorldJ; }
	const Vect3f& GetWorldK() const						{ return WorldK; }

	cTexture* GetRenderTarget()							{ return RenderTarget; }
	IDirect3DSurface9* GetRenderSurface()				{ return RenderSurface; }
	IDirect3DSurface9* GetZBuffer()						{ return pZBuffer;}
	void SetRenderTarget(cTexture *pTexture,IDirect3DSurface9* pZBuf);
	void SetRenderTarget(IDirect3DSurface9 *pSurface,IDirect3DSurface9* pZBuf);

	void EnableGridTest(int grid_dx,int grid_dy,int grid_size);
	
	Mat4f			matProj,matView,matViewProj,matViewProjScr,matProjScr;
	sViewPort		vp;

	//p00-p11 - ближняя плоскость, d00-d11 - дальняя плоскость. 
	void GetFrustumPoint(Vect3f& p00,Vect3f& p01,Vect3f& p10,Vect3f& p11,Vect3f& d00,Vect3f& d01,Vect3f& d10,Vect3f& d11,float rmul=1.0f);
	void DebugDrawFrustum(Color4c color=Color4c(255,255,255,255));

	void Update();

	bool IsBadClip();
	Color4c GetFoneColor(){return phone_color;};
	void SetFoneColor(Color4c c){phone_color=c;};

	//global function
	void SetCopy(Camera* camera, bool copyAll = true);
	virtual void PreDrawScene();
	virtual void DrawScene();

	/////////
	void UpdateViewProjScr();

	Vect2f CalcZMinZMaxShadowReciver();
	sBox6f CalcShadowReciverInSpace(const Mat4f& matrix);

	//Только для reflection камеры
	void SetZTexture(cTexture* zTexture);
	cTexture* GetZTexture(){return pZTexture;}

	bool IsShadow(){return is_shadow_in_current_camera;}
	void SetGrass(BaseGraphObject* grass);
	void SetSecondRT(cTexture* pSecondRTTexture_){pSecondRTTexture=pSecondRTTexture_;}
	cTexture* GetSecondRT(){return pSecondRTTexture;}

	void GetTestGrid(BYTE*& grid,Vect2i& size,int& shl)
	{
		grid=pTestGrid;
		size=TestGridSize;
		shl=TestGridShl;
	}

protected: 
	void UpdateViewport();

	typedef vector<Camera*> Cameras;
	Cameras cameras_;
	Camera* Parent;
	Camera* RootCamera;

	struct ObjectSort
	{
		float distance;
		BaseGraphObject* obj;

		ObjectSort(){}
		ObjectSort(float d,BaseGraphObject* o){distance=d;obj=o;}
	};

	struct ObjectSortByRadius
	{
		bool operator()(const ObjectSort& o1,const ObjectSort& o2)
		{
			return o1.distance>o2.distance;
		}
	};

	// первичные значения
	Vect3f				Pos;						// местоположение камеры
	// viewport
	Vect2f				Focus;			
	Vect2f				Center;
	sRectangle4f		Clip;						// Clip.left,Clip.right,Clip.top,Clip.bottom - 0..1 - размеры видимой области
	Vect2f				zPlane;

	//new
	MatXf			GlobalMatrix;													// глобальная матрица объекта, относительно мировых координат
	cScene			*scene_;														// интерфейс породивший данный класс

	//Camera
	cTexture*					RenderTarget;				// поверхность на которую выводится
	IDirect3DSurface9*			RenderSurface;
	IDirect3DSurface9*			pZBuffer;
	Vect2f						RenderSize;					// размеры устройства вывода

	cTexture*		pSecondRTTexture;

	enum
	{
		PlaneClip3d_size=6,
	};
	Plane		PlaneClip3d[PlaneClip3d_size];				// плоскости отсечения

	typedef vector<BaseGraphObject*> BaseGraphObjects;
	BaseGraphObjects			DrawArray[MAXSCENENODE];
	vector<ObjectSort>			SortArray;
	Vect2f						FocusViewPort;				// фокус графического окна
	Vect2f						ScaleViewPort;				// коэффициенты неоднородности экрана по осям
	Vect3f						WorldI,WorldJ,WorldK;
	BaseGraphObject*			grassObj;
protected:
	void DrawSilhouettePlane();
	void DrawAlphaPlane();

	void ClearZBuffer();
	void ClearFloatZBuffer();
	void ShowClip();

	void CalcClipPlane();

	SceneNode camerapass;
	void DrawSortObject();
	void DrawObjectFirst();
	void DrawObjectFirstSorted();
	void DrawObject2Pass();

	Vect2i TestGridSize;
	int TestGridShl;
	BYTE* pTestGrid;
	void InitGridTest(int grid_dx,int grid_dy,int grid_size);
	void CalcTestForGrid();
	eTestVisible GridTest(Vect3f p[8]);

	void DrawShadowDebug();
	void Set2DRenderState();
	void DrawObjectNoZ(SceneNode nType);
	void DrawObject(SceneNode nType);
	void DrawObjectSpecial(SceneNode nType);
	void DrawSilhouetteObject();
	void DrawTilemapObject();
	void DrawToZBuffer();

	Color4c phone_color;

	cTexture* pZTexture;

	bool is_shadow_in_current_camera;

	int GetSilhouetteStencil(int silhouetteIndex){ return (silhouetteIndex+1);}
};

class CameraPlanarLight : public Camera
{
public:
	CameraPlanarLight(cScene *scene,bool objects);
	~CameraPlanarLight();
	void DrawScene();
	void drawLights();
protected:
	bool objects;
	cTexture* sphereShadowTexture_;
};

class CameraShadowMap : public Camera 
{
public:
	CameraShadowMap(cScene* scene);
	void DrawScene();
};

class CameraMirageMap : public Camera
{
public:
	CameraMirageMap(cScene* scene);
	void DrawScene();
};


////////////////////inline Camera///////////////////////////////////

inline eTestVisible Camera::TestVisible(const Vect3f &center,float radius)
{ // для BoundingSphere с центром center и радиусом radius (при radius=0 - тест видимости точки)
	if(PlaneClip3d[0].distance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[1].distance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[2].distance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[3].distance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[4].distance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[5].distance(center)<-radius)return VISIBLE_OUTSIDE;
	return VISIBLE_INTERSECT;
}

inline bool Camera::TestVisible(int x,int y)
{
	x=x>>TestGridShl;
	y=y>>TestGridShl;
	if(x<0 || x>=TestGridSize.x || y<0 || y>=TestGridSize.y)
		return false;
	if(pTestGrid[x+y*TestGridSize.x])
		return true;
	return false;
}

#endif
