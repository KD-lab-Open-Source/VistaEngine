#ifndef __UNIT_ATTRIBUTE_H__
#define __UNIT_ATTRIBUTE_H__

#include "..\Util\TypeLibrary.h"
#include "..\Util\ResourceSelector.h"
#include "..\Util\LocString.h"

#include "..\Terra\terTools.h"

#include "..\UserInterface\UI_Types.h"
#include "..\UserInterface\UI_Inventory.h"
#include "..\UserInterface\UI_MarkObjectAttribute.h"

#include "EffectReference.h"
#include "SoundAttribute.h"
#include "AbnormalStateAttribute.h"
#include "Parameters.h"
#include "WeaponAttribute.h"
#include "CircleManagerParam.h"
#include "TriggerChainName.h"

#include "AttributeReference.h"
#include "WhellController.h"
#include "Object3dxInterface.h"

#include "..\Physics\RigidBodyNodePrm.h"

typedef vector<Vect2f> Vect2fVect;
typedef vector<Vect2i> Vect2iVect;
typedef vector<Vect3f> Vect3fVect;
typedef vector<Vect3f> Vect3fList;
typedef vector<ParameterArithmetics> UnitParameterArithmeticsList;
typedef vector<EffectAttributeAttachable> EffectAttributes;
typedef vector<LocString> LocStrings;

enum GameType;

class AttributeBase;
class WeaponPrm;
class EffectKey;
class cScene;
class cObject3dx;
class Archive;
class EffectAttributeAttachable;

struct RigidBodyPrm;
typedef StringTablePolymorphicReference<RigidBodyPrm, true> RigidBodyPrmReference;

/////////////////////////////////////////
enum UnitClass
{
	UNIT_CLASS_NONE = 0,

	UNIT_CLASS_ITEM_RESOURCE,
	UNIT_CLASS_ITEM_INVENTORY,

	UNIT_CLASS_REAL,
	UNIT_CLASS_BUILDING,
	UNIT_CLASS_LEGIONARY,
	UNIT_CLASS_TRAIL,
    
	UNIT_CLASS_PROJECTILE,
	UNIT_CLASS_PROJECTILE_BULLET,
	UNIT_CLASS_PROJECTILE_MISSILE,

	UNIT_CLASS_ENVIRONMENT,
	UNIT_CLASS_ENVIRONMENT_SIMPLE,
	UNIT_CLASS_ZONE,
	UNIT_CLASS_EFFECT,
	UNIT_CLASS_SQUAD,

	UNIT_CLASS_MAX
};

// Режими атаки без прямого указания цели.
enum AutoAttackMode
{
	RETURN_TO_POSITION,
	STOP_AND_ATTACK,
	STOP_NOT_ATTACK,

	GOTO_AND_KILL = RETURN_TO_POSITION,
	PATROL = STOP_NOT_ATTACK,
	PATROL_WITH_STOPS = STOP_NOT_ATTACK,

	ATTACK_MODE_DISABLE = STOP_NOT_ATTACK,
	ATTACK_MODE_DEFENCE = STOP_AND_ATTACK,
	ATTACK_MODE_OFFENCE = RETURN_TO_POSITION
};

// Режими атаки при ходьбе.
enum WalkAttackMode
{
	WALK_NOT_ATTACK,
	WALK_AND_ATTACK,
	WALK_STOP_AND_ATTACK,
};

/// Режим автоматического выбора целей
enum AutoTargetFilter
{
	AUTO_ATTACK_ALL,		///< атаковать всех
	AUTO_ATTACK_BUILDINGS,	///< атаковать только здания
	AUTO_ATTACK_UNITS		///< атаковать только юнитов
};

/// Режим движения при патрулировании.
enum PatrolMovementMode
{
	PATROL_MOVE_RANDOM,		///< случайным образом в радиусе партулирования
	PATROL_MOVE_CW,			///< по кругу по часовой стрелке
	PATROL_MOVE_CCW			///< по кругу против часовой стрелки
};

// Клавиши прямого управления.
enum DirectControlKeys {
	DIRECT_KEY_TURN_LEFT = 1,
	DIRECT_KEY_TURN_RIGHT = 2,
	DIRECT_KEY_MOVE_FORWARD = 4,
	DIRECT_KEY_MOVE_BACKWARD = 8,
	DIRECT_KEY_MOUSE_LBUTTON = 16,
	DIRECT_KEY_MOUSE_RBUTTON = 32,
	DIRECT_KEY_MOUSE_MBUTTON = 64,
	DIRECT_KEY_STRAFE_LEFT = 128,
	DIRECT_KEY_STRAFE_RIGHT = 256
};

enum AttackCondition
{
	ATTACK_GROUND,
	ATTACK_ENEMY_UNIT,
	ATTACK_MY_UNIT,
	ATTACK_GROUND_NEAR_ENEMY_UNIT,
	ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING
};

