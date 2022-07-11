#ifndef __UNIT_ITEM_INVENTORY_H__
#define __UNIT_ITEM_INVENTORY_H__

#include "UnitObjective.h"
//#include "IronBuilding.h"

class AttributeItemInventory : public AttributeBase
{
public:

	/// типы снар€жени€
	enum EquipmentType {
		/// не снар€жение
		EQUIPMENT_NONE,
		/// оружие
		EQUIPMENT_WEAPON
	};

	float stayTime;
	float arealRadius;
	float sightRadius;

	int appearanceDelay;
	bool useLifeTime;
	int lifeTime;

	/// занимаемое в инвентаре пространство, в €чейках
	Vect2i inventorySize;
	/// тип €чейки инвентар€
	int inventoryCellType;
	/// вид предмета в инвентаре
	UI_Sprite inventorySprite;
	/// €вл€етс€ ли снар€жением
	EquipmentType equipmentType;
	/// слот снар€жени€
	int equipmentSlot;

	/// тип оружи€ (дл€ снар€жени€-оружи€)
	WeaponPrmReference weaponReference;
    
	AttributeItemInventory();
	void serialize(Archive& ar);
	bool isEquipment() const { return (equipmentType != EQUIPMENT_NONE); }
};


class UnitItemInventory : public UnitObjective
{
public:
	const AttributeItemInventory& attr() const { return safe_cast_ref<const AttributeItemInventory&>(UnitReal::attr()); }

	UnitItemInventory(const UnitTemplate& data);

	void Quant();
	void executeCommand(const UnitCommand& command) {}
	void collision(UnitBase* p, const ContactInfo& contactInfo);
	float parametersSum() const { return parametersSum_; }

private:
		
	float parametersSum_;
	DelayTimer appearanceTimer_;
	DelayTimer killTimer_;
};

#endif //__UNIT_ITEM_INVENTORY_H__
