#ifndef __ACTIONS_H__
#define __ACTIONS_H__

#include "timers.h"
#include "..\TriggerEditor\TriggerExport.h"
#include "..\Units\UnitAttribute.h"
#include "..\Units\Triggers.h"
#include "..\Units\AttributeSquad.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_NetCenter.h"
#include "..\UserInterface\Bubles\Blobs.h"
#include "..\Game\Player.h"
#include "..\Game\SoundApp.h"
#include "..\Sound\Sound.h"
#include "..\Units\DirectControlMode.h"

class MpegSound;
class UnitCommand;
class UnitReal;
class UnitActing;
class UnitBuilding;
class PlaceScanOp;
class RadiusScanOp;
class WeaponScanOp;
class CameraSpline;
class Anchor;
//---------------------------------
// Действия					
//---------------------------------

struct ActionForAI : Action 
{
	bool onlyIfAi; 

	ActionForAI();
	bool automaticCondition() const;
	void serialize(Archive& ar);

	mutable DurationTimer difficultyTimer;
};

class ActionContext : public ActionForAI
{
public:
	bool isContext() const { return true; }
	void setContextUnit(UnitActing* unit) { contextUnit_ = unit; }
	bool checkContextUnit(UnitActing* unit) const;
	void serialize(Archive& ar);
	
protected:
	AttributeUnitOrBuildingReference object_;
	UnitLink<UnitActing> contextUnit_;
	friend Trigger;
};

class ActionContextSquad : public ActionContext
{
public:
	bool checkContextUnit(UnitActing* unit) const;
	void setContextUnit(UnitActing* unit);
	void serialize(Archive& ar);

protected:
	AttributeSquadReference squad_;
	UnitLink<UnitSquad> contextSquad_;
	friend Trigger;
};

struct ActionDelay : Action // Задержка времени
{
	float duration; 
	bool showTimer; 
	bool scaleByDifficulty; 
	bool useNonStopTimer;
	bool randomTime;
	DurationTimer timer;											 
	DurationNonStopTimer nonStopTimer;

	ActionDelay();

	void activate();
	bool workedOut();

	void serialize(Archive& ar);
};

class ActionCreateUnit : public Action
{
public:
	ActionCreateUnit() : anchorLabel_("")
	{
		player_ = -1;
		count_ = 1;
		inTheSameSquad_ = false;
	}

	void serialize(Archive& ar);
	void activate();

private:
	AttributeReference attr_;
	ComboListString anchorLabel_;
	string unitLabel_;
	int count_; 
	int player_;
	bool inTheSameSquad_;
};

class ActionSetCameraRestriction : public Action // ограничения камеры
{
public:
	ActionSetCameraRestriction();
	void serialize(Archive& ar);
	void activate();
private:
	SwitchModeTriple switchMode_;
};

struct ActionSetCamera : Action // Установка Камеры
{
	CameraSplineName cameraSplineName;
	bool smoothTransition; 
	int cycles; 
	const CameraSpline* spline_;

	ActionSetCamera();
	void activate();
	bool workedOut();
	void serialize(Archive& ar);
};

struct ActionSetDefaultCamera : Action // Установка Камеры
{
	float duration_;

	ActionSetDefaultCamera() : duration_(1000) {}
	void activate();
	void serialize(Archive& ar);

};

class ActionSquadMoveToObject : public ActionContextSquad
{
public: 
	ActionSquadMoveToObject();

	bool checkContextUnit(UnitActing* unit) const;
	void serialize(Archive& ar);
	void activate();

private:
	UnitActing* findNearestObject(UnitActing* unit) const;
	UnitActing* nearestUnit(const Vect2f& pos, const RealUnits& unitList) const;
	AttributeUnitOrBuildingReference attrObject_;

	AIPlayerType aiPlayerType_;
	mutable UnitLink<UnitReal> unit_;
};

class ActionSquadMoveToItem : public ActionContextSquad
{
public: 
	ActionSquadMoveToItem();

	bool checkContextUnit(UnitActing* unit) const;
	void serialize(Archive& ar);
	void activate();

private:
	UnitReal* findNearestItem(UnitReal* unit) const;
	AttributeItemReference attrItem;

	mutable UnitLink<UnitReal> unit_;
};

struct ActionOscillateCamera : Action // Тряска Камеры
{
	int duration; 
	float factor; 

