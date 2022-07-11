#include "StdAfx.h"
#include "SelectManager.h"
#include "UnitInterface.h"
#include "CameraManager.h"
#include "GameShell.h"
#include "Squad.h"
#include "IronBuilding.h"
#include "Units\PositionGeneratorCircle.h"
#include "UI_Logic.h"
#include "UnitAttribute.h"
#include "GameCommands.h"

__forceinline UnitInterface* getSquadUnit(UnitInterface* p)
{
	if(UnitInterface* squad = p->getSquadPoint())
		return squad;
	return p;
}

SelectManager* selectManager;

SelectManager::SelectManager()
:selectionPriority_(OTHER),
tested_uniform_(true),
uniformSelection_(true),
uniformSquads_(true),
selected_valid_(true)
{
	player_=NULL;
	selectedUnit_ = 0;
	selectedSlot_ = -1;
	selectedAttribute_ = 0;
	savedSelections_.resize(SAVED_SELECTION_MAX);
	selectionSize_ = 0;

	lastRestoredSelection_ = -1;
}

SelectManager::~SelectManager()
{
}

void SelectManager::clear()
{
	selection_.clear();
	selectionPriority_ = OTHER;
	tested_uniform_ = true;
	uniformSelection_ = true;
	uniformSquads_ = true;
	selected_valid_ = false;
}

UnitInterfaceList::iterator SelectManager::erase(UnitInterfaceList::iterator it)
{
	if(selection_.size() == 1){
		clear();
		return selection_.end();
	}
	selected_valid_ = false;
	tested_uniform_ = uniformSelection_;
	it = selection_.erase(it);
	return it;
}

SelectManager::Slot::Slot(UnitInterface* unit)
{
	units.push_back(unit);
}

const AttributeBase* SelectManager::Slot::attr() const
{
	return units.front()->selectionAttribute();
}

bool SelectManager::Slot::uniform() const
{
	return units.size() > 1 ? true : units.front()->uniform();
}

bool SelectManager::Slot::test(UnitInterface* unit)
{
	if(unit->selectionAttribute() == attr())
		return find(units.begin(), units.end(), getSquadUnit(unit)) != units.end();
	return false;
}

void SelectManager::Transaction::beginTransaction()
{
	xassert(manager_->selectLock_.locked());
	selection_ = manager_->selection_;
}

void SelectManager::Transaction::endTransaction()
{
	manager_->selectionSize_ = manager_->selection_.size();

	UnitInterfaceList listForSelect;
	UnitInterfaceList listForDeselect;

	if(selection_.empty()){
		// селект был пустой, возможно кого-то заселектили
		if(!manager_->selection_.empty()){
			UnitInterfaceList::iterator it;
			FOR_EACH(manager_->selection_, it)
				listForSelect.push_back(*it);
		}
	}
	else if(manager_->selection_.empty()){
		// всех деселектили
		UnitInterfaceList::iterator it;
		FOR_EACH(selection_, it)
			listForDeselect.push_back(*it);
	}
	else {
		// возможно изменился состав селекта
		UnitInterfaceList select = manager_->selection_;
		sort(select.begin(), select.end());
		
		sort(selection_.begin(), selection_.end());
		
		UnitInterfaceList::iterator oit = selection_.begin();
		UnitInterfaceList::iterator nit = select.begin();
		
		for(;;){
			UnitInterface* ou = *oit;
			UnitInterface* nu = *nit;

			if(nu == ou){
				++nit;
				++oit;
			}
			else if(nu > ou){
				listForDeselect.push_back(ou);
				++oit;
			}
			else{
				listForSelect.push_back(nu);
				++nit;
			}

			if(oit == selection_.end()){
				while(nit != select.end()){
					listForSelect.push_back(*nit);
					++nit;
				}
				break;
			}
			
			if(nit == select.end()){
				while(oit != selection_.end()){
					listForDeselect.push_back(*oit);
					++oit;
				}
				break;
			}
		}
	}

	UnitInterfaceList::iterator it;
	FOR_EACH(listForSelect, it)
		(*it)->setSelected(true);

	FOR_EACH(listForDeselect, it)
		(*it)->setSelected(false);

	manager_->handleChange(!listForDeselect.empty() || !listForSelect.empty());

	UnitCommand command(COMMAND_ID_UNIT_SELECTED, manager_->player_->teamIndex());
	command.setShiftModifier(!silentChange_);
	if(!listForSelect.empty())
		universe()->sendCommand(netCommand4G_UnitListCommand(listForSelect, command));
	if(!listForDeselect.empty())
		universe()->sendCommand(netCommand4G_UnitListCommand(listForDeselect, UnitCommand(COMMAND_ID_UNIT_DESELECTED, manager_->player_->teamIndex())));
}

