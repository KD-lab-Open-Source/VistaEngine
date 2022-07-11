#ifndef __RIGID_BODY_PRM_H__
#define __RIGID_BODY_PRM_H__

#include "..\util\TypeLibrary.h"

enum RigidBodyType;

////////////////////////////////////////
struct RigidBodyPrm : PolymorphicBase
{
	////////////////////////////////////////////////
	//     Small usefull structs
	////////////////////////////////////////////////
	struct Oscillator
	{
		float amplitude;
		float omega;
		float amplitude_decrement;
		float omega_increment;
		float omega_disperse;
		
		float phase;
		float omegaDisperced;

		Oscillator();
		void serialize(Archive& ar);

		void set();
		float operator() (float dt, float velocity);
	};

	struct Stiffness
	{
		float stiffness_up;
		float stiffness_down;
		float dz_max_up;
		float dz_max_down;
	
		Stiffness();
		void serialize(Archive& ar);
		float force(float dz) const;
	};


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
	bool ptImpassabilityCheck;

	//=======================================================
	bool moveVertical;

	//=======================================================
	// Флаги проходимости...
	//=======================================================
	enum PassabilityFlags {
		IMPASSABILITY, // Непроходимый.
		PASSABILITY  // Проходимый.
	};
    
	PassabilityFlags waterPass;
	PassabilityFlags groundPass;
	PassabilityFlags impassabilityPass;

	bool waterAnalysis;  // Анализировать воду?

	////////////////////////////////////////////////
	//		General Controls
	////////////////////////////////////////////////
	// Не перемещается по x, y. Перемещается по z и ориентация.
	bool unmovable; 
	// Ненаправленный, двигается в любую сторону
	bool isotropic;
	// Управляется точками, иначе свободный объект
	bool controled_by_points;
	
	// Остановка 
	float brake_damping;//3.5;
	float point_control_slow_distance;
	float point_control_slow_factor;

	//  Steering
	float rudder_speed;
	float steering_duration;
	float steering_linear_velocity_min;
	float steering_acceleration_max;
	float is_point_reached_radius_max; // Max radius for is_point_reached

	// Traction
	float forward_velocity_max;
	float forward_acceleration;
	float verticalFactor;

	// Levelling
	float orientation_torque_factor; 
	float orientation_torque_max; 

	// Info
	float speed_map_factor; // Maps average_velocity -> [0..1]

	////////////////////////////////////////////////
	//     Ground unit parameters
	////////////////////////////////////////////////
	// Terrain analyse
	float analyze_points_density;
	int Dxy_minimal;
	float zero_velocity_z;
	float box_delta_y;
	float dz_max_avr_factor;
	bool canEnablePointAnalyze;

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
	bool flying_down_without_way_points;
	bool flying_height_relative;
	float flying_height;
	Stiffness flying_stiffness;
	Oscillator flying_oscillator_z;
	float flying_vertical_direction_x_factor;
	float flying_vertical_direction_y_factor;

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
	//      Debris
	////////////////////////////////////////////////
	float debris_angular_velocity;

	////////////////////////////////////////////////
	//      Falling trees and fences
	////////////////////////////////////////////////
	bool tree;
	bool fence;
	float fallAngularVelocity;

	////////////////////////////////////////////////
	// Sleep evolution system
	////////////////////////////////////////////////
	bool sleepedAtStart;
	float average_movement_tau;
	bool fcModeAlways;
	bool fcBodyScaner;
	float fcBodyScanerDist;

	////////////////////////////////////////////////
	//      Debug
	////////////////////////////////////////////////
	bool enable_show;

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
};

typedef StringTable<StringTableBasePolymorphic<RigidBodyPrm> > RigidBodyPrmLibrary;
typedef StringTablePolymorphicReference<RigidBodyPrm, true> RigidBodyPrmReference;

#endif //__RIGID_BODY_PRM_H__
