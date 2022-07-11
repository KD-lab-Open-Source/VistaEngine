#ifndef _PERIMETER_TRIGGERS_
#define _PERIMETER_TRIGGERS_

#include <functional>

#include "Handle.h"

typedef unsigned short int ActionMode;
typedef unsigned char ActionModeModifer;

class UnitBase;
class AttributeBase;
class Player;
class SourceAttribute;
class UI_ControlBase;
class UnitInterface;
class SourceBase;

template<class ReferenceList>
bool removeZeros(ReferenceList& referenceList) 
{
	ReferenceList::iterator i = std::remove_if(referenceList.begin(), referenceList.end(), logical_not<ReferenceList::value_type>());
	if(i != referenceList.end()){
		referenceList.erase(i, referenceList.end());
		return true;
	}
	return false;
}

template<class ReferenceList, class Element> // стандартный find будет использовать дорогой конструктор
bool findReference(const ReferenceList& referenceList, const Element* element) 
{
	ReferenceList::const_iterator i;
	FOR_EACH(referenceList, i)
		if(*i == element)
			return true;
	return false;
}

enum RequestResourceType
{
	NEED_RESOURCE_SILENT_CHECK = 0,
	NEED_RESOURCE_TO_ACCESS_UNIT_OR_BUILDING = 1,
	NEED_RESOURCE_TO_INSTALL_BUILDING = 1 << 1,
	NEED_RESOURCE_TO_BUILD_BUILDING = 1 << 2,
	NEED_RESOURCE_TO_PRODUCE_UNIT = 1 << 3,

	NEED_RESOURCE_TO_ACCESS_PARAMETER = 1 << 4,
	NEED_RESOURCE_TO_PRODUCE_PARAMETER = 1 << 5,

	NEED_RESOURCE_TO_UPGRADE = 1 << 6,
	NEED_RESOURCE_TO_MOVE = 1 << 7,

	NEED_RESOURCE_TO_ACCESS_WEAPON = 1 << 8,
	NEED_RESOURCE_TO_FIRE = 1 << 9,
};

enum UnitsConstruction { // Состояние объекта (построен или строится)
	CONSTRUCTED = 1,
	CONSTRUCTING = 2
};

enum UnitsTransformation { // Состояние объекта
	UNIT_STATE_CONSTRUCTED = 1,
	UNIT_STATE_CONSTRUCTING = 2,
	UNIT_STATE_UPGRADING = 4
};


//-------------------------------

class Event : public PolymorphicBase
{
public:
	enum Type {
		CREATE_SOURCE,
		ACTIVATE_SOURCE,
		CREATE_OBJECT,
		PRODUCE_UNIT_IN_SQUAD,
		AIM_AT_OBJECT,
		ATTACK_OBJECT,
		COMPLETE_CURE,
		KILL_OBJECT,
		DESTROY_OBJECT,
		TELEPORTATION,
		CAPTURE_UNIT, 
		PLAYER_STATE, 
		TIME,
		STARTED_PRODUCTION,
		STARTED_PRODUCTION_PARAMETER,
		STARTED_BUILDING,
		SOLD_BUILDING,
		COMPLETE_BUILDING,
		STARTED_UPGRADE,
		COMPLETE_UPGRADE,
		KEYBOARD_CLICK, 
		UI_BUTTON_CLICK,
		UI_BUTTON_CLICK_LOGIC,
		UI_MINIMAP_ACTION_CLICK,
		UI_BUTTON_FOCUS_ON,
		UI_BUTTON_FOCUS_OFF,
		COMMAND_MOVE,
		COMMAND_ATTACK,
		UNIT_ATTACKING,
		UNABLE_BUILD,
		LACK_OF_RESOURCE,
		PRODUCED_PARAMETER,
		ADD_RESOURCE,
		SUB_RESOURCE,
		SELECT_UNIT,
		END_REPLAY,
		NETWORK_DISCONNECT,
		RESET_PROFILE,
		CHANGE_ACTIVE_PLAYER,
		CHANGE_PLAYER_AI,
		GAME_CLOSE,
		STRING, 
		PICK_ITEM,
		REQUEST_ASSEMBLY
	};
	Event(Type type) : type_(type) {}
	Type type() const { return type_; }
	virtual ~Event(){}

	Event(XBuffer& buffer) { buffer.read(type_); }
	virtual void writeNet(XBuffer& buffer) const { buffer.write(type_); }

	static Event* create(XBuffer& buffer);

protected:
	Type type_;
};

typedef PolymorphicHandle<const Event> EventHandle;

class EventParameter : public Event
{
public:
	EventParameter(Type type, const char* signalVariable) : Event(type), signalVariable_(signalVariable) {} 
	const char* signalVariable() const { return signalVariable_; }
private:
	const char* signalVariable_;
};

class EventUnitPlayer : public Event
{
public:
	EventUnitPlayer(Type type, const UnitBase* unit, const Player* player) : Event(type), unit_(unit), player_(player) {}
	const UnitBase* unit() const { return unit_; }
	const Player* player() const { return player_; }

private:
	const UnitBase* unit_;
	const Player* player_;
};

class EventUnitPlayerInt : public Event
{
public:
	EventUnitPlayerInt(Type type, const UnitBase* unit, const Player* player, int id) : Event(type), unit_(unit), player_(player), id_(id) {}
	const UnitBase* unit() const { return unit_; }
	const Player* player() const { return player_; }
	int id() const { return id_;}

private:
	const UnitBase* unit_;
	const Player* player_;
	int id_;
};

