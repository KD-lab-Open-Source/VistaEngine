#include "stdafx.h"
#include "PFTrap.h"
#include "NormalMap.h"
#include "GlobalAttributes.h"
#include "RigidBodyPrm.h"
#include "RigidBodyUnit.h"

#include "CD/CDSphere.h"
#include "CD/CDDual.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_BASE, RigidBodyBase)
REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_ENVIRONMENT, RigidBodyEnvironment)

FORCE_REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_TRAIL, RigidBodyTrail)

///////////////////////////////////////////////////////////////
// RigidBodyBase
///////////////////////////////////////////////////////////////

RigidBodyBase::RigidBodyBase()
:	prm_(0),
	pose_(Se3f::ID),
	centreOfGravityLocal_(Vect3f::ZERO),
	box_(Vect3f::ZERO),
	radius_(0),
	sphere_(0),
	geom_(0),
	geomType_(Geom::GEOM_SPHERE),
	debugColor_(WHITE),
	awakeCounter_(0),
	averagePosition_(Vect3f::ZERO),
	averageOrientation_(Vect3f::ZERO),
	unmovable_(false),
	colliding_(0)
{
	onWater = false;
	onWaterPrev = false;
	onLowWater = false;
	onIce = false;
	onIcePrev = false;
	rigidBodyType = RIGID_BODY_BASE;
}

RigidBodyBase::~RigidBodyBase() 
{
	delete geom_;
}

void RigidBodyBase::build( const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass )
{
	prm_ = &prmIn;
	radius_ = max(sqrt(sqr(boxMax.x) + sqr(boxMax.y)), sqrt(sqr(boxMin.x) + sqr(boxMin.y)));
	
	Vect3f extent(boxMax);
	extent -= boxMin;
	extent /= 2.0f;
	box_.setExtent(extent);
	centreOfGravityLocal_.sub(boxMax, extent);
	sphere_.setRadius(extent.norm());

	delete geom_;
	geom_ = 0;

	if(prm().sphereCollision)
		geomType_ = Geom::GEOM_SPHERE;
	else
		geomType_ = Geom::GEOM_BOX;

	if(!prm().sleepAtStart)
		awake();
}

Vect3f RigidBodyBase::centreOfGravity() const 
{ 
	Vect3f centreOfGravity_(centreOfGravityLocal_);
	pose().xformPoint(centreOfGravity_);
	return centreOfGravity_; 
}

bool RigidBodyBase::evolve( float dt )
{
	if(!asleep()) {
		averagePosition_.interpolate(averagePosition_, position(), prm().average_movement_tau);
		averageOrientation_.interpolate(averageOrientation_, Vect3f(orientation()[0], orientation()[1], orientation()[2]), prm().average_movement_tau);
		float movement = averagePosition_.distance2(position()) + averageOrientation_.distance2(Vect3f(orientation()[0], orientation()[1], orientation()[2]))*100;
		if(prm().enableSleeping && movement < prm().sleepMovemenThreshould)
			--awakeCounter_;
		else
			awakeCounter_ = prm().awakeCounter;
	}
	
	return false;
}

bool RigidBodyBase::checkWater(int xc, int yc, int r ) const 
{
	return pathFinder->checkWater(xc, yc, r);
}

bool RigidBodyBase::checkLowWater(const Vect3f& pos, float r ) const 
{
	if(!environment->water()) 
		return false;
	return environment->water()->isWater(pos, r);
}

bool RigidBodyBase::bodyIntersect( const RigidBodyBase* body, MatX2f bodyPose )
{
	if(isEnvironment() || !prm().controled_by_points)
		return position2D().distance2(bodyPose.trans) < sqr(radius());

	if(radius() >= body->radius())
		return position2D().distance2(bodyPose.trans) < sqr(radius());

	return position2D().distance2(bodyPose.trans) < sqr(radius() + body->radius());
/*
	float angle0 = -atan2f(rotation()[0][1], rotation()[1][1]);
	Mat2f rot0(angle0);

	Mat2f relativeMat = bodyPose.rot;
	relativeMat.invert();
	relativeMat *= rot0;

	return penetrationRectangleRectangle(extent(), position2D(), body->extent(), bodyPose.trans, relativeMat);
*/
}

