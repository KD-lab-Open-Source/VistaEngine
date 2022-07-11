#include "StdAfx.h"
#include "StateBase.h"
#include "Squad.h"
#include "IronBuilding.h"
#include "UnitItemInventory.h"
#include "UnitItemResource.h"
#include "UnitPad.h"
#include "Player.h"
#include "CameraManager.h"

bool UnitReal::changeState(StateBase* state)
{
	return stateController_.changeState(state);
}

void UnitReal::switchToState(StateBase* state)
{
	stateController_.switchToState(state);
}

bool UnitReal::finishState(StateBase* state)
{
	return stateController_.finishState(state);
}

bool UnitReal::interuptState(StateBase* state)
{
	return stateController_.interuptState(state);
}

void UnitReal::startState(StateBase* state)
{
	stateController_.startState(state);
}

/////////////////////////////////////////////////////////////////////////////////////////////

StateController::StateController() :
	owner_(0),
	currentStateID_(CHAIN_NONE),
	currentState_(StateBase::instance()),
	prevState_(0),
	desiredState_(0),
	posibleStates_(0)
{
}

void StateController::initialize(UnitReal* owner, const States* posibleStates)
{
	owner_ = owner;
	posibleStates_ = posibleStates;
}

bool StateController::changeState(StateBase* state)
{
	if(!desiredState_ || desiredState_->priority() < (state->priority())){
		if(prevState_){
			prevState_->finish(owner_, false);
			prevState_ = 0;
		}
		desiredState_ = state;
		return true;
	}
	return false;
}

void StateController::switchToState(StateBase* state)
{
	if(changeState(state))
		prevState_ = currentState_;
	else
		currentState_->finish(owner_, false);
	clearCurrentState();
}

bool StateController::finishState(StateBase* state)
{
	if(state == currentState_){
		currentState_->finish(owner_);
		clearCurrentState();
		if(desiredState_){
			currentStateID_ = desiredState_->id();
			currentState_ = desiredState_;
			desiredState_ = 0;
			currentState_->start(owner_);
			prevState_ = 0;
		}
		return true;
	}
	return false;
}

bool StateController::interuptState(StateBase* state)
{
	if(state == currentState_){
		currentState_->finish(owner_, false);
		clearCurrentState();
		return true;
	}
	return false;
}

void StateController::startState(StateBase* state)
{
	if(currentState()){
		currentState_->finish(owner_, false);
	}else if(prevState_){
		prevState_->finish(owner_, false);
		prevState_ = 0;
	}
	currentStateID_ = state->id();
	currentState_ = state;
	currentState_->start(owner_);
}

void StateController::stateQuant()
{
	if(!posibleStates_)
		return;

	if(currentState_->canFinish(owner_))
		currentState_->finish(owner_);
	if(!owner_->alive())
		return;
	States::const_iterator istate(posibleStates_->begin());
	int statePriority(max(currentState() ? currentState_->priority() : 0, desiredState_ ? desiredState_->priority() : 0));
	while((*istate)->priority() >= statePriority){
		xxassert(istate < posibleStates_->end(), "Нет действия по умолчанию");
		if((*istate)->canStart(owner_)){
			if(prevState_){
				prevState_->finish(owner_, false);
				prevState_ = 0;
			}else if(currentState())
				currentState_->finish(owner_, false);
			desiredState_ = 0;
			currentStateID_ = (*istate)->id();
			currentState_ = *istate;
			currentState_->start(owner_);
			return;
		}
		++istate;
	}
	if(desiredState_){
		if(currentState())
			currentState_->finish(owner_, false);
		currentStateID_ = desiredState_->id();
		currentState_ = desiredState_;
		desiredState_ = 0;
		currentState_->start(owner_);
		prevState_ = 0;
	}
	xxassert(currentState_, "Нет действия по умолчанию");
}

