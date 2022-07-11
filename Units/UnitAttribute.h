#ifndef __UNIT_ATTRIBUTE_H__
#define __UNIT_ATTRIBUTE_H__

#include "Serialization\StringTableReference.h"
#include "Serialization\EnumTable.h"
#include "LocString.h"

#include "Terra\TerToolCtrl.h"
#include "Terra\terra.h"

#include "EffectReference.h"
#include "SoundAttribute.h"
#include "AbnormalStateAttribute.h"
#include "Parameters.h"
#include "WeaponAttribute.h"
#include "CircleManagerParam.h"
#include "TriggerChainName.h"

#include "AttributeReference.h"
#include "Object3dxInterface.h"

#include "Physics\RigidBodyNodePrm.h"
#include "Physics\RigidBodyCarPrm.h"
#include "Physics\WindMap.h"

#include "UserInterface\UI_MarkObjectAttribute.h"
#include "Environment\Anchor.h"
#include "UserInterface\UI_MinimapSymbol.h"
#include "FileUtils\FileTime.h"
#include "Terra\TerrainType.h"

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

/////////////////////////////////////////
enum UnitClass
{
	UNIT_CLASS_NONE = 0,

	UNIT_CLASS_ITEM_RESOURCE,
	UNIT_CLASS_ITEM_INVENTORY,

	UNIT_CLASS_REAL,
	UNIT_CLASS_BUILDING,
	UNIT_CLASS_LEGIONARY,
	UNIT_CLASS_PAD,
	    
	UNIT_CLASS_PROJECTILE,
	UNIT_CLASS_PROJECTILE_BULLET,
	UNIT_CLASS_PROJECTILE_MISSILE,

	UNIT_CLASS_ENVIRONMENT,
	UNIT_CLASS_ENVIRONMENT_SIMPLE,
	UNIT_CLASS_ZONE,
	UNIT_CLASS_EFFECT,
	UNIT_CLASS_SQUAD,

	UNIT_CLASS_PLAYER,

	UNIT_CLASS_MAX
};

// Режими атаки без прямого указания цели.
enum AutoAttackMode
{
	ATTACK_MODE_DISABLE = 0,
	ATTACK_MODE_DEFENCE,
	ATTACK_MODE_OFFENCE
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
	ATTACK_GROUND_NEAR_MY_UNIT,
	ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING
};

enum UpgradeOption
{
	UPGRADE_HERE,
	UPGRADE_ON_THE_DISTANCE,
	UPGRADE_ON_THE_DISTANCE_TO_ENEMY,
	UPGRADE_NEAR_OBJECT,
	UPGRADE_ON_THE_DISTANCE_FROM_ENEMY,
	UPGRADE_ON_THE_ENEMY_DIRECTION,
	UPGRADE_ON_THE_DISCONNECTED_DIRECTION,
	UPGRADE_FROM_UNIT_ON_THE_ENEMY_DIRECTION
};

enum CollisionGroupID
{
	COLLISION_GROUP_ACTIVE_COLLIDER = 1,
	COLLISION_GROUP_COLLIDER = 2,
	COLLISION_GROUP_REAL = COLLISION_GROUP_ACTIVE_COLLIDER | COLLISION_GROUP_COLLIDER,
};

enum ExcludeCollision
{
	EXCLUDE_COLLISION_BULLET = 1, // Пули не сталкиваются между собой
	EXCLUDE_COLLISION_ENVIRONMENT = 2, // Объекты окружения и игровые здания не сталкиваются между собой
	EXCLUDE_COLLISION_LEGIONARY = 4, // Юниты не сталкиваются между собой
};

/////////////////////////////////////////
struct PlacementZoneData : StringTableBase
{
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
	Color4c color () const { return color_; }
	void serialize(Archive& ar);

private:
	Color4c color_;
	float radius_;
};

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

	void setTransport(bool state){ isTransport_ = state; }
	bool targetInsideSightRadius() const { return targetInsideSightRadius_; }
	bool disableEmptyTransportAttack() const { return disableEmptyTransportAttack_; }

private:

	/// начальные установки режимов атаки
	AttackMode attackMode_;

	bool targetInsideSightRadius_;

	bool isTransport_;
	bool disableEmptyTransportAttack_;
};

/////////////////////////////////////////
struct RaceProperty : StringTableBase
{
	ParameterCustom initialResource;
	ParameterCustom resourceCapacity;

	int selectQuantityMax;

	CircleManagerParam circle;
	CircleManagerParam circleTeam;
	CircleManagerParam placementZoneCircle;
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
	const wchar_t* name() const { return locName_.c_str(); }

