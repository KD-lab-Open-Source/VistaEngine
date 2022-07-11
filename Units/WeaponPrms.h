#ifndef __WEAPON_PROJECTILE_H_INCLUDED__
#define __WEAPON_PROJECTILE_H_INCLUDED__

#include "IronBullet.h"
#include "IronLegion.h"
#include "UnitPad.h"
#include "WeaponAttribute.h"
#include "Environment\ChainLightningController.h"

class AttributeProjectile;

/// параметры лучевого оружия
class WeaponBeamPrm : public WeaponPrm
{
public:
	WeaponBeamPrm();

	void serialize(Archive& ar);

	const SourceWeaponAttributes& sourceAttr() const { return sources_; }
	WeaponSourcesCreationMode sourcesCreationMode() const { return sourcesCreationMode_; }

	bool damageAtOnce() const { return damageAtOnce_; }

	float effectScale() const { return effectScale_; }
	EffectKey* effect(float scale, Color4c color) const;
	bool effectStopImmediately() const { return effectStopImmediately_; }

	int environmentDestruction() const { return environmentDestruction_; }

	bool needSurfaceTrace() const { return needSurfaceTrace_; }
	bool needUnitTrace() const { return needUnitTrace_; }
	bool needShieldTrace() const { return needShieldTrace_; }
	const EffectAttribute& shieldEffect() const { return shieldEffect_; }

	bool useChainEffect() const { return useChainEffect_; }
	const ChainLightningAttribute& chainLightningAttribute() const { return chainLightningAttribute_; }

	void initCache(WeaponPrmCache& cache) const;

	const TargetEffects& targetEffects() const { return targetEffects_; }

private:

	bool needSurfaceTrace_;
	bool needShieldTrace_;
	bool needUnitTrace_;

	bool damageAtOnce_;

	BitVector<EnvironmentType> environmentDestruction_;

	bool useChainEffect_;
	ChainLightningAttribute chainLightningAttribute_;

	/// спецэффект луча
	EffectReference effectReference_;
	/// обрывать спецэффект луча по окончании выстрела
	bool effectStopImmediately_;
	/// масштаб эффекта
	float effectScale_;
	/// красить луч в цвет легиона
	bool legionColor_;

	SourceWeaponAttributes sources_;
	WeaponSourcesCreationMode sourcesCreationMode_;

	EffectAttribute shieldEffect_;

	TargetEffects targetEffects_;
};

/// параметры оружия, действующего на зону на мире
class WeaponAreaEffectPrm : public WeaponPrm
{
public:
	WeaponAreaEffectPrm();

	/// позиционирование источников
	enum ZonePositionMode
	{
		/// вокруг себя
		ZONE_POS_OWNER,
		/// вокруг цели
		ZONE_POS_TARGET,
		/// привязать к крысе
		ZONE_POS_MOUSE
	};

	void serialize(Archive& ar);

	ZonePositionMode zonePositionMode() const { return zonePositionMode_; }

	const SourceWeaponAttributes& sources() const { return sources_; }

	int fowRadius() const { return fowRadius_; }

	void initCache(WeaponPrmCache& cache) const;

private:

	/// радиус видимости вокруг точки применения оружия
	int fowRadius_;

	SourceWeaponAttributes sources_;
	ZonePositionMode zonePositionMode_;
	
};

// --------------------------------------------------------------------------
class WeaponPadPrm : public WeaponPrm
{
public:
	enum WorkType
	{
		NONE = 0,	// ничего не делаем
		DIG_IN,		// копаем
		POUR_OUT	// высыпаем
	};

	WeaponPadPrm();

	void serialize(Archive& ar);

	const AttributePad* attrPad() const { return safe_cast<const AttributePad*>(attrPad_.get()); } 
	WorkType type() const { return type_; }

	int surfaceType() const { return surfaceType_; }
	const Toolser& toolser() const { return toolser_; }

	const EffectAttribute& linkToCursor() const { return linkToCursor_; }
	int linkNodeIndex() const { return linkNode_; }

	bool canPickItem(const UnitObjective* unit) const;
	bool canPickUnit(const UnitObjective* unit) const;

	float power() const { return power_; }
	float once() const { return digLenght_; }

	int fowRadius() const { return fowRadius_; }

	bool deselectByPower() const { return deselectByPower_; }

private:
	AttributePadReference attrPad_;
	WorkType type_;
	AttributeItemReferences pickedItems_;
	AttributeUnitReferences pickedUnits_;

	Toolser toolser_;
	typedef BitVector<TerrainType> SurfaceKindBitVector;
	SurfaceKindBitVector surfaceType_;

	EffectAttribute linkToCursor_;
	GenericObject3dxNode linkNode_;

	// длинна траншеи в единицах мира, которая полностью наполнит емкость / скорость в секундах за которую высыпается весь контейнер
	float power_;
	// максимальная длина единичного копания 
	float digLenght_;
	// сбрасывать оружие, после наполнения, опустощения емкости
	bool deselectByPower_;

	int fowRadius_;
};

/// параметры оружия - захвата
class WeaponGripPrm : public WeaponPrm
{
public:
	WeaponGripPrm();
	void serialize(Archive& ar);

	float horizontalImpulse() const { return horizontalImpulse_; }
	float verticalImpulse() const { return verticalImpulse_; }
	float forwardAngle() const { return forwardAngle_; }
	float dispersionAngle() const { return dispersionAngle_; }
	int startGripTime() const { return startGripTime_; }
	int finishGripTime() const { return finishGripTime_; }
	
private:
	float horizontalImpulse_;
	float verticalImpulse_;
	float forwardAngle_;
	float dispersionAngle_;
	int startGripTime_;
	int finishGripTime_;

};


/// параметры метательного оружия
class WeaponProjectilePrm : public WeaponPrm
{
public:
	enum MissileLaunchType
	{
		LAUNCH_SHOT_BEGIN,
		LAUNCH_SHOT_END
	};

	WeaponProjectilePrm();

	bool targetOnWaterSurface() const;
	void initCache(WeaponPrmCache& cache) const;

	void serialize(Archive& ar);

	bool needMissileVisualisation() const;
	const AttributeProjectile* missileID() const;

	MissileLaunchType missileLaunchType() const { return missileLaunchType_; }

private:
	bool needMissileVisualisation_;

	/// ID снаряда
	AttributeProjectileReference missileID_;
	MissileLaunchType missileLaunchType_;
};

class WeaponWaitingSourcePrm : public WeaponPrm
{
public:
	enum ExecuteType{
		MINING,
		DETONATE
	};

	WeaponWaitingSourcePrm();

	void serialize(Archive& ar);

	ExecuteType type() const { return type_; }
	const SourceWeaponAttribute& source() const { return source_; }

	int miningLimit() const { return miningLimit_;	}

	float delayTime() const { return delayTime_; }

private:
	ExecuteType type_;
	SourceWeaponAttribute source_;
	float delayTime_;
	int miningLimit_;
};

#endif