inline UnitInterfaceList::iterator SelectManager::findInSelection(UnitInterface* p) 
{
	return find(selection_.begin(), selection_.end(), getSquadUnit(p));
}

void SelectManager::handleChange(bool wasChanged)
{
	if(wasChanged){
		lastRestoredSelection_ = -1;
		buildSlots();
		updateSelectionPriority();
	}
}

void SelectManager::deselectUnit(UnitInterface* unit)
{
	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	UnitInterfaceList::iterator it = findInSelection(unit);
	if(it != selection_.end())
		erase(it);
}

void SelectManager::saveSelection(int num)
{
	xassert(num >= 0 && num < SAVED_SELECTION_MAX);
	MTAuto select_autolock(selectLock_);
	
	if(unsigned int size = selection_.size()){
		savedSelections_[num].resize(size);
		std::copy(selection_.begin(), selection_.end(), savedSelections_[num].begin());
	}
	else
		savedSelections_[num].clear();
}

void SelectManager::restoreSelection(int num)
{
	xassert(num >= 0 && num < SAVED_SELECTION_MAX);

	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	MTAuto select_autolock(selectLock_);

	if(savedSelections_[num].empty())
		return;

	UnitInterfaceList units;
	UnitInterfaceList::iterator lnk;
	FOR_EACH(savedSelections_[num], lnk)
		if((*lnk)->alive())
			units.push_back(*lnk);
	
	if(units.empty())
		return;

	{
		Transaction auto_transaction(this);

		clear();

		UnitInterfaceList::const_iterator it;
		FOR_EACH(units, it)
			addToSelection(*it);
	}

	if(lastRestoredSelection_ == num){
		Vect2f avg_pos(0.f, 0.f);
		float unit_count = 0.f;
		UnitInterfaceList::const_iterator it;
		FOR_EACH(selection_, it){
			float weight = 0;
			if(UnitSquad* squad = (*it)->getSquadPoint()){
				const LegionariesLinks& units = squad->units();
				LegionariesLinks::const_iterator sit;
				FOR_EACH(units, sit)
					weight += sqrtf((*sit)->radius()) * clamp((*sit)->selectionListPriority() + 1, 1, 3);
			}
			else
				weight = sqrtf((*it)->radius());
			avg_pos += (*it)->position2D() * weight;
			unit_count += weight;
		}
		xassert(unit_count > FLT_EPS);
		avg_pos /= unit_count;

		CameraCoordinate coord = cameraManager->coordinate();
		coord.setPosition(To3D(avg_pos));
		cameraManager->setCoordinate(coord);
		
		lastRestoredSelection_ = -1;
	}
	else
		lastRestoredSelection_ = num;
}

void SelectManager::addSelection(int num)
{
	xassert(num >= 0 && num < SAVED_SELECTION_MAX);

	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	UnitInterfaceList::const_iterator it;
	FOR_EACH(savedSelections_[num], it)
		if(*it && (*it)->alive())
			if(findInSelection(*it) == selection_.end())
				addToSelection(*it);
}

void SelectManager::subSelection(int num)
{
	xassert(num >= 0 && num < SAVED_SELECTION_MAX);

	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	UnitInterfaceList::const_iterator it;
	FOR_EACH(savedSelections_[num], it)
		if(*it && (*it)->alive()){
			UnitInterfaceList::iterator uit = findInSelection(*it);
			if(uit != selection_.end())
				erase(uit);
		}
}

struct ComparePriotity{
	bool operator() (const UnitInterface* unit1, const UnitInterface* unit2)
	{ return unit1->prior(unit2); }
};

