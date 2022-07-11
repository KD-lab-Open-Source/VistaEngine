#include "StdAfx.h"

#include "RenderObjects.h"
#include "CameraManager.h"
#include "Universe.h"

#include "Squad.h"

#include "Sound.h"
#include "Triggers.h"
#include "PositionGeneratorCircle.h"
#include "IronBuilding.h"
#include "Console.h"
#include "Serialization\Dictionary.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\UI_Minimap.h"
#include "Water\CircleManager.h"
#include "Serialization\SerializationFactory.h"
#include "Terra\vMap.h"
#include "AI\PFTrap.h"
#include "Physics\FormationController.h"
#include <map>


UNIT_LINK_GET(UnitSquad)

DECLARE_SEGMENT(UnitSquad)
REGISTER_CLASS(UnitBase, UnitSquad, "—квад")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_SQUAD, UnitSquad);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UnitSquad, WaitingMode, "–ежим ожидани€")
REGISTER_ENUM_ENCLOSED(UnitSquad, WAITING_DISABLE, "не ждать")
REGISTER_ENUM_ENCLOSED(UnitSquad, WAITING_FOR_MAIN_UNIT, "ждать главного")
REGISTER_ENUM_ENCLOSED(UnitSquad, WAITING_ALL, "все ждут")
END_ENUM_DESCRIPTOR_ENCLOSED(UnitSquad, WaitingMode);

///////////////////////////////////////////////

MTSection UnitSquad::unitsLock_;

///////////////////////////////////////////////
UnitSquad::UnitSquad(const UnitTemplate& data)
	: UnitInterface(data)
	, unitsReserved_(0)
	, formationController_(&attr().formations.front(), attr().followMainUnitInAutoMode)
{
	patrolIndex_ = -1;

	attackMode_ = player()->race()->attackModeAttr().attackMode();

	mainUnit_ = 0;
	needUpdateUnits_ = false;

	locked_ = false;

	clearUnitsGraphicsTimer_.start(30000 + logicRND(10000));
	
	waitingMode_ = WAITING_DISABLE;

	squadForUpgrade_ = 0;

	interpolatedPosePrev_ = interpolatedPose_ = pose_;
}

UnitSquad::~UnitSquad()
{
}

void UnitSquad::addRequestedUnit(const UnitActing* factory, UnitLegionary* unit)
{
	RequestedUnits::iterator ri;
	FOR_EACH(requestedUnits_, ri)
		if(ri->factory() == factory){
			xassert(ri->unit() == &unit->attr());
			requestedUnits_.erase(ri);
			break;
		}
	addUnit(unit);
}

void UnitSquad::cancelRequestedUnitProduction(const UnitActing* factory)
{
	RequestedUnits::iterator ri;
	FOR_EACH(requestedUnits_, ri)
		if(ri->factory() == factory){
			ri->request(0);
			break;
		}
}

void UnitSquad::addUnit(UnitLegionary* unit)
{
	xassert(checkFormationConditions(unit->attr().formationType));

	unit->setSquad(this);
	
	formationController_.addUnit(&unit->formationUnit_);

	{
	MTAuto lock(unitsLock_);
	units_.push_back(unit);
	needUpdateUnits_ = true;
	}

	if(unit->directControl())
		setForceMainUnit(units_.back());
	else if(!mainUnit_ || (!mainUnit_->isForceMainUnit() && 
		mainUnit_->selectionListPriority() < unit->selectionListPriority()))
		setMainUnit(units_.back());
	else if(waitingMode())
		unit->enableWaitingMode();
	else
		unit->disableWaitingMode();

	if(selected())
		setSelected(true);

	if(unit != mainUnit_){
		unit->setAttackMode(attackMode_);
		unit->updateTargetFromMainUnit(mainUnit_);
	}else
		setAttackMode(mainUnit_->attackMode());
}

void UnitSquad::removeUnit(UnitLegionary* unit)
{
	{
	MTAuto lock(unitsLock_);
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(*ui == unit){
			if(unit == mainUnit_)
				unit->stopEffect(&attr().mainUnitEffect);
			unit->setSquad(0);
			formationController_.removeUnit(&unit->formationUnit_);
			unit->setForceMainUnit(false);
			units_.erase(ui);
			break;
		}
	needUpdateUnits_ = true;
	
	if(Empty())
		Kill();
	}

	updateMainUnit();
}

