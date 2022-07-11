#ifndef __RIGID_BODY_PRM_H__
#define __RIGID_BODY_PRM_H__

#include "Serialization\StringTableReferencePolymorphic.h"

enum RigidBodyType;

////////////////////////////////////////
struct RigidBodyPrm : PolymorphicBase
{
	////////////////////////////////////////////////
	//     Type and name
	////////////////////////////////////////////////
	enum RigidBodyTypeOld
	{
		UNIT, 
		MISSILE, 
		ROCKET,
		DEBRIS,
		ITEM,
		ENVIRONMENT,
		NOT_EVOLUTION
	};
	RigidBodyTypeOld unit_type; 

	RigidBodyType rigidBodyType; 

	////////////////////////////////////////////////
	// Base physics properties
	////////////////////////////////////////////////
	// Dampings
	Vect3f linear_damping;
	float angular_damping;

	// Bound
	
	bool path_tracking_back;
	bool ptObstaclesCheck;
	
	//=======================================================
	bool moveVertical;

	//=======================================================
	// Флаги проходимости...
	//=======================================================
	bool waterPass;
	bool groundPass;
	bool fieldPass;
	
	bool waterAnalysis;  // Анализировать воду?

	////////////////////////////////////////////////
	//		General Controls
	////////////////////////////////////////////////
	// Не перемещается по x, y. Перемещается по z и ориентация.
	bool unmovable; 
	// Управляется точками, иначе свободный объект
	bool controled_by_points;
	
	//  Steering
	int steering_duration;
	float is_point_reached_radius_max; // Max radius for is_point_reached

	// Traction
	float forward_velocity_max;
	float forward_acceleration;
	float verticalFactor;

	// Levelling
	float orientation_torque_factor; 
	
	////////////////////////////////////////////////
	//     Ground unit parameters
	////////////////////////////////////////////////
	// Terrain analyse
	bool canEnablePointAnalyze;
	bool placeOnWaterSurface;

	// UnderGround
	float digging_depth;
	float digging_depth_velocity;
	bool undergroundMode;

	// Simple Unit Evolve
	float position_tau;
	float orientation_tau;
	float additionalHorizontalRot;
	float additionalForvardRot;


	////////////////////////////////////////////////
	//		Flying 
	////////////////////////////////////////////////
	bool flyingMode;
	bool hoverMode;
	bool flying_height_relative;
	float flying_height;
	bool alwaysMoving;
			
	////////////////////////////////////////////////
	//      Misssile
	////////////////////////////////////////////////
	bool minimize_theta;
	float upper_theta; // degrees
	float lower_theta; // degrees
	float targetCorrectionZ;
	float distance_correction_factor; // из-за ошибки интегрирования снаряды чуть-чуть перелетают.
	int ground_colliding_delay;
	bool ground_collision_enabled; 

	////////////////////////////////////////////////
	//		Rocket
	////////////////////////////////////////////////
	float rocket_vertical_control_distance;
	float rocket_forward_analysis_distance;
	float rocket_target_offset_z;
	bool underwaterRocket; 

	////////////////////////////////////////////////
	//      Falling trees and fences
	////////////////////////////////////////////////
	bool fence;
		
	RigidBodyPrm();

	virtual ~RigidBodyPrm() {}

	void serialize(Archive& ar);
		
	string name() const;
	float calcTurnTheta(float x, float z, float velocity) const;



	// RigidBodyPrmBase

	// Sleeping
	bool sleepAtStart;
	bool enableSleeping;
	int awakeCounter;
	float sleepMovemenThreshould;
	float average_movement_tau;

	// RigidBodyPrmBase
	bool holdOrientation;

	// Collision
	bool sphereCollision;

	// RigidBodyPrmBox
	float linearDamping;
	float angularDamping;
	float gravity;
	float TOI_factor;
	float friction;
	float restitution;
	float relaxationTime;
	bool joint;
	Mat3f axes;
	Vect3f upperLimits; 
	Vect3f lowerLimits;

	// RigidBodyPrmCar
	float suspensionSpringConstant;
	float suspensionDamping;
	float drivingForceLimit;
	float wheelMass;
};

typedef StringTable<StringTableBasePolymorphic<RigidBodyPrm> > RigidBodyPrmLibrary;
typedef StringTableReferencePolymorphic<RigidBodyPrm, true> RigidBodyPrmReference;

#endif //__RIGID_BODY_PRM_H__
