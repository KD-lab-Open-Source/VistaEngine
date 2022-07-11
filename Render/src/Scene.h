#ifndef __SCENE_H_INCLUDED__
#define __SCENE_H_INCLUDED__
#include "UnkLight.h"
#include "czplane.h"
#include "NParticle.h"
#include "..\3dx\Node3dx.h"
#include "..\3dx\Simply3dx.h"

//class UnitBase;
class cFogOfWar;
class cStaticSimply3dx;
class FunctorMapZ;
#include "VisGrid2d.h"

struct ListSimply3dx
{
	cStaticSimply3dx* pStatic;
	vector<cSimply3dx*> objects;
};

class ContainerMiniTextures
{
	vector<string> minitexture_file_name;
	vector<cTexture*> pTextures;
public:
	~ContainerMiniTextures();
	cTexture* Texture(int i);
	const vector<string> &TexNames(){return minitexture_file_name;}
	void SetTexture(int i, const char* fn);
	void Resize(int sz);
};

class cScene : public cUnknownClass, public cDeleteDefaultResource
{
public:
	cScene();
	~cScene();
	// отрисовка
	void Compact();  //Удаляет объектя, находящиеся в кеше, на которые нет внешних ссылок.
	void Draw(cCamera *UCamera);		// отрисовка указанной части мира

	//Время в милисекундах в этих функциях.
	void SetDeltaTime(float dTime);//в милисекундах
	float GetDeltaTime()const{return dTime;};
	int GetDeltaTimeInt()const{return dTimeInt;};

	cCamera* CreateCamera();
	// функции для работы с объектами
	cObject3dx* CreateObject3dx(const char* fname,const char *TexturePath=NULL,bool interpolated=false);
	cObject3dx* CreateLogic3dx(const char* fname, bool interpolated = false);
	cSimply3dx* CreateSimply3dx(const char* fname,const char* visible_group=NULL,const char *TexturePath=NULL);

	//После вызова CreateObject3dxNoAttach нужно будет AttachObj(pObject) вызвать, чтобы прилинковать объект к сцене.
	cObject3dx* CreateObject3dxDetached(const char* fname,const char *TexturePath=NULL,bool interpolated=false);
	cSimply3dx* CreateSimply3dxDetached(const char* fname,const char* visible_group=NULL,const char *TexturePath=NULL);
	bool CreateDebrisesDetached(const c3dx* model,vector<cSimply3dx*>& debrises);
	// функции для работы с источниками света, влияющими на освещение объектов текущей сцены
	//Attribute - сумма флагов из eAttributeLight
	cUnkLight* CreateLight(int Attribute=0, const char* TextureName = 0);
	cUnkLight* CreateLight(int Attribute,cTexture *pTexture);

	cUnkLight* CreateLightDetached(int Attribute=0, const char* TextureName = 0);
	cUnkLight* CreateLightDetached(int Attribute,cTexture *pTexture);
	// функции для работы со следами
//	class cTrail* CreateTrail(const char* TextureName,float TimeLife=1000.f);
	// функции для работы с системой частиц
	cEffect* CreateEffectDetached(EffectKey& el,c3dx* models,bool auto_delete_after_life=false);

	// функции для работы с полигональным объектом
	cPlane* CreatePlaneObj();

	class cChaos* CreateChaos(Vect2f size,LPCSTR str_tex0,LPCSTR str_tex1,LPCSTR str_bump,int tile,bool enable_bump);

	// функции для работы с диспетчером регионов
	class FieldDispatcher* CreateForceFieldDispatcher(int xmax,int ymax, int zeroLayerHeight, const char* TextureName1=0, const char* TextureName2=0);
	class cExternalObj* CreateExternalObj(ExternalObjFunction func, const char* TextureName1=0);

//	cBaseGraphObject* CreateElasticSphere(const char* TextureName1=0, const char* TextureName2=0);

	// функции для работы с картой мира
	virtual class cTileMap* CreateMap(class TerraInterface* terra,int zeroplastnumber=1);
	virtual cFogOfWar* CreateFogOfWar();
	void EnableFogOfWar(bool enable);
	bool IsFogOfWarEnabled(){return enable_fog_of_war;}
	cFogOfWar* GetFogOfWar(){return enable_fog_of_war?pFogOfWar:NULL;};