void UnitSquad::setUsedByTrigger(int priority, const void* action, int time)
{
	__super::setUsedByTrigger(priority, action, time);

	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(*ui)
			(*ui)->UnitInterface::setUsedByTrigger(priority, action, time); 
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
	if(!dead())
		player()->removeUnitAccount(this);

	RequestedUnits::iterator i;
	FOR_EACH(requestedUnits_, i)
		i->stopProduction();
	requestedUnits_.clear();

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
			return; // был найден подход€щий юнит => искать еще одного нет смысла 

		if(!p->alive() || !p->attr().isLegionary() || squad_->player() != p->player())
			return;

		if(squad_->position2D().distance2(p->position2D()) > joinRadius2_)
			return;

		UnitLegionary* unit = safe_cast<UnitLegionary*>(p);


		if(unit->squad() == squad_)
			return;

		if(!attrUnits_.empty())
		{
			UnitSquad* squadIn = 0;
			AttributeUnitReferences::const_iterator i;
			FOR_EACH(attrUnits_, i)
				if(&unit->attr() == (*i))
				{
					// требуетс€ создать новый сквад
					if(unit->squad()->units().size() != 1) 
					{
						squadIn = safe_cast<UnitSquad*>(unit->player()->buildUnit(unit->attr().squad));
						squadIn->setPose(unit->pose(), true);
						unit->squad()->removeUnit(unit);
						squadIn->addUnit(unit);
					}
					else // не требуетс€ создавать сквад
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
	bool testOnly_; // добавл€ть или просто проверить на возможность добавлени€ в сквад
	bool canAdd_; // юнит может быть добавлен или уже добавлен в сквад
	AttributeUnitReferences attrUnits_; // список юнитов которые могут быть добавлены
};

bool UnitSquad::addUnitsFromArea(AttributeUnitReferences attrUnits, float radius, bool testOnly)
{
	AutomaticJoinOp op(this, attrUnits, radius, testOnly);
	universe()->unitGrid.Scan(position2D(), int(radius), op);
	return op.canAdd();
}

bool UnitSquad::offenceMode()
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui){
		UnitLegionary& unit = **ui;
		if(unit.autoAttackMode() == ATTACK_MODE_OFFENCE && unit.fireTargetExist())
			return true;
	}
	return false;
}