void SelectManager::buildSlots()
{
	xassert(getLock().locked());

	selectedSlot_ = -1;
	slots_.clear();
	if(selection_.empty())
		return;
	
	if(selection_.size() > 1)
		std::stable_sort(selection_.begin(), selection_.end(), ComparePriotity());

	selectedSlot_ = 0;

	if(selection_.size() > player_->race()->selectQuantityMax){
		xassert(GlobalAttributes::instance().useStackSelect);
		selected_valid_ = false;
		UnitInterfaceList select = selection_;
		while(slots_.size() + select.size() > player_->race()->selectQuantityMax){
			if(select.empty())
				break;
			UnitInterface* cur = select.back();
			slots_.push_back(Slot(cur));
			for(;;) {
				select.pop_back();
				if(select.empty() || !select.back()->uniform(cur->getUnitReal()))
					break;
				if(selectedUnit_ == select.back())
					selectedSlot_ = slots_.size() - 1;
				slots_.back().units.push_back(select.back());
			}
		}
		for(UnitInterfaceList::reverse_iterator it = select.rbegin(); it != select.rend(); ++it)
			slots_.push_back(Slot(*it));
		reverse(slots_.begin(), slots_.end());
		if(selectedUnit_)
			selectedSlot_ = (slots_.size() - selectedSlot_) - 1;
	}
	else {
		UnitInterfaceList::iterator it;
		FOR_EACH(selection_, it){
			slots_.push_back(Slot(*it));
			if(selectedUnit_ == *it)
				selectedSlot_ = slots_.size() - 1;
		}
	}
}

void SelectManager::testUniform()
{
	xassert(getLock().locked());
	tested_uniform_ = true;	
	uniformSelection_ = true;
	uniformSquads_ = true;
	
	if(!selection_.empty()){
		UnitInterfaceList::const_iterator it = selection_.begin();
		
		const UnitInterface* prev = *it;
		uniformSelection_ = prev->uniform();
		
		while(++it != selection_.end()){
			const UnitInterface* unit = *it;
			
			uniformSquads_ = (&prev->attr() == &unit->attr());
			uniformSelection_ = (uniformSquads_ && uniformSelection_
				&& prev->selectionAttribute() == unit->selectionAttribute()
				&& unit->uniform(prev->getUnitReal()));
			
			if(!uniformSquads_ && !uniformSelection_)
				break;
			
			prev = unit;
		}
	}
}

bool SelectManager::uniform()
{
	if(tested_uniform_)
		return uniformSelection_;
	
	MTAuto select_autolock(selectLock_);

	testUniform();
	return uniformSelection_;
}

void SelectManager::quant()
{
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	if(!changeData_.empty()){
		ChangeData::iterator ci;
		FOR_EACH(changeData_, ci){
			UnitInterfaceList::iterator it = findInSelection(ci->oldSquad);
			if(it != selection_.end())
				erase(it);
		}

		FOR_EACH(changeData_, ci){
			addToSelection(ci->newSquad);

			UnitInterfaceListsContainer::iterator lst;
			FOR_EACH(savedSelections_, lst){
				UnitInterfaceListsContainer::value_type::iterator lnk;
				FOR_EACH(*lst, lnk)
					if(*lnk == ci->oldSquad)
						*lnk = ci->newSquad;
			}
		}
		changeData_.clear();
	}

	for(UnitInterfaceList::iterator it = selection_.begin(); it != selection_.end();){
		if((*it)->player() != player_ || !(*it)->alive())
			it = erase(it);
		else
			it++;
	}
	
	if(selectedUnit_ && !selectedUnit_->alive()){
		selectedUnit_ = 0;
		selected_valid_ = false;
	}
	
	tested_uniform_ = false;

	UnitInterfaceListsContainer::iterator sel;
	FOR_EACH(savedSelections_, sel)
		for(UnitInterfaceList::iterator uit = sel->begin(); uit != sel->end();)
			if((*uit)->alive())
				++uit;
			else
				uit = sel->erase(uit);
}

inline SelectManager::UnitSelectionPriority SelectManager::unitSelectionPriority(UnitInterface* p) const
{
	if(p->getSquadPoint())
		return SQUAD;
	else if(p->attr().isBuilding())
		return BUILDING;

	return OTHER;
}

void SelectManager::updateSelectionPriority()
{
	xassert(getLock().locked());
	selectionPriority_ = OTHER;
	UnitInterfaceList::const_iterator it;
	FOR_EACH(selection_,it)
		selectionPriority_ = min(selectionPriority_, unitSelectionPriority(*it));
}

bool SelectManager::selectArea(const Vect2f& p0, const Vect2f& p1, bool multi, UnitInterface* startTrakingUnit)
{
	MTG();
	xassert(player_);
	
	if(!player_ || gameShell->directControl() || !player_->controlEnabled()) 
		return false;

	if(p0.distance2(p1) < FLT_COMPARE_TOLERANCE)
		return false;

	float x0, x1, y0, y1;

	if(p0.x < p1.x){
		x0 = p0.x;
		x1 = p1.x;
	}
	else {
		x0 = p1.x;
		x1 = p0.x;
	}
	
	if(p0.y < p1.y){
		y0 = p0.y;
		y1 = p1.y;
	}
	else {
		y0 = p1.y;
		y1 = p0.y;
	}

	UI_LogicDispatcher::instance().cancelActions();
	
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	if(!multi)
		clear();

	UnitInterfaceList unit_list;
	gameShell->unitsInArea(Rectf(x0, y0, x1-x0, y1-y0), unit_list, startTrakingUnit);
	
	UnitInterfaceList::iterator unitIter;
	FOR_EACH(unit_list, unitIter)
		addToSelection(*unitIter);

	return true;
}

