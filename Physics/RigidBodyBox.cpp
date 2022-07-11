#include "StdAfx.h"
#include "RigidBodyBox.h"
#include "NormalMap.h"
#include "Math\ConstraintHandlerSimple.h"

///////////////////////////////////////////////////////////////

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_BOX, RigidBodyBox)

///////////////////////////////////////////////////////////////
//
//    class RigidBodyBox
//
///////////////////////////////////////////////////////////////

RigidBodyBox::RigidBodyBox() :
	joint_(0),
	posePrev_(Se3f::ID),
	iceLevel_(0.0f),
	onIce_(false),
	onIcePrev_(false),
	isFrozen_(false),
	angle_(0.0f),
	avoidCollisionAtStart_(false)
{
	setRigidBodyType(RIGID_BODY_BOX);
}

RigidBodyBox::~RigidBodyBox()
{
	detach();
	delete joint_;
}

void RigidBodyBox::build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass)
{
	__super::build(prmIn, center, extent, mass);
	setAngularEvolve(!prm().moveVertical);
	if(prm().joint)
		joint_ = new JointSimple(position(), prm().axes, this, prm().relaxationTime, prm().upperLimits, prm().lowerLimits);
}

void RigidBodyBox::buildGeomMesh(vector<Vect3f>& vertex, bool computeBox)
{
	GeomMesh* mesh = new GeomMesh(vertex);
	if(computeBox){
		setCentreOfGravityLocal(mesh->computeCentreOfGravity());
		setBoundRadius(extent().norm());
		setRadius(sqrtf(sqr(extent().x) + sqr(extent().y)));
	}else
		mesh->setExtent(extent());
	setGeom(mesh);
	Vect3f toi;
	mesh->getTOI(centreOfGravityLocal(), core()->mass() * prm().TOI_factor, toi);
	core()->massMatrix().setTOI(toi);
}