bool RigidBodyBase::pointInBody( const Vect2f& point )
{
	return position2D().distance2(point) < sqr(radius());
}

///////////////////////////////////////////////////////////////
// RigidBodyEnvironment
///////////////////////////////////////////////////////////////

RigidBodyEnvironment::RigidBodyEnvironment() :
	RigidBodyBase(), 
	angle_(0), 
	pointAreaAnalize_(false), 
	holdOrientation_(false),
	verticalOrientation_(false)
{ 
	setColor(GREEN); 
	rigidBodyType = RIGID_BODY_ENVIRONMENT;
}

void RigidBodyEnvironment::build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass)
{
	__super::build(prmIn, boxMin, boxMax, mass);
	pointAreaAnalize_ = radius() < GlobalAttributes::instance().analyzeAreaRadius;
	holdOrientation_ = false;
	verticalOrientation_ = false;
}

void RigidBodyEnvironment::initPose( const Se3f& pose )
{
	setPose(pose);
	angle_ = -atan2f(rotation()[0][1], rotation()[1][1]);
	placeToGround();
}

bool RigidBodyEnvironment::evolve( float dt )
{
	start_timer_auto();

	__super::evolve(dt);

	onWater = pathFinder->checkWater(position().xi(), position().yi(), 0);
	colliding_ = GROUND_COLLIDING;

	if(!asleep()){
		placeToGround();
		return true;
	}
	
	return false;
}

void RigidBodyEnvironment::setRadius(float radius)
{
	__super::setRadius(radius);
	pointAreaAnalize_ = radius_ < GlobalAttributes::instance().analyzeAreaRadius;
}

void RigidBodyEnvironment::placeToGround()
{
	Vect3f normal(Vect3f::K);
	float z = 0;

	if(prm().canEnablePointAnalyze && pointAreaAnalize_){
		z = vMap.GetApproxAlt(position().xi(), position().yi());
		normal = normalMap->normalLinear(position().x, position().y);
	}else{
		if(holdOrientation_){
			if(verticalOrientation_)
				pose_.rot().set(angle_, Vect3f::K, false);
			z = pose_.trans().z + box().computeGroundPenetration(centreOfGravity(), orientation());
		}else
			z = analyseArea(false, normal);
	}
	
	Vect3f cross = Vect3f::K % normal;
	float len = cross.norm();

	if(!prm().holdOrientation && !holdOrientation_){
		pose_.rot() = QuatF::ID;
		if(len > 0.001f)
			pose_.rot().set(acosf(dot(Vect3f::K, normal)/(normal.norm() + 1e-5f)), cross);
		pose_.rot().postmult(QuatF(angle_, Vect3f::K, false));
	}

	pose_.trans().z = z;
}

///////////////////////////////////////////////////////////////
// Вспомогательные общие функции.
///////////////////////////////////////////////////////////////

class WaterScan {
	
	int tile00;
	int tile01;
	int tile11;
	int tile10;

	int tileDeltaX;
	int tileDeltaY;

	int startX;
	int tileDeltaXStart;

	int x;
	int y;

	int tileHLeft;
	int tileHRight;

	int tileScale;
	int tileShift;

	int tileDeltaHX;

public:

	WaterScan(int x_, int y_, int tileShift_){
        tileShift = tileShift_;
		tileScale = 1 << tileShift_;
		startX = x = x_ >> tileShift_;
		y = y_ >> tileShift_;
		tileDeltaX = tileDeltaXStart = x_ - (x << tileShift_);
		tileDeltaY = y_ - (y << tileShift_);

		tile00 = environment->water()->Get(x,y).z;
		tile10 = environment->water()->Get(x+1,y).z;
		tile01 = environment->water()->Get(x,y+1).z;
		tile11 = environment->water()->Get(x+1,y+1).z;

		tileHLeft = tile00 + (tile10-tile00)/tileScale*tileDeltaY;
		tileHRight = tile10 + (tile11-tile10)/tileScale*tileDeltaY;

		tileDeltaHX = (tileHRight - tileHLeft)/tileScale;

	}

