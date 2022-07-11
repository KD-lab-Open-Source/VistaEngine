#include "StdAfx.h"

#include "Serialization\Serialization.h"
#include "UnitAttribute.h"
#include "UnitItemInventory.h"
#include "Universe.h"
#include "IronLegion.h"
#include "Player.h"
#include "Serialization\StringTableImpl.h"

#include "Inventory.h"
#include "UserInterface\UI_Inventory.h"
#include "StreamCommand.h"
#include "Serialization\SerializationFactory.h"
#include "UnicodeConverter.h"
#include "WBuffer.h"

WRAP_LIBRARY(InventoryCellTypeTable, "InventoryCellType", "Типы ячеек инвентаря", "Scripts\\Content\\InventoryCellType", 0, LIBRARY_EDITABLE);
WRAP_LIBRARY(EquipmentSlotTypeTable, "EquipmentSlotType", "Типы слотов снаряжения", "Scripts\\Content\\EquipmentSlotType", 0, LIBRARY_EDITABLE);
WRAP_LIBRARY(QuickAccessSlotTypeTable, "QuickAccessSlotType", "Типы слотов быстрого доступа", "Scripts\\Content\\QuickAccessSlotType", 0, true);

UI_QuickAccessMode InventorySet::quickAccessMode_ = UI_INVENTORY_QUICK_ACCESS_ON;

// ----------------------------- InventoryPosition

void InventoryPosition::serialize(Archive& ar)
{
	ar.serialize(cellIndex_, "cellIndex", 0);
	ar.serialize(inventoryIndex_, "inventoryIndex", 0);
}

// ----------------------------- InventoryItem

void fCommandUpdateInventory(void* data)
{
	UI_ControlInventory** control = (UI_ControlInventory**)data;

	int* item_count = (int*)(control + 1);
	UI_InventoryItem* items = (UI_InventoryItem*)(item_count + 1);

	(*control)->updateItems(items, *item_count);
}

InventoryItem::InventoryItem()
{
	isClone_ = false;
	isActivated_ = false;
}

const AttributeItemInventory* InventoryItem::attribute() const
{
	return safe_cast<const AttributeItemInventory*>(attribute_.get());
}

void InventoryItem::clone(InventoryItem& item) const
{
	item = *this;

	item.isClone_ = true;
	item.parentPosition_ = position_;
}

bool InventoryItem::isClone(const InventoryItem& clone) const
{
	return (clone.isClone_ && clone.parentPosition_ == position_);
}

void InventoryItem::clear()
{ 
	attribute_ = AttributeItemInventoryReference(); 
}

InventoryItem::InventoryItem(const UnitItemInventory* p) : attribute_(&p->attr())
{
	parameters_ = p->parameters();
	arithmetics_ = p->attr().parametersArithmetics;
	isClone_ = false;
	isActivated_ = false;
}

InventoryItem::InventoryItem(const AttributeItemInventory& attr) : attribute_(&attr)
{
	parameters_ = attr.parametersInitial;
	arithmetics_ = attr.parametersArithmetics;
	isClone_ = false;
	isActivated_ = false;
}

InventoryItem::~InventoryItem()
{
}

const Vect2i& InventoryItem::size() const
{
	return attribute()->inventorySize;
}

const UI_Sprite& InventoryItem::sprite(UI_InventoryType inventory_type) const
{
	switch(inventory_type){
	case UI_INVENTORY_QUICK_ACCESS:
		if(!attribute()->quickAccessSprite.isEmpty())
			return attribute()->quickAccessSprite;
	}

	return attribute()->inventorySprite;
}

void InventoryItem::serialize(Archive& ar)
{
	if(!ar.serialize(position_, "inventoryPosition", 0)){ // conversion 12.10.2007
		Vect2i pos;
		int idx;
		ar.serialize(pos, "position", 0);
		ar.serialize(idx, "inventoryIndex", 0);
		position_ = InventoryPosition(pos, idx);
	}

	ar.serialize(isClone_, "isClone", 0);

	ar.serialize(parentPosition_, "inventoryParentPosition", 0);

	ar.serialize(arithmetics_, "arithmetics", 0);
	ar.serialize(parameters_, "parameters", 0);
	ar.serialize(attribute_, "attribute",  0);
}