void UnitSquad::Quant()
{
	start_timer_auto();

	__super::Quant();

	destroyLinkSquad();

	if(clearUnitsGraphicsTimer_.finished()){
		clearUnitsGraphicsTimer_.start(30000 + logicRND(10000));
		streamLogicCommand.set(fCommandSquadClearUnitsGraphics) << this;
	}

	updatePose(false);
	
	if(!isSuspendCommandWork() && patrolMode() && !offenceMode())
		addWayPointS(patrolPoints_[patrolIndex_ = (patrolIndex_ + 1) % patrolPoints_.size()]);

	if(Empty())
		return;

	minimap().addSquad(this);
	
	//if(cameraManager->isVisible(position()))
		universe()->addVisibleUnit(this);

	float charge = 0;
	int count = 0;

	for(RequestedUnits::iterator ri = requestedUnits_.begin(); ri != requestedUnits_.end();){
		if(ri->requested()){
			++ri;
			continue;
		}
		if(!canQueryUnits(ri->unit())){
			ri = requestedUnits_.erase(ri);
			continue;
		}
		bool internalProduction = false;		
		LegionariesLinks::iterator ui;
		FOR_EACH(units_, ui) // —начала пытаемс€ произвести в юнитах сквада
			if((*ui)->attr().canProduce(ri->unit())){ 
				internalProduction = true;
				if(!(*ui)->isProducing() && (*ui)->startProduction(ri->unit(), this, 1)){
					ri->request(*ui);
					break;
				}
			}
		
		if(!internalProduction){
			UnitActing* factory = player()->findFreeFactory(ri->unit());
			if(factory && factory->startProduction(ri->unit(), this, 1)){
				ri->request(factory);
				break;
			}
		}
		++ri;
	}

	if(!UnitActing::freezedByTrigger() && attr().automaticJoin && !automaticJoinTimer_.busy()){
		automaticJoinTimer_.start(logicRND(3000));
		AttributeUnitReferences attrUnits;
		attrUnits.clear();
		universe()->unitGrid.Scan(position2D(), int(attr().automaticJoinRadius), AutomaticJoinOp(this, attrUnits, attr().automaticJoinRadius));
	}

	if(selected()){
		Vect2fVect::const_iterator pit;
		FOR_EACH(patrolPoints_, pit)
			UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_PATROL_POINT,
			Se3f(QuatF::ID, To3D(*pit)), &player()->race()->orderMark(UI_CLICK_MARK_PATROL), this));
	}

	if(!isUnderEditor()){
		if(!UnitActing::freezedByTrigger()){
			formationController_.stateQuant();
			formationController_.quant();
		}

		bool hasEndWayPoint = false;
		if(attr().showWayPoint){
			Vect2f wp;
			if(getManualMovingPosition(wp) && (attr().showTriggerWayPoint || !getUnitReal()->usedByTrigger())){
				hasEndWayPoint = true;
				Vect2f prePos = position2D();
				if(formationController_.wayPoints().size() > 1)
					prePos = *(formationController_.wayPoints().end() - 2);
				float angle = -atan2f(wp.x - prePos.x, wp.y - prePos.y);
				QuatF rot(angle, Vect3f::K, false);
				UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_MOVE_POINT,
					Se3f(rot, To3D(wp)),
					attr().targetPoint.isEmpty() ? &player()->race()->orderMark(UI_CLICK_MARK_WAIPOINT) : &attr().targetPoint,
					this));
			}
		}

		if(attr().showAllWayPoints && !formationController_.wayPoints().empty()){
			vector<Vect2f> path;
			path.push_back(position2D());
			path.push_back(position2D());
			path.insert(path.end(), formationController_.wayPoints().begin(), formationController_.wayPoints().end());
			path.push_back(path.back());
	
			float pathShift = 0.f;
			float prevPoint = 0.f;
			float nextPoint = 0.f;
			int cur = path.size() - 2;
			Vect2f curPos = path.back();
			Vect2f prePos = curPos;

			while(cur != 0){
				pathShift += float(max(attr().showAllWayPointDist, 5));

				while(pathShift >= nextPoint){
					if(--cur == 0)
						break;
					prevPoint = nextPoint;
					nextPoint += path[cur].distance(path[cur + 1]);
					if(nextPoint - prevPoint < FLT_EPS)
						nextPoint += 0.1f;
				}

				if(pathShift < nextPoint){
					float u = (pathShift - prevPoint) / (nextPoint - prevPoint);
					float t2 = u * u;
					float t3 = u * t2;
					curPos = path[cur+2];
					curPos *= -0.5f * u + t2 - 0.5f * t3;
					curPos.scaleAdd(path[cur+1], 1.0f - 2.5f * t2 + 1.5f * t3);
					curPos.scaleAdd(path[cur  ], 2.0f * t2 - 1.5f * t3 + 0.5f * u);
					curPos.scaleAdd(path[cur-1], 0.5f * (-t2 + t3));
				}
				else
					break;

				float angle = -atan2f(curPos.x - prePos.x, curPos.y - prePos.y);
				QuatF rot(angle, Vect3f::K, false);

				UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_MOVE_POINT,
					Se3f(rot, To3D(curPos)), &player()->race()->orderMark(UI_CLICK_MARK_WAY), this));

				if(!hasEndWayPoint){
					hasEndWayPoint = true;
					UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_MOVE_POINT,
						Se3f(rot, To3D(prePos)), &player()->race()->orderMark(UI_CLICK_MARK_WAY), this));
				}

				prePos = curPos;
				
			}
		}
	}
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
	
	if(needUpdate && alive()){
		updateMainUnit();	
		needUpdateUnits_ = true;
	}
}

void UnitSquad::stop()
{
	LegionariesLinks::iterator it;
	FOR_EACH(units_,it)
		(*it)->stop();
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
					squadIn->addUnit(unit);
					break;
				}
			}
			if(ui == unitsLink.end())
				break;
		}
	}
	stop();
	clearTargets();
	patrolPoints_.clear();
	patrolIndex_ = -1;	
	setSquadToFollow(0);
	FOR_EACH(units_, ui)
		(*ui)->clearOrders();
}

void fSe3fInterpolationSquad(XBuffer& stream)
{
	UnitSquad* unit;
	stream.read(unit);
	Se3f p[2];
	stream.read(p, sizeof(p));
	Se3f s;
	s.interpolate(p[0], p[1], StreamInterpolator::factor());
	unit->setInterpolatedPose(s);
}