class EventUnitsPlayer : public Event
{
public:
	typedef vector<UnitInterface*> UnitInterfaceList;
	EventUnitsPlayer(Type type, const UnitInterfaceList& units, const Player* player) : Event(type), units_(units), player_(player) 
	{
		units_.reserve(units.size());
		UnitInterfaceList::const_iterator i;
		FOR_EACH(units, i)
			units_.push_back((*i));
	}
	const UnitInterfaceList& units() const { return units_; }
	const Player* player() const { return player_; }

private:
	UnitInterfaceList units_;
	const Player* player_;
};

class EventUnitAttributePlayer : public Event
{
public:
	EventUnitAttributePlayer(Type type, const AttributeBase* unitAttr, const Player* player) : Event(type), unitAttr_(unitAttr), player_(player) {}
	const AttributeBase& unitAttr() const { return *unitAttr_; }
	const Player* player() const { return player_; }

private:
	const AttributeBase* unitAttr_;
	const Player* player_;
};

class EventUnitUnitAttributePlayer : public Event
{
public:
	EventUnitUnitAttributePlayer(Type type, const AttributeBase* unitAttr, const UnitBase* unit, const Player* player) : 
	  Event(type), unitAttr_(unitAttr), unit_(unit), player_(player) {}
	const AttributeBase& unitAttr() const { return *unitAttr_; }
	const UnitBase* unit() const { return unit_; }
	const Player* player() const { return player_; }

private:
	const AttributeBase* unitAttr_;
	const UnitBase* unit_;
	const Player* player_;
};

class EventSource : public Event
{
public:
	EventSource(Type type, const SourceAttribute* attrSource) : Event(type), attrSource_(attrSource) {}
	const SourceAttribute* attrSource() const { return attrSource_; }

private:
	const SourceAttribute* attrSource_;
};

class EventSourceBase : public Event
{
public:
	EventSourceBase(Type type, const SourceBase* _source) : Event(type), source_(_source) {}
	const SourceBase* source() const { return source_; }

private:
	const SourceBase* source_;
};

class EventUnitMyUnitEnemy : public Event
{
public:
	EventUnitMyUnitEnemy(Type type, const UnitBase* unitMy, const UnitBase* unitEnemy) : Event(type), unitMy_(unitMy), unitEnemy_(unitEnemy) {}
	const UnitBase* unitMy() const { return unitMy_; }
	const UnitBase* unitEnemy() const { return unitEnemy_; }

private:
	const UnitBase* unitMy_;
	const UnitBase* unitEnemy_;
};

enum SwitchMode {  
	OFF = 0, // Выключен
	ON = 1 // Включен
};

enum SwitchModeTriple {
	MODE_OFF,
	MODE_ON,
	MODE_RESTORE
};

enum SquadMoveMode {
	WAIT_FOR_ONE, // хотя бы один
	WAIT_FOR_ALL, // все
	DO_NOT_WAIT   // не ждать
};

class EventTime : public Event
{
public:
	EventTime(int time) : Event(TIME), time_(time) {}
	int time() const { return time_; }

private:
	int time_;
};

class EventKeyBoardClick : public Event
{
public:
	EventKeyBoardClick(Type type, int fullKey) : Event(type), fullKey_(fullKey) {}
	
	EventKeyBoardClick(XBuffer& buffer) : Event(buffer) { buffer.read(fullKey_); }
	void writeNet(XBuffer& buffer) const { Event::writeNet(buffer); buffer.write(fullKey_); }

	int fullKey() const { return fullKey_; }

private:
	int fullKey_;
};

class EventButton : public Event
{
public:
	EventButton(Type type, int playerID, const UI_ControlBase* control) : Event(type), playerID_(playerID), control_(control) {}

	EventButton(XBuffer& buffer);
	void writeNet(XBuffer& buffer) const;

	bool checkPlayerID(int playerID) const { return playerID_ == -1 || playerID_ == playerID; }
	const UI_ControlBase* control() const { return control_; }

private:
	int playerID_;
	const UI_ControlBase* control_;
};

class EventNet : public Event
{
public:
	EventNet(Type type, int data) : Event(type), data_(data) {}

	EventNet(XBuffer& buffer);
	void writeNet(XBuffer& buffer) const;

	bool isHard() const { return data_ != 0; }

private:
	int data_;
};

class EventButtonClick : public EventButton
{
public:
	EventButtonClick(Type type, int playerID, const UI_ControlBase* control, ActionMode inputEvents, ActionModeModifer modifiers, bool enabled)
		: EventButton(type, playerID, control)
		, inputEvents_(inputEvents)
		, modifiers_(modifiers)
		, enabled_(enabled)
	{}

	EventButtonClick(XBuffer& buffer);
	void writeNet(XBuffer& buffer) const;

	int inputEvents() const { return inputEvents_; }
	char modifiers() const { return modifiers_; }
	bool enabled() const { return enabled_; }

private:
	ActionMode inputEvents_;
	ActionModeModifer modifiers_;
	bool enabled_;
};

class EventMinimapClick : public Event
{
public:
	EventMinimapClick(Type type, const Vect2f& crd) : Event(type), crd_(crd) {}
	const Vect2f& coord() const { return crd_; }

private:
	Vect2f crd_;

};

class EventChangePlayerAI : public Event
{
public:
	EventChangePlayerAI(Type type, int playerID) : Event(type), playerID_(playerID) {}

	EventChangePlayerAI(XBuffer& buffer);
	void writeNet(XBuffer& buffer) const;

	int playerID() const { return playerID_; }

private:
	int playerID_;
};

class EventStringPlayer : public Event
{
public:
	EventStringPlayer(const char* name, int playerID) : Event(STRING), name_(name), playerID_(playerID) {}
	const char* name() const { return name_.c_str(); }
	int playerID() const { return playerID_; }

	EventStringPlayer(XBuffer& buffer);
	void writeNet(XBuffer& buffer) const;

private:
	string name_;
	int playerID_;
};

#endif