void InventoryItem::showDebugInfo() const
{
	XBuffer buf(1024, 1);
	buf < "Предмет: " < (attribute() ? attribute()->libraryKey() : "NONE");
	buf < "\nПараметры:\n" < parameters().debugStr();
	buf < "\nАрифметика:\n";
	
	WBuffer out;
	arithmetics().getUIData(out);
	buf < w2a(out.c_str()).c_str();

	buf < "\nisClone: " < (isClone() ? "Yes" : "No") < ", isActivated: " < (isActivated() ? "Yes" : "No");
	show_text2d(Vect2f(0.f, 100.f), buf.c_str(), Color4c::WHITE);
}

// ----------------------------- Inventory

Inventory::Inventory(UI_ControlInventory* control) : control_(control),
	owner_(0)
{
	index_ = 0;
	weaponSlotIndex_ = -1;
}

Inventory::~Inventory()
{
}

bool Inventory::checkCellType(const UnitItemInventory* item) const
{
	return cellType() == item->attr().getInventoryCellType(inventoryType());
}

bool Inventory::checkCellType(const InventoryItem& item) const
{
	return cellType() == item.attribute()->getInventoryCellType(inventoryType());
}

bool Inventory::canPutItem(const UnitItemInventory* item) const
{
	if(!checkCellType(item))
		return false;

	if(control_){
		Vect2i item_size = itemSize(item);
		Vect2i sz = control_->size();
		for(int y = 0; y < sz.y; y++){
			for(int x = 0; x < sz.x; x++){
				if(checkItemPosition(Vect2i(x,y), item_size))
					return true;
			}
		}
	}
	else 
		return true;

	return false;
}

bool Inventory::canPutItem(const UnitItemInventory* item, const Vect2i& position) const
{
	if(!checkCellType(item))
		return false;

	if(control_){
		return checkItemPosition(position, itemSize(item));
	}

	return true;
}

bool Inventory::canPutItem(const InventoryItem& item) const
{
	if(!checkCellType(item))
		return false;

	if(control_){
		Vect2i item_size = itemSize(item);
		Vect2i sz = control_->size();
		for(int y = 0; y < sz.y; y++){
			for(int x = 0; x < sz.x; x++){
				if(checkItemPosition(Vect2i(x,y), item_size))
					return true;
			}
		}
	}
	else 
		return true;

	return false;
}

bool Inventory::canPutItem(const InventoryItem& item, const Vect2i& position) const
{
	if(!checkCellType(item))
		return false;

	if(control_){
		return checkItemPosition(position, itemSize(item));
	}

	return true;
}

bool Inventory::putItem(const UnitItemInventory* item)
{
	if(!checkCellType(item))
		return false;

	InventoryItem itm(item);
	return putItem(itm);
}

bool Inventory::putItem(const UnitItemInventory* item, const Vect2i& position)
{
	if(!checkCellType(item))
		return false;

	InventoryItem itm(item);
	return putItem(itm, position);
}

bool Inventory::putItem(const InventoryItem& item)
{
	if(!checkCellType(item))
		return false;

	if(control_){
		Vect2i sz = control_->size();
		for(int y = 0; y < sz.y; y++){
			for(int x = 0; x < sz.x; x++){
				if(checkItemPosition(Vect2i(x,y), item)){
					items_.push_back(item);
					items_.back().setPosition(Vect2i(x,y));
					onPutItem(items_.back());
					return true;
				}
			}
		}
	}
	else {
		items_.push_back(item);
		onPutItem(items_.back());
		return true;
	}

	return false;
}

bool Inventory::putItem(const InventoryItem& item, const Vect2i& position)
{
	if(!checkCellType(item))
		return false;

	Vect2i pos = adjustItemPosition(position, itemSize(item));

	if(control_){
		if(checkItemPosition(pos, item)){
			items_.push_back(item);
			items_.back().setPosition(pos);
			onPutItem(items_.back());
			return true;
		}
	}
	else {
		items_.push_back(item);
		items_.back().setPosition(pos);
		onPutItem(items_.back());
		return true;
	}

	return false;
}

bool Inventory::removeItem(const InventoryItem* item)
{
	xassert(item);

	InventoryItems::iterator it = std::find(items_.begin(), items_.end(), *item);
	if(it != items_.end()){
		onRemoveItem(*it);
		items_.erase(it);
		return true;
	}

	return false;
}