	ActionOscillateCamera() {
		duration = 30; 
		factor = 1; 
	}

	void activate();
	void serialize(Archive& ar);
};


//-------------------------------------
struct ActionExitFromMission : Action 
{
	void activate();
};

//-------------------------------------

class ActionSaveAuto : public Action
{
public:
	ActionSaveAuto();

	void activate();
	void serialize(Archive& ar);
	bool automaticCondition() const;
private:
	string name_;
};

struct ActionSave : Action // Сохранить игру
{
	void activate();
	void serialize(Archive& ar);
};

//-------------------------------------

struct ActionResetNetCenter : Action 
{
	void activate();
};

struct ActionKillNetCenter : Action 
{
	void activate();
};

class ActionContinueConstruction : public ActionContext
{
public:
	ActionContinueConstruction();

	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);

private:
	mutable UnitLink<UnitReal> unit_;
	MovementMode mode_;
};

//-------------------------------------
class ActionOrderBuildingsOnZone : public ActionForAI
{
public:
    AttributeBuildingReference attrBuilding;

	ActionOrderBuildingsOnZone();

	bool automaticCondition() const;
	void activate(); 
	
	void serialize(Archive& ar);

	void clear();
protected:

	mutable UnitLink<UnitLegionary> builder_;
	mutable int indexScan;
	mutable int scanStep_;

	mutable bool found_;
	mutable Vect2f foundPosition_; 

	mutable Vect2i placement_coords;
	mutable Vect2i scanMin_, scanMax_; // world's scale

	// сканирующий код для поиска места установки здания и связанные с операцией переменные
	enum BuilderState
	{	
		BuildingIdle, 
		BuildingPause, 
		FindingWhereToBuild, 
		FoundWhereToBuild, 
		UnableToFindWhereToBuild 
	};
		
	mutable BuilderState builder_state;
	
	mutable Vect2f best_position;

	mutable DurationTimer building_pause;
	mutable bool startDelay;

	void placeBuildingSpecial() const;
	void startPlace(const Vect2i& scanMin, const Vect2i& scanMax, int scanStep) const;
	void findWhereToBuildQuantSpecial() const;
	void BuildingQuant() const;
	void finishPlacement();

	bool checkBuildingPosition(const AttributeBuildingReference attr, const Vect2f& position, bool checkUnits, Vect2f& snapPosition) const; 
};

class ActionOrderBuildings : public ActionContext
{
public:
    AttributeBuildingReference attrBuilding;

	ActionOrderBuildings();

	bool checkContextUnit(UnitActing* unit) const;
	void activate(); 
	
	void serialize(Archive& ar);

protected:

	mutable UnitLink<UnitLegionary> builder_;
	float radius_;
	float extraDistance_;
	bool closeToEnemy_;

	mutable bool found_;
	mutable Vect2f best_position;

	mutable bool startDelay;

	void finishPlacement();
};

//-------------------------------------
class ActionPickResource : public ActionContextSquad
{
public:
	ActionPickResource();

	bool automaticCondition() const;
	void activate();
	bool checkContextUnit(UnitActing* unit) const;

	void serialize(Archive& ar);

private: 
	ParameterTypeReference parameterType_;

	UnitReal* findNearestResource(UnitReal* unit) const;
	mutable UnitLink<UnitReal> unit_;
};

class ActionSetUnitAttackMode : public ActionContextSquad
{
public:
	bool checkContextUnit(UnitActing* unit) const;
	
	void serialize(Archive& ar);
	void activate(); 
	
private:
	AttackMode attackMode_;
};

class ActionUpgradeUnit : public ActionContext
{
public:
	mutable UpgradeOption upgradeOption;

	ActionUpgradeUnit();
	~ActionUpgradeUnit();

	void serialize(Archive& ar);

	bool checkContextUnit(UnitActing* unit) const;
	bool workedOut();

	bool automaticCondition() const;

	void clear();
private:
	AttributeUnitOrBuildingReferences objects_;
	mutable DurationTimer timeToUpgrade;
	mutable UnitLink<UnitReal> unit_;
	RadiusScanOp* scanOp;
	float extraRadius;
	int upgradeNumber;
	float factorRadius;
	float angle;
	mutable bool firstTime;
	bool interrupt_;
	void positionFound(UnitReal* unit);

