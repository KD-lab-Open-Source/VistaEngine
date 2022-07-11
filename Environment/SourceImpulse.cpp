#include "stdafx.h"

#include "SourceImpulse.h"
#include "Serialization.h"
#include "BaseUnit.h"
#include "UnitActing.h"
#include "EnvironmentSimple.h"
#include "IronBuilding.h"

SourceImpulse::SourceImpulse()
{
	instantaneous = true;
	applyOnFlying = true;
	horizontalImpulse = 100;
	verticalImpulse = 400;
	torqueImpulse = 0;
	environmentTorqueImpulse = 0.0f;
	setScanEnvironment(true);
}

SourceImpulse::SourceImpulse(const SourceImpulse& src) :
SourceEffect(src)
{
	instantaneous = src.instantaneous;
	applyOnFlying = src.applyOnFlying;
	horizontalImpulse = src.horizontalImpulse;
	verticalImpulse = src.verticalImpulse;
	torqueImpulse = src.torqueImpulse;
	environmentTorqueImpulse = src.environmentTorqueImpulse;
	abnormalState_ = src.abnormalState_;
	setScanEnvironment(true);
}

void SourceImpulse::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(instantaneous, "instantaneous", "Мгновенный (для снарядов - сразу исчезает)");
	ar.serialize(applyOnFlying, "applyOnFlying", "Воздействовать на летные юниты");
	ar.serialize(horizontalImpulse, "horizontalImpulse", "Горизонтальный импульс");
	ar.serialize(verticalImpulse, "verticalImpulse", "Вертикальный импульс");
	ar.serialize(torqueImpulse, "torqueImpulse", "Вращающий импульс (не действует на людей)");
	ar.serialize(environmentTorqueImpulse, "environmentTorqueImpulse", "Вращающий импульс (для камней)");
    ar.serialize(abnormalState_, "abnormalState", "воздействие на юниты");
	serializationApply(ar);
}

bool SourceImpulse::getParameters(WeaponSourcePrm& prm) const
{
	prm.setAbnormalState(abnormalState_);

	return true;
}

bool SourceImpulse::setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target)
{
	abnormalState_ = prm.abnormalState();

	return true;
}

void SourceImpulse::quant()
{
	__super::quant();
	
	if(!active())
		return;
	
	if(instantaneous && !isUnderEditor())
		kill();
}

void SourceImpulse::apply(UnitBase* unit)
{
	if(isUnderEditor())
		return;

	__super::apply(unit);

	if(!active())
		return;

	if(!unit->alive())
		return;

	RigidBodyBase* body = unit->rigidBody();
	if(!body)
		return;

	if(abnormalState_.isEnabled() && (unit->attr().isLegionary() || unit->attr().isBuilding()))
		safe_cast<UnitActing*>(unit)->setImpulseAbnormalState(abnormalState_, owner());

	int unitAtackClass(0);
	if(unit->attr().isBuilding()){
		unitAtackClass = safe_cast_ref<const AttributeBuilding&>(unit->attr()).interactionType;
		if(unitAtackClass & (ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2 | ENVIRONMENT_TREE)){
			RigidBodyUnit* bodyUnit(safe_cast<RigidBodyUnit*>(body));
			if(!bodyUnit->isBoxMode()){
				bodyUnit->enableBoxMode();
				bodyUnit->startFall(position());
			}
			return;
		}
		if(unitAtackClass & (ENVIRONMENT_BUILDING | ENVIRONMENT_BARN)) {
			if(fabsf(torqueImpulse) > 0.1f){
				ParameterSet damage(safe_cast<UnitBuilding*>(unit)->parameters());
				damage.set(0);
				damage.set(1e+10, ParameterType::HEALTH);
				damage.set(1e+10, ParameterType::ARMOR);
				safe_cast<UnitBuilding*>(unit)->setDamage(damage, 0);
			}
			return;
		}
	}
	if(unit->attr().isEnvironment()){
		unitAtackClass = unit->unitAttackClass();
		if(unitAtackClass & (ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2 | ENVIRONMENT_TREE)){
			safe_cast<UnitEnvironmentSimple*>(unit)->startFall(position());
			return;
		}
		if(unitAtackClass & (ENVIRONMENT_BUILDING | ENVIRONMENT_BARN)) {
			if(fabsf(torqueImpulse) > 0.1f){
				unit->explode();
				unit->Kill();
			}
			return;
		}
	}
	
	if(unitAtackClass & ATTACK_CLASS_ENVIRONMENT)
		return;

	if((body->isUnit() && 
		(applyOnFlying || !safe_cast<RigidBodyUnit*>(body)->flyingMode()) && 
		!safe_cast<RigidBodyUnit*>(body)->isBoxMode() && 
		!safe_cast<RigidBodyUnit*>(body)->isFrozen() &&
		safe_cast<UnitReal*>(unit)->currentState() != CHAIN_TRIGGER) || 
		(unit->attr().isEnvironment() && body->isBox()) && 
		(body->colliding() & (RigidBodyBase::GROUND_COLLIDING | RigidBodyBase::WATER_COLLIDING))){

		Vect3f dir = unit->position() - position();
		dir.z = 0;
		float norm = dir.norm();
		if(norm > radius())
			return;
		float pulse(sqrtf(safe_cast<RigidBodyPhysics*>(body)->mass() * 10.0f / (norm + 10.0f)));
		dir.z = verticalImpulse * pulse;
		float horizontalPulse(pulse * horizontalImpulse / (norm + 1.f));
		dir.x *= horizontalPulse;
		dir.y *= horizontalPulse;
		
		if(fabsf(torqueImpulse) < 0.1f){
			if(body->prm().moveVertical){
				Se3f pose = body->pose();
				pose.rot().premult(QuatF(logicRNDfrnd(M_PI), Vect3f::K));
				body->setPose(pose);
			}
			if(unit->attr().isEnvironment()){
				Vect3f angVel(Vect3f::ZERO);
				if(fabsf(environmentTorqueImpulse) > 0.1f){
					angVel.cross(Vect3f::K, dir);
					angVel.scale(environmentTorqueImpulse);
				}
				safe_cast<RigidBodyPhysics*>(body)->setImpulse(dir, angVel);
			}
			else
				safe_cast<RigidBodyBox*>(body)->setImpulse(dir);
		}
		else{
			if(body->prm().moveVertical)
				return;
			Vect3f angVel;
			angVel.cross(Vect3f::K, dir);
			angVel.Normalize(torqueImpulse * body->extent().norm2() * pulse / 300.0f);
			safe_cast<RigidBodyPhysics*>(body)->setImpulse(dir, angVel);
		}
		if(body->isUnit()){
			safe_cast<RigidBodyUnit*>(body)->disableWaterAnalysis();
			safe_cast<RigidBodyBox*>(body)->avoidCollisionAtStart();
			safe_cast<RigidBodyUnit*>(body)->enableBoxMode();
		}
		

		body->awake();
	}
}