void StateController::serialize(Archive& ar)
{
	ar.serialize(currentStateID_, "currentState", 0);
	ChainID prevStateID = prevState_ ? prevState_->id() : CHAIN_NONE;
	ar.serialize(prevStateID, "prevState", 0);
	ChainID desiredStateID = desiredState_ ? desiredState_->id() : CHAIN_NONE;
	ar.serialize(desiredStateID, "desiredState", 0);
	if(ar.isInput()){
		currentState_ = UnitAllPosibleStates::instance()->find(currentStateID_);
		if(prevStateID != CHAIN_NONE)
			prevState_ = UnitAllPosibleStates::instance()->find(prevStateID);
		if(desiredStateID != CHAIN_NONE)
			desiredState_ = UnitAllPosibleStates::instance()->find(desiredStateID);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateBase::finish(UnitReal* owner, bool finished) const
{
	owner->clearCurrentState();
	if(finished)
		owner->stopCurrentChain();
}

bool StateBase::firstStart(UnitReal* owner) const
{ 
	return owner->currentState() != id_; 
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFinishedByTimer::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->finishChainDelayTimer();
}

bool StateFinishedByTimer::canFinish(UnitReal* owner) const
{ 
	return owner->chainDelayTimerFinished(); 
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFall::start(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->rigidBody()->isUnitRagDoll())
		owner->stopChains();
	else
		owner->setChainByHealth(CHAIN_FALL);
	unit->rigidBody()->disableWaterAnalysis();
	unit->rigidBody()->setAnimatedRise(true);
	unit->makeStaticXY(UnitActing::STATIC_DUE_TO_RISE);
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlFreeCamera();
}

void StateFall::finish(UnitReal* owner, bool finished) const
{
	if(finished)
		owner->switchToState(StateRise::instance());
	else{
		__super::finish(owner, finished);
		UnitActing* unit(safe_cast<UnitActing*>(owner));
		unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_RISE);
		unit->rigidBody()->enableWaterAnalysis();
		unit->rigidBody()->setAnimatedRise(false);
		unit->enableFlying(0);
		if(unit->isActiveDirectControl())
			cameraManager->setDirectControlOffset(unit->directControlOffset(), 1000);
	}
}

bool StateFall::canStart(UnitReal* owner) const
{
	return 	firstStart(owner) && safe_cast<UnitActing*>(owner)->rigidBody()->isBoxMode();
}

bool StateFall::canFinish(UnitReal* owner) const
{
	return !safe_cast<UnitActing*>(owner)->rigidBody()->isBoxMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateRise::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_RISE, owner->attr().chainRiseTime);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), owner->attr().chainRiseTime);
}

void StateRise::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_RISE);
	if(unit->attr().isLegionary())
		safe_cast<UnitLegionary*>(unit)->formationUnit_.initPose();
	unit->rigidBody()->enableWaterAnalysis();
	unit->rigidBody()->setAnimatedRise(false);
	unit->enableFlying(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateWeaponGrip::start(UnitReal* owner) const
{
	owner->setChainByHealth(CHAIN_WEAPON_GRIP);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlFreeCamera();
}

void StateWeaponGrip::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->detachFromDock();
	if(finished)
		owner->switchToState(StateFall::instance());
	else{
		UnitActing* unit(safe_cast<UnitActing*>(owner));
		if(unit->isActiveDirectControl())
			cameraManager->setDirectControlOffset(unit->directControlOffset(), 1000);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateTransition::start(UnitReal* owner) const
{
	MovementState state = owner->getMovementState();
	MovementState prevState = state;
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->rigidBody()->onDeepWater()) {
		prevState.terrainType() &= ~ANIMATION_ON_WATER;
		prevState.terrainType() |= ANIMATION_ON_GROUND;
	}else{
		prevState.terrainType() &= ~ANIMATION_ON_GROUND;
		prevState.terrainType() &= ~ANIMATION_ON_LOW_WATER;
		prevState.terrainType() |= ANIMATION_ON_WATER;
		unit->makeStaticXY(UnitActing::STATIC_DUE_TO_TRANSITION);
	}
	unit->setMovementStatePrev(state);
	int time = owner->attr().chainTransitionTime;
	owner->setChainByHealthTransition(prevState, state, time);
	if(unit->rigidBody()->waterAnalysis())
		unit->rigidBody()->enableFlyDownModeByTime(time);
	unit->rigidBody()->resetDeepWaterChanged();
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), time);
}

void StateTransition::finish(UnitReal* owner, bool finished) const
{
    __super::finish(owner, finished);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->rigidBody()->disableFlyDownMode();
	unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_TRANSITION);
}

