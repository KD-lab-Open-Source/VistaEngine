#ifndef __TRIGGER_EXPORT_H__
#define __TRIGGER_EXPORT_H__

#include <atltypes.h>
#include "Handle.h"
#include "Serialization\SerializationTypes.h"
#include "Timers.h"
#include "XMath\xmath.h"
#include "XMath\Colors.h"
#include "XTL\UniqueVector.h"

//-----------------------------
class Player;
class TriggerChain;
class Trigger;
class Archive;
class UnitReal;
class UnitActing;
class ContextFilter;
class AttributeBase;
class AttributeSquad;

enum CompareOperator;

//-----------------------------
enum AIPlayerType
{
	AI_PLAYER_TYPE_ME, // Я
	AI_PLAYER_TYPE_ENEMY, // Другой
	AI_PLAYER_TYPE_WORLD, // Мир
	AI_PLAYER_TYPE_ANY // Любой
};

//-----------------------------
struct Condition : ShareHandleBase // Не выполняется никогда (для выключения триггеров)
{
	Condition()
	: aiPlayer_(0)
	{
		inverted_ = false;
		state_ = false; 
		internalColor_ = 0;
	}
	virtual ~Condition() {}

	virtual void setPlayer(Player* aiPlayer) { aiPlayer_ = aiPlayer; }
	
	bool inverted() const { return inverted_; }
	void setInverted(bool inverted) { inverted_ = inverted;}

	bool checkDebug() { return state_ = check() ? !inverted() : inverted(); }
	virtual bool checkDebug(UnitActing* unit);

	virtual bool isContext(ContextFilter& filter) const { return false; }
	virtual bool isSwitcher() const { return false; }
	virtual bool isEvent() const { return false; }
	virtual UnitActing* eventContextUnit() const { return 0; }
	
	virtual void checkEvent(const class Event& event) {}
	virtual void clear() {}
	virtual void clearContext() {} // вызывается после каждого круга сканирования
	virtual void writeInfo(XBuffer& buffer, string offset, bool debug) const;
	virtual bool allowable(bool forLogic) const { return true; }

	bool state() const { return state_; }
	int color() const {	return internalColor_; }

	Player& aiPlayer() const { xassert(aiPlayer_ && "В глобальном триггере использовано недопустимое условие"); return *aiPlayer_; }
    
	virtual void serialize(Archive& ar);

	static bool checkPlayer(const class Player* palyer1, const Player* player2, AIPlayerType playerType);
	static bool compare(int op1, int op2, CompareOperator op);
	static bool compare(float op1, float op2, CompareOperator op, float eps = FLT_EPS);
	
protected:
	bool inverted_; 
	bool state_; 
	int internalColor_;
	Player* aiPlayer_;

	virtual bool check() const { return false; }
	virtual bool check(UnitActing* unit) const { return check(); }

	friend class Trigger;
};

//-----------------------------
struct ConditionSwitcher : Condition // И/ИЛИ
{
	enum Type {
		AND, // И
		OR // ИЛИ
	};
	Type type;
	typedef vector<ShareHandle<Condition> > Conditions; 
	Conditions conditions; 

	ConditionSwitcher();

	void setPlayer(Player* aiPlayer);
	void clear();
	void clearContext();

	bool check() const;
	bool check(UnitActing* unit) const { xassert(0); return false; }
	bool checkDebug(UnitActing* unit);
	void checkEvent(const Event& event);

	bool isContext(ContextFilter& filter) const;
	bool isSwitcher() const { return true; } 
	bool isEvent() const;
	UnitActing* eventContextUnit() const;
	bool allowable(bool forLogic) const;

	void writeInfo(XBuffer& buffer, string offset, bool debug) const;
	void serialize(Archive& ar);

	void remove(Condition* condition);
	void replace(Condition* from, Condition* to);
	void add(Condition* condition);
};

//-----------------------------
struct Action : ShareHandleBase // Пустое действие, вставлять не надо!
{		
	enum AuxType {
		NORMAL_ACTION, 
		ONLY_IF_AI, 
		RESET_TO_START,
		RESET_TO_CURRENT
	};

	Action();
	virtual ~Action() {}

	void setPlayer(Player* player) { aiPlayer_ = player; }

	virtual bool isContext(ContextFilter& filter) const { return false; }
	virtual bool checkContextUnit(UnitActing* unit) const { return true; }
	virtual void setContextUnit(UnitActing* unit) {}
	virtual bool automaticCondition() const { return true; }