	const UI_MarkObjectAttribute& shipmentPositionMark() const { return shipmentPositionMark_; }
	const UI_MarkObjectAttribute& orderMark(UI_ClickModeMarkID mark_id) const { return orderMarks_(mark_id); }

	const EffectAttributeAttachable& unitAttackEffect() const { return unitAttackEffect_; }
	const EffectAttributeAttachable& weaponUpgradeEffect() const { return weaponUpgradeEffect_; }

	const UI_UnitSprite& workForAISprite() const { return workForAISprite_; }
	const EffectAttributeAttachable& workForAIEffect() const { return workForAIEffect_; }

	const UI_UnitSprite& runModeSprite() const { return runModeSprite_; }
	
	const UI_UnitSprite& squadSpriteForOthers() const { return squadSpriteForOthers_; }
	const UI_UnitSprite& squadSpriteForOthersHovered() const { return squadSpriteForOthersHovered_; }

	const UI_MinimapEventStatic& minimapMark(UI_MinimapSymbolID mark_id) const { return minimapMarks_(mark_id); }
	const UI_MinimapSymbol& windMark(WindMap::WindType mark_id) const { return windMarks_(mark_id); }

	const Anchor& anchorForAssemblyPoint() const { return anchorForAssemblyPoint_; }

	const AttackModeAttribute& attackModeAttr() const { return attackModeAttribute_; }

	void serialize(Archive& ar);

	void setCommonTriggers(TriggerChainNames& triggers, GameType gameType) const;

	bool used() const { return used_ || usedAlways_; }
	void setUsed(Color4c skinColor, const char* emblemName) const;
	void setUnused() const;

	struct Skin {
		Color4c skinColor;
		string emblemName;
	};
	typedef vector<Skin> Skins;
	const Skins& skins() const { return skins_; }

	const AttributeBase* playerUnitAttribute() const { return playerUnitAttribute_; }

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

	UI_UnitSprite squadSpriteForOthers_;
	UI_UnitSprite squadSpriteForOthersHovered_;

	/// флажок точки сбора производимых юнитов
	UI_MarkObjectAttribute shipmentPositionMark_;

	typedef EnumTable<UI_ClickModeMarkID, UI_MarkObjectAttribute> MarkObjectAttributes;
	/// визуализация отдачи приказов - атаки, перемещения, ремонта.
	MarkObjectAttributes orderMarks_;

	/// якорь для установки в точку общего сбора
	Anchor anchorForAssemblyPoint_;

	/// визуализация атаки по юниту
	EffectAttributeAttachable unitAttackEffect_;
	/// визуализация апгрейда оружия
	EffectAttributeAttachable weaponUpgradeEffect_;

	typedef EnumTable<UI_MinimapSymbolID, UI_MinimapEventStatic> MiniMapMarks;
	/// обозначения событий на миникарте
	MiniMapMarks minimapMarks_;

	typedef EnumTable<WindMap::WindType, UI_MinimapSymbol> WindMarkAttributes;
	/// визуализация направления ветра на миникарте
	WindMarkAttributes windMarks_;

	/// Настройки режимов атаки
	AttackModeAttribute attackModeAttribute_;

	AttributePlayerReference playerUnitAttribute_;
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
	
	CHAIN_CONNECT = 4,

	CHAIN_MOVEMENTS = 1 << 4,

	CHAIN_STAND = CHAIN_MOVEMENTS + 1,
	CHAIN_WALK = CHAIN_MOVEMENTS + 2,

	CHAIN_BUILDING_STAND = CHAIN_MOVEMENTS + 3,
	
	CHAIN_PAD_STAND = CHAIN_MOVEMENTS + 4,

	CHAIN_CHANGE_FLYING_MODE = CHAIN_MOVEMENTS << 1,

	CHAIN_FLY_DOWN = CHAIN_CHANGE_FLYING_MODE + 1,
	CHAIN_FLY_UP = CHAIN_CHANGE_FLYING_MODE + 2,
	
	CHAIN_WORK = CHAIN_CHANGE_FLYING_MODE << 1,

	CHAIN_BUILD = CHAIN_WORK + 1,
	CHAIN_PICKING = CHAIN_WORK + 2,
	CHAIN_WITH_RESOURCE = CHAIN_WORK + 3,

	CHAIN_MOVE = CHAIN_WORK + 4,

	CHAIN_ATTACK = CHAIN_WORK << 1,
	
	CHAIN_FIRE = CHAIN_ATTACK + 1,
	