bool StateTransition::canStart(UnitReal* owner) const
{
	return safe_cast<UnitActing*>(owner)->rigidBody()->deepWaterChanged();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateMovements::start(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	unit->setMovementChain();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFlyUp::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_FLY_UP, owner->attr().chainFlyDownTime);
}

bool StateFlyUp::canStart(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return firstStart(owner) && unit->rigidBody()->flyDownMode() && unit->rigidBody()->flyingMode();
}

bool StateFlyUp::canFinish(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return (__super::canFinish(owner) && !unit->rigidBody()->flyDownMode()) || !unit->rigidBody()->flyingMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFlyDown::start(UnitReal* owner) const
{
	owner->setChainByHealth(CHAIN_FLY_DOWN);
}

bool StateFlyDown::canStart(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return firstStart(owner) && unit->rigidBody()->flyDownMode() && !unit->rigidBody()->flyingMode();
}

bool StateFlyDown::canFinish(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return !unit->rigidBody()->flyDownMode() || unit->rigidBody()->flyingMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateTouchDown::start(UnitReal* owner) const
{
	safe_cast<UnitLegionary*>(owner)->makeStaticXY(UnitActing::STATIC_DUE_TO_TOUCH_DOWN);
	owner->setChainByHealthWithTimer(CHAIN_TOUCH_DOWN, owner->attr().chainTouchDownTime);
}

void StateTouchDown::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	safe_cast<UnitLegionary*>(owner)->makeDynamicXY(UnitActing::STATIC_DUE_TO_TOUCH_DOWN);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateWork::start(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->nearConstructedBuilding()){
		unit->makeStaticXY(UnitActing::STATIC_DUE_TO_ANIMATION);
		owner->setChainByHealth(CHAIN_BUILD);
		return;
	}
	if(unit->resourcerMode() == UnitLegionary::RESOURCER_PICKING){
		unit->makeStaticXY(UnitActing::STATIC_DUE_TO_ANIMATION);
		owner->setChainByHealth(CHAIN_PICKING);
		if(unit->resourceItem())
			unit->resourceItem()->changeState(StateGiveResource::instance());
	}else{
		owner->setChain(CHAIN_WITH_RESOURCE, unit->resourcerCapacityIndex());
	}
}
void StateWork::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	safe_cast<UnitLegionary*>(owner)->makeDynamicXY(UnitActing::STATIC_DUE_TO_ANIMATION);
}
bool StateWork::canStart(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return unit->nearConstructedBuilding() || unit->resourcerMode() == UnitLegionary::RESOURCER_PICKING 
		|| unit->resourcerMode() == UnitLegionary::RESOURCER_RETURNING;
}
bool StateWork::canFinish(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return !unit->nearConstructedBuilding() && (unit->resourcerMode() == UnitLegionary::RESOURCER_IDLE 
		|| unit->resourcerMode() == UnitLegionary::RESOURCER_FINDING);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateGiveResource::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_GIVE_RESOURCE, owner->attr().chainGiveResourceTime);
	if(owner->attr().isLegionary())
		safe_cast<UnitActing*>(owner)->makeStaticXY(UnitActing::STATIC_DUE_TO_GIVE_RESOURCE);
}

void StateGiveResource::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	if(owner->attr().isLegionary()){
		UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
		unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_GIVE_RESOURCE);
		if(finished){
			unit->giveResource();
		}
	}
}

bool StateGiveResource::canStart(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return firstStart(owner) && unit->resourcerMode() == UnitLegionary::RESOURCER_RETURNING 
		&& unit->resourceCollector() && unit->nearUnit(unit->resourceCollector());
}

bool StateGiveResource::canFinish(UnitReal* owner) const 
{ 
	return __super::canFinish(owner) && (!owner->attr().isLegionary() || safe_cast<UnitLegionary*>(owner)->resourceCollector()); 
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StatePickItem::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_PICK_ITEM, owner->attr().chainPickItemTime);
	safe_cast<UnitActing*>(owner)->makeStaticXY(UnitActing::STATIC_DUE_TO_PICKING_ITEM);
}