	void deleteScanOp();

	// for gun-building
	Vect2f aim_;

	Vect2f normal(const Vect2f& p0, const Vect2f& p1);
	Vect2f getPointInSection(const Vect2f& p0, const Vect2f& p1, float alfa); // 0<=alfa<=1 чтобы принадлежала отрезку
	Vect2f getPointOnDistance(const Vect2f& guide, const Vect2f& point, float distance); 

};

class ConvexHull
{
public:
    typedef std::vector<Vect2f> Polygon;

	ConvexHull(const Polygon& points); // по точкам строит выпуклую оболочку

	const Polygon& getPolygon() { return polygon_; }

private:
	Polygon polygon_;
	Vect2f downRightPoint_;
	bool compareCriteria(Polygon::value_type point1, Polygon::value_type point2);

	 //    Return: >0 for P2 left of the line through P0 and P1
	 //            =0 for P2 on the line
	 //            <0 for P2 right of the line
	int isLeft(const Vect2f& p0, const Vect2f& p1, const Vect2f& p2 ); 
};

class ActionOrderParameters : public ActionForAI
{
public:
	AttributeUnitOrBuildingReference attrFactory;
	int numberOfParameter;
	int countParameters;

	ActionOrderParameters();

	void activate();
	void serialize(Archive& ar);
	bool workedOut();
	bool automaticCondition() const;

	void clear();
private:
	UnitActing* findFreeFactory(const AttributeBase* factoryAttribute) const;

	typedef vector<UnitActing*> FreeFactories;
	FreeFactories freeFactories;
	int inQuery;
	int maxProduce;
};

class ActionOutUnitFromTransport : public ActionContext
{
public:
	void activate();
	void serialize(Archive& ar);
};

class ActionPutUnitInTransport : public ActionContext
{
public:
	ActionPutUnitInTransport()
	{
		distance = 200;
	}

	bool checkContextUnit(UnitActing* unit) const;
	bool automaticCondition() const;
	void activate();
	void serialize(Archive& ar);
private:
	AttributeUnitOrBuildingReference attrTransport;
	mutable UnitLink<UnitReal> unitTransport;
	float distance;
};

class ActionOrderUnits : public ActionForAI
{
public:
	AttributeUnitReference attrUnit;
	int countUnits;

	ActionOrderUnits();

	void activate();
	void serialize(Archive& ar);
	bool workedOut();
	bool automaticCondition() const;

	void clear();
private:
	typedef vector<UnitActing*> FreeFactories;
	FreeFactories freeFactories;
	int inQuery;
	int unitNumber;
	int maxProduce;
	bool checkProduceParameters(const AttributeBase* producedUnit) const;
};

class ActionSellBuilding : public ActionContext
{
public:
	void activate();
};

//-------------------------------------

struct ActionAttackMyUnit : ActionContextSquad 
{
	ActionAttackMyUnit();
	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);
	
private:

	UnitReal* findTarget(UnitReal* unit, const RealUnits& units, float minDist) const;

	AttackMode attackMode_;
	AttributeUnitOrBuildingReferences attrTargets_;
	float maxDistance_;
	int attackTime_;
	mutable UnitLink<UnitReal> target_;

	bool attackEnemy_;
};

struct ActionAttack : ActionContextSquad 
{
	enum AimObject {
		AIM_UNIT,
		AIM_BUILDING,
		AIM_ANY
	};

	ActionAttack();
	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);
	
private:
	AttackMode attackMode_;
	AimObject aimObject_;
	AttributeUnitOrBuildingReferences attrPriorityTarget_;
	AttributeUnitOrBuildingReference attrTarget_;
	int attackTime_;
	mutable UnitLink<UnitReal> target_;

	bool attackEnemy_;
};

struct ActionGuardUnit : ActionContextSquad 
{
	ActionGuardUnit();
	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);
	
private:

	UnitReal* findTarget(UnitReal* unit, const RealUnits& realUnits, float maxDist) const;

	AttackMode attackMode_;
	AttributeUnitOrBuildingReferences attrTargets_;
	int attackTime_;
	mutable UnitLink<UnitReal> target_;
	float maxDistance_;
};


class ActionFollowSquad : public ActionContextSquad
{
public:
	ActionFollowSquad();

	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);
	bool automaticCondition() const;
	UnitSquad* findSquad() const;
	
