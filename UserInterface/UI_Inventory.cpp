#include "StdAfx.h"

#include "Serialization\Serialization.h"

#include "UI_Render.h"
#include "UI_Inventory.h"
#include "UI_Logic.h"
#include "Inventory.h"

BEGIN_ENUM_DESCRIPTOR(UI_InventoryType, "UI_InventoryType")
REGISTER_ENUM(UI_INVENTORY, "просто инвентарь")
REGISTER_ENUM(UI_INVENTORY_EQUIPMENT, "снар€жение")
REGISTER_ENUM(UI_INVENTORY_QUICK_ACCESS, "снар€жение с активацией")
END_ENUM_DESCRIPTOR(UI_InventoryType)

BEGIN_ENUM_DESCRIPTOR(UI_QuickAccessMode, "UI_QuickAccessMode")
REGISTER_ENUM(UI_INVENTORY_QUICK_ACCESS_OFF, "јктиваци€ по клику выключена")
REGISTER_ENUM(UI_INVENTORY_QUICK_ACCESS_ON, "јктиваци€ по клику включена")
END_ENUM_DESCRIPTOR(UI_QuickAccessMode)

// ----------------------------- UI_InventoryItem

UI_InventoryItem::UI_InventoryItem() : sprite_(0)
{
	position_ = Recti(0,0,1,1);
	isActivated_ = false;
}

UI_InventoryItem::UI_InventoryItem(const InventoryItem& item, UI_InventoryType inventory_type) : sprite_(&item.sprite(inventory_type))
{
	position_ = Recti(item.position().cellIndex(), item.size());
	isActivated_ = item.isActivated();
}

void UI_InventoryItem::redraw(const Vect2f& pos, float alpha) const
{
	if(!sprite_)
		return;

	Vect2f size = sprite_->size() / Vect2f(1024.f, 768.f);

	Rectf position(size);
	position.center(pos + Vect2f(size.x/2.f, size.y/2.f));
	
	UI_Render::instance().drawSprite(position, *sprite_, alpha);
}

void UI_InventoryItem::redraw(const Rectf& position, float alpha) const
{
	if(!sprite_)
		return;

	UI_Render::instance().drawSprite(position, *sprite_, alpha);
}

// ----------------------------- UI_ControlInventory

UI_ControlInventory::UI_ControlInventory()
{
	size_ = Vect2i(1,1);
	highlight_ = Recti(0,0,0,0);
	inventoryType_ = UI_INVENTORY;
}

UI_ControlInventory::~UI_ControlInventory()
{
}

void UI_ControlInventory::quant(float dt)
{
	if(!mouseHover())
		highlight_ = Recti(0,0,0,0);

	__super::quant(dt);
}

bool UI_ControlInventory::redraw() const
{
	if(!UI_ControlBase::redraw())
		return false;

	if(size_.x && size_.y){
		Vect2f pos(position().left(), position().top());
		Vect2f cell_sz(position().width() / float(size_.x), position().height() / float(size_.y));

		for(int i = 0; i < size_.y; i++){
			for(int j = 0; j < size_.x; j++){
				redrawCell(Rectf(pos, cell_sz), cellState(Vect2i(j, i)));
				pos.x += cell_sz.x;
			}

			pos.x = position().left();
			pos.y += cell_sz.y;
		}

		for(UI_InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it)
			redrawItem(*it);
	}

	return true;
}

const InventoryItem* UI_ControlInventory::hoverItem(const InventorySet* inv, const Vect2f& pos) const
{
	xassert(inv);

	Vect2i idx_pos;
	if(!getCellCoords(pos, idx_pos))
		return 0;

	int position = inv->getPosition(this, idx_pos);
	return inv->getItem(position);

	return 0;
}

int UI_ControlInventory::cellType() const
{
	switch(inventoryType()){
	case UI_INVENTORY:
		return cellType_.key();
	case UI_INVENTORY_EQUIPMENT:
		return equipmentSlotType_.key();
	case UI_INVENTORY_QUICK_ACCESS:
		return quickAccessSlotType_.key();
	}

	return 0;
}

bool UI_ControlInventory::checkPosition(const Vect2i& pos, const Vect2i& size) const
{
	if(pos.x >= 0 && pos.x + size.x <= size_.x && pos.y >= 0 && pos.y + size.y <= size_.y)
		return true;

	return false;
}

Vect2i UI_ControlInventory::adjustPosition(const Vect2i& pos, const Vect2i& size) const
{
	Vect2i out_pos = pos;

	if(out_pos.x < 0)
		out_pos.x = 0;

	if(out_pos.x + size.x > size_.x)
		out_pos.x = size_.x - size.x;

	if(out_pos.y < 0)
		out_pos.y = 0;

	if(out_pos.y + size.y > size_.y)
		out_pos.y = size_.y - size.y;

	return out_pos;
}

bool UI_ControlInventory::updateItems(const UI_InventoryItem* item_ptr, int item_count)
{
	MTG();

	items_.clear();

	items_.reserve(item_count);
	for(int i = 0; i < item_count; i++)
		items_.push_back(item_ptr[i]);

	return true;
}