enum UpgradeOption
{
	UPGRADE_HERE,
	UPGRADE_ON_THE_DISTANCE,
	UPGRADE_ON_THE_DISTANCE_TO_ENEMY,
	UPGRADE_NEAR_OBJECT,
	UPGRADE_ON_THE_DISTANCE_FROM_ENEMY
};

enum CollisionGroupID
{
	COLLISION_GROUP_ACTIVE_COLLIDER = 1,
	COLLISION_GROUP_COLLIDER = 2,
	COLLISION_GROUP_REAL = COLLISION_GROUP_ACTIVE_COLLIDER | COLLISION_GROUP_COLLIDER,
	COLLISION_GROUP_GROUND_COLLIDER = 4
};

enum ExcludeCollision
{
	EXCLUDE_COLLISION_BULLET = 1,
	EXCLUDE_COLLISION_ENVIRONMENT = 2
};

enum
{
	SAVED_SELECTION_MAX = 15
};

/////////////////////////////////////////
struct PlacementZoneData : StringTableBase
{
	CircleEffect circleColor;
	float showRadius;

	PlacementZoneData(const char* name = "");

	void serialize(Archive& ar);
};

typedef StringTable<PlacementZoneData> PlacementZoneTable;
typedef StringTableReference<PlacementZoneData, false> PlacementZone;

////////////////////////////////////////
class UnitFormationType : public StringTableBase
{
public:
	UnitFormationType(const char* name = "") : StringTableBase(name), radius_(20), color_ (255, 255, 255) {}
	float radius() const { return radius_; }
	sColor4c color () const { return color_; }
	void serialize(Archive& ar);

private:
	sColor4c color_;
	float radius_;
};

/////////////////////////////////////////
struct UnitNumber
{
	UnitFormationTypeReference type;
	ParameterCustom numberParameters;

	void serialize(Archive& ar);
};
typedef vector<UnitNumber> UnitNumbers;

/////////////////////////////////////////

/// Настройки режимов атаки
class AttackMode
{
public:
	AttackMode();

	void serialize(Archive& ar);

	AutoAttackMode autoAttackMode() const { return autoAttackMode_; }
	void setAutoAttackMode(AutoAttackMode attack_mode){ autoAttackMode_ = attack_mode; }

	AutoTargetFilter autoTargetFilter() const { return autoTargetFilter_; }
	void setAutoTargetFilter(AutoTargetFilter filter){ autoTargetFilter_ = filter; }

	WalkAttackMode walkAttackMode() const { return walkAttackMode_; }
	void setWalkAttackMode(WalkAttackMode attack_mode){walkAttackMode_ = attack_mode; }

	WeaponMode weaponMode() const { return weaponMode_; }
	void setWeaponMode(WeaponMode weapon_mode){ weaponMode_ = weapon_mode; }

private:
	AutoAttackMode autoAttackMode_;
	AutoTargetFilter autoTargetFilter_;
	WalkAttackMode walkAttackMode_;
	WeaponMode weaponMode_;
};

class AttackModeAttribute
{
public:
	AttackModeAttribute();

	void serialize(Archive& ar);

	const AttackMode& attackMode() const { return attackMode_; }

	float patrolRadius() const { return patrolRadius_; }
	float patrolStopTime() const { return patrolStopTime_; }
	PatrolMovementMode patrolMovementMode() const { return patrolMovementMode_; }

	bool targetInsideSightRadius() const { return targetInsideSightRadius_; }

private:

	/// начальные установки режимов атаки
	AttackMode attackMode_;

	float patrolRadius_;
	float patrolStopTime_;
	PatrolMovementMode patrolMovementMode_;

	bool targetInsideSightRadius_;
};

/////////////////////////////////////////
struct RaceProperty : StringTableBase
{
	ParameterCustom initialResource;
	ParameterCustom resourceCapacity;

	UnitNumbers unitNumbers;

	int selectQuantityMax;

	CircleManagerParam circle;
	CircleManagerParam circleTeam;
	SelectionBorderColor selection_param;

	SoundReference startConstructionSound;
	SoundReference unableToConstructSound;

	AttributeUnitOrBuildingReferences initialUnits;
	TriggerChainNames commonTriggers;
	TriggerChainNames scenarioTriggers;
	TriggerChainNames battleTriggers;
	TriggerChainNames multiplayerTriggers;

	int produceMultyAmount;

	UI_ScreenReference screenToPreload;

	RaceProperty(const char* name = "");

	const char* fileNameAddition() const { return fileNameAddition_.c_str(); }
	bool instrumentary() const { return instrumentary_;	}
	const char* name() const { return locName_.empty() ? c_str() : locName_.c_str(); }

	const UI_MarkObjectAttribute& shipmentPositionMark() const { return shipmentPositionMark_; }
	const UI_MarkObjectAttribute& orderMark(UI_ClickModeMarkID mark_id) const;