private:
	AttributeSquadReference squadToFollow_;
	float radius_;
	mutable UnitLink<UnitSquad> squad_;
};

class ActionSplitSquad : public ActionContextSquad
{
public:
	bool automaticCondition() const;
	void activate();
};

class ActionJoinSquads : public ActionContextSquad
{
public:
	ActionJoinSquads();

	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	bool automaticCondition() const;
	void serialize(Archive& ar);

private:
	float radius_;
	AttributeUnitReferences attrUnits_;
};

class ActionSetHotKey : public Action
{
public:
	
	void activate();
	void serialize(Archive& ar);

private:
	sKey hotKey_;
	UI_ControlReference controlReference_;
};

class ActionEscapeWater : public ActionContextSquad
{
public:
	ActionEscapeWater() { nearBase_ = 250.f; }
	bool automaticCondition() const;
	void activate();
	void serialize(Archive& ar);

private:
	float nearBase_;
};

class ActionSetWalkMode : public ActionContextSquad
{
public:
	ActionSetWalkMode();
	void serialize(Archive& ar);
	bool checkContextUnit(UnitActing* unit) const;
	void activate();

private:
	MovementMode mode_;
};

class ActionReturnToBase : public ActionContextSquad
{
public:
	MovementMode mode_;

	ActionReturnToBase();
	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);

private:
	AttackMode attackMode_;
	AttributeBuildingReferences headquaters_;
	float radius_;
	mutable UnitLink<UnitReal> headquater;
	int returnTime_;

	UnitReal* findNearestHeadquater(const Vect2f& positionSquad) const;
};

class ActionRestartTriggers : public Action
{
public:
	ActionRestartTriggers();
	AuxType auxType() const { return resetToStart_ ? RESET_TO_START : RESET_TO_CURRENT; }
	void serialize(Archive& ar);

private:
	bool resetToStart_;
};

class ActionSwitchPlayer : public Action
{
public:
	ActionSwitchPlayer();
	void activate();
	void serialize(Archive& ar);

private:
	int playerID;
};

class ActionSetPlayerWin : public Action
{
public:
	void activate() { aiPlayer().setWin(); }
};

class ActionSetPlayerDefeat : public Action
{
public:
	void activate() { aiPlayer().setDefeat(); }
};

class ActionExploreArea : public ActionContextSquad
{
public:	
	int exploringRadius;
	AttributeBuildingReference mainBuilding;

	ActionExploreArea();
	bool checkContextUnit(UnitActing* unit) const;
	bool automaticCondition() const;
	void serialize(Archive& ar);
	void activate();
private:
	float curAngle;
	float curRadius;
};

class ActionSquadMoveToAnchor : public ActionContextSquad
{
public:
	ActionSquadMoveToAnchor();
	void activate();
	void serialize(Archive& ar);
	bool workedOut();
	bool checkContextUnit(UnitActing* unit) const;

	void clear();
private:
	SquadMoveMode mode;
	ComboListString label;
	LegionariesLinks unitList;
	mutable Anchor* anchor;

};

class ActionPutSquadToAnchor : public ActionContextSquad
{
public:
	ActionPutSquadToAnchor();
	void serialize(Archive& ar);
	bool checkContextUnit(UnitActing* unit) const;
	void activate();

private:
	ComboListString label;
	mutable Anchor* anchor;
};

struct ActionSquadMove : ActionForAI // Послать сквад в точку объекта по метке
{
	SquadMoveMode mode;

	AttributeSquadReference attrSquad;
	ComboListString label; 

	ActionSquadMove();
	bool automaticCondition() const;
	void activate();
	void serialize(Archive& ar);
	bool workedOut();

	void clear();
private:
	LegionariesLinks unitList;
	UnitLink<UnitReal> unit;
};

class ActionSwitchTriggers : public Action
{
public:
	enum Mode {
		SWITCH_ON_CURRENT_PLAYER_AI,
		SWITCH_OFF_CURRENT_PLAYER_AI,
		SWITCH_OFF_ALL_PLAYERS_AI,
		SWITCH_OFF_CURRENT_PLAYER_TRIGGERS
	};
	Mode mode;

	ActionSwitchTriggers();

	void activate();
	void serialize(Archive& ar);
};

