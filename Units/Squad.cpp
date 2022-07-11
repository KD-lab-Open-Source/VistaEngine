#include "StdAfx.h"

#include "RenderObjects.h"
#include "CameraManager.h"
#include "Universe.h"

#include "Squad.h"

#include "Sound.h"
#include "Triggers.h"
#include "ExternalShow.h"
#include "PositionGeneratorCircle.h"
#include "IronBuilding.h"
#include "UnitTrail.h"
#include "..\Util\Console.h"
#include "Dictionary.h"

#include "..\UserInterface\UI_Logic.h"

float squad_described_radius_max = 1600;

float squad_position_generator_circle_tolerance = 1;

float squad_find_best_target_scan_radius = 1500;

float squad_contact_radius_factor = 1.2f;
float squad_contact_distance_tolerance = 1;
float squad_reposition_to_attack_radius_tolerance = 0.05f;

int patrol_mode_ignore_targets_time = 5000;

int squad_targets_scan_period = 500;
int squad_targets_clean_period = 2000;
int squad_technician_targets_scan_period = 200;

int static_gun_targets_scan_period = 1000;

int squad_reposition_to_attack_delay = 1000;
int squad_reposition_to_attack_delay_flying = 1000;
float squad_reposition_to_attack_velocity_factor = 1.f/50.f;

float squad_velocity_avr_tau = 0.3f;

UNIT_LINK_GET(UnitSquad)

REGISTER_CLASS(UnitBase, UnitSquad, "Сквад")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_SQUAD, UnitSquad);

MTSection UnitSquad::unitsLock_;

///////////////////////////////////////////////
UnitSquad::UnitSquad(const UnitTemplate& data)
: UnitInterface(data), 
unitNumberManager_(attr().numbers)
{
	average_position = homePosition_ = Vect2f::ZERO;
	
	setRadius(squad_described_radius_max);
	setStablePosition(Vect2f::ZERO);
	setStableOrientation(Mat2f::ID);
	stablePoseRestarted_ = true;

	radius_ = squad_described_radius_max;

	patrolIndex_ = -1;

	squadToFollow_ = 0;
	setCurrentFormation(0);
	lastWayPoint = Vect2f::ZERO;

	attackMode_ = player()->race()->attackModeAttr().attackMode();
	followTimer.start(1000 + logicRNDfabsRnd(2000));

	mainUnit_ = 0;
	needUpdateUnits_ = false;
	underDirectControl_ = false;

	locked_ = false;
	updateSquadWayPoints = false;

	clearUnitsGraphicsTimer_.start(30000 + logicRND(10000));
}

UnitSquad::~UnitSquad()
{
}

void UnitSquad::setPose(const Se3f& pose, bool initPose)
{
	UnitBase::setPose(pose, initPose);
	if(initPose)
		setStablePosition(position());
	average_position = position(); 
	setStableOrientation(Mat2f(angleZ()));
}

bool UnitSquad::addUnit(UnitLegionary* unit, bool set_position, bool set_wayPoint)
{
	xassert(unit->attr().isLegionary());

	UnitLegionary* firstUnit = 0;
	if(!units().empty())
		firstUnit = safe_cast<UnitLegionary*>(getUnitReal());

	setRadius(squad_described_radius_max);
	
	RequestedUnits::iterator ri;
	FOR_EACH(requestedUnits_, ri)
		if(ri->unit == &unit->attr() && ri->requested){
			requestedUnits_.erase(ri);
			break;
		}

	if(!checkFormationConditions(unit->attr().formationType)){
        kdWarning("&Shura", XBuffer(1024, 1) < TRANSLATE("Попытка добавить в сквад юнитов больше, чем назначено ") < attr().c_str());
		//return false;
	}

	// callc squad center;
	Vect3f center = Vect3f::ZERO;
	if(!units_.empty()){
		LegionariesLinks::iterator it;
		FOR_EACH(units_,it)
			center += (*it)->pose().trans();
		center /= units_.size();
	}
	else
		center = unit->position();

	unit->setSquad(this);
	
	{
	MTAuto lock(unitsLock_);
	units_.push_back(unit);
	needUpdateUnits_ = true;
	}
	updateMainUnit();

	unit->setInSquad();

	if(set_wayPoint) {
		if(!firstUnit || !unit->setState(firstUnit)){
			clearWayPoints();
			addWayPoint(lastWayPoint.eq(Vect3f::ZERO) ? pose().trans() : lastWayPoint);
		}
	}

	if(set_position)
		unit->setPose(Se3f(pose().rot(),center), true);

	if(unit->selected())
		universe()->select(this, true, true);
	else if(UnitInterface::selected())
		universe()->select(unit, true, true);

	unit->setAttackMode(attackMode_);

	return true;
}

void UnitSquad::removeUnit(UnitLegionary* unit)
{
	{
	MTAuto lock(unitsLock_);
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(*ui == unit){
			UnitLegionary& unit = **ui;
			unit.setSquad(0);
			units_.erase(ui);
			break;
		}
	needUpdateUnits_ = true;
	
	if(Empty())
		Kill();
	}

	updateMainUnit();
}

void UnitSquad::startUsedByTrigger(int time, UsedByTriggerReason reason)
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(*ui)
			(*ui)->startUsedByTrigger(time, reason);
}

void UnitSquad::setShowAISign(bool showAISign)
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(*ui)
			(*ui)->setShowAISign(showAISign);
}

void fCommandSquadClearUnitsGraphics(XBuffer& stream)
{
	UnitSquad* squad;
	stream.read(squad);
	squad->unitsGraphics_.clear();
	squad->needUpdateUnits_ = true;
}