	CHAIN_AIM = CHAIN_ATTACK + 2,
	
	CHAIN_RELOAD = CHAIN_ATTACK + 3,

	CHAIN_RELOAD_INVENTORY = CHAIN_ATTACK + 4,

	CHAIN_FROZEN = CHAIN_ATTACK << 1,

	CHAIN_GIVE_RESOURCE = CHAIN_FROZEN << 1,

	CHAIN_PICK_ITEM = CHAIN_GIVE_RESOURCE + 1,

	CHAIN_TRANSITION = CHAIN_GIVE_RESOURCE << 1,

	CHAIN_TRANSPORT = CHAIN_TRANSITION << 1,

	CHAIN_LANDING = CHAIN_TRANSPORT + 1,
	CHAIN_UNLANDING = CHAIN_TRANSPORT + 2,
	
	CHAIN_OPEN_FOR_LANDING = CHAIN_TRANSPORT + 3,
	CHAIN_CLOSE_FOR_LANDING = CHAIN_TRANSPORT + 4,
	
	CHAIN_MOVE_TO_CARGO = CHAIN_TRANSPORT + 5,
	CHAIN_LAND_TO_LOAD = CHAIN_TRANSPORT + 6,

	CHAIN_TOUCH_DOWN = CHAIN_TRANSPORT + 7,

	CHAIN_PRODUCTION = CHAIN_TRANSPORT << 1,

	CHAIN_CONSTRUCTION = CHAIN_PRODUCTION << 1,

	CHAIN_UPGRADE = CHAIN_CONSTRUCTION + 1,

	CHAIN_DISCONNECT = CHAIN_CONSTRUCTION + 2,

	CHAIN_BE_BUILT = CHAIN_CONSTRUCTION << 1,

	CHAIN_IS_UPGRADED = CHAIN_BE_BUILT << 1,
	
	CHAIN_UPGRADED_FROM_BUILDING = CHAIN_IS_UPGRADED + 1,
	CHAIN_UPGRADED_FROM_LEGIONARY = CHAIN_IS_UPGRADED + 2,

	CHAIN_RISE = CHAIN_IS_UPGRADED << 1,

	CHAIN_FALL = CHAIN_RISE << 1,

	CHAIN_IN_TRANSPORT = CHAIN_FALL << 1,

	CHAIN_BIRTH = CHAIN_IN_TRANSPORT << 1,

	CHAIN_TELEPORTING = CHAIN_BIRTH + 1,

	CHAIN_OPEN = CHAIN_BIRTH + 2,
	CHAIN_CLOSE = CHAIN_BIRTH + 3,

	CHAIN_WEAPON_GRIP = CHAIN_BIRTH + 4,

	CHAIN_BIRTH_IN_AIR = CHAIN_BIRTH + 5,

	CHAIN_PAD_GET_SMTH = CHAIN_BIRTH + 6,
	CHAIN_PAD_PUT_SMTH = CHAIN_BIRTH + 7,
	CHAIN_PAD_CARRY = CHAIN_BIRTH + 8,
	CHAIN_PAD_ATTACK = CHAIN_BIRTH + 9,

	CHAIN_ITEM_BIRTH = CHAIN_BIRTH + 10,

	CHAIN_DEATH = CHAIN_BIRTH << 1,
	
	CHAIN_HOLOGRAM = CHAIN_DEATH + 1,

	CHAIN_TRIGGER = CHAIN_DEATH + 2,

	CHAIN_UNINSTALL = CHAIN_DEATH + 3
};

enum AnimationTerrainTypeID
{
	ANIMATION_TERRAIN_TYPE0 = TERRAIN_TYPE0,
	ANIMATION_TERRAIN_TYPE1 = TERRAIN_TYPE1,
	ANIMATION_TERRAIN_TYPE2 = TERRAIN_TYPE2,
	ANIMATION_TERRAIN_TYPE3 = TERRAIN_TYPE3,
	ANIMATION_TERRAIN_TYPE4 = TERRAIN_TYPE4,
	ANIMATION_TERRAIN_TYPE5 = TERRAIN_TYPE5,
	ANIMATION_TERRAIN_TYPE6 = TERRAIN_TYPE6,
	ANIMATION_TERRAIN_TYPE7 = TERRAIN_TYPE7,
	ANIMATION_TERRAIN_TYPE8 = TERRAIN_TYPE8,
	ANIMATION_TERRAIN_TYPE9 = TERRAIN_TYPE9,
	ANIMATION_TERRAIN_TYPE10 = TERRAIN_TYPE10,
	ANIMATION_TERRAIN_TYPE11 = TERRAIN_TYPE11,
	ANIMATION_TERRAIN_TYPE12 = TERRAIN_TYPE12,
	ANIMATION_TERRAIN_TYPE13 = TERRAIN_TYPE13,
	ANIMATION_TERRAIN_TYPE14 = TERRAIN_TYPE14,
	ANIMATION_TERRAIN_TYPE15 = TERRAIN_TYPE15,