class ActionAttackLabel : public ActionContext
{
public:
	ActionAttackLabel();
	void activate();
	void serialize(Archive& ar);
	bool workedOut();
	bool checkContextUnit(UnitActing* unit) const;

	void clear();
private:
	ComboListString label_;
	WeaponPrmReference weapon_;
	int timeToAttack_;
	DurationTimer durationTimer_;
	Vect3f position_; 
	bool firstTime;
};

class ActionActivateSpecialWeapon : public ActionContext
{
public:
	ActionActivateSpecialWeapon();

	void serialize(Archive& ar);
	bool automaticCondition() const;
	void activate();

private:
	WeaponPrmReferences weaponref_;
	SwitchMode mode_;
};

struct ActionAttackBySpecialWeapon : ActionContext // Атаковать спецоружием
{
	AttributeReferences unitsToAttack; 
	AttackCondition attackCondition;
	WeaponPrmReferences weaponref_;
	
	ActionAttackBySpecialWeapon();
	~ActionAttackBySpecialWeapon();

	bool automaticCondition() const;
	bool checkContextUnit(UnitActing* unit) const;

	void activate();
	bool workedOut();

	void serialize(Archive& ar);

	void clear();
	
private:
	mutable Vect2f firePosition_;
	mutable UnitLink<UnitBase> targetUnit_;
	mutable UnitLink<UnitBase> virtualUnit;
	mutable float startAngle; 
	mutable float angle;
	float anglex;
	mutable	float factorRadius;
	float timeToAttack_;
	float minDistance;
	mutable int ID;
	DurationTimer resetTime_; 
	DurationTimer durationTimer_;
	float dist;
	bool passAbility;
	mutable bool firstTime;
};

struct ActionActivateObjectByLabel : Action // Активировать объект по метке
{
	ComboListString label;

	ActionActivateObjectByLabel() : active_(true) {}

	void activate();
	void serialize(Archive& ar);

protected:
	bool active_;
};

struct ActionDeactivateObjectByLabel : ActionActivateObjectByLabel // Деактивировать объект по метке
{
	ActionDeactivateObjectByLabel(){
		active_ = false;
	}
};

struct ActionSetControlEnabled : Action // Запретить/разрешить управление игрока
{
	bool controlEnabled; 

	ActionSetControlEnabled() {
		controlEnabled = false; 
	}

	void activate();
	void serialize(Archive& ar);
};

struct ActionSetFreezedByTrigger : Action 
{
	bool freeze; 

	ActionSetFreezedByTrigger() {
		freeze = true; 
	}

	void activate();
	void serialize(Archive& ar);
};

//-------------------------------------
class ShowHeadData 
{
public:
	ShowHeadData(bool camera);

	const char* model() const { return model_.c_str(); }
	const char* chain() const { return chain_.c_str(); }

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

private:
	bool camera_;
	ComboListString modelChain_;
	string model_;
	string chain_;
};

class ActionShowHead : public Action 
{
public:
	ActionShowHead();

	bool automaticCondition() const;
	void activate();
	bool workedOut() { return !timer_(); }
	void serialize(Archive& ar);

private:

	ShowHeadData mainChain_;
	ShowHeadData silenceChain_;
	ShowHeadData cameraChain_;

	UI_MessageSetup messageSetup_;

	int duration_;
	DurationTimer timer_;											 
	bool cycled_;
	bool enable_;
	sColor4c skinColor_;
};

class ActionEnableSounds : public Action
{
public:
	enum SoundType {
		TYPE_SOUND,
		TYPE_VOICE,
		TYPE_MUSIC
	};

	ActionEnableSounds();
	void activate();
	void serialize(Archive& ar);

private:
	SoundType soundType_;
	SwitchModeTriple switchMode_; 
};

class ActionEnableMessage : public Action
{
public:
	ActionEnableMessage();
	void activate();
	void serialize(Archive& ar);

private:
	SwitchMode mode_;
	UI_MessageTypeReference messageType_;
};

class ActionInterrruptMessage : public Action
{
public:
	void serialize(Archive& ar);
	bool automaticCondition() const;
	void activate();

private:
	UI_MessageTypeReference messageType_;
};

class ActionInterruptAnimation : public Action
{
public:
	void serialize(Archive& ar);
	bool automaticCondition() const;
	void activate();

private:
	UI_MessageTypeReference messageType_;
};

