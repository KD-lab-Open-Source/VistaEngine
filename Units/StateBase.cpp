#include "StdAfx.h"
#include "StateBase.h"
#include "IronLegion.h"
#include "IronBuilding.h"
#include "UnitItemInventory.h"
#include "UnitItemResource.h"
#include "Player.h"
#include "CameraManager.h"

bool UnitReal::changeState(StateBase* state)
{
	if(!desiredState_ || desiredState_->priority() < (state->priority())){
		if(prevState_){
			prevState_->finish(this, false);
			prevState_ = 0;
		}
		desiredState_ = state;
		return true;
	}
	return false;
}

void UnitReal::switchToState(StateBase* state)
{
	if(changeState(state))
		prevState_ = currentState_;
	else
		currentState_->finish(this, false);
	currentStateID_ = CHAIN_NONE;
	currentState_ = 0;
}

bool UnitReal::finishState(StateBase* state)
{
	if(state == currentState_){
		currentState_->finish(this);
		currentStateID_ = CHAIN_NONE;
		currentState_ = StateBase::instance();
		if(desiredState_){
			currentStateID_ = desiredState_->id();
			currentState_ = desiredState_;
			desiredState_ = 0;
			currentState_->start(this);
			prevState_ = 0;
		}
		return true;
	}
	return false;
}

bool UnitReal::interuptState(StateBase* state)
{
	if(state == currentState_){
		currentState_->finish(this, false);
		currentStateID_ = CHAIN_NONE;
		currentState_ = StateBase::instance();
		return true;
	}
	return false;
}

void UnitReal::startState(StateBase* state)
{
	if(currentState_){
		currentState_->finish(this, false);
	}else if(prevState_){
		prevState_->finish(this, false);
		prevState_ = 0;
	}
	currentStateID_ = state->id();
	currentState_ = state;
	currentState_->start(this);
}

void UnitReal::stateQuant()
{
	if(!posibleStates_)
		return;

	if(currentState_->canFinish(this)){
		currentState_->finish(this);
		currentStateID_ = CHAIN_NONE;
		currentState_ = 0;
	}
	if(!alive())
		return;
	States::const_iterator istate(posibleStates_->begin());
	int statePriority(max(currentState_ ? currentState_->priority() : 0, desiredState_ ? desiredState_->priority() : 0));
	while((*istate)->priority() >= statePriority){
		xxassert(istate < posibleStates_->end(), "Нет действия по умолчанию");
		if((*istate)->canStart(this)){
			if(prevState_){
				prevState_->finish(this, false);
				prevState_ = 0;
			}else if(currentState_)
				currentState_->finish(this, false);
			desiredState_ = 0;
			currentStateID_ = (*istate)->id();
			currentState_ = *istate;
			currentState_->start(this);
			return;
		}
		++istate;
	}
	if(desiredState_){
		if(currentState_)
			currentState_->finish(this, false);
		currentStateID_ = desiredState_->id();
		currentState_ = desiredState_;
		desiredState_ = 0;
		currentState_->start(this);
		prevState_ = 0;
	}
	xxassert(currentState_, "Нет действия по умолчанию");
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
	owner->setChainByHealth(CHAIN_FALL);
	owner->setCollisionGroup(0);
	owner->rigidBody()->disableWaterAnalysis();
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlFreeCamera();
}

void StateFall::finish(UnitReal* owner, bool finished) const
{
	owner->setCollisionGroup(owner->attr().collisionGroup);
	if(finished)
		owner->switchToState(StateRise::instance());
	else{
		__super::finish(owner, finished);
		owner->rigidBody()->enableWaterAnalysis();
		owner->enableFlying(0);
		UnitActing* unit(safe_cast<UnitActing*>(owner));
		if(unit->isActiveDirectControl())
			cameraManager->setDirectControlOffset(unit->directControlOffset(), 1000);
	}
}

bool StateFall::canStart(UnitReal* owner) const
{
	return 	firstStart(owner) && owner->rigidBody()->isBoxMode();
}

bool StateFall::canFinish(UnitReal* owner) const
{
	return !owner->rigidBody()->isBoxMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateRise::start(UnitReal* owner) const
{
	owner->setChainByHealthWithTimer(CHAIN_RISE, owner->attr().chainRiseTime);
	owner->makeStaticXY(UnitReal::STATIC_DUE_TO_RISE);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), owner->attr().chainRiseTime);
}

