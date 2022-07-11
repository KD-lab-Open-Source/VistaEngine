#ifndef _VISGENERIC_H_
#define _VISGENERIC_H_

#include "Render\Inc\IvisGenericInternal.h"
#include "Render\Inc\VisGenericDefine.h"

class cScene;
class cInterfaceRenderDevice;
class cTexture;

enum eShowType
{
	SHOW_NORMAL=0,
	SHOW_SHADOW,
	SHOW_REDLECTION,
	SHOW_TILEMAP,
	SHOW_OBJECT,
	SHOW_INFO,
	SHOW_MAX
};

class RENDER_API cVisGeneric : public UnknownClass
{
public:
	// инициализационная часть 
	cVisGeneric(bool multiThread);
	virtual ~cVisGeneric();
	cInterfaceRenderDevice* GetRenderDevice();
	// функции для работы со сценой
	virtual cScene* CreateScene();

	void serialize(Archive& ar);
	void editOption();

	// функции для работы со спрайтами
	virtual cTexture* CreateTexture(const char *TextureName);
	virtual cTexture* CreateTexture(int sizex,int sizey,bool alpha);//Всегда 32 битная текстура
	virtual cTexture* CreateRenderTexture(int width,int height,int attr=0,bool enable_assert=true);

	//Возвращает 32 битную текстуру, в которой хранится изображение экрана.
	virtual cTexture* CreateTextureScreen();

	void SetEffectLibraryPath(const char* effect_path,const char* texture_path);

	//
	void SetMipMapLevel(int);
	void SetDrawNumberPolygon(bool);
	void SetShowRenderTextureDBG(bool);
	void SetFavoriteLoadDDS(bool);
	void SetAnisotropic(int);
	int GetAnisotropic();
	int GetMaxAnisotropyLevel();

	//Уровень детализации трехмерных текстур. 
	//0 - просто читается с диска. 1 - в 2 раза меньше, и т.д.
	void SetTextureDetailLevel(int level);
	int GetTextureDetailLevel();

	void SetShadowType(bool shadowEnabled,int shadow_size);//shadow_size=0..4
	bool shadowEnabled() const;
	bool PossibilityShadowMapSelf4x4();
	void SetShadowMapSelf4x4(bool b4x4);

	//Выбирает тени для объектов - OST_SHADOW_NONE - нет теней, 
	//OST_SHADOW_CIRCLE - есть тени кружочками для всех объектов, для которых OST_SHADOW_CIRCLE или OST_SHADOW_REAL
	//OST_SHADOW_REAL - тени как они указаны в параметрах объекта.
	void SetMaximalShadowObject(ObjectShadowType type);
	ObjectShadowType GetMaximalShadowObject()const{return maximal_shadow_object;}


	//Попиксельно либо повертексно вычислять освещение для земли.
	void setTileMapVertexLight(bool vertexLight);

	//Включить мелкодетальную текстуру.
	void SetTilemapDetail(bool b);
	bool GetTilemapDetail();

	void SetMapLevel(float);//0..100 Детализация карты.

	//Силуэты у объектов. Если центр объекта не видим, 
	//и он закрывается землёй либо оъкектом, который не может генерировать силуэт,
	//то силуэт видим.
	void EnableSilhouettes(bool enable); 
	bool IsSilhouettesEnabled() const { return silhouettes_enabled; }
	void SetSilhouetteColor(int index, Color4c color);
	Color4c GetSilhouetteColor(int index) const;
	int GetSilhouetteColorCount() const;

	//
	void SetEnableBump(bool enable);
	bool GetEnableBump();
	bool PossibilityBump();

	//
	bool PossibilityOcclusion();
	void EnableOcclusion(bool b);

	void SetFloatZBufferType(int type);
	int GetFloatZBufferType();

	void SetUseTextureCache(bool enable);
	bool GetUseTextureCache();
	void SetUseMeshCache(bool enable);
	bool GetUseMeshCache();

	void SetEnableDOF(bool enable);
	bool GetEnableDOF();

