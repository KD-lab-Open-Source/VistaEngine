#include "stdafx.h"
#include "..\Environment\Anchor.h"
#include "RigidBodyPrm.h"
#include "RigidBodyTrail.h"
#include "UnitTrail.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_TRAIL, RigidBodyTrail)

RigidBodyTrail::RigidBodyTrail()
{
	dockingPoint_ = 0;
	docked_ = false;
	docking_ = false;
	rigidBodyType = RIGID_BODY_TRAIL;
}

bool RigidBodyTrail::evolve( float dt )
{
	if(!way_points.empty() && docked_){
		TrailPoint* dockingPointPrev = dockingPoint_;
		checkDockingPoint();
		if(dockingPointPrev != dockingPoint_)
			docked_ = false;
		else {
			safe_cast<UnitReal*>(ownerUnit_)->stop();
			way_points.clear();
		}
	}

	if(docked_) {
		posePrev_ = pose();
		setColor(YELLOW);
		return false;
	}
	else if(docking_) {
		setColor(RED);
		if(interpolateDockingPose(getDockingPose())){
			docked_ = true;
			docking_ = false;
		}
		return true;
	}
	else {
		docking_ = checkDockingPoint();
		if(dockingPoint_)
			way_points[0] = getDockingPose().trans();

		bool retVal = __super::evolve(dt);
		
		if(docking_) {
			safe_cast<UnitReal*>(ownerUnit_)->stop();
			way_points.clear();
		}

		return retVal;
	}
}

bool RigidBodyTrail::checkDockingPoint()
{
	if(way_points.empty())
		return false;

	UnitTrail* unit = safe_cast<UnitTrail*>(ownerUnit_);
	dockingPoint_ = unit->checkTrailPoint(Vect2f(way_points.front()));
	
	if(!dockingPoint_)
		return false;

	Vect2f dockingPoint2D(dockingPoint_->pose.trans());
	if(dockingPoint2D.distance2(position2D()) < sqr(dockingPoint_->radius))
		return true;

	return false;
}

bool RigidBodyTrail::interpolateDockingPose( const Se3f& poseIn )
{
	bool retValue = false;

	Vect3f dir = poseIn.trans() - position();
	float delta = forwardVelocity() * 0.1f / dir.norm();
	
	if(delta > 1.0f){
		delta = 1.0f;
		retValue = true;
	}
	else
		dir.Normalize(forwardVelocity() * 0.1f);
	
	QuatF rotNew;
	rotNew.slerp(pose().rot(), poseIn.rot(), delta);

	Se3f poseNew;
	poseNew.trans() = position() + dir;
	poseNew.rot() = rotNew;

	posePrev_ = pose();
	setPose(poseNew);

	return retValue;
}

Se3f RigidBodyTrail::getDockingPose()
{
	Se3f poseRet(dockingPoint_->pose);

	QuatF addRot(QuatF::ID);
	Vect3f localDelta(Vect3f::ZERO);

	switch(dockingPoint_->dockingSide) {
	case TrailPoint::SIDE_BACK:
		localDelta = Vect3f(0, boxMax().y, 0);
		break;
	case TrailPoint::SIDE_FRONT:
		addRot.set(M_PI, Vect3f::K);
		localDelta = Vect3f(0, boxMin().y, 0);
		break;
	case TrailPoint::SIDE_RIGHT:
		addRot.set(M_PI/2.0f, Vect3f::K);
		localDelta = Vect3f(boxMax().x, 0, 0);
		break;
	case TrailPoint::SIDE_LEFT:
		addRot.set(-M_PI/2.0f, Vect3f::K);
		localDelta = Vect3f(boxMin().x, 0, 0);
		break;
	}

	poseRet.rot().postmult(addRot);
	poseRet.xformPoint(localDelta);
	poseRet.trans() = localDelta;

    return poseRet;
}

Vect3f RigidBodyTrail::getEnterPoint()
{
	Vect3f point = dockingPoint_->pose.trans() - position();
	point.Normalize(boundRadius());
	point += position();
	return point;
}

bool RigidBodyTrail::getEnterLine( Vect2f& p1, Vect2f& p2 )
{
	if(!dockingPoint_)
		return false;

	Vect3f lp1, lp2;

	switch(dockingPoint_->dockingSide) {
	case TrailPoint::SIDE_BACK:
		lp1.set(boxMin().x, boxMin().y, boxMax().z);
		lp2.set(boxMax().x, boxMin().y, boxMax().z);
		break;
	case TrailPoint::SIDE_FRONT:
		lp1.set(boxMin().x, boxMax().y, boxMax().z);
		lp2.set(boxMax().x, boxMax().y, boxMax().z);
		break;
	case TrailPoint::SIDE_LEFT:
		lp1.set(boxMax().x, boxMin().y, boxMax().z);
		lp2.set(boxMax().x, boxMax().y, boxMax().z);
		break;
	case TrailPoint::SIDE_RIGHT:
		lp1.set(boxMin().x, boxMin().y, boxMax().z);
		lp2.set(boxMin().x, boxMax().y, boxMax().z);
		break;
	}

	pose().xformPoint(lp1);
	pose().xformPoint(lp2);

	p1.set(lp1.x, lp1.y);
	p2.set(lp2.x, lp2.y);

	return true;
}

