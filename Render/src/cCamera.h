#ifndef __C_CAMERA_H_INCLUDED__
#define __C_CAMERA_H_INCLUDED__

class cMeshBank;
class cObjTile;
class cObjMesh;

enum eSceneNode
{
	SCENENODE_OBJECTFIRST	=	0,
	SCENENODE_OBJECT_TILEMAP,
	SCENENODE_OBJECT_NOZ_BEFORE_GRASS,
	SCENENODE_OBJECT_NOZ_AFTER_GRASS,
	SCENENODE_OBJECT		,
	SCENENODE_FLAT_SILHOUETTE,
	SCENENODE_OBJECTSPECIAL	,
	SCENENODE_OBJECT_2PASS,
	SCENENODE_UNDERWATER	,
	MAXSCENENODE			,
	SCENENODE_OBJECTSORT	,
	SCENENODE_ZPASS,
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

struct IDirect3DSurface9;

class cCamera : public cUnknownClass, public sAttribute
{
protected:
	friend cScene;
	cCamera(cScene *UClass);
	virtual ~cCamera();
public:
	//Преобразование из координат мира в экранные координаты.
	//pv - без перспективной коррекции.
	//pe - с коррекцией по перспективе.
	virtual void ConvertorWorldToViewPort(const Vect3f *pw,Vect3f *pv,Vect3f *pe);
	virtual void ConvertorWorldToViewPort(const Vect3f *pw,float WorldRadius,Vect3f *pe,int *ScreenRadius);

	void CorrectWorldRadius(const Vect3f *pw, float *WorldRadius, int ScreenRadius);
	//
	//screen_pos - положение на экране (x=-0.5 - левый угол, +0.5 правый угол, у=-0.5 - верх, +0.5 - низ)
	virtual void ConvertorCameraToWorld(const Vect2f *screen_pos,Vect3f *pw);

	//По точке на экране возвращает луч в глобальных координатах
	virtual void GetWorldRay(const Vect2f& screen_pos,Vect3f& pos,Vect3f& dir);
	// функции позиционирования камеры и изменения ее матрицы

	///SetPosition принимает матрицу в координатах камеры.
	///Эта матрица является инвертированной по сравнению с матрией для установки объектов.
	virtual void SetPosition(const MatXf& matrix);
	virtual const MatXf& GetPosition(){return GetMatrix();};
	//Положение камеры на мире.
	inline Vect3f& GetPos()									{ return Pos; }

	inline MatXf& GetMatrix()								{ return GlobalMatrix; }

	//Делает быстро операцию (GetMatrix()*p).z
	inline float xformZ(const Vect3f& p){return  p.x*GlobalMatrix.R.zx+p.y*GlobalMatrix.R.zy+p.z*GlobalMatrix.R.zz+GlobalMatrix.d.z;}


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
	virtual void GetPlaneClip(sPlane4f PlaneClip[5],const sRectangle4f *Rect);

	inline class cScene*& GetScene()						{ return IParent; }
	void GetLighting(Vect3f& l);
	
	virtual void SetAttr(int attribute)
	{ 
		sAttribute::SetAttribute(attribute); 
	}
	virtual void ClearAttr(int attribute=0xFFFFFFFF)		{ sAttribute::ClearAttribute(attribute); }
	virtual int GetAttr(int attribute=0xFFFFFFFF)			{ return sAttribute::GetAttribute(attribute); }

	//cCamera

	cCamera* GetRoot()	{return RootCamera;}
	cCamera* GetParent(){return Parent;}
	void ClearParent(){Parent=0;}
	void AttachChild(cCamera *child);
	cCamera* FindChildCamera(int AttributeCamera);

	// функции для работы с пирамидой видимости
	virtual void SetClip(const sRectangle4f &clip);
	inline const sRectangle4f& GetClip()			{ return Clip; }
	inline const Vect2f& GetZPlane()				{ return zPlane; }
	void SetZPlaneTemp(Vect2f zp)					{ zPlane=zp;Update(); }

	inline float GetFocusX()						{ return Focus.x; }
	inline float GetFocusY()						{ return Focus.y; }
	inline float GetCenterX()						{ return Center.x; }
	inline float GetCenterY()						{ return Center.y; }

	inline eSceneNode GetCameraPass()				{ return camerapass; }
	inline void SetCameraPass(eSceneNode node)		{ camerapass=node; }

