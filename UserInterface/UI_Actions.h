#ifndef __UI_ACTIONS_H__
#define __UI_ACTIONS_H__

#include "UI_Types.h"
#include "UI_References.h"
#include "Units\Parameters.h"
#include "Environment\SourceBase.h"
#include "Game\GameOptions.h"
#include "UnitCommand.h"
#include "Controls.h"
#include "Network\NetPlayer.h"
#include "Game\PlayerStatistics.h"
#include "Units\AttributeSquad.h"

enum PostEffectType;

class UI_ActionDataSaveGameList : public UI_ActionDataFull
{
public:
	UI_ActionDataSaveGameList();

	void serialize(Archive& ar);

	bool shareList() const { return shareList_; }

	GameType gameType() const { return gameType_; }
	bool autoSaveType() const { return autoType_; }

	bool resetSaveGameName() const { return resetSaveGameName_;	}

protected:

	/// брать из общего списка или из профиля
	bool shareList_;
	/// При выборе заполнять поле ввода именни игры для сохранения
	bool resetSaveGameName_;
	/// выбирать тип игры автоматически по текущей миссии
	bool autoType_;
	/// фильтр по типу сейва
	GameType gameType_;
};

class UI_ActionDataSaveGameListFixed : public UI_ActionDataSaveGameList
{
public:
	UI_ActionDataSaveGameListFixed();

	void serialize(Archive& ar);

	bool inherit() const { return inheritPredefine_; }
	bool isPredefine() const { return setPredefine_; }

private:
	bool inheritPredefine_;
	bool setPredefine_;
};

class UI_ActionDataHostList : public UI_ActionDataFull
{
public:
	UI_ActionDataHostList() {}

	void serialize(Archive& ar);

	const GameListInfoTypes& format() const { return format_; }
	const Color4c& startedGameColor() const { return startedGameColor_; }

private:
	GameListInfoTypes format_;
	Color4c startedGameColor_;
};

class UI_ActionDataPlayer : public UI_ActionDataFull
{
public:
	UI_ActionDataPlayer();

	void serialize(Archive& ar);

	int playerIndex() const { return playerIndex_; }
	int teamIndex() const { return teamIndex_; }
	
	bool useLoadedMission() const { return useLoadedMission_; }
	bool readOnly() const { return readOnly_; }
	bool onlyOperateControl() const { return onlyOperateControl_; }

private:

	bool useLoadedMission_;
	bool readOnly_;
	bool onlyOperateControl_;
	/// индекс команды
	int playerIndex_;
	/// индекс в команде
	int teamIndex_;
};

class UI_ActionPlayerStatistic : public UI_ActionDataUpdate
{
public:
	enum Type {
		LOCAL,
		GLOBAL
	};

	UI_ActionPlayerStatistic();

	void serialize(Archive& ar);

	Type statisticType() const { return statisticType_; }
	int playerIndex() const { return playerIndex_; }
	StatisticType type() const { return type_; }

private:

	Type statisticType_;
	/// индекс игрока
	int playerIndex_;
	/// параметр статистики
	StatisticType type_;
};

class UI_ActionDataSelectPlayer : public UI_ActionDataFull
{
	bool byRaceName_;
public:
	UI_ActionDataSelectPlayer() : byRaceName_(false) {}

	void serialize(Archive& ar);

	bool selectByRaceName() const { return byRaceName_;}
};

class UI_ActionDataPlayerParameter : public UI_ActionDataUpdate
{
public:
	UI_ActionDataPlayerParameter() { maxValue_ = -1.f; }

	void serialize(Archive& ar);

	int parameterIndex() const { return parameter_.key(); }
	float parameterMax() const { return maxValue_; }

private:

	/// тип выводимого параметра
	ParameterTypeReference parameter_;
	/// максимальное значение, если отсутствует в максимальных параметрах игрока
	float maxValue_;
};

class UI_ActionDataPlayerDefParameter : public UI_ActionDataUpdate
{
public:
	UI_ActionDataPlayerDefParameter() {}

	void serialize(Archive& ar);

	const ParameterTypeReferenceZero& par() const { return parameter_; }

private:
	ParameterTypeReferenceZero parameter_;
};

class UI_ActionDataUnitParameter : public UI_ActionDataUpdate
{
public:
	enum ParameterClass {
		LOGIC,
		LEVEL,
		GROUND,
		PRODUCTION_PROGRESS
	};