void RigidBodyBox::initPose(const Se3f& poseNew)
{
	setPose(poseNew);
	posePrev_ = pose();

	orientation().xform(core()->velocity().linear());
	
	checkGround();
	if(avoidCollisionAtStart_)
		colliding_ = onWater() ? WATER_COLLIDING : GROUND_COLLIDING;

	if(!angularEvolve() && isBox())
		setAngle(computeAngleZ());
	if(joint_){
		Mat3f axes = prm().axes;
		pose().xformVect(axes.xrow());
		pose().xformVect(axes.yrow());
		pose().xformVect(axes.zrow());
		joint_->setPose(position(), axes);
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

void RigidBodyBox::boxEvolve(float dt)
{
	if(avoidCollisionAtStart_){
		colliding_ = 0;
		avoidCollisionAtStart_ = false;
	}
	__super::boxEvolve(dt);
	if(!angularEvolve() && ((!prm().moveVertical && colliding_) || fabsf(core()->velocity().angular().z) > 1.e-3f)){
		addPathTrackingAngle(core()->velocity().angular().z * dt);
		Vect3f normal(Vect3f::K);
		if(!prm().moveVertical && colliding_)
			analyzeArea(waterAnalysis(), normal);
		setOrientation(QuatF(angle(), normal));
	}
}

void RigidBodyBox::preEvolve()
{
	posePrev_ = pose();
	colliding_ = 0;
}

bool RigidBodyBox::evolve(float dt)
{
	start_timer_auto();
	sleepCount(dt);
	checkGround();
	if(!asleep()){
		if(isFrozen()){
			posePrev_ = pose();
			if(onIce()){
				Vect2i center(position().xi(), position().yi());
				float groundZNew(max(float(vMap.getApproxAlt(center.x, center.y)), environment->water()->GetZFast(center.x, center.y) - iceLevel_));
				if(fabs(groundZNew - position().z) > 0.1f){
					setPoseZ(groundZNew);
					return true;
				}
			}
			return false;
		}
		return true;
	}
	return false;
}

void RigidBodyBox::addImpulseLinear(const Vect3f& linearPulse)
{
	__super::addImpulse(linearPulse, Vect3f::ZERO);
	setAngularEvolve(false);
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
	pulse.scale(core()->mass() * 50.f);
	__super::addImpulse(pulse, Vect3f::ZERO);
}

void RigidBodyBox::attach()
{ 
	collide_->attach(this);
	if(joint_)
		collide_->attach(joint_);
}

void RigidBodyBox::detach()
{ 
	if(joint_)
		collide_->detach(joint_);
	collide_->detach(this);
}

void RigidBodyBox::setParameters(float gravity, float friction, float restitution)
{
	core()->massMatrix().setGravityZ(-gravity);
	friction_ = friction;
	restitution_ = restitution;
}

void RigidBodyBox::makeFrozen(bool onIce)
{ 
	detach();
	isFrozen_ = true;
	onIce_ = onIce;
	iceLevel_ = onIce ? environment->water()->GetZFast(position().xi(), position().yi()) - position().z : 0.0f;
}

void RigidBodyBox::unFreeze()
{ 
	if(isFrozen_){
		isFrozen_ = false; 
		attach();
	}
}

bool RigidBodyBox::iceMapCheck( int xc, int yc, int r )
{
	if(!environment->temperature())
		return false;

	int xL = (xc-r)>>environment->temperature()->gridShift();
	int xR = (xc+r)>>environment->temperature()->gridShift();
	int yT = (yc-r)>>environment->temperature()->gridShift();
	int yD = (yc+r)>>environment->temperature()->gridShift();

	int x,y;
	for(y=yT; y<=yD; y++)
		for(x=xL; x<=xR; x++)
			if(!environment->temperature()->checkTile(x,y)) return false;

	return true;
}

bool RigidBodyBox::iceMapCheckPrev( int xc, int yc, int r )
{
	if(!environment->temperature())
		return false;

	int xL = (xc-r)>>environment->temperature()->gridShift();
	int xR = (xc+r)>>environment->temperature()->gridShift();
	int yT = (yc-r)>>environment->temperature()->gridShift();
	int yD = (yc+r)>>environment->temperature()->gridShift();

	int x,y;
	for(y=yT; y<=yD; y++)
		for(x=xL; x<=xR; x++)
			if(!environment->temperature()->checkTilePrev(x,y)) return false;

	return true;
}

void RigidBodyBox::checkGround()
{
	if(isFrozen() && !onIce())
		return;
	bool onAnyWaterPrev = onWater() || onLowWater();
	onIcePrev_ = onIce();
	onLowWater_ = checkLowWater(position(), radius());
	int position_x = position().xi();
	int position_y = position().yi();
	bool pointOnWater = environment->water()->GetZ(position_x, position_y) > vMap.getApproxAlt(position_x, position_y) + 2;
	onIce_ = pointOnWater && environment->temperature() && environment->temperature()->checkTileWorld(position_x, position_y);
	if(onLowWater()) {
		onWater_ = checkWater(position_x, position_y, 0);
		if(onWater() || onIce())
			onLowWater_ = false;
	} else
		onWater_ = false;
	// Проверки на замерзание.
	if(colliding() && onAnyWaterPrev && onIce() 
		&& environment->temperature()->isOnIce(posePrev().trans(), radius())){
		bool fullIce = iceMapCheck(position_x, position_y, round(radius()));
		bool fullIcePrev = iceMapCheckPrev(position_x, position_y, round(radius()));
		if(fullIce && !fullIcePrev) {
			makeFrozen(true);
			onWater_ = false;
		} else if(fullIce && fullIcePrev) {
			onIce_ = true;
			onWater_ = false;
		} else {
			onIce_ = false;
			onWater_ = true;
		}
	}
	if(!onIce()){
		if(isFrozen())
			unFreeze();
		else if(onIcePrev())
			awake();
	}
}

void RigidBodyBox::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(isFrozen_, "isFrozen", 0);
	if(isFrozen())
		ar.serialize(iceLevel_, "iceLevel", 0);
}