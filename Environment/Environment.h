#ifndef __ENVIRONMENT_H_INCLUDED__
#define __ENVIRONMENT_H_INCLUDED__

#include "Timers.h"
#include "EnvironmentColors.h"

class FogOfWar;
class cChaos;
class cScene;
class cTileMap;
class cFallout;
class cWater;
class cWaterBubble;
class cCoastSprites;
class cCloudShadow;
class EnvironmentTime;
class cTemperature;
class cFixedWavesContainer;
class cEnvironmentEarth;
class LensFlareRenderer;
class Archive;
class cFallLeaves;
class PostEffectUnderWater;
class PostEffectBloom;
class PostEffectMonochrome;
class PostEffectDOF;
class PostEffectMirage;
class PostEffectColorDodge;
class PostEffectManager;
class Flash;
struct CameraRestriction;
struct CameraBorder;
class GrassMap;
class FieldOfViewMap;

enum SerializeFilter
{
	SERIALIZE_WORLD_DATA = 1 << 0,
	SERIALIZE_PRESET_DATA = 1 << 1,
	SERIALIZE_GLOBAL_DATA = 1 << 2,
};

enum Outside_Environment
{
	ENVIRONMENT_NO,
	ENVIRONMENT_CHAOS,
	ENVIRONMENT_WATER,
	ENVIRONMENT_EARTH,
};

class Environment
{
public:
	Environment::Environment(cScene* scene, cTileMap* terrain, bool isWater, bool isFogOfWar, bool isTemperature);
	~Environment();

	void setTileMap(cTileMap* terrain);
	cTileMap* tileMap() const { return tileMap_; }

	void logicQuant();
	void graphQuant(float dt, Camera* camera);
	void drawPostEffects(float dt, Camera* camera);

	void showEditor();

	cScene* scene() { return scene_; }
	cWater* water() { return water_; }
	GrassMap* grass() {return grassMap;}
	cWaterBubble* waterBubble() {return waterBubble_;}
	Flash* flash() {return flash_;}
	cFallout* fallout() { return fallout_; }
	cCoastSprites* GetCoastSprites() { return pCoastSprite; }
	WaterPlumeAttribute& waterPlumeAtribute(){ return waterPlumeAtribute_; }
	cTemperature* temperature(){ return temperature_;}
	LensFlareRenderer* lensFlare(){ return lensFlare_; }
	FogOfWar* fogOfWar(){ return fogOfWar_; }
	cFallLeaves* fallLeaves(){ return fallLeaves_; }

	EnvironmentTime* environmentTime() { return environmentTime_; }
	float getTime() const;
	bool isDay() const;
	bool dayChanged() const;

	PostEffectManager* PEManager(){return PEManager_;}
	
	bool isVisibleUnderForOfWar(const Vect2i& pos) const;
	bool isUnderWaterSilouette(const Vect3f& pos);

	void serialize(Archive& ar);
	const char* presetName()const{ return presetName_.c_str(); }
	void setPresetName(const char* name) { presetName_ = name; }
	void loadPreset();
	void savePreset();
	void loadGlobalParameters();
	void saveGlobalParameters();

	cFixedWavesContainer* fixedWaves() {return fixedWaves_;}

	FieldOfViewMap* fieldOfViewMap() { return fieldOfViewMap_; }

	void drawCameraRestriction(bool world, Color4c& color);

	float GetGameFrustrumZMin() const { return game_frustrum_z_min_; }
	float GetGameFrustrumZMaxHorizontal() const { return game_frustrum_z_max_horizontal_; }
	float GetGameFrustrumZMaxVertical() const { return game_frustrum_z_max_vertical_; }

	void setFogEnabled(bool enable) { fog_enable_ = enable; }
	void setFogTempDisabled(bool disable){ fogTempDisabled_ = disable; }
	bool isFogEnabled() const { return fog_enable_; }
	bool isFogTempDisabled() const { return fogTempDisabled_; }
	void setFogStart(float fogStart) { fog_start_ = fogStart; }
	void setFogEnd(float fogEnd) { fog_end_ = fogEnd; }
	float fogStart() const { return fog_start_; }
	float fogEnd() const { return fog_end_; }

	const Color4f& minimapWaterColor() { return minimapWaterColor_; }

	float minimapZonesAlpha() const { return minimapZonesAlpha_; }

	bool show_fog_of_war;
	static bool flag_EnableTimeFlow;
	static bool flag_ViewWaves;

private:
	string presetName_;
	bool presetLoaded_;

	Outside_Environment outside_;
	float outsideHeight_;
	float fog_start_;
	float fog_end_;
	float game_frustrum_z_min_;
	float game_frustrum_z_max_vertical_;
	float game_frustrum_z_max_horizontal_;

	float hideByDistanceFactor_; 
	float hideByDistanceRange_;
	bool hideSmoothly_;

	bool effectHideByDistance_;
	float effectNearDistance_;
	float effectFarDistance_;

	float dayTimeScale_;
	float nightTimeScale_;
	int height_fog_circle_;

	WaterPlumeAttribute waterPlumeAtribute_;

	cScene* scene_;
	cTileMap* tileMap_;
	cWater* water_;
	cTemperature* temperature_;
	cWaterBubble* waterBubble_;
	EnvironmentTime* environmentTime_;
	cCoastSprites* pCoastSprite;
	Flash* flash_;
	LensFlareRenderer* lensFlare_;

	FogOfWar* fogOfWar_;
	bool  fog_enable_;
	bool  fogTempDisabled_;

    string waterSourceBubbleTexture;
	string ice_snow_texture;
	string ice_bump_texture;

	cFallout* fallout_;
	cChaos* chaos;
	cCloudShadow* cloud_shadow;
	PostEffectManager* PEManager_;
	GrassMap *grassMap;
	cFallLeaves* fallLeaves_;
	cFixedWavesContainer* fixedWaves_;
	UnknownHandle<FieldOfViewMap> fieldOfViewMap_;
	
	cEnvironmentEarth* env_earth;
    string chaosWorldGround0;
    string chaosWorldGround1;
    string chaosOceanBump;
    string env_earth_texture;

	string water_ice_snow_texture;
	string water_ice_bump_texture;
	string water_ice_cleft_texture;

	float minimapZonesAlpha_;

	Color4f minimapWaterColor_;
	Color4f underWaterColor;
	Color4f underWaterFogColor;
	Vect2f underWaterFogPlanes;
	string underWaterTextureName;
	float underWaterSpeedDistortion;
	bool underWaterAlways;

	float bloomLuminance;
	bool enableBloom;
	bool enableDOF;
	Vect2f DofParams;
	float dofPower;
};


extern Environment* environment;

#endif
