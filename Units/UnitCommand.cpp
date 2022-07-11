#include "StdAfx.h"
#include "UnitCommand.h"
#include "UnitInterface.h"

BEGIN_ENUM_DESCRIPTOR(CommandID, "CommandID")
REGISTER_ENUM(COMMAND_ID_POINT, "Идти в точку");
REGISTER_ENUM(COMMAND_ID_ATTACK, "Атаковать точку");
REGISTER_ENUM(COMMAND_ID_OBJECT, "Исследовать объект");
REGISTER_ENUM(COMMAND_ID_FIRE, "Стрелять в точку");
REGISTER_ENUM(COMMAND_ID_FIRE_OBJECT, "Стрелять в объект");
REGISTER_ENUM(COMMAND_ID_TALK, "Говорить речь");

REGISTER_ENUM(COMMAND_ID_UPGRADE, "Апгрейд");
REGISTER_ENUM(COMMAND_ID_PRODUCE, "Производство юнитов");
REGISTER_ENUM(COMMAND_ID_CANCEL_PRODUCTION, "Отмена производства юнитов");
REGISTER_ENUM(COMMAND_ID_PRODUCE_PARAMETER, "Производство параметров");
REGISTER_ENUM(COMMAND_ID_PRODUCTION_INC, "Заказать юнита в сквад");
REGISTER_ENUM(COMMAND_ID_PRODUCTION_DEC, "Удалить заказанного юнита из сквада");

REGISTER_ENUM(COMMAND_ID_SELF_ATTACK_MODE, "Сменить режим атаки");
REGISTER_ENUM(COMMAND_ID_WALK_ATTACK_MODE, "Сменить режим движения");
REGISTER_ENUM(COMMAND_ID_WEAPON_MODE, "Сменить режим оружия");
REGISTER_ENUM(COMMAND_ID_AUTO_TARGET_FILTER, "Сменить режим автоматического выбора целей");
REGISTER_ENUM(COMMAND_ID_AUTO_TRANSPORT_FIND, "Сменить режим автоматического поиска транспорта");
REGISTER_ENUM(COMMAND_ID_SET_FORMATION, "Установить формацию скваду");
REGISTER_ENUM(COMMAND_ID_SET_MAIN_UNIT, "Установить главного юнита в скваде");
REGISTER_ENUM(COMMAND_ID_SET_MAIN_UNIT_BY_INDEX, "Установить главного юнита в скваде по номеру");
REGISTER_ENUM(COMMAND_ID_MAKE_STATIC, "Включить одиночный режим");
REGISTER_ENUM(COMMAND_ID_MAKE_DYNAMIC, "Выключить одиночный режим");
REGISTER_ENUM(COMMAND_ID_CHANGE_MOVEMENT_MODE, "Бег/Ходьба/На корточках/Лежа");

REGISTER_ENUM(COMMAND_ID_CAMERA_FOCUS, "Позиционировать камеру на юнита");
REGISTER_ENUM(COMMAND_ID_CAMERA_MOVE, "Позиционировать камеру на якорь");
REGISTER_ENUM(COMMAND_ID_SELECT_SELF, "Заселектить себя");
REGISTER_ENUM(COMMAND_ID_DIRECT_CONTROL, "Прямое управление");
REGISTER_ENUM(COMMAND_ID_SYNDICAT_CONTROL, "Прямое управление имени Карла")
REGISTER_ENUM(COMMAND_ID_DIRECT_KEYS, "Клавиши прямого управления");

REGISTER_ENUM(COMMAND_ID_EXPLODE_UNIT, "Убей себя");
REGISTER_ENUM(COMMAND_ID_KILL_UNIT, "Убить юнита тихо");
REGISTER_ENUM(COMMAND_ID_UNINSTALL, "Деинсталлировать здание");

REGISTER_ENUM(COMMAND_ID_PUT_OUT_TRANSPORT, "Выгнать юнитов из транспорта");
REGISTER_ENUM(COMMAND_ID_PUT_UNIT_OUT_TRANSPORT, "Выгнать юнита из транспорта");

END_ENUM_DESCRIPTOR(CommandID)

