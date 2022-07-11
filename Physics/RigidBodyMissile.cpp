#include "stdafx.h"
#include "RigidBodyPrm.h"
#include "RigidBodyMissile.h"
#include "normalMap.h"
#include "PFTrap.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_MISSILE, RigidBodyMissile)

// Digging
float diggingModeDamping = 0.1f;
int diggingModeDelay = 500;

RigidBodyMissile::RigidBodyMissile()
{
	posePrev_ = Se3f::ID;
	onWater = false;
	onLowWater = false;
	onWaterPrev = false;
	onIce = false;
	waterAnalysis_ = false;
	target_ = Vect3f::ZERO;

	setColor(YELLOW);
	rigidBodyType = RIGID_BODY_MISSILE;
	directControl_ = false;
}

void RigidBodyMissile::build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass)
{
	__super::build(prmIn, boxMin, boxMax, mass);

	unmovable_ = prm().unmovable;
	setForwardVelocity(prm().forward_velocity_max);
	flyingHeight_ = prm().undergroundMode ? -20.0f : prm().flying_height;
	velocity_ = angularVelocity_ = Vect3f::ZERO;
	acceleration = angular_acceleration = Vect3f::ZERO;
	linear_damping = prm().linear_damping;
	angular_damping = prm().angular_damping;
	colliding_ = 0;
	missile_started = false;
	waterAnalysis_ = prm().waterAnalysis;
}

void RigidBodyMissile::EulerEvolve(float dt)
{
	posePrev_ = pose();

	Vect3f pos = position();
	pos.scaleAdd(velocity(), dt);
	setPosition(pos);

	QuatF quat = orientation();
	Vect3f wdt;
	wdt.scale(angularVelocity(), 0.5f*dt);
	quat.s() += -quat.x() * wdt.x - quat.y() * wdt.y - quat.z() * wdt.z,
	quat.x() += quat.s() * wdt.x + quat.z() * wdt.y - quat.y() * wdt.z,
	quat.y() += -quat.z() * wdt.x + quat.s() * wdt.y + quat.x() * wdt.z,
	quat.z() += quat.y() * wdt.x - quat.x() * wdt.y + quat.s() * wdt.z;
	quat.normalize();

	// Linear damping anisotropic - apply in local frame
	// Анизатропный дампинг приводит к повороту вектора скорости, при пастрекинге это вредно. Юнит разгоняется.
	orientation().invXform(velocity_);
	velocity_.x *= 1.f - linear_damping.x*dt;
	velocity_.y *= 1.f - linear_damping.y*dt;
	velocity_.z *= 1.f - linear_damping.z*dt;
	orientation().xform(velocity_);

	acceleration.z -= prm().gravity;
	velocity_.scaleAdd(acceleration, dt);

	// Apply isotropic angular damping and acceleration
	angularVelocity_.scale(1.f - angular_damping*dt);
	angularVelocity_.scaleAdd(angular_acceleration, dt);

	acceleration.set(0, 0, 0);
	angular_acceleration.set(0, 0, 0);

	// Restore dampings wich can be changed by logic
	linear_damping = prm().linear_damping;
	angular_damping = prm().angular_damping;

	setOrientation(quat);
}

void RigidBodyMissile::apply_levelling_torque(const Vect3f& current_axis, const Vect3f& target_axis, float torque_factor)
{
	// 1. Axes should be in global frame
	// 2. Parameter orientation_torque_factor is used
	Vect3f cross = current_axis % target_axis;
	float len = cross.norm();
	if(len > 0.0001f)
		angular_acceleration.scaleAdd(cross, Acos(dot(current_axis, target_axis)/(current_axis.norm()*target_axis.norm() + 1e-5))*torque_factor/len);
}

bool RigidBodyMissile::evolve(float dt)
{
	__super::evolve(dt);

	switch(prm().unit_type){
	case RigidBodyPrm::MISSILE:
		EulerEvolve(dt);
		missile_quant(dt);
		break;
	case RigidBodyPrm::ROCKET:
		EulerEvolve(dt);
		rocket_analysis(dt);
		break;
	}

	return true;
}

