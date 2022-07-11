#include "StdAfx.h"

#include "Serialization.h"

#include "UI_Render.h"
#include "UI_Inventory.h"
#include "UI_Logic.h"
#include "Inventory.h"

// ----------------------------- UI_InventoryItem

UI_InventoryItem::UI_InventoryItem() : sprite_(0)
{
	position_ = Recti(0,0,1,1);
}

UI_InventoryItem::UI_InventoryItem(const InventoryItem& item) : sprite_(&item.sprite())
{
	position_ = Recti(item.position(), item.size());
}

void UI_InventoryItem::redraw(const Rectf& position, float alpha) const
{
	if(!sprite_)
		return;

	Rectf pos = position;
	Vect2f sprite_size = UI_Render::instance().relativeSpriteSize(*sprite_);

	pos.left(pos.left() + (pos.width() - sprite_size.x)/2.f);
	pos.top(pos.top() + (pos.height() - sprite_size.y)/2.f);

	pos.width(sprite_size.x);
	pos.height(sprite_size.y);

	UI_Render::instance().drawSprite(pos, *sprite_, alpha);
}

// ----------------------------- UI_ControlInventory

UI_ControlInventory::UI_ControlInventory()
{
	size_ = Vect2i(1,1);

	cellType_ = 0;
	isEquipment_ = false;
}

UI_ControlInventory::~UI_ControlInventory()
{
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
				redrawCell(Rectf(pos, cell_sz), isCellEmpty(Vect2i(j, i)));
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

bool UI_ControlInventory::isCellEmpty(const Vect2i& cell_index) const
{
	MTG();

	for(UI_InventoryItems::const_iterator it = items_.begin(); it != items_.end(); ++it){
		Recti pos = itemPosition(*it);
		if(cell_index.x >= pos.left() && cell_index.x < pos.right() && cell_index.y >= pos.top() && cell_index.y < pos.bottom())
				return false;
	}

	return true;
}

void UI_ControlInventory::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(isEquipment_, "isEquipment", "снар€жение");

	ar.serialize(size_, "size", "количество €чеек");
	ar.serialize(cellType_, "cellType", "тип €чейки");

	ar.serialize(cellEmptySprite_, "cellEmptySprite", "вид пустой €чейки");
	ar.serialize(cellSprite_, "cellSprite", "вид заполненной €чейки");

	if(ar.isInput()){
		if(size_.x <= 0) size_.x = 1;
		if(size_.y <= 0) size_.y = 1;
	}
	
	postSerialize();
}

void UI_ControlInventory::redrawCell(const Rectf& position, bool empty_state) const
{
	const UI_Sprite& spr = (!empty_state || cellSprite_.isEmpty()) ? cellSprite_ : cellEmptySprite_;
	UI_Render::instance().drawSprite(position, spr, alpha());
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

Vect2i UI_ControlInventory::screenCoords(const Vect2i& index_coords) const
{
	if(size_.x && size_.y)
		return Vect2f(position().width() / float(size_.x) * index_coords.x,
			position().height() / float(size_.y) * index_coords.y);
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
		&& index_coords.y >= 0 && index_coords.x < size_.y);
}

Vect2i UI_ControlInventory::itemSize(const UI_InventoryItem& item) const
{
	return isEquipment() ? Vect2i(1,1) : item.position().size();
}

Recti UI_ControlInventory::itemPosition(const UI_InventoryItem& item) const
{
	if(isEquipment()){
		Recti pos = item.position();
		pos.size(Vect2i(1,1));
		return pos;
	}
	return item.position();
}
