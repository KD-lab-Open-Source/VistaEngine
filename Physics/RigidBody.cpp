#include "stdafx.h"
#include "RigidBodyPrm.h"
#include "RigidBodyBase.h"

REGISTER_CLASS(RigidBodyPrm, RigidBodyPrm, "Базовая физика");

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(RigidBodyPrm, RigidBodyTypeOld, "RigidBodyType")
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, UNIT, "UNIT");
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, MISSILE, "MISSILE");
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, ROCKET, "ROCKET");
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, DEBRIS, "DEBRIS");
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, NOT_EVOLUTION, "NOT_EVOLUTION");
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, ENVIRONMENT, "ENVIRONMENT");
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, ITEM, "ITEM");
END_ENUM_DESCRIPTOR_ENCLOSED(RigidBodyPrm, RigidBodyTypeOld)

BEGIN_ENUM_DESCRIPTOR(RigidBodyType, "RBType")
REGISTER_ENUM(RIGID_BODY_BASE, "RIGID_BODY_BASE");
REGISTER_ENUM(RIGID_BODY_ENVIRONMENT, "RIGID_BODY_ENVIRONMENT");
REGISTER_ENUM(RIGID_BODY_UNIT, "RIGID_BODY_UNIT");
REGISTER_ENUM(RIGID_BODY_MISSILE, "RIGID_BODY_MISSILE");
REGISTER_ENUM(RIGID_BODY_BOX, "RIGID_BODY_BOX");
REGISTER_ENUM(RIGID_BODY_TRAIL, "RIGID_BODY_TRAIL");
REGISTER_ENUM(RIGID_BODY_UNIT_RAG_DOLL, "RIGID_BODY_UNIT_RAG_DOLL");
REGISTER_ENUM(RIGID_BODY_NODE, "RIGID_BODY_NODE");
END_ENUM_DESCRIPTOR(RigidBodyType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(RigidBodyPrm, PassabilityFlags, "PassabilityFlags")
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, IMPASSABILITY, "непроходимая")
REGISTER_ENUM_ENCLOSED(RigidBodyPrm, PASSABILITY, "проходимая")
END_ENUM_DESCRIPTOR_ENCLOSED(RigidBodyPrm, PassabilityFlags)

RigidBodyPrm::Oscillator::Oscillator()
{
	phase = 0;
	amplitude = 10;
	omega = 6;
	amplitude_decrement = 0.01f;
	omega_increment = 0.01f;
	omega_disperse = 0.05f;
	set();
}
 
void RigidBodyPrm::Oscillator::set()
{
	phase = logicRNDfrnd(1.f)*M_PI; 
	omegaDisperced = logicRNDfrnd(1.f)*omega*omega_disperse; 
}

float RigidBodyPrm::Oscillator::operator() (float dt, float velocity)
{ 
	return amplitude*sin(phase += omegaDisperced*(1 + omega_increment*velocity)*dt)/(1 + amplitude_decrement*velocity); 
}

void RigidBodyPrm::Oscillator::serialize(Archive& ar) {
	ar.serialize(amplitude, "amplitude", "amplitude");
	ar.serialize(omega, "omega", "omega");
	ar.serialize(amplitude_decrement, "amplitude_decrement", "amplitude_decrement");
	ar.serialize(omega_increment, "omega_increment", "omega_increment");
	ar.serialize(omega_disperse, "omega_disperse", "omega_disperse");
	if(ar.isInput())
		set();
}

RigidBodyPrm::Stiffness::Stiffness() 
{
	stiffness_up = 10;
	stiffness_down = 10;
	dz_max_up = 15;
	dz_max_down = 15;
}

void RigidBodyPrm::Stiffness::serialize(Archive& ar) 
{
	ar.serialize(stiffness_up, "stiffness_up", "stiffness_up");
	ar.serialize(stiffness_down, "stiffness_down", "stiffness_down");
	ar.serialize(dz_max_up, "dz_max_up", "dz_max_up");
	ar.serialize(dz_max_down, "dz_max_down", "dz_max_down");
}

float RigidBodyPrm::Stiffness::force(float dz) const 
{ 
	if(dz > 0)
	{
		if(dz > dz_max_up)
			dz = dz_max_up;
		return dz*stiffness_up;
	}
	else
	{
		if(dz < -dz_max_down)
			dz = -dz_max_down;
		return dz*stiffness_down;
	}
}