void UnitSquad::Kill()
{
	UnitBase::Kill();

	streamLogicCommand.set(fCommandSquadClearUnitsGraphics) << this;

	LegionariesLinks units = units_;
	LegionariesLinks::iterator ui;
	FOR_EACH(units, ui)
		if(*ui)
			(*ui)->Kill();
}

class AutomaticJoinOp 
{
public:
	AutomaticJoinOp(UnitSquad* squad, AttributeUnitReferences attrUnits, float joinRadius, bool testOnly = false) 
		: squad_(squad), attrUnits_(attrUnits), joinRadius2_(sqr(joinRadius)), testOnly_(testOnly) 
		{
			canAdd_ = false;
		}

	void operator () (UnitBase* p) {

		if(testOnly_ && canAdd_) 
			return; // был найден подходящий юнит => искать еще одного нет смысла 

		if(!p->alive() || !p->attr().isLegionary() || squad_->player() != p->player())
			return;

		if(squad_->position2D().distance2(p->position2D()) > joinRadius2_)
			return;

		UnitLegionary* unit = safe_cast<UnitLegionary*>(p);


		if(unit->squad() == squad_ || &unit->squad()->attr() != &squad_->attr())
			return;

		if(squad_->units().size() < unit->squad()->units().size()) // ограничим перетягивание
			return;

		if(!attrUnits_.empty())
		{
			UnitSquad* squadIn = 0;
			AttributeUnitReferences::const_iterator i;
			FOR_EACH(attrUnits_, i)
				if(&unit->attr() == (*i))
				{
					// требуется создать новый сквад
					if(unit->squad()->units().size() != 1) 
					{
						squadIn = safe_cast<UnitSquad*>(unit->player()->buildUnit(unit->attr().squad));
						squadIn->setPose(unit->pose(), true);
						unit->squad()->removeUnit(unit);
						squadIn->addUnit(unit, false, false);
					}
					else // не требуется создавать сквад
						squadIn = unit->squad();

					break;
				}
			if(!squadIn)
				return;
		}

		if(!squad_->canAddWholeSquad(unit->squad(), joinRadius2_)) // влазит ли сквад целиком?
			return;

		if(testOnly_){
			canAdd_ = true;
			return;
		}

		canAdd_ = true;
		bool selected = squad_->selected() || unit->squad()->selected();
		
		squad_->addSquad(unit->squad(), joinRadius2_);
		
		if(selected)
			universe()->select(squad_, false);
	}

	bool canAdd() const { return canAdd_; }

private:
	UnitSquad* squad_;
	float joinRadius2_;
	bool testOnly_; // добавлять или просто проверить на возможность добавления в сквад
	bool canAdd_; // юнит может быть добавлен или уже добавлен в сквад
	AttributeUnitReferences attrUnits_; // список юнитов которые могут быть добавлены
};

bool UnitSquad::addUnitsFromArea(AttributeUnitReferences attrUnits, float radius, bool testOnly)
{
	AutomaticJoinOp op(this, attrUnits, radius, testOnly);
	universe()->unitGrid.Scan(position2D(), radius, op);
	return op.canAdd();
}

void UnitSquad::Quant()
{
	start_timer_auto();

	__super::Quant();

	destroyLinkSquad();

	if(clearUnitsGraphicsTimer_()){
		clearUnitsGraphicsTimer_.start(30000 + logicRND(10000));
		streamLogicCommand.set(fCommandSquadClearUnitsGraphics) << this;
	}

	calcCenter();

	if(Empty())
		return;
	
	if(cameraManager->isVisible(position()))
		universe()->addVisibleUnit(this);

	setPose(Se3f(orientation(), To3D(average_position)), false);

	followQuant();

	float charge = 0;
	int count = 0;

	bool internalProduction = false;
	RequestedUnits::iterator ri;
	FOR_EACH(requestedUnits_, ri){
		if(ri->requested || !player()->checkUnitNumber(ri->unit) || !checkFormationConditions(ri->unit->formationType)) 
			continue;
			
		LegionariesLinks::iterator ui;
		FOR_EACH(units_, ui) // Сначала пытаемся произвести в юнитах сквада
			if((*ui)->attr().canProduce(ri->unit)){ 
				internalProduction = true;
				if(!(*ui)->isProducing() && (*ui)->startProduction(ri->unit, this, 1)){
					ri->requested = true;
					break;
				}
			}
		
		if(!internalProduction){
			UnitActing* factory = player()->findFreeFactory(ri->unit);
			if(factory && factory->startProduction(ri->unit, this, 1)){
				ri->requested = true;
				break;
			}
		}
	}

	if(attr().automaticJoin && !automaticJoinTimer_()){
		automaticJoinTimer_.start(logicRND(3000));
		AttributeUnitReferences attrUnits;
		attrUnits.clear();
		universe()->unitGrid.Scan(position2D(), attr().automaticJoinRadius, AutomaticJoinOp(this, attrUnits, attr().automaticJoinRadius));
	}

	if(selected()){
		Vect2fVect::const_iterator pit;
		FOR_EACH(patrolPoints_, pit)
			UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_PATROL_POINT,
			Se3f(QuatF::ID, To3D(*pit)), &player()->race()->orderMark(UI_CLICK_MARK_PATROL), this));
	}

	followLeaderQuant();

//#ifndef _FINAL_VERSION_
//	LegionariesLinks::iterator ui;
//	FOR_EACH(units_, ui){
//		if(ui != units_.begin() && (*ui)->selectionListPriority() > mainUnit_->selectionListPriority())
//			xassert(0);
//	}
//#endif
}

void UnitSquad::destroyLinkSquad()
{
	bool needUpdate = false;
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(!*ui){
			MTAuto lock(unitsLock_);
			ui = remove(units_.begin(), units_.end(), LegionariesLink());
			units_.erase(ui, units_.end());
			needUpdate = true;
			break;
		}


	if(units_.empty())
		Kill();
	
	if(needUpdate){
		updateMainUnit();	
		needUpdateUnits_ = true;
	}
}

