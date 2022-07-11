#include "stdafx.h"
#include "AI\PFTrap.h"
#include "NormalMap.h"
#include "GlobalAttributes.h"
#include "RigidBodyBase.h"

#include "CD\CD2D.h"
#include "CD\CDSphere.h"
#include "CD\CDDual.h"


///////////////////////////////////////////////////////////////

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_BASE, RigidBodyBase)
REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_ENVIRONMENT, RigidBodyEnvironment)

///////////////////////////////////////////////////////////////
//
//    class RigidBodyCollision
//
///////////////////////////////////////////////////////////////

RigidBodyCollision::RigidBodyCollision() : 
	pose_(Se3f::ID),
	centreOfGravityLocal_(Vect3f::ZERO),
	centreOfGravity_(Vect3f::ZERO),
	sphereCollision_(true),
	sphere_(0),
	geom_(0)
{
}

RigidBodyCollision::~RigidBodyCollision() 
{
	setGeom(0);
}

bool RigidBodyCollision::checkLowWater(const Vect3f& pos, float r) const 
{
	if(!environment->water()) 
		return false;
	return environment->water()->isWater(pos, r);
}

///////////////////////////////////////////////////////////////
//
//    class WaterScan
//
///////////////////////////////////////////////////////////////

RigidBodyBase::WaterScan::WaterScan(int x_, int y_, int tileShift_)
{
	tileShift = tileShift_;
	tileScale = 1 << tileShift_;
	startX = x_ >> tileShift_;
	y = y_ >> tileShift_;
	tileDeltaXStart = x_ - (startX << tileShift_);
	tileDeltaY = y_ - (y << tileShift_);
}

int RigidBodyBase::WaterScan::goNextX()
{
	if(tileDeltaX == tileScale){
		++x;
		nextTile(tile10, tile11);
		tileDeltaX = 0;
	}
	int valueH = tileHLeft + tileDeltaHX * tileDeltaX;
	++tileDeltaX;
	return valueH >> 16;
}

void RigidBodyBase::WaterScan::goNextY()
{
	x = startX;
	tileDeltaX = tileDeltaXStart;
	if(tileDeltaY == tileScale){
		++y;
		tileDeltaY = 0;
	}
	nextTile(environment->water()->Get(x, y).SZ(), environment->water()->Get(x, y + 1).SZ());
	++tileDeltaY;
}

void RigidBodyBase::WaterScan::nextTile(int tile00, int tile01)
{
	tile10 = environment->water()->Get(x + 1, y).SZ();
	tile11 = environment->water()->Get(x + 1, y + 1).SZ();
	tileHLeft = tile00 + ((tile10 - tile00) >> tileShift) * tileDeltaY;
	int tileHRight = tile10 + ((tile11 - tile10) >> tileShift) * tileDeltaY;
	tileDeltaHX = (tileHRight - tileHLeft) >> tileShift;
}

///////////////////////////////////////////////////////////////
// RigidBodyBase
///////////////////////////////////////////////////////////////

RigidBodyBase::RigidBodyBase() :
	prm_(0),
	radius_(0),
	debugColor_(Color4c::WHITE),
	awakeCounter_(0),
	averagePosition_(Vect3f::ZERO),
	averageOrientation_(Vect3f::ZERO),
	colliding_(0),
	onWater_(false),
	onLowWater_(false),
	rigidBodyType_(RIGID_BODY_BASE),
	ptBoundCheck_(false)
{
}

void RigidBodyBase::build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass)
{
	prm_ = &prmIn;
	radius_ = max(sqrt(sqr(center.x + extent.x) + sqr(center.y + extent.y)), 
		sqrt(sqr(center.x - extent.x) + sqr(center.y - extent.y)));
	setGeom(new GeomBox(extent));
	setCentreOfGravityLocal(center);
	setBoundRadius(extent.norm());
	setSphereCollision(prm().sphereCollision);
	if(!prm().sleepAtStart)
		awake();
}

bool RigidBodyBase::evolve(float dt)
{
	if(!asleep()) {
		averagePosition_.interpolate(averagePosition_, position(), prm().average_movement_tau);
		averageOrientation_.interpolate(averageOrientation_, Vect3f(orientation()[0], orientation()[1], orientation()[2]), prm().average_movement_tau);
		float movement = averagePosition_.distance2(position()) + averageOrientation_.distance2(Vect3f(orientation()[0], orientation()[1], orientation()[2]))*100;
		if(prm().enableSleeping && movement < prm().sleepMovemenThreshould){
			if(!--awakeCounter_)
				sleep();
		}else
			awake();
	}
	return false;
}

bool RigidBodyBase::checkWater(int xc, int yc, int r) const 
{
	return pathFinder->checkWater(xc, yc, r);
}

bool RigidBodyBase::bodyIntersect(const RigidBodyBase* body, const Vect2f& bodyPosition, const Mat2f& bodyOrientation)
{
	if(ptBoundCheck() && body->ptBoundCheck())
		return penetrationRectangleRectangle(extent(), position2D(), Mat2f(computeAngleZ()), body->extent(), bodyPosition, bodyOrientation);

	if(ptBoundCheck())
		return penetrationCircleRectangle(body->radius(), bodyPosition, extent(), position2D(), Mat2f(computeAngleZ()));

	if(body->ptBoundCheck())
		return penetrationCircleRectangle(radius(), position2D(), body->extent(), bodyPosition, bodyOrientation);

	if(isEnvironment() || !prm().controled_by_points)
		return position2D().distance2(bodyPosition) < sqr(radius());

	if(radius() >= body->radius())
		return position2D().distance2(bodyPosition) < sqr(radius());

	return position2D().distance2(bodyPosition) < sqr(radius() + body->radius());
}