	// функции теста видимости
	eTestVisible TestVisible(const Vect3f &min,const Vect3f &max);
	eTestVisible TestVisibleComplete(const Vect3f &min,const Vect3f &max);
	inline bool TestVisible(int x,int y);	
	eTestVisible TestVisible(const MatXf &matrix,const Vect3f &min,const Vect3f &max);
	inline eTestVisible TestVisible(const Vect3f &center,float radius=0);

	void Attach(int pos,cBaseGraphObject *UObject);
	inline void Attach(int pos,cBaseGraphObject *UObject,const MatXf &m,const Vect3f &min,const Vect3f &max);
	void AttachNoRecursive(int pos,cBaseGraphObject* pbox);

	// инлайновые функции доступа к полям класса
	inline cBaseGraphObject*& GetDraw(int pos,int number)				{ return DrawArray[pos][number]; }
	inline int GetNumberDraw(int pos)							{ return DrawArray[pos].size(); }

	inline sPlane4f& GetPlaneClip3d(int number)					{ return PlaneClip3d[number]; }
	inline int GetNumberPlaneClip3d()							{ return PlaneClip3d_size; }

	inline const Vect2f& GetFocusViewPort()						{ return FocusViewPort; }
	inline const Vect2f& GetScaleViewPort()						{ return ScaleViewPort; }
	// функции для работы с отрисовкой
	inline const Vect2f& GetRenderSize()						{ return RenderSize; }
	inline const Vect3f& GetWorldI()							{ return WorldI; }
	inline const Vect3f& GetWorldJ()							{ return WorldJ; }
	inline const Vect3f& GetWorldK()							{ return WorldK; }

	inline cTexture* GetRenderTarget()							{ return RenderTarget; }
	inline IDirect3DSurface9* GetRenderSurface()				{ return RenderSurface; }
	inline IDirect3DSurface9* GetZBuffer()						{ return pZBuffer;}
	void SetRenderTarget(cTexture *pTexture,IDirect3DSurface9* pZBuf);
	void SetRenderTarget(IDirect3DSurface9 *pSurface,IDirect3DSurface9* pZBuf);

	void EnableGridTest(int grid_dx,int grid_dy,int grid_size);

	
	inline int GetHReflection(){VISASSERT(GetAttr(ATTRCAMERA_REFLECTION));return h_reflection;}

	CMatrix			matProj,matView,matViewProj,matViewProjScr,matProjScr;
	sViewPort		vp;

	//p00-p11 - ближняя плоскость, d00-d11 - дальняя плоскость. 
	void GetFrustumPoint(Vect3f& p00,Vect3f& p01,Vect3f& p10,Vect3f& p11,Vect3f& d00,Vect3f& d01,Vect3f& d10,Vect3f& d11,float rmul=1.0f);
	void DebugDrawFrustum(sColor4c color=sColor4c(255,255,255,255));

	void Update();

	Vect2f CalculateZMinMax(MatXf& view);

	bool IsBadClip();
	void DrawTestGrid();
	sColor4c GetFoneColor(){return phone_color;};
	void SetFoneColor(sColor4c c){phone_color=c;};

	//global function
	void SetCopy(cCamera* pCamera);
	virtual void PreDrawScene();
	virtual void DrawScene();


	/////////
	void UpdateViewProjScr();

	Vect2f CalcZMinZMaxShadowReciver();
	sBox6f CalcShadowReciverInSpace(D3DXMATRIX matrix);

	//Только для reflection камеры
	void SetZTexture(cTexture* zTexture);
	cTexture* GetZTexture(){return pZTexture;};

	inline bool IsShadow(){return is_shadow_in_current_camera;}
	void SetGrass(cBaseGraphObject* grass);
	void SetSecondRT(cTexture* pSecondRTTexture_){pSecondRTTexture=pSecondRTTexture_;};
	cTexture* GetSecondRT(){return pSecondRTTexture;};
protected: 
	void SetHReflection(int h) {h_reflection=h;}
	void UpdateViewport();

	vector<cCamera*>	child;
	cCamera*	Parent;
	cCamera*	RootCamera;
	int h_reflection;

	struct ObjectSort
	{
		float distance;
		cBaseGraphObject* obj;

		inline ObjectSort(){}
		inline ObjectSort(float d,cBaseGraphObject* o){distance=d;obj=o;}
	};

	struct ObjectSortByRadius
	{
		inline bool operator()(const ObjectSort& o1,const ObjectSort& o2)
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
	cScene			*IParent;														// интерфейс породивший данный класс