	ANIMATION_ON_GROUND = ANIMATION_TERRAIN_TYPE0 | ANIMATION_TERRAIN_TYPE1 
		| ANIMATION_TERRAIN_TYPE2 | ANIMATION_TERRAIN_TYPE3 | ANIMATION_TERRAIN_TYPE4 
		| ANIMATION_TERRAIN_TYPE5 | ANIMATION_TERRAIN_TYPE6 | ANIMATION_TERRAIN_TYPE7 
		| ANIMATION_TERRAIN_TYPE8 | ANIMATION_TERRAIN_TYPE9 | ANIMATION_TERRAIN_TYPE10 
		| ANIMATION_TERRAIN_TYPE11	| ANIMATION_TERRAIN_TYPE12 | ANIMATION_TERRAIN_TYPE13 
		| ANIMATION_TERRAIN_TYPE14	| ANIMATION_TERRAIN_TYPE15,

	ANIMATION_ON_LOW_WATER = 1 << 16,
	ANIMATION_ON_WATER = 1 << 17,
	ANIMATION_ON_LAVA = 1 << 18,

	ANIMATION_ALL_SURFACES = ANIMATION_ON_GROUND | ANIMATION_ON_WATER | ANIMATION_ON_LOW_WATER | ANIMATION_ON_LAVA,
};

enum AnimationStateID
{
	ANIMATION_STATE_LEFT = 1 << 0,
	ANIMATION_STATE_RIGHT = 1 << 1,
	ANIMATION_STATE_FORWARD = 1 << 2,
	ANIMATION_STATE_BACKWARD = 1 << 3,

	ANIMATION_STATE_ALL_SIDES = ANIMATION_STATE_LEFT | ANIMATION_STATE_RIGHT | ANIMATION_STATE_FORWARD | ANIMATION_STATE_BACKWARD,

	ANIMATION_STATE_CRAWL = 1 << 4,
	ANIMATION_STATE_GRABBLE = 1 << 5,
	ANIMATION_STATE_WALK = 1 << 6,
	ANIMATION_STATE_RUN = 1 << 7,

	ANIMATION_STATE_ALL_POSE = ANIMATION_STATE_CRAWL | ANIMATION_STATE_GRABBLE | ANIMATION_STATE_WALK | ANIMATION_STATE_RUN,

	ANIMATION_STATE_STAND = 1 << 8,
	ANIMATION_STATE_MOVE = 1 << 9,
	ANIMATION_STATE_TURN = 1 << 10,
	ANIMATION_STATE_WAIT = 1 << 11,
	ANIMATION_STATE_ATTACK = 1 << 12,

	ANIMATION_STATE_ALL_MOVEMENTS = ANIMATION_STATE_STAND | ANIMATION_STATE_MOVE | ANIMATION_STATE_TURN | ANIMATION_STATE_WAIT | ANIMATION_STATE_ATTACK,

	ANIMATION_STATE_DEFAULT =  ANIMATION_STATE_ALL_POSE | ANIMATION_STATE_ALL_SIDES | ANIMATION_STATE_ALL_MOVEMENTS
};

enum MovementStateID
{
	MOVEMENT_STATE_LEFT = ANIMATION_STATE_LEFT,
	MOVEMENT_STATE_RIGHT = ANIMATION_STATE_RIGHT,
	MOVEMENT_STATE_FORWARD = ANIMATION_STATE_FORWARD,
	MOVEMENT_STATE_BACKWARD = ANIMATION_STATE_BACKWARD,

	MOVEMENT_STATE_ALL_SIDES = MOVEMENT_STATE_LEFT | MOVEMENT_STATE_RIGHT | MOVEMENT_STATE_FORWARD | MOVEMENT_STATE_BACKWARD,
	
	MOVEMENT_STATE_CRAWL = ANIMATION_STATE_CRAWL,
	MOVEMENT_STATE_GRABBLE = ANIMATION_STATE_GRABBLE,
	MOVEMENT_STATE_WALK = ANIMATION_STATE_WALK,
	MOVEMENT_STATE_RUN = ANIMATION_STATE_RUN,
	
