#include "stdafx.h"
#include "RigidBodyBase.h"
#include "Serialization\Serialization.h"
#include "Serialization\SerializationFactory.h"
#include "Serialization\EnumDescriptor.h"
#include "Serialization\RangedWrapper.h"

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
REGISTER_ENUM(RIGID_BODY_UNIT_RAG_DOLL, "RIGID_BODY_UNIT_RAG_DOLL");
END_ENUM_DESCRIPTOR(RigidBodyType)

RigidBodyPrm::RigidBodyPrm() 
{
	unit_type = UNIT; 
	rigidBodyType = RIGID_BODY_UNIT; 

	linear_damping.set(6, 0.2f, 5);
	angular_damping = 6;

	unmovable = false; 
	controled_by_points = true;

	steering_duration = 1000;
	is_point_reached_radius_max = 4; // Max radius for is_point_reached

	forward_velocity_max = 20;
	forward_acceleration = 40;

	orientation_torque_factor = 9; 

	canEnablePointAnalyze = true;
	placeOnWaterSurface = false;

	digging_depth = 150;
	digging_depth_velocity = 10;
	undergroundMode = false;

	hoverMode = false;
	flying_height_relative = true;
	flying_height = 50;
	flyingMode = false;
	alwaysMoving = false;

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

	moveVertical = false;
	
	waterPass = false;
	fieldPass = false;
	groundPass = true;

	waterAnalysis = false;

	path_tracking_back = false;

	awakeCounter = 4;
	average_movement_tau = 0.1f;
	
	fence = false;
	verticalFactor = 1.0f;

	position_tau = 0.2f;
	orientation_tau = 1.0f;
	additionalHorizontalRot = 0.0f;
	additionalForvardRot = 0.0f;

	ptObstaclesCheck = true;

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
	suspensionSpringConstant = 1.1f;
	suspensionDamping = 3.0f;
	drivingForceLimit = 2.0f;
	wheelMass = 0.1f;
}

