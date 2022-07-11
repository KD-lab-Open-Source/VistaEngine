#include "stdafx.h"
#include "GlobalAttributes.h"
#include "normalMap.h"
#include "UnitActing.h"
#include "RigidBodyCar.h"

///////////////////////////////////////////////////////////////
//
//    class JointWheel
//
///////////////////////////////////////////////////////////////

RigidBodyWheel::JointWheel::JointWheel(RigidBodyCar* body, int wheelIndex, 
									   float upperLimit, float lowerLimit, float drivingForceLimit) :
body_(body),
wheelIndex_(wheelIndex),
upperLimit_(upperLimit),
lowerLimit_(lowerLimit),
enableDriving_(false),
drivingForceLimit_(drivingForceLimit)
{
}

void RigidBodyWheel::JointWheel::compute(ConstraintHandler& ch) const
{
	Vect6fN2f j1(Vect6f::ZERO, body_->core()->velocity().vectN2f().size());
	j1.vectN2f()[wheelIndex_].x = -1; 
	ch.push_back(new IterateQuantitiesInteraction<Vect6fN2f, MassMatrixFull6fN2f>(
		body_->core(), body_->core(), body_->core()->massMatrix().computeWheelJ(wheelIndex_), j1, 
		body_->wheelPose(wheelIndex_).x , lowerLimit_, upperLimit_, body_->relaxationTime()));
	if(enableDriving_){
		Vect6fN2f j2(Vect6f::ZERO, body_->core()->velocity().vectN2f().size());
		j2.vectN2f()[wheelIndex_].y = 1; 
		ch.push_back(new IterateQuantitiesInteraction<Vect6fN2f, MassMatrixFull6fN2f>(
			body_->core(), body_->core(), body_->core()->massMatrix().computeDrivingWheelJ(wheelIndex_), j2, 
			desiredVelocity_, Limits(drivingForceLimit_)));
	}
}

///////////////////////////////////////////////////////////////
//
//    class RigidBodyWheel
//
///////////////////////////////////////////////////////////////

RigidBodyWheel::RigidBodyWheel(int index, RigidBodyCar* parent, const Vect3f& wheelDisplacement, 
							   float radius, int graphicNodeSuspension, int graphicNodeWheel, 
							   float k, float mu, int wheelType, float upperLimit, float lowerLimit, 
							   float drivingForceLimit, bool waterAnalysis, float waterLevel) :
	index_(index),
	parent_(parent),
	wheelPose_(Vect2f::ZERO),
	wheelAxes_(Vect3f::K, Vect3f::I_),
	wheelDisplacement_(wheelDisplacement),
	graphicNodeSuspension_(graphicNodeSuspension),
	graphicNodeWheel_(graphicNodeWheel),
	k_(k),
	mu_(mu),
	steeringAngle_(0.0f),
	wheelType_(wheelType),
	waterAnalysis_(waterAnalysis),
	waterLevel_(waterLevel),
	lavaLevel_(0.0f),
	joint_(parent, index_, upperLimit, lowerLimit, drivingForceLimit)
{
	setBoundRadius(radius);
}

void RigidBodyWheel::show()
{
	if(showDebugRigidBody.boundingBox){
		Vect3f vertex[8];
		GeomBox(Vect3f(boundRadius(), boundRadius(), boundRadius())).computeVertex(vertex, centreOfGravity(), orientation());
		show_vector(vertex[0], vertex[1], vertex[2], vertex[3], Color4c::RED);
		show_vector(vertex[4], vertex[5], vertex[6], vertex[7], Color4c::RED);
		show_vector(vertex[0], vertex[1], vertex[5], vertex[4], Color4c::RED);
		show_vector(vertex[3], vertex[2], vertex[6], vertex[7], Color4c::RED);
	}

	if(showDebugRigidBody.boundRadius)
		show_vector(centreOfGravity(), boundRadius(), Color4c::RED);
}

///////////////////////////////////////////////////////////////
//
//    class ContactSimple6fN2f
//
///////////////////////////////////////////////////////////////

ContactSimple6fN2f::ContactSimple6fN2f(const Vect3f& point, const Vect3f& normal, RigidBodyCar* body0, float penetration, int wheelIndex)
{
	point_ = point;
	body0_ = body0;
	xassert(isEq(normal.norm(), 1.0f, 0.01f));
	axes_.xrow() = normal;
	if(body0_->friction() > FLT_EPS){
		if(fabsf(normal.x) < 0.5f)
			axes_.yrow().cross(normal, Vect3f::I);
		else
			axes_.yrow().cross(normal, Vect3f::J);
		axes_.yrow().normalize();
		axes_.zrow().cross(axes_.yrow(), axes_.xrow());
		axes_.zrow().normalize();
	}
	penetration_ = penetration;
	wheelIndex_ = wheelIndex;
}

void ContactSimple6fN2f::compute(ConstraintHandler& ch) const
{
	Vect3f local_point0 = point_;
	local_point0.sub(body0_->centreOfGravity());
	if(body0_->friction() > FLT_EPS){
		ch.push_back(new IterateQuantitiesSimpleFriction<Vect6fN2f, MassMatrixFull6fN2f>(body0_->core(), 
			body0_->core()->massMatrix().computeJ(axes_.xrow(), local_point0, wheelIndex_), 
			penetration_ / body0_->relaxationTime(), Limits::GREATER_ZERO, body0_->restitution(), 
			body0_->friction()));
		ch.push_back(new IterateQuantitiesSimple<Vect6fN2f, MassMatrixFull6fN2f>(body0_->core(), 
			body0_->core()->massMatrix().computeJ(axes_.yrow(), local_point0, wheelIndex_)));
		ch.push_back(new IterateQuantitiesSimple<Vect6fN2f, MassMatrixFull6fN2f>(body0_->core(), 
			body0_->core()->massMatrix().computeJ(axes_.zrow(), local_point0, wheelIndex_)));
	}else{
		ch.push_back(new IterateQuantitiesSimple<Vect6fN2f, MassMatrixFull6fN2f>(body0_->core(), 
			body0_->core()->massMatrix().computeJ(axes_.xrow(), local_point0, wheelIndex_), 
			penetration_ / body0_->relaxationTime(), Limits::GREATER_ZERO, body0_->restitution()));
	}
}

///////////////////////////////////////////////////////////////
//
//    class MassMatrix6fN2f
//
///////////////////////////////////////////////////////////////

const MassMatrix6fN2f::MassMatrix6f2f MassMatrix6fN2f::ZERO;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyCar
//
///////////////////////////////////////////////////////////////

void RigidBodyCar::build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass)
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
}

void RigidBodyCar::computeForceAndTOI(float dt)
{
	start_timer_auto();
	colliding_ = 0;
	//if(collisionObject()->groundCollision(this))
	//	colliding_ = GROUND_COLLIDING;
	core_.massMatrix().groundCollision();
	__super::computeForceAndTOI(dt);
	core_.computeForceAndTOI(Mat3f(orientation()), dt, true);
}