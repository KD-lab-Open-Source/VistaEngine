#ifndef __RIGID_BODY_MISSILE_H__
#define __RIGID_BODY_MISSILE_H__

#include "RigidBodyBase.h"
#include "Timers.h"

///////////////////////////////////////////////////////////////
// RigidBodyMissile
///////////////////////////////////////////////////////////////

class RigidBodyMissile : public RigidBodyBase
{
public:
	RigidBodyMissile();
	
	void build(const RigidBodyPrm& prm, const Vect3f& center, const Vect3f& extent, float mass);
	
	bool evolve(float dt); // возвращает, было ли перемещение

	void setPose(const Se3f& pose);
	
	const Se3f& posePrev() const { return posePrev_; }
	const Vect3f& direction() const { return direction_; }

	// Velocity
	const Vect3f& velocity() { return velocity_; }
	void setVelocity(const Vect3f& velocity) { velocity_ = velocity; }
	
	const Vect3f& angularVelocity() const { return angularVelocity_; }
	void setAngularVelocity(const Vect3f& angularVelocity) { angularVelocity_ = angularVelocity; }
	
	// Ground units
	float forwardVelocity() const { return forwardVelocity_; }
	void setForwardVelocity(float v){ forwardVelocity_ = v; }
	bool is_point_reached(const Vect2f& point);
	
	bool waterAnalysis() const { return waterAnalysis_; }
		
	float flyingHeight() const { return flyingHeight_; }
	
	// Missiles
	void startMissile(const Vect3f& ownerVelocity, const Vect3f& position, const Vect3f& target); // direction used while keep_direction_time 
	void missileStart() { missile_started = true; }
	
	// Rocket, Debris
	void startRocket(const Vect3f& ownerVelocity, bool directControl);
	bool missileStarted() const { return missile_started; }
	bool directControl() const { return directControl_; }

	void clearTarget() { targetSet_ = false; } 
	void setTarget(const Vect3f& targetPosition) { target_ = targetPosition; targetSet_ = true; }

	void show();

private:
	void EulerEvolve(float dt);
	void apply_levelling_torque(const Vect3f& current_axis, const Vect3f& target_axis, float torque_factor);
	void rocket_analysis(float dt);
	int checkCollisionPointed(const Vect3f& pos);
	void missile_quant(float dt);
	void setPosition(const Vect3f& position);
	void setOrientation(const QuatF& orientation) 
	{ 
		__super::setOrientation(orientation); 
		direction_.set(2.f * (orientation.x() * orientation.y() - orientation.z() * orientation.s()),
			2.f * (sqr(orientation.s()) + sqr(orientation.y()) - 0.5f),
			2.f * (orientation.y() * orientation.z() + orientation.x() * orientation.s()));
	}
	int heightWithWater(int x, int y);

	// State
	Se3f posePrev_; 
	Vect3f direction_;

	Vect3f velocity_, angularVelocity_;
	Vect3f acceleration, angular_acceleration;

	Vect3f linear_damping;
	float angular_damping;

	bool waterAnalysis_;
		
	LogicTimer suppress_steering_timer;
	LogicTimer steering_timer;
	
	float forwardVelocity_;
	float flyingHeight_;
		
	// Control
	Vect3f target_;
	
	// Missile
	bool missile_started;
	LogicTimer ground_colliding_timer;
	
	// Control
	bool targetSet_;

	bool directControl_;
};

#endif // __RIGID_BODY_MISSILE_H__
