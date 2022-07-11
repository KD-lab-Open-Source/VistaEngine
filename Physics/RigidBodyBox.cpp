#include "StdAfx.h"
#include "RigidBodyPrm.h"
#include "RigidBodyBox.h"
#include "NormalMap.h"
#include "PFTrap.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_BOX, RigidBodyBox)

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
//
//    class RigidBodyBox
//
///////////////////////////////////////////////////////////////

RigidBodyBox::RigidBodyBox() :
	joint_(0),
	collide_(0),
	dtRest_(0),
	angularEvolve(true),
	angle_(0),
	avoidCollisionAtStart_(false)
{
	collide_.setBody(this);
	rigidBodyType = RIGID_BODY_BOX;
}

RigidBodyBox::~RigidBodyBox()
{
	delete joint_;
}

void RigidBodyBox::build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass)
{
	__super::build(prmIn, boxMin, boxMax, mass);
	angularEvolve = !prm().moveVertical;
	if(prm().joint)
		joint_ = new Joint(position(), prm().axes, this, 0, 0, -1, prm().relaxationTime, prm().upperLimits, prm().lowerLimits);
	collide_.set(joint_);
}

void RigidBodyBox::buildGeomMesh(vector<Vect3f>& vertex, bool computeBox)
{
	xassert(!geom_);
	geom_ = new GeomMesh(vertex);
	geomType_ = Geom::GEOM_MESH;
	if(computeBox){
		Vect3f extent;
		geom_->computeCentreOfGravity(centreOfGravityLocal_);
		geom_->computeExtent(centreOfGravityLocal(), extent);
		geom_->setExtent(extent);
		box().setExtent(extent);
		sphere().setRadius(extent.norm());
		radius_ = sqrtf(sqr(extent.x) + sqr(extent.y));
	}
	geom_->getTOI(centreOfGravityLocal(), mass_ * prm().TOI_factor, toi_);
	checkTOI();
}

void RigidBodyBox::initPose(const Se3f& pose)
{
	setPose(pose);
	posePrev_ = pose_;

	pose_.xformVect(velocity_.linear());
	
	checkGround();
	colliding_ = onWater ? WATER_COLLIDING : GROUND_COLLIDING;

	if(!angularEvolve && isBox()){
		Mat3f rot = rotation();
		angle_ = atan2f(rot[1][0], rot[0][0]);
	}
	if(joint_){
		Mat3f axes = prm().axes;
		pose.xformVect(axes.xrow());
		pose.xformVect(axes.yrow());
		pose.xformVect(axes.zrow());
		joint_->setPose(pose.trans(), axes);
	}
}

void RigidBodyBox::setPose(const Se3f& pose)
{
	Se3f poseNew = pose;
	float r(maxRadius() + 2.0f);
	poseNew.trans().x = clamp(pose.trans().x, r, vMap.H_SIZE - 1 - r);
	poseNew.trans().y = clamp(pose.trans().y, r, vMap.V_SIZE - 1 - r);
	__super::setPose(poseNew);
}

void RigidBodyBox::addVelocity(const Vect6f& velocity) 
{ 
	if(angularEvolve)
		velocity_ += velocity;
	else
		velocity_.linear() += velocity.getLinear();
}

void RigidBodyBox::addContact(const Vect3f& point, const Vect3f& normal, float penetration)
{
	contacts_.push_back(ContactSimple(point, normal, this, penetration));
}

