#ifndef __ENVIRONMENT_COLORS_H__
#define __ENVIRONMENT_COLORS_H__

#include "Render\src\NParticleKey.h"
#include "Physics\WindMap.h"

struct SunMoonAttribute{
	SunMoonAttribute();
	void serialize(Archive& ar);

	string SunName;
	string MoonName;
	float sunSize;
	float moonSize;
};

enum SkyElementType{
	SKY_ELEMENT_NIGHT,
	SKY_ELEMENT_DAY,
	SKY_ELEMENT_DAYNIGHT
};

struct SkyName{
	SkyName()
	: type(SKY_ELEMENT_DAYNIGHT)
	{}
	void serialize(Archive& ar);

	string Name;
	SkyElementType type;
};

struct SkyObjAttribute{
	SkyObjAttribute(); // CONVERSION
	void serialize(Archive& ar);
	vector<SkyName> sky_elements_names;
};

class Archive;

class ShadowingOptions 
{
public:
	ShadowingOptions ();
	ShadowingOptions (float _user_ambient_factor, float _user_ambient_maximal, float _user_diffuse_factor);
	inline bool operator==(const ShadowingOptions& rhs) const {
		const float small_value = 0.0001f;
		return (abs (ambientFactor - rhs.ambientFactor) < small_value &&
				abs (ambientMax - rhs.ambientMax) < small_value &&
				abs (diffuseFactor - rhs.diffuseFactor) < small_value);
	}
	void serialize(Archive& ar);

	float ambient(const Color4f& sunColor) const {
		float colorAvr = (sunColor.r + sunColor.g + sunColor.b)/3.0f;
	 	return min(colorAvr*ambientFactor, ambientMax);
	}

	void scaleDiffuse(Color4f& color) {
		color.r *= diffuseFactor;
		color.g *= diffuseFactor;
		color.b *= diffuseFactor;
	}

private:
	float ambientFactor;
	float ambientMax;
	float diffuseFactor;
};

class EnvironmentTimeColors
{
public:
	EnvironmentTimeColors();
	void serialize(Archive& ar);
	void mergeColor(KeysColor& out/*0.00-24.00*/,const KeysColor& day/*6.00-18.00*/,const KeysColor& night/*18.00-6.00*/);

	KeysColor fone_color;
	KeysColor reflect_sky_color;
	KeysColor sun_color;
	KeysColor fog_color;
	KeysColor shadow_color;
	KeysColor circle_shadow_color;

	float shadow_intensity;
	float shadowDecay;
	ShadowingOptions shadowing;
	ShadowingOptions objectShadowing;

	float slant_angle;
	float latitude_angle;
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
	Color4c color_;

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

#endif //__ENVIRONMENT_COLORS_H__
