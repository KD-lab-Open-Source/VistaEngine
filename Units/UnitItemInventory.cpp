#include "StdAfx.h"
#include "UnitItemInventory.h"
#include "Player.h"
#include "RangedWrapper.h"

UNIT_LINK_GET(UnitItemInventory)

REGISTER_CLASS(AttributeBase, AttributeItemInventory, "Предмет-инвентарь")
REGISTER_CLASS(UnitBase, UnitItemInventory, "Предмет-инвентарь");
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ITEM_INVENTORY, UnitItemInventory)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeItemInventory, EquipmentType, "Типы снаряжения")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_NONE, "Не снаряжение")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_WEAPON, "Оружие")
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeItemInventory, EquipmentType)

AttributeItemInventory::AttributeItemInventory()
: AttributeBase()
{
	unitClass_ = UNIT_CLASS_ITEM_INVENTORY;
	unitAttackClass = ATTACK_CLASS_LIGHT;
	collisionGroup = COLLISION_GROUP_REAL;
	rigidBodyPrm = RigidBodyPrmReference("Building");

	stayTime = 5;
	arealRadius = 100;
	sightRadius = 300;

	appearanceDelay = 0;
	useLifeTime = false;
	lifeTime = 60;

	inventorySize = Vect2i(1, 1);
	inventoryCellType = 0;

	equipmentType = EQUIPMENT_NONE;
	equipmentSlot = 0;
}

void AttributeItemInventory::serialize(Archive& ar)
{
    __super::serialize(ar);

	collisionGroup = COLLISION_GROUP_REAL;
	unitClass_ = UNIT_CLASS_ITEM_INVENTORY;
	excludeCollision = unitClass() == UNIT_CLASS_ITEM_INVENTORY ? EXCLUDE_COLLISION_BULLET : 0;

	ar.serialize(parametersInitial, "parametersInitial", "Добавляемые параметры (расходуемые)");
	ar.serialize(parametersArithmetics, "parametersArithmetics", "Арифметика при надевании");

	ar.serialize(inventorySize, "inventorySize", "Занимаемое пространство");
	ar.serialize(inventoryCellType, "inventoryCellType", "Тип ячейки");
	ar.serialize(inventorySprite, "inventorySprite", "Картинка");
	ar.serialize(equipmentType, "equipmentType", "Тип снаряжения");

	if(isEquipment()){
		ar.serialize(equipmentSlot, "equipmentSlot", "Слот снаряжения");

		switch(equipmentType){
		case EQUIPMENT_WEAPON:
			ar.serialize(weaponReference, "weaponReference", "Оружие");
			break;
		}
	}

	ar.serialize(appearanceDelay, "appearanceDelay", "Задержка появления");
	ar.serialize(useLifeTime, "useLifeTime", "Ограничивать время жизни");
	if(useLifeTime)
		ar.serialize(lifeTime, "lifeTime", "Время жизни");

	ar.serialize(mass, "mass", "Масса");

}

UnitItemInventory::UnitItemInventory(const UnitTemplate& data) 
: UnitObjective(data)
{ 
	if(attr().appearanceDelay){
		appearanceTimer_.start(attr().appearanceDelay*1000);
		setCollisionGroup(0);
		streamLogicCommand.set(fCommandSetIgnored, model()) << true;
		stopPermanentEffects();
	}
	if(attr().useLifeTime)
		killTimer_.start((attr().appearanceDelay + attr().lifeTime)*1000);
	
	posibleStates_ = UnitItemPosibleStates::instance();
}

void UnitItemInventory::Quant()
{
	start_timer_auto();

	__super::Quant();

	if(abnormalStateType())
		parameters_.set(0.001f);

	if(appearanceTimer_()){
		appearanceTimer_.stop();
		setCollisionGroup(attr().collisionGroup);
		streamLogicCommand.set(fCommandSetIgnored, model()) << false;
		startPermanentEffects();
	}

	if(killTimer_())
		Kill();
	
}

void UnitItemInventory::collision(UnitBase* p, const ContactInfo& contactInfo)
{
	__super::collision(p, contactInfo);

}