	const EffectAttributeAttachable& unitAttackEffect() const { return unitAttackEffect_; }
	const EffectAttributeAttachable& weaponUpgradeEffect() const { return weaponUpgradeEffect_; }

	const UI_UnitSprite& workForAISprite() const { return workForAISprite_; }
	const EffectAttributeAttachable& workForAIEffect() const { return workForAIEffect_; }

	const UI_UnitSprite& runModeSprite() const { return runModeSprite_; }

	const UI_MinimapSymbol& minimapMark(UI_MinimapSymbolID mark_id) const;

	const AttackModeAttribute& attackModeAttr() const { return attackModeAttribute_; }

	void serialize(Archive& ar);

	void setCommonTriggers(TriggerChainNames& triggers, GameType gameType) const;

	bool used() const { return used_ || usedAlways_; }
	void setUsed(sColor4c skinColor, const char* emblemName) const;
	void setUnused() const;

	struct Skin {
		sColor4c skinColor;
		string emblemName;
	};
	typedef vector<Skin> Skins;
	const Skins& skins() const { return skins_; }

private:
	string fileNameAddition_;
	LocString locName_;
	bool instrumentary_;
	bool usedAlways_;
	mutable bool used_;
	mutable Skins skins_;
	
	// находится под управлением AI
	UI_UnitSprite workForAISprite_;
	EffectAttributeAttachable workForAIEffect_;

	// включен режим бега
	UI_UnitSprite runModeSprite_;

	/// флажок точки сбора производимых юнитов
	UI_MarkObjectAttribute shipmentPositionMark_;

	typedef std::vector<UI_MarkObjectAttribute> MarkObjectAttributes;
	/// визуализация отдачи приказов - атаки, перемещения, ремонта.
	MarkObjectAttributes orderMarks_;

	/// визуализация атаки по юниту
	EffectAttributeAttachable unitAttackEffect_;
	/// визуализация апгрейда оружия
	EffectAttributeAttachable weaponUpgradeEffect_;

	typedef vector<UI_MinimapSymbol> MiniMapMarks;
	/// обозначения событий на миникарте
	MiniMapMarks minimapMakrs_;

	/// Настройки режимов атаки
	AttackModeAttribute attackModeAttribute_;
};

#include "AttributeReference.h"

/////////////////////////////////////////////
//		Анимация
/////////////////////////////////////////////
// Цепочки анимации
enum ChainID
{
	CHAIN_NONE = 0,

	CHAIN_NIGHT = 1,

	CHAIN_CARGO_LOADED = 2,

	CHAIN_SLOT_IS_EMPTY = 3,

	CHAIN_MOVEMENTS = 1 << 4,

	CHAIN_STAND = CHAIN_MOVEMENTS + 1,
	CHAIN_WALK = CHAIN_MOVEMENTS + 2,
	CHAIN_RUN = CHAIN_MOVEMENTS + 3,
	CHAIN_TURN = CHAIN_MOVEMENTS + 4,

	CHAIN_GO_WALK = CHAIN_MOVEMENTS + 5,
	CHAIN_STOP_WALK = CHAIN_MOVEMENTS + 6,
	CHAIN_GO_RUN = CHAIN_MOVEMENTS + 7,
	CHAIN_STOP_RUN = CHAIN_MOVEMENTS + 8,

	CHAIN_BUILDING_STAND = CHAIN_MOVEMENTS + 9,
	
	CHAIN_CHANGE_FLYING_MODE = CHAIN_MOVEMENTS << 1,

	CHAIN_FLY_DOWN = CHAIN_CHANGE_FLYING_MODE + 1,
	CHAIN_FLY_UP = CHAIN_CHANGE_FLYING_MODE + 2,

	CHAIN_WORK = CHAIN_CHANGE_FLYING_MODE << 1,

	CHAIN_BUILD = CHAIN_WORK + 1,
	CHAIN_PICKING = CHAIN_WORK + 2,
	CHAIN_STAND_WITH_RESOURCE = CHAIN_WORK + 3,
	CHAIN_WALK_WITH_RESOURCE = CHAIN_WORK + 4,

	CHAIN_MOVE = CHAIN_WORK + 5,

	CHAIN_ATTACK = CHAIN_WORK << 1,
	
	CHAIN_FIRE = CHAIN_ATTACK + 1,
	CHAIN_FIRE_WALKING = CHAIN_ATTACK + 2,
	CHAIN_FIRE_RUNNING = CHAIN_ATTACK + 3,

	CHAIN_AIM = CHAIN_ATTACK + 4,
	CHAIN_AIM_WALKING = CHAIN_ATTACK + 5,
	CHAIN_AIM_RUNNING = CHAIN_ATTACK + 6,

	CHAIN_GIVE_RESOURCE = CHAIN_ATTACK << 1,

	CHAIN_TRANSITION = CHAIN_GIVE_RESOURCE << 1,

	CHAIN_TRANSPORT = CHAIN_TRANSITION << 1,

