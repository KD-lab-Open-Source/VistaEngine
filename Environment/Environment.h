#ifndef __ENVIRONMENT_H_INCLUDED__
#define __ENVIRONMENT_H_INCLUDED__

#include "Grid2D.h"
#include "SwapVector.h"
#include "Handle.h"
#include "UnitLink.h"
#include "Timers.h"
#include "Starforce.h"
#include "..\Render\inc\umath.h"
#include "EnvironmentColors.h"
#include "ShadowWrapper.h"

const float rain_multiplicator=0.01f;

class cFogOfWar;
class cChaos;
class cScene;
class cTileMap;
class cFallout;
class cWater;
class cWaterBubble;
class cCoastSprites;
class cCloudShadow;
class cEnvironmentTime;
class cTemperature;
class cFixedWavesContainer;
class cEnvironmentEarth;
class LensFlareRenderer;
class UnitReal;
class RegionDispatcher;
class Archive;
class cFallLeaves;

class UnderWaterEffect;
class BloomEffect;
class MonochromeEffect;
class DOFEffect;
class MirageEffect;
class ColorDodgeEffect;
class PostEffectManager;

class ParameterCustom;
class MissionDescription;
struct CameraRestriction;
struct CameraBorder;
class GrassMap;

class SoundAttribute;
class SoundController;

enum SourceType;
class SourceAttribute;
class SourceBase;
class BaseUniverseObject;
class UnitBase;
class Anchor;

class ShowChangeController;
typedef ShareHandle<ShowChangeController> SharedShowChangeController;

class SoundEnvironmentManager;

class cFlash{
	bool active_;
	sColor4c color_;
	
	bool inited_;
	float intensity_[2];
	int count_;
	float intensitySum_;
	InterpolationTimer timer_;
public:
	cFlash();
	~cFlash();
	void init(float _intensity);
	void setActive(bool _active);
	
	void setColor(sColor4c _color);
	
	void setIntensity();
	void draw();
	void addIntensity(float _intensity);
	void addFlash() {count_++;}
	void decFlash() {count_--; if(count_<0)count_=0;}
};

typedef Grid2D<SourceBase, 7, GridVector<SourceBase, 8> > SourceBaseGridType;

class Environment{
public:
	Environment(int worldSizeX, int worldSizeY, cScene* scene, cTileMap* terrain, const MissionDescription& description);
	~Environment();

	void logicQuant();
	void graphQuant(float dt);
	void drawUI(float dt);
	void drawPostEffects(float dt);
	void drawBlackBars(float opacity = 1.0f);

	/// создаёт, устанавливает и запускает источник
	/// при некоторых условиях может вернуть ноль
	SourceBase* createSource(const SourceAttribute* attribute, const Se3f& pose, bool allow_limited_lifetime = true, bool* startFlag = 0);
	SourceBase* addSource (const SourceBase* original);
	void flushNewSources();

	void setSourceOnMouse(const SourceBase* source);

	/// добавляет на мир якорь для привязки
	Anchor* addAnchor();
	Anchor* addAnchor(const Anchor* original);

	void deselectAll();
	void deleteSelected();
	void showEditor();

	cScene* scene() { return scene_; }
	cWater* water() { return water_; }
	GrassMap* grass() {return grassMap;}
	cWaterBubble* waterBubble() {return waterBubble_;}
	cFlash* flash() {return flash_;}
	cFallout* fallout() { return fallout_; }
	cCoastSprites* GetCoastSprites() { return pCoastSprite; }
	WaterPlumeAttribute& waterPlumeAtribute(){ return environmentAttributes_.waterPlume_;}
	cTemperature* temperature(){ return temperature_;}
	LensFlareRenderer* lensFlare(){ return lensFlare_; }
	cFogOfWar* fogOfWar(){ return 0;} // fogOfWar_; } // Временный фикс тумана
	cFallLeaves* fallLeaves(){ return fallLeaves_; }
	ShadowWrapper& shadowWrapper() { return shadowWrapper_; }
	bool isVisibleUnderForOfWar(const UnitBase*, bool checkAllwaysVisible = false) const;
	bool isPointVisibleUnderForOfWar(const Vect2i& pos) const;

	cEnvironmentTime* environmentTime() { return environmentTime_; }
	float getTime() const;
	bool isDay() const;
	bool dayChanged() const;

	PostEffectManager* PEManager(){return PEManager_;}
	bool isUnderWaterSilouette(const Vect3f& pos);

	STARFORCE_API void serializeSourcesAndAnchors(Archive& ar);
	STARFORCE_API void serializeParameters(Archive& ar);
	STARFORCE_API void serialize(Archive& ar);

	SourceBaseGridType sourceGrid;

	typedef SwapVector< ShareHandle<SourceBase> > Sources;
	void getTypeSources(SourceType type, Sources& out);
	Sources& sources(){return sources_;};

	typedef SwapVector<SharedShowChangeController> ShowChangeControllers;
	ShowChangeController* addShowChangeController(const ShowChangeController& ctrl);

	cFixedWavesContainer* fixedWaves() {return fixedWaves_;}

	void setShowFogOfWar (bool show);

	string sourceNamesComboList(SourceType sourceType);
	SourceBase* getSource(SourceType sourceType, const char* sourceName);