void StatePickItem::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	safe_cast<UnitLegionary*>(owner)->makeDynamicXY(UnitActing::STATIC_DUE_TO_PICKING_ITEM);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool StateAttack::canStart(UnitReal* owner) const 
{ 
	return firstStart(owner) && safe_cast<UnitActing*>(owner)->weaponChainQuant(owner->getMovementState()); 
}

bool StateAttack::canFinish(UnitReal* owner) const 
{ 
	return !safe_cast<UnitActing*>(owner)->weaponChainQuant(owner->getMovementState()); 
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFrozen::start(UnitReal* owner) const
{
	owner->setAdditionalChain(CHAIN_FROZEN);	
}

void StateFrozen::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->stopAdditionalChain(CHAIN_FROZEN);
}

bool StateFrozen::canStart(UnitReal* owner) const
{
	return firstStart(owner) && owner->isFrozenAttack();
}

bool StateFrozen::canFinish(UnitReal* owner) const
{
	StateMovements::instance()->start(owner);
	return !owner->isFrozenAttack();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateTeleporting::start(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
    owner->startChainTimer(safe_cast_ref<const AttributeBuilding&>(unit->teleport()->attr()).teleportationTime*1000);
	owner->hide(UnitReal::HIDE_BY_TELEPORT, true);
	unit->makeStatic(UnitActing::STATIC_DUE_TO_TELEPORTATION);
	unit->clearTeleport();
	unit->clearOrders();
	Vect2f delta(logicRNDfrnd(1.f), logicRNDfrnd(1.f));
	delta *= (owner->radius() + unit->teleportTo()->radius())/(delta.norm() + 0.01f);
	Se3f pose(unit->teleportTo()->pose());
	pose.trans().x += delta.x;
	pose.trans().y += delta.y;
	owner->setPose(pose, true);
}

void StateTeleporting::finish(UnitReal* owner, bool finished) const 
{
	__super::finish(owner, finished);
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->teleportTo()){
		owner->hide(UnitReal::HIDE_BY_TELEPORT, false);
		unit->makeDynamic(UnitActing::STATIC_DUE_TO_TELEPORTATION);
		unit->clearTeleportTo();
	}else
		owner->Kill();
}

bool StateTeleporting::canStart(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return unit->teleport() && unit->nearUnit(unit->teleport());
}