///////////////////////////////////////////////////////////////
//
//    class RigidBodyEnvironment
//
///////////////////////////////////////////////////////////////

RigidBodyEnvironment::RigidBodyEnvironment() :
	RigidBodyBase(), 
	angle_(0), 
	pointAreaAnalize_(false), 
	holdOrientation_(false),
	verticalOrientation_(false)
{ 
	setColor(Color4c::GREEN); 
	setRigidBodyType(RIGID_BODY_ENVIRONMENT);
}

void RigidBodyEnvironment::build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass)
{
	__super::build(prmIn, center, extent, mass);
	pointAreaAnalize_ = radius() < GlobalAttributes::instance().analyzeAreaRadius;
	holdOrientation_ = false;
	verticalOrientation_ = false;
}

void RigidBodyEnvironment::initPose( const Se3f& pose )
{
	setPose(pose);
	float angle = computeAngleZ();
	if(!isEq(angle, angle_, 0.02f))
		angle_ = angle;
	placeToGround();
}

bool RigidBodyEnvironment::evolve( float dt )
{
	start_timer_auto();
	__super::evolve(dt);
	onWater_ = checkWater(position().xi(), position().yi(), 0);
	colliding_ = GROUND_COLLIDING;
	if(!asleep()){
		placeToGround();
		return true;
	}
	return false;
}

void RigidBodyEnvironment::setRadius(float radiusNew)
{
	__super::setRadius(radiusNew);
	pointAreaAnalize_ = radius() < GlobalAttributes::instance().analyzeAreaRadius;
}

void RigidBodyEnvironment::placeToGround()
{
	Vect3f normal;
	if(prm().canEnablePointAnalyze && pointAreaAnalize_){
		setPoseZ(float(vMap.getApproxAlt(position().xi(), position().yi())));
		normal = normalMap->normalLinear(position().x, position().y);
	}else{
		if(holdOrientation_)
			setPoseZ(position().z + box()->computeGroundPenetration(centreOfGravity(), orientation()));
		else
			setPoseZ(analyzeArea(false, normal));
	}
	if(!prm().holdOrientation && !holdOrientation_){
		QuatF orientationNew(angle_, Vect3f::K, false);
		normal.normalize();
		orientationNew.premult(QuatF(acosf(normal.z), Vect3f(-normal.y, normal.x, 0.0f)));
		setOrientation(orientationNew);
	}else{
		if(verticalOrientation_)
			setOrientation(QuatF(angle_, Vect3f::K, false));
	}
}

float RigidBodyBase::analyzeWaterFast(const Vect2i& center, int r, Vect3f& normalNonNormalized)
{
	int Sz = 0;
	int Sxz = 0;
	int Syz = 0;
	int delta = max(vMap.w2m(r), 1);
	int xc = clamp(vMap.w2m(center.x), delta, vMap.GH_SIZE - delta - 4);
	int yc = clamp(vMap.w2m(center.y), delta, vMap.GV_SIZE - delta - 4);
	WaterScan waterScan(xc - delta, yc - delta, 2);
	for(int y = -delta; y <= delta; y++){
		waterScan.goNextY();
		for(int x = -delta; x <= delta; x++){
			int z = max(vMap.getZGrid(x + xc, y + yc), waterScan.goNextX());
			Sz += z;
			Sxz += x*z;
			Syz += y*z;
		}
	}
	float N = sqr(2.0f * delta + 1.0f);
	float t = 3.0f / (N * delta * (delta + 1.0f));
	normalNonNormalized.set(-Sxz * t, -Syz * t, 4.0f);
	return (float)Sz / N;
}

float RigidBodyBase::analyzeArea(bool analyseWater, Vect3f& normalNonNormalized)
{
	if(analyseWater)
		return analyzeWaterFast(position2D(), round(radius()), normalNonNormalized);
	return vMap.analyzeArea(position2D(), round(radius()), normalNonNormalized);
}

///////////////////////////////////////////////////////////////

RigidBodyBase* RigidBodyBase::buildRigidBody(const RigidBodyType& rigidBodyType, const RigidBodyPrm* prm, const Vect3f& center, const Vect3f& extent, float mass)
{
	RigidBodyBase* body = RigidBodyFactory::instance().create(rigidBodyType);
	body->build(*prm, center, extent, mass);
	return body;
}

///////////////////////////////////////////////////////////////

bool RigidBodyBase::bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo) 
{ 
	if(body->isUnitRagDoll())
		return body->bodyCollision(this, contactInfo); 
	if(collisionObject()->bodyCollision(this, body, contactInfo)){
		contactInfo.bodyPart1 = bodyPart();
		contactInfo.bodyPart2 = body->bodyPart();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////

int RigidBodyBase::computeBoxSectionPenetration(Vect3f& contactPoint, const Vect3f& sectionBegin, const Vect3f& sectionEnd) const
{
	Vect3f center(centreOfGravity());
	contactPoint.sub(sectionBegin, center);
	orientation().invXform(contactPoint);
	Vect3f end;
	end.sub(sectionEnd, center);
	orientation().invXform(end);
	int collision = box()->box().computeBoxSectionPenetration(contactPoint, end)? 0 : -1;
	orientation().xform(contactPoint);
	contactPoint.add(center);
	return collision;
}