void UnitSquad::updatePose(bool initPose)
{
	if(initPose && mainUnit_)
		formationController_.computePose();
	formationController_.computeRadius();
	setRadius(formationController_.radius());
	setPose(Se3f(formationController_.orientation(), To3D(formationController_.position())), initPose);
}

void UnitSquad::setPose(const Se3f& poseIn, bool initPose)
{
	Se3f posePrev = pose();

	__super::setPose(poseIn, initPose);

	if(initPose){
		formationController_.initPose(position2D(), angleZ());
		setInterpolatedPose(pose());
		interpolatedPosePrev_ = pose();
	}else{
		Se3f graphPose = pose();
		streamLogicInterpolator.set(fSe3fInterpolationSquad) << this << interpolatedPosePrev_ << graphPose;
		interpolatedPosePrev_ = graphPose;
	}
}

void UnitSquad::executeCommand(const UnitCommand& command)
{
	if(!command.isUnitValid())
		return;

	LegionariesLinks::iterator ui;
	
	switch(command.commandID()){
	
	case COMMAND_ID_DIRECT_CONTROL:
		getUnitReal()->setDirectControl(command.commandData() ? DIRECT_CONTROL_ENABLED : DIRECT_CONTROL_DISABLED);
		break;
	case COMMAND_ID_SYNDICAT_CONTROL:
		getUnitReal()->setDirectControl(command.commandData() ? SYNDICATE_CONTROL_ENABLED : DIRECT_CONTROL_DISABLED);
		break;
	case COMMAND_ID_DIRECT_KEYS:
		getUnitReal()->setDirectKeysCommand(command);
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
			(*ui)->toggleAutoFindTransport(command.commandData() != 0);
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
			if(unit->attr().isInventoryItem() || unit->attr().isResourceItem()){
				LegionariesLinks::iterator ui;
				FOR_EACH(units_, ui){
					if((*ui)->setItemToPick(unit, true))
						return;
					if((*ui)->attr().resourcer && (*ui)->setResourceItem(unit) || (*ui)->attr().canPick(unit)){
						addWayPointS(unit->position2D());
						return;
					}
				}
				getUnitReal()->setItemToPick(unit, false);
				return;
			}
			if(unit->attr().isTransport() && safe_cast<UnitActing*>(unit->getUnitReal())->putInTransport(this))
				return;
			if(unit->attr().isBuilding() && (player() == unit->player() || unit->player()->isWorld())
			&& safe_cast_ref<const AttributeBuilding&>(unit->attr()).teleport && !units_.empty()
			&& safe_cast_ref<const AttributeBuilding&>(unit->attr()).canTeleportate(getUnitReal())){
				UnitReal* teleportTo = safe_cast<UnitBuilding*>(unit)->findTeleport();
				if(teleportTo){
					clearOrders();
					FOR_EACH(units_, ui)
						(*ui)->setTeleport(unit->getUnitReal(), teleportTo);
				}
				return;
			}
			if(setConstructedBuilding(unit->getUnitReal(), command.shiftModifier()))
				return;
			if(unit->player() == player() && !canAttackTarget(WeaponTarget(unit, command.commandData()))){
				setSquadToFollow(unit->getSquadPoint());
			}else{
				selectWeapon(command.commandData());
				addTarget(unit);
				FOR_EACH(units_, ui)
					if(player() != unit->player() && !unit->player()->isWorld())
						player()->checkEvent(EventUnitPlayer(Event::COMMAND_ATTACK, *ui, player()));
			}
		}
		} break;
																			 
	case COMMAND_ID_FIRE_OBJECT:{
		selectWeapon(command.commandData());
		UnitInterface* unit = command.unit();
		if(unit){
			if(unit->player() != player() || canAttackTarget(WeaponTarget(unit, command.commandData()))){
				selectWeapon(command.commandData());
				addTarget(unit, false);
				FOR_EACH(units_, ui)
					if(player() != unit->player() && !unit->player()->isWorld())
						player()->checkEvent(EventUnitPlayer(Event::COMMAND_ATTACK, *ui, player()));
			}
		}
		} break;

	case COMMAND_ID_PATROL: {
		Vect2f clampPoint = command.position();
		clampPoint.x = clamp(clampPoint.x, 0.0f, vMap.H_SIZE - 1.0f);
		clampPoint.y = clamp(clampPoint.y, 0.0f, vMap.V_SIZE - 1.0f);
		if(patrolPoints_.empty() || (patrolIndex_ == 1 && patrolPoints_.empty())){
			clearOrders();					
			addWayPointS(clampPoint);
		}
		patrolPoints_.push_back(clampPoint);
		break;				
		}
	case COMMAND_ID_ATTACK:{
		clearOrders();
		selectWeapon(command.commandData());
		addTarget(command.position());
		FOR_EACH(units_, ui)
			player()->checkEvent(EventUnitPlayer(Event::COMMAND_ATTACK, *ui, player()));
		break;}

	case COMMAND_ID_FIRE:{
		selectWeapon(command.commandData());
		addTarget(command.position(), false);
		FOR_EACH(units_, ui)
			player()->checkEvent(EventUnitPlayer(Event::COMMAND_ATTACK, *ui, player()));
		break;}

	case COMMAND_ID_POINT:	
		clearOrders();
		addWayPointS(Vect2f(command.position()));
		FOR_EACH(units_, ui)
			player()->checkEvent(EventUnitPlayer(Event::COMMAND_MOVE, *ui, player()));
		break;

	case COMMAND_ID_STOP: 
		clearOrders();
		fireStop();
		addWayPointS(position2D());
		clearOrders();
		break; 
	
	case COMMAND_ID_WEAPON_DEACTIVATE:
		fireStop(command.commandData());
		break;

	case COMMAND_ID_PRODUCTION_INC:
		if(command.attribute() && canQueryUnits(command.attribute())) 
			requestedUnits_.push_back(command.attribute());
		break;
	case COMMAND_ID_PRODUCTION_DEC: {
		RequestedUnits::iterator i;
		FOR_EACH(requestedUnits_, i)
			if(i->unit() == command.attribute()){
				i->stopProduction();
				requestedUnits_.erase(i);
				break;
			}
		} break;
	
	case COMMAND_ID_SPLIT_SQUAD:
		split(command.commandData() != 0);
		break;

	case COMMAND_ID_FOLLOW_SQUAD:{
		UnitInterface* unit = command.unit();
		setSquadToFollow(unit ? unit->getSquadPoint() : 0);
		break;}

	case COMMAND_ID_SET_FORMATION:
		formationController_.changeFormation(&attr().formations[command.commandData()]);
		break;

	case COMMAND_ID_SET_MAIN_UNIT:
		FOR_EACH(units_, ui)
			if(&(*ui)->attr() == command.attribute()){
				setForceMainUnit(*ui);
				break;
			}
		break;

	case COMMAND_ID_SET_MAIN_UNIT_BY_INDEX:
		if((command.commandData() < units_.size()) && !units_[command.commandData()]->isForceMainUnit())
			setForceMainUnit(units_[command.commandData()]);
		break;
	
	case COMMAND_ID_MAKE_STATIC:
		setWaitingMode(WAITING_FOR_MAIN_UNIT);
		break;

	case COMMAND_ID_MAKE_DYNAMIC:
		setWaitingMode(WAITING_DISABLE);
		break;

	case COMMAND_ID_BUILD:
		clearOrders();
		setConstructedBuilding(safe_cast<UnitReal*>(command.unit()), command.shiftModifier());
		break;
	case COMMAND_ID_UPGRADE:
		squadForUpgrade_ = 0;
		FOR_EACH(units_, ui)
			(*ui)->executeCommand(command);
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

