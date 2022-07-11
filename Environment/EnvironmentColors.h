#ifndef __ENVIRONMENT_COLORS_H__
#define __ENVIRONMENT_COLORS_H__

class Archive;
#include "..\..\render\src\NParticleKey.h"
#include "..\Physics\WindMap.h"

class EnvironmentTimeColors
{
public:
	EnvironmentTimeColors();

	void serialize(Archive& ar);
	void serializeColors(Archive& ar,bool is_local);
	void ReplaceGlobal(EnvironmentTimeColors& global);

	CKeyColor fone_color;
	CKeyColor reflect_sky_color;
	CKeyColor sun_color;
	CKeyColor fog_color;
	CKeyColor shadow_color;
	CKeyColor circle_shadow_color;
	bool global_fone_color;
	bool global_reflect_sky_color;
	bool global_sun_color;
	bool global_fog_color;
	bool global_shadow_color;
	bool global_circle_shadow_color;
};
struct CoastSpriteSimpleAttributes
{
public:	
	CoastSpriteSimpleAttributes();
	void serialize(Archive& ar);

	float minSize_;
	float maxSize_;
	float deltaPos_;
	float beginDeep_;
	float endDeep_;
	float intensity1_;
	float intensity2_;
	string textureName_;
};
struct CoastSpriteMovingAttributes : public CoastSpriteSimpleAttributes
{
public:
	CoastSpriteMovingAttributes();
	void serialize(Archive& ar);

	float speed_;
};

struct CoastSpritesAttributes
{
public:
	CoastSpritesAttributes();

	void serialize(Archive& ar);
	bool modeStay_;
	bool modeMove_;
	float heightOverWater_;
	bool dieInCoast_;
	CoastSpriteSimpleAttributes simpleSprites_;
	CoastSpriteMovingAttributes movingSprites_;
};

struct FallaoutSnowAttributes
{
	FallaoutSnowAttributes();
	void serialize(Archive& ar);

	bool enable_;
	float size_;
	string textureName_;

};

struct FalloutRainRipplesAttributes
{
	FalloutRainRipplesAttributes();
	void serialize(Archive& ar);

	float size_;
	float time_;
	int minWaterHeight_;
	string textureName_;

};
struct FalloutRainAttributes : public FallaoutSnowAttributes
{
	FalloutRainAttributes();
	void serialize(Archive& ar);

	FalloutRainRipplesAttributes ripples_;
	sColor4c color_;

};
struct FalloutAttributes
{
	FalloutAttributes();
	void serialize(Archive& ar);

	FalloutRainAttributes rain_;
	FallaoutSnowAttributes snow_;
	float intensity_;
};

struct WaterPlumeAttribute
{
	float waterPlumeFrequency;
	string waterPlumeTextureName;

	WaterPlumeAttribute();
	void serialize(Archive& ar);
};
struct EnvironmentAttributes
{
	EnvironmentAttributes();
	void serialize(Archive& ar);

	sColor4c fogOfWarColor_;
	unsigned char scout_area_alpha_;
	int fogMinimapAlpha_;
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
	float rainConstant_;
	int water_dampf_k_;
	int height_fog_circle_;
	int miniDetailTexResolution_;

	EnvironmentTimeColors timeColors_;
	CoastSpritesAttributes coastSprites_;
	FalloutAttributes fallout_;
	WaterPlumeAttribute waterPlume_;
	WindMapAttributes windMap_;
};

#endif //__ENVIRONMENT_COLORS_H__
