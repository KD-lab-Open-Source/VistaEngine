#include "StdAfx.h"

#include "Universe.h"
#include "IronBullet.h"
#include "..\Environment\Environment.h"

#include "Squad.h"
#include "Serialization.h"
#include "TypeLibraryImpl.h"

#include "EffectController.h"
#include "..\Environment\SourceShield.h"
#include "RenderObjects.h"

REGISTER_CLASS(UnitBase, ProjectileBase, "снаряды");
REGISTER_CLASS(UnitBase, ProjectileBullet, "пули");
REGISTER_CLASS(UnitBase, ProjectileMissile, "ракеты");

REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PROJECTILE, ProjectileBase);
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PROJECTILE_BULLET, ProjectileBullet);
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PROJECTILE_MISSILE, ProjectileMissile);

WRAP_LIBRARY(AttributeProjectileTable, "AttributeProjectile", "Снаряды", "Scripts\\Content\\AttributeProjectile", 0, true);

struct RigidBodyPrmProjectileReference : RigidBodyPrmReference
{
	RigidBodyPrmProjectileReference() {}
	RigidBodyPrmProjectileReference(const char* name) : RigidBodyPrmReference(name) {}
	RigidBodyPrmProjectileReference(const RigidBodyPrmReference& data) : RigidBodyPrmReference(data) {}

	bool validForComboList(const RigidBodyPrm& attr) const {
		return &attr ? attr.unit_type == RigidBodyPrm::DEBRIS || attr.unit_type == RigidBodyPrm::MISSILE || attr.unit_type == RigidBodyPrm::ROCKET : false;
	}
	bool refineComboList() const { return true; }
};

AttributeProjectile::AttributeProjectile(const char* name) 
: StringTableBase(name)
{
	unitAttackClass = ATTACK_CLASS_MISSILE;
	unitClass_ = UNIT_CLASS_PROJECTILE;
	
	excludeCollision = EXCLUDE_COLLISION_BULLET;
	collisionGroup = COLLISION_GROUP_ACTIVE_COLLIDER;
	sourcesCreationMode_ = WEAPON_SOURCES_CREATE_ON_GROUND_HIT;

	LifeTime = 0;
	collisionCounter = 0;

	rigidBodyPrm = RigidBodyPrmReference("Missile");

	enablePathTracking = false;
}

void AttributeProjectile::serialize(Archive& ar)
{
	StringTableBase::serialize(ar);

	AttributeBase::serialize(ar);

	RigidBodyPrmProjectileReference rigidBodyProjectile = rigidBodyPrm;
	ar.serialize(rigidBodyProjectile, "rigidBodyPrm", "Тип снаряда");
	rigidBodyPrm = rigidBodyProjectile;

	ar.serialize(forwardVelocity, "forwardVelocity", "Скорость полета");
	if(!forwardVelocity)
		forwardVelocity = rigidBodyPrm->forward_velocity_max;

	if(ar.isInput()){
		collisionGroup = COLLISION_GROUP_REAL;
		
		if(!rigidBodyPrm)
			rigidBodyPrm = RigidBodyPrmReference("Missile");

		switch(rigidBodyPrm->unit_type){
		case RigidBodyPrm::DEBRIS:
			unitClass_ = UNIT_CLASS_PROJECTILE;
			break;
		case RigidBodyPrm::MISSILE:
			unitClass_ = UNIT_CLASS_PROJECTILE_BULLET;
			break;
		case RigidBodyPrm::ROCKET:
			unitClass_ = UNIT_CLASS_PROJECTILE_MISSILE;
			break;
		}
	}
	
    ar.serialize(damage_, "damage", "повреждения");
	ar.serialize(environmentDestruction, "environmentDestruction", "Разрушение объектов окружения");

	ar.serialize(shildExplosionEffect_, "shildExplosionEffect", "спецэффект попадания в защитное поле");
	ar.serialize(explosionState_, "explosionState", "воздействие на цель при попадании");

	ar.serialize(sourcesCreationMode_, "sourcesCreationMode", "создавать источники");

	ar.serialize(LifeTime, "LifeTime", "Время жизни, секунды");
	if(!LifeTime)
		LifeTime = 10;
	if(rigidBodyPrm->unit_type == RigidBodyPrm::DEBRIS && rigidBodyPrm->ground_collision_enabled)
		ar.serialize(collisionCounter, "collisionCounter", "Число отскоков");
	if(rigidBodyPrm->unit_type == RigidBodyPrm::ROCKET && rigidBodyPrm->undergroundMode)
		ar.serialize(traceTerTool, "traceTerTool", "след");

	ar.serialize(mass, "mass", "Масса");

	collisionGroup = COLLISION_GROUP_ACTIVE_COLLIDER;
}