RigidBodyPrm::RigidBodyPrm() 
{
	unit_type = UNIT; 
	rigidBodyType = RIGID_BODY_UNIT; 

	linear_damping.set(6, 0.2f, 5);
	angular_damping = 6;

	unmovable = false; 
	isotropic = false;
	controled_by_points = true;

	brake_damping = 6;//3.5;
	point_control_slow_distance = 20;
	point_control_slow_factor = 0.1f;

	rudder_speed = 1.5;
	steering_duration = 1000;
	steering_linear_velocity_min = 20;
	steering_acceleration_max = 15;
	is_point_reached_radius_max = 4; // Max radius for is_point_reached

	forward_velocity_max = 20;
	forward_acceleration = 40;

	orientation_torque_factor = 9; 

	speed_map_factor = 0.025f; // Maps average_velocity -> [0..1]

	analyze_points_density = .4f;
	Dxy_minimal = 6;
	zero_velocity_z = 5;
	box_delta_y = 4;
	dz_max_avr_factor = 0;
	canEnablePointAnalyze = true;

	digging_depth = 150;
	digging_depth_velocity = 10;
	undergroundMode = false;

	flying_down_without_way_points = false;
	flying_height_relative = true;
	flying_height = 50;
	flying_vertical_direction_x_factor = 0.30f;
	flying_vertical_direction_y_factor = 0;//0.01;
	flyingMode = false;

	minimize_theta = 0;
	upper_theta = 90; // degrees
	lower_theta = -45; // degrees
	targetCorrectionZ = 0;
	distance_correction_factor = 0.96f; // из-за ошибки интегрирования снаряды чуть-чуть перелетают.
	ground_colliding_delay = 100;
	ground_collision_enabled = false;

	rocket_vertical_control_distance = 120;
	rocket_forward_analysis_distance = 80;
	rocket_target_offset_z = 0;
	underwaterRocket = 0;

	debris_angular_velocity = 3.14f;

	enable_show = true;

	moveVertical = false;
	
	waterPass = IMPASSABILITY;
	impassabilityPass = IMPASSABILITY;
	groundPass = PASSABILITY;

	waterAnalysis = false;

	path_tracking_back = false;

	sleepedAtStart = false;
	awakeCounter = 4;
	average_movement_tau = 0.1f;
	fcModeAlways = false;
	fcBodyScaner = true;
	fcBodyScanerDist = 0;

	tree = false;
	fence = false;
	fallAngularVelocity = 1;
	verticalFactor = 1.4f;

	position_tau = 0.2f;
	orientation_tau = 0.2f;
	additionalHorizontalRot = 0.0f;
	additionalForvardRot = 0.0f;

	ptObstaclesCheck = true;
	ptImpassabilityCheck = true;

	///////////////////////////////////////////

	sleepAtStart = true;
	enableSleeping = true;
	awakeCounter = 4;
	sleepMovemenThreshould = 0.01f;

	sphereCollision = false;

	holdOrientation = false;

	linearDamping = 0;
	angularDamping = 0;

	gravity = 0;
	TOI_factor = 1.0f;
	friction = 1.0f;
	restitution = 0.2f;
	relaxationTime = 0.1f;

	joint = false;
	axes = Mat3f::ZERO;
	upperLimits = Vect3f::ZERO; 
	lowerLimits = Vect3f::ZERO;
}

