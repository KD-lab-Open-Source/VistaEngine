#ifndef __UNIT_ITEM_INVENTORY_H__
#define __UNIT_ITEM_INVENTORY_H__

#include "UnitObjective.h"
#include "UserInterface\UI_Types.h"

class AttributeItemInventory : public AttributeBase
{
public:

	/// типы снаряжения
	enum EquipmentType {
		/// не снаряжение
		EQUIPMENT_NONE,
		/// снаряжение
		EQUIPMENT_GENERAL,
		/// оружие
		EQUIPMENT_WEAPON,
		/// боеприпасы
		EQUIPMENT_AMMO,
		
		EQUIPMENT_HEALTH
	};

	float stayTime;
	float arealRadius;
	float sightRadius;

	int appearanceDelay;
	bool useLifeTime;
	int lifeTime;

	/// импульс прыжка если не влезает в инвентарь
	float jumpImpulse;
	float jumpAngularSpeed;

	/// занимаемое в инвентаре пространство, в ячейках
	Vect2i inventorySize;
	/// тип ячейки инвентаря
	InventoryCellType inventoryCellType;
	/// вид предмета в инвентаре
	UI_Sprite inventorySprite;
	/// вид предмета в инвентаре быстрого доступа
	UI_Sprite quickAccessSprite;
	/// является ли снаряжением
	EquipmentType equipmentType;
	/// слот снаряжения
	EquipmentSlotType equipmentSlotType;
	/// слот быстрого доступа
	QuickAccessSlotType quickAccessSlotType;

	/// тип оружия (для снаряжения-оружия)
	WeaponPrmReference weaponReference;
	/// тип боеприпасов (для снаряжения-боеприпасов)
	WeaponAmmoTypeReference ammoTypeReference;
    
	AttributeItemInventory();
	void serialize(Archive& ar);
	bool isEquipment() const { return (equipmentType != EQUIPMENT_NONE); }
	int getInventoryCellType(UI_InventoryType inventory_type = UI_INVENTORY) const;

	/// вывод описания на земле
	LocString tipText;
};


class UnitItemInventory : public UnitObjective
{
public:
	const AttributeItemInventory& attr() const { return safe_cast_ref<const AttributeItemInventory&>(UnitReal::attr()); }

	UnitItemInventory(const UnitTemplate& data);

	void Quant();
	void graphQuant(float dt);

	void executeCommand(const UnitCommand& command) {}
	float parametersSum() const { return parametersSum_; }

	/// Подпрыгивание когда не влезает в инвентарь.
	void jump();

	void setPose(const Se3f& poseIn, bool initPose);

private:
		
	float parametersSum_;
	LogicTimer appearanceTimer_;
	LogicTimer killTimer_;
};

#endif //__UNIT_ITEM_INVENTORY_H__