void StateRise::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_RISE);
	owner->rigidBody()->enableWaterAnalysis();
	owner->enableFlying(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateWeaponGrip::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_WEAPON_GRIP);
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
	if(owner->rigidBody()->onDeepWater) {
		prevState &= ~MOVEMENT_STATE_ON_WATER;
		prevState |= MOVEMENT_STATE_ON_GROUND;
	}else{
		prevState &= ~MOVEMENT_STATE_ON_GROUND;
		prevState &= ~MOVEMENT_STATE_ON_LOW_WATER;
		prevState |= MOVEMENT_STATE_ON_WATER;
		owner->makeStaticXY(UnitReal::STATIC_DUE_TO_TRANSITION);
	}
	int time = owner->attr().chainTransitionTime;
	owner->setChainTransition(prevState, state, time);
	if(owner->rigidBody()->waterAnalysis())
		owner->rigidBody()->enableFlyDownModeByTime(time);
	owner->rigidBody()->deepWaterChanged = false;
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), time);
}

void StateTransition::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->rigidBody()->disableFlyDownMode();
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_TRANSITION);
}

bool StateTransition::canStart(UnitReal* owner) const
{
	return owner->rigidBody()->deepWaterChanged;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateMovements::start(UnitReal* owner) const
{
	if(owner->rigidBody()->unitMoveOrRotate()){
		if(owner->rigidBody()->getRotationMode() != RigidBodyUnit::ROT_NONE)
			owner->setChainByHealth(CHAIN_TURN);
		else{
			if(owner->rigidBody()->isRunMode())
				owner->setChainByHealth(CHAIN_RUN);
			else
				owner->setChainByHealth(CHAIN_WALK);
		}
	}else
		owner->setChain(CHAIN_STAND);
}

void StateMovements::finish(UnitReal* owner, bool finished) const 
{
	__super::finish(owner, false);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFlyUp::start(UnitReal* owner) const
{
	owner->setChainWithTimer(CHAIN_FLY_UP, owner->attr().chainFlyDownTime);
}

bool StateFlyUp::canStart(UnitReal* owner) const
{
	return firstStart(owner) && owner->rigidBody()->isFlyDownMode() && owner->rigidBody()->flyingMode();
}

bool StateFlyUp::canFinish(UnitReal* owner) const
{
	return (__super::canFinish(owner) && !owner->rigidBody()->isFlyDownMode()) || !owner->rigidBody()->flyingMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateFlyDown::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_FLY_DOWN);
}

bool StateFlyDown::canStart(UnitReal* owner) const
{
	return firstStart(owner) && owner->rigidBody()->isFlyDownMode() && !owner->rigidBody()->flyingMode();
}

bool StateFlyDown::canFinish(UnitReal* owner) const
{
	return !owner->rigidBody()->isFlyDownMode() || owner->rigidBody()->flyingMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateWork::start(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->nearConstructedBuilding()){
		owner->makeStaticXY(UnitReal::STATIC_DUE_TO_ANIMATION);
		owner->setChainByHealth(CHAIN_BUILD);
		return;
	}
	if(unit->resourcerMode() == UnitLegionary::RESOURCER_PICKING){
		owner->makeStaticXY(UnitReal::STATIC_DUE_TO_ANIMATION);
		owner->setChainByHealth(CHAIN_PICKING);
		if(unit->resourceItem())
			unit->resourceItem()->changeState(StateGiveResource::instance());
	}else{
		owner->setChain(owner->rigidBody()->unitMove() ? CHAIN_WALK_WITH_RESOURCE : CHAIN_STAND_WITH_RESOURCE, unit->resourcerCapacityIndex());
	}
}
void StateWork::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_ANIMATION);
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
	owner->setChainWithTimer(CHAIN_GIVE_RESOURCE, owner->attr().chainGiveResourceTime);
	if(owner->attr().isLegionary())
		owner->makeStaticXY(UnitReal::STATIC_DUE_TO_GIVE_RESOURCE);
}

void StateGiveResource::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	if(owner->attr().isLegionary()){
		owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_GIVE_RESOURCE);
		if(finished){
			safe_cast<UnitLegionary*>(owner)->giveResource();
		}
	}
}

bool StateGiveResource::canStart(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return firstStart(owner) && unit->resourcerMode() == UnitLegionary::RESOURCER_RETURNING 
		&& unit->resourceCollector() && owner->nearUnit(unit->resourceCollector());
}