bool Inventory::removeItem(const Vect2i& position)
{
	if(inventoryType() != UI_INVENTORY){
		for(InventoryItems::iterator it = items_.begin(); it != items_.end(); ++it){
			if(position == it->cellIndex()){
				onRemoveItem(*it);
				items_.erase(it);
				return true;
			}
		}
	}
	else {
		for(InventoryItems::iterator it = items_.begin(); it != items_.end(); ++it){
			if(position.x >= it->cellIndex().x && position.x < it->cellIndex().x + it->size().x && 
				position.y >= it->cellIndex().y && position.y < it->cellIndex().y + it->size().y){
					onRemoveItem(*it);
					items_.erase(it);
					return true;
				}
		}
	}

	return false;
}

bool Inventory::removeClonedItems(const InventoryItem* item)
{
	if(isEquipment()){
		bool ret = false;
		for(int i = 0; i < items_.size(); i++){
			if(item->isClone(items_[i])){
				removeItem(&items_[i]);
				i--;
				ret = true;
			}
		}
		return ret;
	}

	return false;
}

const InventoryItem* Inventory::getItem(const Vect2i& cell_index) const
{
	if(inventoryType() != UI_INVENTORY){
		for(InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
			if(cell_index == it->cellIndex())
				return &*it;
		}
	}
	else {
		for(InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
			if(cell_index.x >= it->cellIndex().x && cell_index.x < it->cellIndex().x + it->size().x && 
				cell_index.y >= it->cellIndex().y && cell_index.y < it->cellIndex().y + it->size().y)
					return &*it;
		}
	}

	return 0;
}

InventoryItem* Inventory::getItem(const Vect2i& cell_index)
{
	if(inventoryType() != UI_INVENTORY){
		for(InventoryItems::iterator it = items_.begin(); it != items_.end(); ++it){
			if(cell_index == it->cellIndex())
				return &*it;
		}
	}
	else {
		for(InventoryItems::iterator it = items_.begin(); it != items_.end(); ++it){
			if(cell_index.x >= it->cellIndex().x && cell_index.x < it->cellIndex().x + it->size().x && 
				cell_index.y >= it->cellIndex().y && cell_index.y < it->cellIndex().y + it->size().y)
					return &*it;
		}
	}

	return 0;
}

bool Inventory::isEquipment() const
{
	if(control_)
		return control_->isEquipment();

	return false;
}

bool Inventory::isQuickAccess() const
{
	if(control_)
		return control_->isQuickAccess();

	return false;
}

UI_InventoryType Inventory::inventoryType() const
{
	if(control_)
		return control_->inventoryType();

	return UI_INVENTORY;
}

int Inventory::cellType() const
{
	if(control_)
		return control_->cellType();

	return 0;
}

Vect2i Inventory::size() const
{
	if(control_)
		return control_->size();

	return Vect2i(0,0);
}

void Inventory::updateControl() const
{
	if(control_){
		uiStreamCommand.set(fCommandUpdateInventory);
		uiStreamCommand << (void*)control_;
		uiStreamCommand << items_.size();
	
		for(InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it)
			uiStreamCommand << UI_InventoryItem(*it, control_->inventoryType());
	}
}

void Inventory::serialize(Archive& ar)
{
	ar.serialize(items_, "items", 0);
	ar.serialize(index_, "index", 0);

	UI_InventoryReference ref(control_);
	ar.serialize(ref, "controlReference", 0);

	if(ar.isInput())
		control_ = ref.control();
}

bool Inventory::isCellEmpty(const Vect2i& cell_index) const
{
	return (getItem(cell_index) == 0);
}

bool Inventory::checkItemPosition(const Vect2i& pos, const InventoryItem& item) const
{
	return checkItemPosition(pos, itemSize(item));
}

bool Inventory::checkItemPosition(const Vect2i& pos, const Vect2i& size) const
{
	if(!control_)
		return true;

	if(control_->checkPosition(pos, size)){
		for(int y = 0; y < size.y; y++){
			for(int x = 0; x < size.x; x++){
				if(!isCellEmpty(pos + Vect2i(x,y)))
					return false;
			}
		}
	}
	else
		return false;

	return true;
}

Vect2i Inventory::adjustItemPosition(const Vect2i& pos, const Vect2i& size) const
{
	if(control_)
		return control_->adjustPosition(pos, size);

	return pos;
}

