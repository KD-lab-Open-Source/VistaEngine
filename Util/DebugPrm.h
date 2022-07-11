#ifndef _DEBUG_PRM_H_
#define _DEBUG_PRM_H_

#include "LibraryWrapper.h"
extern bool debugShowEnabled;

struct ShowDebugRigidBody {
	bool boundingBox;
	bool mesh;
	bool radius;
	bool inscribedRadius;
	bool boundRadius;
	bool wayPoints;
	bool autoPTPoint;
	bool contacts;
	bool movingDirection;
	bool localFrame;
	bool velocity;
	bool acceleration;
	float linearScale;
	float angularScale;
	bool rocketTarget;
	bool velocityValue;
	bool ptVelocityValue;
	bool average_movement;
	bool showPathTracking_ObstaclesLines;
	bool showPathTracking_ImpassabilityLines;
	bool showPathTracking_VehicleRadius;
	bool ground_colliding;
	bool onLowWater;
	bool target;
	bool showMode;
	bool showDebugMessages;
	int debugMessagesCount;

	ShowDebugRigidBody();
	void serialize(Archive& ar);
};

struct ShowDebugUnitBase {
	bool radius;
	bool libraryKey;
	bool modelName;
	bool abnormalState;
	bool effects;
	bool clan;
	bool producedPlacementZone;
	bool lodDistance_;
	
	ShowDebugUnitBase();
	void serialize(Archive& ar);
};

struct ShowDebugUnitInterface {
	bool debugString;
	ShowDebugUnitInterface();
	void serialize(Archive& ar);
};

struct ShowDebugUnitReal {
	bool target;
	bool fireResponse;
	bool toolzer;
	bool positionValue;
	bool unitID;
	bool producedUnitQueue;
	bool transportSlots;
	bool docking;
	bool chain;
	bool parameters;
	bool parametersMax;
	bool waterDamage;
	bool iceDamage;
	bool lavaDamage;
	bool earthDamage;
	bool attackModes;
	bool currentChain;
	bool homePosition;
	
	ShowDebugUnitReal();
	void serialize(Archive& ar);
};

struct ShowDebugLegionary {
	bool invisibility;
	bool level;
	bool transport;
	bool resourcerMode;
	bool resourcerProgress;
	bool resourcerCapacity;
	bool trace;
	bool aimed;
	
	ShowDebugLegionary();
	void serialize(Archive& ar);
};

struct ShowDebugBuilding {
	bool status;
	bool basement;
	
	ShowDebugBuilding();
	void serialize(Archive& ar);
};


struct ShowDebugSquad {
	bool position;
	bool wayPoints;
	bool umbrella;
	bool fire_radius;
	bool sight_radius;
	bool described_radius;
	bool attackAction;
	bool squadToFollow;
	bool order;
	bool unitsNumber;
	
	ShowDebugSquad();
	void serialize(Archive& ar);
};

struct ShowDebugUnitEnvironment {
	bool rigidBody;
	bool modelName;
	bool environmentType;
	bool treeType;
	bool treeMode;

	ShowDebugUnitEnvironment();
	void serialize(Archive& ar);
};

struct ShowDebugPlayer {
	bool placeOp;
	bool scanBound;
	bool resource;
	bool showSelectedOnly;
	bool showStatistic;
	bool saveLogStatistic;
	bool showSearchRegion;
	
	ShowDebugPlayer();
	void serialize(Archive& ar);
};

struct ShowDebugSource {
	bool enable;
	bool name;
	bool label;
	bool radius;
	bool axis;
	bool type;
	bool zoneDamage;
	bool zoneStateDamage;
	bool dontShowInfo;
	bool showEnvironmentPoints;
	bool showLightningUnitChainRadius;
	mutable int sourceCount;
	
	ShowDebugSource();
	void serialize(Archive& ar);
};

struct ShowDebugWeapon {
	bool enable;
	bool showSelectedOnly;
	bool direction;
	bool horizontalAngle;
	bool verticalAngle;
	bool gripVelocity;
	bool targetingPosition;
	bool ownerTarget;
	bool ownerSightRadius;
	bool autoTarget;
	bool angleValues;
	bool angleLimits;
	bool parameters;
	bool damage;
	bool load;
	bool fireRadius;

	int showWeaponID;
	
	ShowDebugWeapon();
	void serialize(Archive& ar);
};

struct ShowDebugInterface {
	bool showDebug;
	int writeLog;
	bool disableTextures;
	bool background;
	bool screens;
	bool controlBorder;
	bool hoveredControlBorder;
	bool hoveredControlExtInfo;
	bool focusedControlBorder;
	bool marks;
	bool hoverUnitBound;
	bool enableAllNetControls;
	bool showUpMarksCount;
	bool showAimPosition;
	bool showSelectManager;
	bool cursorReason;
	bool logicDispatcher;
	bool showTransformInfo;

	bool needShow() const { return debugShowEnabled && showDebug; }
	int getShowUpMarksCount() const;

	ShowDebugInterface();
	void serialize(Archive& ar);
};

struct ShowDebugEffects {
	bool showName;
	bool axis;

	ShowDebugEffects();
	void serialize(Archive& ar);
};

struct ShowDebugTerrain {
	bool showBuildingInfo;
	int showTerrainSpecialInfo;

	ShowDebugTerrain();
	void serialize(Archive& ar);
};

extern bool debugWireFrame;
extern bool debugShowWatch;
extern int debug_window_sx;
extern int debug_window_sy;
extern ShowDebugRigidBody showDebugRigidBody;
extern ShowDebugUnitBase showDebugUnitBase;
extern ShowDebugUnitInterface showDebugUnitInterface;
extern ShowDebugUnitReal showDebugUnitReal;
extern ShowDebugLegionary showDebugLegionary;
extern ShowDebugBuilding showDebugBuilding;
extern ShowDebugSquad showDebugSquad;
extern ShowDebugUnitEnvironment showDebugUnitEnvironment;
extern bool show_environment_type;
extern ShowDebugPlayer showDebugPlayer;
extern ShowDebugSource showDebugSource;
extern ShowDebugWeapon showDebugWeapon;
extern ShowDebugInterface showDebugInterface;
extern ShowDebugEffects showDebugEffects;
extern ShowDebugTerrain showDebugTerrain;
extern int show_filth_debug;
extern float show_vector_zmin;
extern float show_vector_zmax;
extern int mt_interface_quant;
extern int ht_intf_test;
extern bool disableHideUnderscore;
extern int terMaxTimeInterval;
extern int logicTimePeriod;
extern float logicTimePeriodInv;
extern float logicPeriodSeconds;
extern int debug_show_briefing_log;
extern int debug_show_mouse_position;
extern int debug_show_energy_consumption;
extern int debug_allow_mainmenu_gamespy;
extern int debug_allow_replay;
extern bool show_second_map;
extern bool show_pathfinder_map;
extern bool show_pathtracking_map_for_selected;
extern bool show_normal_map;
extern bool show_wind_map;
extern bool debug_stop_time;
extern bool showDebugWaterHeight;
extern bool showDebugAnchors;
extern bool cameraGroundColliding;
extern bool showDebugCrashSystem;
extern bool showDebugNumSounds;
extern bool debugDisableFogOfWar;
extern bool debugDisableSpecialExitProcess;

struct DebugPrm : public LibraryWrapper<DebugPrm>
{
	bool enableKeyHandlers;
	bool forceDebugKeys;
	float debugDamage;
	float debugDamageArmor;
	bool showNetStat;
	
	DebugPrm();
	void serialize(Archive& ar);
};

#endif //_DEBUG_PRM_H_