bool SelectManager::selectUnit(UnitInterface* p, bool shiftPressed, bool no_deselect) 
{
	MTG();
	if(!player_ || !player_->controlEnabled()) 
		return false;

	UI_LogicDispatcher::instance().cancelActions();
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	if(selection_.empty())
		addToSelection(p);
	else if(!shiftPressed){
		clear();
		addToSelection(p);
	}
	else {
		UnitInterfaceList::iterator it = findInSelection(p);
		if(it != selection_.end()){
			if(!no_deselect)
				erase(it);
			else if(!p->selected()){ // незаселекченный юнит в заселекченном скваде
				p->setSelected(true);
				UnitCommand command(COMMAND_ID_UNIT_SELECTED, player_->teamIndex());
				command.setShiftModifier(false);
				universe()->sendCommand(netCommand4G_UnitCommand(p->unitID().index(), command));
			}
		}
		else
			addToSelection(p);
	}
	
	return true;
}

bool SelectManager::selectUnitForced(UnitInterface* p)
{
	MTG();

	UI_LogicDispatcher::instance().cancelActions();
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	if(selection_.empty())
		addToSelection(p);
	else {
		clear();
		addToSelection(p);
	}

	return true;
}

void SelectManager::selectAllOnScreen(const AttributeBase* unitType) 
{
	MTG();
	xassert(player_);
	if(!player_ || gameShell->directControl() || !player_->controlEnabled()) 
		return;

	UI_LogicDispatcher::instance().cancelActions();
	
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	clear();

	UnitInterfaceList unit_list;
	gameShell->unitsInArea(Rectf(-0.5f, -0.5f, 1.f, 1.f), unit_list, 0, unitType);
	
	UnitInterfaceList::iterator unitIter;
	FOR_EACH(unit_list, unitIter)
		addToSelection(*unitIter);
}

void SelectManager::selectAll(const AttributeBase* unitType, bool onlyPowered) 
{
	MTG();
	xassert(unitType);
	if(!player_ || gameShell->directControl() || !player_->controlEnabled()) 
		return;

	UI_LogicDispatcher::instance().cancelActions();
	
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);

	clear();

	CUNITS_LOCK(player_);
	
	if(unitType->isSquad()){
		const SquadList& unit_list = player_->squadList(safe_cast<const AttributeSquad*>(unitType));
		SquadList::const_iterator un_it;
		FOR_EACH(unit_list, un_it)
				addToSelection(*un_it);
	}
	else {
		const RealUnits& unit_list = player_->realUnits(unitType);
		RealUnits::const_iterator un_it;
		FOR_EACH(unit_list, un_it){
			UnitObjective* unit = *un_it;
			if(unit->inTransport()){
				UnitLegionary* l_unit = safe_cast<UnitLegionary*>(unit);
				if(l_unit->transport())
					addToSelection(l_unit->transport());
			}
			else if(onlyPowered){
				if(unitType->isBuilding() && safe_cast<UnitBuilding*>(unit)->isConnected())
					addToSelection(unit);
			}
			else
				addToSelection(unit);
		}
	}
}

void SelectManager::changeUnit(UnitInterface* oldUnit, UnitInterface* newUnit)
{
	xassert(newUnit != oldUnit);
	MTAuto select_autolock(selectLock_);

	UnitInterface* oldSquad = getSquadUnit(oldUnit);
	UnitInterface* newSquad = getSquadUnit(newUnit);

	if(oldSquad->selected() && oldSquad != newSquad)
		changeData_.push_back(ChangeDatum(oldSquad, newSquad));
}

inline bool sortByLogicRadius(UnitInterface* unit1, UnitInterface* unit2)
{
	return unit1->radius() > unit2->radius();
}

