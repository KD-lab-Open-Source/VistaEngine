#ifndef __SELECT_MANAGER_H__
#define __SELECT_MANAGER_H__

#include "UnitLink.h"
#include "UI_Enums.h"
#include "Timers.h"

class Player;
class AttributeBase;
class UnitInterface;
class UnitItemResource;
class UnitItemInventory;
class UnitReal;
class UnitActing;
class UnitBuilding;
class WeaponTarget;
class UnitCommand;
enum CommandID;

typedef vector<UnitInterface*> UnitInterfaceList; 

typedef UnitLink<UnitInterface> UnitInterfaceLink;
typedef vector<UnitInterfaceLink> UnitInterfaceLinks;
typedef vector<UnitInterfaceList> UnitInterfaceListsContainer;

class SelectManager
{
public:
	struct Slot
	{
		Slot(UnitInterface* unit);
		UnitInterfaceList units;

		const AttributeBase* attr() const;
		bool uniform() const;
		bool test(UnitInterface* unit);
	};
	typedef vector<Slot> Slots;

	SelectManager();
	~SelectManager();

	void quant();
	
	void setPlayer(Player* player) { player_ = player; }
	void clearPlayer() { player_ = 0; }

	// номер активного слота
	int selectedSlot() const { return selectedSlot_; }
	// любой юнит из активного слота
	UnitInterface* selectedUnit();
	// юнит из слота, но только если там не пачка, (-1) - из активного слота
	UnitInterface* getUnitIfOne(int slotIndex = -1);
	// selectionAttribute юнита в активном слоте
	const AttributeBase* selectedAttribute() const { return selectedAttribute_; }
	// список слотов селекта
	void getSelectList(Slots& out);
	// в селекте никого и ничего нет
	bool isSelectionEmpty() const { return selectionSize_ == 0; }
	// количество юнитов в селекте
	int selectionSize() const { return selectionSize_; }
	/// флаг однородности селекта (все юниты одного типа (включая внутри сквада) и одного уровня)
	bool uniform();

	bool selectArea(const Vect2f& p0, const Vect2f& p1, bool multi, UnitInterface* startTrakingUnit);
	bool selectUnit(UnitInterface* p, bool shift_pressed, bool no_deselect = false);
	bool selectUnitForced(UnitInterface* p);
	void selectAll(const AttributeBase* p);
	void selectAllOnScreen(const AttributeBase* p);
	
	void changeUnit(UnitInterface* oldUnit, UnitInterface* newUnit);

	void sendCommandSlot(const UnitCommand& command, int slotIndex);

	void makeCommand(const UnitCommand& command, bool forSelectedObjectOnly = false);
	void makeCommandAttack(WeaponTarget& target, bool shiftPressed = false);
	void makeCommandSubtle(const CommandID& command_id, const Vect3f& position, bool shiftPressed = false);

	void deselectAll();
	void deselectUnit(UnitInterface* unit);

	void setSelectedObject(int index);
	void leaveOnlySlot(int index);
	void leaveOnlyType(int index);

	void deselectObject(int index);

	void saveSelection(int num);
	void restoreSelection(int num);
	void addSelection(int num);
	void subSelection(int num);

	bool canAttack(const WeaponTarget& target, bool checkDistance = false, bool check_fow = false) const;
	bool canPickItem(const UnitItemInventory* item) const;
	bool canExtractResource(const UnitItemResource* item) const;
	bool canBuild(const UnitReal* building) const;
	bool canRun() const;
	bool canPutInTransport(const UnitActing* transport) const;
	bool canTeleportate(const UnitBuilding* teleport) const;

	bool isSelected(const AttributeBase* attribute, bool singleOnly, bool needUniform);
	bool isSquadSelected(const AttributeBase* attribute, bool singleOnly) const;
	/// возвращает true, если заселекчен транспорт с юнитом определённого типа на борту
	bool isInTransport(const AttributeBase* attribute, bool singleOnly) const;

	bool squadsMerge();
	Accessibility canMergeSquads() const;
	bool squadSplit();
	bool canSplitSquad() const;

	void explodeUnit();

	void serialize (Archive& ar);
	MTSection& getLock(){ return selectLock_; }

private:
	bool selected_valid_;
	bool tested_uniform_;
	bool uniformSelection_;
	bool uniformSquads_;

	int lastRestoredSelection_;

	int selectionSize_;

	MTSection selectLock_;
	
	UnitInterfaceList selection_;
	Slots slots_;
	UnitInterface* selectedUnit_;
	const AttributeBase* selectedAttribute_;
	int selectedSlot_;

	// активный слот
	void validateSelectedObject();

	UnitInterfaceListsContainer savedSelections_;

	class Transaction
	{
		SelectManager* manager_;
		UnitInterfaceList selection_;
		bool silentChange_;

		void beginTransaction();
		void endTransaction();
	public:
		Transaction(SelectManager* manager, bool silent = false){
			manager_ = manager;
			silentChange_ = silent;
			beginTransaction();
		}
		~Transaction() {
			endTransaction();
		}
	};

	Player* player_;

	UnitInterfaceList::iterator findInSelection(UnitInterface* p);
	UnitInterfaceList::iterator erase(UnitInterfaceList::iterator it);
	void push(UnitInterface* p);

	void addToSelection(UnitInterface* p);

	void buildSlots();
	
	void testUniform();

	void clear();

	enum UnitSelectionPriority{
		SQUAD,
		BUILDING,
		OTHER
	};
	UnitSelectionPriority selectionPriority_;
	UnitSelectionPriority unitSelectionPriority(UnitInterface* p) const;
	
	void updateSelectionPriority();
	void handleChange(bool wasChanged);
};

extern SelectManager* selectManager;

#define CSELECT_AUTOLOCK() MTAuto select_autolock(selectManager->getLock())

#endif /* __SELECT_MANAGER_H__ */
