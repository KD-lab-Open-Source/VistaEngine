#ifndef __SCENE_H_INCLUDED__
#define __SCENE_H_INCLUDED__
#include "UnkLight.h"
#include "czplane.h"
#include "NParticle.h"
#include "Render\3dx\Node3dx.h"
#include "Render\3dx\Simply3dx.h"

class FogOfWar;
class cStaticSimply3dx;
class FunctorMapZ;
#include "VisGrid2d.h"

struct RENDER_API ListSimply3dx
{
	cStaticSimply3dx* pStatic;
	vector<cSimply3dx*> objects;
};

class RENDER_API cScene : public UnknownClass, public ManagedResource
{
public:
	cScene();
	~cScene();
	// отрисовка
	void Compact();  //Удаляет объектя, находящиеся в кеше, на которые нет внешних ссылок.
	void Draw(Camera* camera);		// отрисовка указанной части мира

	//Время в милисекундах в этих функциях.
	void SetDeltaTime(float dTime);//в милисекундах
	float GetDeltaTime()const{return dTime;};
	int GetDeltaTimeInt()const{return dTimeInt;};

	Camera* CreateCamera();
	// функции для работы с объектами
	cObject3dx* CreateObject3dx(const char* fname,const char *TexturePath=0,bool interpolated=false);
	cObject3dx* CreateLogic3dx(const char* fname, bool interpolated = false);
	cSimply3dx* CreateSimply3dx(const char* fname,const char* visible_group=0,const char *TexturePath=0);

	//После вызова CreateObject3dxNoAttach нужно будет AttachObj(pObject) вызвать, чтобы прилинковать объект к сцене.
	cObject3dx* CreateObject3dxDetached(const char* fname,const char *TexturePath=0,bool interpolated=false);
	cSimply3dx* CreateSimply3dxDetached(const char* fname,const char* visible_group=0,const char *TexturePath=0);
	bool CreateDebrisesDetached(const c3dx* model,vector<cSimply3dx*>& debrises);
	// функции для работы с источниками света, влияющими на освещение объектов текущей сцены
	//Attribute - сумма флагов из eAttributeLight
	cUnkLight* CreateLight(int Attribute=0, const char* TextureName = 0);
	cUnkLight* CreateLight(int Attribute,cTexture *pTexture);

	cUnkLight* CreateLightDetached(int Attribute=0, const char* TextureName = 0);
	cUnkLight* CreateLightDetached(int Attribute,cTexture *pTexture);
	// функции для работы с системой частиц
	cEffect* CreateEffectDetached(EffectKey& el,c3dx* models,bool auto_delete_after_life=false);

	// функции для работы с полигональным объектом
	cPlane* CreatePlaneObj();

	// функции для работы с картой мира
	virtual class cTileMap* CreateMap(bool isTrueColor = true);
	virtual FogOfWar* CreateFogOfWar();
	void EnableFogOfWar(bool enable);
	bool IsFogOfWarEnabled(){return enable_fog_of_war;}
	FogOfWar* GetFogOfWar(){return enable_fog_of_war?pFogOfWar:0;};

	//Trace переименовали в TraceSegment
	virtual bool TraceSegment(const Vect3f& pStart,const Vect3f& pFinish,Vect3f *pTrace=0);
	virtual bool TraceDir(const Vect3f& pStart,const Vect3f& pDir,Vect3f *pTrace=0);

	//Добавить/удалить объект из сцены. 
	void AttachObj(BaseGraphObject *UObj);
	void DetachObj(BaseGraphObject *UObj);

	// доступ к переменным
	inline int GetNumberLight()									{ return UnkLightArray.size(); }
	inline cUnkLight* GetLight(int number)						{ return (cUnkLight*)UnkLightArray[number]; }

	cTileMap *GetTileMap(){return tileMap_;}
	
	inline Vect2i GetWorldSize(){return Size;}
	void DeleteAutoObject();

	MTSection& GetLockDraw(){return lock_draw;}

	void SetSunShadowDir(const Vect3f& direction);
	void SetSunDirection(const Vect3f& direction);
	void SetSunColor(const Color4f& ambient, const Color4f& diffuse, const Color4f& specular);
	const Vect3f& GetSunDirection()const{return sun_direction;}
	const Color4f& GetSunAmbient()const{return sun_ambient;}
	const Color4f& GetSunDiffuse()const{return sun_diffuse;}
	const Color4f& GetSunSpecular()const{return sun_specular;}
	Color4f GetPlainLitColor();//Для объектов, у которых нормаль направленна вверх, или они освещаются просто цветом.

	void HideAllObjectLights(bool hide);//Днем ночные источники света не горят.
	void HideSelfIllumination(bool hide);//Днем самосвечения не нужно.

	void GetLighting(Color4f &Ambient,Color4f &Diffuse,Color4f &Specular,Vect3f &LightDirection);
	void GetLighting(Vect3f *LightDirection);
	void GetLightingShadow(Vect3f& LightDirection);