void RigidBodyPrm::serialize(Archive& ar) 
{
	ar.serialize(unit_type, "unit_type", "unit_type");
	ar.serialize(rigidBodyType, "rigidBodyType", "rigidBodyType");
	
	if(ar.openBlock("Общие параметры", "Общие параметры")){
		ar.serialize(linear_damping, "linear_damping", "linear_damping");
		ar.serialize(angular_damping, "angular_damping", "angular_damping");
		
		ar.closeBlock();
	}

	if(ar.openBlock("", "Управление")){
		ar.serialize(unmovable, "unmovable", "unmovable");
		ar.serialize(controled_by_points, "controled_by_points", "controled_by_points");

		ar.serialize(steering_duration, "steering_duration", "steering_duration");
		ar.serialize(is_point_reached_radius_max, "is_point_reached_radius_max", "is_point_reached_radius_max");

		ar.serialize(forward_velocity_max, "forward_velocity_max", "forward_velocity_max");
		ar.serialize(forward_acceleration, "forward_acceleration", "forward_acceleration");
		ar.serialize(orientation_torque_factor, "orientation_torque_factor", "orientation_torque_factor");
		ar.serialize(moveVertical, "moveVertical", "moveVertical");
		ar.serialize(RangedWrapperf(verticalFactor, 0.0f, 1.0f, 0.01f), "verticalFactor", "verticalFactor");

		ar.serialize(canEnablePointAnalyze, "canEnablePointAnalyze", "canEnablePointAnalyze");
		ar.serialize(placeOnWaterSurface, "placeOnWaterSurface", "placeOnWaterSurface");

		ar.serialize(position_tau, "position_tau", "position_tau");
		ar.serialize(orientation_tau, "orientation_tau", "orientation_tau");
		ar.serialize(additionalHorizontalRot, "additionalHorizontalRot", "additionalHorizontalRot");
		ar.serialize(additionalForvardRot, "additionalForvardRot", "additionalForvardRot");

		ar.closeBlock();
	}

	if(ar.openBlock("", "Подземные")){
		ar.serialize(digging_depth, "digging_depth", "digging_depth");
		ar.serialize(digging_depth_velocity, "digging_depth_velocity", "digging_depth_velocity");
		ar.serialize(undergroundMode, "undergroundMode", "underground mode");
		ar.closeBlock();
	}

	if(ar.openBlock("", "Летные")){
		ar.serialize(flyingMode, "flyingMode", "Летный тип");
		ar.serialize(hoverMode, "hoverMode", "hoverMode");
		ar.serialize(flying_height_relative, "flying_height_relative", "flying_height_relative");
		ar.serialize(flying_height, "flying_height", "flying_height");
		ar.serialize(alwaysMoving, "alwaysMoving", "alwaysMoving");
		ar.closeBlock();
	}

	if(ar.openBlock("", "Снаряды")){
		ar.serialize(minimize_theta, "minimize_theta", "minimize_theta");
		ar.serialize(upper_theta, "upper_theta", "upper_theta");
		ar.serialize(lower_theta, "lower_theta", "lower_theta");
		ar.serialize(targetCorrectionZ, "targetCorrectionZ", "targetCorrectionZ");
		ar.serialize(distance_correction_factor, "distance_correction_factor", "distance_correction_factor");
		ar.serialize(ground_colliding_delay, "ground_colliding_delay", "ground_colliding_delay");
		ar.serialize(ground_collision_enabled, "ground_collision_enabled", "ground_collision_enabled");
		ar.closeBlock();
	}

	if(ar.openBlock("", "Ракеты")){
		ar.serialize(rocket_vertical_control_distance, "rocket_vertical_control_distance", "rocket_vertical_control_distance");
		ar.serialize(rocket_forward_analysis_distance, "rocket_forward_analysis_distance", "rocket_forward_analysis_distance");
		ar.serialize(rocket_target_offset_z, "rocket_target_offset_z", "rocket_target_offset_z");
		ar.serialize(underwaterRocket, "underwaterRocket", "underwaterRocket");
		ar.closeBlock();
	}

	if(ar.openBlock("", "Падающие деревья и заборы")){
		ar.serialize(fence, "fence", "fence");
		ar.closeBlock();
	}
    
	if(ar.openBlock("", "Поиск пути")){
		ar.serialize(waterPass, "waterPass", "WaterPass");
		ar.serialize(groundPass, "groundPass", "GroundPass");
		ar.serialize(fieldPass, "fieldPass", "fieldPass");
		ar.serialize(waterAnalysis, "waterAnalysis", "waterAnalysis");
		ar.serialize(path_tracking_back, "path_tracking_back", "path_tracking_move_back");
		ar.serialize(ptObstaclesCheck, "ptObstaclesCheck", "ObstaclesCheck");
		ar.closeBlock();
	}

	if(ar.openBlock("", "RigidBodyPrmBase")){
		ar.serialize(sleepAtStart, "sleepAtStart", "sleepAtStart");
		ar.serialize(enableSleeping, "enableSleeping", "enableSleeping");
		ar.serialize(awakeCounter, "awakeCounter", "awakeCounter");
		ar.serialize(sleepMovemenThreshould, "sleepMovemenThreshould", "sleepMovemenThreshould");
		ar.serialize(average_movement_tau, "average_movement_tau", "average_movement_tau");
		ar.serialize(sphereCollision, "sphereCollision", "sphereCollision");
		ar.closeBlock();
	}

	if(ar.openBlock("", "RigidBodyPrmEnvironment")){
		ar.serialize(holdOrientation, "holdOrientation", "holdOrientation");
		ar.closeBlock();
	}

	if(ar.openBlock("", "RigidBodyPrmBox")){
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

	if(ar.openBlock("", "RigidBodyPrmCar")){
		ar.serialize(suspensionSpringConstant, "suspensionSpringConstant", "suspensionSpringConstant");
		ar.serialize(suspensionDamping, "suspensionDamping", "suspensionDamping");
		ar.serialize(drivingForceLimit, "drivingForceLimit", "drivingForceLimit");
		ar.serialize(wheelMass, "wheelMass", "wheelMass");
		ar.closeBlock();
	}

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
	float t30 = -2.0f * z;
	float t16 = x * x;
	float t11 = z * z + t16;
	float t29 = -2.0f * t11;
	float t28 = 0.5f / t11;
	float t13 = v * v;
	float t27 = z * gravity - t13;
	float t15 = 1.0f / v;
	float t26 = t11 * t15;
	float t25 = t28 * t30;
	float t9 = t13 * t13 + (t13 * t30 - t16 * gravity) * gravity;
	if(t9 < 0){
		//xassertStr(forward_velocity_max < FLT_EPS && "1. Can't reach the target. Increase forward_velocity_max for ", name);
		return M_PI_4;
	}
	t9 = sqrtf(t9);
	float t8 = t9 + t27;
	float t17 = t8 * t29;
	if(t17 < FLT_EPS) {
		//xassertStr(forward_velocity_max < FLT_EPS && "2. Can't reach the target. Increase forward_velocity_max for ", name);
		return M_PI_4;
	}
	t17 = sqrtf(t17);
	float t24 = (t8 * t25 + gravity) / t17 * t26;
	float t7 = t27 - t9;
	float t18 = t7 * t29;
	if(t18 < FLT_EPS) {
		//xassertStr(forward_velocity_max < FLT_EPS && "3. Can't reach the target. Increase forward_velocity_max for ", name);
		return M_PI_4;
	}
	t18 = sqrtf(t18);
	float t23 = (t7 * t25 + gravity) / t18 * t26;
	float t22 = t15 * x * t28;
	float t21 = t18 * t22;
	float t20 = t17 * t22;
	float r[4];
	r[0] = atan2(t23, t21);
	r[1] = atan2(-t23, -t21);
	r[2] = atan2(t24, t20);
	r[3] = atan2(-t24, -t20);
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