void SelectManager::makeCommandSubtle(const CommandID& command_id, const Vect3f& position, bool shiftPressed, bool byMinimap)
{
	if(gameShell->underFullDirectControl() || !gameShell->controlEnabled())
		return;

	MTG();
	MTAuto select_autolock(selectLock_);
	if(!selection_.empty()){
		UnitCommand unitCommand(command_id, To3D(position), 0);
		unitCommand.setShiftModifier(shiftPressed);
		unitCommand.setMiniMap(byMinimap);

		if(selection_.size() > 1){
			UnitInterfaceList selectionCommandCopy = selection_;
			std::sort(selectionCommandCopy.begin(), selectionCommandCopy.end(), sortByLogicRadius);
			universe()->sendCommand(netCommand4G_UnitListCommand(selectionCommandCopy, unitCommand, true));
		}
		else
			selection_.front()->sendCommand(unitCommand);
	}
}

void SelectManager::makeCommand(const UnitCommand& command, bool forSelectedObjectOnly)
{
	MTG();
	if(!gameShell->controlEnabled())
		return;
	MTAuto select_autolock(selectLock_);
	if(forSelectedObjectOnly){
		validateSelectedObject();
		if(selectedSlot_ >= 0)
			sendCommandSlot(command, selectedSlot_);
	}
	else if(selection_.size() > 1)
		universe()->sendCommand(netCommand4G_UnitListCommand(selection_, command));
	else if(!selection_.empty())
			selection_.front()->sendCommand(command);
}

void SelectManager::makeCommandAttack(WeaponTarget& target, bool shiftPressed, bool byMinimap, bool moveToTarget)
{
	if(gameShell->underFullDirectControl())
		return;

	MTG();
	MTAuto select_autolock(selectLock_);

	UnitCommand cmd = moveToTarget 
		? (target.unit()
			? UnitCommand(COMMAND_ID_OBJECT, target.unit(), target.weaponID())
			: UnitCommand(COMMAND_ID_ATTACK, target.position(), target.weaponID()))
		: (target.unit()
			? UnitCommand(COMMAND_ID_FIRE_OBJECT, target.unit(), target.weaponID())
			: UnitCommand(COMMAND_ID_FIRE, target.position(), target.weaponID()));

	if(!shiftPressed)
	if(const WeaponPrm* prm = WeaponPrm::getWeaponPrmByID(target.weaponID()))
		if(prm->alwaysPutInQueue())
			shiftPressed = true;

	cmd.setShiftModifier(shiftPressed);
	cmd.setMiniMap(byMinimap);

	UnitInterfaceList::iterator ui;
	FOR_EACH(selection_, ui){
		if((*ui)->canAttackTarget(target))
			(*ui)->sendCommand(cmd);
	}
}

void SelectManager::sendCommandSlot(const UnitCommand& command, int slotIndex)
{
	MTG();
	MTAuto select_autolock(selectLock_);
	if(slotIndex >= slots_.size())
		return;
	xassert(slotIndex >= 0);
	UnitInterfaceList::iterator ui;
	FOR_EACH(slots_[slotIndex].units, ui)
		(*ui)->sendCommand(command);
}

inline void SelectManager::push(UnitInterface* p)
{
	xassert(getLock().locked());
	if(findInSelection(p) == selection_.end()){
		selection_.push_back(p);
		tested_uniform_ = false;
	}
}

void SelectManager::addToSelection(UnitInterface* p)
{
	xassert(getLock().locked());
	if(!p->alive() || !p->selectAble() || p->player() != player_)
		return;
	p = getSquadUnit(p);
	UnitSelectionPriority unitPriority = unitSelectionPriority(p);
	if(unitPriority == selectionPriority_){
		if(GlobalAttributes::instance().useStackSelect || selection_.size() < player_->race()->selectQuantityMax)
			push(p);
	}
	else if(unitPriority < selectionPriority_){
		clear();
		selectionPriority_ = unitPriority;
		push(p);
	}
}

void SelectManager::explodeUnit()
{
	MTG();
	MTAuto select_autolock(selectLock_);
	universe()->sendCommand(netCommand4G_UnitListCommand(selection_, UnitCommand(COMMAND_ID_EXPLODE_UNIT_DEBUG, 0)));
}

void SelectManager::deselectAll()
{
	MTAuto select_autolock(selectLock_);

	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	Transaction auto_transaction(this);

	clear();
}

void SelectManager::deselectObject(int slotIndex)
{
	if(isSelectionEmpty())
		return;

	MTAuto select_autolock(selectLock_);

	if(slotIndex >= slots_.size())
		return;
	xassert(slotIndex >= 0);

	Transaction auto_transaction(this);

	erase(findInSelection(slots_[slotIndex].units.front()));
}