	void SetSkyCubemap(cTexture* pTexture);
	cTexture* GetSkyCubemap(){return pSkyCubemap;};
	static void DisableSkyCubeMap(){is_sky_cubemap=false;};
	static bool IsSkyCubemap(){return is_sky_cubemap;}

	void AddCircleShadow(const Vect3f& pos,float r,Color4c intensity);//internal

	void GetAllObject3dx(vector<cObject3dx*>& objectsList);
	vector<ListSimply3dx>& GetAllSimply3dxList();
	void GetAllEffects(vector<cEffect*>& effectsList);

	bool DebugInUpdateList();

	void EnableReflection(bool enable);
	bool IsReflection(){return enable_reflection;}

	Camera* reflectionCamera(){return reflectionCamera_;}
	void setZReflection(float zreflection, float weight);
	float zReflection() const { return zreflection_; }

	bool IsIntensityShadow(){return shadow_intensity.r<0.99f || shadow_intensity.g<0.99f || shadow_intensity.b<0.99f;}
	const Color4f& GetShadowIntensity(){return shadow_intensity;}
	void SetShadowIntensity(const Color4f& f);
	Vect3f& GetShadowCameraOffset() { return shadow_camera_offset; }

	void SetCircleShadowIntensity(Color4c c){circle_shadow_intensity=c;}
	Color4c GetCircleShadowIntensity(){return circle_shadow_intensity;}
	
	Camera* GetMirageCamera(){return mirageCamera_;}

	void UpdateLists(int cur_quant);
private:
	void Animate();
	float				dTime;
	int					dTimeInt;
	Vect2i				Size;						// размер мира
	sGrid2d				UnkLightArray;				// массив источников света сцены
	sGrid2d				grid;
	QuatTree			tree;
	bool in_update_list;
	int prev_graph_logic_quant;

	Color4f shadow_intensity;
	Color4c circle_shadow_intensity;
	bool shadowEnabled_;
	Vect3f shadow_camera_offset;
	sBox6f fixShadowBox;
	MatXf fixShaddowLightMatrix;

	cTileMap* tileMap_;
	bool enable_fog_of_war;
	FogOfWar* pFogOfWar;

	Vect2i TileNumber;
	int tile_size;
	MTSection lock_draw;

	Camera*		shadowCamera_;
	Camera*		lightCamera_;
	Camera*		lightObjectsCamera_;
	Camera*		mirageCamera_;
	Camera*		floatZBufferCamera_;

	bool			enable_reflection;
	Camera*		reflectionCamera_;
	cTexture*		pReflectionRenderTarget;
	struct IDirect3DSurface9*	pReflectionZBuffer;
	float zreflection_;

	void AddReflectionCamera(Camera* camera);
	void AddLightCamera(Camera* camera);
	void AddShadowCamera(Camera* camera);
	void AddPlanarCamera(Camera* camera,bool light,bool toObjects);
	void AddMirageCamera(Camera* camera);
	void AddFloatZBufferCamera(Camera* camera);

	void CreateShadowmap();
    void FixShadowMapCamera(Camera* camera, Camera* shadowCamera);
	void fixShadowMapCameraTSM(Camera* camera, Camera* shadowCamera);
    void CalcShadowMapCamera(Camera* camera, Camera* shadowCamera);


	bool separately_sun_dir_shadow;
	Vect3f sun_direction;
	Vect3f sun_dir_shadow;
	Color4f sun_ambient,sun_diffuse,sun_specular;

	cTexture*  pSkyCubemap;
	static bool is_sky_cubemap;
	vector<cObject3dx*> visible_objects;

	struct CircleShadow
	{
		Vect3f pos;
		float radius;
		Color4c color;
	};
	vector<CircleShadow> circle_shadow;
	bool TraceUnified(const Vect3f& in_start,const Vect3f& in_dir,Vect3f *pTrace,bool clamp_by_dir);
	friend class CameraPlanarLight;

	vector<ListSimply3dx> simply_objects;
	void AttachSimply3dx(cSimply3dx* pObj);
	void DetachSimply3dx(cSimply3dx* pObj);
	void RemoveEmptyStaticSimply3dx();

	void BuildTree();
	void CaclulateLightAmbient();

	Vect2f CalcZMinZMaxShadowReciver();

	///////////// Для создания/удаления объектов в правильный момент при многопоточности.
	MTSection critial_attach;

	struct AddData
	{
		int quant;
		BaseGraphObject* object;
	};

	vector<AddData> add_list;
	struct sErase
	{
		int quant;
		list<BaseGraphObject*> erase_list;
	};
	list<sErase> erase_list;

	void AttachObjReal(BaseGraphObject *UObj);
	void DetachObjReal(BaseGraphObject *UObj);
	void mtUpdate(int quant);

	/////////////
	virtual void deleteManagedResource();
	virtual void restoreManagedResource();
	void CreateReflectionSurface();
	void DeleteReflectionSurface();

	bool hide_lights;
	bool hide_selfillumination;

	friend Camera;
};

void SaveKindObjNotFree();

#endif
