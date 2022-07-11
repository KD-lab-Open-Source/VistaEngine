#ifndef __WEAPON_PROJECTILE_H_INCLUDED__
#define __WEAPON_PROJECTILE_H_INCLUDED__

#include "IronBullet.h"
#include "WeaponAttribute.h"
#include "..\Environment\ChainLightningController.h"

class AttributeProjectile;

/// параметры лучевого оружия
class WeaponBeamPrm : public WeaponPrm
{
public:
	WeaponBeamPrm();

	void serialize(Archive& ar);

	const SourceWeaponAttributes& sourceAttr() const { return sources_; }
	WeaponSourcesCreationMode sourcesCreationMode() const { return sourcesCreationMode_; }

	float effectScale() const { return effectScale_; }
	EffectKey* effect(float scale, sColor4c color) const;
	bool effectStopImmediately() const { return effectStopImmediately_; }

	int environmentDestruction() const { return environmentDestruction_; }

	bool needSurfaceTrace() const { return needSurfaceTrace_; }
	bool needUnitTrace() const { return needUnitTrace_; }
	bool needShieldTrace() const { return needShieldTrace_; }
	const EffectAttribute& shieldEffect() const { return shieldEffect_; }

	bool useChainEffect() const { return useChainEffect_; }
	const ChainLightningAttribute& chainLightningAttribute() const { return chainLightningAttribute_; }

	void initCache(WeaponPrmCache& cache) const;

private:

	bool needSurfaceTrace_;
	bool needShieldTrace_;
	bool needUnitTrace_;

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

/// параметры оружия - захвата
class WeaponGripPrm : public WeaponPrm
{
public:
	WeaponGripPrm();
	void serialize(Archive& ar);

	float horizontalImpulse() const { return horizontalImpulse_; }
	float verticalImpulse() const { return verticalImpulse_; }
	float dispersionAngle() const { return dispersionAngle_; }
	int startGripTime() const { return startGripTime_; }
	int finishGripTime() const { return finishGripTime_; }
	
private:
	float horizontalImpulse_;
	float verticalImpulse_;
	float dispersionAngle_;
	int startGripTime_;
	int finishGripTime_;

};


/// параметры метательного оружия
class WeaponProjectilePrm : public WeaponPrm
{
public:
	WeaponProjectilePrm();

	bool targetOnWaterSurface() const;
	void initCache(WeaponPrmCache& cache) const;

	void serialize(Archive& ar);

	bool needMissileVisualisation() const;
	const AttributeProjectile* missileID() const;

private:
	bool needMissileVisualisation_;

	/// ID снаряда
	AttributeProjectileReference missileID_;
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
