#ifndef __ACTIONS_ENVIRONMENTAL_H__
#define __ACTIONS_ENVIRONMENTAL_H__

#include "Serialization/SerializationTypes.h"
#include "Serialization/RangedWrapper.h"
#include "TriggerEditor/TriggerExport.h"
#include "SourceBase.h"
#include "Triggers.h"
#include "Physics/WindMap.h"
#include "Starforce.h"
#include "Units/LabelObject.h"
#ifdef MAELSTROM_DATA
#include "Environment/EnvironmentColors.h"	// CoastSpritesAttributes, for ActionSetCoastSprites
#endif

struct ActionActivateSources : Action
{
	ActionActivateSources();
	void activate();
	void serialize(Archive& ar);

protected:
	typedef vector<LabelSource> Sources;
	Sources sources_;
	bool active_;
};

class ActionKillSource : public Action
{
	void activate();
	void serialize(Archive& ar);
	bool automaticCondition() const;

protected:
	LabelSource source_;
};

struct ActionDeactivateSources : ActionActivateSources
{
	ActionDeactivateSources();
};

/////////////////////////////////////////////////////

struct ActionActivateMinimapMark : Action
{
	ActionActivateMinimapMark() : active_(true) {}

	void activate();
	void serialize(Archive& ar);

protected:
	bool active_;
	LabelObject anchor_;
};

struct ActionDeactivateMinimapMarks : ActionActivateMinimapMark
{
	ActionDeactivateMinimapMarks(){
		active_ = false;
	}
};
/////////////////////////////////////////////////////
class ActionSetEffect : public Action
{
public:
	enum Effects {
		EFFECT_MONOCHROME,
		EFFECT_BLOOM,
		EFFECT_UNDER_WATER,
		EFFECT_DOF,
		EFFECT_COLORDODGE
	};

	ActionSetEffect();

	void serialize(Archive& ar);
	bool automaticCondition() const;
	void activate();

private:
	SwitchMode switchMode;
	Effects effects;
};


class ActionSetWind : public Action
{
public:
	WindMap::WindType windType;
	float angleDirection;
	float windPower;

	ActionSetWind();
	void serialize(Archive& ar);
	void activate(); 
};

struct ActionSetFallout : Action
{
	enum TypeFallout {
		NONE,
		RAIN,
		SNOW
	};
	TypeFallout type;
	float intencity;
	float time;
	float rainConstant;

	ActionSetFallout();
	void activate();
	void serialize(Archive& ar);
};

struct ActionSetFalloutType : Action
{
	ActionSetFallout::TypeFallout type;
	float intencity;
	float time;

	ActionSetFalloutType();
	void activate();
	void serialize(Archive& ar);
};

struct ActionSetFalloutFlood : Action
{
	float rainConstant;

	ActionSetFalloutFlood();
	void activate();
	void serialize(Archive& ar);
};

class ActionSetWaterLevel : public Action
{
public:
	ActionSetWaterLevel() 
	{	
		waterHeight = 0.f; 
		water_dampf_k = 1;
	}

	float waterHeight;
	int water_dampf_k;
	
	void activate(); 

	void serialize(Archive& ar)
	{
		__super::serialize(ar);
		ar.serialize(waterHeight, "waterHeight", "Уровень воды");
		ar.serialize(RangedWrapperi(water_dampf_k,1,15), "water_dampf_k", "Скорость течения");
	}
};


/////////////////////////////////////////////////////

struct ActionSetEnvironmentTime : Action
{
	float time;

	ActionSetEnvironmentTime();
	void activate();
	void serialize(Archive& ar);
};

/////////////////////////////////////////////////////
class ActionSetReflectSkyColor : public Action
{
public:
	ActionSetReflectSkyColor();

	void serialize(Archive& ar);
	void activate();

private:
	Color4c reflectSkyColor_;
	SwitchMode switchMode_;
};

class ActionSetTimeScale : public Action
{
public:
	ActionSetTimeScale();

	void serialize(Archive& ar);
	void activate();

private:
	float dayTimeScale_;
	float nightTimeScale_;
};

class ActionSetWaterOpacity : public Action
{
public:
	ActionSetWaterOpacity();

	void serialize(Archive& ar);
	void activate();

private:
	KeysColor opacityGradient_;
};

class ActionSetSilhouette : public Action
{
public:
	void activate(){};
};

#ifdef MAELSTROM_DATA
// Maelstrom's own action, dropped by 2008 along with Environment's copy of the coast sprite
// attributes -- the sprites are still here and still read from the world, only nothing could
// change them again once the world was loaded. Every screen of the main menu sets its own
// (14 call sites in Scripts\Content\Triggers\MAIN MENU.scr, the "Waves*" trigger of each
// screen's environment block), so without it the shoreline keeps the menu world's opening
// settings through every screen.
class ActionSetCoastSprites : public Action
{
public:
	ActionSetCoastSprites();

	void serialize(Archive& ar);
	void activate();

private:
	CoastSpritesAttributes coastSprites_;
};
#endif

class ActionSetFogOfWar : public Action
{
public:
	ActionSetFogOfWar();
	void activate();
	void serialize(Archive& ar);

private:
	SwitchModeTriple mode_;

};

struct ActionSetFog : Action 
{
	float fog_start,fog_end;
	bool fog_enable;

	ActionSetFog();
	void activate();
	void serialize(Archive& ar);
};

////////////////////////////////////////////////////

class ActionSetWaterColor : public Action
{
public:
	ActionSetWaterColor();
	void activate();
	bool workedOut();
	void serialize(Archive& ar);

private:
	float transitionTime_;
	InterpolationLogicTimer transitionTimer_;
	Color4c riverColor_, seaColor_;
	Color4c riverColorPrev_, seaColorPrev_;
};


#endif //__ACTIONS_ENVIRONMENTAL_H__