ProjectileBase::ProjectileBase(const UnitTemplate& data) : UnitReal(data),
	target_(NULL),
	sourcePosition_(Vect3f::ZERO),
	targetPosition_(Vect3f::ZERO),
	collisionUnit_(0)
{
	affectMode_ = AFFECT_ENEMY_UNITS;
	collisionCounter_ = attr().collisionCounter;
	if(rigidBody_->isBox())
		safe_cast<RigidBodyBox*>(rigidBody_)->setForwardVelocity(attr().forwardVelocity);
	else
		rigidBody()->setForwardVelocity(attr().forwardVelocity);
	traceShieldPositionPrev_ = Vect3f::ZERO;
	explosionSourcesEnabled = attr().createSourcesOnGroundHit();
}

void ProjectileBase::collision(UnitBase* p, const ContactInfo& contactInfo)
{
	if(confirmCollision(p)){
		//setPose(rigidBody_->pose(), false); // Вызов мог произойти из промежуточных перемещений
		collisionUnit_ = p;
		explode();
		p->setDamage(weaponParameters_.damage(), ignoredUnit(), &contactInfo);
		if(attr().createSourcesOnTargetHit())
			createExplosionSources(p->position2D());

		Kill();
	}
}

void ProjectileBase::setSource(UnitActing* ownerUnit, const Se3f& pose)
{
	ownerUnit_ = ownerUnit;
	sourcePosition_ = pose.trans();
	setPose(pose, true);
	traceShieldPositionPrev_ = pose.trans();
}

void ProjectileBase::setTarget(UnitInterface* targetUnit,const Vect3f& target,float target_delta)
{
	target_ = targetUnit;

	if(targetUnit)
		targetUnit->computeTargetPosition(targetPosition_);
	else{
		targetPosition_ = target;
//		targetPosition_.z = rigidBody()->heightWithWater(target.xi(), target.yi());
	}

	if(target_delta > FLT_EPS){
		float angle = M_PI * 2.0f * logicRNDfrand();
		float radius = logicRNDfrnd(1.f) * target_delta;
		targetPosition_.x += radius * cos(angle);
		targetPosition_.y += radius * sin(angle);
	}

	if(attr().LifeTime)
		killTimer_.start(attr().LifeTime*1000);
}

void ProjectileBase::setExplosionParameters(AffectMode affect_mode, const WeaponPrmCache& weapon_prm, int damage_divide)
{
	affectMode_ = affect_mode;
	weaponParameters_ = weapon_prm;
	if(damage_divide > 1){
		WeaponDamage dmg = weaponParameters_.damage();
		dmg *= 1.f / float(damage_divide);
		weaponParameters_.setDamage(dmg);
	}
}

void ProjectileBase::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(target_, "target", 0);
	ar.serialize(ownerUnit_, "ownerUnit", 0);
	ar.serialize(sourcePosition_, "sourcePosition", 0);
	ar.serialize(targetPosition_, "targetPosition", 0);
	ar.serialize(killTimer_, "killTimer", 0);
	ar.serialize(affectMode_, "affectMode", 0);
	ar.serialize(weaponParameters_, "weaponParameters", 0);
}

bool ProjectileBase::confirmCollision(const UnitBase* p) const 
{ 
	if(!alive())
		return false;
	
	if(p->unitAttackClass() & ENVIRONMENT_INDESTRUCTIBLE & attr().environmentDestruction)
		return false;

	if(p->unitAttackClass() & (ENVIRONMENT_FENCE2 | ENVIRONMENT_BARN | ENVIRONMENT_BUILDING |
		ENVIRONMENT_BIG_BUILDING | ENVIRONMENT_STONE | ENVIRONMENT_INDESTRUCTIBLE))
		return true;

	if(p->attr().isEnvironment())
		return false;

	switch(explosionAffectMode()){
	case AFFECT_OWN_UNITS:
		return player() == p->player();
	case AFFECT_FRIENDLY_UNITS:
		return !isEnemy(p);
	case AFFECT_ALLIED_UNITS:
		return !isEnemy(p) && (player() != p->player());
	case AFFECT_ENEMY_UNITS:
		return isEnemy(p);
	case AFFECT_ALL_UNITS:
		return true;
	case AFFECT_NONE_UNITS:
		return false;
	}

	return false;
}


