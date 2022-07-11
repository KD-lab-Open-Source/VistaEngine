#ifndef __UI_INVENTORY_H__
#define __UI_INVENTORY_H__

#include "UI_Types.h"
#include "UI_References.h"

class Archive;
class Inventory;
class InventoryItem;
class UI_InventoryLogicData;

/// √рафическое представление предмета в инвентаре
class UI_InventoryItem
{
public:
	UI_InventoryItem();
	explicit UI_InventoryItem(const InventoryItem& item);
	UI_InventoryItem(const Recti& position, const UI_Sprite& sprite) : position_(position),
		sprite_(&sprite) { }

	const Recti& position() const { return position_; }
	void setPosition(const Recti& position){ position_ = position; }

	const UI_Sprite* sprite() const { return sprite_; }
	void setSprite(const UI_Sprite& sprite){ sprite_ = &sprite; }

	void redraw(const Rectf& position, float alpha) const;

private:

	/// занимаемое в инвентаре пространство
	Recti position_;

	/// внешний вид предмета
	const UI_Sprite* sprite_;
};

typedef std::vector<UI_InventoryItem> UI_InventoryItems;

/// »нвентарь, графическое представление.
class UI_ControlInventory : public UI_ControlBase
{
public:
	UI_ControlInventory();
	~UI_ControlInventory();

	bool redraw() const;

	const Vect2i& size() const { return size_; }
	int cellType() const { return cellType_; }

	bool isEquipment() const { return isEquipment_; }

	/// проверка, влезет ли предмет по указанным координатам
	/// лежащие в инвентаре предметы не учитываютс€, смотритс€ только по €чейкам
	bool checkPosition(const Vect2i& pos, const Vect2i& size) const;
	Vect2i adjustPosition(const Vect2i& pos, const Vect2i& size) const;

	bool updateItems(const UI_InventoryItem* item_ptr, int item_count);

	void serialize(Archive& ar);

protected:

	bool inputEventHandler(const UI_InputEvent& event);

private:

	/// количество €чеек
	Vect2i size_;

	/// тип €чеек
	/** 
	по этому типу провер€етс€, может ли объект
	быть положен в €чейку
	*/
	int cellType_;

	/// инвентарь - снар€жение
	bool isEquipment_;

	/// вид €чейки в пустом состо€нии
	UI_Sprite cellEmptySprite_;
	/// вид заполненной €чейки
	UI_Sprite cellSprite_;

	UI_InventoryItems items_;

	/// возвращает true, если в €чейке \a cell_index ничего не лежит
	bool isCellEmpty(const Vect2i& cell_index) const;

	void redrawCell(const Rectf& position, bool empty_state) const;
	void redrawItem(const UI_InventoryItem& item) const;

	/// пересчЄт координат из €чеек в экранные
	Rectf screenCoords(const Recti& index_coords) const;
	/// пересчЄт координат из €чеек в экранные
	Vect2i screenCoords(const Vect2i& index_coords) const;

	/// пересчЄт экранных координат в €чейки
	/// возвращает false, если точка не попадает в сетку
	bool getCellCoords(const Vect2f& screen_coords, Vect2i& index_coords) const;

	/// список предметов, лежащих в инвентаре
	/// берЄтс€ из логического диспетчера интерфейса
	const UI_InventoryItems* items() const { return &items_; }

	Vect2i itemSize(const UI_InventoryItem& item) const;
	Recti itemPosition(const UI_InventoryItem& item) const;
};

typedef UI_ControlReferenceRefined<UI_ControlInventory> UI_InventoryReference;
typedef std::list<UI_InventoryReference> UI_InventoryReferences;

#endif /* __UI_INVENTORY_H__ */

