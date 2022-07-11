#include "StdAfx.h"

#include "Serialization.h"
#include "UnitAttribute.h"
#include "UnitItemInventory.h"
#include "UnitActing.h"

#include "Inventory.h"
#include "..\UserInterface\UI_Inventory.h"
#include "StreamCommand.h"

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
	position_ = Vect2i(0,0);
	inventoryIndex_ = 0;
}

const AttributeItemInventory* InventoryItem::attribute() const
{
	return safe_cast<const AttributeItemInventory*>(attribute_.get());
}

void InventoryItem::clear()
{ 
	attribute_ = AttributeItemInventoryReference(); 
}

InventoryItem::InventoryItem(const UnitItemInventory* p) : attribute_(&p->attr())
{
	parameters_ = p->parameters();
	inventoryIndex_ = 0;
}

InventoryItem::~InventoryItem()
{
}

const Vect2i& InventoryItem::size() const
{
	return attribute()->inventorySize;
}

const UI_Sprite& InventoryItem::sprite() const
{
	return attribute()->inventorySprite;
}

void InventoryItem::serialize(Archive& ar)
{
	ar.serialize(position_, "position", 0);
	ar.serialize(inventoryIndex_, "inventoryIndex", 0);
	ar.serialize(parameters_, "parameters", 0);
	ar.serialize(attribute_, "attribute",  0);
}

// ----------------------------- Inventory

Inventory::Inventory(UI_ControlInventory* control) : control_(control),
	owner_(0)
{
}

Inventory::~Inventory()
{
}

bool Inventory::checkCellType(const UnitItemInventory* item) const
{
	int cell_type = isEquipment() ? item->attr().equipmentSlot : item->attr().inventoryCellType;
	return (cell_type == cellType());
}

bool Inventory::checkCellType(const InventoryItem& item) const
{
	int cell_type = isEquipment() ? item.attribute()->equipmentSlot : item.attribute()->inventoryCellType;
	return (cell_type == cellType());
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
	if(isEquipment()){
		for(InventoryItems::iterator it = items_.begin(); it != items_.end(); ++it){
			if(position == it->position()){
				onRemoveItem(*it);
				items_.erase(it);
				return true;
			}
		}
	}
	else {
		for(InventoryItems::iterator it = items_.begin(); it != items_.end(); ++it){
			if(position.x >= it->position().x && position.x < it->position().x + it->size().x && 
				position.y >= it->position().y && position.y < it->position().y + it->size().y){
					onRemoveItem(*it);
					items_.erase(it);
					return true;
				}
		}
	}

	return false;
}

const InventoryItem* Inventory::getItem(const Vect2i& cell_index) const
{
	if(isEquipment()){
		for(InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
			if(cell_index == it->position())
				return &*it;
		}
	}
	else {
		for(InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
			if(cell_index.x >= it->position().x && cell_index.x < it->position().x + it->size().x && 
				cell_index.y >= it->position().y && cell_index.y < it->position().y + it->size().y)
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
			uiStreamCommand << UI_InventoryItem(*it);
	}
}

void Inventory::serialize(Archive& ar)
{
	ar.serialize(items_, "items", 0);

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
	return isEquipment() ? Vect2i(1,1) : item->attr().inventorySize;
}

Vect2i Inventory::itemSize(const InventoryItem& item) const
{
	return isEquipment() ? Vect2i(1,1) : item.attribute()->inventorySize;
}

bool Inventory::onPutItem(const InventoryItem& item)
{
	if(owner_){
		owner_->onPutItem(item, *this);
		return true;
	}
	return false;
}

bool Inventory::onRemoveItem(const InventoryItem& item)
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

bool InventorySet::add(UI_ControlInventory* control)
{
	if(control){
		inventories_.push_back(Inventory(control)); 
		inventories_.back().setOwner(this);
		return true;
	}

	return false;
}

const InventoryItem* InventorySet::getItem(int position) const
{
	int inventory_idx;
	Vect2i cell_idx;

	if(parsePosition(position, inventory_idx, cell_idx))
		return inventories_[inventory_idx].getItem(cell_idx);

	return 0;
}

bool InventorySet::isCellEmpty(int position) const
{
	int inventory_idx;
	Vect2i cell_idx;

	if(parsePosition(position, inventory_idx, cell_idx))
		return inventories_[inventory_idx].isCellEmpty(cell_idx);

	return true;
}

bool InventorySet::canPutItem(const UnitItemInventory* item, int position) const
{
	if(position == -1){
		for(Inventories::const_iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if(it->canPutItem(item))
				return true;
		}
	}
	else {
		int inventory_idx;
		Vect2i cell_idx;

		if(parsePosition(position, inventory_idx, cell_idx))
			return inventories_[inventory_idx].canPutItem(item, cell_idx);
	}

	return false;
}

bool InventorySet::putItem(const UnitItemInventory* item, int position)
{
	if(position == -1){
		for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if(it->putItem(item))
				return true;
		}
	}
	else {
		int inventory_idx;
		Vect2i cell_idx;

		if(parsePosition(position, inventory_idx, cell_idx))
			return inventories_[inventory_idx].putItem(item, cell_idx);
	}

	return false;
}