void RigidBodyMissile::show()
{
	__super::show();

	if(showDebugRigidBody.acceleration){
		show_vector(position(), acceleration*showDebugRigidBody.linearScale, GREEN);
		show_vector(position(), angular_acceleration*showDebugRigidBody.angularScale, BLUE);
	}

	if(showDebugRigidBody.wayPoints && !way_points.empty()){
		Vect3f pos = To3D(position());
		vector<Vect3f>::iterator i;
		FOR_EACH(way_points, i){
			Vect3f p=To3D(*i);
			show_line(pos, p, YELLOW);
			show_vector(p, 1, YELLOW);
			pos = p;
			}
		show_vector(target_, 3, YELLOW);
		}

	if(showDebugRigidBody.movingDirection){
		show_vector(position(), velocity()*0.1f, MAGENTA);
		}

	if(showDebugRigidBody.localFrame){
		show_vector(position(), rotation().xcol(), X_COLOR);
		show_vector(position(), rotation().ycol(), Y_COLOR);
		show_vector(position(), rotation().zcol(), Z_COLOR);
		}

	if(showDebugRigidBody.velocityValue){
		XBuffer buf;
		buf.SetDigits(2);
		buf <= velocity().norm();
		show_text(position(), buf, MAGENTA);
	}

	if(showDebugRigidBody.average_movement){
		XBuffer buf;
		buf.SetDigits(2);
		float movement = averagePosition_.distance2(position()) + averageOrientation_.distance2(Vect3f(orientation()[0], orientation()[1], orientation()[2]))*100;
		buf <= movement;
		show_text(position(), buf, YELLOW);
	}

	if(showDebugRigidBody.velocity){
		show_vector(position(), velocity()*showDebugRigidBody.linearScale, GREEN);
		show_vector(position(), angularVelocity()*showDebugRigidBody.angularScale, BLUE);
	}

	if(showDebugRigidBody.ground_colliding){
		XBuffer buf;
		buf < "colliding: " <= colliding();
		show_text(position(), buf, YELLOW);
	}
	
	if(showDebugRigidBody.onLowWater){
		XBuffer buf;
		buf < "On: " < (onWater ? "water " : "") < (onLowWater ? "lowWater " : "") < (onIce ? "ice " : "");
		show_text(position(), buf, YELLOW);
	}
	
	if(showDebugRigidBody.target)
		show_vector(target_, 1, RED);

}

bool RigidBodyMissile::is_point_reached(const Vect2f& point)
{ 
	
	Vect2f p0 = posePrev_.trans();
	Vect2f p1 = position();
	Vect2f axis = p1 - p0;
	float norm2 = axis.norm2();
	float dist2 = 0;
	if(norm2 < 0.1){
		dist2 = p0.distance2(point);
	}
	else{
		Vect2f dir = point - p0;
		float dotDir = dot(axis, dir);
		if(dotDir < 0)
			dist2 = p0.distance2(point);
		else if(dot(axis, point - p1) > 0)
			dist2 = p1.distance2(point);
		else
			dist2 = (dir - axis*(dotDir/norm2)).norm2();
	}

	return dist2 < sqr(radius() + prm().is_point_reached_radius_max);
}

int RigidBodyMissile::checkCollisionPointed(const Vect3f& pos)
{  
	int x = pos.xi();
	int y = pos.yi();
	int z = pos.zi();
	int zt = vMap.GVBuf[vMap.offsetGBufC(x >> kmGrid, y >> kmGrid)];
	int result = z < zt ? GROUND_COLLIDING : 0;
	if(pathFinder->checkWater(x, y, 0) && environment->water()->GetZFast(x, y) > z){
		onWater = true;
		if(waterAnalysis())
			result |= environment->temperature() && environment->temperature()->isOnIce(position()) ? GROUND_COLLIDING : WATER_COLLIDING;
	}
	return result;
}

int RigidBodyMissile::heightWithWater(int x, int y)
{
	if(waterAnalysis())
		return max(vMap.GVBuf[vMap.offsetGBufC(x >> kmGrid, y >> kmGrid)], 
			round(environment->water()->GetZ(x, y)));
	else
		return vMap.GVBuf[vMap.offsetGBufC(x >> kmGrid, y >> kmGrid)];
}

void RigidBodyMissile::setPosition(const Vect3f& position)
{
	Vect3f positionNew = position;
	
	float r(radius() + 2.0f);
	if(positionNew.x < r){
		positionNew.x = r;
		colliding_ |= WORLD_BORDER_COLLIDING;
	}else{
		float maxx(vMap.H_SIZE - 1 - r);
		if(positionNew.x > maxx){
			positionNew.x = maxx;
			colliding_ |= WORLD_BORDER_COLLIDING;
		}
	}
	if(positionNew.y < r){
		positionNew.y = r;
		colliding_ |= WORLD_BORDER_COLLIDING;
	}else{
		float maxy(vMap.V_SIZE - 1 - r);
		if(positionNew.y > maxy){
			positionNew.y = maxy;
			colliding_ |= WORLD_BORDER_COLLIDING;
		}
	}

	pose_.trans() = positionNew;
	matrix_.trans() = positionNew;
}