bool ProjectileBase::isExplosionSourcesEnabled() const
{
	return explosionSourcesEnabled;
}

void ProjectileBase::explode()
{
	if(target() && ownerUnit_){
		if((rigidBody_->colliding() & (RigidBodyBase::GROUND_COLLIDING | RigidBodyBase::WATER_COLLIDING)) || collisionUnit_ != target()){
			if(ownerUnit_->position2D().distance2(target()->position2D()) > ownerUnit_->position2D().distance2(position2D()))
				ownerUnit_->attackTargetUnreachable(true);
		}
	}

	if(!collisionUnit_ && ((rigidBody_->colliding() & (RigidBodyBase::GROUND_COLLIDING | RigidBodyBase::WATER_COLLIDING)) || !target()) && !(rigidBody_->colliding() & RigidBodyBase::WORLD_BORDER_COLLIDING))
		UnitReal::explode();
	else{
		if(collisionUnit_){
			if(collisionUnit_ == target() && ownerUnit_)
				ownerUnit_->attackTargetUnreachable(false);

			if(attr().explosionState().isEnabled())
				collisionUnit_->setAbnormalState(weaponParameters_.abnormalState(), 0);
//				collisionUnit_->setAbnormalState(attr().explosionState(), 0);
		}
		if(deathAttr().explodeReference->enableExplode && model()){
			explosionSourcesEnabled = false;
			startState(StateDeath::instance());
		}
	}
}

void ProjectileBase::Quant()
{
	start_timer_auto();

	UnitBase::Quant();
	MoveQuant();

	if(!alive()){
		if(currentState_->canFinish(this)){
			currentState_->finish(this);
			currentStateID_ = CHAIN_NONE;
			currentState_ = 0;
		}
		return;
	}

	if(rigidBody_->colliding() & RigidBodyBase::WORLD_BORDER_COLLIDING)
		Kill();
	
	if(alive() && !isDocked()){
		Vect3f intersection, center;
		if(SourceShield::traceShieldsDelta(traceShieldPositionPrev_, position(), player(), intersection, center)){

			SourceShield::shieldExplodeEffect(center, intersection, attr().shildExplosionEffect());

			if(model())
				streamLogicInterpolator.set(fHideByFactor, model()) << intersection.distance(traceShieldPositionPrev_)/(position().distance(traceShieldPositionPrev_) + 0.01f);

			explosionSourcesEnabled = false;
			explode();
			Kill();
		}
		else if((killTimer_.was_started() && !killTimer_()) 
			|| (rigidBody_->colliding() && !collisionCounter_)){
			explode();
			Kill();
		}
  		else if((rigidBody_->colliding() & (RigidBodyBase::GROUND_COLLIDING | RigidBodyBase::WATER_COLLIDING)) && !rigidBody_->prm().undergroundMode && collisionCounter_--){
			if(attr().createSourcesOnGroundHit())
				createExplosionSources(position2D());
		}
	}
	else {
		if(killTimer_.was_started())
			killTimer_.pause();
	}

	traceShieldPositionPrev_ = position();
}

void ProjectileBase::fowQuant()
{
 	const FogOfWarMap* fow = universe()->activePlayer()->fogOfWarMap();
	if(!fow || !model())
		return;

	if(!terScene->IsFogOfWarEnabled()){
		if(hiddenGraphic() & HIDE_BY_FOW)
			hide(HIDE_BY_FOW, false);
		return;
	}

	switch(attr().fow_mode){
	case FVM_HISTORY_TRACK:
		switch(fow->getFogState(position2D().xi(), position2D().yi())){
		case FOGST_NONE:
			hide(HIDE_BY_FOW, false);
			break;
		case FOGST_HALF:
			if(!(hiddenGraphic() & HIDE_BY_FOW)){
//				universe()->addFowModel(model());
				hide(HIDE_BY_FOW, true);
			}
			break;
		case FOGST_FULL:
			hide(HIDE_BY_FOW, true);
			break;
		}
		break;
	case FVM_NO_FOG:
		hide(HIDE_BY_FOW, fow->getFogState(position2D().xi(), position2D().yi()) != FOGST_NONE);
		break;
	}
}

