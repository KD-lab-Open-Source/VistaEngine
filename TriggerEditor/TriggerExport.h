#ifndef __TRIGGER_EXPORT_H__
#define __TRIGGER_EXPORT_H__

#include <atltypes.h>
#include "Handle.h"
#include "SerializationTypes.h"
#include "Timers.h"

//-----------------------------
class Player;
class TriggerChain;
class Trigger;
class Archive;
class UnitReal;
class UnitActing;

enum CompareOperator;

enum ExtendedPlayerType
{
	EXTENDED_PLAYER_MY_CLAN, // мой клан
	EXTENDED_PLAYER_ENEMY, // враг
	EXTENDED_PLAYER_ANY // любой
};

//-----------------------------
enum AIPlayerType
{
	AI_PLAYER_TYPE_ME, // Я
	AI_PLAYER_TYPE_ENEMY, // Другой
	AI_PLAYER_TYPE_WORLD, // Мир
	AI_PLAYER_TYPE_ANY // Любой
};

//-----------------------------
template <class T>
class TriggerAllocator {
public:

  typedef T        value_type;
  typedef value_type *       pointer;
  typedef const T* const_pointer;
  typedef T&       reference;
  typedef const T& const_reference;
  typedef size_t     size_type;
  typedef ptrdiff_t  difference_type;

  template <class _Tp1> struct rebind {
    typedef TriggerAllocator<_Tp1> other;
  };
  TriggerAllocator() {}
  TriggerAllocator(const allocator<T>&) {}
  ~TriggerAllocator() {}
  
  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }
  
  T* allocate(size_type n, const void* = 0) { 
    return n != 0 ? reinterpret_cast<value_type*>(triggerInterface().malloc(n * sizeof(value_type))) : 0;
  }
  
  void deallocate(pointer p, size_type n) {
	  //xassert( (p == 0) == (n == 0) );
      if (p != 0) 
		  triggerInterface().free(p);
  }

  size_type max_size() const
    { return size_t(-1) / sizeof(T); }
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
	bool checkDebug(UnitActing* unit);

	virtual bool isContext() const { return false; }
	virtual bool isSwitcher() const { return false; }
	virtual bool isEvent() const { return false; }
	virtual UnitActing* eventContextUnit() const { return 0; }
	
	virtual void checkEvent(const class Event& event) {}
	virtual void clear() {}
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

	static bool scanWait_;
	
	virtual bool check() const { return false; }
	virtual bool check(UnitActing* unit) const { return check(); }

	friend class Trigger;
};

//-----------------------------
struct Action : ShareHandleBase // Пустое действие, вставлять не надо!
{		
	enum AuxType {
		NORMAL_ACTION, 
		RESET_TO_START,
		RESET_TO_CURRENT
	};

	Action();
	virtual ~Action() {}

	void setPlayer(Player* player) { aiPlayer_ = player; }

	virtual bool isContext() const { return false; }
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

private:
	Player* aiPlayer_;
	
	static const TriggerChain* triggerChain_;
};

//-----------------------------
struct CPointSerialized : CPoint
{
	CPointSerialized() {
		x = y = INT_MIN;
	}

	bool valid() const {
 		return x != INT_MIN || y != INT_MIN;
	}

	void serialize(Archive& ar);
};

struct CRectSerialized : CRect
{
	CRectSerialized() {
		left = top = right = bottom = INT_MIN;
	}

	bool valid() const {
		return left != INT_MIN || right != INT_MIN || bottom != INT_MIN || top != INT_MIN;
	}

	void serialize(Archive& ar);
};

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

	const char* triggerName() const {
		return triggerName_.c_str();
	}
	void setTriggerName(const char* triggerName) {
		triggerName_ = triggerName;
	}
	
	bool active() const {
		return active_;
	}

	bool autoRestarted() const { 
		return autoRestarted_ == THICK; 
	}
	void setAutoRestarted(bool autoRestarted) {
		autoRestarted_ = autoRestarted ? THICK : THIN;
	}

	int getType() const {
		return colorType_;
	}
	void setType(int type)
	{
		colorType_ = static_cast<ColorType>(type);
	}

	const CPointSerialized& parentOffset() const {
		return parentOffset_;
	}
	void setParentOffset(const CPoint& offset) {
		static_cast<CPoint&>(parentOffset_) = offset;
	}
    
	const CPointSerialized& childOffset() const {
		return childOffset_;
	}
	void setChildOffset(const CPoint& offset) {
		static_cast<CPoint&>(childOffset_) = offset;
	}

	void serialize(Archive& ar);

	Trigger* parent;
	Trigger* child;

private:
	string triggerName_;
	bool active_; 
	ColorType colorType_; 
	Type autoRestarted_;

	CPointSerialized parentOffset_;
	CPointSerialized childOffset_;

	friend class TriggerChain;
};

typedef vector<TriggerLink, TriggerAllocator<TriggerLink> > OutcomingLinksList;
typedef vector<TriggerLink*, TriggerAllocator<TriggerLink*> > IncomingLinksList;