void RigidBodyMissile::setPose(const Se3f& pose)
{
	Se3f poseNew = pose;
	float r(radius() + 2.0f);
	poseNew.trans().x = clamp(pose.trans().x, r, vMap.H_SIZE - 1 - r);
	poseNew.trans().y = clamp(pose.trans().y, r, vMap.V_SIZE - 1 - r);
	pose_ = poseNew;
	matrix_.set(poseNew);
}

void RigidBodyMissile::startMissile(const Vect3f& ownerVelocity, const Vect3f& position, const Vect3f& target)
{
	setPosition(position);
	target_ = target;
	target_.z += prm().targetCorrectionZ;
				 
	Vect3f r = target_ - position;
	float psi = r.psi() - M_PI/2;
	float theta = prm().calcTurnTheta(sqrt(sqr(r.x) + sqr(r.y)), r.z, forwardVelocity());

	setOrientation(Mat3f(psi, Z_AXIS)*Mat3f(theta, X_AXIS));

	velocity_.scale(rotation().ycol(), forwardVelocity());

	velocity_ += ownerVelocity;

	posePrev_ = pose();

	missile_started = true;
	ground_colliding_timer.start(prm().ground_colliding_delay);
}

void RigidBodyMissile::missile_quant(float dt)
{
	if(!ground_colliding_timer){
		if((colliding_ = checkCollisionPointed(position()) | (colliding_ & WORLD_BORDER_COLLIDING)) != 0){
			for(int i = 0; i < 8; i++){
				Vect3f mid = (position() + posePrev_.trans())/2;
				if(checkCollisionPointed(mid))
					pose_.trans() = mid;
				else
					posePrev_.trans() = mid;
			}
		}
	}
	
	apply_levelling_torque(rotation().ycol(), velocity(), prm().orientation_torque_factor);
}

void RigidBodyMissile::startRocket(const Vect3f& ownerVelocity, bool directControl)
{
	directControl_ = directControl;

	velocity_ = ownerVelocity;

	suppress_steering_timer.start(300);
	steering_timer.start(prm().steering_duration);
}

void RigidBodyMissile::rocket_analysis(float dt)
{
	colliding_ = checkCollisionPointed(position()) | (colliding_ & WORLD_BORDER_COLLIDING);

	Vect3f y_axis;
	if(suppress_steering_timer()){
		y_axis = rotation().ycol();
		setColor(BLUE);
		colliding_ = 0;
	}else if(steering_timer() && !way_points.empty()){
		target_ = way_points.front();
		y_axis = target_ - position();
		if(prm().undergroundMode || prm().underwaterRocket){
			y_axis.z = steering_timer() < 400 ? -flyingHeight_ : 0.0f;
		}else if(!directControl_){
			Vect2f direction = y_axis;
			Vect2f delta = direction;
			delta *= prm().rocket_forward_analysis_distance / (delta.norm() + 0.01f);
			Vect2f forward = delta + position();
			int z_max_forward = heightWithWater(forward.xi(), forward.yi());
			z_max_forward += flyingHeight_;
			if(direction.norm2() < sqr(prm().rocket_vertical_control_distance) || z_max_forward < target_.z || steering_timer() < 500)
				y_axis.z += prm().rocket_target_offset_z;
			else
				y_axis.set(delta.x, delta.y, z_max_forward - position().z);
		}
		if(showDebugRigidBody.rocketTarget)
			show_line(position(), position() + y_axis, RED);
		float norm = y_axis.norm2();
		if(norm > 0.5f)
			y_axis /= sqrtf(norm);
		else
			y_axis = rotation().ycol();
	}else
		y_axis = rotation().ycol();

	apply_levelling_torque(rotation().ycol(), y_axis, prm().orientation_torque_factor);
	if(velocity().norm() < forwardVelocity())
		acceleration.scaleAdd(y_axis, prm().forward_acceleration); 

	if(prm().undergroundMode){
		unsigned short p = vMap.GABuf[vMap.offsetGBufWorldC(position().xi(), position().yi())];
		if(p & GRIDAT_INDESTRUCTABILITY) {
			linear_damping.x += diggingModeDamping;
			linear_damping.y += diggingModeDamping;
			linear_damping.z += diggingModeDamping;
		}
		flyingHeight_ += velocity().z * dt;
		pose_.trans().z = heightWithWater(position().xi(), position().yi()) + flyingHeight_; 
	}else if(prm().underwaterRocket){
		pose_.trans().z = heightWithWater(position().xi(), position().yi()) - prm().digging_depth;
	}else if(colliding_ != 0){
		for(int i = 0; i < 8; i++){
			Vect3f mid = (position() + posePrev_.trans())/2;
			if(checkCollisionPointed(mid))
				pose_.trans() = mid;
			else
				posePrev_.trans() = mid;
		}
		acceleration = Vect3f::ZERO;
	}
}