	enum UnitType {
		SELECTED,
		SPECIFIC,
		PLAYER,
		UNIT_LIST,
		HOVERED
	};

	UI_ActionDataUnitParameter();

	void serialize(Archive& ar);

	UnitType unitType() const { return unitType_; }

	ParameterClass type() const { return type_;	}
	int parameterIndex() const { return parameter_.key(); }
	float parameterMax() const { return maxValue_; }
	
	const AttributeBase* attribute() const { return attributeReference_; }

	bool updateAndCheck(int showId, float parameterValue) const;

private:

	/// Общий вид выводимого параметра
	ParameterClass type_;
	/// тип выводимого параметра
	ParameterTypeReference parameter_;
	/// у кого берем параметры для вывода
	UnitType unitType_;
	/// максимальное значение, если отсутствует в максимальных параметрах юнита
	float maxValue_;
	/// для какого юнита выводить
	AttributeUnitActingReference attributeReference_;
	/// настройка условия показа
	bool onIncrease_;
	bool onDecrease_;
	float showTime_;

	/// управление показом
	mutable LogicTimer showTimer_;
	mutable int lastShowId_;
	mutable float lastShowValue_;
};

struct UI_ActionDataHint
{
	UI_ActionDataHint() : showDelay(-1.f) {}

	float showDelay;
};

class UI_ActionDataUnitHint : public UI_ActionDataUpdate, public UI_ActionDataHint
{
public:
	UI_ActionDataUnitHint(){
		hintType_ = SHORT;
		unitType_ = HOVERED;
		line_ = 0;
	}
	
	void serialize(Archive& ar);

	enum UI_ActionDataUnitHintType{
		FULL,
		SHORT
	};

	enum UI_ActionDataUnitHintUnitType{
		SELECTED,
		HOVERED,
		CONTROL,
		INVENTORY,
		UNIT_LIST
	};

	UI_ActionDataUnitHintType hintType() const { return hintType_;	}
	UI_ActionDataUnitHintUnitType unitType() const { return unitType_; }
	int lineNum() const { return line_; }

private:
	UI_ActionDataUnitHintType hintType_;
	UI_ActionDataUnitHintUnitType unitType_;
	int line_;
};

class UI_ActionDataControlHint : public UI_ActionDataUpdate, public UI_ActionDataHint
{
public:
	UI_ActionDataControlHint() { type_ = 0; }

	int type() const { return type_; }

	void serialize(Archive& ar);
private:
	int type_;
};

class UI_ActionDataBindGameType : public UI_ActionDataUpdate
{
public:
	enum UI_BindGameType{
		UI_GT_NETGAME = 1,
		UI_GT_BATTLE = 1 << 1,
		UI_GT_SCENARIO = 1 << 2
	};

	UI_ActionDataBindGameType()
		: gameType_(UI_GT_BATTLE)
		, replay_(UI_ANY) {}

	void serialize(Archive& ar);
	
	bool checkGameType(UI_BindGameType tp) const { return gameType_ & tp; }
	TripleBool replay() const { return replay_; }

private:
	BitVector<UI_BindGameType> gameType_;
	TripleBool replay_;
};

class UI_ActionDataPause : public UI_ActionDataUpdate
{
public:
	enum PauseType {
		USER_PAUSE	= 1 << 0,
		MENU_PAUSE	= 1 << 1,
		NET_PAUSE	= 1 << 2
	};
	
	UI_ActionDataPause() : type_(USER_PAUSE) {}

	int type() const { return type_; }

	void serialize(Archive& ar);

private:
	BitVector<PauseType> type_;
};

class UI_ActionDataBindErrorStatus : public UI_ActionDataUpdate
{
public:
	UI_ActionDataBindErrorStatus() : type_(UI_NET_WAITING) { }

	void serialize(Archive& ar);
	
	UI_NetStatus errorStatus() const { return type_; }

private:
	UI_NetStatus type_;
};

class UI_ActionDataClickMode : public UI_ActionDataFull
{
public:
	UI_ActionDataClickMode() : modeID_(UI_CLICK_MODE_MOVE) { }

	void serialize(Archive& ar);

	UI_ClickModeID modeID() const { return modeID_; }
	const WeaponPrm* weaponPrm() const { return weaponPrmReference_; }

private:

