#ifndef __RIGID_BODY_MISSILE_H__
#define __RIGID_BODY_MISSILE_H__

#include "RigidBodyBase.h"
#include "Timers.h"

class RigidBodyMissile : public RigidBodyBase {
public:
	RigidBodyMissile();
	
	void build(const RigidBodyPrm& prm, const Vect3f& boxMin, const Vect3f& boxMax, float mass);
	
	bool evolve(float dt); // возвращает, было ли перемещение

	void setPose(const Se3f& pose);
	void setPosition(const Vect3f& position);
	void setOrientation(const QuatF& orientation) { pose_.rot() = orientation; matrix_.rot() = orientation; }

	const Mat3f& rotation() const { return matrix_.rot(); }

	// Velocity
	const Vect3f& velocity() { return velocity_; }
	void setVelocity(const Vect3f& velocity) { velocity_ = velocity; }
	
	const Vect3f& angularVelocity() const { return angularVelocity_; }
	void setAngularVelocity(const Vect3f& angularVelocity) { angularVelocity_ = angularVelocity; }
	
	// Ground units
	float forwardVelocity() const { return forwardVelocity_; }
	void setForwardVelocity(float v){ forwardVelocity_ = v; }
	bool is_point_reached(const Vect2f& point);
	
	// States
	bool groundColliding() const { return colliding_ & GROUND_COLLIDING; } // сталкивается, стоит на земле
	bool waterColliding() const { return colliding_ & WATER_COLLIDING; } // сталкивается, стоит на воде
	
	bool waterAnalysis() const { return waterAnalysis_; }
		
	float flyingHeight() const { return flyingHeight_; }
	
	// Missiles
	void startMissile(const Vect3f& ownerVelocity, const Vect3f& position, const Vect3f& target); // direction used while keep_direction_time 
	void missileStart() { missile_started = true; }
	
	// Rocket, Debris
	void startRocket(const Vect3f& ownerVelocity, bool directControl);
	bool missileStarted() const { return missile_started; }
	bool directControl() const { return directControl_; }

	// Control
	vector<Vect3f> way_points;
	
	int heightWithWater(int x, int y);

	void show();

private:
	
	// State
	Se3f posePrev_; 
	MatXf matrix_;

	Vect3f velocity_, angularVelocity_;
	Vect3f acceleration, angular_acceleration;

	Vect3f linear_damping;
	float angular_damping;

	bool waterAnalysis_;
		
	DurationTimer suppress_steering_timer;
	DurationTimer steering_timer;
	
	float forwardVelocity_;
	float flyingHeight_;
		
	// Control
	Vect3f target_;
	
	// Missile
	bool missile_started;
	DurationTimer ground_colliding_timer;
	

	bool directControl_;

	void EulerEvolve(float dt);

	void apply_levelling_torque(const Vect3f& current_axis, const Vect3f& target_axis, float torque_factor);
	void rocket_analysis(float dt);
	int checkCollisionPointed(const Vect3f& pos);
	void missile_quant(float dt);
};

#endif // __RIGID_BODY_MISSILE_H__