bool StateTeleporting::canFinish(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return __super::canFinish(owner) || !unit->teleportTo();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateBeBuild::start(UnitReal* owner) const
{
	UnitBuilding* unit(safe_cast<UnitBuilding*>(owner));
	unit->setChainByFactor(CHAIN_BE_BUILT, unit->constructionProgress());
}

void StateBeBuild::finish(UnitReal* owner, bool finished) const 
{
	__super::finish(owner, finished);
	if(owner->isConnected())
		owner->setAdditionalChain(CHAIN_CONNECT);
}

bool StateBeBuild::canStart(UnitReal* owner) const
{
	return !safe_cast<UnitBuilding*>(owner)->isConstructed();
}

bool StateBeBuild::canFinish(UnitReal* owner) const
{
	UnitBuilding* unit(safe_cast<UnitBuilding*>(owner));
	if(!unit->isConnected())
		unit->disableAnimation(UnitReal::ANIMATION_DISCONNECTED);
	else
		unit->enableAnimation(UnitReal::ANIMATION_DISCONNECTED);
	return unit->isConstructed();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateDisconnect::start(UnitReal* owner) const
{
	owner->setAdditionalChainWithTimer(CHAIN_DISCONNECT, owner->attr().chainDisconnectTime);
}

void StateDisconnect::finish(UnitReal* owner, bool finished) const 
{
	bool timerFinished = __super::canFinish(owner);
	__super::finish(owner, false);
	if(owner->attr().killAfterDisconnect && timerFinished)
		owner->Kill();
	else{
		owner->stopAdditionalChain(CHAIN_DISCONNECT);
		if(finished)
			owner->setAdditionalChain(CHAIN_CONNECT);
	}
}

bool StateDisconnect::canStart(UnitReal* owner) const
{
	return firstStart(owner) && !owner->isConnected();
}

bool StateDisconnect::canFinish(UnitReal* owner) const
{
	if(owner->attr().killAfterDisconnect && __super::canFinish(owner))
		return true;
	StateBuildingStand::instance()->canStart(owner);
	return safe_cast<UnitBuilding*>(owner)->isConnected();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool StateBuildingStand::canStart(UnitReal* owner) const
{
    owner->setChainByHealth(CHAIN_MOVEMENTS);
	return true;
}

void StateBuildingStand::start(UnitReal* owner) const
{
	owner->setAdditionalChain(CHAIN_CONNECT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool StateItemStand::canStart(UnitReal* owner) const
{
	float parametersSum = 0;
	if(owner->attr().isInventoryItem())
		parametersSum = safe_cast<UnitItemInventory*>(owner)->parametersSum();
	else
		parametersSum = safe_cast<UnitItemResource*>(owner)->parametersSum();
	owner->setChainByFactor(CHAIN_STAND, parametersSum > FLT_EPS ? 1.f - owner->parameters().sum() / parametersSum : 0);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateProduction::start(UnitReal* owner) const
{
	owner->setAdditionalChainByHealth(CHAIN_PRODUCTION);
}

bool StateProduction::canStart(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return firstStart(owner) && ((unit->producedUnit() && !unit->shippedUnit()) || (unit->producedParameter() && unit->productionParameterStarted()));
}

bool StateProduction::canFinish(UnitReal* owner) const
{
	StateBuildingStand::instance()->canStart(owner);
	owner->setAdditionalChainByHealth(CHAIN_PRODUCTION);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return (!unit->producedUnit() || unit->shippedUnit()) && (!unit->producedParameter() || !unit->productionParameterStarted());
}

void StateProduction::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	owner->stopAdditionalChain(CHAIN_PRODUCTION);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateMoveToCargo::start(UnitReal* owner) const
{
	canFinish(owner);
}

void StateMoveToCargo::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->cargo() && finished){
		if(unit->rigidBody()->flyingMode())
			owner->switchToState(StateLandToLoad::instance());
		else
			owner->switchToState(StateOpenForLanding::instance());
	}
		
}

bool StateMoveToCargo::canFinish(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(!unit->cargo()){
		float dist, distMin = FLT_INF;
		LegionariesLinks::iterator i;
		FOR_EACH(unit->transportSlots(), i)
			if(*i && !(*i)->isDocked() && distMin > (dist = (*i)->position2D().distance2(owner->position2D()))){
				distMin = dist;
				unit->setCargo(*i);
			}
		if(!unit->cargo())
			return true;
	}
	if(!unit->rigidBody()->prm().flyingMode || unit->nearUnit(unit->cargo(), owner->attr().transportLoadRadius))
		return true;
	StateMovements::instance()->start(owner);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateLandToLoad::start(UnitReal* owner) const
{
	safe_cast<UnitActing*>(owner)->disableFlying(UnitActing::FLYING_DUE_TO_TRANSPORT_LOAD);
	StateFlyDown::instance()->start(owner);
}

void StateLandToLoad::finish(UnitReal* owner, bool finished) const
{
	if(finished)
		owner->switchToState(StateOpenForLanding::instance());
	else{
		__super::finish(owner, finished);
		safe_cast<UnitActing*>(owner)->enableFlying(UnitActing::FLYING_DUE_TO_TRANSPORT_LOAD);
	}
}

bool StateLandToLoad::canFinish(UnitReal* owner) const
{
	return !safe_cast<UnitActing*>(owner)->rigidBody()->flyDownMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateOpenForLanding::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_OPEN_FOR_LANDING, owner->attr().chainOpenForLandingTime);
	safe_cast<UnitActing*>(owner)->makeStaticXY(UnitActing::STATIC_DUE_TO_OPEN_FOR_LOADING);
}

void StateOpenForLanding::finish(UnitReal* owner, bool finished) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->setReadyForLanding(false);
	if(finished){
		owner->finishChainDelayTimer();
		owner->switchToState(StateCloseForLanding::instance());
	}else{
		__super::finish(owner, finished);
		for(int i = 0; i < unit->transportSlots().size(); i++)
			if(UnitLegionary* transportSlot = unit->transportSlots()[i])
				if(!transportSlot->isDocked())
					transportSlot->clearTransport();
		unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_OPEN_FOR_LOADING);
		unit->enableFlying(UnitActing::FLYING_DUE_TO_TRANSPORT_LOAD);
	}
}

bool StateOpenForLanding::canFinish(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(__super::canFinish(owner)){
		unit->setReadyForLanding(true);
		owner->finishChainDelayTimer();
	}
	if(unit->isReadyForLanding()){
		if(!unit->cargo()){
			if(StateMoveToCargo::instance()->canFinish(owner)){
				if(!unit->cargo())
					return true;
			}else{
				if(unit->rigidBody()->prm().flyingMode)
					return true;
			}
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateCloseForLanding::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_CLOSE_FOR_LANDING, owner->attr().chainCloseForLandingTime);
}

void StateCloseForLanding::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_OPEN_FOR_LOADING);
	unit->enableFlying(UnitActing::FLYING_DUE_TO_TRANSPORT_LOAD);
	if(finished){
		if(unit->cargo())
			owner->switchToState(StateMoveToCargo::instance());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateLanding::start(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->makeStatic(UnitActing::STATIC_DUE_TO_SITTING_IN_TRANSPORT);
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlFreeCamera();
}

void StateLanding::finish(UnitReal* owner, bool finished) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->transport()){
		unit->transport()->makeDynamicXY(UnitActing::STATIC_DUE_TO_TRANSPORT_LOADING);
		unit->transport()->setReadyForLanding(true);
		if(finished){
			owner->finishChainDelayTimer();
			owner->switchToState(StateInTransport::instance());
			return;
		}
	}
	owner->detachFromDock();
	unit->putUnitOutTransport();
	unit->makeDynamic(UnitActing::STATIC_DUE_TO_SITTING_IN_TRANSPORT);
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), 1000);
	__super::finish(owner, finished);
}