Vect2i Inventory::itemSize(const UnitItemInventory* item) const
{
	return (inventoryType() != UI_INVENTORY) ? Vect2i(1,1) : item->attr().inventorySize;
}

Vect2i Inventory::itemSize(const InventoryItem& item) const
{
	return (inventoryType() != UI_INVENTORY) ? Vect2i(1,1) : item.attribute()->inventorySize;
}

bool Inventory::onPutItem(InventoryItem& item)
{
	item.setInventoryIndex(index_);

	if(owner_){
		owner_->onPutItem(item, *this);
		return true;
	}
	return false;
}

bool Inventory::onRemoveItem(InventoryItem& item)
{
	if(owner_){
		owner_->onRemoveItem(item, *this);
		return true;
	}
	return false;
}

// ----------------------------- InventorySet

InventorySet::InventorySet() : owner_(0)
{
	takenItemPosition_ = 0;
}

void InventorySet::setOwner(UnitActing* unit)
{
	owner_ = unit;

	for(UnitActing::WeaponSlots::const_iterator it = unit->weaponSlots().begin(); it != unit->weaponSlots().end(); ++it){
		for(Inventories::iterator inv = inventories_.begin(); inv != inventories_.end(); ++inv){
			if(inv->isEquipment() && inv->cellType() == it->attribute()->equipmentSlotType()){
				inv->setWeaponSlotIndex(it - unit->weaponSlots().begin());
				break;
			}
		}
	}
}

bool InventorySet::add(UI_ControlInventory* control)
{
	if(control){
		inventories_.push_back(Inventory(control)); 
		inventories_.back().setOwner(this);
		inventories_.back().setIndex(inventories_.size() - 1);
		return true;
	}

	return false;
}

const InventoryItem* InventorySet::getItem(int position) const
{
	InventoryPosition pos;

	if(parsePosition(position, pos))
		return inventories_[pos.inventoryIndex()].getItem(pos.cellIndex());

	return 0;
}

InventoryItem* InventorySet::getItem(const InventoryPosition& pos)
{
	return inventories_[pos.inventoryIndex()].getItem(pos.cellIndex());
}

bool InventorySet::findWeaponAmmo(const WeaponAmmoType* ammo_type, InventoryPosition& pos)
{
	bool ret = false;
	float charge = 0.f;
	for(Inventories::const_iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(!it->isEquipment()){
			for(InventoryItems::const_iterator item = it->items().begin(); item != it->items().end(); ++item){
				if(item->attribute()->equipmentType == AttributeItemInventory::EQUIPMENT_AMMO && item->attribute()->ammoTypeReference == ammo_type){
					float item_charge = item->parameters().findByType(ParameterType::AMMO, 0.f);
					if(!ret || item_charge < charge){
						pos = item->position();
						ret = true;
					}
				}
			}
		}
	}

	return ret;
}

bool InventorySet::isCellEmpty(int position) const
{
	InventoryPosition pos;

	if(parsePosition(position, pos))
		return inventories_[pos.inventoryIndex()].isCellEmpty(pos.cellIndex());

	return true;
}

bool InventorySet::canPutItem(const UnitItemInventory* item, int position) const
{
	bool prm_request = false;
	if(item->attr().isEquipment())
		prm_request = owner_->requestResource(item->attr().accessValue, NEED_RESOURCE_SILENT_CHECK);

	if(position == -1){
		for(Inventories::const_iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if((prm_request || !it->isEquipment()) && it->canPutItem(item))
				return true;
		}
	}
	else {
		InventoryPosition pos;

		if(parsePosition(position, pos)){
			if(prm_request || !inventories_[pos.inventoryIndex()].isEquipment())
				return inventories_[pos.inventoryIndex()].canPutItem(item, pos.cellIndex());
		}
	}

	return false;
}

bool InventorySet::canPutItem(const InventoryItem& item, int position) const
{
	bool prm_request = false;
	if(item.attribute()->isEquipment())
		prm_request = owner_->requestResource(item.attribute()->accessValue, NEED_RESOURCE_SILENT_CHECK);

	if(position == -1){
		for(Inventories::const_iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if((prm_request || !it->isEquipment()) && it->canPutItem(item))
				return true;
		}
	}
	else {
		InventoryPosition pos;

		if(parsePosition(position, pos)){
			if(prm_request || !inventories_[pos.inventoryIndex()].isEquipment())
				return inventories_[pos.inventoryIndex()].canPutItem(item, pos.cellIndex());
		}
	}

	return false;
}

