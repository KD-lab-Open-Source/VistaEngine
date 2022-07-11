#ifndef __UI_INVENTORY_H__
#define __UI_INVENTORY_H__

#include "UI_Types.h"
#include "UI_Sprite.h"
#include "UI_References.h"

#include "Inventory.h"

class Archive;
class UI_InventoryLogicData;

/// √рафическое представление предмета в инвентаре
class UI_InventoryItem
{
public:
	UI_InventoryItem();
	UI_InventoryItem(const InventoryItem& item, UI_InventoryType inventory_type);
	UI_InventoryItem(const Recti& position, const UI_Sprite& sprite) : position_(position),
		sprite_(&sprite), isActivated_(false) { }

	const Recti& position() const { return position_; }
	void setPosition(const Recti& position){ position_ = position; }

	const UI_Sprite* sprite() const { return sprite_; }
	void setSprite(const UI_Sprite& sprite){ sprite_ = &sprite; }

	bool isActivated() const { return isActivated_; }

	void redraw(const Vect2f& position, float alpha) const;
	void redraw(const Rectf& position, float alpha) const;

private:

	bool isActivated_;

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

	void quant(float dt);
	bool redraw() const;

	const Vect2i& size() const { return size_; }
	int cellType() const;

	bool isEquipment() const { return inventoryType_ == UI_INVENTORY_EQUIPMENT; }
	bool isQuickAccess() const { return inventoryType_ == UI_INVENTORY_QUICK_ACCESS; }
	UI_InventoryType inventoryType() const { return inventoryType_; }

	/// проверка, влезет ли предмет по указанным координатам
	/// лежащие в инвентаре предметы не учитываютс€, смотритс€ только по €чейкам
	bool checkPosition(const Vect2i& pos, const Vect2i& size) const;
	Vect2i adjustPosition(const Vect2i& pos, const Vect2i& size) const;

	bool updateItems(const UI_InventoryItem* item_ptr, int item_count);

	const InventoryItem* hoverItem(const InventorySet* inv, const Vect2f& screen_pos) const;

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
	InventoryCellType cellType_;

	UI_InventoryType inventoryType_;
	/// тип слота (дл€ снар€жени€)
	EquipmentSlotType equipmentSlotType_;
	/// тип слота (дл€ снар€жени€ быстрого доступа)
	QuickAccessSlotType quickAccessSlotType_;

	/// вид €чейки в пустом состо€нии
	UI_Sprite cellEmptySprite_;
	/// вид заполненной €чейки
	UI_Sprite cellSprite_;
	/// вид активированной €чейки (дл€ инвентар€ быстрого доступа)
	UI_Sprite cellActiveSprite_;

	/// подсвеченна€ область (показывает куда л€жет предмет с мыши)
	Recti highlight_;

	UI_InventoryItems items_;

	enum CellState {
		CELL_EMPTY,
		CELL_NOT_EMPTY,
		CELL_HIGHLITED,
		CELL_ACTIVATED
	};

	CellState cellState(const Vect2i& cell_index) const;

	void redrawCell(const Rectf& position, CellState state = CELL_EMPTY) const;
	void redrawItem(const UI_InventoryItem& item) const;

	/// пересчЄт координат из €чеек в экранные
	Rectf screenCoords(const Recti& index_coords) const;
	Vect2f screenCoords(const Vect2i& index_coords) const;

	/// пересчЄт экранных координат в €чейки
	/// возвращает false, если точка не попадает в сетку
	bool getCellCoords(const Vect2f& screen_coords, Vect2i& index_coords) const;

	/// список предметов, лежащих в инвентаре
	/// берЄтс€ из логического диспетчера интерфейса
	const UI_InventoryItems* items() const { return &items_; }

	Vect2i itemSize(const UI_InventoryItem& item) const;
	Recti itemPosition(const UI_InventoryItem& item) const;
	Vect2i itemPosition(const Vect2i& index_coords) const;

	bool canPutItem(const Vect2i& pos, const Vect2i& size) const;
};

typedef UI_ControlReferenceRefined<UI_ControlInventory> UI_InventoryReference;
typedef vector<UI_InventoryReference> UI_InventoryReferences;

#endif /* __UI_INVENTORY_H__ */