UnitCommand::UnitCommand()
{
	commandID_ = COMMAND_ID_UPGRADE;
	commandData_ = 0;
	position_ = Vect3f::ZERO;
	angleZ_ = 0;
	shiftModifier_ = false;
	miniMap_ = false;
	unitIndex_ = 0;
}

UnitCommand::UnitCommand(CommandID commandID, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = Vect3f::ZERO;
	angleZ_ = 0;
	shiftModifier_ = false;
	miniMap_ = false;
	unitIndex_ = 0;
}

UnitCommand::UnitCommand(CommandID commandID, const Vect3f& position, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = position;
	angleZ_ = 0;
	shiftModifier_ = false;
	miniMap_ = false;
	unitIndex_ = 0;
}

UnitCommand::UnitCommand(CommandID commandID, UnitInterface* unit, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = Vect3f::ZERO;
	angleZ_ = 0;
	shiftModifier_ = false;
	miniMap_ = false;
	unitIndex_ = unit ? unit->unitID().index() : 0;
}

UnitCommand::UnitCommand(CommandID commandID, const AttributeBase* attribute, const Vect3f& position, UnitInterface* unit)
: attributeReference_(attribute)
{
	commandID_ = commandID;
	commandData_ = 0;
	position_ = position;
	angleZ_ = 0;
	shiftModifier_ = false;
	miniMap_ = false;
	unitIndex_ = unit ? unit->unitID().index() : 0;
}

UnitCommand::UnitCommand(CommandID commandID, UnitInterface* unit, const Vect3f& position, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = position;
	angleZ_ = 0;
	shiftModifier_ = false;
	miniMap_ = false;
	unitIndex_ = unit ? unit->unitID().index() : 0;
}

UnitInterface* UnitCommand::unit() const
{
	return safe_cast<UnitInterface*>(UnitID::get(unitIndex_));
}

bool UnitCommand::operator==(const UnitCommand& command) const 
{ 
	return commandID() == command.commandID()
	&& commandData() == command.commandData() 
	&& command.position().eq(position())
	&& attribute() == command.attribute()
	&& unitIndex_ == command.unitIndex_;
}

void UnitCommand::write(XBuffer& out) const
{
	unsigned char useFlag = 0;
	if(commandData_)
		if(commandData_ <= 255)
			useFlag |= USE_SHORT_DATA;
		else
			useFlag |= USE_DATA;
	if(!position_.eq(Vect3f::ZERO))
		useFlag |= USE_POSITION;
	if(unitIndex_)
		useFlag |= USE_UNIT;
	if(attributeReference_)
		useFlag |= USE_ATTRIBUTE;
	if(shiftModifier_)
		useFlag |= USE_SHIFT;
	if(fabsf(angleZ_) > FLT_EPS)
		useFlag |= USE_ANGLE;
	if(miniMap_)
		useFlag |= USE_MINIMAP;

	char commandID = commandID_;
	out.write(commandID);

	out.write(useFlag);
	if(useFlag & USE_SHORT_DATA)
		out.write((unsigned char)commandData_);
	else if(useFlag & USE_DATA)
		out.write(commandData_);
	if(useFlag & USE_POSITION)
		out.write(position_);
	if(useFlag & USE_UNIT)
		out.write(unitIndex_);
	if(useFlag & USE_ATTRIBUTE)
		out.write(attributeReference_);
	if(useFlag & USE_ANGLE)
		out.write(angleZ_);
}

void UnitCommand::read(XBuffer& in) 
{
	char commandID;
	in.read(commandID);
	commandID_ = (CommandID)commandID;
	
	unsigned char useFlag;
	in.read(useFlag);
	if(useFlag & USE_SHORT_DATA){
		unsigned char cdata;
		in.read(cdata);
		commandData_ = cdata;
	}
	else if(useFlag & USE_DATA)
		in.read(commandData_);
	if(useFlag & USE_POSITION)
		in.read(position_);
	if(useFlag & USE_UNIT)
		in.read(unitIndex_);
	if(useFlag & USE_ATTRIBUTE)
		in.read(attributeReference_);
	if(useFlag & USE_ANGLE)
		in.read(angleZ_);

	shiftModifier_ = (useFlag & USE_SHIFT) != 0;
	miniMap_ = (useFlag & USE_MINIMAP) != 0;
}