	MOVEMENT_STATE_ALL_POSE = MOVEMENT_STATE_CRAWL | MOVEMENT_STATE_GRABBLE | MOVEMENT_STATE_WALK | MOVEMENT_STATE_RUN,

	MOVEMENT_STATE_STAND = ANIMATION_STATE_STAND,
	MOVEMENT_STATE_MOVE = ANIMATION_STATE_MOVE,
	MOVEMENT_STATE_TURN = ANIMATION_STATE_TURN,
	MOVEMENT_STATE_WAIT = ANIMATION_STATE_WAIT,

	MOVEMENT_STATE_ALL_MOVEMENTS = MOVEMENT_STATE_STAND | MOVEMENT_STATE_MOVE | MOVEMENT_STATE_TURN | MOVEMENT_STATE_WAIT,
	 
	MOVEMENT_STATE_ON_LOW_WATER = ANIMATION_ON_LOW_WATER,
	MOVEMENT_STATE_ON_WATER = ANIMATION_ON_WATER,
	MOVEMENT_STATE_ON_LAVA = ANIMATION_ON_LAVA,
	MOVEMENT_STATE_ON_GROUND = 1 << 20,

	MOVEMENT_STATE_ALL_SURFACES = MOVEMENT_STATE_ON_GROUND | MOVEMENT_STATE_ON_WATER | MOVEMENT_STATE_ON_LOW_WATER | MOVEMENT_STATE_ON_LAVA,
};

// Типы движений
enum MovementMode {
	MODE_CRAWL = ANIMATION_STATE_CRAWL,
	MODE_GRABBLE = ANIMATION_STATE_GRABBLE,
	MODE_WALK = ANIMATION_STATE_WALK,
	MODE_RUN = ANIMATION_STATE_RUN
};

enum ShootingOnMoveMode
{
	SHOOT_WHILE_IN_TRANSPORT	= 1,				///< может стрелять когда сидит в транспорте

	SHOOT_WHILE_LYING			= MODE_CRAWL >> 4,	///< может стрелять когда лежит
	SHOOT_WHILE_ON_ALL_FOURS	= MODE_GRABBLE >> 4,///< может стрелять когда присел на корточки
	SHOOT_WHILE_STANDING		= MODE_WALK >> 4,	///< может стрелять когда стоит

	SHOOT_WHILE_CRAWLING		= MODE_CRAWL,		///< может стрелять когда ползет
	SHOOT_WHILE_GRABBLING		= MODE_GRABBLE,		///< может стрелять когда идёт на корточках
	SHOOT_WHILE_MOVING			= MODE_WALK,		///< может стрелять когда идёт
	SHOOT_WHILE_RUNNING			= MODE_RUN			///< может стрелять когда бежит
};

class MovementState
{
public:
	typedef BitVector<AnimationStateID> AnimationState;
	typedef BitVector<AnimationTerrainTypeID> AnimationTerrainType;
	MovementState(int state = 0, int terrainType = 0);
	const AnimationState& state() const { return state_; }
	const AnimationTerrainType& terrainType() const { return terrainType_; }
	AnimationState& state() { return state_; }
	AnimationTerrainType& terrainType()  { return terrainType_; }
	bool operator == (const MovementState& state) const { return state_ == state.state_ && terrainType_ == state.terrainType_; }
	bool operator != (const MovementState& state) const { return state_ != state.state_ || terrainType_ != state.terrainType_; }
	bool operator < (const MovementState& state) const { return state_ < state.state_ || (state_ == state.state_ && terrainType_ < state.terrainType_);	}
	MovementState operator | (const MovementState& state) const;
	MovementState operator & (const MovementState& state) const;
	MovementState operator = (const BitVector<MovementStateID>& state);
	void serialize(Archive& ar);
	
private:
	AnimationState state_;
	AnimationTerrainType terrainType_;

public:
	static MovementState DEFAULT;
	static MovementState EMPTY;
};



class StringAnimatedWeaponList
{
public:
	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	WeaponAnimationType animationType() const { return weapon_; }
	const char* c_str() const { return name_; }

protected:
	ComboListString name_;
	WeaponAnimationType weapon_;

	void update();
};

struct AnimationChain
{
	string name_;
	ChainID chainID;
	MovementState movementState;
	AbnormalStateTypeReferences abnormalStateTypes;
	int abnormalStateMask;
	int counter;
	int possibility;
	MovementState transitionToState;
	StringAnimatedWeaponList weapon;