bool StateLanding::canStart(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return firstStart(owner) && unit->transport() && unit->moveToTransport();
}

bool StateLanding::canFinish(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(!__super::canFinish(owner) && unit->transport()){
		if(unit->transport()->isReadyForLanding()){
			UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
			const TransportSlot& slot = unit->transport()->attr().transportSlots[unit->transportSlotIndex()];
			if(slot.dockWhenLanding)
				unit->attachToDock(unit->transport(), slot.node, false);
			owner->setChainByHealthWithTimer(CHAIN_LANDING, owner->attr().chainLandingTime);
			unit->transport()->makeStaticXY(UnitActing::STATIC_DUE_TO_TRANSPORT_LOADING);
			unit->transport()->setReadyForLanding(false);
		}else{
			if(!owner->chainDelayTimerWasStarted())
				StateMovements::instance()->start(owner);
		}
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateUnlanding::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_UNLANDING, owner->attr().chainUnlandingTime);
}

void StateUnlanding::finish(UnitReal* owner, bool finished) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	unit->makeDynamic(UnitActing::STATIC_DUE_TO_SITTING_IN_TRANSPORT);
	__super::finish(owner, finished);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateInTransport::start(UnitReal* owner) const
{
	owner->setChainByHealth(CHAIN_IN_TRANSPORT);
	safe_cast<UnitLegionary*>(owner)->putInTransport();
}

void StateInTransport::finish(UnitReal* owner, bool finished) const
{
	owner->detachFromDock();
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	unit->putUnitOutTransport();
	if(finished)
		owner->switchToState(StateUnlanding::instance());
	else
		unit->makeDynamic(UnitActing::STATIC_DUE_TO_SITTING_IN_TRANSPORT);
	__super::finish(owner, finished);
}

bool StateInTransport::canFinish(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->canFireInTransport())
		StateAttack::instance()->canFinish(owner);
	return !unit->transport();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateOpen::start(UnitReal* owner) const
{
	owner->setAdditionalChainWithTimer(CHAIN_OPEN, owner->attr().chainOpenTime);
}

void StateOpen::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	owner->stopAdditionalChain(CHAIN_OPEN);
	if(finished)
		owner->switchToState(StateClose::instance());
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->shippedUnit())
		unit->shippedUnit()->finishState(StateBirth::instance());
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateClose::start(UnitReal* owner) const
{
	owner->setAdditionalChainWithTimer(CHAIN_CLOSE, owner->attr().chainCloseTime);
}

