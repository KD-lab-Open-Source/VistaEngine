#include "StdAfx.h"
#include "RigidBodyPrm.h"
#include "RigidBodyPhysics.h"
#include "NormalMap.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysics
//
///////////////////////////////////////////////////////////////

RigidBodyPhysics::RigidBodyPhysics() :
mass_(0),
toi_(Vect3f::ZERO),
velocity_(Vect6f::ZERO),
linearDamping(0),
angularDamping(0),
forceExternal_(Vect6f::ZERO),
pulseExternal_(Vect6f::ZERO),
forceExternalTemporal_(Vect6f::ZERO),
gravity_(Vect3f::ZERO),
restitution_(0),
relaxationTime_(0),
friction_(0),
waterAnalysis_(false),
waterWeight_(0),
waterLevel_(0),
iceLevel_(0),
isFrozen_(false),
posePrev_(Se3f::ID)
{
	checkTOI();
	setColor(RED);
}

void RigidBodyPhysics::build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass)
{
	__super::build(prmIn, boxMin, boxMax, mass);
	linearDamping = prm().linearDamping;
	angularDamping = prm().angularDamping;
	mass_ = mass;
	geom()->getTOI(mass_ * prm().TOI_factor, toi_);
	gravity_.set(0, 0, -prm().gravity);
	friction_ = prm().friction;
	restitution_ = prm().restitution;
	relaxationTime_  = prm().relaxationTime;
	waterAnalysis_ = prm().waterAnalysis;
}

void RigidBodyPhysics::checkTOI()
{
	for(int i = 0; i < 3; ++i)
		if(fabsf(toi_[i]) < 1.e-5f)
			toi_[i] = 1.e-5f;
}

void RigidBodyPhysics::computeForceAndTOI(Vect6f& force, MassMatrix& massMatrixWorldInverse_, float dt, bool coriolisForce)
{
	Mat3f r(orientation());
	Mat3f r_i;
	r_i.invert(r);
	Mat3f toi_world = Mat3f::ZERO;
	Mat3f toi_world_i = Mat3f::ID;
	for(int i = 0; i < 3; ++i){
		toi_world[i][i] = toi_[i];
		toi_world_i[i][i] /= toi_[i];
	}
	toi_world.postmult(r_i);
	toi_world.premult(r);
	MassMatrix massMatrixWorld_(mass_, toi_world);
	toi_world_i.postmult(r_i);
	toi_world_i.premult(r);
	massMatrixWorldInverse_.set(1.0f / mass_, toi_world_i);

	force.linear().scale(gravity_, mass_);
	if(coriolisForce){
		massMatrixWorld_.multiply(force.angular(), velocity_.angular());
		force.angular().postcross(velocity_.angular());
	}else
		force.angular() = Vect3f::ZERO;
	force += forceExternal_;
	force += forceExternalTemporal_;
	pulseExternal_ /= dt;
	force += pulseExternal_;
	pulseExternal_ = Vect6f::ZERO;
}

void RigidBodyPhysics::sleepCount(float dt)
{
	__super::evolve(dt);
}

bool RigidBodyPhysics::evolve(float dt)
{
	velocity_.linear() *= 1 - linearDamping;
	velocity_.angular() *= 1 - angularDamping;
	pose_.trans().scaleAdd(velocity_.linear(), dt);
	float absq = velocity_.angular().norm2();
	if(absq > 1.e-3f){
		absq = sqrtf(absq);
		QuatF q_local;
		float theta = absq * dt / 2.0f;
		float normsin = sinf(theta) / absq;
		q_local[0] = cosf(theta);
		for(int i = 1; i < 4; i++)
			q_local[i] = velocity_[i + 2] * normsin;
		pose_.rot().premult(q_local);
		pose_.rot().normalize();
	}
	return true;
}

void RigidBodyPhysics::applyExternalForce(const Vect3f& force, const Vect3f& point)
{
	forceExternal_.linear() += force;
	Vect3f temp;
	temp.sub(point, centreOfGravity());
	temp.postcross(force);
	forceExternal_.angular() += temp;

}

void RigidBodyPhysics::applyExternalImpulse(const Vect3f& pulse, const Vect3f& point)
{
	pulseExternal_.linear() += pulse;
	Vect3f temp;
	temp.sub(point, centreOfGravity());
	temp.precross(pulse);
	pulseExternal_.angular() += temp;
}

void RigidBodyPhysics::setImpulse(const Vect3f& linearPulse, const Vect3f& angularPulse)
{
	pulseExternal_.set(linearPulse, angularPulse);
}

void RigidBodyPhysics::setTemporalForce(const Vect3f& linearForce, const Vect3f& angularForce)
{
	forceExternalTemporal_.set(linearForce, angularForce);
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

void RigidBodyPhysics::checkGroundCollision()
{
	if(onWater && waterAnalysis()){
		if(geom()->waterCollision(this)){
			colliding_ = WATER_COLLIDING;
			Vect3f waterImpulse(environment->water()->GetVelocity(position().xi(), position().yi()), 0);
			pulseExternal_.linear().scaleAdd(waterImpulse, mass_ * waterWeight_);
		}
	}
	else{
		if(geom()->groundCollision(this))
			colliding_ = GROUND_COLLIDING;
	}
}

void RigidBodyPhysics::makeFrozen(bool onIce_)
{ 
	isFrozen_ = true;
	onIce = onIce_;
	setIceLevel(onIce ? environment->water()->GetZFast(position().xi(), position().yi()) - position().z : 0.0f);
}

bool RigidBodyPhysics::iceMapCheck( int xc, int yc, int r )
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

bool RigidBodyPhysics::iceMapCheckPrev( int xc, int yc, int r )
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

void RigidBodyPhysics::checkGround()
{
	if(isFrozen() && !onIce)
		return;

	onWaterPrev = onWater;
	onIcePrev = onIce;

	bool onLowWaterPrev = onLowWater;
	onLowWater = checkLowWater(position(), radius());
	int position_x = position().xi();
	int position_y = position().yi();
	bool pointOnWater = environment->water()->GetZ(position_x, position_y) > vMap.GetApproxAlt(position_x, position_y) + 2;
	onIce = pointOnWater && environment->temperature() && environment->temperature()->checkTileWorld(position_x, position_y);
	if(onLowWater) {
		onWater = checkWater(position().xi(), position().yi(), 0.0f);
		if(onWater || onIce)
			onLowWater = false;
	} else
		onWater = false;

	// Проверки на замерзание.
	if(colliding() && (onWaterPrev || onLowWaterPrev) && onIce)
		if(environment->temperature()->isOnIce(posePrev().trans(), radius())) {
			bool fullIce = iceMapCheck(position().xi(), position().yi(), round(radius()));
			bool fullIcePrev = iceMapCheckPrev(position().xi(), position().yi(), round(radius()));
			if(fullIce && !fullIcePrev) {
				makeFrozen(true);
				onWater = false;
			} else if(fullIce && fullIcePrev) {
				onIce = true;
				onWater = false;
			} else {
				onIce = false;
				onWater = true;
			}
		}

	if(!onIce){
		if(isFrozen_)
			isFrozen_ = false;
		else if(onIcePrev )
			awake();
	}

}