	int animationAcceleration;
	int period; // mS
	bool cycled; 
	bool supportedByLogic;
	bool reversed; 
	bool syncBySound; 
	bool randomPhase; 
	bool stopPermanentEffects;
	
	/// коэффициент для радиуса слышимости
	float noiseRadiusFactor; 

	EffectAttributes effects;

	SoundReferences soundReferences;
	SoundMarkers soundMarkers;

	AnimationChain();

	const char* name() const;
	const char* groupName() const { return weapon.animationType().c_str(); }

	int abnormalStatePriority() const;
	bool checkAbnormalState(const AbnormalStateType* astate) const;
	bool compare(const AnimationChain& rhs, MovementState mstate) const;

	void serialize(Archive& ar);

	int chainIndex() const { return chainIndex_; }
	int animationGroup() const { return animationGroup_; }
	VisibilityGroupIndex visibilityGroup() const { return VisibilityGroupIndex(visibilityGroup_); }

	int editorIndex() const{ return editorIndex_; }
	void setEditorIndex(int editorIndex){ editorIndex_ = editorIndex; }
private:
	ChainName chainIndex_;
	AnimationGroupName animationGroup_;
	VisibilityGroupName visibilityGroup_;
	int editorIndex_;
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
	float triggerDelayFactor; // Коэффициент триггера задержка

	int	orderBuildingsDelay;
	int orderUnitsDelay;
	int orderParametersDelay;
	int upgradeUnitDelay;

	LocString locName;
	const wchar_t* name() const { return locName.c_str(); }

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
	ParameterCustom necessaryParameters;
	bool canFire;
	bool destroyWithTransport;
	bool visible;
	bool requiredForMovement;
	bool dockWhenLanding;
	Logic3dxNode node;

	TransportSlot();
	void serialize(Archive& ar);
	bool check(UnitFormationTypeReference type, const ParameterSet& parameters) const;
};

typedef vector<TransportSlot> TransportSlots;

////////////////////////////////////////

enum FOWVisibleMode
{
	FVM_ALLWAYS,
	FVM_HISTORY_TRACK,
	FVM_NO_FOG,
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
	bool accurateBound;
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
	ParameterTypeReferenceZero unitNumberMaxType;
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
	bool showSelectRadius;

	CircleManagerParam fireRadiusCircle;
	CircleManagerParam fireMinRadiusCircle;
	CircleManagerParam fireDispRadiusCircle;
	CircleManagerParam signRadiusCircle;
	CircleManagerParam noiseRadiusCircle;
	CircleManagerParam hearingRadiusCircle;
	
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

	WeaponSlotAttributes weaponAttributes;

	bool hasAutomaticAttackMode;
	AttackModeAttribute attackModeAttribute;

	EffectAttributeAttachable noiseTargetEffect;

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

	bool enableVerticalFactor;
	bool useLocalPTStopRadius;
	float localPTStopRadius;

	float getPTStopRadius() const;

	float sightRadiusNightFactor;
	float sightFogOfWarFactor;
	float sightSector;
	int sightSectorColorIndex;
	bool showSightSector;

	float forwardVelocity;
	float forwardVelocityRunFactor;
	float sideMoveVelocityFactor;
	float backMoveVelocityFactor;
	bool upgradeStop;
	float velocityFactorsByTerrain[TERRAIN_TYPES_NUMBER];
	int impassability;
	float flyingHeight;
	float flyingHeightDeltaFactor;
	float flyingHeightDeltaPeriod;
	float additionalHorizontalRot;
	float waterLevel;
	bool realSuspension;
	WheelDescriptorList wheelList;
	RigidBodyCarPrm rigidBodyCarPrm;
	bool sinkInLava;
	float moveSinkVelocity;
	float standSinkVelocity;
	
	AttributeItemReferences leavingItems;
	bool leavingItemsRandom;
	
	enum UnitUI_StateType {
		UI_STATE_TYPE_NORMAL = 0,
		UI_STATE_TYPE_SELECTED,
		UI_STATE_TYPE_WAITING
	};
	
	typedef EnumTable<UnitUI_StateType, UI_ShowModeUnitSpriteReference> SelectSprites;
	SelectSprites selectSprites_;

	typedef vector<UI_ShowModeUnitSpriteReference> UI_ShowModeUnitSpriteReferences;
	UI_ShowModeUnitSpriteReferences ui_faces_;
	
	/// обозначение юнита на миникарте
	UI_MinimapSymbolType minimapSymbolType_;
	UI_MinimapSymbol minimapSymbol_;
	UI_MinimapSymbol minimapSymbolWaiting_;
	UI_MinimapSymbol minimapSymbolSpecial_;
	bool hasPermanentSymbol_;
	bool showUpgradeEvent_;
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