	typedef SwapVector<ShareHandle<Anchor> > Anchors;
	Anchor* findAnchor(const char* anchorName) const;
	Anchors& anchors(){return anchors_;}

	// flags
	static bool flag_EnableTimeFlow;
	static bool flag_ViewWaves;

	bool waterIsLava() const { return water_is_lava; }
	bool waterIsIce() const { return anywhereIce; }
	float waterPathFindingHeight() { return waterPFHeight; }

	void SetWaterTechnique();

	bool show_fog_of_war;

	void showDebug() const;
	void drawCameraRestriction(bool world, sColor4c& color);

	float GetGameFrustrumZMin() const { return environmentAttributes_.game_frustrum_z_min_; };
	float GetGameFrustrumZMaxHorizontal() const { return environmentAttributes_.game_frustrum_z_max_horizontal_; };
	float GetGameFrustrumZMaxVertical() const { return environmentAttributes_.game_frustrum_z_max_vertical_; };

	bool soundAttach(const SoundAttribute* sound, const BaseUniverseObject *obj);
	void soundRelease(const SoundAttribute* sound, const BaseUniverseObject *obj);
	bool soundIsPlaying(const SoundAttribute* sound, const BaseUniverseObject *obj);

	float minimapAngle() const { return minimapAngle_; }

	void setFogEnabled(bool enable) { fog_enable_ = enable; }
	void setFogTempDisabled(bool disable){ fogTempDisabled_ = disable; }
	bool isFogEnabled() const { return fog_enable_; }
	bool isFogTempDisabled() const { return fogTempDisabled_; }
	void setFogStart(float fogStart) { environmentAttributes_.fog_start_ = fogStart; }
	void setFogEnd(float fogEnd) { environmentAttributes_.fog_end_ = fogEnd; }
	float fogStart() const { return environmentAttributes_.fog_start_; }
	float fogEnd() const { return environmentAttributes_.fog_end_; }

	const CameraRestriction& cameraRestriction() const{ return cameraRestriction_; }
	void setCameraRestriction(const CameraRestriction& restriction, bool ownCameraRestriction);
	bool ownCameraRestriction() const{ return ownCameraRestriction_; }
	
	const CameraBorder& cameraBorder() const{ return cameraBorder_; }
	void setCameraBorder(const CameraBorder& border);
	SoundEnvironmentManager* soundEnvironmentManager() {return sndEnvManager;}
	void SetDofParams(Vect2f& params);

	void setTimeScale(float dayTimeScale, float nightTimeScale);

	const CoastSpritesAttributes& coastSpritesAttributtes() const { return environmentAttributes_.coastSprites_; }
	void setCoastSpritesAttributtes(const CoastSpritesAttributes& coastSprites);
private:
	bool ownAttributes_;
	EnvironmentAttributes environmentAttributes_;

	cScene* scene_;
	cWater* water_;
	cTemperature* temperature_;
	cWaterBubble* waterBubble_;
	cEnvironmentTime* environmentTime_;
	cCoastSprites* pCoastSprite;
	cFlash* flash_;
	LensFlareRenderer* lensFlare_;

	cFogOfWar* fogOfWar_;
	bool  fog_enable_;
	bool  fogTempDisabled_;

	int waterHeight;
    string waterSourceBubbleTexture;
	bool water_is_lava;
	string ice_snow_texture;
	string ice_bump_texture;
	string lava_texture;

	Sources sources_;
	Sources newSources_;

	friend void fCommandAddShowChangeController(XBuffer& stream);
	ShowChangeControllers showChangeControllers_;

	// для показа на КРИ2006
	UnitLink<SourceBase> sourceOnMouse_;

	Anchors anchors_;

	cFixedWavesContainer* fixedWaves_;

	cFallout* fallout_;
	cChaos* chaos;
	//WaterPlumeAttribute waterPlumeAttribute_;
	cCloudShadow* cloud_shadow;
	PostEffectManager* PEManager_;
	GrassMap *grassMap;
	cFallLeaves* fallLeaves_;
	
	cEnvironmentEarth* env_earth;
    string chaosWorldGround0;
    string chaosWorldGround1;
    string chaosOceanBump;
    string env_earth_texture;
	float waterPFHeight;
	bool need_alpha_tracking;

	bool ownCameraRestriction_;
	CameraRestriction& cameraRestriction_;
	CameraBorder& cameraBorder_;

	float minimapAngle_;

	void reload();
	void clearSources();

	string water_ice_snow_texture;
	string water_ice_bump_texture;
	string water_ice_cleft_texture;
	bool anywhereIce;

	typedef SwapVector<SoundController> SoundControllers;
	SoundControllers sounds_;

	sColor4f underWaterColor;
	sColor4f underWaterFogColor;
	Vect2f underWaterFogPlanes;
	string underWaterTextureName;
	float underWaterSpeedDistortion;
	bool underWaterAlways;

	float bloomLuminance;
	bool enableBloom;
	bool enableDOF;
	Vect2f DofParams;
	float dofPower;

	ShadowWrapper shadowWrapper_;

	SoundEnvironmentManager* sndEnvManager;

	friend struct ShowDebugInterface;
};


extern Environment* environment;

RegionDispatcher& TileMapRegion(int i);

#endif