	CHAIN_LANDING = CHAIN_TRANSPORT + 1,

	CHAIN_OPEN_FOR_LANDING = CHAIN_TRANSPORT + 2,
	CHAIN_CLOSE_FOR_LANDING = CHAIN_TRANSPORT + 3,
	
	CHAIN_MOVE_TO_CARGO = CHAIN_TRANSPORT + 4,
	CHAIN_LAND_TO_LOAD = CHAIN_TRANSPORT + 5,

	CHAIN_PRODUCTION = CHAIN_TRANSPORT << 1,

	CHAIN_CONSTRUCTION = CHAIN_PRODUCTION << 1,

	CHAIN_UPGRADE = CHAIN_CONSTRUCTION + 1,

	CHAIN_DISCONNECT = CHAIN_CONSTRUCTION + 3,

	CHAIN_BE_BUILT = CHAIN_CONSTRUCTION << 1,

	CHAIN_IS_UPGRADED = CHAIN_BE_BUILT << 1,
	
	CHAIN_UPGRADED_FROM_BUILDING = CHAIN_IS_UPGRADED + 1,
	CHAIN_UPGRADED_FROM_LEGIONARY = CHAIN_IS_UPGRADED + 2,

	CHAIN_RISE = CHAIN_IS_UPGRADED << 1,

	CHAIN_FALL = CHAIN_RISE << 1,

	CHAIN_BIRTH = CHAIN_FALL << 1,

	CHAIN_TELEPORTING = CHAIN_BIRTH + 1,

	CHAIN_OPEN = CHAIN_BIRTH + 2,
	CHAIN_CLOSE = CHAIN_BIRTH + 3,

	CHAIN_WEAPON_GRIP = CHAIN_BIRTH + 4,

	CHAIN_BIRTH_IN_AIR = CHAIN_BIRTH + 5,

	CHAIN_IN_TRANSPORT = CHAIN_BIRTH << 1,

	CHAIN_DEATH = CHAIN_IN_TRANSPORT << 1,
	
	CHAIN_HOLOGRAM = CHAIN_DEATH + 1,

	CHAIN_TRIGGER = CHAIN_DEATH + 2,

	CHAIN_UNINSTALL = CHAIN_DEATH + 3
};

enum MovementStateID
{
	MOVEMENT_STATE_LEFT = 1,
	MOVEMENT_STATE_RIGHT = 2,
	MOVEMENT_STATE_FORWARD = 4,
	MOVEMENT_STATE_BACKWARD = 8,

	MOVEMENT_STATE_ALL_SIDES = MOVEMENT_STATE_LEFT | MOVEMENT_STATE_RIGHT | MOVEMENT_STATE_FORWARD | MOVEMENT_STATE_BACKWARD,
	
	MOVEMENT_STATE_ON_GROUND = 16,
	MOVEMENT_STATE_ON_LOW_WATER = 32,
	MOVEMENT_STATE_ON_WATER = 64,
	
	MOVEMENT_STATE_ALL_SURFACES = MOVEMENT_STATE_ON_GROUND | MOVEMENT_STATE_ON_WATER | MOVEMENT_STATE_ON_LOW_WATER,

	MOVEMENT_STATE_DEFAULT = MOVEMENT_STATE_ALL_SURFACES | MOVEMENT_STATE_ALL_SIDES
};

typedef BitVector<MovementStateID> MovementState;

struct AnimationChain
{
	ChainID chainID;
	MovementState movementState;
	AbnormalStateTypeReferences abnormalStateTypes;
	int abnormalStateMask;
	int counter;
	MovementState transitionToState;

	int animationAcceleration;
	int period; // mS
	bool cycled; 
	bool supportedByLogic;
	bool reversed; 
	bool syncBySound; 
	bool randomPhase; 
	bool stopPermanentEffects;
	int priority;

	EffectAttributes effects;

	SoundReferences soundReferences;
	SoundMarkers soundMarkers;

	AnimationChain();

	const char* name() const;

	int abnormalStatePriority() const;
	bool checkAbnormalState(const AbnormalStateType* astate) const;
	bool compare(const AnimationChain& rhs) const;

	void setDefaultPriority();
	void serialize(Archive& ar);

	int chainIndex() const { return chainIndex_; }
	int animationGroup() const { return animationGroup_; }
	C3dxVisibilityGroup visibilityGroup() const { return C3dxVisibilityGroup(visibilityGroup_); }

private:
	ChainName chainIndex_;
	AnimationGroupName animationGroup_;
	VisibilityGroupName visibilityGroup_;
};

typedef vector<AnimationChain> AnimationChains;
typedef pair<AnimationChains::const_iterator, AnimationChains::const_iterator> AnimationChainsInterval;

////////////////////////////////////////
class InterfaceTV
{
public:
	InterfaceTV(){
		radius_ = 80;
		position_ = Vect2f(0, 0);
		orientation_ = Vect3f(70, 0, 0);
	}