	//typedef std::vector<TraceInfo> TraceInfos;
	typedef std::vector<TerToolCtrl> TraceInfos;
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
	bool rotToNoiseTarget;
	bool ptSideMove;
	bool ptBoundCheck;

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
	AttributeItemInventoryReferences equipment;
	bool dropInventoryItems;

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
		ParameterCustom accessValue;
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

	bool canBeTransparent;
	FOWVisibleMode fow_mode;

	int chainTransitionTime;
	int chainLandingTime;
	int chainUnlandingTime;
	int chainGiveResourceTime;
	int chainTouchDownTime;
	int chainPickItemTime;
	int chainRiseTime;
	int chainChangeMovementMode;
	int chainOpenTime;
	int chainCloseTime;
	int chainBirthTime;
	int chainOpenForLandingTime;
	int chainCloseForLandingTime;
	int chainFlyDownTime;
	int chainUpgradedFromBuilding;
	int chainUpgradedFromLegionary;
	int chainUninsatalTime;
	int chainDisconnectTime;
	bool killAfterDisconnect;
	// невидимый
	bool invisible;
	bool canChangeVisibility;
	UnitColor transparenceDiffuseForAlien;
	UnitColor transparenceDiffuseForClan;

	VisibilitySetName nightVisibilitySet;
	VisibilityGroupOfSet dayVisibilityGroup;
	VisibilityGroupOfSet nightVisibilityGroup;

    bool defaultDirectControlEnabled;
	bool syndicateControlAimEnabled;
	bool disablePathTrackingInSyndicateControl;
	float syndicatControlCameraRestrictionFactor;
	Vect3f syndicateControlOffset;
	Object3dxNode directControlNode;
	Vect3f directControlOffset;
	Vect3f directControlOffsetWater;
	float directControlThetaMin;
	float directControlThetaMax;

	enum SelectSpriteTypes
	{
		ORDINARY,
		UNPOWERED
	};
	
	struct SelectSprite {
		SelectSprite() : showSelectSpritesForOthers(true), ownSelectSpritesForOthers(false) {}
		void serialize(Archive& ar);

		UI_UnitSprite selectSpriteNormal;
		UI_UnitSprite selectSpriteHover;
		UI_UnitSprite selectSpriteSelected;

		bool showSelectSpritesForOthers;
		bool ownSelectSpritesForOthers;
		UI_UnitSprite unitSpriteForOthers;
		UI_UnitSprite unitSpriteForOthersHovered;
	};

	typedef EnumTable<SelectSpriteTypes, SelectSprite> SelectHoverSprites;
	SelectHoverSprites selectSprites;

	bool showSpriteForUnvisible;
	bool selectBySprite;

	bool useOffscreenSprites;

	UI_SpriteReference offscreenSprite;
	UI_SpriteReference offscreenSpriteForEnemy;
	
	UI_SpriteReference offscreenMultiSprite;
	UI_SpriteReference offscreenMultiSpriteForEnemy;

	enum ObjectLodPredefinedType lodDistance;

	//---------------------------------------
	AttributeBase();
	virtual ~AttributeBase(){}

	void preload() const;

	UnitClass unitClass() const { return unitClass_; }

	AttributeType attributeType() const;

	bool isBuilding() const { return unitClass() == UNIT_CLASS_BUILDING; }
	bool isLegionary() const { return unitClass() == UNIT_CLASS_LEGIONARY; }
	bool isPad() const { return unitClass() == UNIT_CLASS_PAD; }
	bool isPlayer() const { return unitClass() == UNIT_CLASS_PLAYER; }
	bool isResourceItem() const { return unitClass() == UNIT_CLASS_ITEM_RESOURCE; }
	bool isInventoryItem() const { return unitClass() == UNIT_CLASS_ITEM_INVENTORY; }
	bool isSquad() const { return unitClass() == UNIT_CLASS_SQUAD; }
	bool isProjectile() const { return unitClass() == UNIT_CLASS_PROJECTILE || unitClass() == UNIT_CLASS_PROJECTILE_BULLET || unitClass() == UNIT_CLASS_PROJECTILE_MISSILE; }
	bool isEnvironmentBuilding() const { return unitClass() == UNIT_CLASS_ENVIRONMENT; }
	bool isEnvironmentSimple() const { return unitClass() == UNIT_CLASS_ENVIRONMENT_SIMPLE; }
	bool isEnvironment() const { return isEnvironmentBuilding() || isEnvironmentSimple(); }
	bool isActing() const { return isBuilding() || isLegionary() || isPad() || isPlayer(); } 
	bool isObjective() const { return isActing() || isResourceItem() || isInventoryItem(); } 
	bool isReal() const { return isObjective() || isProjectile(); }
	