bool StateGiveResource::canFinish(UnitReal* owner) const 
{ 
	return __super::canFinish(owner) && (!owner->attr().isLegionary() || safe_cast<UnitLegionary*>(owner)->resourceCollector()); 
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

void StateTeleporting::start(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
    owner->startChainTimer(safe_cast_ref<const AttributeBuilding&>(unit->teleport()->attr()).teleportationTime*1000);
	owner->hide(UnitReal::HIDE_BY_TELEPORT, true);
	owner->makeStatic(UnitReal::STATIC_DUE_TO_TELEPORTATION);
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
		owner->makeDynamic(UnitReal::STATIC_DUE_TO_TELEPORTATION);
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

bool StateBeBuild::canStart(UnitReal* owner) const
{
	return !safe_cast<UnitBuilding*>(owner)->isConstructed();
}

bool StateBeBuild::canFinish(UnitReal* owner) const
{
	return safe_cast<UnitBuilding*>(owner)->isConstructed();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateDisconnect::start(UnitReal* owner) const
{
	owner->setAdditionalChain(CHAIN_DISCONNECT);
}

void StateDisconnect::finish(UnitReal* owner, bool finished) const 
{
	__super::finish(owner, false);
	owner->stopAdditionalChain(CHAIN_DISCONNECT);
}

bool StateDisconnect::canStart(UnitReal* owner) const
{
	return firstStart(owner) && !safe_cast<UnitBuilding*>(owner)->isConnected();
}

bool StateDisconnect::canFinish(UnitReal* owner) const
{
	StateBuildingStand::instance()->canStart(owner);
	return safe_cast<UnitBuilding*>(owner)->isConnected();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool StateBuildingStand::canStart(UnitReal* owner) const
{
    owner->setChainByHealth(CHAIN_STAND);
	return true;
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
	return unit->producedUnit() && !unit->shippedUnit();
}

bool StateProduction::canFinish(UnitReal* owner) const
{
	StateBuildingStand::instance()->canStart(owner);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	return !unit->producedUnit() || unit->shippedUnit();
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
		if(owner->rigidBody()->flyingMode())
			owner->switchToState(StateLandToLoad::instance());
		else
			owner->switchToState(StateOpenForLanding::instance());
	}
		
}

bool StateMoveToCargo::canFinish(UnitReal* owner) const
{
	UnitActing* unit(safe_cast<UnitActing*>(owner));
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
	if(!owner->rigidBody()->prm().flyingMode || owner->nearUnit(unit->cargo(), owner->rigidBody()->flyingMode() ? 10 : owner->attr().transportLoadRadius))
		return true;
	StateMovements::instance()->start(owner);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateLandToLoad::start(UnitReal* owner) const
{
	owner->disableFlying(UnitReal::FLYING_DUE_TO_TRANSPORT_LOAD);
	StateFlyDown::instance()->start(owner);
}

void StateLandToLoad::finish(UnitReal* owner, bool finished) const
{
	if(finished)
		owner->switchToState(StateOpenForLanding::instance());
	else{
		__super::finish(owner, finished);
		owner->enableFlying(UnitReal::FLYING_DUE_TO_TRANSPORT_LOAD);
	}
}

bool StateLandToLoad::canFinish(UnitReal* owner) const
{
	return !owner->rigidBody()->isFlyDownMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateOpenForLanding::start(UnitReal* owner) const
{
	owner->setChainWithTimer(CHAIN_OPEN_FOR_LANDING, owner->attr().chainOpenForLandingTime);
	owner->makeStaticXY(UnitReal::STATIC_DUE_TO_OPEN_FOR_LOADING);
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
		owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_OPEN_FOR_LOADING);
		owner->enableFlying(UnitReal::FLYING_DUE_TO_TRANSPORT_LOAD);
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
				if(owner->rigidBody()->prm().flyingMode)
					return true;
			}
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateCloseForLanding::start(UnitReal* owner) const
{
	owner->setChainWithTimer(CHAIN_CLOSE_FOR_LANDING, owner->attr().chainCloseForLandingTime);
}

void StateCloseForLanding::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_OPEN_FOR_LOADING);
	owner->enableFlying(UnitReal::FLYING_DUE_TO_TRANSPORT_LOAD);
	if(finished){
		if(safe_cast<UnitActing*>(owner)->cargo())
			owner->switchToState(StateMoveToCargo::instance());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateLanding::start(UnitReal* owner) const
{
	owner->makeStatic(UnitReal::STATIC_DUE_TO_SITTING_IN_TRANSPORT);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlFreeCamera();
}

void StateLanding::finish(UnitReal* owner, bool finished) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(unit->transport()){
		unit->transport()->makeDynamicXY(UnitReal::STATIC_DUE_TO_TRANSPORT_LOADING);
		unit->transport()->setReadyForLanding(true);
		if(finished){
			owner->finishChainDelayTimer();
			owner->switchToState(StateInTransport::instance());
			return;
		}
	}
	unit->putUnitOutTransport();
	if(unit->isActiveDirectControl())
		cameraManager->setDirectControlOffset(unit->directControlOffset(), 1000);
	__super::finish(owner, finished);
}

bool StateLanding::canStart(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	return firstStart(owner) && unit->transport() && owner->nearUnit(unit->transport(), unit->attr().isLegionary() ? 0.0f : 16.0f);
}

bool StateLanding::canFinish(UnitReal* owner) const
{
	UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
	if(!__super::canFinish(owner) && unit->transport()){
		if(unit->transport()->isReadyForLanding()){
			UnitLegionary* unit(safe_cast<UnitLegionary*>(owner));
			owner->setChainByHealthWithTimer(CHAIN_LANDING, owner->attr().chainLandingTime);
			unit->transport()->makeStaticXY(UnitReal::STATIC_DUE_TO_TRANSPORT_LOADING);
			unit->transport()->setReadyForLanding(false);
		}else{
			StateMovements::instance()->start(owner);
		}
		return false;
	}
	return true;
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
	safe_cast<UnitLegionary*>(owner)->putUnitOutTransport();
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
	if(finished){
		owner->switchToState(StateClose::instance());
	}
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
	if(unit->productionCounter() > 0)
		unit->shipProduction();
	else
		unit->finishProduction();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateUpgrade::start(UnitReal* owner) const
{
	owner->setChainWithTimer(CHAIN_UPGRADE, owner->attr().upgrades[owner->executedUpgradeIndex()].chainUpgradeTime, 
		owner->attr().upgrades[owner->executedUpgradeIndex()].chainUpgradeNumber);
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
	owner->disableFlying(UnitReal::FLYING_DUE_TO_UPGRADE);
}

void StateIsUpgraded::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_UPGRADE);
	owner->enableFlying(UnitReal::FLYING_DUE_TO_UPGRADE);
	UnitActing* unit(safe_cast<UnitActing*>(owner));
	unit->setUpgradeProgresParameters();
	if(finished)
		unit->player()->checkEvent(EventUnitUnitAttributePlayer(Event::COMPLETE_UPGRADE, &unit->beforeUpgrade(), unit, unit->player()));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateBirth::start(UnitReal* owner) const
{
	owner->setChain(CHAIN_BIRTH);
}

void StateBirth::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->detachFromDock();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateBirthInAir::start(UnitReal* owner) const
{
	owner->makeStaticXY(UnitReal::STATIC_DUE_TO_ANIMATION);
	owner->makeDynamic(UnitReal::STATIC_DUE_TO_BUILDING);
	owner->setChain(CHAIN_FLY_DOWN);
}

void StateBirthInAir::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, finished);
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_ANIMATION);
	for(int i = 0; i < safe_cast<UnitActing*>(owner)->transportSlots().size(); i++)
		owner->setAdditionalChain(CHAIN_SLOT_IS_EMPTY, i);
}