	UI_ClickModeID modeID_;

	/// ссылка на оружие
	/// используется если надо указать цель для атаки определённому оружию
	WeaponPrmReference weaponPrmReference_;
};

template<ActionMode mode, int refEditType>
// refEditType: 1 - unit, 2 - building
class UI_ActionDataUnitRef : public UI_ActionDataBase<mode>
{
public:
	UI_ActionDataUnitRef() {}
	
	void serialize(Archive& ar);
	
	const AttributeBase* attribute() const { return attributeReference_; }

private:
	AttributeUnitOrBuildingReference attributeReference_;
};

typedef UI_ActionDataUnitRef<UI_ACTION_MOUSE_LB, 2>  UI_ActionDataBuildingAction;
typedef UI_ActionDataUnitRef<UI_ACTION_TYPE_UPDATE, 1 | 2>  UI_ActionDataUnitOrBuildingRef;

class UI_ActiobDataUnitHovered : public UI_ActionDataUnitOrBuildingRef
{
public:
	UI_ActiobDataUnitHovered() : UI_ActionDataUnitOrBuildingRef(), ownUnit_(true) {}

	void serialize(Archive& ar);

	bool own() const { return ownUnit_; }

private:
	bool ownUnit_;
};

class UI_ActionDataUnitOrBuildingUpdate : public UI_ActionDataUpdate
{
public:
	UI_ActionDataUnitOrBuildingUpdate(): uniformSelection_(false), singleSelect_(false) {}

	void serialize(Archive& ar);
	
	const AttributeBase* attribute() const { return attributeReference_; }
	bool uniformSelection() const {return uniformSelection_; }
	bool single() const { return singleSelect_; }

private:
	bool singleSelect_;
	bool uniformSelection_;
	AttributeUnitOrBuildingReference attributeReference_;
};

class UI_ActionDataSelectUnit : public UI_ActionDataAction
{
public:
	UI_ActionDataSelectUnit() : onlyPowered_(false) {}

	void serialize(Archive& ar);

	const AttributeBase* attribute() const { return attributeReference_; }
	const AttributeSquad* squad() const { return squadRef_; }
	bool onlyPowered() const { return onlyPowered_; }

private:
	bool onlyPowered_;
	AttributeUnitOrBuildingReference attributeReference_;
	AttributeSquadReference squadRef_;
};

class UnitObjective;
class UI_ActionDataFindUnit : public UI_ActionDataFull
{
public:
	UI_ActionDataFindUnit();
	void serialize(Archive& ar);

	int attributeCount() const { return unitReferences_.size(); }
	const AttributeBase* attribute(int idx) const { dassert(idx < unitReferences_.size()); return unitReferences_[idx]; }
	bool select() const { return select_; }
	bool targeting() const { return targeting_; }
	bool showCount() const { return showCount_; }

	UnitObjective* getUnit() const;
	void init() const {
		unitIdx_ = 0;
		referenceIdx_ = 0;
	}

private:
	bool select_;
	bool targeting_;
	bool showCount_;
	typedef vector<AttributeUnitObjectiveReference> UnitAttributeReferences;
	UnitAttributeReferences unitReferences_;
	mutable int unitIdx_;
	mutable int referenceIdx_;
};

class UI_ActionDataSquadRef : public UI_ActionDataUpdate
{
public:
	UI_ActionDataSquadRef() : showCount_(false) {}
	void serialize(Archive& ar);
	const class AttributeSquad* attribute() const { return squadRef_; }
	bool showCount() const { return showCount_; }

private:
	bool showCount_;
	AttributeSquadReference squadRef_;
};

class UI_ActionUnitState : public UI_ActionDataUpdate
{
public:
	
	enum UI_ActionUnitStateType {
		UI_UNITSTATE_SELF_ATTACK,
		UI_UNITSTATE_WEAPON_MODE,
		UI_UNITSTATE_AUTO_TARGET_FILTER,
		UI_UNITSTATE_WALK_ATTACK_MODE,
		UI_UNITSTATE_RUN,
		UI_UNITSTATE_AUTO_FIND_TRANSPORT,
		UI_UNITSTATE_CAN_DETONATE_MINES,
		UI_UNITSTATE_CAN_UPGRADE,
		UI_UNITSTATE_IS_UPGRADING,
		UI_UNITSTATE_CAN_BUILD,
		UI_UNITSTATE_IS_BUILDING,
		UI_UNITSTATE_CAN_PRODUCE_PARAMETER,
		UI_UNITSTATE_SQUAD_CAN_QUERY_UNITS,
		UI_UNITSTATE_IS_IDLE,
		UI_UNITSTATE_COMMON_OPERABLE
	};

