#ifndef __CONDITIONS_H__
#define __CONDITIONS_H__

#include "Timers.h"
#include "TriggerEditor\TriggerExport.h"
#include "Units\UnitAttribute.h"
#include "UserInterface\UI_References.h"
#include "Environment\Environment.h"
#include "Units\Triggers.h"
#include "Units\RealUnit.h"
#include "Network\NetPlayer.h"
#include "Units\LabelObject.h"

class AttributeSquad;
class AttributeBuilding;
typedef StringTableReference<AttributeSquad, false> AttributeSquadReference;
typedef vector<AttributeSquadReference> AttributeSquadReferences;

typedef BitVector<UnitsConstruction> ConstructionState;
typedef BitVector<UnitsTransformation> TransformationState;

enum ScopeType
{
	SCOPE_GLOBAL,
	SCOPE_PROFILE,
	SCOPE_PLAYER,
	SCOPE_UNIVERSE,
	SCOPE_MISSION_DESCRIPTION
};

enum CompareOperator
{
	COMPARE_LESS,	// Меньше
	COMPARE_LESS_EQ, // Меньше либо равно
	COMPARE_EQ, // Равно
	COMPARE_NOT_EQ, // Не равно
	COMPARE_GREATER, // Больше
	COMPARE_GREATER_EQ // Больше либо равно		 
};

enum Activity 
{
	ACTIVITY_MOVING = 1, // двигается куда-то
	ACTIVITY_ATTACKING = 2, //атакует
	ACTIVITY_PRODUCING = 4, // производит что-то
	ACTIVITY_CONSTRUCTING = 8, // строится сам
	ACTIVITY_BUILDING = 16, // строит кого-то
	ACTIVITY_UPGRADING = 32, //апгрейдится
	ACTIVITY_PICKING_RESOURCE = 64, //собирает ресурс
	ACTIVITY_TELEPORTATING = 128, //телепортируется
	ACTIVITY_MOVING_TO_TRANSPORT = 256,
	ACTIVITY_WAITING_FOR_PASSENGER = 512
};

class ConditionEvent : public Condition
{
public:
	ConditionEvent();
	bool check() const { return timer_.busy(); }
	void setCheck() { timer_.start(timeOut_ * 1000); }
	void clear() { __super::clear(); timer_.stop(); }
	void serialize(Archive& ar);

	bool isEvent() const { return true; }

protected:
	int timeOut_;

private:
	LogicTimer timer_;
};

class ConditionContext : public Condition
{
public:
	ConditionContext();

	bool checkDebug(UnitActing* unit);
	bool check() const { xassert(0); return false; }
	bool check(UnitActing* unit) const { xassert(0); return false; } // не надо вызывать из производных классов, их check - это предикат
	bool isContext(ContextFilter& filter) const;
	void serialize(Archive& ar);

	void clear();
	void clearContext();
	bool allowable(bool forLogic) const { return forLogic || ignoreContext_; }

protected:
	AttributeUnitOrBuildingReferences objects_;
	AttributeSquadReference squad_;
	bool squadFilter_;
	bool ignoreContext_;
	int ignoreContextCounter_;
};

class ConditionContextSquad : public ConditionContext // Наследовать только для сквадовых условий!!!
{
public:
	ConditionContextSquad() { squadFilter_ = true; }
};

struct ConditionContextEvent : ConditionContext
{
	ConditionContextEvent();

	UnitActing* eventContextUnit() const { return eventContextUnit_; }
	void setEventContextUnit(UnitActing* unit);
	bool checkUnit(UnitActing* unit) const;

	bool check(UnitActing* unit) const;
    void clear();

	void serialize(Archive& ar);

	bool isEvent() const { return true; } 

protected:
	UnitLink<UnitActing> eventContextUnit_;
	int timeOut_;
	LogicTimer timer_;
};

struct ConditionIsPlayerActive : Condition 
{
	bool check() const;
	bool allowable(bool forLogic) const { return !forLogic; }
};

struct ConditionIsPlayerAI : Condition 
{
	bool check() const;
};

class ConditionCheckDirectControl : public Condition 
{
public:
	ConditionCheckDirectControl();
	bool check() const;
	void serialize(Archive& ar);
	
private:
	bool syndicateControl_;
};

