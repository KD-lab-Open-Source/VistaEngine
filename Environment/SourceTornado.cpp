#include "stdafx.h"
#include "SourceTornado.h"
#include "BaseUnit.h"
#include "..\Physics\RigidBodyPrm.h"
#include "..\Physics\RigidBodyUnit.h"
#include "UnitEnvironment.h"
#include "EnvironmentSimple.h"
#include "IronBuilding.h"

SourceTornado::SourceTornado()
:SourceDamage(), toolser_()
{
	height_ = 200.0f;
	tornadoFactor_ = 50.0f;
	setAffectMode(AFFECT_ALL_UNITS);
}

void SourceTornado::apply( UnitBase* unit )
{
	__super::apply(unit);

	if(!active())
		return;

	if(!unit->alive())
		return;

	if(unit->attr().isEnvironment()){
		UnitEnvironment * unitEnvironment = safe_cast<UnitEnvironment*>(unit);
		if(unitEnvironment->environmentType() == ENVIRONMENT_BUILDING ||
			unitEnvironment->environmentType() == ENVIRONMENT_BARN) {
				unitEnvironment->explode();
				unitEnvironment->Kill();
				return;
			}
	}

	RigidBodyBase* body = unit->rigidBody();
	if(!body)
		return;

	if(unit->attr().isBuilding()){
		const AttributeBuilding& attr = safe_cast_ref<const AttributeBuilding&>(unit->attr());
		if(attr.interactionType & (AttributeBuilding::INTERACTION_NORMAL | AttributeBuilding::INTERACTION_BIG_BUILDING))
			return;
		if(attr.interactionType & (ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2 | ENVIRONMENT_TREE)){
			RigidBodyUnit* bodyUnit(safe_cast<RigidBodyUnit*>(body));
			if(!bodyUnit->isBoxMode()){
				bodyUnit->enableBoxMode();
				bodyUnit->startFall(position());
			}
			return;
		}
		ParameterSet damage(safe_cast<UnitBuilding*>(unit)->parameters());
		damage.set(0);
		damage.set(1e+10, ParameterType::HEALTH);
		damage.set(1e+10, ParameterType::ARMOR);
		safe_cast<UnitBuilding*>(unit)->setDamage(damage, 0);
		return;
	}

	Vect2f pointPos(unit->position().x, unit->position().y);
	Vect2f tornadoPos(position().x, position().y);

	Vect2f dir = (tornadoPos - pointPos);

	if(dir.norm2() > sqr(radius_))
		return;
	
	if(unit->attr().isActing()  &&	safe_cast<UnitReal*>(unit)->currentState() != CHAIN_TRIGGER){
		safe_cast<RigidBodyUnit*>(body)->disableWaterAnalysis();
		safe_cast<RigidBodyBox*>(body)->setAngularVelocity(getTorque(unit->position()) / unit->attr().mass);
		safe_cast<RigidBodyBox*>(body)->enableAngularEvolve(false);
		safe_cast<RigidBodyUnit*>(body)->enableBoxMode();
		body->setVelocity(getForce(unit->position()) * tornadoFactor_ / unit->attr().mass);
	}

	if(unit->attr().isEnvironmentSimple()){
		safe_cast<UnitEnvironmentSimple*>(unit)->enableBoxMode();
		unit->rigidBody()->setVelocity(getForce(unit->position()) * tornadoFactor_ / unit->attr().mass);
		safe_cast<RigidBodyBox*>(unit->rigidBody())->setAngularVelocity(getRandomTorque(unit->position()) / unit->attr().mass);
	}
}

void SourceTornado::serialize( Archive& ar )
{
	__super::serialize(ar);
    ar.serialize(height_, "height", "высота");
    ar.serialize(tornadoFactor_, "tornadoFactor", "мощность");

    ar.serialize(toolser_, "toolser", "тулзер");

	serializationApply(ar);
}

void SourceTornado::start()
{
	__super::start();
	setScanEnvironment(true);
	toolser_.start(pose());
}

void SourceTornado::stop()
{
	__super::stop();
	setScanEnvironment(false);
	toolser_.stop();
}

Vect3f SourceTornado::getForce( const Vect3f& unitPos )
{

	Vect2f pointPos(unitPos.x, unitPos.y);
	Vect2f tornadoPos(position().x, position().y);

	Vect2f dir = (tornadoPos - pointPos);
	
	float pointDist = dir.norm();
	float horizontalForce = 1.0f + (1.0f + sin(pointDist/radius_ * 2.0f*M_PI - M_PI/2.0f))/2.1f;
	dir.Normalize(horizontalForce);
	Vect3f horizontDir(dir.x, dir.y, 0);

	float rotAngle = 90 - sin((pointDist/radius_) * M_PI/2.0)*90;
	QuatF rotQuat(rotAngle / (180.0f/M_PI), Vect3f::K);
	rotQuat.xform(horizontDir);

	float pointH = unitPos.z - position().z;
	horizontDir += Vect3f(-dir.x, -dir.y, 0) * (pointH/height_) * 3.0;
	float verticalForce = 0.5f * (1.0f-(pointDist/radius_)) + sin(pointH/height_ * M_PI);

	return Vect3f(horizontDir.x, horizontDir.y, verticalForce);
}

Vect3f SourceTornado::getTorque( const Vect3f& unitPos )
{

	Vect2f pointPos(unitPos.x, unitPos.y);
	Vect2f tornadoPos(position().x, position().y);

	Vect2f dir = (tornadoPos - pointPos);
	
	float pointDist = dir.norm();
	float horizontalForce = 1.0f + (1.0f + sin(pointDist/radius_ * 2.0f*M_PI - M_PI/2.0f))/2.1f;

	return Vect3f(0,0,horizontalForce);
}

Vect3f SourceTornado::getRandomTorque( const Vect3f& unitPos )
{

	Vect2f pointPos(unitPos.x, unitPos.y);
	Vect2f tornadoPos(position().x, position().y);

	Vect2f dir = (tornadoPos - pointPos);
	
	float pointDist = dir.norm();
	float horizontalForce = 1.0f + (1.0f + sin(pointDist/radius_ * 2.0f*M_PI - M_PI/2.0f))/2.1f;

	return Vect3f(horizontalForce*0.3f,horizontalForce*0.7f,horizontalForce);
}

void SourceTornado::quant()
{
	__super::quant();
	toolser_.setPosition(pose());
}

SourceTornado::SourceTornado( const SourceTornado& other )
:SourceDamage(other)
{
	height_ = other.height_;
	tornadoFactor_ = other.tornadoFactor_;
	toolser_ = other.toolser_;
	setAffectMode(AFFECT_ALL_UNITS);
}