void UnitSquad::clearOrders()													   
{
	bool clearAll(true);
	bool allInOneTransport(true);
	UnitActing* transport = 0;
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->inTransport()){
			clearAll = false;
			transport = (*ui)->transport();
		}
		else
            allInOneTransport = false;
	
	if(allInOneTransport)
		FOR_EACH(units_, ui)
			if((*ui)->transport() != transport){
				allInOneTransport = false;
				break;
			}
	if(!clearAll && !allInOneTransport){
		UnitSquad* squadIn = safe_cast<UnitSquad*>(player()->buildUnit(&attr()));
		squadIn->setPose(pose(), true);
		for(;;){
			const LegionariesLinks& unitsLink = units();
			LegionariesLinks::const_iterator ui;
			FOR_EACH(unitsLink, ui){
				UnitLegionary* unit = *ui;
				if(unit->inTransport()){
					removeUnit(unit);
					squadIn->addUnit(unit, false, false);
					break;
				}
			}
			if(ui == unitsLink.end())
				break;
		}
	}
	clearWayPoints();													 	  
	clearTargets();
	patrolPoints_.clear();
	patrolIndex_ = -1;	
	setSquadToFollow(0);
	FOR_EACH(units_, ui)
		(*ui)->clearOrders();
}

void UnitSquad::setSquadToFollow(UnitBase* squadToFollow) 
{ 
	squadToFollow_ = squadToFollow; 
}

void UnitSquad::followQuant()
{
	if(!squadToFollow_ || Empty())
		return;

	if(!followTimer()) {
		followTimer.start(1000 + logicRND(2000));
		Vect2f v0 = position2D();
		Vect2f v1 = squadToFollow_->position2D();
		if(v0.distance2(v1) > sqr((radius() + squadToFollow_->radius()) *1.5f)){
			Vect2f position = v0 - v1;
			position.normalize((radius() + squadToFollow_->radius()) *1.5f);
			position += v1;
			clearWayPoints();
			addWayPoint(position);
		}
	}
}