UI_ControlInventory::CellState UI_ControlInventory::cellState(const Vect2i& cell_index) const
{
	MTG();

	if(highlight_.width() && highlight_.height()){
		if(cell_index.x >= highlight_.left() && cell_index.x < highlight_.right() && cell_index.y >= highlight_.top() && cell_index.y < highlight_.bottom())
			return CELL_NOT_EMPTY;
	}

	for(UI_InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
		Recti pos = itemPosition(*it);
		if(cell_index.x >= pos.left() && cell_index.x < pos.right() && cell_index.y >= pos.top() && cell_index.y < pos.bottom())
			return it->isActivated() ? CELL_ACTIVATED : CELL_NOT_EMPTY;
	}

	return CELL_EMPTY;
}


void UI_ControlInventory::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(inventoryType_, "inventoryType", "тип инвентар€");

	if(inventoryType_ == UI_INVENTORY_EQUIPMENT)
		ar.serialize(equipmentSlotType_, "equipmentSlotTypeReference", "тип слота снар€жени€");
	else if(inventoryType_ == UI_INVENTORY_QUICK_ACCESS)
		ar.serialize(quickAccessSlotType_, "quickAccessSlotTypeReference", "тип слота снар€жени€ с активацией");
	else
		ar.serialize(cellType_, "cellTypeReference", "тип €чейки");

	ar.serialize(size_, "size", "количество €чеек");

	ar.serialize(cellEmptySprite_, "cellEmptySprite", "вид пустой €чейки");
	ar.serialize(cellSprite_, "cellSprite", "вид заполненной €чейки");
	if(inventoryType_ == UI_INVENTORY_QUICK_ACCESS)
		ar.serialize(cellActiveSprite_, "cellActiveSprite", "вид активированной €чейки");

	if(ar.isInput()){
		if(size_.x <= 0) size_.x = 1;
		if(size_.y <= 0) size_.y = 1;
	}
}

void UI_ControlInventory::redrawCell(const Rectf& position, CellState state) const
{
	const UI_Sprite* spr = &cellEmptySprite_;
	switch(state){
		case CELL_ACTIVATED:
			if(!cellActiveSprite_.isEmpty())
				spr = &cellActiveSprite_;
			else if(!cellSprite_.isEmpty())
				spr = &cellSprite_;
			break;
		case CELL_NOT_EMPTY:
			if(!cellSprite_.isEmpty())
				spr = &cellSprite_;
			break;

	}

	UI_Render::instance().drawSprite(position, *spr, alpha());
}

void UI_ControlInventory::redrawItem(const UI_InventoryItem& item) const
{
	item.redraw(screenCoords(itemPosition(item)), alpha());
}

Rectf UI_ControlInventory::screenCoords(const Recti& index_coords) const
{
	if(size_.x && size_.y){
		Vect2f pos(position().left(), position().top());
		Vect2f sz(position().width() / float(size_.x), position().height() / float(size_.y));

		return Rectf(pos.x + index_coords.left() * sz.x, pos.y + index_coords.top() * sz.y,
			sz.x * index_coords.width(), sz.y * index_coords.height());
	}
	else
		return Rectf(0,0,0,0);
}

Vect2f UI_ControlInventory::screenCoords(const Vect2i& index_coords) const
{
	if(size_.x && size_.y){
		Vect2f pos(position().left(), position().top());
		Vect2f sz(position().width() / float(size_.x), position().height() / float(size_.y));

		return Vect2f(pos.x + index_coords.x * sz.x, pos.y + index_coords.y * sz.y);
	}
	else
		return Vect2f(0,0);
}

bool UI_ControlInventory::getCellCoords(const Vect2f& screen_coords, Vect2i& index_coords) const
{
	Vect2f pos = screen_coords - Vect2f(position().left(), position().top());
	Vect2f cell_sz(position().width() / float(size_.x), position().height() / float(size_.y));
	pos /= cell_sz;

	index_coords = Vect2i(int(pos.x), int(pos.y));

	return (index_coords.x >= 0 && index_coords.x < size_.x
		&& index_coords.y >= 0 && index_coords.y < size_.y);
}

Vect2i UI_ControlInventory::itemPosition(const Vect2i& cell_index) const
{
	MTG();

	for(UI_InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
		Recti pos = itemPosition(*it);
		if(cell_index.x >= pos.left() && cell_index.x < pos.right() && cell_index.y >= pos.top() && cell_index.y < pos.bottom())
			return pos.left_top();
	}

	return Vect2i(-1,-1);
}

Vect2i UI_ControlInventory::itemSize(const UI_InventoryItem& item) const
{
	return (inventoryType() != UI_INVENTORY) ? Vect2i(1,1) : item.position().size();
}

Recti UI_ControlInventory::itemPosition(const UI_InventoryItem& item) const
{
	if(inventoryType() != UI_INVENTORY){
		Recti pos = item.position();
		pos.size(Vect2i(1,1));
		return pos;
	}
	return item.position();
}

bool UI_ControlInventory::canPutItem(const Vect2i& pos, const Vect2i& size) const
{
	if(!checkPosition(pos, size))
		return false;

	Recti pos0(pos, size);

	for(UI_InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
		Recti pos = itemPosition(*it);
		Recti intersection = pos.intersection(pos0);
		if(intersection.width() && intersection.height())
			return false;
	}

	return true;
}