bool StateBirthInAir::canFinish(UnitReal* owner) const
{
	return !owner->rigidBody()->isFlyDownMode();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateTrigger::start(UnitReal* owner) const
{
	if(owner->attr().isActing())
		safe_cast<UnitActing*>(owner)->clearAttackTarget();
	owner->setUnitState(UnitReal::TRIGGER_MODE);
	owner->stop();
	owner->makeStaticXY(UnitReal::STATIC_DUE_TO_TRIGGER);
	if(owner->attr().isLegionary())
		safe_cast<UnitLegionary*>(owner)->clearOrders();
	owner->stopChains();
}

void StateTrigger::finish(UnitReal* owner, bool finished) const
{
	__super::finish(owner, false);
	owner->setUnitState(UnitReal::AUTO_MODE);
	owner->stopChains();
	owner->makeDynamicXY(UnitReal::STATIC_DUE_TO_TRIGGER);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StateUninstal::start(UnitReal* owner) const
{
	owner->setChainWithTimer(CHAIN_UNINSTALL, owner->attr().chainLandingTime);
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
	return owner->corpseQuant();
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
	push_back(StateInTransport::instance());
	push_back(StateOpen::instance());
	push_back(StateClose::instance());
	push_back(StateUpgrade::instance());
	push_back(StateIsUpgraded::instance());
	push_back(StateBirth::instance());
	push_back(StateBirthInAir::instance());
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