	void serialize(Archive& ar);

	float radius() const { return radius_; }
	const Vect2f& position() const { return position_; }
	const Vect3f& orientation() const { return orientation_; }
	const AnimationChain& chain() const { return chain_; }

private:
	float radius_;
	Vect2f position_;
	Vect3f orientation_;
	AnimationChain chain_;
};

////////////////////////////////////////

class ArmorFactors
{
public:
	ArmorFactors();
	void serialize(Archive& ar);
	
	float factor(const Vect3f& localPos) const;
	bool used() const { return used_; }

private:
	float front, back;
	float left, right;
	float top;
	bool used_;
};

////////////////////////////////////////
struct DifficultyPrm : StringTableBase
{
	int aiDelay; // Задержка АИ
	float triggerDelayFactor; // Коэффициент триггера задержка

	int	orderBuildingsDelay;
	int orderUnitsDelay;
	int orderParametersDelay;
	int upgradeUnitDelay;
	int simpleScanDelay;
	int eventScanDelay;
	//int attackBySpecialWeaponDelay;
	LocString locName;
	const char* name() const { return locName.empty() ? c_str() : locName.c_str(); }

	DifficultyPrm(const char* name = "");
	void serialize(Archive& ar);
};

////////////////////////////////////////

enum AttributeType 
{
	ATTRIBUTE_NONE,
	ATTRIBUTE_LIBRARY,
	ATTRIBUTE_AUX_LIBRARY,
	ATTRIBUTE_SQUAD,
	ATTRIBUTE_PROJECTILE
};

////////////////////////////////////////

class HarmAttribute 
{
public:
	typedef vector<AbnormalStateEffect> AbnormalStateEffects;
	AbnormalStateEffects abnormalStateEffects;

	HarmAttribute();
	
	const AbnormalStateEffect* abnormalStateEffect(const AbnormalStateType* type) const; 

	void serializeAbnormalStateEffects(Archive& ar);
	void serializeSources(Archive& ar);
	void serializeEnvironment(Archive& ar);
	void serialize(Archive& ar);

	const DeathAttribute& deathAttribute(const AbnormalStateType* type) const;

private:
	DeathAttribute deathAttribute_;
};

////////////////////////////////////////

// Транспорт
struct TransportSlot
{
	UnitFormationTypeReferences types;
	bool canFire;
	bool destroyWithTransport;
	bool visible;
	bool requiredForMovement;
	Logic3dxNode node;

	TransportSlot();
	void serialize(Archive& ar);
	bool checkType(UnitFormationTypeReference type) const;
};

typedef vector<TransportSlot> TransportSlots;

////////////////////////////////////////

enum FOWVisibleMode
{
	FVM_ALLWAYS,
	FVM_HISTORY_TRACK,
	FVM_NO_FOG,
};

enum ShadowType
{
	SHADOW_CIRCLE,
	SHADOW_REAL,
	SHADOW_DISABLED
};

class ModelShadow
{
public:
	ModelShadow();
	ShadowType shadowType() const { return shadowType_; }
	float shadowRadiusRelative() const { return shadowRadiusRelative_; }
	void set(ShadowType shadowType, float shadowRadiusRelative);
	void setType(ShadowType shadowType) { shadowType_ = shadowType; }
	void setShadowType(c3dx* model, float radius = 1.0f) const;
	void serialize(Archive& ar);
	void serializeForEditor(Archive& ar, float factor);

private:
	ShadowType shadowType_;
	float shadowRadiusRelative_;
};

////////////////////////////////////////
struct BodyTypeString : StringTableBaseSimple
{
	BodyTypeString(const char* name = "") : StringTableBaseSimple(name) {}
};

typedef StringTable<BodyTypeString> BodyPartTypeTable;
typedef StringTableReference<BodyTypeString, true> BodyPartType;

struct BodyPartAttribute
{
	enum Functionality
	{
		LIFE = 1,
		MOVEMENT = 2,
		PRODUCTION = 4,
		UPGRADE = 8,
		FIRE = 16
	};

	BodyPartType bodyPartType;
	BitVector<Functionality> functionality;
	typedef vector<int> Weapons;
	Weapons weapons;

	int percent;

	VisibilityGroupOfSet defaultGarment;

	struct Garment 
	{
		VisibilityGroupOfSet visibilityGroup;
		AttributeItemInventoryReference item;
		void serialize(Archive& ar);
	};
	typedef vector<Garment> Garments;

	Garments possibleGarments;

	struct AutomaticGarment : Garment
	{
		ParameterCustom parameters;
		void serialize(Archive& ar);
	};
	typedef vector<AutomaticGarment> AutomaticGarments;

	AutomaticGarments automaticGarments;

	RigidBodyModelPrm rigidBodyBodyPartPrm;

	BodyPartAttribute();
	void serialize(Archive& ar);

	int visibilitySet() const { return visibilitySet_; }

private:
	VisibilitySetName visibilitySet_;
};
typedef vector<BodyPartAttribute> BodyPartAttributes;