struct ConditionCheckRace : Condition 
{
	Race race; 

	bool check() const;
	void serialize(Archive& ar);
};

class ConditionCheckOutWater : public Condition
{
public:
	enum OutWater {
		WATER_IS_WATER,
		WATER_IS_ICE,
		WATER_IS_LAVA
	};

	ConditionCheckOutWater() { outWater_ = WATER_IS_WATER; }

	void serialize(Archive& ar);
	bool check() const;

private:
	OutWater outWater_;
};

class ConditionCheckSourceNearUnit : public ConditionContext
{
public:
	ConditionCheckSourceNearUnit();
	void serialize(Archive& ar);
	bool check(UnitActing* unit) const;
	void operator()(SourceBase* source) const;

private:
	float radius_;
	SourceReferences sources_;
	mutable bool found_;
};

class ConditionSourceActivated : public ConditionEvent 
{
public:
	void serialize(Archive& ar);
	void checkEvent(const Event& event); 

private:
	LabelSource source_;
};


struct ConditionCreateSource : ConditionEvent
{
	SourceReference sourceref;
	
	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionObjectPercentOwner : public Condition
{
public:
	ConditionObjectPercentOwner();

	bool check() const;
	void serialize(Archive &ar);

private:
	int percent;
	CompareOperator compareOperator;
	AttributeUnitOrBuildingReferences attrSet;
	bool countFriends;
	mutable int totalMax;
};

class ConditionObjectUnseen : public Condition
{
public:
	int percent;
	CompareOperator compareOperator;
	AttributeItemReference attrItem;

	ConditionObjectUnseen()
	{
		percent = 0;
		compareOperator = COMPARE_GREATER;
	}
	
	bool check() const;
	void serialize(Archive& ar);
};

class ConditionUnitUnseen : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionObjectWorking : public ConditionContext
{
public:
	ConditionObjectWorking();

	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	BitVector<Activity> activity_;
};


//---------------------------------------
class AIPlayerScanner
{
public:
	AIPlayerScanner(AIPlayerType playerType = AI_PLAYER_TYPE_ME);
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	Player& player(Player& aiPlayer);
	bool next(Player& aiPlayer); // true, если _не_ все игроки просканированы
private:
	AIPlayerType playerType_;
	Player* player_;
};

//---------------------------------------
struct ConditionCreateObject : ConditionEvent 
{
	AttributeReferences objects_; 
	AIPlayerType playerType;

