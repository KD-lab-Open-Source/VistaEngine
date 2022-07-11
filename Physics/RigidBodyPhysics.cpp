#include "StdAfx.h"
#include "RigidBodyPhysics.h"
#include "Math\ConstraintHandler.h"
#include "NormalMap.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysicsBase
//
///////////////////////////////////////////////////////////////

ConstraintHandler* RigidBodyPhysicsBase::collide_ = 0;

RigidBodyPhysicsBase::RigidBodyPhysicsBase() :
	index_(-1),
	friction_(0),
	restitution_(0),
	relaxationTime_(0.1f),
	waterLevel_(0.0f)
{
}

void RigidBodyPhysicsBase::computeForceAndTOI(float dt)
{
	Vect3f centreOfGravity_ = centreOfGravity();
	float r(maxRadius() + 3.0f);
	if(position().x < r)
		addContact(centreOfGravity_, Vect3f::I, r - position().x);
	else{
		float maxx(vMap.H_SIZE - 1 - r);
		if(position().x > maxx)
			addContact(centreOfGravity_, Vect3f::I_, position().x - maxx);
	}
	if(position().y < r)
		addContact(centreOfGravity_, Vect3f::J, r - position().y);
	else{
		float maxy = vMap.V_SIZE - 1 - r;
		if(position().y > maxy)
			addContact(centreOfGravity_, Vect3f::J_, position().y - maxy);
	}
}

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysics
//
///////////////////////////////////////////////////////////////

RigidBodyPhysics::RigidBodyPhysics() :
	waterAnalysis_(false),
	waterWeight_(0.0f),
	angularEvolve_(true)
{
	setColor(Color4c::RED);
}

void RigidBodyPhysics::build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass)
{
	__super::build(prmIn, center, extent, mass);
	core()->massMatrix().setDamping(prm().linearDamping, prm().angularDamping);
	Vect3f toi;
	collisionObject()->getTOI(mass * prm().TOI_factor, toi);
	core()->massMatrix().setMass(mass);
	core()->massMatrix().setTOI(toi);
	core()->massMatrix().setGravityZ(-prm().gravity);
	friction_ = prm().friction;
	restitution_ = prm().restitution;
	relaxationTime_ = prm().relaxationTime;
	setWaterAnalysis(prm().waterAnalysis);
}

void RigidBodyPhysics::addContact(const Vect3f& point, const Vect3f& normal, float penetration)
{
	if(attached())
		collide_->addContact(new ContactSimple(point, normal, this, penetration));
}

void RigidBodyPhysics::computeForceAndTOI(float dt)
{
	start_timer_auto();
	if(onWater() && waterAnalysis()){
		if(collisionObject()->waterCollision(this)){
			colliding_ = WATER_COLLIDING;
			Vect3f waterImpulse(environment->water()->GetVelocity(position().xi(), position().yi()), 0);
			core()->pulseExternal().linear().scaleAdd(waterImpulse, core()->mass() * waterWeight());
		}
	}
	else{
		if(collisionObject()->groundCollision(this))
			colliding_ = GROUND_COLLIDING;
	}
	__super::computeForceAndTOI(dt);
	core()->computeForceAndTOI(Mat3f(orientation()), dt, !isBox());
}

void RigidBodyPhysics::sleepCount(float dt)
{
	__super::evolve(dt);
}

void RigidBodyPhysics::boxEvolve(float dt)
{
	core()->evolve(dt);
	Se3f poseNew(orientation(), centreOfGravity());
	core()->massMatrix().evolve(poseNew, core()->velocity(), dt, angularEvolve());
	setCentreOfGravityPose(poseNew);
	float r(maxRadius() + 2.0f);
	float clamped(false);
	Vect3f clampedPosition(position());
	if(clampedPosition.x < r){
		clampedPosition.x = r;
		clamped = true;
	}else{
		float maxX(vMap.H_SIZE - 1 - r);
		if(clampedPosition.x > maxX){
			clampedPosition.x = maxX;
			clamped = true;
		}
	}
	if(clampedPosition.y < r){
		clampedPosition.y = r;
		clamped = true;
	}else{
		float maxY(vMap.V_SIZE - 1 - r);
		if(clampedPosition.y > maxY){
			clampedPosition.y = maxY;
			clamped = true;
		}
	}
	if(clamped){
		setPosition(clampedPosition);
		clampedPosition.z += box()->computeGroundPenetration(centreOfGravity(), orientation());
		setPosition(clampedPosition);
		colliding_ |= WORLD_BORDER_COLLIDING;
	}
}

void RigidBodyPhysics::updateRegion(float x1, float y1, float x2, float y2)
{
	if(!asleep()) 
		return;

	Vect2f center(centreOfGravity());

	float b1 = center.x - boundRadius();
	float b2 = center.x + boundRadius();

	if(x1 > b2 || x2 < b1) 
		return;

	b1 = center.y - boundRadius();
	b2 = center.y + boundRadius();

	if(y1 > b2 || y2 < b1) 
		return;

	awake();
}

void RigidBodyPhysics::serialize(Archive& ar)
{
	if(!prm().moveVertical)
		ar.serialize(angularEvolve_, "angularEvolve", 0);
	if(prm().waterAnalysis)
		ar.serialize(waterAnalysis_, "waterAnalysis", 0);
}