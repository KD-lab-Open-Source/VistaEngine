#ifndef __IRONBULLET_H__
#define __IRONBULLET_H__

#include "UnitActing.h"
#include "WeaponAttribute.h"
#include "LaunchData.h"
#include "Physics\RigidBodyMissile.h"

#include "Environment\SourceImpulse.h"

class AttributeProjectile : public AttributeBase, public StringTableBase
{
public:
	AttributeProjectile(const char* name = "");
	void serialize(Archive& ar);
	const char* libraryKey() const { return c_str(); }

	const WeaponDamage& damage() const { return damage_; }

	const EffectAttribute& shildExplosionEffect() const { return shildExplosionEffect_; }
	const AbnormalStateAttribute& explosionState() const { return explosionState_; }

	bool createSourcesOnTargetHit() const {
		return (sourcesCreationMode_ == WEAPON_SOURCES_CREATE_ALWAYS ||
			sourcesCreationMode_ == WEAPON_SOURCES_CREATE_ON_TARGET_HIT);
	}
	bool createSourcesOnGroundHit() const {
		return (sourcesCreationMode_ == WEAPON_SOURCES_CREATE_ALWAYS ||
			sourcesCreationMode_ == WEAPON_SOURCES_CREATE_ON_GROUND_HIT);
	}

	int LifeTime;
	int collisionCounter;
	
	bool exactCollision;
	
	float radiusToExplode;

	bool applyImpulse;

	float impulseStrength;

	BitVector<EnvironmentType> environmentStop;

	VisibilityGroupIndex dockedVisibilityGroup() const { return VisibilityGroupIndex(dockedVisibilityGroup_); }
	VisibilityGroupIndex firingVisibilityGroup() const { return VisibilityGroupIndex(firingVisibilityGroup_); }
	
	const TargetEffects& hitExplosionEffects() const { return hitExplosionEffects_; }

private:
	VisibilityGroupName dockedVisibilityGroup_;
	VisibilityGroupName firingVisibilityGroup_;

	WeaponDamage damage_;

	/// эффекты попадания по целям
	TargetEffects hitExplosionEffects_;

	/// эффект взрыва при попадании в защитное поле
	EffectAttribute shildExplosionEffect_;
	/// воздействие на цель при попадании
	AbnormalStateAttribute explosionState_;
	WeaponSourcesCreationMode sourcesCreationMode_;
};

typedef StringTable<AttributeProjectile> AttributeProjectileTable;
typedef StringTableReference<AttributeProjectile, true> AttributeProjectileReference;

/// Базовый класс для снарядов, ракет, бомб и т.д.
class ProjectileBase : public UnitReal
{
public:
	ProjectileBase(const UnitTemplate& data);
	
	const AttributeProjectile& attr() const {
		return safe_cast_ref<const AttributeProjectile&>(__super::attr());
	}

	void collision(UnitBase* p, const ContactInfo& contactInfo);
	void testCollision();

	void Quant();
	void fowQuant();
	void Kill();

	bool corpseQuant();

	void setPose(const Se3f& poseIn, bool initPose);

	virtual void wayPointController() {}

	void explode();
	bool isExplosionSourcesEnabled() const;

	virtual void setSource(UnitActing* ownerUnit, const Se3f& pose); // Вызывать перед setTarget
	virtual void setTarget(UnitInterface* targetUnit, const Vect3f& targetPosition, float targetDelta = 0.0f);
	bool checkInPathTracking(const UnitBase* tracker) const { return false; } 

	UnitBase* ignoredUnit() const { return ownerUnit_; };

	void setExplosionParameters(AffectMode affect_mode, const WeaponPrmCache& weapon_prm, int damage_divide, bool canShootThroughShield);
	AffectMode explosionAffectMode() const { return affectMode_; }
	const WeaponPrmCache* explosionParameters() const { return &weaponParameters_; }

	void serialize(Archive& ar);

protected:
	UnitLink<UnitActing> ownerUnit_;

	/// подтверждение столкновения - надо ли взрываться
	virtual bool confirmCollision(const UnitBase* p) const;

	const UnitInterface* target() const { return target_; }

	void chainQuant();

	Vect3f sourcePosition_;
	Vect3f targetPosition_;

	int collisionCounter_;
	UnitLink<UnitInterface> target_;

private:
	AffectMode affectMode_;

	/// Параметры выстрелившего снаряд оружия.
	WeaponPrmCache weaponParameters_;

	UnitBase* collisionUnit_;

	LogicTimer killTimer_;
	Vect3f traceShieldPositionPrev_;

	bool explosionSourcesEnabled;

	bool canShootThroughShield_;
};

class ProjectileBullet : public ProjectileBase
{
public:
	ProjectileBullet(const UnitTemplate& data);

	RigidBodyMissile* rigidBody() const { return safe_cast<RigidBodyMissile*>(rigidBody_); }
	
	void setTarget(UnitInterface* targetUnit, const Vect3f& targetPosition, float targetDelta);

protected:
	bool confirmCollision(const UnitBase* p) const;
};

class ProjectileMissile : public ProjectileBase
{
public:
	ProjectileMissile(const UnitTemplate& data);

	RigidBodyMissile* rigidBody() const { return safe_cast<RigidBodyMissile*>(rigidBody_); }

	void Quant();
	void wayPointController();

	void setTarget(UnitInterface* targetUnit,const Vect3f& targetPosition, float targetDelta);

protected:
	bool confirmCollision(const UnitBase* p) const;

private:
	TerToolCtrl  traceCtrl_;
	bool traceStarted_;
};

#endif //__IRONBULLET_H__