	//Trace переименовали в TraceSegment
	virtual bool TraceSegment(const Vect3f& pStart,const Vect3f& pFinish,Vect3f *pTrace=0);
	virtual bool TraceDir(const Vect3f& pStart,const Vect3f& pDir,Vect3f *pTrace=0);

	//Добавить/удалить объект из сцены. 
	void AttachObj(cBaseGraphObject *UObj);
	void DetachObj(cBaseGraphObject *UObj);

	// доступ к переменным
	inline int GetNumberLight()									{ return UnkLightArray.size(); }
	inline cUnkLight* GetLight(int number)						{ return (cUnkLight*)UnkLightArray[number]; }

	class cTileMap *GetTileMap(){return TileMap;}
	

	inline Vect2i GetWorldSize(){return Size;}
	void DisableTileMapVisibleTest();
	void DeleteAutoObject();

	MTSection& GetLockDraw(){return lock_draw;}

	void SetSun(Vect3f direction,sColor4f ambient,sColor4f diffuse,sColor4f specular);
	void SetSunShadowDir(Vect3f direction);
	void SetSunDirection(Vect3f direction);
	void SetSunColor(sColor4f ambient,sColor4f diffuse,sColor4f specular);
	const Vect3f& GetSunDirection()const{return sun_direction;}
	const sColor4f& GetSunAmbient()const{return sun_ambient;}
	const sColor4f& GetSunDiffuse()const{return sun_diffuse;}
	const sColor4f& GetSunSpecular()const{return sun_specular;}
	sColor4f GetPlainLitColor();//Для объектов, у которых нормаль направленна вверх, или они освещаются просто цветом.

	void HideAllObjectLights(bool hide);//Днем ночные источники света не горят.
	void HideSelfIllumination(bool hide);//Днем самосвечения не нужно.

	void GetLighting(sColor4f &Ambient,sColor4f &Diffuse,sColor4f &Specular,Vect3f &LightDirection);
	void GetLighting(Vect3f *LightDirection);
	void GetLightingShadow(Vect3f *LightDirection);

	void SetSkyCubemap(cTexture* pTexture);
	cTexture* GetSkyCubemap(){return pSkyCubemap;};
	static void DisableSkyCubeMap(){is_sky_cubemap=false;};
	static bool IsSkyCubemap(){return is_sky_cubemap;}

	void AddCircleShadow(const Vect3f& pos,float r,sColor4c intensity);//internal

	//GetTilemapDetailTextures Плохой, очень плохой интерфейс у этой функции, должна быть в tilemap либо еще где. Ну и не так выглядеть.
	ContainerMiniTextures& GetTilemapDetailTextures();

	void GetAllObject3dx(vector<cObject3dx*>& objectsList);
	vector<ListSimply3dx>& GetAllSimply3dxList();
	void GetAllEffects(vector<cEffect*>& effectsList);

	bool DebugInUpdateList();

	void EnableReflection(bool enable);
	bool IsReflection(){return enable_reflection;}

	cCamera* GetReflectionCamera(){return ReflectionDrawNode;}

	bool IsIntensityShadow(){return shadow_intensity.r<0.99f || shadow_intensity.g<0.99f || shadow_intensity.b<0.99f;}
	const sColor4f& GetShadowIntensity(){return shadow_intensity;}
	void SetShadowIntensity(const sColor4f& f);
	Vect3f& GetShadowCameraOffset() { return shadow_camera_offset; }

	void SetCircleShadowIntensity(sColor4c c){circle_shadow_intensity=c;}
	sColor4c GetCircleShadowIntensity(){return circle_shadow_intensity;}
	int CheckLightMapType();
	cTexture* GetShadowMap();
	cTexture* GetLightMap();
	cTexture* GetLightMapObjects();
	struct IDirect3DSurface9* GetZBuffer();

	Vect4f& GetPlanarNodeParam(){return planar_node_param;}
	void SetZReflection(float zreflection_){zreflection=zreflection_;}
	cCamera* GetMirageCamera(){return mirage_draw_node;}