void ProjectileBase::setPose(const Se3f& poseIn, bool initPose)
{
	__super::setPose(poseIn, initPose);

	fowQuant();
}

void ProjectileBase::chainQuant()
{
	setChain(isDocked() ? CHAIN_STAND : CHAIN_WALK);
}

//----------------------------------------
ProjectileBullet::ProjectileBullet(const UnitTemplate& data) : ProjectileBase(data)
{
}

void ProjectileBullet::setTarget(UnitInterface* targetUnit,const Vect3f& targetPosition, float targetDelta)
{
	ProjectileBase::setTarget(targetUnit, targetPosition, targetDelta);

	xassert(ownerUnit_ && ownerUnit_->rigidBody());

	rigidBody()->startMissile(ownerUnit_->rigidBody()->velocity(), pose().trans(), targetPosition_);
}

bool ProjectileBullet::confirmCollision(const UnitBase* p) const
{
	if(ProjectileBase::confirmCollision(p) && rigidBody_ && rigidBody()->missileStarted())
		return true;

	return false;
}

//-----------------------------------------------

ProjectileMissile::ProjectileMissile(const UnitTemplate& data) : ProjectileBase(data)
{
	traceStarted_ = false;
	if(rigidBody_->prm().undergroundMode){
		model()->SetAttr(ATTRUNKOBJ_IGNORE);
		collisionCounter_ = INT_INF;
		setCollisionGroup(0);
		traceCtrl_ = attr().traceTerTool;
	}
}

void ProjectileMissile::Quant()
{
	ProjectileBase::Quant();

	if(!alive())
		return;
	
	if(!isDocked()){
		if((rigidBody_->prm().undergroundMode || rigidBody_->prm().underwaterRocket) && position2D().distance2(targetPosition_) < 100){
			explode();
			if(target())
				const_cast<UnitInterface*>(target())->setDamage(explosionParameters()->damage(), ignoredUnit());
			Kill();
		}
		
		if((rigidBody_->prm().underwaterRocket && rigidBody()->groundColliding()) || (rigidBody_->prm().undergroundMode && rigidBody()->flyingHeight() > 0.0f)){
			explode();
			Kill();
		}

		if(rigidBody_->prm().undergroundMode && !traceCtrl_.isEmpty())
			if(traceStarted_)
				traceCtrl_.setPosition(pose());
			else{
				traceCtrl_.start(pose());
				traceStarted_ = true;
			}
		
	}
}

void ProjectileMissile::WayPointController()
{
	if(isDocked()){
		stopPermanentEffects();
	}
	else {
		startPermanentEffects();

		if(!rigidBody()->directControl() && target() && target()->attr().isObjective()){
			const UnitReal* unit = safe_cast<const UnitReal*>(target());
			if(!unit->hiddenLogic() && !unit->initPoseTimer()){
				target()->computeTargetPosition(targetPosition_);
				rigidBody()->way_points.clear();
				rigidBody()->way_points.push_back(targetPosition_);
			}
			else
				target_ = 0;
		}
		else if(rigidBody()->is_point_reached(targetPosition_))
			rigidBody()->way_points.clear();
	}
}

void ProjectileMissile::setTarget(UnitInterface* targetUnit,const Vect3f& targetPosition, float targetDelta)
{
	ProjectileBase::setTarget(targetUnit, targetPosition, targetDelta);

	xassert(ownerUnit_ && ownerUnit_->rigidBody());

	rigidBody()->startRocket(ownerUnit_->rigidBody()->velocity(), ownerUnit_->isDirectControl());
	rigidBody()->way_points.push_back(targetPosition_);
}

bool ProjectileMissile::confirmCollision(const UnitBase* p) const
{
	if(ProjectileBase::confirmCollision(p) && !isDocked()) 
		return true;

	return false;
}

void ProjectileBase::Kill()
{
	__super::Kill();
	//model()->SetAttr(ATTRUNKOBJ_IGNORE);
}