bool UnitSquad::getManualMovingPosition(Vect2f& point) const
{
	point = wayPoint();
	return getUnitReal()->unitState() == UnitReal::MOVE_MODE && !wayPointsEmpty();
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

bool UnitSquad::putToInventory(const UnitItemInventory* item)
{
	MTL();
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->putToInventory(item))
				return true;

	return false;
}

bool UnitSquad::putToInventory(const InventoryItem& item)
{
	MTL();
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		if(UnitLegionary* unit = *ui)
			if(unit->putToInventory(item))
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
	if(locked() || requestedUnits_.size() > 0)
		return DISABLED;
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
	__super::graphQuant(dt);

	if(selected()){
		if(attr().automaticJoin)
			universe()->circleManager()->addCircle(interpolatedPose().trans(), attr().automaticJoinRadius, attr().automaticJoinRadiusEffect);
		universe()->circleManager()->addCircle(interpolatedPose().trans(), attr().joinRadius, attr().joinRadiusEffect);

		showSelect(interpolatedPose().trans());
	}
}

const UI_ShowModeSprite* UnitSquad::getSelectSprite() const
{
	AttributeBase::UnitUI_StateType type = selected() ? AttributeBase::UI_STATE_TYPE_SELECTED : AttributeBase::UI_STATE_TYPE_NORMAL;
	if(const UI_ShowModeSprite* sprites = attr().getSelectSprite(type))
		return sprites;
	return getUnitReal()->attr().getSelectSprite(type);
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
	if(!dead())
		UnitBase::player()->removeUnitAccount(this);

	UnitBase::changeUnitOwner(player);

	RequestedUnits::iterator i;
	FOR_EACH(requestedUnits_, i)
		i->stopProduction();
	requestedUnits_.clear();
}