struct ProducedParameters
{
	ParameterArithmetics arithmetics;
	int time;
	ParameterCustom cost;
	UI_ShowModeSpriteReference sprites_;
	string signalVariable;
	AccessBuildingsList accessBuildingsList;
	ParameterCustom accessValue;
	bool automatic;

	ProducedParameters();
	void serialize(Archive& ar);
};

////////////////////////////////////////

class AttributeBase : public PolymorphicBase
{
public:
	string modelName;
	float boundScale;
	float boundRadius;
	float boundHeight;
	sBox6f boundBox;
	float radius_;
	bool showSilhouette;
	bool hideByDistance;
	bool selectBySphere;

	Vect2f basementExtent;
	
	AnimationChains animationChains;
 
	int dockNodeNumber;
	float creationTime;
	ParameterCustom installValue;
	ParameterCustom creationValue;
	ParameterCustom cancelConstructionValue;
	int accountingNumber;
	bool needBuilders;
	bool inheritHealthArmor;
	AccessBuildingsList accessBuildingsList;
	ParameterCustom accessValue;
	ParameterCustom uninstallValue;

	ParameterCustom parametersInitial;
	ParameterArithmetics parametersArithmetics;

	// Вывод при селекте
	typedef vector<ParameterShowSetting> ParamShowContainer;
	ParamShowContainer parameterShowSettings;
	ShowChangeParameterSettings showChangeParameterSettings;
	const ShowChangeSettings* getShowChangeSettings(int idx) const; //индекс в ParameterTypeTable
	LocString tipsName;
	int initialHeightUIParam;
	float selectCircleRelativeRadius;
	float selectRadius;
	sColor4c selectCircleColor;

	CircleEffect fireRadiusCircle;
	CircleEffect fireMinRadiusCircle;
	CircleEffect signRadiusCircle;
	
	ParameterArithmetics deathGainArithmetics;

	enum ProductionRequirement
	{
		PRODUCE_EVERYWHERE,
		PRODUCE_ON_WATER,
		PRODUCE_ON_TERRAIN
	};
	ProductionRequirement productionRequirement;
	float productionNightFactor;
	ParameterCustom productivity;
	ParameterCustom productivityTotal;

	ArmorFactors armorFactors;

	typedef std::vector<WeaponSlotAttribute> WeaponSlotAttributes;
	WeaponSlotAttributes weaponAttributes;

	bool hasAutomaticAttackMode;
	AttackModeAttribute attackModeAttribute;

	/// режимы оповещения о замеченных врагах
	enum AttackTargetNotificationMode 
	{
		/// оповещать свой сквад
		TARGET_NOTIFY_SQUAD = 1,
		/// оповещать всех в заданном радиусе
		TARGET_NOTIFY_ALL = 2
	};
	BitVector<AttackTargetNotificationMode> attackTargetNotificationMode;
	/// радиус оповещения о замеченных врагах
	float attackTargetNotificationRadius;

	EffectAttributes permanentEffects;

	bool internal;
	
	UnitFormationTypeReference formationType;

	AttackClass unitAttackClass;
	/// если true, то юнита никто не будет атаковать автоматически, 
	/// он не будет учтен в статистике убитых юнитов, АИ не будет его рассматривать как цель
	/// будет атакован только по явному указанию
	bool excludeFromAutoAttack;

	BitVector<ExcludeCollision> excludeCollision;
	BitVector<CollisionGroupID> collisionGroup;
	float contactWeight;

	PlacementZone producedPlacementZone;
	float producedPlacementZoneRadius;

	bool enablePathFind;
	bool enableVerticalFactor;
	bool useLocalPTStopRadius;
	float localPTStopRadius;

	float getPTStopRadius() const;

	float iconDistanceFactor;

	float sightRadiusNightFactor;
	float forwardVelocity;
	float forwardVelocityRunFactor;
	float sideMoveVelocityFactor;
	float backMoveVelocityFactor;
	bool dieStop;
	bool upgradeStop;
	float velocityByTerrainFactors[4];
	float flyingHeight;
	float waterLevel;
	float targetFlyRadius;
	WheelDescriptorList wheelList;

	ModelShadow modelShadow;
		
	AttributeItemReferences leavingItems;
	
	/// обозначение юнита на миникарте
	UI_MinimapSymbolType minimapSymbolType_;
	UI_MinimapSymbol minimapSymbol_;
	bool hasPermanentSymbol_;
	UI_MinimapSymbol minimapPermanentSymbol_;
	float minimapScale_;

	TerToolCtrl traceTerTool;

	struct TraceInfo
	{
		TraceInfo(){ surfaceKind_ = SURFACE_KIND_1 | SURFACE_KIND_2 | SURFACE_KIND_3 | SURFACE_KIND_4; }

		BitVector<SurfaceKind> surfaceKind_;
		TerToolCtrl traceTerTool_;