bool InventorySet::putItem(const UnitItemInventory* item, int position)
{
	bool prm_request = false;
	if(item->attr().isEquipment())
		prm_request = owner_->requestResource(item->attr().accessValue, NEED_RESOURCE_SILENT_CHECK);

	if(position == -1){
		for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if(it->isQuickAccess() && it->putItem(item))
				return true;
		}

		for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if((prm_request || !it->isEquipment()) && it->putItem(item))
				return true;
		}
	}
	else {
		InventoryPosition pos;

		if(parsePosition(position, pos)){
			if(prm_request || !inventories_[pos.inventoryIndex()].isEquipment())
				return inventories_[pos.inventoryIndex()].putItem(item, pos.cellIndex());
		}
	}

	return false;
}

bool InventorySet::putItem(const InventoryItem& item, int position)
{
	bool prm_request = false;
	if(item.attribute()->isEquipment())
		prm_request = owner_->requestResource(item.attribute()->accessValue, NEED_RESOURCE_SILENT_CHECK);

	if(position == -1){
		for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if(it->isQuickAccess() && it->putItem(item))
				return true;
		}

		for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if((prm_request || !it->isEquipment()) && it->putItem(item))
				return true;
		}
	}
	else {
		InventoryPosition pos;

		if(parsePosition(position, pos)){
			if(prm_request || !inventories_[pos.inventoryIndex()].isEquipment())
				return inventories_[pos.inventoryIndex()].putItem(item, pos.cellIndex());
		}
	}

	return false;
}

bool InventorySet::putItem(const InventoryItem& item, UI_InventoryType inventory_type)
{
	if(inventory_type == UI_INVENTORY_EQUIPMENT){
		if(!owner_->requestResource(item.attribute()->accessValue, NEED_RESOURCE_SILENT_CHECK))
			return false;
	}

	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(it->inventoryType() == inventory_type && it->putItem(item))
			return true;
	}

	return false;
}

bool InventorySet::removeItem(int position)
{
	InventoryPosition pos;

	if(parsePosition(position, pos))
		return removeItem(pos);

	return false;
}

bool InventorySet::removeItem(const InventoryPosition& pos)
{
	return inventories_[pos.inventoryIndex()].removeItem(pos.cellIndex());
}

bool InventorySet::onPutItem(InventoryItem& item, const Inventory& inventory)
{
	if(!owner_)
		return false;

	if(inventory.isEquipment()){
		owner_->putOnItem(item);
		owner_->applyParameterArithmetics(item.arithmetics());
		switch(item.attribute()->equipmentType){
		case AttributeItemInventory::EQUIPMENT_WEAPON:
			return owner_->replaceWeaponInSlot(inventory.weaponSlotIndex(), item);
		}
	}
	return true;
}

bool InventorySet::onRemoveItem(InventoryItem& item, const Inventory& inventory)
{
	if(!owner_)
		return false;

	if(inventory.isEquipment()){
		owner_->putOffItem(item);
		owner_->applyParameterArithmetics(ParameterArithmetics(item.arithmetics(), true));

		if(item.isClone()){
			if(InventoryItem* parent = getItem(item.parentPosition())){
				bool has_clones = false;
				for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
					if(it->isEquipment()){
						for(InventoryItems::const_iterator itm = it->items().begin(); itm != it->items().end(); ++itm){
							if(itm->isClone() && &*itm != &item && itm->parentPosition() == parent->position()){
								has_clones = true;
								break;
							}
						}
					}
					if(has_clones)
						break;
				}

				if(!has_clones)
					parent->setActivated(false);
			}
		}

		switch(item.attribute()->equipmentType){
		case AttributeItemInventory::EQUIPMENT_WEAPON:
			return owner_->removeWeaponFromSlot(inventory.weaponSlotIndex());
		}
	}
	else if(inventory.isQuickAccess()){
		if(item.isActivated()){
			for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
				if(it->isEquipment()){
					for(InventoryItems::const_iterator itm = it->items().begin(); itm != it->items().end(); ++itm){
						if(itm->isClone() && itm->parentPosition() == item.position()){
							removeItem(itm->position());
							if(!item.isActivated())
								return true;
						}
					}
				}
			}
		}
	}

	return true;
}