	virtual void activate() {}
	virtual bool workedOut() { return true; }

	const char* name() const;
	Player& aiPlayer() const { xassert(aiPlayer_ && "В глобальном триггере использовано недопустимое действие"); return *aiPlayer_; }

	virtual AuxType auxType() const { return NORMAL_ACTION; }

	virtual void serialize(Archive& ar) {}
	
	static const TriggerChain* triggerChain() { xassert(triggerChain_); return triggerChain_; } 
	static void setTriggerChain(const TriggerChain* triggerChain) { triggerChain_ = triggerChain; }

	virtual void clear() {}
	virtual void clearContext() {} // вызывается после каждого круга сканирования

private:
	Player* aiPlayer_;
	
	static const TriggerChain* triggerChain_;
};

//-----------------------------

enum ColorType
{
	STRATEGY_RED, // Красный
	STRATEGY_GREEN, // Зеленый
	STRATEGY_BLUE, // Синий
	STRATEGY_YELLOW, // Желтый
	STRATEGY_COLOR_0,
	STRATEGY_COLOR_1,
	STRATEGY_COLOR_2,
	STRATEGY_COLOR_3,
	STRATEGY_COLOR_4,
	STRATEGY_COLOR_5,
	STRATEGY_COLOR_MAX,
	LINK_TYPES_COUNT = STRATEGY_COLOR_MAX
};

struct TriggerLink // Связь
{
	enum Type {
		THIN, // Тонкая
		THICK // Толстая
	};

	TriggerLink();

	void activate(TriggerChain& triggerChain);
	void deactivate(TriggerChain& triggerChain);

	const char* triggerName() const { return triggerName_.c_str(); }
	void setTriggerName(const char* triggerName) { triggerName_ = triggerName; }
	
	bool active() const { return active_; }

	bool autoRestarted() const { return autoRestarted_ == THICK; }
	void setAutoRestarted(bool autoRestarted) { autoRestarted_ = autoRestarted ? THICK : THIN; }

	int colorType() const { return colorType_; }
	void setColorType(int type) { colorType_ = static_cast<ColorType>(type); }
   
	Vect2f parentPoint() const;
	Vect2f childPoint() const;

	void setParentPoint(const Vect2f& point);
	void setChildPoint(const Vect2f& point);

	void fixParentOffset();
	void fixChildOffset();

	void serialize(Archive& ar);

	Trigger* parent;
	Trigger* child;

	static Color4c colors[STRATEGY_COLOR_MAX];
	static Color4c debugColors[2];

private:
	string triggerName_;
	bool active_; 
	ColorType colorType_; 
	Type autoRestarted_;

	Vect2i parentOffset_;
	Vect2i childOffset_;

	friend class TriggerChain;
};

typedef vector<TriggerLink> OutcomingLinksList;
typedef vector<TriggerLink*> IncomingLinksList;

class ContextFilter
{
public:
	void addUnit(const AttributeBase* attr);
	void addSquad(const AttributeSquad* attr);

	UniqueVector<const AttributeBase*> attributes;
};

class Trigger // Триггер
{
public:
	enum State {
		SLEEPING, // Проверяет входные связи
		CHECKING, // Проверяет условия
		WORKING, // Выполняется
		DONE // Выполнен
	};

	enum {
		SIZE_X = 200,
		SIZE_Y = 30,
		OFFSET_X = 30
	};

	Trigger();

	void setPlayer(Player* player);
	void quant(TriggerChain& triggerChain);
	bool checkCondition();
	void checkEvent(const Event& event);
	void activate(TriggerChain& triggerChain);
	bool removeLinkByChild(Trigger* child);
	
	void initState(TriggerChain& triggerChain);
	void setState(State state, TriggerChain& triggerChain, int activateOffset = 0);

	const char* displayText() const;
	const char* debugDisplayText(bool debug) const;
	const char* debugDisplayTextAction() const;
	bool active() const;

	bool isChild(const Trigger& child) const;

	const char* name() const { return name_.c_str(); }
	void setName(const char* name);

	State state() const { return state_; }
	
	const Color4c& color() const {	return color_; }
	void setColor(const Color4c& color) { color_ = color;	}

	bool selected() const { return selected_; }
	void setSelected(bool selected) { selected_ = selected; }