	bool isTransport() const { return !transportSlots.empty(); }

	bool hasProdused() const { return !producedUnits.empty() || !producedParameters.empty(); }

	virtual float radius() const;

	void initGeometryAttribute();
	void refreshChains();
	virtual bool isChainNecessary(ChainID chainID) const { return true; }

	const AnimationChain* animationChain(ChainID chainID, int counter = -1, const AbnormalStateType* astate = 0, MovementState movementState = MovementState::DEFAULT, WeaponAnimationType weapon = WeaponAnimationType("")) const; // По умолчанию - случайная
	const AnimationChain* animationChainByFactor(ChainID chainID, float factor, const AbnormalStateType* astate = 0, MovementState movementState = MovementState::DEFAULT) const;
	const AnimationChain* animationChainTransition(float factor, const AbnormalStateType* astate, MovementState stateFrom, MovementState stateTo, WeaponAnimationType weapon = WeaponAnimationType("")) const;

	int animationChainIndex(const AnimationChain* chain) const;
	const AnimationChain* animationChainByIndex(int index) const;
	
	bool canProduce(const AttributeBase* attribute) const;
	int productionNumber(const AttributeBase* attribute) const;

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
	static const AttributeBase* currentAttribute() { return currentAttribute_; }

	static bool createModel(const char* model_name);
	static void releaseModel();
	static void setModel(cObject3dx* model, cObject3dx* logicModel);
	static void setCurrentLibraryKey(const char* currentLibraryKey) { currentLibraryKey_ = currentLibraryKey; }
	static void setCurrentAttribute(const AttributeBase* currentAttribute) { currentAttribute_ = currentAttribute; }

	const wchar_t* interfaceName(int num) const { xassert(num >= 0); return num < interfaceName_.size() ? interfaceName_[num].c_str() : L""; }
	const wchar_t* interfaceDescription(int num) const { xassert(num >= 0); return num < interfaceDescription_.size() ? interfaceDescription_[num].c_str() : L""; }
	const UI_ShowModeSprite* getSelectSprite(UnitUI_StateType type = UI_STATE_TYPE_NORMAL) const;
	const UI_ShowModeSprite* getUnitFace(int num) const { xassert(num >= 0); return num < ui_faces_.size() ? &*ui_faces_[num] : 0; }

protected:
	UnitClass unitClass_;

	static float producedPlacementZoneRadiusMax_;

	static cScene* scene_;
	static cObject3dx* model_;
	static cObject3dx* logicModel_;
	static const char* currentLibraryKey_;
	static const AttributeBase* currentAttribute_;

	LocStrings interfaceName_;
	LocStrings interfaceDescription_;

private:
	FileTime modelTime_;
	string libraryKey_;
	
	AbnormalStateAttribute waterEffect; // Прокеширован в AttributeCache, брать оттуда
	AbnormalStateAttribute lavaEffect; // Прокеширован в AttributeCache, брать оттуда
	AbnormalStateAttribute iceEffect; // Прокеширован в AttributeCache, брать оттуда
	AbnormalStateAttribute earthEffect; // Прокеширован в AttributeCache, брать оттуда

	AnimationChainsInterval findAnimationChainInterval(ChainID chainID, const AbnormalStateType* astate, MovementState movementState, WeaponAnimationType weapon = WeaponAnimationType()) const;
	AnimationChainsInterval findTransitionChainInterval(const AbnormalStateType* astate, MovementState stateFrom, MovementState stateTo, WeaponAnimationType weapon) const;

	class RigidBodyModelPrmBuilder
	{
	public:
		RigidBodyModelPrmBuilder(RigidBodyModelPrm& modelPrm, BodyPartAttributes& bodyParts, cObject3dx* model);

	private:
		RigidBodyModelPrm::const_iterator findParentBody(int index) const;

		RigidBodyModelPrm& modelPrm_;
		cObject3dx* model_;
	};

protected:
	UI_CursorReference selectionCursor_;
	// Не сериализуется. Нужен, чтобы можно было менять курсор во время выполнения
	// без опасения запортить библиотеку
	const UI_Cursor* selectionCursorProxy_;

	friend class AttributeCache;
};

#endif