		void serialize(Archive& ar);
	};

	typedef std::vector<TraceInfo> TraceInfos;
	/// следы
	TraceInfos traceInfos;

	typedef std::vector<Logic3dxNode> TraceNodes;
	/// логические объекты, к которым привязываются следы
	TraceNodes traceNodes;

	Logic3dxNode gripNode;

	RigidBodyPrmReference rigidBodyPrm; // Параметры физики
	float mass;
	float waterWeight_;
	float waterVelocityFactor;
	float resourceVelocityFactor;
	float waterFastVelocityFactor;
	float manualModeVelocityFactor;
	bool enablePathTracking;
	float pathTrackingAngle;
	bool enableRotationMode;
	float angleRotationMode;
	float angleSwimRotationMode;
	bool rotToTarget;
	bool ptSideMove;

	BitVector<EnvironmentType> environmentDestruction;
    
	HarmAttribute harmAttr;

	InterfaceTV interfaceTV;
	int selectionListPriority;

	ShowEvent transportSlotShowEvent;
	UI_UnitSprite transportSlotEmpty;
	UI_UnitSprite transportSlotFill;

	const UI_Cursor* selectionCursorProxy() const { return selectionCursorProxy_; }
	void setSelectionCursorProxy(const UI_Cursor* cursor){ selectionCursorProxy_ = cursor; }

	UI_InventoryReferences inventories;

	BodyPartAttributes bodyParts;

	RigidBodyModelPrm rigidBodyModelPrm;

	struct Upgrade 
	{
		AttributeUnitOrBuildingReference upgrade;
		bool automatic;
		bool built;
		int level;
		ParameterCustom upgradeValue;
		ParameterCustom accessParameters;
		int chainUpgradeNumber;
		int chainUpgradeTime;
		
		Upgrade();
		void serialize(Archive& ar);
	};
	typedef StaticMap<int, Upgrade> Upgrades;
	Upgrades upgrades;
	bool upgradeAutomatically;

	TransportSlots transportSlots;
	float transportLoadRadius;
	float transportLoadDirectControlRadius;
	bool checkRequirementForMovement;
	ParameterArithmetics additionToTransport;
	int transportSlotsRequired;

	struct ProducedUnits
	{
		AttributeUnitReference unit;
		int number;
		ProducedUnits() : number(1) {}
		void serialize(Archive& ar);
	};
	typedef StaticMap<int, ProducedUnits> ProducedUnitsVector;
	ProducedUnitsVector producedUnits;
	int producedUnitQueueSize;
	vector<Logic3dxNode> dockNodes;
	bool automaticProduction;
	int totalProductionNumber;
	AttributeReferences producedThisFactories;

	typedef StaticMap<int, ProducedParameters> ProducedParametersList;
	ProducedParametersList producedParameters;
	bool produceParametersAutomatically;

	ParameterCustom resourceCapacity;

	bool canBeCaptured;
	bool putInIdleList;
	bool isHero;
	bool isStrategicPoint;
	bool accountInCondition;

	bool transparent_mode;
	FOWVisibleMode fow_mode;

	int chainTransitionTime;
	int chainLandingTime;
	int chainGiveResourceTime;
	int chainRiseTime;
	//int chainGoToRunTime;
	//int chainStopToRunTime;
	int chainOpenTime;
	int chainCloseTime;
	int chainBirthTime;
	int chainOpenForLandingTime;
	int chainCloseForLandingTime;
	int chainFlyDownTime;
	int chainUpgradedFromBuilding;
	int chainUpgradedFromLegionary;
	int chainUninsatalTime;
	// невидимый
	bool invisible;
	bool canChangeVisibility;
	UnitColor transparenceDiffuseForAlien;
	UnitColor transparenceDiffuseForClan;

	bool defaultDirectControlEnabled;
	Object3dxNode directControlNode;
	Vect3f directControlOffset;
	Vect3f directControlOffsetWater;
	float directControlThetaMin;
	float directControlThetaMax;
	
	enum ObjectLodPredefinedType lodDistance;

	//---------------------------------------
	AttributeBase();
	virtual ~AttributeBase(){}

	void preload() const;

	UnitClass unitClass() const { return unitClass_; }

	AttributeType attributeType() const;