	void SelectWaterFunctorZ(FunctorGetZ* p);
	FunctorGetZ* GetTerraFunctor()const{return (FunctorGetZ*)TerraFunctor;}
	FunctorGetZ* GetWaterFunctor()const{return WaterFunctor;}
private:
	struct FixShadowDrawNodeParam {
		sBox6f box;
		MatXf light_matrix;

		FixShadowDrawNodeParam() {
			box.min.set(0,0,0);
			box.max.set(1,1,1);
			light_matrix=MatXf::ID;
		}
	} fix_shadow;

	void Animate();
	float				dTime;
	int					dTimeInt;
	Vect2i				Size;						// размер мира
	sGrid2d				UnkLightArray;				// массив источников света сцены
	sGrid2d				grid;
	QuatTree			tree;
	bool in_update_list;
	int prev_graph_logic_quant;

	sColor4f shadow_intensity;
	sColor4c circle_shadow_intensity;
	int lightmap_type;
	Vect3f shadow_camera_offset;

	class cTileMap *TileMap;
	bool enable_fog_of_war;
	cFogOfWar* pFogOfWar;

	Vect2i TileNumber;
	int tile_size;
	MTSection lock_draw;

	Vect4f planar_node_param;//x,y=left,top, z=1/sizex,w=1/sizey

	cCamera*		shadow_draw_node;
	cCamera*		light_draw_node;
	cCamera*		light_objects_draw_node;
	cCamera*		mirage_draw_node;
	cCamera*		floatZBufferCamera;

	bool			enable_reflection;
	cCamera*		ReflectionDrawNode;
	cTexture*		pReflectionRenderTarget;
	struct IDirect3DSurface9*	pReflectionZBuffer;
	float zreflection;

	bool disable_tilemap_visible_test;

	void AddReflectionCamera(cCamera *DrawNode);
	void AddLightCamera(cCamera *DrawNode);
	void AddShadowCamera(cCamera* DrawNode);
	void AddPlanarCamera(cCamera *DrawNode,bool light,bool toObjects = false);
	void AddMirageCamera(cCamera* DrawNode);
	void AddFloatZBufferCamera(cCamera* DrawNode);

	void CreateShadowmap();
    void FixShadowMapCamera(cCamera *DrawNode, cCamera *shadow_draw_node);
    void CalcShadowMapCamera(cCamera *DrawNode, cCamera *shadow_draw_node);
	void CalcShadowMapCameraTSM(cCamera *DrawNode, cCamera *ShadowDrawNode);

	void UpdateLists(int cur_quant);

	ContainerMiniTextures MiniTextures;

	bool separately_sun_dir_shadow;
	Vect3f sun_direction;
	Vect3f sun_dir_shadow;
	sColor4f sun_ambient,sun_diffuse,sun_specular;

	cTexture*  pSkyCubemap;
	static bool is_sky_cubemap;
	vector<cObject3dx*> visible_objects;

	struct CircleShadow
	{
		Vect3f pos;
		float radius;
		sColor4c color;
	};
	vector<CircleShadow> circle_shadow;
	bool TraceUnified(const Vect3f& in_start,const Vect3f& in_dir,Vect3f *pTrace,bool clamp_by_dir);
	friend class cD3DRender;
	friend class cWater;

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
		cBaseGraphObject* object;
	};

	vector<AddData> add_list;
	struct sErase
	{
		int quant;
		list<cBaseGraphObject*> erase_list;
	};
	list<sErase> erase_list;

	void AttachObjReal(cBaseGraphObject *UObj);
	void DetachObjReal(cBaseGraphObject *UObj);
	void mtUpdate(int quant);

	/////////////
	virtual void DeleteDefaultResource();
	virtual void RestoreDefaultResource();
	void CreateReflectionSurface();
	void DeleteReflectionSurface();

	cCamera* reflection_copy;
	sBox6f reflection_bound_box;

	bool hide_lights;
	bool hide_selfillumination;

	FunctorMapZ* TerraFunctor;
	FunctorGetZ* WaterFunctor;
};

void SaveKindObjNotFree();

//Подстава в том, что модели не сразу удаляются.
//Поэтому модели нельзя выгружать во время игры
UNLOAD_ERROR UnloadSharedResource(const char* file_name,bool logging=true);


#endif