class Trigger // Триггер
{
public:
	enum State {
		SLEEPING, // Проверяет входные связи
		CHECKING, // Проверяет условия
		WORKING, // Выполняется
		DONE // Выполнен
	};

	Trigger();

	void setPlayer(Player* player);
	void quant(TriggerChain& triggerChain);
	bool checkCondition();
	Player* getExtendedPlayer();
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

	const char* name() const {
		return name_.c_str();
	}
	void setName(const char* name);

	State state() const { return state_; }
	
	int color() const {	return internalColor_; }
	void setColor(int color) { internalColor_ = color;	}

	bool selected() const {
		return selected_;
	}
	void setSelected(bool selected) {
		selected_ = selected;
	}

	const CPointSerialized& cellIndex() const {
		return cellIndex_;
	}
	void setCellIndex(int x, int y) {
		cellIndex_.SetPoint(x, y);
	}
	void setCellIndex(const CPoint& cellIndex) {
		static_cast<CPoint&>(cellIndex_) = cellIndex;
	}

	const CRectSerialized& boundingRect() const {
		return boundingRect_;
	}
	void setBoundingRect(const CRect& boundingRect) {
		static_cast<CRect&>(boundingRect_) = boundingRect;
	}

	const OutcomingLinksList& outcomingLinks() const {
		return outcomingLinks_;
	}
	OutcomingLinksList& outcomingLinks() {
		return outcomingLinks_;
	}

	const IncomingLinksList& incomingLinks() const {
		return incomingLinks_;
	}
	IncomingLinksList& incomingLinks() {
		return incomingLinks_;
	}

	void serialize(Archive& ar);

	ShareHandle<Condition> condition; 
	ShareHandle<Action> action; 

private:
	string name_; 
	OutcomingLinksList outcomingLinks_; 
	IncomingLinksList incomingLinks_; 
	State state_; 
	int executionCounter_;
	int internalColor_;
	
	CPointSerialized cellIndex_;
	CRectSerialized boundingRect_;
	bool selected_;
	bool breakWhenActivate_;
	bool randomizeOutcomingLinks_;

	bool onlyIfAi_;
	bool isContext_;
	int indexScan_;
	int indexScanPlayer_;
    DurationTimer delayTimer_;

	bool extendedScan_;
	ExtendedPlayerType extendedPlayerType_;
	int unitsPerQuant_;

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

typedef vector<Trigger, TriggerAllocator<Trigger> > TriggerList;

//-----------------------------
class TriggerChain // Стратегия игрока
{
public:
	typedef vector<TriggerEvent, TriggerAllocator<TriggerEvent> > TriggerEventList;

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

	bool removeLink(int parentIndex, int childIndex);
	TriggerLink* addLink(int parentIndex, int childIndex, int type);

	Trigger* addTrigger(Trigger const& trigger);
	Trigger* insertTrigger(int pos, Trigger const& triger);
	void removeTrigger(int triggerIndex);
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

	const CRectSerialized& boundingRect() const {
		return boundingRect_;
	}
	void setBoundingRect(const RECT& boundingRect) {
		static_cast<RECT&>(boundingRect_) = boundingRect;
	}

	const CRectSerialized& viewRect() const {
		return viewRect_;
	}
	void setViewRect(const RECT& viewRect) {
		static_cast<RECT&>(viewRect_) = viewRect;
	}

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

	CRectSerialized boundingRect_;
	CRectSerialized viewRect_;

	TriggerEventList triggerEvents_;

	typedef vector<Trigger*, TriggerAllocator<Trigger*> > ActiveTriggers;
	ActiveTriggers activeTriggers_;
	ActiveTriggers activeTriggersEvent_;

	static int currentVersion_;
};

//-----------------------------
typedef ShareHandle<Action> ActionPtr;
typedef ShareHandle<Condition> ConditionPtr;

class AttribEditorInterface;

//-----------------------------
class TriggerInterface
{
public:
	virtual const char* actionComboList() = 0;  // |-separated
	virtual const char* conditionComboList() = 0; // |-separated
	virtual ActionPtr createAction(int typeIndex) = 0; // index in actionComboList
	virtual ConditionPtr createCondition(int typeIndex) = 0; // index in conditionComboList
	virtual ConditionPtr createConditionSwitcher() = 0;
	virtual void copyTrigger(Trigger& destination, const Trigger& source) = 0;
	
	virtual const char* actionName(const Action& action) = 0;  
	virtual const char* conditionName(const Condition& condition) = 0;  


	virtual AttribEditorInterface& attribEditorInterface() = 0;
	virtual bool editCondition(Trigger& trigger, HWND hwnd) = 0;
	virtual bool editAction(Trigger& trigger, HWND hwnd) = 0;
	virtual bool editTrigger(Trigger& trigger, HWND hwnd) = 0;

	virtual void* malloc(size_t n) = 0;
	virtual void free(void* p) = 0;
};

TriggerInterface& triggerInterface();

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