class ActionMessage : public Action // Cообщение
{
public:
	enum Type {
		MESSAGE_ADD,
		MESSAGE_REMOVE
	};

	ActionMessage();
	~ActionMessage();

	void activate();
	bool workedOut();
	bool automaticCondition() const;

	void serialize(Archive& ar);

	void interrupt() { mustInterrupt_ = true; } 
	const UI_MessageSetup& messageSetup() const { return messageSetup_; } 

private:

	UI_MessageSetup messageSetup_;

	float delay_; 
	float pause_;
	float fadeTime_;

	Type type_;

	DurationTimer delayTimer_;
	DurationTimer pauseTimer_;
	DurationTimer durationTimer_;

	bool started_;
	bool mustInterrupt_;
};

class ActionTask : public Action // Задача
{
public:
	ActionTask();

	void activate();
	bool workedOut();
	void serialize(Archive& ar);
	bool automaticCondition() const;

private:

	UI_TaskStateID state_;
	UI_MessageSetup messageSetup_;

	/// второстепенные задачи отображаются другим цветом
	bool isSecondary_;

	DurationTimer durationTimer_;
};

class ActionSetGamePause : public Action
{
public:
	ActionSetGamePause();

	void activate();
	void serialize(Archive& ar);

protected:
	SwitchMode switchType;
};

class ActionSetCameraAtSquad : public Action // Установить камеру на сквад (с возможностью слежения)
{
public:
	DurationTimer timer;
	AttributeSquadReference attrSquad;

	ActionSetCameraAtSquad();

	void activate();
	void serialize(Archive& ar);
	bool workedOut();
	bool automaticCondition() const;
private:
	int transitionTime;	
	UnitSquad* findSquad(const AttributeBase* attr, const Vect2f& nearPosition, float distanceMin) const;
};

class ActionSetDirectControl : public Action
{
public:
    AttributeUnitReference attr;
	ActionSetDirectControl();
	bool automaticCondition() const;
	void activate();
	void serialize(Archive& ar);

private: 
	DirectControlMode controlMode_; 
	mutable UnitReal* unit;
};

class ActionSetCameraFromObject : public Action
{
public:
	void activate();
};

struct ActionSetCameraAtObject : ActionContext // Установить камеру на объект
{
	int transitionTime; 
	bool setFollow;
	int turnTime; 
	CameraSplineName cameraSplineName;
	const CameraSpline* spline_;

	ActionSetCameraAtObject() {
		transitionTime = 0;
		setFollow = false;
		turnTime = 0;
		spline_ = 0;
	}

	void activate();
	bool workedOut();
	void serialize(Archive& ar);

private:
	bool turnStarted_;
};


struct ActionSetObjectAnimation : ActionContext 
{
	ActionSetObjectAnimation();
	~ActionSetObjectAnimation();

	bool automaticCondition() const;
	void activate();
	bool workedOut();
	void serialize(Archive& ar);
	
	void interrupt() { mustInterrupt_ = true; } 
	const UI_MessageSetup& messageSetup() const { return messageSetup_; } 

private:
	SwitchMode switchMode;
	DurationTimer timer;
	int counter;
	UI_MessageSetup messageSetup_;

	bool mustInterrupt_;
};

//---------------------------------

class ActionSoundMessage : public Action
{
public:
	ActionSoundMessage()
	{ 
		switchMode_ = ON;
	}

	bool automaticCondition() const;
	void activate();
	void serialize(Archive& ar);

	SwitchMode switchMode_;
	SoundReference soundReference;
};

struct ActionSelectUnit : Action // Селектировать юнита
{
	AttributeReference unitID; 

	void activate();
	void serialize(Archive& ar);
};

class ActionDeselect : public Action // Деселект
{
public:
	void activate();
};

/// выход из игры
class ActionGameQuit : public Action 
{
public:
	void activate();
};

// UI actions

struct ActionSetInterface : Action // Включить/выключить интерфейс
{
	bool enableInterface; 

	ActionSetInterface() {
		enableInterface = true; 
	}

	void activate();
	void serialize(Archive& ar);
};

/// Создать подсистему для игры по сети
struct ActionCreateNetClient : Action 
{
	ActionCreateNetClient() : type_(UI_NetCenter::LAN) { }

