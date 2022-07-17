#ifndef __CONDITIONS_H__
#define __CONDITIONS_H__

#include "Timers.h"
#include "..\TriggerEditor\TriggerExport.h"
#include "UnitAttribute.h"
#include "..\UserInterface\UI_References.h"
#include "..\Environment\Environment.h"
#include "Triggers.h"
#include "RealUnit.h"
#include "NetPlayer.h"

class AttributeSquad;
typedef StringTableReference<AttributeSquad, false> AttributeSquadReference;

string editLabelDialog();
string editSignalVariableDialog();

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

struct ConditionSwitcher : Condition // И/ИЛИ
{
	enum Type {
		AND, // И
		OR // ИЛИ
	};
	Type type;
	typedef vector<ShareHandle<Condition>, TriggerAllocator<ShareHandle<Condition> > > Conditions; 
	Conditions conditions; 

	ConditionSwitcher();

	void setPlayer(Player* aiPlayer);
	void clear();

	bool check() const;
	bool check(UnitActing* unit) const;
	void checkEvent(const Event& event);

	bool isContext() const;
	bool isSwitcher() const { return true; } 
	bool isEvent() const;
	UnitActing* eventContextUnit() const;
	bool allowable(bool forLogic) const;

	void writeInfo(XBuffer& buffer, string offset, bool debug) const;
	void serialize(Archive& ar);
};

class ConditionEvent : public Condition
{
public:
	ConditionEvent();
	bool check() const { return timer_(); }
	void setCheck() { timer_.start(timeOut_ * 1000); }
	void clear() { __super::clear(); timer_.stop(); }
	void serialize(Archive& ar);

	bool isEvent() const { return true; }

protected:
	int timeOut_;

private:
	DurationTimer timer_;
};

class ConditionContext : public Condition
{
public:
	ConditionContext();

	bool check(UnitActing* unit) const;
	bool isContext() const { return true; }
	void serialize(Archive& ar);

	void startGoodContextTimer();
	void clear();
	bool allowable(bool forLogic) const { return forLogic || ignoreContext_; }

protected:
	AttributeUnitOrBuildingReferences objects_;
	bool ignoreContext_;
	DurationTimer goodContextTimer_;
};

class ConditionContextSquad : public ConditionContext
{
public:
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

protected:
	AttributeSquadReference squad_;
};

struct ConditionContextEvent : ConditionContext
{
	ConditionContextEvent();

	UnitActing* eventContextUnit() const { return eventContextUnit_; }
	void setEventContextUnit(UnitActing* unit) { if(!ignoreContext_) eventContextUnit_ = unit; timer_.start(timeOut_* 1000); }

	bool check(UnitActing* unit) const { return __super::check(unit) && timer_() && (ignoreContext_ || eventContextUnit_ == unit); }
    void clear() { __super::clear(); eventContextUnit_ = 0; timer_.stop(); }

	void serialize(Archive& ar);

	bool isEvent() const { return true; } 

protected:
	UnitLink<UnitActing> eventContextUnit_;
	int timeOut_;
	DurationTimer timer_;
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

class ConditionObjectWorking : public ConditionContextSquad
{
public:
	ConditionObjectWorking();

	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:
	BitVector<Activity> activity_;
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

class ConditionBuildingAttacking : public ConditionEvent
{
public:
	AttributeBuildingReferences objects_; 

	void serialize(Archive& ar);
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

struct ConditionObjectsExists : Condition // Объект существует
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
	ComboListString signalVariable;

	ConditionProducedParameter() :
	signalVariable("")
	{}

	bool check() const;
	void serialize(Archive& ar);
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

class ConditionObjectAimed : public ConditionContextSquad
{
public:
	bool check(UnitActing* unit) const;
};

class ConditionObjectIsNotUnderAttack : public ConditionContextSquad
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
    AttributeUnitOrBuildingReference objectEnemy_; 
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
	string label_; 

};

//---------------------------------------
struct ConditionObjectByLabelExists : Condition // Объект по метке существует
{
	ComboListString label;

	ConditionObjectByLabelExists() :
	label("")
	{}

	bool check() const;
	void serialize(Archive& ar);
};	

struct ConditionKillObjectByLabel : ConditionEvent // Объект по метке уничтожен
{
	ComboListString label;
	AIPlayerType playerType;

	ConditionKillObjectByLabel() :
	label("")
	{
		playerType = AI_PLAYER_TYPE_ME;
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);
};

class ConditionObjectNearAnchorByLabel : public Condition
{
public:

	ConditionObjectNearAnchorByLabel();
	bool check() const;
	void serialize(Archive& ar);

private:

	ConstructionState constructedAndConstructing; 
	ComboListString label; 
	AttributeReferences objects_; 
	AIPlayerType playerType; 
	float distance; 
	bool onlyVisible_;
};

class ConditionMyObjectNearAnchorByLabel : public ConditionContext
{
public:

	ConditionMyObjectNearAnchorByLabel();
	bool check(UnitActing* unit) const;
	void serialize(Archive& ar);

private:

	ComboListString label; 
	float distance; 
	bool onlyVisible_;
};


struct ConditionObjectNearObjectByLabel : Condition // Возле объекта по метке находится объект указанного типа
{
	ConditionObjectNearObjectByLabel();
	bool check() const;
	void serialize(Archive& ar);

	ComboListString label; 
	AttributeReferences objects_; 
	AIPlayerType playerType; 
	float distance; 
	bool objectConstructed; 
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

class ConditionObjectsInRadiusAimed : public ConditionContextSquad
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
	void show(Vect2f pos, const sColor4c& color);

	Player* player_;
	float radius_;
	AttributeUnitOrBuildingReferences units_;
	int count_;
	Vect2f position_;
	bool aimed_;
};

struct ConditionDistanceBetweenObjects : ConditionContextSquad 
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
	mutable bool found_;
	mutable UnitReal* unit1_;
	mutable int waitCounter_;
	enum { WAIT_COUNTER = 10 };
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
	ConditionStatusTimeBase() : Condition() { time_ = 0; }

	bool check() const {
		bool status = checkStatus();
		if(timer_.was_started()){
			if(!status)
				timer_.stop();
		}
		else 
			if(status)
				timer_.start();
			else
				return false;

		return timer_() >= time_ * 1000;
	}

	virtual bool checkStatus() const = 0;

	void serialize(Archive& ar)
	{
		__super::serialize(ar);
		ar.serialize(time_, "time", "Время проверки статуса (сек.)");
	}

	float time_;
	mutable MeasurementTimer timer_;
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
		ANY, LAN, ONLINE
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

class ConditionUnitSelecting : public ConditionEvent
{
public:
	ConditionUnitSelecting() 
	{
		unitState_ = UNIT_STATE_CONSTRUCTED | UNIT_STATE_CONSTRUCTING | UNIT_STATE_UPGRADING; 
	}

	void checkEvent(const Event& event);
	void serialize(Archive& ar);

private:
	AttributeUnitOrBuildingReferences objects_; 
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