void UnitSquad::executeCommand(const UnitCommand& command)
{
	if(!command.isUnitValid())
		return;

	LegionariesLinks::iterator ui;
	
	switch(command.commandID()){
	
	case COMMAND_ID_DIRECT_CONTROL:
		safe_cast<UnitActing*>(getUnitReal())->setDirectControl(command.commandData() ? DIRECT_CONTROL_ENABLED : DIRECT_CONTROL_DISABLED);
		underDirectControl_ = command.commandData(); 
		break;
	case COMMAND_ID_SYNDICAT_CONTROL:
		safe_cast<UnitActing*>(getUnitReal())->setDirectControl(command.commandData() ? SYNDICATE_CONTROL_ENABLED : DIRECT_CONTROL_DISABLED);
		underDirectControl_ = command.commandData(); 
		break;
	case COMMAND_ID_DIRECT_KEYS:
		safe_cast<UnitActing*>(getUnitReal())->setDirectKeysCommand(command);
		break;
	case COMMAND_ID_SELF_ATTACK_MODE:
		attackMode_.setAutoAttackMode((AutoAttackMode)command.commandData());
		FOR_EACH(units_, ui)
			(*ui)->setAutoAttackMode(attackMode_.autoAttackMode());
		break;
	case COMMAND_ID_WALK_ATTACK_MODE:
		attackMode_.setWalkAttackMode((WalkAttackMode)command.commandData());
		FOR_EACH(units_, ui)
			(*ui)->setWalkAttackMode(attackMode_.walkAttackMode());
		break;
	case COMMAND_ID_AUTO_TRANSPORT_FIND:
		FOR_EACH(units_, ui)
			(*ui)->toggleAutoFindTransport(command.commandData());
		break;
	case COMMAND_ID_WEAPON_MODE:
		attackMode_.setWeaponMode((WeaponMode)command.commandData());
		FOR_EACH(units_, ui)
			(*ui)->setWeaponMode(attackMode_.weaponMode());
		break;
	case COMMAND_ID_AUTO_TARGET_FILTER:
		attackMode_.setAutoTargetFilter((AutoTargetFilter)command.commandData());
		FOR_EACH(units_, ui)
			(*ui)->setAutoTargetFilter(attackMode_.autoTargetFilter());
		break;

	case COMMAND_ID_OBJECT:{
		selectWeapon(command.commandData());
		clearOrders();
		UnitInterface* unit = command.unit();

		if(unit){
			if(unit->attr().isTrail() && player() == unit->player()){
				LegionariesLinks::iterator ui;
				FOR_EACH(units_, ui)
					(*ui)->setTrail(safe_cast<UnitTrail*>(unit), command.position());
				return;
			}
			if(unit->attr().isInventoryItem() || unit->attr().isResourceItem()){
				LegionariesLinks::iterator ui;
				FOR_EACH(units_, ui){
					if(!(*ui)->setItemToPick(unit)){
						if((*ui)->attr().resourcer && (*ui)->setResourceItem(unit) || (*ui)->attr().canPick(unit)){
							(*ui)->addWayPoint(unit->position());
						}
						else{
							// !!! Юниты-не сборщики не должны ходить к предметам
							//(*ui)->selectWeapon(command.commandData());
							//(*ui)->setManualTarget(unit);
						} 
					}
				}
				return;
			}
			if(unit->attr().isTransport() && (player() == unit->player() || unit->player()->isWorld())
			  && safe_cast<UnitActing*>(unit->getUnitReal())->putInTransport(this))
					return;
			else if(unit->attr().isBuilding() && (player() == unit->player() || unit->player()->isWorld())
			  && safe_cast_ref<const AttributeBuilding&>(unit->attr()).teleport && !units_.empty()
			  && safe_cast_ref<const AttributeBuilding&>(unit->attr()).canTeleportate(getUnitReal())){
				UnitReal* teleportTo = safe_cast<UnitBuilding*>(unit)->findTeleport();
				if(teleportTo){
					clearOrders();
					FOR_EACH(units_, ui)
						(*ui)->setTeleport(safe_cast<UnitReal*>(unit), teleportTo);
				}
				return;
			}
			else if(setConstructedBuilding(safe_cast<UnitReal*>(command.unit()), command.shiftModifier()))
				return;
			else if(unit->player() == player() && !canAttackTarget(WeaponTarget(unit, command.commandData())))
				setSquadToFollow(unit);
			else {
				selectWeapon(command.commandData());
				addTarget(unit);
				FOR_EACH(units_, ui)
					if(player() != unit->player() && !unit->player()->isWorld())
						player()->checkEvent(EventUnitPlayer(Event::COMMAND_ATTACK, *ui, player()));
			}
		}
		} break;
																			 
	case COMMAND_ID_PATROL: {
		Vect2f clampPoint = command.position();
		clampPoint.x = clamp(clampPoint.x, 0, vMap.H_SIZE - 1);
		clampPoint.y = clamp(clampPoint.y, 0, vMap.V_SIZE - 1);
		if(patrolPoints_.empty() || (patrolIndex_ == 1 && patrolPoints_.empty())){
			clearOrders();					
			addWayPoint(clampPoint);
		}
		patrolPoints_.push_back(clampPoint);
		break;				
		}
	case COMMAND_ID_ATTACK:
		clearOrders();
		selectWeapon(command.commandData());
		addTarget(command.position());
		FOR_EACH(units_, ui)
			player()->checkEvent(EventUnitPlayer(Event::COMMAND_ATTACK, *ui, player()));
		break;																			  

	case COMMAND_ID_POINT:	
		clearOrders();					
		setLastWayPoint(Vect2f(command.position()));
		FOR_EACH(units_, ui) {
			(*ui)->addWayPoint(Vect2f(command.position()));
			player()->checkEvent(EventUnitPlayer(Event::COMMAND_MOVE, *ui, player()));
		}
		break;

	case COMMAND_ID_STOP: 
		clearOrders();
		fireStop();
		addWayPoint(position2D());
		clearOrders();
		break; 

	case COMMAND_ID_PRODUCTION_INC:
		if(command.attribute() && player()->checkUnitNumber(command.attribute()) && checkFormationConditions(command.attribute()->formationType)) 
			requestedUnits_.push_back(command.attribute());
		break;
	case COMMAND_ID_PRODUCTION_DEC: {
		RequestedUnits::iterator i;
		FOR_EACH(requestedUnits_, i)
			if(i->unit == command.attribute()){
				requestedUnits_.erase(i);
				break;
			}
		} break;
	case COMMAND_ID_PRODUCTION_PAUSE_ON: {
		RequestedUnits::iterator i;
		FOR_EACH(requestedUnits_, i)
			if(i->unit == command.attribute())
				i->paused = true;
		} break;
	case COMMAND_ID_PRODUCTION_PAUSE_OFF: {
		RequestedUnits::iterator i;
		FOR_EACH(requestedUnits_, i)
			if(i->unit == command.attribute())
				i->paused = false;
		} break;

	case COMMAND_ID_SPLIT_SQUAD:
		split(command.commandData() != 0);
		break;

	case COMMAND_ID_FOLLOW_SQUAD:
		setSquadToFollow(command.unit());
		break;

	case COMMAND_ID_SET_FORMATION:
		setCurrentFormation(command.commandData());
		break;

	case COMMAND_ID_BUILD:
		clearOrders();
		setConstructedBuilding(safe_cast<UnitReal*>(command.unit()), command.shiftModifier());
		break;

	default: {
		LegionariesLinks units = units_;
		LegionariesLinks::iterator ui;
		FOR_EACH(units, ui)
			(*ui)->executeCommand(command);
	  }
	}

	UnitInterface::executeCommand(command);
}

void UnitSquad::setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo)
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->setDamage(damage, agressor, contactInfo);
}

bool UnitSquad::locked() const
{
	return locked_;
}

void UnitSquad::setLocked(bool locked)
{
	locked_ = locked;
}

bool UnitSquad::canAttackTarget(const WeaponTarget& target, bool check_fow) const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->canAttackTarget(target, check_fow))
				return true;

	return false;
}

bool UnitSquad::canFire(int weaponID, RequestResourceType triggerAction) const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->canFire(weaponID, triggerAction))
				return true;

	return false;
}

bool UnitSquad::canDetonateMines() const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->canDetonateMines())
				return true;

	return false;
}

bool UnitSquad::fireDistanceCheck(const WeaponTarget& target, bool check_fow) const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->fireDistanceCheck(target, check_fow))
				return true;	
	
	return false;
}

bool UnitSquad::canPutToInventory(const UnitItemInventory* item) const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->canPutToInventory(item))
				return true;

	return false;
}

bool UnitSquad::canExtractResource(const UnitItemResource* item) const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->canExtractResource(item))
				return true;

	return false;
}

bool UnitSquad::canBuild(const UnitReal* building) const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->canBuild(building))
				return true;

	return false;
}

bool UnitSquad::canRun() const
{
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(UnitLegionary* unit = *ui)
			if(!unit->canRun())
				return false;

	return true;
}

Accessibility UnitSquad::canUpgrade(int upgradeNumber, RequestResourceType triggerAction) const
{
	Accessibility min = CAN_START;
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(Accessibility var = (*ui)->canUpgrade(upgradeNumber, triggerAction)){
			if(var < min)
				min = var;
		}
		else
			return DISABLED;
	return min;
}