	bool PossibilityBumpChaos();

	void SetUseLogicQuant(bool use){use_logic_quant=use;}
	bool GetUseLogicQuant(){return use_logic_quant;}

	//Только в логическом потоке
	void SetLogicQuant(int quant){logic_quant=quant;}
	int GetLogicQuant(){return logic_quant;};

	//Только в графическом потоке
	void SetGraphLogicQuant(int quant){graph_logic_quant=quant;}
	int GetGraphLogicQuant(){return graph_logic_quant;};

	void SetGlobalParticleRate(float r);

	//Для эффектов, которые автоматически аттачатся к моделям.
	const char* GetEffectPath()const{return effect_path.c_str();};
	const char* GetEffectTexturePath()const{return effect_texture_path.c_str();};

	class cLib3dx* GetObjLib(){return Lib3dx;}

	//Модель может исчезать в зависимости от расстояния.
	float GetHideFactor();
	void SetHideFactor(float hide_factor);
	bool GetHideSmoothly();
	void SetHideSmoothly(bool hideSmoothly);//true - модель исчезает становясь постепенно прозрачной.
	float GetHideRange();
	void SetHideRange(float hide_range);//Только если HideSmoothly=true. За какой промежуток расстояния модель исчезает.

	bool GetShadowTSM();
	void SetShadowTSM(bool enable);

	bool IsMultithread(){return is_multithread;};

	class CameraShader* GetShaders(){return shaders;}

private:
	virtual void SetData(cInterfaceRenderDevice *pData1);

	int	 logic_quant,graph_logic_quant;
	bool use_logic_quant;

	class cLib3dx			*Lib3dx;
	class cLibSimply3dx		*LibSimply3dx;
	friend class cD3DRender;
	class CameraShader* shaders;

	void InitShaders();
	void ReleaseShaders();

	string effect_path,effect_texture_path;

	bool	 silhouettes_enabled;
	Color4c silhouette_color;
	std::vector<Color4c> silhouette_colors;

	bool is_multithread;

	ObjectShadowType maximal_shadow_object;

	string optionFileName_;
};

extern RENDER_API class cVisGeneric		*gb_VisGeneric;

///////////////////////////////////////////////////////

extern bool Option_VSync;
extern int	Option_MipMapLevel;
extern int	Option_TextureDetailLevel;
extern bool	Option_DrawNumberPolygon;
extern bool Option_shadowEnabled;
extern int	Option_ShadowSizePower;
extern float Option_MapLevel;
extern int	Option_ShowRenderTextureDBG;
extern bool	Option_FavoriteLoadDDS;
extern bool	Option_EnableBump;
extern float Option_ParticleRate;
extern bool	Option_tileMapVertexLight;
extern bool	Option_DetailTexture;
extern int	Option_FloatZBufferType;
extern bool	Option_EnableOcclusionQuery;
extern bool Option_ShowType[SHOW_MAX];
extern bool	Option_DprintfLoad;

extern float Option_HideFactor;
extern float Option_HideRange;
extern bool Option_HideSmoothly;

extern bool Option_UseTextureCache;
extern bool Option_UseMeshCache;
extern bool Option_UseDOF;
extern bool Option_AccessibleZBuffer;
extern bool	Option_EnablePreload3dx;

extern bool	Option_EnablePerfhud;
extern bool	Option_EnablePureDevice;
extern int Option_cameraDebug;

extern bool	Option_filterShadow;
extern bool	Option_shadowTSM;
extern float Option_trapezoidFactor;
extern bool Option_showDebugTSM;
extern float Option_reflectionOffset;

extern float AlphaMaxiumBlend;
extern float AlphaMiniumShadow;

struct DebugShowSwitch
{
	bool tilemap;
	bool water;
	bool objects;
	bool simplyObjects;
	bool grass;
	bool effects;
	DebugShowSwitch();
	void serialize(Archive& ar);
};

extern DebugShowSwitch debugShowSwitch;

////////////////////////////////////////////



#endif // _VISGENERIC_H_