	const Vect2i& cellIndex() const { return cellIndex_; }
	void setCellIndex(const Vect2i& cellIndex) { cellIndex_ = cellIndex; }

	Vect2f leftTop() const { return Vect2f(cellIndex_)*gridSizeSpaced(); }

	const OutcomingLinksList& outcomingLinks() const { return outcomingLinks_; }
	OutcomingLinksList& outcomingLinks() { return outcomingLinks_; }

	const IncomingLinksList& incomingLinks() const { return incomingLinks_; }
	IncomingLinksList& incomingLinks() { return incomingLinks_; }

	void serialize(Archive& ar);

	ShareHandle<Condition> condition; 
	ShareHandle<Action> action; 

	static Color4c debugColors[DONE + 1];
	static Vect2f gridSize() { return Vect2f(SIZE_X, SIZE_Y); }
	static Vect2f gridSizeSpaced() { return Vect2f(SIZE_X + OFFSET_X, SIZE_Y*2); }
	static const char* currentTriggerName() { return currentTriggerName_; }

private:
	string name_; 
	OutcomingLinksList outcomingLinks_; 
	IncomingLinksList incomingLinks_; 
	State state_; 
	int executionCounter_;
	Color4c color_;
	
	Vect2i cellIndex_;
	bool selected_;
	bool breakWhenActivate_;
	bool randomizeOutcomingLinks_;

	bool onlyIfAi_;
	bool isContext_;
	ContextFilter contextFilter_;
	int unitIndex_;
	int listIndex_;
	int playerIndex_;

	bool extendedScan_;
	int unitsPerQuant_;

	static const char* currentTriggerName_;

	friend TriggerChain;
};

//-----------------------------
struct TriggerEvent
{
	string event;
	string triggerName;
	Trigger::State state;

	TriggerEvent() {
		state = Trigger::SLEEPING;
	}

	void serialize(Archive& ar);
};

typedef vector<Trigger> TriggerList;

//-----------------------------
class TriggerChain // Стратегия игрока
{
public:
	typedef vector<TriggerEvent> TriggerEventList;

	string name;
	TriggerList triggers; // 0-й trigger - стартовый

	TriggerChain();
	
	void load(const char* name);
	void save(); // non-const
    void initialize();
	void setAIPlayerAndTriggerChain(Player* aiPlayer); 

	TriggerChain& operator=(const TriggerChain& rhs);
	bool operator==(const TriggerChain& rhs) const;

	void buildLinks();
	void quant(bool pause);
	void checkEvent(const Event& event);
	Trigger* find(const char* name);
	Trigger* findBySubString(const char* subString, int occurence = 0);

	string uniqueName(const char* name);
	void removeTrigger(int triggerIndex);
	void renameTrigger(const char* oldName, const char* newName);
	int triggerIndex(const Trigger& trigger) const;

	void setLogData(const TriggerEventList& data);
	const TriggerEventList& logData() const { return triggerEvents_; }
	bool isLogValid() const;

	int numLogRecords() const { return triggerEvents_.size(); }
	const char* logRecordText(int index) const { return triggerEvents_[index].event.c_str(); }
	void setLogRecord(int index);
	void addLogRecord(const Trigger& trigger, const char* event);

	void activateTrigger(Trigger* trigger);
	void deactivateTrigger(Trigger* trigger);

	void serializeProperties(Archive& ar);
	void serialize(Archive& ar);

	void restart(bool resetToStart);

	bool ignorePause() const { return checkType_ == IGNORE_PAUSE; }
	bool checkDeterminism() const { return checkType_ == LOGIC_TRIGGER; }

	enum CheckType {
		LOGIC_TRIGGER,
		INTERFACE_TRIGGER,
		IGNORE_PAUSE,
		NO_CHECK
	};

private:
	int version_;
	CheckType checkType_;

	TriggerEventList triggerEvents_;

	typedef vector<Trigger*> ActiveTriggers;
	ActiveTriggers activeTriggers_;
	ActiveTriggers activeTriggersEvent_;

	static int currentVersion_;
};

//-----------------------------
typedef ShareHandle<Action> ActionPtr;
typedef ShareHandle<Condition> ConditionPtr;

//-----------------------------

class MissionDescriptionForTrigger
{
public:
	MissionDescriptionForTrigger();
	void serialize(Archive& ar);
	class MissionDescription* operator()() const;

private:
	bool useLoadedMission_;
};

#endif //__TRIGGER_EXPORT_H__