	UI_ActionUnitState();

	UI_ActionUnitStateType type() const { return type_; }
	const AttributeBase* attribute() const { return attributeReference_; }
	int data() const { return data_; }
	bool checkOnlyMainUnitInSquad() const { return checkOnlyMainUnitInSquad_; }
	bool playerUnit() const { return forPlayerUnit_; }
	
	void serialize(Archive& ar);

	const AttributeBase* getAttribute(const AttributeBase* context) const;

private:

	UI_ActionUnitStateType type_;
	AttributeUnitOrBuildingReference attributeReference_;
	int data_;
	bool checkOnlyMainUnitInSquad_;
	bool forPlayerUnit_;
};

class UI_ActionDataParams : public UI_ActionDataUpdate
{
public:
	enum Type {
		SELECTED_UNIT_THEN_PLAYER,
		PLAYER_UNIT,
		SELECTRD_UNIT_ONLY,
		PLAYER_ONLY
	};

	enum ThatDoThanExist {
		HIDE,
		SHOW,
		DISABLE,
		ENABLE
	};

	UI_ActionDataParams();
	void serialize(Archive& ar);

	const ParameterSet& prms() const { return param_; }
	Type type() const { return type_; }
	ThatDoThanExist thatDoThanExist() const { return thatDoThanExist_; }

private:
	Type type_;
	ThatDoThanExist thatDoThanExist_;
	ParameterCustom param_;
};

class UI_ActionDataIdleUnits : public UI_ActionDataFull
{
	UnitFormationTypeReference type_;

public:
	UI_ActionDataIdleUnits() {}
	
	const UnitFormationTypeReference& type() const { return type_; }

	void serialize(Archive& ar);
};


class UI_ActionDataUnitCommand : public UI_ActionDataFull
{
public:
	UI_ActionDataUnitCommand();

	void serialize(Archive& ar);
	const UnitCommand& command() const { return command_; }
	bool playerUnit() const { return forPlayerUnit_; }
	bool sendForAll() const { return sendForAll_; }

private:

	UnitCommand command_;
	// посылать команду всему селекту, когда он неоднороден
	bool sendForAll_;
	bool forPlayerUnit_;
};

class UI_ActionDataWeaponRef : public UI_ActionDataUpdate
{
public:
	UI_ActionDataWeaponRef();
	void serialize(Archive& ar);

	const WeaponPrm* weaponPrm() const { return weaponPrmReference_; }
	bool playerUnit() const { return forPlayerUnit_; }
	bool showAutoFire() const { return showAutoFire_; }

private:
	bool forPlayerUnit_;
	bool showAutoFire_;
	WeaponPrmReference weaponPrmReference_;
};

class UI_ActionDataWeapon : public UI_ActionDataAction
{
public:
	UI_ActionDataWeapon();
	void serialize(Archive& ar);

	const WeaponPrm* weaponPrm() const { return weaponPrmReference_; }
	bool playerUnit() const { return forPlayerUnit_; }

private:
	bool forPlayerUnit_;
	WeaponPrmReference weaponPrmReference_;
};

class UI_ActionDataWeaponActivate : public UI_ActionDataWeapon
{
public:
	UI_ActionDataWeaponActivate() : fireOnce_(true) {}
	void serialize(Archive& ar);

	bool fireOnce() const { return fireOnce_; }

private:
	bool fireOnce_;
};


class UI_ActionDataWeaponReload : public UI_ActionDataUpdate
{
public:
	UI_ActionDataWeaponReload() : show_only_not_full_(false) { forPlayerUnit_ = false; }
	void serialize(Archive& ar);

	bool ShowNotCharged() const { return show_only_not_full_; }
	bool playerUnit() const { return forPlayerUnit_; }
	const WeaponPrm* weaponPrm() const { return weaponPrmReference_; }

private:
	bool forPlayerUnit_;
	bool show_only_not_full_;
	WeaponPrmReference weaponPrmReference_;
};