	//cCamera
	cTexture					*RenderTarget;				// поверхность на которую выводится
	IDirect3DSurface9*			RenderSurface;
	IDirect3DSurface9*			pZBuffer;
	Vect2f						RenderSize;					// размеры устройства вывода

	cTexture*		pSecondRTTexture;

	enum
	{
		PlaneClip3d_size=6,
	};
	sPlane4f		PlaneClip3d[PlaneClip3d_size];				// плоскости отсечения

	vector<cBaseGraphObject*>			DrawArray[MAXSCENENODE];
	vector<ObjectSort>			SortArray;
	Vect2f						FocusViewPort;				// фокус графического окна
	Vect2f						ScaleViewPort;				// коэффициенты неоднородности экрана по осям
	Vect3f						WorldI,WorldJ,WorldK;
	cBaseGraphObject*			grassObj;
protected:
	void DrawSilhouettePlane();
	void DrawAlphaPlane();

	vector<cBaseGraphObject*>	arZPlane;
	void ClearZBuffer();
	void ClearFloatZBuffer();
	void ShowClip();

	void CalcClipPlane();

	eSceneNode camerapass;
	void DrawSortObject();
	void DrawObjectFirst();
	void DrawObject2Pass();

	Vect2i TestGridSize;
	int TestGridShl;
	BYTE* pTestGrid;
	void InitGridTest(int grid_dx,int grid_dy,int grid_size);
	void CalcTestForGrid();
	inline eTestVisible GridTest(Vect3f p[8]);

	void DrawShadowDebug();
	void Set2DRenderState();
	void DrawObjectNoZ(eSceneNode nType);
	void DrawObject(eSceneNode nType);
	void DrawObjectSpecial(eSceneNode nType);
	void DrawSilhouetteObject();
	void DrawTilemapObject();
	void DrawToZBuffer();

	sColor4c phone_color;

	cTexture* pZTexture;

	bool is_shadow_in_current_camera;

	int GetSilhouetteStencil(int silhouetteIndex){ return (silhouetteIndex+1);}

public:
	void GetTestGrid(BYTE*& grid,Vect2i& size,int& shl)
	{
		grid=pTestGrid;
		size=TestGridSize;
		shl=TestGridShl;
	}
};

////////////////////inline cCamera///////////////////////////////////

inline eTestVisible cCamera::TestVisible(const Vect3f &center,float radius)
{ // для BoundingSphere с центром center и радиусом radius (при radius=0 - тест видимости точки)
	if(PlaneClip3d[0].GetDistance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[1].GetDistance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[2].GetDistance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[3].GetDistance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[4].GetDistance(center)<-radius)return VISIBLE_OUTSIDE;
	if(PlaneClip3d[5].GetDistance(center)<-radius)return VISIBLE_OUTSIDE;
	return VISIBLE_INTERSECT;
}

bool cCamera::TestVisible(int x,int y)
{
	x=x>>TestGridShl;
	y=y>>TestGridShl;
	if(x<0 || x>=TestGridSize.x || y<0 || y>=TestGridSize.y)
		return false;
	if(pTestGrid[x+y*TestGridSize.x])
		return true;
	return false;
}


inline void cCamera::Attach(int pos,cBaseGraphObject *UObject,const MatXf &m,const Vect3f &min,const Vect3f &max)
{ 
	if(TestVisible(m,min,max))
		DrawArray[pos].push_back(UObject);
	else
		SortArray.push_back( ObjectSort(UObject->GetPosition().trans().distance(GetPos()),UObject) );

	vector<cCamera*>::iterator it;
	FOR_EACH(child,it)
	{
		cCamera* c=*it;
		if(c->GetAttribute(UObject->GetAttr(ATTRCAMERA_REFLECTION|ATTRCAMERA_SHADOW)))
			if(c->TestVisible(m,min,max))
				if( pos!=SCENENODE_OBJECTSORT )
					c->DrawArray[pos].push_back(UObject);
				else
					c->SortArray.push_back( ObjectSort(UObject->GetPosition().trans().distance(GetPos()),UObject) );
	}
}


class cCameraPlanarLight:public cCamera
{
public:
	cCameraPlanarLight(cScene *UClass,bool objects);
	virtual void DrawScene();
protected:
	bool objects;
};

class cCameraShadowMap : public cCamera 
{
public:
	cCameraShadowMap(cScene* scene);
	virtual void DrawScene();
};

class cCameraMirageMap : public cCamera
{
public:
	cCameraMirageMap(cScene* scene);
	virtual void DrawScene();
};

#endif