	int goNextX(){
		int valueH = tileHLeft + tileDeltaHX * tileDeltaX;
		tileDeltaX++;
		if(tileDeltaX == tileScale){
			tile00 = tile10;
			tile01 = tile11;
			x++;
			tile10 = environment->water()->Get(x+1,y).z;
			tile11 = environment->water()->Get(x+1,y+1).z;
			tileHLeft = tile00 + ((tile10-tile00)>>tileShift)*tileDeltaY;
			tileHRight = tile10 + ((tile11-tile10)>>tileShift)*tileDeltaY;
			tileDeltaHX = (tileHRight - tileHLeft)>>tileShift;
			tileDeltaX = 0;
		}
		return valueH;
	}

	void goNextY(){
		tileDeltaY++;
		x = startX;
		tileDeltaX = tileDeltaXStart;
		
		if(tileDeltaY == tileScale){
			y++;
			tileDeltaY = 0;
		}

		tile00 = environment->water()->Get(x,y).z;
		tile10 = environment->water()->Get(x+1,y).z;
		tile01 = environment->water()->Get(x,y+1).z;
		tile11 = environment->water()->Get(x+1,y+1).z;

		tileHLeft = tile00 + (tile10-tile00)/tileScale*tileDeltaY;
		tileHRight = tile10 + (tile11-tile10)/tileScale*tileDeltaY;

		tileDeltaHX = (tileHRight - tileHLeft)/tileScale;

	}
};

///////////////////////////////////////////////////////////////

float RigidBodyBase::analyseArea(bool analyseWater, Vect3f& normalNonNormalized)
{
	int Sz = 0;
	int Sxz = 0;
	int Syz = 0;
	int delta = max(vMap.w2m(round(radius())), 1);
	int xc = clamp(vMap.w2m(position2D().x), delta, vMap.GH_SIZE - delta - 1);
	int yc = clamp(vMap.w2m(position2D().y), delta, vMap.GV_SIZE - delta - 1);


	if(analyseWater){
		WaterScan waterScan(xc - delta, yc - delta, 2);
		for(int y = -delta;y <= delta;y++){
			for(int x = -delta;x <= delta;x++){
				int z = vMap.GVBuf[vMap.offsetGBuf(x + xc, y + yc)];
				z += waterScan.goNextX() >> 16;
				Sz += z;
				Sxz += x*z;
				Syz += y*z;
			}
			waterScan.goNextY();
		}
	}
	else
		for(int y = -delta;y <= delta;y++){
			for(int x = -delta;x <= delta;x++){
				int z = vMap.GVBuf[vMap.offsetGBuf(x + xc, y + yc)];
				Sz += z;
				Sxz += x*z;
				Syz += y*z;
			}
		}

	float t14 = delta*delta;
	float t13 = 2.0f*delta+1.0f;
	float t9 = 2.0f*delta;
	float t8 = 6.0f*t14;
	float N = 4.0f*t14+t9+t13;
	float A = 3.0f*Sxz/delta/(t9+1.0f+t8+(3.0f+(4.0f*delta+2.0f)*delta)*delta);
	float B = 3.0f*Syz/delta/(t8+t13+(3.0f+(4.0f*delta+2.0f)*delta)*delta);
	normalNonNormalized.set(-A, -B, 4);
	return (float)Sz/N;
}

///////////////////////////////////////////////////////////////

RigidBodyBase* RigidBodyBase::buildRigidBody(const RigidBodyType& rigidBodyType, const RigidBodyPrm* prm, const Vect3f& boxMin, const Vect3f& boxMax, float mass )
{
	RigidBodyBase* body = RigidBodyFactory::instance().create(rigidBodyType);
	body->build(*prm, boxMin, boxMax, mass);
	return body;
}

///////////////////////////////////////////////////////////////

bool RigidBodyBase::bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo) 
{ 
	if(body->isUnitRagDoll())
		return body->bodyCollision(this, contactInfo); 
	if(geom()->bodyCollision(this, body, contactInfo)){
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
	pose_.rot().invXform(contactPoint);
	Vect3f end;
	end.sub(sectionEnd, center);
	pose_.rot().invXform(end);
	int collision(-1);
	if(box_.box().computeBoxSectionPenetration(contactPoint, end))
		collision = 0;
	pose_.rot().xform(contactPoint);
	contactPoint.add(center);
	return collision;
}