void StateClose::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	owner->stopAdditionalChain(CHAIN_CLOSE);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(finished && (unit->productionCounter() > 0))
		unit->shipProduction();
	else
		unit->finishProduction();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateUpgrade::start(UnitReal* owner) const
{
	if(owner->attr().upgrades[owner->executedUpgradeIndex()].chainUpgradeNumber >= 0)
		owner->setChainWithTimer(CHAIN_UPGRADE, owner->attr().upgrades[owner->executedUpgradeIndex()].chainUpgradeTime, 
			owner->attr().upgrades[owner->executedUpgradeIndex()].chainUpgradeNumber);
	else
		owner->setChainByHealthWithTimer(CHAIN_UPGRADE, owner->attr().upgrades[owner->executedUpgradeIndex()].chainUpgradeTime);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlFreeCamera();
}

void StateUpgrade::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(finished)
		unit->finishUpgrade();
	else
		unit->cancelUpgrade();
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), 1000);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateIsUpgraded::start(UnitReal* owner) const
{
	safe_cast<UnitActing*>(owner)->disableFlying(UnitActing::FLYING_DUE_TO_UPGRADE);
}

void StateIsUpgraded::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->makeDynamicXY(UnitActing::STATIC_DUE_TO_UPGRADE);
	unit->enableFlying(UnitActing::FLYING_DUE_TO_UPGRADE);
	unit->setUpgradeProgresParameters();
	if(owner->attr().isBuilding() && owner->isConnected())
		owner->setAdditionalChain(CHAIN_CONNECT);
	if(finished)
		unit->player()->checkEvent(EventUnitUnitAttributePlayer(Event::COMPLETE_UPGRADE, &unit->beforeUpgrade(), unit, unit->player()));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateBirth::start(UnitReal* owner) const
{
	owner->setChainByHealth(CHAIN_BIRTH);
}

void StateBirth::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->detachFromDock();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateItemBirth::start(UnitReal* owner) const
{
	owner->setChainWithTimer(CHAIN_ITEM_BIRTH, owner->attr().chainBirthTime);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateBirthInAir::start(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->makeStaticXY(UnitActing::STATIC_DUE_TO_ANIMATION);
	unit->makeDynamic(UnitActing::STATIC_DUE_TO_BUILDING);
	owner->setChainByHealth(CHAIN_FLY_DOWN);
}

void StateBirthInAir::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	safe_cast<UnitActing*>(owner)->makeDynamicXY(UnitActing::STATIC_DUE_TO_ANIMATION);
	for(int i = 0; i < safe_cast<UnitActing*>(owner)->transportSlots().size(); i++)
		owner->setAdditionalChain(CHAIN_SLOT_IS_EMPTY, i);
	owner->switchToState(StateTouchDown::instance());
}

bool StateBirthInAir::canFinish(UnitReal* owner) const
{
	return !safe_cast<UnitActing*>(owner)->rigidBody()->flyDownMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateTrigger::start(UnitReal* owner) const
{
	if(owner->attr().isActing()){
		UnitActing* unit(safe_cast<UnitActing*>(owner));
		unit->clearAttackTarget();
		unit->stop();
		unit->makeStaticXY(UnitActing::STATIC_DUE_TO_TRIGGER);
	}
	owner->setUnitState(UnitReal::TRIGGER_MODE);
	if(owner->attr().isLegionary())
		safe_cast<UnitLegionary*>(owner)->clearOrders();
	owner->stopChains();
}

void StateTrigger::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	owner->setUnitState(UnitReal::AUTO_MODE);
	owner->stopChains();
	if(owner->attr().isActing())
		safe_cast<UnitActing*>(owner)->makeDynamicXY(UnitActing::STATIC_DUE_TO_TRIGGER);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool StatePadStand::canStart(UnitReal* owner) const
{
    owner->setAdditionalChainByFactor(CHAIN_PAD_STAND, owner->player()->currentCapacity());
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StatePadGet::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_PAD_GET_SMTH);
}

bool StatePadGet::canFinish(UnitReal* owner) const
{
	StatePadStand::instance()->canStart(owner);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StatePadPut::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_PAD_PUT_SMTH);
}

bool StatePadPut::canFinish(UnitReal* owner) const
{
	StatePadStand::instance()->canStart(owner);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StatePadCarry::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_PAD_CARRY);
}