void RigidBodyPrm::serialize(Archive& ar) 
{
	ar.serialize(unit_type, "unit_type", "unit_type");
	ar.serialize(rigidBodyType, "rigidBodyType", "rigidBodyType");
	
	ar.openBlock("Общие параметры", "Общие параметры");
	ar.serialize(linear_damping, "linear_damping", "linear_damping");
	ar.serialize(angular_damping, "angular_damping", "angular_damping");
	
	ar.serialize(fcModeAlways, "fcModeAlways", "fcModeAlways");
	ar.serialize(fcBodyScaner, "fcBodyScaner", "fcBodyScaner");
	ar.serialize(fcBodyScanerDist, "fcBodyScanerDist", "fcBodyScanerDist");
	ar.closeBlock();

	ar.openBlock("", "Управление");
	ar.serialize(unmovable, "unmovable", "unmovable");
	ar.serialize(isotropic, "isotropic", "isotropic");
	ar.serialize(controled_by_points, "controled_by_points", "controled_by_points");

	ar.serialize(brake_damping, "brake_damping", "brake_damping");
	ar.serialize(point_control_slow_distance, "point_control_slow_distance", "point_control_slow_distance");
	ar.serialize(point_control_slow_factor, "point_control_slow_factor", "point_control_slow_factor");

	ar.serialize(rudder_speed, "rudder_speed", "rudder_speed");
	ar.serialize(steering_duration, "steering_duration", "steering_duration");
	ar.serialize(steering_linear_velocity_min, "steering_linear_velocity_min", "steering_linear_velocity_min");
	ar.serialize(steering_acceleration_max, "steering_acceleration_max", "steering_acceleration_max");
	ar.serialize(is_point_reached_radius_max, "is_point_reached_radius_max", "is_point_reached_radius_max");

	ar.serialize(forward_velocity_max, "forward_velocity_max", "forward_velocity_max");
	ar.serialize(forward_acceleration, "forward_acceleration", "forward_acceleration");
	ar.serialize(orientation_torque_factor, "orientation_torque_factor", "orientation_torque_factor");
	ar.serialize(moveVertical, "moveVertical", "moveVertical");
	ar.serialize(verticalFactor, "verticalFactor", "verticalFactor");

	ar.serialize(speed_map_factor, "speed_map_factor", "speed_map_factor");

	ar.serialize(analyze_points_density, "analyze_points_density", "analyze_points_density");
	ar.serialize(Dxy_minimal, "Dxy_minimal", "Dxy_minimal");
	ar.serialize(zero_velocity_z, "zero_velocity_z", "zero_velocity_z");
	ar.serialize(box_delta_y, "box_delta_y", "box_delta_y");
	ar.serialize(dz_max_avr_factor, "dz_max_avr_factor", "dz_max_avr_factor");
	ar.serialize(canEnablePointAnalyze, "canEnablePointAnalyze", "canEnablePointAnalyze");

	ar.serialize(position_tau, "position_tau", "position_tau");
	ar.serialize(orientation_tau, "orientation_tau", "orientation_tau");
	ar.serialize(additionalHorizontalRot, "additionalHorizontalRot", "additionalHorizontalRot");
	ar.serialize(additionalForvardRot, "additionalForvardRot", "additionalForvardRot");

	ar.closeBlock();

	ar.openBlock("", "Подземные");
	ar.serialize(digging_depth, "digging_depth", "digging_depth");
	ar.serialize(digging_depth_velocity, "digging_depth_velocity", "digging_depth_velocity");
	ar.serialize(undergroundMode, "undergroundMode", "underground mode");
	ar.closeBlock();

	ar.openBlock("", "Летные");
	ar.serialize(flyingMode, "flyingMode", "Летный тип");
	ar.serialize(flying_down_without_way_points, "flying_down_without_way_points", "flying_down_without_way_points");
	ar.serialize(flying_height_relative, "flying_height_relative", "flying_height_relative");
	ar.serialize(flying_height, "flying_height", "flying_height");
	ar.serialize(flying_stiffness, "flying_stiffness", "flying_stiffness");
	ar.serialize(flying_oscillator_z, "flying_oscillator_z", "flying_oscillator_z");
	ar.serialize(flying_vertical_direction_x_factor, "flying_vertical_direction_x_factor", "flying_vertical_direction_x_factor");
	ar.serialize(flying_vertical_direction_y_factor, "flying_vertical_direction_y_factor", "flying_vertical_direction_y_factor");
	ar.closeBlock();

	ar.openBlock("", "Снаряды");
	ar.serialize(minimize_theta, "minimize_theta", "minimize_theta");
	ar.serialize(upper_theta, "upper_theta", "upper_theta");
	ar.serialize(lower_theta, "lower_theta", "lower_theta");
	ar.serialize(targetCorrectionZ, "targetCorrectionZ", "targetCorrectionZ");
	ar.serialize(distance_correction_factor, "distance_correction_factor", "distance_correction_factor");
	ar.serialize(ground_colliding_delay, "ground_colliding_delay", "ground_colliding_delay");
	ar.serialize(ground_collision_enabled, "ground_collision_enabled", "ground_collision_enabled");
	ar.closeBlock();

	ar.openBlock("", "Ракеты");
	ar.serialize(rocket_vertical_control_distance, "rocket_vertical_control_distance", "rocket_vertical_control_distance");
	ar.serialize(rocket_forward_analysis_distance, "rocket_forward_analysis_distance", "rocket_forward_analysis_distance");
	ar.serialize(rocket_target_offset_z, "rocket_target_offset_z", "rocket_target_offset_z");
	ar.serialize(underwaterRocket, "underwaterRocket", "underwaterRocket");
	ar.closeBlock();

	ar.openBlock("", "Осколки");
	ar.serialize(debris_angular_velocity, "debris_angular_velocity", "debris_angular_velocity");
	ar.closeBlock();

	ar.openBlock("", "Падающие деревья и заборы");
	ar.serialize(tree, "tree", "falling");
	ar.serialize(fence, "fence", "fence");
	ar.serialize(fallAngularVelocity, "fallAngularVelocity", "fallAngularVelocity");
	ar.closeBlock();
    
	ar.openBlock("", "Поиск пути");
	ar.serialize(waterPass, "waterPass", "WaterPass");
	ar.serialize(groundPass, "groundPass", "GroundPass");
	ar.serialize(impassabilityPass, "impassabilityPass", "ImpassabilityPass");
	ar.serialize(waterAnalysis, "waterAnalysis", "waterAnalysis");
	ar.serialize(path_tracking_back, "path_tracking_back", "path_tracking_move_back");
	ar.serialize(ptObstaclesCheck, "ptObstaclesCheck", "ObstaclesCheck");
	ar.serialize(ptImpassabilityCheck, "ptImpassabilityCheck", "ImpassabilityCheck");
	ar.closeBlock();

	ar.openBlock("", "Отладка");
	ar.serialize(enable_show, "enable_show", "enable_show");
	ar.closeBlock();

	ar.openBlock("", "RigidBodyPrmBase");
	ar.serialize(sleepAtStart, "sleepAtStart", "sleepAtStart");
	ar.serialize(enableSleeping, "enableSleeping", "enableSleeping");
	ar.serialize(awakeCounter, "awakeCounter", "awakeCounter");
	ar.serialize(sleepMovemenThreshould, "sleepMovemenThreshould", "sleepMovemenThreshould");
	ar.serialize(average_movement_tau, "average_movement_tau", "average_movement_tau");
	ar.serialize(sphereCollision, "sphereCollision", "sphereCollision");
	ar.closeBlock();

	ar.openBlock("", "RigidBodyPrmEnvironment");
	ar.serialize(holdOrientation, "holdOrientation", "holdOrientation");
	ar.closeBlock();

	ar.openBlock("", "RigidBodyPrmBox");
	ar.serialize(linearDamping, "linearDamping", "linearDamping");
	ar.serialize(angularDamping, "angularDamping", "angularDamping");
	ar.serialize(gravity, "gravity", "gravity");
	ar.serialize(TOI_factor, "TOI_factor", "TOI_factor");
	ar.serialize(friction, "friction", "friction");
	ar.serialize(restitution, "restitution", "restitution");
	ar.serialize(relaxationTime, "relaxationTime", "relaxationTime");
	ar.serialize(joint, "joint", "joint");
	ar.serialize(axes, "axes", "axes");
	ar.serialize(upperLimits, "upperLimits", "upperLimits");
	ar.serialize(lowerLimits, "lowerLimits", "lowerLimits");
	ar.closeBlock();

}

