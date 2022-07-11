#ifndef __UNIT_COMMAND_H__
#define __UNIT_COMMAND_H__

#include "AttributeReference.h"
#include "UnitID.h"

class AttributeBase;
class UnitInterface;
class UI_ControlBase;

enum CommandID
{
	COMMAND_ID_NONE = 0,

	//---------------------------
	//	Scalar 
	COMMAND_ID_EXPLODE_UNIT,
	COMMAND_ID_EXPLODE_UNIT_DEBUG,
	COMMAND_ID_KILL_UNIT,

	COMMAND_ID_STOP,
	COMMAND_ID_UPGRADE,
	COMMAND_ID_PRODUCE,
	COMMAND_ID_PRODUCE_PARAMETER,
	COMMAND_ID_CANCEL_PRODUCTION, // commandData

	COMMAND_ID_GO_RUN,
	COMMAND_ID_STOP_RUN,

	COMMAND_ID_PUT_OUT_TRANSPORT,
	COMMAND_ID_PUT_UNIT_OUT_TRANSPORT, // commandData

	// Buildings
	COMMAND_ID_POWER_ON,
	COMMAND_ID_POWER_OFF,
	COMMAND_ID_UNINSTALL,
	COMMAND_ID_HOLD_CONSTRUCTION,
	COMMAND_ID_CONTINUE_CONSTRUCTION,
	COMMAND_ID_CANCEL_UPGRADE,
	
	COMMAND_ID_PRODUCTION_INC, // attribute()
	COMMAND_ID_PRODUCTION_DEC, // attribute()
	COMMAND_ID_PRODUCTION_PAUSE_ON, // attribute()
	COMMAND_ID_PRODUCTION_PAUSE_OFF, // attribute()

	// Squad
	COMMAND_ID_SET_FORMATION,
	COMMAND_ID_SELF_ATTACK_MODE,
	COMMAND_ID_WALK_ATTACK_MODE,
	COMMAND_ID_WEAPON_MODE,
	COMMAND_ID_AUTO_TARGET_FILTER,
	COMMAND_ID_AUTO_TRANSPORT_FIND,

	//---------------------------
	// Point
	COMMAND_ID_POINT,

	COMMAND_ID_BUILDING_START,
	COMMAND_ID_PATROL,
	COMMAND_ID_ATTACK,
	
	//---------------------------
	// Object
	COMMAND_ID_OBJECT,
	
	COMMAND_ID_ADD_SQUAD,
	COMMAND_ID_SPLIT_SQUAD,
	COMMAND_ID_FOLLOW_SQUAD,
	COMMAND_ID_BUILD,

	//---------------------------
	// Weapon
	COMMAND_ID_WEAPON_ACTIVATE,

	//---------------------------
	// Camera
	COMMAND_ID_CAMERA_FOCUS,

	//---------------------------
	// Прямое управление
	COMMAND_ID_DIRECT_CONTROL,
	COMMAND_ID_SYNDICAT_CONTROL,
	COMMAND_ID_DIRECT_KEYS,
	COMMAND_ID_DIRECT_CHANGE_WEAPON,
	COMMAND_ID_DIRECT_PUT_IN_TRANSPORT,
	COMMAND_ID_DIRECT_PUT_OUT_TRANSPORT,

	//---------------------------
	// прямое наведение
	COMMAND_ID_DIRECT_SHOOT,
	COMMAND_ID_DIRECT_SHOOT_MOUSE,

	//---------------------------
	// Инвентарь
	/// удалить предмет
	COMMAND_ID_ITEM_REMOVE,
	/// выкинуть предмет на мир
	COMMAND_ID_ITEM_DROP,
	/// активировать предмет
	COMMAND_ID_ITEM_ACTIVATE,
	/// вынуть предмет на мышь
	COMMAND_ID_ITEM_TAKE,
	/// положить предмет обратно
	COMMAND_ID_ITEM_RETURN,
	/// выкинуть вынутый предмет на мир
	COMMAND_ID_ITEM_TAKEN_DROP,

	// Командный режим
	COMMAND_ID_UNIT_SELECTED, // cooperativeIndex
	COMMAND_ID_UNIT_DESELECTED,
};

class UnitCommand
{
public:
	UnitCommand();
	UnitCommand(CommandID commandID, int commandData = 0);
	UnitCommand(CommandID commandID, const Vect3f& position, int commandData = 0);
	UnitCommand(CommandID commandID, class UnitInterface* unit, int commandData = 0);
	UnitCommand(CommandID commandID, const AttributeBase* attribute, const Vect3f& position, UnitInterface* unit);
	UnitCommand(CommandID commandID, UnitInterface* unit, const Vect3f& position, int commandData);
	
	CommandID commandID() const { return commandID_; }
	
	void setPosition(const Vect3f& pos) { position_ = pos; }
	const Vect3f& position() const { return position_; }
	UnitInterface* unit() const; // проверять на 0, т.к. объект может погибнуть, пока идет команда
	const AttributeBase* attribute() const { return attributeReference_; }
	bool isUnitValid() const; 
	
	bool shiftModifier() const { return shiftModifier_; } // Для isSuspendCommand()-команд означает постановку в очередь, для других - дополнительный модификатор
	void setShiftModifier(bool flag = true) { shiftModifier_ = flag; }
	
	int commandData() const { return commandData_; }
	
	bool operator==(const UnitCommand& command) const;

	void write(XBuffer& out) const;
	void read(XBuffer& in);

	void serialize(Archive& ar);

private:
	CommandID commandID_;
	Vect3f position_;
	int unitIndex_;
	unsigned int commandData_;
	AttributeReference attributeReference_;
	bool shiftModifier_;

	enum UseFlag {
		USE_DATA = 1,
		USE_POSITION = 2,
		USE_UNIT = 4,
		USE_ATTRIBUTE = 8,
		USE_SHIFT = 16
	};
};

#endif //__UNIT_COMMAND_H__