void UnitCommand::serialize(Archive& ar)
{
	AutoAttackMode unitAttackMode;
	WalkAttackMode unitWalkMode;
	WeaponMode unitWeaponMode;
	AutoTargetFilter unitTargetFilter;
	MovementMode movementMode;
	ar.serialize(commandID_, "commandID", "&Команда");
	switch(commandID_){
		case COMMAND_ID_SELF_ATTACK_MODE:
			unitAttackMode = (AutoAttackMode)commandData_;
			ar.serialize(unitAttackMode, "commandData", "Режим атаки");
			commandData_ = unitAttackMode;
			break;
		case COMMAND_ID_WALK_ATTACK_MODE:
			unitWalkMode = (WalkAttackMode)commandData_;
			ar.serialize(unitWalkMode, "commandData", "Режим движения");
			commandData_ = unitWalkMode;
			break;
		case COMMAND_ID_WEAPON_MODE:
			unitWeaponMode = (WeaponMode)commandData_;
			ar.serialize(unitWeaponMode, "commandData", "Режим оружия");
			commandData_ = unitWeaponMode;
			break;
		case COMMAND_ID_WEAPON_ACTIVATE:
			ar.serialize(commandData_, "weaponID", 0);
			ar.serialize(shiftModifier_, "shiftModifier", "Однократное включение");
			break;
		case COMMAND_ID_AUTO_TARGET_FILTER:
			unitTargetFilter = (AutoTargetFilter)commandData_;
			ar.serialize(unitTargetFilter, "commandData", "Режим автоматического выбора целей");
			commandData_ = unitTargetFilter;
			break;
		case COMMAND_ID_AUTO_TRANSPORT_FIND:{
			bool setAutoFind = commandData_;
			ar.serialize(setAutoFind, "commandData", "Искать транспорт автоматически");
			commandData_ = setAutoFind;
			break;
											}
		case COMMAND_ID_UPGRADE:
			ar.serialize(commandData_, "commandData", "Номер апгрейда");
			break;
		case COMMAND_ID_PRODUCE:
			ar.serialize(commandData_, "commandData", "Номер производства");
			ar.serialize(shiftModifier_, "shiftModifier", "Заказать много");
			break;
		case COMMAND_ID_PRODUCE_PARAMETER:
			ar.serialize(commandData_, "commandData", "Номер производства");
			break;
		case COMMAND_ID_SET_FORMATION:
			ar.serialize(commandData_, "commandData", "Номер формации");
			break;
		case COMMAND_ID_SET_MAIN_UNIT_BY_INDEX:
			ar.serialize(commandData_, "commandData", "Номер юнита в скваде");
			break;
		case COMMAND_ID_SET_MAIN_UNIT:
		case COMMAND_ID_PRODUCTION_INC:
		case COMMAND_ID_PRODUCTION_DEC:{
			AttributeUnitReference reference = attributeReference_;
			ar.serialize(reference, "attribute", "Юнит");
			attributeReference_ = reference;
			break; }
		//case COMMAND_ID_OBJECT:
		//	ar.serialize(unitIndex_, "unitID", 0);
		case COMMAND_ID_FIRE:
		case COMMAND_ID_ATTACK:
		case COMMAND_ID_WEAPON_DEACTIVATE:
			ar.serialize(commandData_, "weaponID", 0);
			break;
		case COMMAND_ID_TALK:
			ar.serialize(commandData_, "commandData", "Номер цепочки");
			break;
		case COMMAND_ID_CHANGE_MOVEMENT_MODE:
			movementMode  = (MovementMode)commandData_;
			if(ar.serialize(movementMode, "commandData", "Режим движения"))
				commandData_ = movementMode;
			else
				commandData_ = MODE_WALK;
			break;
	}
}

bool UnitCommand::isUnitValid() const 
{
	if(!unitIndex_)
		return true;
	return unit();
}


