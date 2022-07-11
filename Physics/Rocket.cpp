#include "stdafx.h"
#include "terra.h"
#include "RigidBody.h"
#include "..\Environment\Environment.h"

void RigidBody::startRocket(const RigidBody* owner)
{
	if(owner)
		velocity_ = owner->velocity();

	if(prm().undergroundMode){
		setDiggingMode(true);
		flyingHeight_ = deltaZ_ = -prm().digging_depth;
	}

	set_debug_color(WHITE);
	suppress_steering_timer.start(prm().suppress_steering_duration);
}

void RigidBody::rocket_analysis(float dt)
{
	set_debug_color(GREEN);

	colliding_ = checkCollisionPointed(position());

	int z_max = heightWithWater(position().xi(), position().yi());
	
	Vect2f delta = !way_points.empty() ? way_points.front() - position() : rotation().ycol();
	delta *= prm().rocket_forward_analysis_distance/(delta.norm() + 0.01);
	Vect2f forward = delta + position();
	int z_max_forward = heightWithWater(forward.xi(), forward.yi());
	z_max_forward += flyingHeight();

	Vect3f y_axis;
	if(!way_points.empty()){
		target = way_points.front();
		Vect2f direction = target - position();
		if(direction.norm2() < sqr(prm().rocket_vertical_control_distance) || z_max_forward < target.z){
			delta = direction;
			z_max_forward = target.z + prm().rocket_target_offset_z;
		} 
		y_axis = Vect3f(delta.x, delta.y, z_max_forward - position().z);
		if(showDebugRigidBody.rocketTarget)
			show_line(position(), position() + y_axis, RED);
		y_axis.normalize();					    
	}
	else
		y_axis = rotation().ycol();

	if(suppress_steering_timer()){
		y_axis = rotation().ycol();
		set_debug_color(BLUE);
		colliding_ = 0;
	}

	apply_levelling_torque(rotation().ycol(), y_axis, prm().orientation_torque_factor);
	if(velocity().norm() < forwardVelocity())
		acceleration.scaleAdd(y_axis, prm().forward_acceleration); 

	applyDiggingForce();

	if(prm().undergroundMode)
		pose_.trans().z = z_max - 20; 
	else if(prm().underwaterRocket){
		pose_.trans().z = z_max - prm().digging_depth;
	}
	else if(colliding_ != 0){
		for(int i = 0; i < 8; i++){
			Vect3f mid = (position() + posePrev_.trans())/2;
			if(checkCollisionPointed(mid))
				pose_.trans() = mid;
			else
				posePrev_.trans() = mid;
		}
		velocity_ = Vect3f::ZERO;
		acceleration = Vect3f::ZERO;
	}
}