void SelectManager::setSelectedObject(int slotIndex)
{
	MTAuto select_autolock(selectLock_);

	if(slotIndex < 0){
		selectedSlot_ = -1;
		selectedAttribute_ = 0;
		selectedUnit_ = 0;
	}
	else if(slotIndex < slots_.size()){
		selectedSlot_ = slotIndex;
		selectedAttribute_ = slots_[selectedSlot_].attr();
		selectedUnit_ = slots_[selectedSlot_].units.front();
	}
	
	selected_valid_ = true;
}

void SelectManager::leaveOnlySlot(int index)
{
	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	MTAuto select_autolock(selectLock_);

	if(index < 0 && index >= slots_.size())
		return;

	Transaction auto_transaction(this);

	Slot slot = slots_[index];
	clear();
	UnitInterfaceList::iterator it;
	FOR_EACH(slot.units, it)
		addToSelection(*it);
}

void SelectManager::leaveOnlyType(int index)
{
	if(gameShell->directControl() || !player_->controlEnabled())
		return;

	MTAuto select_autolock(selectLock_);

	if(index < 0 && index >= slots_.size())
		return;

	const AttributeBase* attr = slots_[index].attr();

	Transaction auto_transaction(this);
	
	UnitInterfaceList::iterator it = selection_.begin();
	while(it != selection_.end())
		if((*it)->selectionAttribute() != attr)
			it = erase(it);
		else
			it++;
}

void SelectManager::validateSelectedObject()
{ 
	xassert(getLock().locked());

	if(slots_.empty()){
		xassert(selection_.empty());
		setSelectedObject(-1);
		return;
	}

	xassert(selectedSlot_ >= 0 && selectedSlot_ < slots_.size());

	if(UnitInterface* unit = selectedUnit_){
		if(selected_valid_){
			dassert(slots_[selectedSlot_].test(unit));
			return;
		}
		else if(slots_[selectedSlot_].test(unit)){
			selectedAttribute_ = slots_[selectedSlot_].attr();
			selected_valid_ = true;
			return;
		}
		int idx = 0;
		Slots::iterator it;
		FOR_EACH(slots_, it)
			if(it->test(unit)){
				setSelectedObject(idx);
				return;
			}
			else
				++idx;
	}
	
	if(GlobalAttributes::instance().useStackSelect && selectedAttribute_){
		int idx = 0;
		Slots::const_iterator it;
		FOR_EACH(slots_, it)
			if(it->attr() ==  selectedAttribute_){
				setSelectedObject(idx);
				return;
			}
			else
				++idx;
	}

	setSelectedObject(0);
}

UnitInterface* SelectManager::selectedUnit()
{
	if(UnitInterface* unit = selectedUnit_)
		if(selected_valid_)
			return unit;

	if(isSelectionEmpty())
		return 0;

	MTAuto select_autolock(selectLock_);
	
	validateSelectedObject();
	return selectedUnit_;
}

UnitInterface* SelectManager::getUnitIfOne(int slotIndex)
{
	if(isSelectionEmpty())
		return 0;

	MTAuto select_autolock(selectLock_);

	if(slotIndex < 0){
		validateSelectedObject();
		if(selectedSlot_ >= 0)
			return slots_[selectedSlot_].units.size() > 1 ? 0 : slots_[selectedSlot_].units.front();
		return 0;
	}
	else if(slotIndex < slots_.size()) {
		validateSelectedObject();
		return slots_[slotIndex].units.size() > 1 ? 0 : slots_[slotIndex].units.front();
	}
	return 0;
}

void SelectManager::getSelectList(Slots& out)
{
	if(isSelectionEmpty()){
		out.clear();
		return;
	}

	MTAuto select_autolock(selectLock_);

	out = slots_;
}

bool SelectManager::canAttack(const WeaponTarget& target, bool checkDistance, bool check_fow) const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	UnitInterfaceList::const_iterator ui;
	FOR_EACH(selection_, ui){
		const UnitInterface* unit = *ui;
		if(checkDistance || unit->attr().isBuilding()){
			if(unit->fireDistanceCheck(target, check_fow))
				return true;
		}
		else if(unit->canAttackTarget(target, check_fow))
				return true;
	}
	
	return false;
}

int SelectManager::getTraficabilityCommonFlag()
{
	if(isSelectionEmpty())
		return 0;

	MTAuto select_autolock(selectLock_);

	int traficabilityCommonFlag;
	
	UnitInterfaceList::const_iterator ui;
	if(GlobalAttributes::instance().uniformCursor)
	{ // общий для всех тип проходимости
		traficabilityCommonFlag = 0;
		FOR_EACH(selection_, ui)
			traficabilityCommonFlag |= (*ui)->impassability();
	}
	else { // все доступные типы проходимости
		traficabilityCommonFlag = 0xFFFFFFFF;
		FOR_EACH(selection_, ui)
			traficabilityCommonFlag &= (*ui)->impassability();
	}

	return traficabilityCommonFlag;
}