Accessibility UnitSquad::canProduceParameter(int number) const
{
	Accessibility min = CAN_START;
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(Accessibility var = (*ui)->canProduceParameter(number)){
			if(var < min)
				min = var;
		}
		else
			return DISABLED;
	return min;
}

Accessibility UnitSquad::canProduction(int number) const
{
	Accessibility min = CAN_START;
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(Accessibility var = (*ui)->canProduction(number)){
			if(var < min)
				min = var;
		}
		else
			return DISABLED;
	return min;
}

//---------------------------
void UnitSquad::graphQuant(float dt)
{
	if(selected()){
		if(attr().automaticJoin)
			getUnitReal()->drawCircle(RADIUS_SQUAD_AUTOJOINE, position(), attr().automaticJoinRadius, attr().automaticJoinRadiusEffect);
		getUnitReal()->drawCircle(RADIUS_SQUAD_JOINE, position(), attr().joinRadius, attr().joinRadiusEffect);
	}
}

void UnitSquad::setSelected(bool selected)
{
	LegionariesLinks::iterator ui;

	__super::setSelected(selected);
	FOR_EACH(units_, ui)
		(*ui)->setSelected(selected);
}

bool UnitSquad::uniform(const UnitReal* unit) const
{
	if(!unit)
		unit = getUnitReal();
	
	const LegionariesLinks& unitsLink = units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(unitsLink, ui)
		if(!(*ui)->uniform(unit))
			return false;

	return true;
}

bool UnitSquad::prior(const UnitInterface* unit) const
{
	return UnitSquad::getUnitReal()->prior(unit->getUnitReal());
}

bool UnitSquad::isInTransport(const AttributeBase* attribute, bool singleOnly) const
{
	if(units_.empty() || singleOnly && units_.size() > 1)
		return false;

	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui){
		if((*ui)->attr().isTransport() && (*ui)->isInTransport(attribute))
			return true;
	}

	return false;
}

void UnitSquad::changeUnitOwner(Player* player)
{
	UnitBase::changeUnitOwner(player);

	requestedUnits_.clear();
}

void UnitSquad::calcCenter()
{
	LegionariesLinks::iterator ui;
	bool updateRadius = true;
//	FOR_EACH(units_, ui)
//		if(!(*ui)->wayPoints().empty())
//			updateRadius = false;

	
	// Calc new position
	Vect2f prev_average_position = average_position;
	average_position = Vect2f::ZERO;
	int counter = 0;
	bool allUnitsInPosition = true;
	bool attack_mode = false;
	FOR_EACH(units_, ui){
		UnitLegionary& unit = **ui;
		//xassert(unit.inSquad() || unit.attr().is_base_unit);
		//if(!unit.inSquad() || n_complex_units && unit.attr().is_base_unit) // не учитывать не дошедших и базовых, когда есть производные
		//	continue;
		average_position += unit.position2D();
		if(unit.autoAttackMode() == ATTACK_MODE_OFFENCE && unit.fireTargetExist())
			attack_mode = true;
		counter++;
	}

	if(!isSuspendCommandWork() && patrolMode() && counter && !attack_mode)
		addWayPoint(patrolPoints_[patrolIndex_ = (patrolIndex_ + 1) % patrolPoints_.size()]);

	stablePoseRestarted_ = false;
	average_position /= counter;

	// Расчитываем новый радиус для свободной формации.
//	if(attr().formations[currentFormation].formationPattern == FormationPatternReference()) {
		float described_radius = 0; //sqr(squad_described_radius_max);
		FOR_EACH(units_, ui){
			UnitLegionary& unit = **ui;
			if(!unit.inSquad())
				continue;
			described_radius = max(described_radius, unit.position2D().distance2(average_position) + sqr(unit.radius()));
		}
		described_radius = sqrtf(described_radius);
		
//		if(updateRadius)
			setRadius(min(radius(), described_radius));
//	} else {
//		setRadius(attr().formations[currentFormation].formationPattern->describedRadius());
//	}



//	average_velocity = average_velocity*(1.f - squad_velocity_avr_tau) + (average_position - prev_average_position)*squad_velocity_avr_tau;
}

void UnitSquad::clearWayPoints()
{
	wayPoints_.clear();

	LegionariesLinks::iterator it;
	FOR_EACH(units_,it)
		if((*it)->inSquad())
			(*it)->stop();
}

