#include "StdAfx.h"
#include "UnitCommand.h"
#include "UnitInterface.h"

BEGIN_ENUM_DESCRIPTOR(CommandID, "CommandID")
REGISTER_ENUM(COMMAND_ID_UPGRADE, "Апгрейд");
REGISTER_ENUM(COMMAND_ID_SELF_ATTACK_MODE, "Сменить режим атаки");
REGISTER_ENUM(COMMAND_ID_WALK_ATTACK_MODE, "Сменить режим движения");
REGISTER_ENUM(COMMAND_ID_WEAPON_MODE, "Сменить режим оружия");
REGISTER_ENUM(COMMAND_ID_AUTO_TARGET_FILTER, "Сменить режим автоматического выбора целей");
REGISTER_ENUM(COMMAND_ID_AUTO_TRANSPORT_FIND, "Сменить режим автоматического поиска транспорта");
REGISTER_ENUM(COMMAND_ID_PRODUCE, "Производство юнитов");
REGISTER_ENUM(COMMAND_ID_CANCEL_PRODUCTION, "Отмена производства юнитов");
REGISTER_ENUM(COMMAND_ID_PRODUCE_PARAMETER, "Производство параметров");
REGISTER_ENUM(COMMAND_ID_SET_FORMATION, "Установить формацию скваду");
REGISTER_ENUM(COMMAND_ID_GO_RUN, "Перейти на бег");
REGISTER_ENUM(COMMAND_ID_STOP_RUN, "Перейти на ходьбу");
REGISTER_ENUM(COMMAND_ID_PRODUCTION_INC, "Заказать юнита в сквад");
REGISTER_ENUM(COMMAND_ID_PRODUCTION_DEC, "Удалить заказанного юнита из сквада");
REGISTER_ENUM(COMMAND_ID_PRODUCTION_PAUSE_ON, "Заморозить производство юнита в сквад");
REGISTER_ENUM(COMMAND_ID_PRODUCTION_PAUSE_OFF, "Продолжить производство юнита в сквад");
REGISTER_ENUM(COMMAND_ID_PUT_OUT_TRANSPORT, "Выгнать юнитов из транспорта");
REGISTER_ENUM(COMMAND_ID_PUT_UNIT_OUT_TRANSPORT, "Выгнать юнита из транспорта");
REGISTER_ENUM(COMMAND_ID_CAMERA_FOCUS, "Позиционировать камеру на юнита");
REGISTER_ENUM(COMMAND_ID_EXPLODE_UNIT, "Убей себя");
REGISTER_ENUM(COMMAND_ID_KILL_UNIT, "Убить юнита тихо");
REGISTER_ENUM(COMMAND_ID_DIRECT_CONTROL, "Прямое управление");
REGISTER_ENUM(COMMAND_ID_SYNDICAT_CONTROL, "Прямое управление имени Карла")
REGISTER_ENUM(COMMAND_ID_DIRECT_KEYS, "Клавиши прямого управления");
REGISTER_ENUM(COMMAND_ID_UNINSTALL, "Деинсталлировать здание");
REGISTER_ENUM(COMMAND_ID_POINT, "Идти в точку");
REGISTER_ENUM(COMMAND_ID_ATTACK, "Атаковать точку");
REGISTER_ENUM(COMMAND_ID_OBJECT, "Исследовать объект");
END_ENUM_DESCRIPTOR(CommandID)

UnitCommand::UnitCommand()
{
	commandID_ = COMMAND_ID_UPGRADE;
	commandData_ = 0;
	position_ = Vect3f::ZERO;
	shiftModifier_ = false;
	unitIndex_ = 0;
}

UnitCommand::UnitCommand(CommandID commandID, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = Vect3f::ZERO;
	shiftModifier_ = false;
	unitIndex_ = 0;
}

UnitCommand::UnitCommand(CommandID commandID, const Vect3f& position, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = position;
	shiftModifier_ = false;
	unitIndex_ = 0;
}

UnitCommand::UnitCommand(CommandID commandID, UnitInterface* unit, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = Vect3f::ZERO;
	shiftModifier_ = false;
	unitIndex_ = unit ? unit->unitID().index() : 0;
}

UnitCommand::UnitCommand(CommandID commandID, const AttributeBase* attribute, const Vect3f& position, UnitInterface* unit)
: attributeReference_(attribute)
{
	commandID_ = commandID;
	commandData_ = 0;
	position_ = position;
	shiftModifier_ = false;
	unitIndex_ = unit ? unit->unitID().index() : 0;
}

UnitCommand::UnitCommand(CommandID commandID, UnitInterface* unit, const Vect3f& position, int commandData)
{
	commandID_ = commandID;
	commandData_ = commandData;
	position_ = position;
	shiftModifier_ = false;
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
	char useFlag = 0;
	if(commandData_)
		useFlag |= USE_DATA;
	if(!position_.eq(Vect3f::ZERO))
		useFlag |= USE_POSITION;
	if(unitIndex_)
		useFlag |= USE_UNIT;
	if(attributeReference_)
		useFlag |= USE_ATTRIBUTE;
	if(shiftModifier_)
		useFlag |= USE_SHIFT;

	char commandID = commandID_;
	out.write(commandID);

	out.write(useFlag);
	if(useFlag & USE_DATA)
		out.write(commandData_);
	if(useFlag & USE_POSITION)
		out.write(position_);
	if(useFlag & USE_UNIT)
		out.write(unitIndex_);
	if(useFlag & USE_ATTRIBUTE)
		out.write(attributeReference_);
	if(useFlag & USE_SHIFT)
		out.write(shiftModifier_);
}

void UnitCommand::read(XBuffer& in) 
{
	char commandID;
	in.read(commandID);
	commandID_ = (CommandID)commandID;
	
	char useFlag;
	in.read(useFlag);
	if(useFlag & USE_DATA)
		in.read(commandData_);
	if(useFlag & USE_POSITION)
		in.read(position_);
	if(useFlag & USE_UNIT)
		in.read(unitIndex_);
	if(useFlag & USE_ATTRIBUTE)
		in.read(attributeReference_);
	if(useFlag & USE_SHIFT)
		in.read(shiftModifier_);
}

void UnitCommand::serialize(Archive& ar)
{
	AutoAttackMode unitAttackMode;
	WalkAttackMode unitWalkMode;
	WeaponMode unitWeaponMode;
	AutoTargetFilter unitTargetFilter;

	ar.serialize(commandID_, "commandID", "Команда");
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
		case COMMAND_ID_PRODUCTION_INC:
		case COMMAND_ID_PRODUCTION_DEC:
		case COMMAND_ID_PRODUCTION_PAUSE_ON:
		case COMMAND_ID_PRODUCTION_PAUSE_OFF: {
			AttributeUnitReference reference = attributeReference_;
			ar.serialize(reference, "attribute", "Юнит");
			attributeReference_ = reference;
			break; }
		//case COMMAND_ID_OBJECT:
		//	ar.serialize(unitIndex_, "unitID", 0);
		case COMMAND_ID_ATTACK:
			ar.serialize(commandData_, "weaponID", "Номер оружия");
		case COMMAND_ID_POINT:
			ar.serialize(position_, "position", "Координата точки");
			break;
	}
}

bool UnitCommand::isUnitValid() const 
{
	if(!unitIndex_)
		return true;
	return unit();
}