bool SelectManager::canPickItem(const UnitItemInventory* item) const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	UnitInterfaceList::const_iterator ui;
	if(GlobalAttributes::instance().uniformCursor)
	{ // должны удовлетворять все
		FOR_EACH(selection_, ui)
			if(!(*ui)->canPutToInventory(item))
				return false;
		return true;
	
	}
	else { // достаточно что бы удовлетворял хоть один
		FOR_EACH(selection_, ui)
			if((*ui)->canPutToInventory(item))
					return true;
	}

	return false;
}

bool SelectManager::canExtractResource(const UnitItemResource* item) const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	UnitInterfaceList::const_iterator ui;
	if(GlobalAttributes::instance().uniformCursor){
		FOR_EACH(selection_, ui)
			if(!(*ui)->canExtractResource(item))
				return false;
		return true;
	}
	else {
		FOR_EACH(selection_, ui)
			if((*ui)->canExtractResource(item))
				return true;
	}

	return false;
}

bool SelectManager::canBuild(const UnitReal* building) const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	UnitInterfaceList::const_iterator ui;
	if(GlobalAttributes::instance().uniformCursor){
		FOR_EACH(selection_, ui)
			if(!(*ui)->canBuild(building))
				return false;
		return true;
	}
	else {
		FOR_EACH(selection_, ui)
			if((*ui)->canBuild(building))
				return true;
	}

	return false;
}

bool SelectManager::canRun() const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	UnitInterfaceList::const_iterator ui;
	FOR_EACH(selection_, ui)
		if((*ui)->canRun())
			return true;

	return false;
}

bool SelectManager::canPutInTransport(const UnitActing* transport) const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);
	
	UnitInterfaceList::const_iterator ui;
	if(GlobalAttributes::instance().uniformCursor){
		FOR_EACH(selection_, ui)
			if(!transport->canPutInTransport(*ui))
				return false;
		return true;
	}
	else {
		FOR_EACH(selection_, ui)
			if(transport->canPutInTransport(*ui))
				return true;
	}

	return false;
}

bool SelectManager::canTeleportate(const UnitBuilding* teleport) const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);
	
	UnitInterfaceList::const_iterator ui;
	if(GlobalAttributes::instance().uniformCursor){
		FOR_EACH(selection_, ui)
			if(!(*ui)->alive() || !teleport->attr().canTeleportate((*ui)->getUnitReal()))
				return false;
		return true;
	}
	else {
		FOR_EACH(selection_, ui)
			if((*ui)->alive() && teleport->attr().canTeleportate((*ui)->getUnitReal()))
				return true;
	}

	return false;
}

bool SelectManager::isSelected(const AttributeBase* attribute, bool singleOnly, bool needUniform)
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);
	
	if(selection_.empty() || singleOnly && selection_.size() > 1)
		return false;

	if(needUniform){
		if(!uniform())
			return false;
		return selectedAttribute() == attribute;
	}
	else {
		UnitInterfaceList::const_iterator i_unit;
		FOR_EACH(selection_, i_unit)
			if((*i_unit)->selectionAttribute() == attribute)
				return true;
	}
	return false;
}

bool SelectManager::isSquadSelected(const AttributeBase* attribute, bool singleOnly) const
{
	MTAuto select_autolock(selectLock_);
	
	UnitSquad* found = 0;
	UnitInterfaceList::const_iterator i_unit;
	FOR_EACH(selection_, i_unit){
		UnitSquad* squad = (*i_unit)->getSquadPoint();
		if(squad && (!attribute || &squad->attr() == attribute)){
			if(singleOnly && found && found != squad)
				return false;
			found = squad;
		}
		else 
			return false;
	}
	return found;
}

bool SelectManager::isInTransport(const AttributeBase* attribute, bool singleOnly) const
{
	if(isSelectionEmpty() || singleOnly && selectionSize() > 1)
		return false;

	MTAuto select_autolock(selectLock_);

	if(selection_.empty() || singleOnly && selection_.size() > 1)
		return false;

	UnitInterfaceList::const_iterator i_unit;
	FOR_EACH(selection_, i_unit){
		if((*i_unit)->attr().isSquad()){
			const UnitSquad* squad = (*i_unit)->getSquadPoint();
			if(squad->isInTransport(attribute, singleOnly))
				return true;
		}
		else {
			if((*i_unit)->attr().isTransport()){
				if(safe_cast<const UnitActing*>(*i_unit)->isInTransport(attribute))
					return true;
			}
		}
	}

	return false;
}