	ConditionCreateObject()
	{
		playerType = AI_PLAYER_TYPE_ME;
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionSoldBuilding : public ConditionEvent
{
public:
	AttributeBuildingReferences objects_; 

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionCommandMove : public ConditionEvent
{
public:
	AttributeUnitReferences objects_; 

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionCheckPersonalParameter : public ConditionContext
{
public:
	ConditionCheckPersonalParameter() { value_ = 1.f; }	
	
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);
private:
	float value_;
	ParameterTypeReference parameterType_;
};

class ConditionCheckParameterProducing : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);
private:
	ComboListString signalVariable_;
};


class ConditionCommandMoveSquad : public ConditionEvent
{
public:
	AttributeSquadReference attr;

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionGetResourceLevel : public Condition
{
public:
	ConditionGetResourceLevel();

	void serialize(Archive& ar);
	bool check() const;

private:
	ParameterTypeReference parameterType_;
	float counter_;
	CompareOperator compareOperator_;
};

class ConditionCommandAttackSquad : public ConditionEvent
{
public:
	AttributeSquadReference attr;

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionCommandAttack : public ConditionEvent
{
public:
	AttributeUnitOrBuildingReferences objects_; 
	WeaponPrmReferences weaponref_;

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionWeatherEnabled : public Condition
{
public:
	bool check() const;
};

class ConditionObjectAttacking : public ConditionEvent
{
public:
	void serialize(Archive& ar);
	void checkEvent(const Event& event);
private:
	AttributeUnitOrBuildingReferences objects_; 
	WeaponPrmReference weapon_;
};

class ConditionObjectByLabelAttacking : public ConditionObjectAttacking
{
public:
	void serialize(Archive& ar);
	void checkEvent(const Event& event);
private:
	LabelUnit label_;
};

class ConditionRequestedAssembly : public ConditionEvent
{
public:
	void checkEvent(const Event& event);
};

class ConditionEnvironmentTime : public Condition
{
public:
	ConditionEnvironmentTime();

	void serialize(Archive& ar);
	bool check() const;

private:
	float time_;
	CompareOperator compareOperator_;
};

class ConditionStartBuild : public ConditionEvent
{
public:
	AttributeBuildingReferences objects_; 

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionStartProduction : public ConditionEvent
{
public:
	AttributeUnitReferences objects_; 

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionStartProductionParameter : public ConditionEvent
{
public:
	AttributeUnitOrBuildingReference producer_; 

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};


class ConditionCompleteCure : public ConditionEvent
{
public:
	AttributeUnitOrBuildingReferences objects_;

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

class ConditionCompleteBuild : public ConditionEvent
{
public:
	AttributeBuildingReferences objects_; 

	void serialize(Archive& ar);
	void checkEvent(const Event& event);
};

struct ConditionUnableToBuild : ConditionEvent
{
	AttributeBuildingReferences objects_;

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

struct ConditionKillObject : ConditionEvent 
{
	AttributeReferences objects_; 
	AIPlayerType playerType;

	ConditionKillObject()
	{
		playerType = AI_PLAYER_TYPE_ME;
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionObjectUnderShield : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionAllBuildingsConnected : public Condition
{
public:
	bool check() const;
	void serialize(Archive& ar);

private:
	AttributeBuildingReferences objects;
};

class ConditionBuildingsConnectedToZone : public Condition 
{
public:
	ConditionBuildingsConnectedToZone();

	bool check() const;
	void serialize(Archive& ar);
private:
	AttributeBuildingReferences objects; 
	int counter; 
	AIPlayerType playerType;
	CompareOperator compareOperator;

	int countBuildingsConnected(const Player* player, const AttributeBase* building) const;
};	

struct ConditionObjectsExists : Condition 
{
	AttributeReferences objects; 
	int counter; 
	AIPlayerType playerType;
	ConstructionState constructedAndConstructing; 
	CompareOperator compareOperator;

	ConditionObjectsExists();

	bool check() const;
	void serialize(Archive& ar);
};	

class ConditionCompareObjectsCount : public Condition
{
public:
	ConditionCompareObjectsCount();
	bool check() const;
	void serialize(Archive& ar);
private:
	CompareOperator compare_;
	mutable AIPlayerScanner playerScanner_;
	AttributeUnitOrBuildingReferences group1_;
	AttributeUnitOrBuildingReferences group2_;
	bool connectedOnly_;
};

class ConditionCompareSquadsCount : public Condition
{
public:
	ConditionCompareSquadsCount();
	bool check() const;
	void serialize(Archive& ar);
private:
	CompareOperator compare_;
	mutable AIPlayerScanner playerScanner_;
	AttributeSquadReferences squads1_;
	AttributeSquadReferences squads2_;
};

class ConditionCheckSurfaceNearObjectByLabel : public Condition
{
public:
	ConditionCheckSurfaceNearObjectByLabel();
	bool check() const;
	void serialize(Archive& ar);

private:
	CompareOperator compareOperator_;
	int height_;
	int radius_;
	LabelObject anchor_;
};

class ConditionCheckSurfaceProducedZone : public ConditionContext
{
public:
	ConditionCheckSurfaceProducedZone();
	void serialize(Archive& ar);
	bool check(UnitActing* unit) const;
private:
	int percent_;
};

class ConditionBuildingsConnected : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionBuildingsConnectedMinimal : public ConditionContext
{
public:
	ConditionBuildingsConnectedMinimal();
	bool check(UnitActing* unit) const;
	void clearContext();

private:
	mutable int connectionsMin_;
	mutable int rounds_;
};

class ConditionUnderWater : public Condition
{
public:
	ConditionUnderWater();
	bool check() const;
	void serialize(Archive& ar);

private:
	int radius_;
	bool onlyDeep_;

	LabelObject anchor_;
};

class ConditionCheckSurface : public ConditionContext
{
public:
	ConditionCheckSurface();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	float deviationCosMin_;
};

class ConditionCheckResource : public ConditionEvent
{
public:
	ConditionCheckResource();
	void serialize(Archive& ar);
	void checkEvent(const Event& event);

private:
	ParameterTypeReference parameterType;
	BitVector<RequestResourceType> requestResourceType;
};

class ConditionUnitCaptured : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

struct ConditionCaptureBuilding : ConditionEvent
{
	enum PlayerType{
		MY_PLAYER,
		ENEMY_PLAYER
	};

	enum Participators{
		FRIENDS,
		ENEMIES,
		ALL
	};

	PlayerType playerType_;
	Participators participators_;
	AttributeUnitOrBuildingReferences objects_; 

	ConditionCaptureBuilding();
	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionProducedAllParameters : public Condition
{
public:
	bool check() const;
	void serialize(Archive& ar);

private:
	AttributeUnitOrBuildingReference object_;
	
};

class ConditionProduceParameterFinish : public ConditionEvent
{
public:
	ComboListString signalVariable;

	ConditionProduceParameterFinish() :	signalVariable("") {}
	
	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionProducedParameter : public Condition
{
public:
	bool check() const;
	void serialize(Archive& ar);
private:
	ComboListString signalVariable_;
	mutable AIPlayerScanner playerScanner_;
};

class ConditionStartUpgrade : public ConditionEvent
{
public:
	AttributeUnitOrBuildingReferences objects_; 
	AttributeUnitOrBuildingReferences objectsBefore_;

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionCompleteUpgrade : public ConditionEvent
{
public:
	AttributeUnitOrBuildingReferences objectsBefore_; 
	AttributeUnitOrBuildingReferences objects_; 

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

//---------------------------------------
class ConditionPercentOfMaxUnits : public Condition
{
public:
	ConditionPercentOfMaxUnits() {percent = 100; compareOperator = COMPARE_LESS_EQ; }

	bool check() const;
	void serialize(Archive& ar);

private:
	CompareOperator compareOperator;
	int percent;
	ParameterTypeReferenceZero type_; 
};

class ConditionObjectOnWater : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionObjectOnIce : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionObjectDemaged : public ConditionContext
{
public:

	ConditionObjectDemaged();

	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);
private:
	ParameterTypeReference parameterType_;
	int percent_;
};

class ConditionObjectAimed : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
	void clear();

private:
	mutable LogicTimer timer_;
};

class ConditionObjectHearNoise : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);
private:
	AttributeUnitOrBuildingReferences attrTargets_;
};

class ConditionUnitProducing : public Condition
{
public:
	bool check() const;
	void serialize(Archive& ar);
private:
	AttributeUnitReferences objects_;
	mutable AIPlayerScanner playerScanner_;
};

class ConditionObjectIsNotUnderAttack : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionObjectUnderAttack : public ConditionContextEvent
{ 
public:
	enum WeaponType {
		ANY_TYPE,
		SHORT_RANGE, 
		LONG_RANGE,
		FROM_INTERFACE
	};

	ConditionObjectUnderAttack();

	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private:
    AttributeUnitOrBuildingReferences objectsEnemy_; 
	int damagePercent_;
	WeaponPrmReferences weapons_;
	WeaponType weaponType_;
	bool checkUnit_;
};

//---------------------------------------
class ConditionSquadSufficientUnits : public ConditionContextSquad // Cквад состоит из юнитов в указанном количестве
{
public:
    ConditionSquadSufficientUnits();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	CompareOperator compareOperator_;
	int counter_;
};

class ConditionObjectByLabel : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	LabelUnit label_; 
};

//---------------------------------------
struct ConditionObjectByLabelExists : Condition // Объект по метке существует
{
	bool check() const;
	void serialize(Archive& ar);

private:
	LabelObject anchor_;
};	

struct ConditionKillObjectByLabel : ConditionEvent // Объект по метке уничтожен
{
	ConditionKillObjectByLabel() 
	{
		playerType = AI_PLAYER_TYPE_ME;
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private:
	LabelUnit label_;
	AIPlayerType playerType;
};

class ConditionAnchorOnScreen : public Condition
{
public:
	ConditionAnchorOnScreen() : distance_(1000) {}
	bool check() const;
	void serialize(Archive& ar);

private:
	LabelObject anchor_;
	int distance_;
};

class ConditionObjectNearAnchorByLabel : public Condition
{
public:

	ConditionObjectNearAnchorByLabel();
	bool check() const;
	void serialize(Archive& ar);

private:
	int countfindUnit(Player* player, const AttributeBase* attr) const;

	ConstructionState constructedAndConstructing; 
	LabelObject anchor_; 
	AttributeReferences objects_; 
	AIPlayerType playerType; 
	float distance; 
	bool onlyVisible_;
	int count_;
};

class ConditionMyObjectNearAnchorByLabel : public ConditionContext
{
public:

	ConditionMyObjectNearAnchorByLabel();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	LabelObject anchor_; 
	float distance; 
	bool onlyVisible_;
};

class ConditionObjectBuildingInProgress : public ConditionContext
{
public:

	bool check(UnitActing* unit) const;
};

class ConditionCountObjectsInRadius : public ConditionContext
{
public:
	ConditionCountObjectsInRadius();
	void serialize(Archive& ar);

	bool check(UnitActing* unit) const;

private:
	float radius_;
	AttributeUnitOrBuildingReferences attributes_;
	int count_;
};

class ConditionObjectsInRadiusAimed : public ConditionContext
{
public:
	ConditionObjectsInRadiusAimed();
	void serialize(Archive& ar);

	bool check(UnitActing* unit) const;

private:
	float radius_;
	AttributeUnitOrBuildingReferences attributes_;
};


class ConditionCompareCountObjectsInRadius : public ConditionContext
{
public:
	ConditionCompareCountObjectsInRadius();
	void serialize(Archive& ar);

	bool check(UnitActing* unit) const;

private:
	float radius_;
	AttributeUnitOrBuildingReferences attributesLess_;
	AttributeUnitOrBuildingReferences attributesGreater_;
};

struct CountObjectsScanOp
{
	CountObjectsScanOp(Player* player, const AttributeUnitOrBuildingReferences& units, float radius, bool aimed = false);

	void checkInRadius(const Vect2f& position); 
	void operator()(UnitBase* unit);
	int foundNumber() const { return count_; }
	void show(Vect2f pos, const Color4c& color);

	Player* player_;
	float radius_;
	AttributeUnitOrBuildingReferences units_;
	int count_;
	Vect2f position_;
	bool aimed_;
};

struct ConditionDistanceBetweenObjects : ConditionContext 
{
	ConditionDistanceBetweenObjects();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

	bool operator()(UnitBase* unit2);

private:
	AttributeReferences objects; 
	float distance; 
	AIPlayerType aiPlayerType_;   
	bool onlyLegionaries_;
	bool onlyVisible_;
	mutable LogicTimer foundTimer_;
	mutable UnitReal* unit1_;
};

struct ConditionTimeMatched : ConditionEvent // Осталось времени меньше, чем указано
{
	int time; 
	
	ConditionTimeMatched() {
		time = 60; 
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

struct ConditionNetworkDisconnect : ConditionEvent // Разорвано соединение с игровым сервером
{
	ConditionNetworkDisconnect() { timeOut_ = 1; hardDisconnect_ = true; }
	void checkEvent(const Event& event);

	void serialize(Archive& ar);

private:
	bool hardDisconnect_;
};

struct ConditionKeyboardClick : ConditionEvent 
{
	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private:
	typedef vector<sKey> Keys;
	Keys keys_;
};

struct ConditionClickOnButton : ConditionEvent // Клик по кнопке
{
	ConditionClickOnButton();

	bool check() const { return wasClicked_; }
	void checkEvent(const Event& event);
	void clear() { wasClicked_ = false; }
	void serialize(Archive& ar);

private:
	bool wasClicked_;
	bool onlyEnabled_;
	bool ignorePause_;

	BitVector<UI_UserSelectEvent> events_;
	BitVector<UI_UserEventMouseModifers> modifiers_;

	UI_ControlReference control_;
	mutable UI_ControlBase* controlPtr_;
};

struct ConditionStatusTimeBase : Condition 
{
	ConditionStatusTimeBase();

	bool check() const;

	virtual bool checkStatus() const = 0;

	void serialize(Archive& ar);

	float time_;
	mutable LogicTimer timer_;
};

struct ConditionFocusOnButton : ConditionStatusTimeBase // Мышь над кнопкой
{
	ConditionFocusOnButton() : controlPtr_(0) {}

	bool checkStatus() const;
	void serialize(Archive& ar);
	bool allowable(bool forLogic) const { return !forLogic; }

private:
	UI_ControlReference control_;
	mutable UI_ControlBase* controlPtr_;
};

struct ConditionButtonFocus : ConditionEvent // Мышь навелась на кнопку
{
	ConditionButtonFocus() : controlPtr_(0), onFocus_(true) {}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
	bool allowable(bool forLogic) const { return !forLogic; }

private:
	UI_ControlReference control_;
	mutable UI_ControlBase* controlPtr_;

	bool onFocus_;
};


class ConditionLastNetStatus : public Condition
{
public:
	ConditionLastNetStatus() : status_(UI_NET_WAITING) { }

	bool check() const;
	void serialize(Archive& ar);

private:

	UI_NetStatus status_;
};

class ConditionMissionSelected : public Condition
{
public:
	ConditionMissionSelected() {}
	bool check() const;
};

class ConditionNeedUpdate : public Condition
{
public:
	ConditionNeedUpdate() {}
	bool check() const;
};

class ConditionCheckGameType : public Condition
{
public:
	ConditionCheckGameType();

	bool check() const;
	void serialize(Archive& ar);

private:
	MissionDescriptionForTrigger missionDescription_;
	GameType gameType_;
};

class ConditionPredefineGame : public Condition
{
public:
	ConditionPredefineGame();

	bool check() const;
	void serialize(Archive& ar);

private:
	MissionDescriptionForTrigger missionDescription;
	ScenarioGameType scenarioGameType_;
	TeamGameType teamGameType_;
};

class ConditionCheckPause : public Condition
{
public:
	ConditionCheckPause();

	bool check() const;
	void serialize(Archive& ar);

private:
	bool pausedByUser_;
	bool pausedByMenu_;
};

class ConditionUI_ControlState : public ConditionStatusTimeBase // включено оределённое состояние кнопки
{
public:
	ConditionUI_ControlState() : controlPtr_(0), state_(0) { }

	bool checkStatus() const;
	void serialize(Archive& ar);
	bool allowable(bool forLogic) const { return !forLogic; } 

private:

	int state_;
	UI_ControlReference control_;
	mutable UI_ControlBase* controlPtr_;
};

class ConditionUI_StringSelected : public Condition // в списке выбрана строка
{
public:
	ConditionUI_StringSelected() :
		controlPtr_(0),
		anyStringSelected_(true) {
		selectedStringIndex_ = 0;
	}

	bool check() const;
	void serialize(Archive& ar);
	bool allowable(bool forLogic) const { return !forLogic; }

private:
	bool anyStringSelected_;
	int selectedStringIndex_;
	
	UI_ControlReference control_;
	mutable UI_ControlBase* controlPtr_;
};

class ConditionUI_ProfilesEmpty : public Condition // нет ни одного профиля
{
public:
	ConditionUI_ProfilesEmpty() { }

	bool check() const;
	bool allowable(bool forLogic) const { return !forLogic; }
};

class ConditionUI_ProfileSelected : public Condition // нет ни одного профиля
{
public:
	ConditionUI_ProfileSelected() { }

	bool check() const;
	bool allowable(bool forLogic) const { return !forLogic; }
};

class ConditionUI_NeedDiskOpConfirmation : public Condition // надо подтвердить перезапись сэйва, реплея или профайла.
{
public:
	ConditionUI_NeedDiskOpConfirmation(){ }

	bool check() const;
};


class ConditionNeedCommitSettings : public Condition // нужно подтвердить новые настройки
{
public:
	ConditionNeedCommitSettings() { }

	bool check() const;
};

struct ConditionOnlyMyClan : Condition // Остался только мой клан
{
	bool checkAuxPlayers;

	ConditionOnlyMyClan();
	bool check() const;
	void serialize(Archive& ar);
};

struct ConditionNoUnitsLeft : Condition 
{
	ConditionNoUnitsLeft() { playerNum_ = -1; }

	bool check() const;
	void serialize(Archive& ar);

	int playerNum_;
};


struct ConditionDifficultyLevel : Condition // Уровень сложности
{
	Difficulty difficulty; 

	ConditionDifficultyLevel() {}

	bool check() const;
	void serialize(Archive& ar);
};

struct ConditionUserSave : Condition 
{
	ConditionUserSave();
	bool check() const { return userSave_; }

private:
	bool userSave_;
};

struct ConditionIsMultiplayer : Condition 
{
	enum Mode {
		ANY,
		LAN,
		ONLINE,
		DIRECT
	};

	ConditionIsMultiplayer();
	bool check() const;
	void serialize(Archive& ar);

	Mode mode;
};

class ConditionCheckUnitsInTransport : public ConditionContext
{
public:
	ConditionCheckUnitsInTransport();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);
private:
	int count_;
	AttributeUnitReferences units_;
	CompareOperator compareOperator_;
};

class ConditionUnitInTransport : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionUnitLevel : public ConditionContext
{
public:
	ConditionUnitLevel();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	int level_;
	CompareOperator compareOperator_;
};

class ConditionSelected : public Condition
{
public:
	ConditionSelected();
	bool check() const;
	void serialize(Archive& ar);

private:
	AttributeUnitOrBuildingReferences objects_;
	bool singleOnly; // только один
	bool uniform; // только одного типа
};

class ConditionUnitSelecting : public ConditionContextEvent
{
public:
	ConditionUnitSelecting() 
	{
		unitState_ = UNIT_STATE_CONSTRUCTED | UNIT_STATE_CONSTRUCTING | UNIT_STATE_UPGRADING; 
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private: 
    TransformationState unitState_; 
};


class ConditionSquadSelected : public Condition
{
public:
	ConditionSquadSelected();
	bool check() const;
	void serialize(Archive& ar);

private:
	AttributeSquadReference attribute;
	bool singleOnly; // только один
};


struct ConditionPlayerParameters : Condition
{
	bool check() const;
	void serialize(Archive& ar);

private:
	struct Data 
	{
		ParameterTypeReference type;
		CompareOperator operation;
		float value;
		Data();
		void serialize(Archive& ar);
	};
	typedef vector<Data> Vector;
	Vector vector_;
};

struct ConditionPercentOfPlayerResource : Condition
{
	ConditionPercentOfPlayerResource();
	bool check() const;
	void serialize(Archive& ar);

private:
	ParameterTypeReference type_;
	CompareOperator operation_;
	int percent_;
};

class ConditionPlayerWin : public Condition
{
	bool checkAuxPlayers;
public:
	ConditionPlayerWin();
	bool check() const;
	void serialize(Archive& ar);
};

class ConditionPlayerDefeat : public Condition
{
public:
	bool check() const;
};

class ConditionPlayerByNumberDefeat : public Condition
{
public:
	ConditionPlayerByNumberDefeat();
	bool check() const;
	void serialize(Archive& ar);

private:
	int playerIndex;
};

class ConditionCheckInt : public Condition
{
public:
	ConditionCheckInt();
	void serialize(Archive& ar);
	bool check() const;

private:
	ScopeType scope_;
	string name_;
	CompareOperator op_;
	int value_;
	MissionDescriptionForTrigger missionDescription;
};

struct ConditionEndReplay : ConditionEvent 
{
	void checkEvent(const Event& event){
		if(event.type() == Event::END_REPLAY)
            setCheck();
	}
};

class ConditionEventComing : public ConditionEvent 
{
public:
	ConditionEventComing();
	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private:
	Event::Type eventType_;
};

class ConditionScreenRatio : public Condition
{
public:
	ConditionScreenRatio();
	bool check() const;
	void serialize(Archive& ar);

private:
	CompareOperator op_;
	float ratio_;
};

class ConditionEventString : public ConditionEvent 
{
public:
	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private:
	string name_;
};


#endif //__CONDITIONS_H__