void UnitSquad::addWayPoint(const Vect2f& point, bool setLeaderPoint, bool enableMoveMode)
{
	setLastWayPoint(point);

	if(units_.size() == 1) {
		safe_cast<UnitLegionary*>(getUnitReal())->addWayPoint(point, enableMoveMode);
		return;
	}

	if(attr().formations[currentFormation].formationPattern == FormationPatternReference()) {
//    	float radiusMax = 0;
		LegionariesLinks::iterator it;

//		if(!GlobalAttributes::instance().enablePathTrackingRadiusCheck) {
//			FOR_EACH(units_,it)
//				if((*it)->radius() > radiusMax)
//					radiusMax = (*it)->rigidBody()->radius();
//		}

		PositionGeneratorCircle<LegionariesLinks> positionGenerator;
		positionGenerator.init(0, point,  &units_);

		Vect2f posPoint;
		it = units_.begin();

		if(!setLeaderPoint) {
			it++;
			positionGenerator.get(*it, false, true);
		}

		while(it != units_.end()){ 
			posPoint = positionGenerator.get(*it, false, true);
			(*it)->addWayPoint(posPoint, enableMoveMode);
//			(*it)->addWayPoint(point, enableMoveMode);
			++it;
		}
	} else {
		// callc squad center;
		Vect3f center = Vect3f::ZERO;
		LegionariesLinks::iterator it;
		FOR_EACH(units_,it)
			center += (*it)->pose().trans();
		center /= units_.size();

		positionFormation.initPositions();
		positionFormation.setPosition(Vect2f(center.x, center.y), point, attr().formations[currentFormation].rotateFront);

		bool passabilitySquadPose = true;
		FOR_EACH(units_,it) {
			Vect2f unitPoint = positionFormation.getUnitPositionGloabal((*it)->attr().formationType);
			if(!(*it)->rigidBody()->checkImpassabilityStatic(Vect3f(unitPoint.x, unitPoint.y, 0)))
				passabilitySquadPose = false;
		}

		if(passabilitySquadPose) {
			positionFormation.initPositions();
			positionFormation.setPosition(Vect2f(center.x, center.y), point, attr().formations[currentFormation].rotateFront);
			it = units_.begin();
			if(!setLeaderPoint)
				it++;
			while(it != units_.end()){ 
				(*it)->addWayPoint(positionFormation.getUnitPositionGloabal((*it)->attr().formationType), enableMoveMode);
				++it;
			}
		} else if(!attr().formations[currentFormation].hardFormation) {
			float radiusMax = 0;
			LegionariesLinks::iterator it;
			if(!GlobalAttributes::instance().enablePathTrackingRadiusCheck) {
				FOR_EACH(units_,it)
					if((*it)->radius() > radiusMax)
						radiusMax = (*it)->rigidBody()->radius();
			}

			PositionGeneratorCircle<LegionariesLinks> positionGenerator;
			positionGenerator.init(0, point,  &units_);

			Vect2f posPoint;
			it = units_.begin();

			if(!setLeaderPoint)
				it++;

			while(it != units_.end()){ 
				posPoint = positionGenerator.get(*it, true, true);
				(*it)->addWayPoint(posPoint, enableMoveMode);
				++it;
			}
		}
	}
}

void UnitSquad::clearTargets()
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui) {
		(*ui)->setManualTarget(NULL);
	}
}

void UnitSquad::fireStop()
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->fireStop();
}

void UnitSquad::addTarget(UnitInterface* target)
{
	xassert(target);
	if(Empty())
		return;

	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->setManualTarget(target);
}

bool lineCircleIntersection(const Vect2f& p0, const Vect2f& p1, const Vect2f& pc, float radius, Vect2f& result)
{
	// Ищет первое пересечение отрезка (0..1) с окружностью 
	Vect2f dp = p1 - p0;
	Vect2f dc = p0 - pc;
	float dp_2 = dp.norm2();
	float dc_2 = dc.norm2();
	float dp_dc = dot(dp, dc);
	float det2 = sqr(dp_dc) + dp_2*(sqr(radius) - dc_2);
	if(det2 < FLT_EPS)
		return false;
	float t = (-dp_dc - sqrt(det2))/(dp_2 + 0.001);
	if(t > 0 && t < 1){
		result = p0 + dp*t;
		xassert(fabs(result.distance(pc) - radius) < 1);
		return true;
	}
	return false;
}

void UnitSquad::selectWeapon(int weapon_id)
{
	if(Empty())
		return;

	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui){
		(*ui)->selectWeapon(weapon_id);
	}
}

void UnitSquad::addTarget(const Vect3f& v)
{
	if(Empty())
		return;

	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui){
		(*ui)->setTargetPoint(v);
	}
}

void UnitSquad::showDebugInfo()
{
	__super::showDebugInfo();

	if(showDebugUnitInterface.debugString)
		show_text(position(), debugString_.c_str(), GREEN);

	if(showDebugSquad.wayPoints){
		Vect3f posPrev = position();
		Vect2fVect::iterator i;
		FOR_EACH(wayPoints_, i){
			show_line(posPrev, To3D(*i), BLUE);
			posPrev = To3D(*i);
		}
	}

	if(showDebugSquad.position){
		show_vector(To3D(stablePose().trans), 5, GREEN);
		show_vector(position(), 4, RED);
	}

	if(showDebugSquad.described_radius)
		show_vector(position(), radius(), GREEN);

	if(showDebugSquad.unitsNumber){
		XBuffer buf;
		buf <= unitsNumber();
		show_text(position(), buf, BLUE);
	}

	//if(showDebugSquad.attackAction && attackAction())
	//	show_text(position(), "Attack", BLUE);

	if(showDebugSquad.squadToFollow && squadToFollow_)
		show_line(position(), squadToFollow_->position(), MAGENTA);
}

bool UnitSquad::addSquad(UnitSquad* squad, float extraRadius2)
{
	if(floatEqual(extraRadius2, 0.f))
		extraRadius2 = sqr(attr().automaticJoinRadius);

	if(squad == this || !squad)
		return false;

	float dist2 = position().distance2(squad->position());
	if(floatEqual(extraRadius2, sqr(attr().automaticJoinRadius))){
		if(dist2 > sqr(attr().joinRadius) || dist2 > sqr(squad->attr().joinRadius))
			return false;
	}
	else
		if(dist2 > extraRadius2)
			return false;

	universe()->changeUnitNotify(squad, this);

	bool ret = false;
	while(!squad->units_.empty()){
		UnitLegionary* unit = squad->units_.front();
		if(checkFormationConditions(unit->attr().formationType)){
			squad->removeUnit(unit);
			if(addUnit(unit, false, true))
				ret = true;
		}
		else
			break;
	}

	return ret;
}