bool SelectManager::squadsMerge()
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	if(selection_.empty())
		return false;

	universe()->sendCommand(netCommand4G_UnitListCommand(selection_, UnitCommand(COMMAND_ID_ADD_SQUAD)));

	return true;
}

Accessibility SelectManager::canMergeSquads() const
{
	if(selectionSize_ < 2)
		return DISABLED;

	MTAuto select_autolock(selectLock_);

	if(selection_.size() < 2 || !uniformSquads_ || !selection_.front()->attr().isSquad())
		return DISABLED;

	const UnitSquad* squad = safe_cast<const UnitSquad*>(selection_.front());

	UnitInterfaceList::const_iterator it = selection_.begin() + 1;
	for(; it != selection_.end(); it++)
		if(squad->canAddSquad(safe_cast<const UnitSquad*>(*it)))
			return CAN_START;

	return ACCESSIBLE;
}

bool SelectManager::squadSplit()
{
	MTAuto select_autolock(selectLock_);

	if(selection_.empty()) return false;

	UnitInterfaceList::iterator isquad;
	FOR_EACH(selection_, isquad){
		if((*isquad)->attr().isSquad())
			break;
	}

	if(isquad == selection_.end())
		return false;

	UnitInterfaceList::iterator it = isquad;
	it++;

	for(; it != selection_.end(); it++){
		if((*it)->attr().isSquad())
			return false;
	}

	(*isquad)->sendCommand(UnitCommand(COMMAND_ID_SPLIT_SQUAD, 1));

	Transaction auto_transaction(this);

	clear();

	return true;
}

bool SelectManager::canSplitSquad() const
{
	if(isSelectionEmpty())
		return false;

	MTAuto select_autolock(selectLock_);

	if(selection_.empty()) return false;

	UnitInterfaceList::const_iterator isquad;
	FOR_EACH(selection_, isquad){
		if((*isquad)->attr().isSquad())
			break;
	}

	if(isquad == selection_.end() || safe_cast<const UnitSquad*>(*isquad)->unitsNumber() <= 1)
		return false;

	UnitInterfaceList::const_iterator it = isquad;
	it++;

	for(; it != selection_.end(); it++){
		if((*it)->attr().isSquad())
			return false;
	}

	return true;
}

void SelectManager::serialize(Archive& ar)
{
	MTAuto select_autolock(selectLock_);
	Transaction auto_transaction(this);
	
	UnitInterfaceLinks selectionTmp;
	if(ar.isOutput()){
		UnitInterfaceList::const_iterator it;
		FOR_EACH(selection_, it)
			if((*it)->alive())
				selectionTmp.push_back(*it);
	}
	
	ar.serialize(selectionTmp, "selection", 0);
	
	UnitInterfaceLink selected(selectedUnit_);
	ar.serialize(selected, "selectedUnit", 0);
	selectedUnit_ = selected;
	
	if(ar.isInput()){
		UnitInterfaceLinks::const_iterator it;
		clear();
		FOR_EACH(selectionTmp, it){
			if(UnitInterface* unit = *it){
				if(UnitSquad* squad = unit->getSquadPoint())
					squad->updateMainUnit();
				addToSelection(unit);
			}
		}
		
		selected_valid_ = false;
	}
	

	vector<UnitInterfaceLinks> savedSelections;

	if(ar.isOutput()){
		UnitInterfaceListsContainer::iterator sel;
		FOR_EACH(savedSelections_, sel){
			savedSelections.push_back(UnitInterfaceLinks());
			UnitInterfaceList::iterator uit;
			FOR_EACH(*sel, uit)
				if((*uit)->alive())
					savedSelections.back().push_back(*uit);
		}
		xassert(savedSelections.size() == SAVED_SELECTION_MAX);
	}

	ar.serialize(savedSelections, "savedSelections", 0);
	xassert(savedSelections_.size() == savedSelections.size());

	if(ar.isInput()){
		xassert(savedSelections.size() == SAVED_SELECTION_MAX);
		for(int idx = 0; idx < SAVED_SELECTION_MAX; ++idx){
			UnitInterfaceLinks::const_iterator it;
			FOR_EACH(savedSelections[idx], it)
				if(UnitInterface* unit = *it)
					savedSelections_[idx].push_back(unit);
		}
	}
}