bool StatePadCarry::canFinish(UnitReal* owner) const
{
	StatePadStand::instance()->canStart(owner);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StatePadAttack::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_PAD_ATTACK);
}

bool StatePadAttack::canFinish(UnitReal* owner) const
{
	StatePadStand::instance()->canStart(owner);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateUninstal::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_UNINSTALL, owner->attr().chainUninsatalTime);
}

void StateUninstal::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	owner->Kill();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateDeath::start(UnitReal* owner) const
{
	owner->setCorpse();
}

void StateDeath::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->UnitBase::explode();
	owner->killCorpse();
}

bool StateDeath::canFinish(UnitReal* owner) const
{
	return __super::canFinish(owner) || owner->corpseQuant();
}

/////////////////////////////////////////////////////////////////////////////////////////////

UnitLegionaryPosibleStates::UnitLegionaryPosibleStates()
{
	push_back(StateTeleporting::instance());
	push_back(StateFall::instance());
	push_back(StateProduction::instance());
	push_back(StateLanding::instance());
	push_back(StateTransition::instance());
	push_back(StateGiveResource::instance());
	push_back(StateFrozen::instance());
	push_back(StateAttack::instance());
	push_back(StateWork::instance());
	push_back(StateFlyDown::instance());
	push_back(StateFlyUp::instance());
	push_back(StateMovements::instance());
}

/////////////////////////////////////////////////////////////////////////////////////////////

UnitBuildingPosibleStates::UnitBuildingPosibleStates()
{
	push_back(StateFall::instance());
	push_back(StateBeBuild::instance());
	push_back(StateDisconnect::instance());
	push_back(StateProduction::instance());
	push_back(StateAttack::instance());
	push_back(StateBuildingStand::instance());
}

/////////////////////////////////////////////////////////////////////////////////////////////

UnitItemPosibleStates::UnitItemPosibleStates()
{
	push_back(StateItemStand::instance());
}

/////////////////////////////////////////////////////////////////////////////////////////////

UnitPadPosibleStates::UnitPadPosibleStates()
{
	push_back(StatePadStand::instance());
}

/////////////////////////////////////////////////////////////////////////////////////////////

UnitAllPosibleStates::UnitAllPosibleStates()
{
	push_back(StateBase::instance());
	push_back(StateFall::instance());
	push_back(StateRise::instance());
	push_back(StateWeaponGrip::instance());
	push_back(StateTransition::instance());
	push_back(StateMovements::instance());
	push_back(StateFlyUp::instance());
	push_back(StateFlyDown::instance());
	push_back(StateWork::instance());
	push_back(StateGiveResource::instance());
	push_back(StatePickItem::instance());
	push_back(StateFrozen::instance());
	push_back(StateAttack::instance());
	push_back(StateTeleporting::instance());
	push_back(StateBeBuild::instance());
	push_back(StateDisconnect::instance());
	push_back(StateBuildingStand::instance());
	push_back(StateItemStand::instance());
	push_back(StateProduction::instance());
	push_back(StateMoveToCargo::instance());
	push_back(StateLandToLoad::instance());
	push_back(StateOpenForLanding::instance());
	push_back(StateCloseForLanding::instance());
	push_back(StateLanding::instance());
	push_back(StateUnlanding::instance());
	push_back(StateInTransport::instance());
	push_back(StateOpen::instance());
	push_back(StateClose::instance());
	push_back(StateUpgrade::instance());
	push_back(StateIsUpgraded::instance());
	push_back(StateBirth::instance());
	push_back(StateItemBirth::instance());
	push_back(StateBirthInAir::instance());
	push_back(StatePadStand::instance());
	push_back(StatePadCarry::instance());
	push_back(StatePadAttack::instance());
	push_back(StatePadGet::instance());
	push_back(StatePadPut::instance());
	push_back(StateTrigger::instance());
	push_back(StateUninstal::instance());
	push_back(StateDeath::instance());
}

StateBase* UnitAllPosibleStates::find(ChainID id)
{
	iterator i;
	FOR_EACH(*this, i)
		if((*i)->id() == id)
			return *i;
	xxassert(false, "Неизвестное состояние");
	return StateBase::instance();
}

/////////////////////////////////////////////////////////////////////////////////////////////