bool UnitSquad::canAddWholeSquad(const UnitSquad* squad, float extraRadius2) const
{
	if(floatEqual(extraRadius2, 0.f))
		extraRadius2 = sqr(attr().automaticJoinRadius);

	if(squad == this)
		return false;

	if(squad->locked())
		return false;

	if(!attr().enableJoin || attr().automaticJoin || !squad->attr().enableJoin || squad->attr().automaticJoin)
		return false;

	float dist2 = position().distance2(squad->position());
	float joinRadius1 = sqr(attr().joinRadius);
	float joinRadius2 = sqr(squad->attr().joinRadius);
	if (!floatEqual(extraRadius2, sqr(attr().automaticJoinRadius))){
		if(dist2 > extraRadius2)
			return false;
	}
	else
		if(dist2 > joinRadius1 || dist2 > joinRadius2)
			return false;

	typedef std::map<UnitFormationTypeReference, int> FormationMap;
	FormationMap fmap;
	FormationMap::iterator i;

	for(LegionariesLinks::const_iterator it = squad->units_.begin(); it != squad->units_.end(); ++it){
		UnitFormationTypeReference type = (*it)->attr().formationType;
		i = fmap.find(type);
		if(i == fmap.end())
			fmap.insert(std::make_pair(type, 1));
		else
			i->second++;
	}
	
	FOR_EACH(fmap, i){
		if(!checkFormationConditions(i->first, i->second))
			return false;
	}
	return true;

}

bool UnitSquad::canAddSquad(const UnitSquad* squad) const
{
	if(squad == this)
		return false;

	if(!attr().enableJoin || attr().automaticJoin || !squad->attr().enableJoin || squad->attr().automaticJoin)
		return false;

	float dist2 = position().distance2(squad->position());
	if(dist2 > sqr(attr().joinRadius) || dist2 > sqr(squad->attr().joinRadius))
		return false;

	for(LegionariesLinks::const_iterator it = squad->units_.begin(); it != squad->units_.end(); ++it){
		UnitLegionary* unit = *it;
		if(checkFormationConditions(unit->attr().formationType))
			return true;
	}

	return false;
}

void UnitSquad::split(bool need_select)
{
	while(!units_.empty()){
		UnitLegionary* unit = units_.front();
		removeUnit(unit);

		UnitSquad* sq = safe_cast<UnitSquad*>(player()->buildUnit(unit->attr().squad));
		sq->setPose(unit->pose(), true);

		sq->setAttackMode(attackMode_);
		sq->addUnit(unit, false, false);

		if(need_select && !unit->selected())
			universe()->select(sq);
	}
}

float UnitSquad::weaponChargeLevel(int weapon_id) const
{
//	MTL();
	float charge = 0.f;
	int count = 0;
	LegionariesLinks::const_iterator ui;

	FOR_EACH(units_, ui){
		if((*ui)->inSquad()){
			if((*ui)->hasWeapon(weapon_id)){
				charge += (*ui)->weaponChargeLevel(weapon_id);
				count++;
			}
		}
	}

	return count ? charge / float(count) : 0.f;
}

float UnitSquad::formationRadius() const
{
	if(Empty())
		return attr().formationRadiusBase;
	return safe_cast<const UnitLegionary*>(getUnitReal())->formationRadius();
}

void UnitSquad::holdProduction()
{
	//for(int i = 0; i < 3; i++)
	//	atomsPaused_[i] = 1;
}

void UnitSquad::unholdProduction()
{
	//for(int i = 0; i < 3; i++)
	//	atomsPaused_[i] = 0;
}

float UnitSquad::productionProgress() const
{
	if(Empty())
		return 0;

	float progress_min = 0.f;

	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui){
		float tmp = (*ui)->productionProgress();
		if(tmp > FLT_EPS && progress_min < FLT_EPS || tmp < progress_min)
			progress_min = tmp;
	}

	return progress_min;
}

float UnitSquad::productionParameterProgress() const{
	if(Empty())
		return 0;

	float progress_min = 0.f;

	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui){
		float tmp = (*ui)->productionParameterProgress();
		if(tmp > FLT_EPS && progress_min < FLT_EPS || tmp < progress_min)
			progress_min = tmp;
	}

	return progress_min;
}

bool UnitSquad::checkFormationConditions(UnitFormationTypeReference unitType, int numberUnits) const
{
	if(numberUnits <= 0) 
		return false;

	int currentNumber = 0;
	LegionariesLinks::const_iterator si;
	FOR_EACH(units_, si)
		if((*si)->attr().formationType == unitType) 
			currentNumber++;

	if(attr().formations[currentFormation].formationPattern == FormationPatternReference()) {
		int number = unitNumberManager_.number(unitType);
		if(number == -1){
			if(Empty()){
				kdWarning("&Shura", XBuffer(1024, 1) < TRANSLATE("Не настроено количество юнитов в скваде ") < attr().c_str());
				return true;
			}
			return false;
		}

		if(currentNumber + numberUnits > number) 
			return false;
	}
	else {
		int formationNumber = 0;
		FormationPattern::Cells::const_iterator ci;
		FOR_EACH(attr().formations[currentFormation].formationPattern->cells(), ci)
			if(ci->type == unitType)
				formationNumber++;
		
		if(currentNumber + numberUnits > formationNumber)
			return false;
	}
	    
	return true;
}

void UnitSquad::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(stablePose_.trans, "stablePosition", 0);

//	float curvatureRadius = position_generator.curvatureRadius();
//	ar.serialize(curvatureRadius, "curvatureRadius", 0);
//	position_generator.setMode(PositionGenerator::Square, curvatureRadius);

	ar.serialize(units_, "units_", 0);
	xassert(!units_.empty());
	needUpdateUnits_ = true;

	ar.serialize(average_position, "averagePosition", 0);
	ar.serialize(wayPoints_, "wayPoints", 0);
	ar.serialize(currentFormation, "currentFormation", 0);
	ar.serialize(patrolPoints_, "patrolPoints", 0);
	ar.serialize(patrolIndex_, "patrolIndex", 0);

	ar.serialize(squadToFollow_, "squadToFollow", 0);
	
	ar.serialize(requestedUnits_, "requestedUnits", 0);

	ar.serialize(lastWayPoint, "lastWayPoint", 0);

	ar.serialize(attackMode_, "attackMode", 0);

	if(ar.isInput()){ // Нельзя работать со списком юнитов - ссылки еще не восстановлены!
		//LegionariesLinks::iterator ui;
		//FOR_EACH(units_, ui)
		//	(*ui)->setCollisionGroup((*ui)->collisionGroup() | COLLISION_GROUP_REAL);

		Vect2fVect wayPoints;
		swap(wayPoints, wayPoints_);
		Vect2fVect::const_iterator vi;
		FOR_EACH(wayPoints, vi)
			addWayPoint(*vi);
	}
}