void RigidBodyBox::boxEvolve(float dt)
{
	checkGroundCollision();
	collide_.update(contacts_, dt, angularEvolve);
	collide_.computeDV(5);
	addVelocity(collide_.getDV());
	pose_.trans() = centreOfGravity();
	if(avoidCollisionAtStart_){
		colliding_ = 0;
		avoidCollisionAtStart_ = false;
	}
	if(angularEvolve)
		__super::evolve(dt);
	else
		evolveHoldingOrientation(dt);			
	Vect3f centreOfGravity_(centreOfGravityLocal());
	orientation().xform(centreOfGravity_);
	pose_.trans().sub(centreOfGravity_);
	contacts_.clear();
	centreOfGravity_ = centreOfGravity();
	float rmax(maxRadius() + 3.0f);
	float rmin(maxRadius() + 2.0f);
	float clamped(false);
	if(pose_.trans().x < rmax){
		if(pose_.trans().x < rmin){
			pose_.trans().x = rmin;
			clamped = true;
		}
		addContact(centreOfGravity_, Vect3f::I, rmax - pose_.trans().x);
	}else{
		float maxx(vMap.H_SIZE - 1 - rmax);
		if(pose_.trans().x > maxx){
			float maxX(vMap.H_SIZE - 1 - rmin);
			if(pose_.trans().x > maxX){
				pose_.trans().x = maxX;
				clamped = true;
			}
			addContact(centreOfGravity_, Vect3f::I_, pose_.trans().x - maxx);
		}
	}
	if(pose_.trans().y < rmax){
		if(pose_.trans().y < rmin){
			pose_.trans().y = rmin;
			clamped = true;
		}
		addContact(centreOfGravity_, Vect3f::J, rmax - pose_.trans().y);
	}else{
		float maxy(vMap.V_SIZE - 1 - rmax);
		if(pose_.trans().y > maxy){
			float maxY(vMap.V_SIZE - 1 - rmin);
			if(pose_.trans().y > maxY){
				pose_.trans().y = maxY;
				clamped = true;
			}
			addContact(centreOfGravity_, Vect3f::J_, pose_.trans().y - maxy);
		}
	}
	if(clamped){
		pose_.trans().z += box().computeGroundPenetration(centreOfGravity(), orientation());
		colliding_ |= WORLD_BORDER_COLLIDING;
	}
}

bool RigidBodyBox::evolve(float dt)
{
	start_timer_auto();
	sleepCount(dt);
	checkGround();
	posePrev_ = pose();
	if(!asleep()){
		if(isFrozen()){
			if(onIce){
				Vect2i center(position().xi(), position().yi());
				float groundZNew(max(float(vMap.GetApproxAlt(center.x, center.y)), environment->water()->GetZFast(center.x, center.y) - iceLevel()));
				if(fabs(groundZNew - position().z) > 0.1f){
					pose_.trans().z = groundZNew;
					return true;
				}
			}
			return false;
		}
		colliding_ = 0;
		dt += dtRest_;
		const float dtStep = 0.02f;
		while(dt >= dtStep){
			boxEvolve(dtStep);
			dt -= dtStep;
		}
		dtRest_ = dt;
		forceExternalTemporal_ = Vect6f::ZERO;
		return true;
	}
	contacts_.clear();
	forceExternalTemporal_ = Vect6f::ZERO;
	return false;
}

void RigidBodyBox::evolveHoldingOrientation(float dt){
	velocity_.linear() *= 1 - linearDamping;
	velocity_.angular() *= 1 - angularDamping;
	pose_.trans().scaleAdd(velocity_.linear(), dt);
	if((!prm().moveVertical && colliding_) || fabsf(velocity_.angular().z) > 1.e-3f){
		angle_ += velocity_.angular().z * dt;
		Vect3f normal(Vect3f::K);
		if(!prm().moveVertical && colliding_)
			analyseArea(waterAnalysis(), normal);
		pose_.rot().set(angle_, normal);
	}
}

void RigidBodyBox::setImpulse(const Vect3f& linearPulse)
{
	pulseExternal_.set(linearPulse, Vect3f::ZERO);
	angularEvolve = false;
}

void RigidBodyBox::setTemporalForce(const Vect3f& linearForce)
{
	forceExternalTemporal_.set(linearForce, Vect3f::ZERO);
	angularEvolve = false;
}

void RigidBodyBox::avoidCollisionAtStart() 
{ 
	avoidCollisionAtStart_ = true; 
	colliding_ = 0; 
}

void RigidBodyBox::startFall(const Vect3f& point)
{
	Vect3f pulse(position());
	pulse.sub(point);
	pulse.z = 0;
	if(pulse.norm2() < 0.10f)
		pulse.set(logicRNDfrnd(1.0f), logicRNDfrnd(1.0f), 0);
	pulse.normalize();
	if(prm().fence){
		Vect3f axisY(Vect3f::J);
		orientation().xform(axisY);
		pulse = axisY * SIGN(axisY.dot(pulse));
	}
	pulse.scale(mass() * 50.f);
	applyExternalImpulse(pulse);
}

void RigidBodyBox::setAngle(float angle)
{ 
	angle_ = angle;
	if(!isFrozen()){
		Vect3f normal(Vect3f::K);
		if(!prm().moveVertical && colliding_)
			analyseArea(waterAnalysis(), normal);
		pose_.rot().set(angle_, normal);
	}
}