bool InventorySet::reloadWeapon(int slot_index)
{
	if(slot_index < 0 || slot_index >= owner_->weaponSlots().size())
		return false;
	
	WeaponBase* weapon = owner_->weaponSlots()[slot_index].weapon();
	const WeaponAmmoType* ammo_type = weapon->weaponPrm()->ammoType();
	if(!ammo_type) return false;

	float ammo_max = weapon->parameters().findByType(ParameterType::AMMO_CAPACITY, 0.f);
	float ammo_cur = weapon->parameters().findByType(ParameterType::AMMO, 0.f);

	InventoryPosition pos;
	if(findWeaponAmmo(weapon->weaponPrm()->ammoType(), pos)){
		if(InventoryItem* item = getItem(pos)){
			float item_ammo = item->parameters().findByType(ParameterType::AMMO, 0.f);
			ammo_cur += item_ammo;
			if(ammo_max > FLT_EPS && ammo_cur > ammo_max){
				item_ammo = ammo_cur - ammo_max;
				ammo_cur = ammo_max;
				item->setParameter(item_ammo, ParameterType::AMMO);
			}
			else 
				removeItem(pos);

			weapon->setParameter(ammo_cur, ParameterType::AMMO);
			return true;
		}
	}

	return false;
}

bool InventorySet::removeWeapon(int slot_index)
{
	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(it->weaponSlotIndex() == slot_index){
			if(InventoryItem* item = it->getItem(Vect2i(0,0))){
				if(item->isClone())
					return removeItem(item->parentPosition());
				else
					return it->removeItem(Vect2i(0,0));
			}
		}

	}

	return false;
}

bool InventorySet::updateWeapon(int slot_index)
{
	if(slot_index < 0 || slot_index >= owner_->weaponSlots().size())
		return false;
	
	WeaponBase* weapon = owner_->weaponSlots()[slot_index].weapon();
	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(it->weaponSlotIndex() == slot_index){
			if(InventoryItem* item = it->getItem(Vect2i(0,0))){
				float ammo = weapon->parameters().findByType(ParameterType::AMMO, 0.f);
				float durability = weapon->parameters().findByType(ParameterType::WEAPON_DURABILITY, 0.f);

				item->setParameter(ammo, ParameterType::AMMO);
				item->setParameter(durability, ParameterType::WEAPON_DURABILITY);

				if(item->isClone()){
					if(InventoryItem* item1 = getItem(item->parentPosition())){
						item1->setParameter(ammo, ParameterType::AMMO);
						item1->setParameter(durability, ParameterType::WEAPON_DURABILITY);
					}
				}
				return true;
			}
		}
	}

	return false;
}

bool InventorySet::takeItem(int position)
{
	if(takenItem_())
		return false;

	InventoryPosition pos;

	if(parsePosition(position, pos)){
		int inventory_idx = pos.inventoryIndex();
		Vect2i cell_idx = pos.cellIndex();
		switch(inventories_[inventory_idx].inventoryType()){
		case UI_INVENTORY:
			if(const InventoryItem* item = inventories_[inventory_idx].getItem(cell_idx)){
				takenItem_ = *item;
				takenItemPosition_ = position;
				return inventories_[inventory_idx].removeItem(cell_idx);
			}
			break;
		case UI_INVENTORY_EQUIPMENT:
			if(const InventoryItem* item = inventories_[inventory_idx].getItem(cell_idx)){
				if(!item->isClone()){
					takenItem_ = *item;
					takenItemPosition_ = position;
				}
				return inventories_[inventory_idx].removeItem(cell_idx);
			}
			break;
		case UI_INVENTORY_QUICK_ACCESS:
			if(InventoryItem* item = inventories_[inventory_idx].getItem(cell_idx)){
				if(quickAccessMode_ == UI_INVENTORY_QUICK_ACCESS_OFF){
					removeClonedItems(item);

					takenItem_ = *item;
					takenItemPosition_ = position;

					return inventories_[inventory_idx].removeItem(cell_idx);
				}
				else if(quickAccessMode_ == UI_INVENTORY_QUICK_ACCESS_ON){
					if(item->attribute()->equipmentType == AttributeItemInventory::EQUIPMENT_HEALTH){
						owner_->applyParameterArithmetics(item->arithmetics());
						removeItem(item->position());
						return true;
					}
					else if(!removeClonedItems(item)){
						InventoryItem itm;
						item->clone(itm);
						if(putToEquipmentSlot(itm)){
							item->setActivated(true);
							return true;
						}
					}
					else {
						item->setActivated(false);
						return true;
					}
				}
			}
			break;
		}
	}

	return false;
}