	void activate();
	void serialize(Archive& ar);

private:
	UI_NetCenter::NetType type_;
};

class UI_Screen;

/// включить определённый экран интерфейса
struct ActionSelectInterfaceScreen : Action 
{
	ActionSelectInterfaceScreen(){ }

	void activate();
	void serialize(Archive& ar);
	bool workedOut();

private:
	UI_ScreenReference screenReference_;
	DurationTimer timer_;
	DurationNonStopTimer nonStopTimer_;
};

/// спрятать/показать контрол
class ActionInterfaceHideControl : public Action
{
public:
	ActionInterfaceHideControl() : hideControl_(true){ }

	void activate();
	void serialize(Archive& ar);

private:
	UI_ControlReference controlReference_;
	bool hideControl_;
};

/// спрятать/показать контрол, из интерфейса не перекрывается
class ActionInterfaceHideControlTrigger : public Action
{
public:
	ActionInterfaceHideControlTrigger() : hideControl_(true){ }

	void activate();
	void serialize(Archive& ar);

private:
	UI_ControlReference controlReference_;
	bool hideControl_;
};

class ActionInterfaceControlOperate : public Action
{
public:

	void activate();
	bool workedOut();
	void serialize(Archive& ar);

private:
	AtomActions actions_;
};

/// включить состояние контрола по номеру
class ActionInterfaceSetControlState : public Action
{
public:
	ActionInterfaceSetControlState() : state_(0){ }

	void activate();
	void serialize(Archive& ar);

private:
	UI_ControlReference controlReference_;
	int state_;
};

/// запретить/разрешить контрол
class ActionInterfaceTogglAccessibility : public Action
{
public:
	ActionInterfaceTogglAccessibility() : enableControl_(true){ }

	void activate();
	void serialize(Archive& ar);

private:
	UI_ControlReference controlReference_;
	bool enableControl_;
};

/// управление анимацией контрола
class ActionInterfaceUIAnimationControl : public Action
{
public:
	ActionInterfaceUIAnimationControl() : action_(PLAY_ACTION_RESTART){ }

	void activate();
	void serialize(Archive& ar);

private:
	UI_ControlReference controlReference_;
	PlayControlAction action_;
};


/// отсылка команды заселекченному юниту
class ActionUI_UnitCommand : public ActionContext
{
public:
	ActionUI_UnitCommand();
	~ActionUI_UnitCommand();
	void activate();
	void serialize(Archive& ar);

protected:
	UnitCommand& unitCommand;
};

/// старт игры
class ActionUI_GameStart : public Action
{
public:
	ActionUI_GameStart() { paused_ = false; }
	void serialize(Archive& ar);
	void activate();

private:
	bool paused_;
};

/// старт сетевой игры
class ActionUI_LanGameStart : public Action
{
public:

	void activate();
};

/// присоединение к сетевой игре
class ActionUI_LanGameJoin : public Action
{
public:

	void activate();
};

/// создание сетевой игры
class ActionUI_LanGameCreate : public Action
{
public:

	void activate();
};

/// выключить интерфейсный экран
class ActionUI_ScreenSwitchOff : public Action
{
public:

	void activate();
	bool workedOut();
};

/// подтвердить или отклонить перезапись сэйва, реплея или профиля
class ActionUI_ConfirmDiskOp : public Action
{
public:
	ActionUI_ConfirmDiskOp(){ confirmDiskOp_ = true; }

	void activate();
	void serialize(Archive& ar);

private:

	bool confirmDiskOp_;
};

class ActionToggleBuildingInstaller : public Action
{
public:
	ActionToggleBuildingInstaller(){ }

	void activate();
	void serialize(Archive& ar);

private:

	AttributeBuildingReference attributeReference_;
};

//---------------------------------

class ActionSetUnitLevel : public ActionContext
{
public:
	ActionSetUnitLevel() { level_ = 1; }

	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);
private:

	int level_;
};

class ActionSetUnitSelectAble : public ActionContext
{
public:
	ActionSetUnitSelectAble() { switchMode_ = OFF; }

	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);

private:
	SwitchMode switchMode_;
};

class ActionSetUnitInvisible : public ActionContext
{
public:
	ActionSetUnitInvisible() { switchMode_ = ON; }

	bool checkContextUnit(UnitActing* unit) const;
	void activate();
	void serialize(Archive& ar);

private:
	SwitchMode switchMode_;
};