string RigidBodyPrm::name() const
{
	RigidBodyPrmReference ref(this);
	return ref.c_str();
}

float RigidBodyPrm::calcTurnTheta(float x, float z, float velocity) const 
{
	if(unit_type == ROCKET)
		return atan2(z, x);

	x += FLT_EPS;
	z += FLT_EPS;
	x *= distance_correction_factor;
	float v = velocity;
	float t30 = -2.0*z;
	float t16 = x*x;
	float t11 = z*z+t16;
	float t29 = -2.0*t11;
	float t28 = 1.0/t11/2.0;
	float t13 = v*v;
	float t27 = z*gravity-t13;
	float t15 = 1.0/v;
	float t26 = t11*t15;
	float t25 = t28*t30;
	float t9 = t13*t13+(t13*t30-t16*gravity)*gravity;
	if(t9 < 0) {
		//xassert_s(forward_velocity_max < FLT_EPS && "1. Can't reach the target. Increase forward_velocity_max for ", name);
		return M_PI/4;
	}
	t9 = sqrt(t9);
	float t8 = t9+t27;
	float t17 = t8*t29;
	if(t17 < FLT_EPS) {
		//xassert_s(forward_velocity_max < FLT_EPS && "2. Can't reach the target. Increase forward_velocity_max for ", name);
		return M_PI/4;
	}
	t17 = sqrt(t17);
	float t24 = (t8*t25+gravity)/t17*t26;
	float t7 = -t9+t27;
	float t18 = t7*t29;
	if(t18 < FLT_EPS) {
		//xassert_s(forward_velocity_max < FLT_EPS && "3. Can't reach the target. Increase forward_velocity_max for ", name);
		return M_PI/4;
	}
	t18 = sqrt(t18);
	float t23 = (t7*t25+gravity)/t18*t26;
	float t22 = t15*x*t28;
	float t21 = t18*t22;
	float t20 = t17*t22;
	float r[4];
	r[0] = atan2(t23,t21);
	r[1] = atan2(-t23,-t21);
	r[2] = atan2(t24,t20);
	r[3] = atan2(-t24,-t20);
	float lower = G2R(lower_theta);
	float upper = G2R(upper_theta);
	float theta = r[0] > upper ? upper : (r[0] < lower ? lower : r[0]);
	if(minimize_theta){
		for(int i = 1; i < 4; i++)
			if(fabs(theta) > fabs(r[i]) && r[i] < upper && r[i] > lower)
				theta = r[i];
		}
	else
		for(int i = 1; i < 4; i++)
			if(fabs(theta) < fabs(r[i]) && r[i] < upper && r[i] > lower)
				theta = r[i];
	return theta;
}