void UnitSquad::RequestedUnit::serialize(Archive& ar)
{
	ar.serialize(unit, "unit", 0);
	ar.serialize(paused, "paused", 0);
	ar.serialize(requested, "requested", 0);
}

bool UnitSquad::setConstructedBuilding(UnitReal* building, bool queued)
{
	if(building->attr().isBuilding() && building->player() == player() && !building->isConstructed()){
		bool log = false;
		LegionariesLinks::iterator ui;
		FOR_EACH(units_, ui)
			if((*ui)->setConstructedBuilding(building, queued))
				log = true;
		return log;
	}
	return false;
}

void UnitSquad::applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics)
{
	unitNumberManager_.apply(arithmetics);
}

bool UnitSquad::selectAble() const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->selectAble())
			return true;
	
	return false;
}

bool UnitSquad::isSuspendCommand( CommandID commandID )
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->isSuspendCommand(commandID))
			return true;

	return false;
}

bool UnitSquad::canSuspendCommand(const UnitCommand& command) const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->canSuspendCommand(command))
			return true;

	return false;
}

bool UnitSquad::isSuspendCommandWork()
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->isSuspendCommandWork())
			return true;

	return false;
}

bool UnitSquad::fireTargetExist() const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->getUnitState() == UnitReal::ATTACK_MODE && (*ui)->fireTargetExist())
			return true;

	return false;
}

bool UnitSquad::isWorking() const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->isWorking())
			return true;

	return false;
}

float UnitSquad::getMinVelocity() const
{
	float ret = 1e4;
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui){
		float unitVelocity = (*ui)->forwardVelocity();
		if((unitVelocity < ret) && (*ui)->rigidBody()->unitMove())
			ret = unitVelocity;
	}

	return ret;
}

float UnitSquad::getAvrVelocity() const
{
	float ret = 0.0f;
	int velocityCount = 0;
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->rigidBody()->unitMove()) {
			ret += (*ui)->forwardVelocity();
			velocityCount++;
		}

	if(velocityCount)
		return ret / velocityCount;
	
	return 0.0f;
}

void UnitSquad::setCurrentFormation( int num )
{
	xassert(num < attr().formations.size());
	currentFormation = num;
	if(attr().formations[currentFormation].formationPattern != FormationPatternReference())
		positionFormation.setFormationPattern(attr().formations[currentFormation].formationPattern);
}

int UnitSquad::unitsNumber(const AttributeBase* attribute) const
{
	int count = 0;

	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if (&(*ui)->attr() == attribute)
			++count;

	return count;
}

bool UnitSquad::contain(const AttributeBase* attribute) const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if (&(*ui)->attr() == attribute)
			return true;

	return false;
}

void UnitSquad::updateMainUnit()
{
	MTL();
	if(alive()){
		xassert(!units_.empty());
		LegionariesLinks::iterator mainUnit = units_.begin();
		LegionariesLinks::iterator i;
		FOR_EACH(units_, i){
			if((*i)->selectionListPriority() > (*mainUnit)->selectionListPriority())
				mainUnit = i;
		}
		if(needUpdateUnits_){
			MTAuto lock(unitsLock_);
			swap(*units_.begin(), *mainUnit);
		}else
			swap(*units_.begin(), *mainUnit);
		mainUnit_ = *units_.begin();
	}
	xassert(mainUnit_);
}

const LegionariesLinks& UnitSquad::graphUnits() const
{ 
	MTG();
	if(needUpdateUnits_){
		MTAuto lock(unitsLock_);
		needUpdateUnits_ = false;
		unitsGraphics_ = units_;
	}
	return unitsGraphics_; 
}

void UnitSquad::followLeaderQuant()
{
//	if(attr().formations[currentFormation].formationPattern == FormationPatternReference())
//		return;

	if(units().empty() || units().size() == 1)
		return;

	// При атаке спец.оружием, автоматическом режиме или при прямом управлении сквад двигаеться за лидером.
	if(underDirectControl_ || (getUnitReal()->getUnitState() == UnitReal::ATTACK_MODE && !getUnitReal()->wayPoints().empty() && safe_cast<UnitActing*>(getUnitReal())->selectedWeaponID()) || getUnitReal()->getUnitState() == UnitReal::AUTO_MODE) {
		if(updateSquadWayPoints) {
			const LegionariesLinks& unitsLink = units();
			LegionariesLinks::const_iterator it;
			it = unitsLink.begin();
			it++;
			for(;it != unitsLink.end(); it++)
				if(!((*it)->selectedWeaponID() && ((*it)->unitState == UnitReal::ATTACK_MODE)))
					(*it)->stop();
		
			addWayPoint(getUnitReal()->position(), false, false);
			updateSquadWayPoints = false;
		}
	}

	if(getUnitReal()->isMoving())
		updateSquadWayPoints = true;
		
}

void UnitSquad::setAttackMode(const AttackMode& attack_mode)
{ 
	attackMode_ = attack_mode; 
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->setAttackMode(attackMode_);
}

void UnitSquad::relaxLoading()
{
	destroyLinkSquad();
	if(alive())
		updateMainUnit();
}