bool InventorySet::putItem(const InventoryItem& item, int position)
{
	if(position == -1){
		for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it){
			if(it->putItem(item))
				return true;
		}
	}
	else {
		int inventory_idx;
		Vect2i cell_idx;

		if(parsePosition(position, inventory_idx, cell_idx))
			return inventories_[inventory_idx].putItem(item, cell_idx);
	}

	return false;
}

bool InventorySet::removeItem(int position)
{
	int inventory_idx;
	Vect2i cell_idx;

	if(parsePosition(position, inventory_idx, cell_idx))
		return inventories_[inventory_idx].removeItem(cell_idx);

	return false;
}

bool InventorySet::onPutItem(const InventoryItem& item, const Inventory& inventory)
{
	if(!owner_)
		return false;

	if(inventory.isEquipment()){
		owner_->putOnItem(item.attribute());
		switch(item.attribute()->equipmentType){
		case AttributeItemInventory::EQUIPMENT_WEAPON:
			if(UI_ControlInventory* control = inventory.control())
				return owner_->replaceWeapon(control->cellType(), item.attribute()->weaponReference);
			break;
		}
	}
	return true;
}

bool InventorySet::onRemoveItem(const InventoryItem& item, const Inventory& inventory)
{
	if(!owner_)
		return false;

	if(inventory.isEquipment()){
		owner_->putOffItem(item.attribute());
		switch(item.attribute()->equipmentType){
		case AttributeItemInventory::EQUIPMENT_WEAPON:
			if(UI_ControlInventory* control = inventory.control())
				return owner_->removeWeapon(control->cellType(), item.attribute()->weaponReference);
			break;
		}
	}
	return true;
}

bool InventorySet::takeItem(int position)
{
	if(takenItem_())
		return false;

	int inventory_idx;
	Vect2i cell_idx;

	if(parsePosition(position, inventory_idx, cell_idx)){
		if(const InventoryItem* item = inventories_[inventory_idx].getItem(cell_idx)){
			takenItem_ = *item;
			takenItemPosition_ = position;
			return inventories_[inventory_idx].removeItem(cell_idx);
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
				inventories_[index].putItem(*item, item->position());
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

bool InventorySet::parsePosition(int position, int& inventory_index, Vect2i& cell_index) const
{
	int idx = 0;
	for(int i = 0; i < inventories_.size(); i++){
		Vect2i sz = inventories_[i].size();
		int n = sz.x * sz.y + 1;
		if(idx + n > position){
			position -= idx;
			cell_index = (sz.x) ? Vect2i(position % sz.x, position / sz.x) : Vect2i(0,0);
			inventory_index = i;
			return true;
		}
		else
			idx += n;
	}

	return false;
}

void InventorySet::clearItems()
{
	for(Inventories::iterator it = inventories_.begin(); it != inventories_.end(); ++it)
		it->clearItems();
}