bool InventorySet::returnItem(int position)
{
	if(!takenItem_())
		return false;

	if(position != -1){
		if(putItem(takenItem_, position)){
			takenItem_.clear();
			return true;
		}
	}
	else {
		if(putItem(takenItem_, takenItemPosition_) || putItem(takenItem_)){
			takenItem_.clear();
			return true;
		}
	}

	return false;
}

void InventorySet::dropItems()
{
	float drop_radius = 0.f;
	if(owner_->attr().isLegionary())
		drop_radius = safe_cast<UnitLegionary*>(owner_)->attr().itemDropRadius;

	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		for(InventoryItems::const_iterator itm = it->items().begin(); itm != it->items().end(); ++itm){
			float angle = logicRNDfrand()*M_PI*2.f;
			float r = logicRNDfrand() * drop_radius;

			Vect2f pos(r * cosf(angle), r * sinf(angle));

			UnitBase* p = universe()->worldPlayer()->buildUnit(itm->attribute());
			p->setPose(Se3f(MatXf(Mat3f::ID, To3D(pos + owner_->position2D()))), true);
			safe_cast<UnitInterface*>(p)->setParameters(itm->parameters());
		}
	}

	clearItems();
}

void InventorySet::updateUI() const
{
	for(Inventories::const_iterator it = inventories_.begin(); it != inventories_.end(); ++it)
		it->updateControl();
}

void InventorySet::serialize(Archive& ar)
{
	if(ar.isOutput()){
		InventoryItems items;
		for(int i = 0; i < inventories_.size(); i++){
			for(InventoryItems::const_iterator item = inventories_[i].items().begin(); item != inventories_[i].items().end(); ++item){
				items.push_back(*item);
				items.back().setInventoryIndex(i);
			}
		}
		ar.serialize(items, "items", 0);
	}
	else {
		InventoryItems items;
		ar.serialize(items, "items", 0);

		clearItems();
		for(InventoryItems::const_iterator item = items.begin(); item != items.end(); ++item){
			int index = item->inventoryIndex();
			if(index >= 0 && index < inventories_.size())
				inventories_[index].putItem(*item, item->cellIndex());
		}
	}

	ar.serialize(takenItem_, "takenItem", 0);
	ar.serialize(takenItemPosition_, "takenItemPosition", 0);
}

int InventorySet::getPosition(const UI_ControlInventory* control, const Vect2i& pos) const
{
	int index = 0;
	for(Inventories::const_iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		Vect2i sz = it->size();
		if(it->control() == control){
			index += pos.x + pos.y * sz.x;
			break;
		}
		else
			index += sz.x * sz.y + 1;
	}

	return index;
}

bool InventorySet::parsePosition(int position, InventoryPosition& out_position) const
{
	int idx = 0;
	for(int i = 0; i < inventories_.size(); i++){
		Vect2i sz = inventories_[i].size();
		int n = sz.x * sz.y + 1;
		if(idx + n > position){
			position -= idx;
			out_position = InventoryPosition((sz.x) ? Vect2i(position % sz.x, position / sz.x) : Vect2i(0,0), i);
			return true;
		}
		else
			idx += n;
	}

	return false;
}

bool InventorySet::removeClonedItems(const InventoryItem* item)
{
	bool ret = false;
	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(it->removeClonedItems(item))
			ret = true;
	}

	return ret;
}

bool InventorySet::putToEquipmentSlot(const InventoryItem& item)
{
	bool prm_request = false;
	if(item.attribute()->isEquipment()){
		if(!owner_->requestResource(item.attribute()->accessValue, NEED_RESOURCE_SILENT_CHECK))
			return false;
	}

	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(it->isEquipment()){
			if(it->putItem(item))
				return true;
		}
	}

	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
		if(it->isEquipment()){
			if(it->checkCellType(item)){
				for(InventoryItems::const_iterator itm = it->items().begin(); itm != it->items().end(); ++itm){
					if(itm->isClone()){
						if(it->removeItem(&*itm))
							return it->putItem(item);
					}
				}
			}
		}
	}

	return false;
}

void InventorySet::clearItems()
{
	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it)
		it->clearItems();
}