	bool isBuilding() const { return unitClass() == UNIT_CLASS_BUILDING; }
	bool isLegionary() const { return unitClass() == UNIT_CLASS_LEGIONARY || unitClass() == UNIT_CLASS_TRAIL; }
	bool isTrail() const { return unitClass() == UNIT_CLASS_TRAIL; }
	bool isResourceItem() const { return unitClass() == UNIT_CLASS_ITEM_RESOURCE; }
	bool isInventoryItem() const { return unitClass() == UNIT_CLASS_ITEM_INVENTORY; }
	bool isSquad() const { return unitClass() == UNIT_CLASS_SQUAD; }
	bool isProjectile() const { return unitClass() == UNIT_CLASS_PROJECTILE || unitClass() == UNIT_CLASS_PROJECTILE_BULLET || unitClass() == UNIT_CLASS_PROJECTILE_MISSILE; }
	bool isEnvironmentBuilding() const { return unitClass() == UNIT_CLASS_ENVIRONMENT; }
	bool isEnvironmentSimple() const { return unitClass() == UNIT_CLASS_ENVIRONMENT_SIMPLE; }
	bool isEnvironment() const { return isEnvironmentBuilding() || isEnvironmentSimple(); }
	bool isActing() const { return isBuilding() || isLegionary(); } 
	bool isObjective() const { return isActing() || isResourceItem() || isInventoryItem(); } 
	bool isReal() const { return isObjective() || isProjectile(); }
	
	bool isTransport() const { return !transportSlots.empty(); }

	bool hasProdused() const { return !producedUnits.empty() || !producedParameters.empty(); }

	virtual float radius() const;

	void initGeometryAttribute();
	void refreshChains(bool editArchive);
	virtual bool isChainNecessary(ChainID chainID) const { return true; }

	const AnimationChain* animationChain(ChainID chainID, int counter = -1, const AbnormalStateType* astate = 0, MovementState movementState = MOVEMENT_STATE_DEFAULT) const; // По умолчанию - случайная
	const AnimationChain* animationChainByFactor(ChainID chainID, float factor, const AbnormalStateType* astate = 0, MovementState movementState = MOVEMENT_STATE_DEFAULT) const;
	const AnimationChain* animationChainTransition(float factor, const AbnormalStateType* astate, MovementState stateFrom, MovementState stateTo) const;

	int animationChainIndex(const AnimationChain* chain) const;
	const AnimationChain* animationChainByIndex(int index) const;
	
	bool canProduce(const AttributeBase* attribute) const;

	bool hasInventory() const { return !inventories.empty(); }

	void calcBasementPoints(float angle, const Vect2f& center, Vect2i points[4]) const;

	// Кешируется, может отствавать в момент после редактирования до записи
	// У основной библиотеки формат "Раса, юнит", у сквадов и снарядов - просто имя
	virtual const char* libraryKey() const; 
	//const UnitName& unitName() const { return unitAttributeID_.unitName(); }
	//const Race& race() const { return unitAttributeID_.race(); }

	virtual void serialize(Archive& ar);

	static float producedPlacementZoneRadiusMax() { return producedPlacementZoneRadiusMax_; }

	static cObject3dx* model(){ return model_; }
	static cObject3dx* logicModel(){ return logicModel_; }
	static const char* currentLibraryKey() { return currentLibraryKey_; }

	static bool createModel(const char* model_name);
	static void releaseModel();
	static void setModel(cObject3dx* model, cObject3dx* logicModel);
	static void setCurrentLibraryKey(const char* currentLibraryKey) { currentLibraryKey_ = currentLibraryKey; }

	const char* interfaceName(int num) const { xassert(num >= 0); return num < interfaceName_.size() ? interfaceName_[num].c_str() : ""; }
	const char* interfaceDescription(int num) const { xassert(num >= 0); return num < interfaceDescription_.size() ? interfaceDescription_[num].c_str() : ""; }

protected:
	UnitClass unitClass_;

	static float producedPlacementZoneRadiusMax_;

	static cScene* scene_;
	static cObject3dx* model_;
	static cObject3dx* logicModel_;
	static const char* currentLibraryKey_;

	LocStrings interfaceName_;
	LocStrings interfaceDescription_;

private:
	string libraryKey_;
	
	AbnormalStateAttribute waterEffect; // Прокеширован в AttributeCache, брать оттуда
	AbnormalStateAttribute lavaEffect; // Прокеширован в AttributeCache, брать оттуда
	AbnormalStateAttribute iceEffect; // Прокеширован в AttributeCache, брать оттуда
	AbnormalStateAttribute earthEffect; // Прокеширован в AttributeCache, брать оттуда

	AnimationChainsInterval findAnimationChainInterval(ChainID chainID, const AbnormalStateType* astate, MovementState movementState) const;
	AnimationChainsInterval findTransitionChainInterval(const AbnormalStateType* astate, MovementState stateFrom, MovementState stateTo) const;

	class RigidBodyModelPrmBuilder
	{
	public:
		RigidBodyModelPrmBuilder(RigidBodyModelPrm& modelPrm, BodyPartAttributes& bodyParts, cObject3dx* model);

	private:
		RigidBodyModelPrm::const_iterator findParentBody(int index) const;

		RigidBodyModelPrm& modelPrm_;
		cObject3dx* model_;
	};

	UI_CursorReference selectionCursor_;
	// Не сериализуется. Нужен, чтобы можно было менять курсор во время выполнения
	// без опасения запортить библиотеку
	const UI_Cursor* selectionCursorProxy_;

	friend class AttributeCache;
};

#endif