class ActionSetIgnoreFreezedByTrigger : public ActionContext
{
public:
	ActionSetIgnoreFreezedByTrigger();

	void activate();
	void serialize(Archive& ar);

private:
	SwitchMode switchMode_;
};

class ActionUnitClearOrders : public ActionContextSquad
{
public:
	void activate();
};

struct ActionShowReel : Action 
{
	ActionShowReel();
	void activate();
	void serialize(Archive& ar);
	bool workedOut();

	string binkFileName;
	string soundFileName;
	int alpha;
	DurationTimer timer;
	bool localizedVideo;
	bool localizedVoice;
	bool stopBGMusic;
	bool skip;
};

struct ActionShowLogoReel : Action 
{
	string bkgName;
	string fishName;
	string logoName;
	string groundName;
	float fishAlpha;
	float logoAlpha;
	float fadeTime;
	cBlobsSetting blobsSetting;
	bool localized;
	int workTime;
	SoundLogoAttributes soundAttributes;

	DurationTimer timer;

	ActionShowLogoReel();
	void activate();
	void serialize(Archive& ar);
};

struct ActionStartMission : Action 
{
	string missionName;
	GameType gameType_;
	bool paused_;

	ActionStartMission() : gameType_(GAME_TYPE_SCENARIO) { paused_ = false; }
	bool automaticCondition() const;
	void activate();
	void serialize(Archive& ar);
};

class ActionLoadGameAuto : public Action
{
public:
	ActionLoadGameAuto() : gameType_(GAME_TYPE_SCENARIO) {}

	bool automaticCondition() const;
	bool checkMissionExistence(const string& missionName, GameType gameType) const;
	void activate();
	void serialize(Archive& ar);
private:
	string missionName;
	mutable string name;
	GameType gameType_;
};

struct ActionSetCurrentMission : ActionStartMission 
{
	void activate();
};

struct ActionReseCurrentMission : Action 
{
	void activate();
};


struct ActionUnitParameterArithmetics : ActionForAI
{
	AttributeUnitReference unit;
	ParameterArithmetics arithmetics;

	ActionUnitParameterArithmetics() 
	{
		onlyIfAi = false; // CONVERSION 05.10.06
	}
	void activate();
	void serialize(Archive& ar);
};

struct ActionObjectParameterArithmetics : ActionContext
{
	ParameterArithmetics arithmetics;

	void activate();
	void serialize(Archive& ar);
};

struct ActionAIUnitCommand : public ActionUI_UnitCommand
{
	ActionAIUnitCommand();
	void activate();
};

/// Установить курсор, который будет действовать, пока не вызовется ActionFreeCursor
struct ActionSetCursor : Action
{
	UI_CursorReference cursor;

	void activate();
	void serialize(Archive& ar);
};

/// Отменить установку курсора
struct ActionFreeCursor : Action
{
	void activate();
};

/// Сменить курсор для выбора юнита данного типа
struct ActionChangeUnitCursor : Action
{
	AttributeReference attribute;
	UI_CursorReference cursor;

	void activate();
	void serialize(Archive& ar);
};

/// Сменить курсор общих действий
struct ActionChangeCommonCursor : Action
{
	UI_CursorType cursorType;
	UI_CursorReference cursor;
	
	ActionChangeCommonCursor() : cursorType(UI_CURSOR_PASSABLE) {}

	void activate();
	void serialize(Archive& ar);
};

class ActionSetSignalVariable : public Action
{
public:
	enum Acting  
	{
		ACTION_ADD,
		ACTION_REMOVE
	};

	ComboListString signalVariable;
	Acting acting_;

	ActionSetSignalVariable() :
	signalVariable("")
	{ acting_ = ACTION_ADD; }
	
	void serialize(Archive& ar);
	void activate();
};

class ActionSetInt : public Action
{
public:
	ActionSetInt();
	void serialize(Archive& ar);
	void activate();

private:
	enum ScopeType scope_;
	string name_;
	int value_;
	MissionDescriptionForTrigger missionDescription;
	bool valueBool_;
};

class ActionSetCutScene : public Action
{
public:
	ActionSetCutScene();
	void serialize(Archive& ar);
	void activate();

private:
	SwitchMode switchMode;
};


#endif //__ACTIONS_H__