void UnitSquad::clearTargets()
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui) {
		(*ui)->setManualTarget(NULL);
	}
}

void UnitSquad::fireStop(int wid)
{
	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->fireStop(wid);
}

void UnitSquad::addTarget(UnitInterface* target, bool moveToTarget)
{
	xassert(target);
	if(Empty())
		return;

	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->setManualTarget(target, moveToTarget);
}

bool lineCircleIntersection(const Vect2f& p0, const Vect2f& p1, const Vect2f& pc, float radius, Vect2f& result)
{
	// »щет первое пересечение отрезка (0..1) с окружностью 
	Vect2f dp = p1 - p0;
	Vect2f dc = p0 - pc;
	float dp_2 = dp.norm2();
	float dc_2 = dc.norm2();
	float dp_dc = dot(dp, dc);
	float det2 = sqr(dp_dc) + dp_2*(sqr(radius) - dc_2);
	if(det2 < FLT_EPS)
		return false;
	float t = (-dp_dc - sqrtf(det2))/(dp_2 + 0.001f);
	if(t > 0 && t < 1){
		result = p0 + dp*t;
		xassert(fabsf(result.distance(pc) - radius) < 1.0f);
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

void UnitSquad::addTarget(const Vect3f& v, bool moveToTarget)
{
	if(Empty())
		return;

	LegionariesLinks::iterator ui;
	FOR_EACH(units_, ui){
		(*ui)->setTargetPoint(v, false, moveToTarget);
	}
}

void UnitSquad::showDebugInfo()
{
	__super::showDebugInfo();

	formationController_.showDebugInfo();

	if(showDebugUnitInterface.debugString)
		show_text(position(), debugString_.c_str(), Color4c::GREEN);

	if(showDebugSquad.position)
		show_vector(position(), 4, Color4c::RED);
	
	if(showDebugSquad.described_radius){
		show_vector(position(), radius(), Color4c::GREEN);
		Vect3f forwardDirection(Vect3f::J);
		forwardDirection.scale(radius());
		pose().xformPoint(forwardDirection);
		show_line(position(), forwardDirection, Color4c::GREEN);
	}

	if(showDebugSquad.unitsNumber){
		XBuffer buf;
		buf <= unitsNumber();
		show_text(position(), buf, Color4c::BLUE);
	}

	if(showDebugSquad.usedByTrigger && usedByTrigger())
		show_text(position(), usedTriggerName(), Color4c::YELLOW);

	//if(showDebugSquad.attackAction && attackAction())
	//	show_text(position(), "Attack", BLUE);
}

void UnitSquad::addSquad(UnitSquad* squad, float extraRadius2)
{
	if(isEq(extraRadius2, 0.f))
		extraRadius2 = sqr(attr().automaticJoinRadius);

	if(squad == this || !squad)
		return;

	float dist2 = position().distance2(squad->position());
	if(isEq(extraRadius2, sqr(attr().automaticJoinRadius))){
		if(dist2 > sqr(attr().joinRadius) || dist2 > sqr(squad->attr().joinRadius))
			return;
	}
	else
		if(dist2 > extraRadius2)
			return;

	if(squad->selected())
		universe()->changeSelection(squad, this);

	if(waitingMode() == WAITING_ALL || squad->waitingMode() == WAITING_FOR_MAIN_UNIT)
		setWaitingMode(squad->waitingMode());
	
	while(!squad->units_.empty()){
		UnitLegionary* unit = squad->units_.front();
		if(checkFormationConditions(unit->attr().formationType)){
			squad->removeUnit(unit);
			addUnit(unit);
			unit->updateWayPointsForNewUnit(getUnitReal());
		}
	}
}

bool UnitSquad::canAddWholeSquad(const UnitSquad* squad, float extraRadius2) const
{
	if(isEq(extraRadius2, 0.f))
		extraRadius2 = sqr(attr().automaticJoinRadius);

	if(squad == this)
		return false;

	if(squad->locked())
		return false;

	if(!attr().enableJoin || !squad->attr().enableJoin)
		return false;

	float dist2 = position().distance2(squad->position());
	float joinRadius1 = sqr(attr().joinRadius);
	float joinRadius2 = sqr(squad->attr().joinRadius);
	if (!isEq(extraRadius2, sqr(attr().automaticJoinRadius))){
		if(dist2 > extraRadius2)
			return false;
	}
	else
		if(dist2 > joinRadius1 || dist2 > joinRadius2)
			return false;

	typedef std::map<UnitFormationTypeReference, int> FormationMap;
	FormationMap fmap;
	FormationMap::iterator i;

	LegionariesLinks::const_iterator it;
	FOR_EACH(squad->units_, it){
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

	LegionariesLinks::const_iterator it;
	FOR_EACH(squad->units_, it){
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

		sq->addUnit(unit);

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
		if((*ui)->hasWeapon(weapon_id)){
			charge += (*ui)->weaponChargeLevel(weapon_id);
			count++;
		}
	}

	return count ? charge / float(count) : 0.f;
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

void UnitSquad::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(units_, "units_", 0);
	xassert(dead() || !units_.empty());
	needUpdateUnits_ = true;

	ar.serialize(patrolPoints_, "patrolPoints", 0);
	ar.serialize(patrolIndex_, "patrolIndex", 0);

	ar.serialize(requestedUnits_, "requestedUnits", 0);

	ar.serialize(attackMode_, "attackMode", 0);

	if(universe()->userSave()){
		ar.serialize(formationController_, "formationController", 0);

		ar.serialize(waitingMode_, "waitingMode", 0);

		ar.serialize(squadForUpgrade_, "squadForUpgrade", 0);
	}

	if(ar.isInput())
		interpolatedPosePrev_ = interpolatedPose_ = pose_;
}

void UnitSquad::RequestedUnit::stopProduction() 
{ 
	if(requested())
		factory_->finishProduction(); 
}

float UnitSquad::RequestedUnit::productionProgress() const 
{ 
	if(!requested())
		return 0.0f;
	xassert(factory_->producedUnit());
	return factory_->productionProgress(); 
}

void UnitSquad::RequestedUnit::serialize(Archive& ar)
{
	ar.serialize(unit_, "unit", 0);
	ar.serialize(factory_, "factory", 0);
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

bool UnitSquad::selectAble() const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if((*ui)->selectAble())
			return true;
	
	return false;
}

const UI_MinimapSymbol* UnitSquad::minimapSymbol() const
{
	switch(attr().minimapSymbolType_){
	case UI_MINIMAP_SYMBOLTYPE_SELF:
		return selected() ? &attr().minimapPermanentSymbol_ : &attr().minimapSymbol_;
	case UI_MINIMAP_SYMBOLTYPE_DEFAULT:
		return &player()->race()->minimapMark(UI_MINIMAP_SYMBOL_UNIT);
	}
	return 0;
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
		if((*ui)->unitState() == UnitReal::ATTACK_MODE && (*ui)->fireTargetExist())
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

int UnitSquad::unitsNumber(const AttributeBase* attribute) const
{
	int count = 0;

	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if (&(*ui)->attr() == attribute)
			++count;

	return count;
}

int UnitSquad::unitsNumberMax() const
{
	return round(parameters().findByType(ParameterType::NUMBER_OF_UNITS, 100));
}

bool UnitSquad::contain(const AttributeBase* attribute) const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		if (&(*ui)->attr() == attribute)
			return true;

	return false;
}

void UnitSquad::setMainUnit(LegionariesLinks::reference mainUnit)
{
	UnitLegionary* prevMainUnit = *units_.begin();
	if(prevMainUnit != mainUnit){
		if(DirectControlMode directControlMode = prevMainUnit->directControl()){
			prevMainUnit->setDirectControl(DIRECT_CONTROL_DISABLED);
			mainUnit->setDirectControl(directControlMode, false);
		}
		prevMainUnit->stopEffect(&attr().mainUnitEffect);
		if(waitingMode_ == WAITING_FOR_MAIN_UNIT)
			prevMainUnit->enableWaitingMode();
		MTAuto lock(unitsLock_);
		swap(*units_.begin(), mainUnit);
		needUpdateUnits_ = true;
		mainUnit_ = *units_.begin();
		if(player()->active() && waitingMode_ != WAITING_ALL)
			mainUnit_->startEffect(&attr().mainUnitEffect);
		if(waitingMode_ == WAITING_FOR_MAIN_UNIT)
			mainUnit_->disableWaitingMode();
	}else if(mainUnit_ != *units_.begin()){
		mainUnit_ = *units_.begin();
		if(player()->active() && waitingMode_ != WAITING_ALL)
			mainUnit_->startEffect(&attr().mainUnitEffect);
		if(waitingMode_ == WAITING_FOR_MAIN_UNIT)
			mainUnit_->disableWaitingMode();
	}
	formationController_.setMainUnit(&mainUnit_->formationUnit_);
}

void UnitSquad::setForceMainUnit(LegionariesLinks::reference mainUnit)
{
	LegionariesLinks::iterator i;
	FOR_EACH(units_, i)
		(*i)->setForceMainUnit(false);
	mainUnit->setForceMainUnit(true);
	setMainUnit(mainUnit);
}

void UnitSquad::findAndSetForceMainUnit(UnitLegionary* unit)
{
	LegionariesLinks::iterator i;
	FOR_EACH(units_, i)
		if(*i == unit){
			setForceMainUnit(*i);
			break;
		}
}

void UnitSquad::updateMainUnit()
{
	MTL();
	if(alive()){
		xassert(!units_.empty());
		LegionariesLinks::iterator mainUnit = units_.begin();
		LegionariesLinks::iterator i;
		FOR_EACH(units_, i){
			if((*i)->isForceMainUnit()){
				mainUnit = i;
				break;
			}
			if((*i)->selectionListPriority() > (*mainUnit)->selectionListPriority())
				mainUnit = i;
		}
		setMainUnit(*mainUnit);
	}
	xassert(mainUnit_);
}

void UnitSquad::setWaitingMode(WaitingMode mode)
{
	if(waitingMode() == mode)
		return;

	LegionariesLinks::iterator ui;
	switch(waitingMode_ = mode){
	case WAITING_DISABLE:
		FOR_EACH(units_, ui)
			(*ui)->disableWaitingMode();
		break;
	case WAITING_FOR_MAIN_UNIT:
		for(ui = units_.begin() + 1; ui < units_.end(); ++ui)
			(*ui)->enableWaitingMode();
		break;
	case WAITING_ALL:
		mainUnit_->stopEffect(&attr().mainUnitEffect);
		FOR_EACH(units_, ui)
			(*ui)->enableWaitingMode();
	}
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

void UnitSquad::setAttackMode(const AttackMode& attack_mode)
{ 
	attackMode_ = attack_mode; 
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->setAttackMode(attackMode_);
}

bool UnitSquad::canQueryUnits(const AttributeBase* attribute) const 
{ 
	if(unitsNumber() + unitsReserved_ >= unitsNumberMax() || !checkFormationConditions(attribute->formationType))
		return false;
	return player()->checkUnitNumber(attribute, &attr()); 
}

void UnitSquad::reserveUnitNumber(const AttributeBase* attribute, int counter)
{
	unitsReserved_ += counter;
}

void UnitSquad::relaxLoading()
{
	destroyLinkSquad();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units_, ui)
		formationController_.addUnit(&(*ui)->formationUnit_);
	if(alive())
		updateMainUnit();
	updatePose(true);
}

bool UnitSquad::checkFormationConditions(UnitFormationTypeReference unitType, int numberUnits) const
{
	if(numberUnits <= 0) 
		return false;

	if(unitsNumber() + numberUnits > unitsNumberMax())
		return false;

	if(!attr().allowedUnits.empty() && find(attr().allowedUnits.begin(), attr().allowedUnits.end(), unitType) == attr().allowedUnits.end())
		return false;

	int currentNumber = 0;
	LegionariesLinks::const_iterator si;
	FOR_EACH(units_, si)
		if((*si)->attr().formationType == unitType) 
			currentNumber++;

	return formationController_.checkNumber(unitType, currentNumber + numberUnits);
}