class UI_ActionOption : public UI_ActionDataAction
{
public:
	UI_ActionOption();
	
	void serialize(Archive& ar);

	
	UI_ActionOption(UI_OptionType, GameOptionType);

	UI_OptionType ActionType() const { return action_type_; }
	GameOptionType Option() const { return option_; }

private:

	UI_OptionType action_type_;
	GameOptionType option_;
	
};

class UI_ActionKeys : public UI_ActionDataAction
{
public:
	UI_ActionKeys();
	UI_ActionKeys(UI_OptionType, InterfaceGameControlID = (InterfaceGameControlID)0);

	void serialize(Archive& ar);

	UI_OptionType ActionType() const { return action_type_; }
	InterfaceGameControlID Option() const { return option_; }

private:

	UI_OptionType action_type_;
	InterfaceGameControlID option_;

};

class UI_ActionTriggerVariable : public UI_ActionDataFull
{
public:
	enum UI_ActionTriggerVariableType {
		GLOBAL,
		MISSION_DESCRIPTION
	};

	UI_ActionTriggerVariable() : type_(GLOBAL), number_(0) {}

	void serialize(Archive& ar);

	UI_ActionTriggerVariableType type() const { return type_; }
	
	const char* name() const { return variableName_.c_str(); }
	int number() const { return number_; }

private:
	UI_ActionTriggerVariableType type_;
	
	string variableName_;
	char number_;
};


class UI_ActionBindEx : public UI_ActionDataUpdate
{
public:
	UI_ActionBindEx();
	
	void serialize(Archive& ar);
	
	enum UI_ActionBindExType {
		UI_BIND_PRODUCTION,
		UI_BIND_PRODUCTION_SQUAD,
		UI_TRANSPORT,
		UI_BIND_ANY
	};

	enum UI_ActionBindExSelSize{
		UI_SEL_ANY,
		UI_SEL_SINGLE,
		UI_SEL_MORE_ONE
	};

	enum UI_ActionBindExQueueSize{
		UI_QUEUE_ANY,
		UI_QUEUE_EMPTY,
		UI_QUEUE_NOT_EMPTY
	};

	UI_ActionBindExSelSize selectionSize() const { return selSize_; }
	UI_ActionBindExQueueSize queueSize() const { return queueSize_; }
	UI_ActionBindExType type() const { return type_; }
	bool uniformSelection() const { return uniformSelection_; }

private:
	
	UI_ActionBindExType type_;
	UI_ActionBindExSelSize selSize_;
	UI_ActionBindExQueueSize queueSize_;
	bool uniformSelection_;
};

class UI_ActionDataStateChange : public UI_ActionDataAction
{
public:
	UI_ActionDataStateChange(){ reverseDirection_ = false; }

	bool reverseDirection() const { return reverseDirection_; }

	void serialize(Archive& ar);

private:

	bool reverseDirection_;
};


class UI_ActionDataProduction : public UI_ActionDataUpdate
{
public:
	UI_ActionDataProduction(){ productionNumber_ = -1; forPlayerUnit_ = false; }

	int productionNumber() const { return productionNumber_; }
	bool playerUnit() const { return forPlayerUnit_; }

	void serialize(Archive& ar);

private:
	bool forPlayerUnit_;
	int productionNumber_;
};

class UI_ActionDataSelectionOperate : public UI_ActionDataAction
{
public:
	enum SelectionCommand{
		LEAVE_SLOT_ONLY,
		LEAVE_TYPE_ONLY,
		SAVE_SELECT,
		RETORE_SELECT,
		ADD_SELECT,
		SUB_SELECT
	};
	UI_ActionDataSelectionOperate();

	SelectionCommand command() const { return command_; }
	int selectonID() const { return selectonID_; }

	void serialize(Archive& ar);

private:

	SelectionCommand command_;
	int selectonID_;
};

class UI_ActionAutochangeState : public UI_ActionDataUpdate
{
	Rangef interval_;
	
public:

	UI_ActionAutochangeState() : interval_(5, 5) {}

	const Rangef& time() const { return interval_; }
	
	void serialize(Archive& ar);
};

class UI_ActionExternalControl : public UI_ActionDataAction
{
public:
	
	const AtomActions& actions() const { return actions_; }

	void serialize(Archive& ar);

private:
	
	AtomActions actions_;
};

class UI_ActionDataSourceOnMouse : public UI_ActionDataAction
{
	const SourceReference sourceReference_;

public:

	UI_ActionDataSourceOnMouse() {}

	const SourceReference& attr() const { return sourceReference_; }

	void serialize(Archive& ar);
};

class UI_ActionDataModalMessage : public UI_ActionDataAction
{
public:

	enum Action{
		CLOSE,
		CLEAR
	};
	
	UI_ActionDataModalMessage() : type_(CLOSE) {}

	Action type() const { return type_; }

	void serialize(Archive& ar);

private:

	Action type_;
};

class UI_ActionDataMessageList : public UI_ActionDataUpdate
{
public:
	UI_ActionDataMessageList() : reverse_(false), ignoreMessageDisabling_(false), firstOnly_(false) {}

	const UI_MessageTypeReferences& types() const { return types_; }
	bool reverse() const { return reverse_; }
	bool firstOnly() const { return firstOnly_; }
	bool ignoreMessageDisabling() const { return ignoreMessageDisabling_; }

	void serialize(Archive& ar);

private:

	bool reverse_;
	bool ignoreMessageDisabling_;
	bool firstOnly_;
	UI_MessageTypeReferences types_;
};

class UI_ActionDataTaskList : public UI_ActionDataUpdate
{
public:
	UI_ActionDataTaskList() : reverse_(false) {}

	bool reverse() const { return reverse_; }

	void serialize(Archive& ar);

private:

	bool reverse_;
};

class UI_ActionDataUnitFace : public UI_ActionDataUpdate
{
public:
	UI_ActionDataUnitFace() { num_ = 0; }

	int faceNum() const { return num_; }

	void serialize(Archive& ar);

private:
	int num_;
};

class UI_ActionDataDirectControlCursor : public UI_ActionDataUpdate
{
public:
	UI_ActionDataDirectControlCursor();

	void serialize(Archive& ar);

	float cursorScale(float aim_distance) const;

private:

	Rangef scale_;
	Rangef scaleDist_;
};

class UI_ActionDataDirectControlWeaponLoad : public UI_ActionDataUpdate
{
public:
	UI_ActionDataDirectControlWeaponLoad();

	void serialize(Archive& ar);

	enum WeaponType
	{
		WEAPON_PRIMARY,
		WEAPON_SECONDARY
	};

	WeaponType weaponType() const { return weaponType_; }

private:

	/// зарядку какого оружия показывать
	WeaponType weaponType_;
};

class UI_ActionDataConfirmDiskOp : public UI_ActionDataAction
{
public:
	UI_ActionDataConfirmDiskOp(){ confirmDiskOp_ = true; }

	void serialize(Archive& ar);

	bool confirmDiskOp() const { return confirmDiskOp_; }

private:

	bool confirmDiskOp_;
};

class UI_ActionDataStatBoard : public UI_ActionDataFull
{
public:
	UI_ActionDataStatBoard(){}

	void serialize(Archive& ar);

	const ShowStatisticTypes& format() const { return format_; }

private:
	ShowStatisticTypes format_;
};

class UI_ActionDataPostEffect : public UI_ActionDataAction
{
public:
	UI_ActionDataPostEffect();
	
	void serialize(Archive& ar);
	
	PostEffectType postEffect() const { return type_; }
	bool enable() const { return enable_; }

private:
	
	bool enable_;
	PostEffectType type_;
};
 
class UI_ActionDataPauseGame : public UI_ActionDataAction
{
public:
	UI_ActionDataPauseGame() : enable_(false) {}

	void serialize(Archive& ar);
	bool enable() const { return enable_; }
	bool onlyLocal() const { return onlyLocal_; }

private:
	
	bool onlyLocal_;
	bool enable_;
};

class UI_ActionDataGlobalStats : public UI_ActionDataFull
{
public:
	enum Task {
		REFRESH,
		GOTO_BEGIN,				
		FIND_ME,
		GET_PREV,
		GET_NEXT
	};

	UI_ActionDataGlobalStats() : task_(REFRESH) {}

	void serialize(Archive& ar);

	Task task() const { return task_;}

private:
	Task task_;
};

#endif /* __UI_ACTIONS_H__ */
