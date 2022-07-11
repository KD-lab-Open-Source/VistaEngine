#include "StdAfx.h"
#include "UnitActing.h"
#include "UniverseX.h"
#include "Squad.h"
#include "IronLegion.h"
#include "IronBuilding.h"
#include "UserInterface\UI_Logic.h"
#include "GameOptions.h"
#include "UserInterface\UserInterface.h"
#include "Environment\Environment.h"
#include "Water\Water.h"
#include "Water\SkyObject.h"
#include "RenderObjects.h"
#include "MicroAI.h"
#include "AI\PFTrap.h"
#include "UnitItemInventory.h"
#include "CameraManager.h"
#include "StreamCommand.h"
#include "Water\CircleManager.h"
#include "UnitItemResource.h"
#include "PositionGeneratorSquad.h"
#include "Physics\crash\CrashSystem.h"

BEGIN_ENUM_DESCRIPTOR(DirectControlMode, "DirectControlMode")
REGISTER_ENUM(DIRECT_CONTROL_DISABLED, "None")
REGISTER_ENUM(DIRECT_CONTROL_ENABLED, "Direct Control")
REGISTER_ENUM(SYNDICATE_CONTROL_ENABLED, "Syndicate Control")
END_ENUM_DESCRIPTOR(DirectControlMode)

UNIT_LINK_GET(UnitActing)

bool UnitActing::freezedByTrigger_;

#pragma warning(disable: 4355)

UnitActing::UnitActing(const UnitTemplate& data) 
: UnitObjective(data)
{
	flyingReason_ = 0;
	staticReason_ = 0;
	
	if(rigidBody() && rigidBody()->isUnit()){
		rigidBody()->setWaterLevel(attr().waterLevel);
		rigidBody()->setImpassability(attr().impassability);
		rigidBody()->setEnvironmentDestruction(attr().environmentDestruction);
		rigidBody()->setFieldFlag(1 << (20 + player()->playerID()));
		enableFlying(0);
	}

	isMoving_ = false;

	rotationDeath = 0;

	isUnderWaterSilouettePrev_ = false;

	if(attr().realSuspension && !attr().rigidBodyCarPrm.empty()){
		modelLogic()->Update();
		rigidBody()->setRealSuspension(model(), attr().rigidBodyCarPrm);
	}else{
		if(attr().wheelList.size() >= 3){
			modelLogic()->Update();
			rigidBody()->setWhellController(attr().wheelList, model(), modelLogic());
		}
	}

	initWeapon();

	attackMode_ = attackModeAttr().attackMode();

	directKeysCommand = UnitCommand(COMMAND_ID_DIRECT_KEYS, 0, Vect3f::ZERO, 0);
	
	weaponAnimationMode_ = false;
	directControlWeaponSlot_ = 0;

	selectedWeaponID_ = 0;
	preSelectedWeaponID_ = 0;

	targetUnit_ = 0;
	targetPoint_ = Vect3f::ZERO;
	targetPointEnable = false;

	specialTargetUnit_ = 0;
	specialTargetPoint_ = Vect3f::ZERO;
	specialTargetPointEnable = false;

	noiseTarget_ = 0;
	hadTargetUnit_ = false;

	directControlFireMode_ = 0;

	directControl_ = DIRECT_CONTROL_DISABLED;

	activeDirectControl_ = DIRECT_CONTROL_DISABLED;

	attackTargetUnreachable_ = false;

	landToUnloadStart = false;
	putOutTransportIndex_ = -1;
	readyForLanding = false;
	transportSlots_.resize(attr().transportSlots.size(), 0);
	playerPrev_ = 0;

	autoFindTransport_ = false;

	producedUnit_ = 0;
	shippedUnit_ = 0;
	wasProduced_ = false;
	shippedSquad_ = 0;
	productionCounter_ = 0;
	shipmentPosition_.set(0, -1);
	teleportProduction_ = false;
	squadReserved_ = false;

	productivityRest_ = attr().productivityTotal;
	producedParameter_ = 0;
	
	totalProductionCounter_ = attr().totalProductionNumber;

	aiScanTimer.start(1000 + logicRNDfabsRnd(2000));

	executedUpgradeIndex_ = -1;
	upgradeTime_ = 0;
	previousUpgradeIndex_ = -1;
	finishUpgradeTime_ = 0;

	ignoreFreezedByTrigger_ = false;

	bodyParts_.reserve(attr().bodyParts.size());
	BodyPartAttributes::const_iterator i;
	FOR_EACH(attr().bodyParts, i){
		bodyParts_.push_back(BodyPart(*i));
		updatePart(bodyParts_.back());
	}

	beforeUpgrade_ = 0;

	selectionListPriority_ = attr().selectionListPriority;
	
	showParamController_.create(this, player()->resource(), ShowChangeParametersController::SET);

	if(attr().defaultDirectControlEnabled)
		setDirectControl(GlobalAttributes::instance().directControlMode, false);

	isInvisiblePrev_ = false;
	specialMinimapSymbolActivated_ = false;
}

UnitActing::~UnitActing()
{
	xassert(!isRegisteredInRealUnits());
	weaponSlots_.clear();
}

void UnitActing::Kill()
{
	if(!transportSlots_.empty()){
		if(isDirectControl())
			findMainUnitInTransport();
		for(int i = 0; i < transportSlots_.size(); i++){
			UnitLink<UnitLegionary> unit(transportSlots_[i]);
			if(unit && unit->alive()){
				if(attr().transportSlots[i].destroyWithTransport)
					unit->Kill();
				else{
					if(rigidBody()->isSinking())
						unit->rigidBody()->startSinking(rigidBody()->flyingHeightCurrent());
					unit->clearTransport();
				}
				transportSlots_[i] = 0;
			}
		}
	}

	setDirectControl(DIRECT_CONTROL_DISABLED);
	
	__super::Kill();
	
	RealsLinks::iterator iu;
	FOR_EACH(dockedUnits, iu)
		if(UnitReal* unit = *iu)
			unit->interuptState(StateWeaponGrip::instance());
	dockedUnits.clear();

	WeaponSlots::iterator it;
	FOR_EACH(weaponSlots_, it)
		it->kill();

	cancelProduction();
}

void UnitActing::updateSelectionListPriority()
{ 
	if(attr().isTransport()){
		selectionListPriority_ = attr().selectionListPriority;
		bool setForceMainUnit_ = false;
		LegionariesLinks::const_iterator i;
		FOR_EACH(transportSlots_, i){
			UnitLegionary* unit(*i);
			if(unit && unit->isDocked()){
				if(unit->isForceMainUnit())
					setForceMainUnit_ = true;
				if(unit->selectionListPriority() > selectionListPriority_)
					selectionListPriority_ = unit->selectionListPriority();
			}
		}
		if(attr().isLegionary())
			safe_cast<UnitLegionary*>(this)->setForceMainUnit(setForceMainUnit_);
	}
	if(UnitSquad* squad = getSquadPoint())
		squad->updateMainUnit();
}

void UnitActing::serialize(Archive& ar)
{
    __super::serialize(ar);
	
	showParamController_.setOwner(this);

	if(universe()->userSave()){
		if(currentState() == CHAIN_DEATH)
			ar.serialize(rotationDeath, "rotationDeath", 0);
		
		int staticReason(staticReason_), flyingReason(flyingReason_);
		if(rigidBody()->prm().flyingMode)
			ar.serialize(flyingReason, "flyingReason", 0);
		ar.serialize(staticReason, "staticReason", 0);
		if(ar.isInput()){
			if(flyingReason)
				disableFlying(flyingReason);
			if(staticReason)
				makeStatic(staticReason);
		}
		ar.serialize(dockedUnits, "dockedUnits", 0);
		if(!transportSlots_.empty()){
			ar.serialize(selectionListPriority_, "selectionListPriority", 0);
			ar.serialize(transportSlots_, "transportSlots", 0);
			ar.serialize(cargo_, "cargo", 0);
			ar.serialize(landToUnloadStart, "landToUnloadStart", 0);
			if(landToUnloadStart)
				ar.serialize(putOutTransportIndex_, "putOutTransportIndex", 0);
			ar.serialize(readyForLanding, "readyForLanding", 0);
		}
		ar.serialize(autoFindTransport_, "autoFindTransport", 0);

		ar.serialize(productivityRest_, "productivityRest", 0);
		BodyParts::iterator i;
		FOR_EACH(bodyParts_, i)
			ar.serialize(*i, "bodyPart", 0);

		ar.serialize(executedUpgradeIndex_, "executedUpgradeIndex", 0);
		if(ar.isInput() && executedUpgrade())
			reserveSquadNumber(executedUpgrade(), 1);
		ar.serialize(upgradeTime_, "upgradeTime", 0);
		if(executedUpgradeIndex_ >= 0)
			ar.serialize(finishUpgradeTime_, "previousUpgradeTime", 0);
		if(currentState() & CHAIN_IS_UPGRADED)
			ar.serialize(previousUpgradeIndex_, "previousUpgradeIndex", 0);
		
		if(attr().canChangeVisibility){
			ar.serialize(visibleTimer_, "visibleTimer", 0);
			if(!attr().invisible)
				ar.serialize(unVisibleTimer_, "unVisibleTimer", 0);
		}

		ar.serialize(attackMode_, "attackMode", 0);
		
		if(!weaponSlots_.empty()){
			ar.serialize(weaponSlots_, "weaponSlots", 0);
			ar.serialize(weaponAnimationMode_, "weaponAnimationMode", 0);
			ar.serialize(selectedWeaponID_, "selectedWeaponID", 0);
			ar.serialize(targetUnit_, "targetUnit", 0);
			ar.serialize(targetPointEnable, "targetPointEnable", 0);
			if(targetPointEnable)
				ar.serialize(targetPoint_, "targetPoint", 0);
			ar.serialize(specialTargetUnit_, "specialTargetUnit", 0);
			ar.serialize(specialTargetPointEnable, "specialTargetPointEnable", 0);
			if(specialTargetPointEnable)
				ar.serialize(specialTargetPoint_, "specialTargetPoint", 0);
			ar.serialize(noiseTarget_, "noiseTarget", 0);
			ar.serialize(attackTargetUnreachable_, "attackTargetUnreachable", 0);
			ar.serialize(directControlWeaponSlot_, "directControlWeaponSlot", 0);
		}

		ar.serialize(*rigidBody(), "rigidBody", 0);

		if(ar.isOutput()){
            if(!attr().producedUnits.empty() && producedUnit_){
				AttributeBase::ProducedUnitsVector::const_iterator iu;
				FOR_EACH(attr().producedUnits, iu)
					if(iu->second.unit == producedUnit_)
						ar.serialize(iu->first, "producedUnitNum", 0);
			}
			else
				ar.serialize(-1, "producedUnitNum", 0);
			if(!attr().producedParameters.empty() && producedParameter_){
				AttributeBase::ProducedParametersList::const_iterator ip; 
				FOR_EACH(attr().producedParameters, ip)
					if(&(ip->second) == producedParameter_)
						ar.serialize(ip->first, "producedParametersNum", 0);
			}
			else
				ar.serialize(-1, "producedParametersNum", 0);

			ar.serialize(playerPrev_ ? playerPrev_->playerID() : -1, "playerPrev", 0);
		}else{
			int producedNum = -1;
			if(!attr().producedUnits.empty()){
				ar.serialize(producedNum, "producedUnitNum", 0);
				if(producedNum >= 0)
					producedUnit_ = attr().producedUnits[producedNum].unit;
			}
			if(!attr().producedParameters.empty()){
				producedNum = -1;
				ar.serialize(producedNum, "producedParametersNum", 0);
				if(producedNum >= 0)
					producedParameter_ = &(attr().producedParameters[producedNum]);
			}

			int playerPrev = -1;
			ar.serialize(playerPrev, "playerPrev", 0);
			if(playerPrev >= 0)
				playerPrev_ = universe()->findPlayer(playerPrev);
		}

		ar.serialize(directControl_, "directControlMode", 0);

		if(!attr().producedUnits.empty() || !attr().producedParameters.empty()){
			ar.serialize(producedQueue_, "producedQueue", 0);
			ar.serialize(resourceItemForProducedUnit_, "resourceItemForProducedUnit", 0);
			ar.serialize(shipmentPosition_, "shipmentPosition", 0);
			if(producedUnit_){
				ar.serialize(callFinishProductionTimer_, "callFinishProductionTimer", 0);
				ar.serialize(teleportProduction_, "teleportProduction", 0);
				ar.serialize(squadReserved_, "squadReserved", 0);
				ar.serialize(productionCounter_, "productionCounter", 0);
				if(ar.isInput())
					player()->reserveUnitNumber(producedUnit_, productionCounter_);
				ar.serialize(productionConsumer_, "productionConsumer", 0);
				ar.serialize(wasProduced_, "wasProduced", 0);
				if(wasProduced_)
					ar.serialize(shippedUnit_, "shippedUnit", 0);
				ar.serialize(shippedSquad_, "shippedSquad", 0);
				if(ar.isInput() && !shippedSquad_)
					if(producedUnit_->isLegionary())
						player()->reserveUnitNumber(safe_cast<const AttributeLegionary*>(producedUnit_)->squad, -1);
			}
			if(producedParameter_)
				ar.serialize(productionParameterTimer_, "productionParameterTimer", 0);
			ar.serialize(productivityRest_, "productivityRest", 0);
			ar.serialize(totalProductionCounter_, "totalProductionCounter", 0);
		}

		ar.serialize(specialMinimapSymbolActivated_, "specialMinimapSymbolActivated", 0);
	}
}

void UnitActing::relaxLoading()
{
	if(!attr().producedUnits.empty() && producedUnit_ && shippedSquad_)
		shippedSquad_->reserveUnitNumber(producedUnit_, productionCounter_);

	DirectControlMode mode = directControl();
	if(mode && alive()){
		directControl_ = DIRECT_CONTROL_DISABLED;
		setDirectControl(mode);
	}
}

void UnitActing::executeCommand(const UnitCommand& command)
{
	switch(command.commandID()){
	case COMMAND_ID_KILL_UNIT:
		setRegisteredInPlayerStatistics(false);
		Kill();
		break;

	case COMMAND_ID_DIRECT_CONTROL:
		setDirectControl(command.commandData() ? DIRECT_CONTROL_ENABLED : DIRECT_CONTROL_DISABLED);
		break;
	case COMMAND_ID_DIRECT_KEYS:
		setDirectKeysCommand(command);
		break;

	case COMMAND_ID_DIRECT_CHANGE_WEAPON:
		changeDirectControlWeapon(command.commandData());
		break;

	case COMMAND_ID_POINT:
		if(attr().isBuilding() || isProducing())
			if(!attr().producedUnits.empty()){
				resourceItemForProducedUnit_ = 0;
				if(!pathFinder->checkImpassability(command.position().xi(), command.position().yi(), impassability()))
					shipmentPosition_ = command.position();
				return;
			}
		fireStop();
		targetUnit_ = 0;
		break;

	case COMMAND_ID_PRODUCE:
		if(!command.shiftModifier()){
			if(attr().producedUnits.exists(command.commandData())){
				const AttributeBase* attribute = attr().producedUnits[command.commandData()].unit;
				if(attribute)
					startProduction(attribute, 0, attr().producedUnits[command.commandData()].number);
			}
		}
		else{
			for(int i = 0; i < player()->race()->produceMultyAmount; i++)
				executeCommand(UnitCommand(COMMAND_ID_PRODUCE, command.commandData()));
		}
		break;

	case COMMAND_ID_PRODUCE_PARAMETER:
		if(attr().producedParameters.exists(command.commandData()) && producedQueue_.size() < attr().producedUnitQueueSize){
			const ProducedParameters& prm = attr().producedParameters[command.commandData()];
			if(accessibleParameter(prm) && !player()->checkUniqueParameter(prm.signalVariable.c_str())){
				subResource(prm.cost);
				player()->addUniqueParameter(prm.signalVariable.c_str());
				producedQueue_.push_back(ProduceItem(PRODUCE_RESOURCE, command.commandData()));
				if(!isProducing() || producedQueue_.size() == 1)
					startProductionParamater(prm);
			}
		}
		break;

	case COMMAND_ID_CANCEL_PRODUCTION:
		if(command.commandData() >= 0 && command.commandData() < producedQueue_.size()){
			if(!command.commandData() && shippedUnit_)
				break;

			cancelUnitProduction(producedQueue_[command.commandData()]);
			
			if(!command.commandData()){
				cancelProduction();
			}
			else{
				producedQueue_.erase(producedQueue_.begin() + command.commandData());
				if(command.commandData() == 1 && !isProducing())
					finishProduction();
			}
		}
		break;

	case COMMAND_ID_OBJECT: {
		UnitInterface* unit = command.unit();
		if(unit && (attr().isBuilding() || isProducing()) && !attr().producedUnits.empty() && 
		  (unit->attr().isResourceItem() || unit->attr().isInventoryItem()) ){
			resourceItemForProducedUnit_ = unit;
			shipmentPosition_ = command.unit()->position2D();
		}
		else {
			stop();
			selectWeapon(command.commandData());
			setManualTarget(command.unit());
		}
		if(command.unit() && (isEnemy(command.unit()) || command.unit()->player()->isWorld()))
			wayPointsClear();
		break;
		}

	case COMMAND_ID_STOP: 
		stop();
		break;

	case COMMAND_ID_PUT_OUT_TRANSPORT: 
		if(rigidBody()->flyingMode()){
			landToUnloadStart = true;
			disableFlying(FLYING_DUE_TO_TRANSPORT_UNLOAD);
			putOutTransportIndex_ = -1;
		} else {
			if(!rigidBody()->flyDownMode()){
				putOutTransport();
			} else {
				landToUnloadStart = true;
				putOutTransportIndex_ = -1;
			}
		}
		break;

	case COMMAND_ID_PUT_UNIT_OUT_TRANSPORT: 
		if(rigidBody()->flyingMode()){
			landToUnloadStart = true;
			disableFlying(FLYING_DUE_TO_TRANSPORT_UNLOAD);
			putOutTransportIndex_ = command.commandData();
		} else {
			if(!rigidBody()->flyDownMode()){
				putOutTransport(command.commandData());
			} else {
				landToUnloadStart = true;
				putOutTransportIndex_ = command.commandData();
			}
		}
		break;

	case COMMAND_ID_DIRECT_PUT_OUT_TRANSPORT: {
		int slot = findMainUnitInTransport();
		if(slot >= 0)
			executeCommand(UnitCommand(COMMAND_ID_PUT_UNIT_OUT_TRANSPORT, slot));
		break;
		}
	case COMMAND_ID_UPGRADE:
		if(!executedUpgrade())
			upgrade(command.commandData());
		break;

	case COMMAND_ID_CANCEL_UPGRADE:
		interuptState(StateUpgrade::instance());
		break;

	case COMMAND_ID_UNIT_SELECTED:
		if(player()->active() && command.commandData() == player()->teamIndex())
			setActiveDirectControl(directControl());
		break;
	case COMMAND_ID_UNIT_DESELECTED:
		setDirectControl(DIRECT_CONTROL_DISABLED);
		break;

	case COMMAND_ID_WEAPON_ACTIVATE:
		if(WeaponBase* p = findWeapon(command.commandData()))
			p->toggleAutoFire(command.shiftModifier());
		break;
	case COMMAND_ID_WEAPON_DEACTIVATE:
		fireStop(command.commandData());
		break;
	case COMMAND_ID_WEAPON_ID:
		setPreSelectedWeaponID(command.commandData());
		break;
	}

	__super::executeCommand(command);
}

void fSendCommandUnit(void* data)
{
	UnitActing** unit = (UnitActing**)data;
	(*unit)->sendCommand(*(UnitCommand*)(unit+1));
}

bool UnitActing::isInvisible() const
{
	return !visibleTimer_.busy() && (attr().invisible || unVisibleTimer_.busy());
}

void UnitActing::setVisibility(bool visible, float time)
{
	// если хоть кто-то подсвечивает, то юнит виден
	// если никто не подсвечивает, то если юнит невидимка или кем-то скрыт, то он невидим
	// одним таймером не обойтись, т.к. будет зависеть от пор€дка обхода действующих скрывателей/детекторов

	if(!attr().canChangeVisibility)
		return;

	if(visible)
		visibleTimer_.start(time > FLT_EPS ? int(time * 1000.0f) : 250);
	else
		unVisibleTimer_.start(time > FLT_EPS ? int(time * 1000.0f) : 250);
}

void UnitActing::graphQuant(float dt)
{
	__super::graphQuant(dt);

	if(selected()){
		showCircles(interpolatedPose().trans());

		if(selectedWeaponID_) if(const WeaponBase* weapon = findWeapon(selectedWeaponID_))
			if(weapon->weaponPrm()->hasTargetMark() && specialFireTargetExist() && !fireDistanceCheck())
				UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, 
				Se3f(QuatF::ID, specialFirePosition()), &weapon->weaponPrm()->targetMark(), this, 0));

		int wid = UI_LogicDispatcher::instance().selectedWeaponID();
		WeaponSlots::const_iterator it;
		FOR_EACH(weaponSlots_, it){
			WeaponBase* cur = it->weapon();
			if(selectedWeaponID_ && selectedWeaponID_ == cur->weaponPrm()->ID() ||
				wid && wid  == cur->weaponPrm()->ID() ||
				cur->weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_DEFAULT)
				cur->showInfo(interpolatedPose().trans());
		}
		
		if((attr().isBuilding() && !attr().producedUnits.empty()) || producedUnit_ && !teleportProduction_){
			UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_SHIPMENT_POINT, 
				Se3f(QuatF::ID, To3D(shipmentPosition())), &player()->race()->shipmentPositionMark(), this));
		}
	}

	if(attr().isTransport() && checkShowEvent(attr().transportSlotShowEvent) && isVisibleUnderForOfWar() && !unvisible()){
		Vect2f currentSlot(-0.5f*transportSlots().size(),  0.f);
		Vect3f pos = interpolatedPose().trans();
		LegionariesLinks::const_iterator it;
		FOR_EACH(transportSlots(), it){
			UnitInterface* unit = *it;
			if(unit && unit->isDocked())
				attr().transportSlotFill.draw(pos, attr().initialHeightUIParam, player()->unitColor(), currentSlot);
			else
				attr().transportSlotEmpty.draw(pos, attr().initialHeightUIParam, player()->unitColor(), currentSlot);
			currentSlot.x += 1.f;
		}
	}
}

void UnitActing::frozenQuant()
{
	if(rigidBody()->isUnderWaterSilouette()){
		if(!isUnderWaterSilouettePrev_){
			streamLogicCommand.set(fCommandSetUnderwaterSiluet) << model() << true;
			isUnderWaterSilouettePrev_ = true;
		}
		if(!isFrozen())
			setColor(player()->underwaterColor());
	}else if(isUnderWaterSilouettePrev_){
		streamLogicCommand.set(fCommandSetUnderwaterSiluet) << model() << false;
		isUnderWaterSilouettePrev_ = false;
	}

	frozenCounter_--;
	freezeAnimationCounter_--;

	if(rigidBody()->isFrozen() && rigidBody()->onIce())
		makeFrozen();

	if(isFrozen()){
		makeStatic(STATIC_DUE_TO_FROZEN);
		disableAnimation(ANIMATION_FROZEN);
		if(frozenCounter_ > 0 && iceEffect().isEnabled())
			setAbnormalState(iceEffect(), 0);
	} else {
		if(getStaticReason() & STATIC_DUE_TO_FROZEN){
			makeDynamic(STATIC_DUE_TO_FROZEN);
			enableAnimation(ANIMATION_FROZEN);
			if(currentState() & ~(CHAIN_DEATH | CHAIN_FALL)){
				rigidBody()->disableBoxMode();
				rigidBody()->setWaterAnalysis(rigidBody()->prm().waterAnalysis);
				enableFlying(0);
			}
		}
	}
}

bool UnitActing::canForceFire()
{
	WeaponSlots::iterator it;
	FOR_EACH(weaponSlots_, it)
		if(it->weapon()->weaponPrm()->syndicateControlMode() == WEAPON_SYNDICATE_CONTROL_FORCE)
			return true;
	return false;
}

void UnitActing::Quant()
{
	if(!alive()){
		UnitReal::Quant();
		frozenQuant();
		return;
	}
	
	if(freezedByTrigger_ && currentState() != CHAIN_TRIGGER && !ignoreFreezedByTrigger_){
		fowQuant();
		return;
	}

	if(canFireInCurentState())
		attackQuant();

	if(attr().checkRequirementForMovement){
		bool requirmentForMovement = false;
		for(int i = 0; i < transportSlots_.size(); i++)
			if(attr().transportSlots[i].requiredForMovement && (!transportSlots_[i] || !transportSlots_[i]->isDocked())){
				requirmentForMovement = true;
				break;
			}
		if(!requirmentForMovement)
			makeDynamicXY(STATIC_DUE_TO_TRANSPORT_REQUIREMENT);
		else
			makeStaticXY(STATIC_DUE_TO_TRANSPORT_REQUIREMENT);
	}

	if(playerPrev_){
		ParameterSet possessionBack;
		possessionBack.setPossessionRecoveryBack(parameters());
		parameters_.scaleAdd(possessionBack, -logicPeriodSeconds);
		if(parameters().possession() < FLT_EPS)
			changeUnitOwner(playerPrev_);
	}
	else{
		ParameterSet possession;
		possession.setPossessionRecovery(parameters(), parametersMax());
		parameters_.scaleAdd(possession, logicPeriodSeconds);
	}

	executeDirectKeys(directKeysCommand);

	if(isSyndicateControl() && attr().syndicateControlAimEnabled && canForceFire() && unitState() != ATTACK_MODE){
		player()->setUpdateShootPoint();
		setTargetPoint(To3D(player()->shootPoint2D()), true);
	}

	weaponQuant();

	__super::Quant();

	frozenQuant();
	
	weaponPostQuant();

	if(!alive())
		return;

	if(noiseTarget())
		startEffect(&attr().noiseTargetEffect);
	else
		stopEffect(&attr().noiseTargetEffect);

	RealsLinks::iterator dockedUnit(dockedUnits.begin());
	while(dockedUnit != dockedUnits.end()){
		if(UnitReal* unit = *dockedUnit){
			unit->updateDockPose();
			++dockedUnit;
		}
		else
			dockedUnit = dockedUnits.erase(dockedUnit);
	}

	if(attr().upgradeAutomatically && !executedUpgrade() && isConstructed()){
		AttributeBase::Upgrades::const_iterator i;
		FOR_EACH(attr().upgrades, i)
			if(i->second.automatic && upgrade(i->first))
				break;
	}

	if(landToUnloadStart && !rigidBody()->flyDownMode()){
		landToUnloadStart = false;
		putOutTransport(putOutTransportIndex_);
		enableFlying(FLYING_DUE_TO_TRANSPORT_UNLOAD);
	}

	if(currentState() < CHAIN_CONSTRUCTION || currentState() == CHAIN_OPEN  || currentState() == CHAIN_CLOSE){
		if(!attr().productivity.empty()){
			bool enableProduction = true;
			switch(attr().productionRequirement){
				case AttributeBase::PRODUCE_ON_WATER:
					enableProduction = rigidBody()->onWater();
					break;
				case AttributeBase::PRODUCE_ON_TERRAIN:
					enableProduction = !rigidBody()->onWater();
					break;
			}

			if(enableProduction){
				ParameterSet delta = attr().productivity;
				float factor = parameters().findByType(ParameterType::RESOURCE_PRODUCTIVITY_FACTOR, 1);
				float t = fabsf((environment->environmentTime()->GetTime() - 12.f)/12.f);
				factor *= 1.f - t + attr().productionNightFactor*t;
				delta *= factor;
				delta.clamp(productivityRest_);
				productivityRest_.subClamped(delta);
				player()->addResource(delta, true);
				showParamController_.add(delta);
			}
		}

		if(attr().produceParametersAutomatically){
			AttributeBase::ProducedParametersList::const_iterator i;
			FOR_EACH(attr().producedParameters, i)
				if(i->second.automatic && producedQueue_.empty())
					executeCommand(UnitCommand(COMMAND_ID_PRODUCE_PARAMETER, i - attr().producedParameters.begin()));
		}

		if(producedUnit_ && !wasProduced_ && !isFrozen() && productionConsumer_.addQuant(productionConsumer_.delta()))
			shipProduction();

		if(producedParameter_ && !productionParameterTimer_.busy()){
			applyParameterArithmetics(producedParameter_->arithmetics);
			if(!producedParameter_->signalVariable.empty()){
				player()->addUniqueParameter(producedParameter_->signalVariable.c_str(), 1);
				player()->checkEvent(EventParameter(Event::PRODUCED_PARAMETER, producedParameter_->signalVariable.c_str()));
			}
			finishState(StateProduction::instance());
			finishProduction();
		}

		if(callFinishProductionTimer_.finished()){
			callFinishProductionTimer_.stop();
			finishProduction();
		}
	}

	if(rigidBody()->colliding() && impulseAbnormalState_.isEnabled()){
		setAbnormalState(impulseAbnormalState_, impulseOwner_);
		impulseAbnormalState_ = AbnormalStateAttribute();
		impulseOwner_ = 0;
	}

	if(isInvisiblePrev_ != isInvisible())
		dayQuant(isInvisiblePrev_ = isInvisible());
	
	if(isInvisible())
		setColor(universe()->activePlayer()->isEnemy(this)
				? attr().transparenceDiffuseForAlien
				: attr().transparenceDiffuseForClan);
}

bool UnitActing::corpseQuant()
{

	if(!UnitReal::corpseQuant())
		return false;

	if(deathAttr().explodeReference->enableRagDoll){
		rigidBody()->awake();
		return false;
	}

	if(deathAttr().explodeReference->animatedDeath){
		if(deathAttr().explodeReference->alphaDisappear){
			float opacityNew(0.0002f * chainDelayTimer.timeRest());
			if(opacityNew < 1.0f)
				setOpacity(opacityNew);
		}else if(chainDelayTimer.timeRest() < 3000){
			makeStatic(STATIC_DUE_TO_DEATH);
			Se3f nextPose(pose());
			nextPose.trans().z -= height() / 20.0f;
			setPose(nextPose, false);
			rigidBody()->setPose(nextPose);
		}
		return false;
	}

	if(rigidBody()->colliding()) {
		if(rigidBody()->waterColliding() || corpseColisionTimer_ > 7){
			stopEffect(&deathAttr().effectAttrFly);
			if(deathAttr().explodeReference->enableExplode)
				universe()->crashSystem->addCrashModel(deathAttr(), model(), position(), lastContactPoint_, lastContactWeight_, 
				GlobalAttributes::instance().debrisLyingTime, UnitBase::rigidBody()->velocity());
			return true;
		}
		++corpseColisionTimer_;
		rigidBody()->awake();
		if(rotationDeath){
			rotationDeath = 0;
			rigidBody()->setGravityZ(-rigidBody()->prm().gravity);
		}
		return false;
	}
	rigidBody()->awake();
	if(rotationDeath != 0){
		Vect3f angularVelocity;
		if(rigidBody()->prm().alwaysMoving){
			angularVelocity.set(0.0f, rotationDeath, 0.0f);
			pose().xformVect(angularVelocity);
		}else{
			angularVelocity.set(0.0f, 0.0f, rotationDeath);
		}
		rigidBody()->setAngularVelocity(angularVelocity);
	}
	return false;
}

MovementState UnitActing::getMovementState()
{
	MovementState state;

	// ¬ыставл€ем признаки поверхностей.
	if(rigidBody()->onDeepWater())
		if(water->isLava())
			state.terrainType() |= ANIMATION_ON_LAVA;
		else
			state.terrainType() |= ANIMATION_ON_WATER;
	else if(rigidBody()->onLowWater())
		state.terrainType() |= ANIMATION_ON_LOW_WATER;
	else
		state.terrainType() |= 1 << pathFinder->getTerrainType(position().xi(), position().yi());

	return state;
}

void UnitActing::shipProduction()
{
	if(shippedSquad_)
		shippedSquad_->reserveUnitNumber(producedUnit_, -1);
	player()->reserveUnitNumber(producedUnit_, -1);
	shippedUnit_ = safe_cast<UnitLegionary*>(player()->buildUnit(producedUnit_));
	wasProduced_ = true;
	universe()->checkEvent(EventUnitPlayer(!teleportProduction_ ? Event::CREATE_OBJECT : Event::PRODUCE_UNIT_IN_SQUAD, shippedUnit_, player()));
	shippedUnit_->setAttackMode(attackMode());
	shippedUnit_->toggleAutoFindTransport(autoFindTransport());
	if(squadReserved_){
		player()->reserveUnitNumber(safe_cast<const AttributeLegionary*>(producedUnit_)->squad, -1);
		squadReserved_ = false;
	}
	if(!teleportProduction_){
		xassert(attr().dockNodes.size() > producedUnit_->dockNodeNumber);
		shippedUnit_->changeState(StateBirth::instance());
		shippedUnit_->attachToDock(this, attr().dockNodes[producedUnit_->dockNodeNumber], true);
		if(shippedUnit_->attr().squad && !shippedSquad_) {
			shippedSquad_ = safe_cast<UnitSquad*>(player()->buildUnit(&*shippedUnit_->attr().squad));
			shippedSquad_->reserveUnitNumber(producedUnit_, productionCounter_ - 1);
            shippedSquad_->setPose(shippedUnit_->pose(), true);
			shipmentPosition_ = getFreePosition(shipmentPosition_);
			shippedSquad_->addUnit(shippedUnit_);
			shippedSquad_->addWayPointS(shipmentPosition());
		}
		else{
			shippedSquad_->addUnit(shippedUnit_);
			shippedUnit_->updateWayPointsForNewUnit(shippedSquad_->getUnitReal());
		}
		if(shippedSquad_){
			shippedSquad_->setLocked(true);
			if(resourceItemForProducedUnit_ &&
				resourceItemForProducedUnit_->attr().isResourceItem() && shippedSquad_->canExtractResource(safe_cast<const UnitItemResource*>(&*resourceItemForProducedUnit_))){
				UnitCommand command(COMMAND_ID_OBJECT, resourceItemForProducedUnit_, Vect3f::ZERO, 0);
				shippedSquad_->executeCommand(command);
			}
		}
		finishState(StateProduction::instance());
		startState(StateOpen::instance());
	}
	else{
		if(!shippedSquad_)
			shippedUnit_->Kill();
		else{
			shippedUnit_->changeState(StateBirth::instance());
			shippedUnit_->startChainTimer(attr().chainBirthTime);
			shippedSquad_->addRequestedUnit(this, shippedUnit_);
			shippedUnit_->setPose(shippedSquad_->getNewUnitPosition(shippedUnit_), true);
			shippedUnit_->updateWayPointsForNewUnit(shippedSquad_->getUnitReal());
		}
		shippedSquad_ = 0;
        xassert(productionCounter_ == 1);
		finishProduction();
	}
	--productionCounter_;
}

bool UnitActing::producedAllParameters() const
{
	AttributeBase::ProducedParametersList::const_iterator i;
	FOR_EACH(attr().producedParameters, i)
		if(!player()->checkUniqueParameter(i->second.signalVariable.c_str(), 1))
			return false;

	return true;
}

Accessibility UnitActing::canProduceParameter(int number) const
{
	if(!attr().producedParameters.exists(number))
		return DISABLED;

	const ProducedParameters& prm = attr().producedParameters[number];

	if(!requestResource(prm.accessValue, NEED_RESOURCE_SILENT_CHECK))
		return DISABLED;

	if(player()->checkUniqueParameter(prm.signalVariable.c_str())){
		ProducedQueue::const_iterator it;
		FOR_EACH(producedQueue_, it)
			if(it->type_ == PRODUCE_RESOURCE && it->data_ == number)
				return ACCESSIBLE;
		return DISABLED;
	}
	
	if(currentState() > CHAIN_PRODUCTION && currentState() != CHAIN_OPEN  && currentState() != CHAIN_CLOSE)
		return ACCESSIBLE; 
	
	if(producedQueue_.size() >= attr().producedUnitQueueSize)
		return ACCESSIBLE;

	if(!requestResource(prm.cost, NEED_RESOURCE_SILENT_CHECK))
		return ACCESSIBLE;

	if(!player()->accessibleByBuildings(prm))
		return ACCESSIBLE;

	return CAN_START;
}

Accessibility UnitActing::canProduction(int number) const
{
	if(!attr().producedUnits.exists(number))
		return DISABLED;
	
	const AttributeBase* requestedUnit = attr().producedUnits[number].unit;

	if(!requestedUnit)
		return DISABLED;

	if(!requestResource(requestedUnit->accessValue, NEED_RESOURCE_SILENT_CHECK))
		return DISABLED;

	if(!requestResource(attr().producedUnits[number].accessValue, NEED_RESOURCE_SILENT_CHECK))
		return DISABLED;

	if(currentState() > CHAIN_PRODUCTION && currentState() != CHAIN_OPEN  && currentState() != CHAIN_CLOSE)
		return ACCESSIBLE;

	if(!player()->checkUnitNumber(requestedUnit))
		return ACCESSIBLE;

	if(producedQueue_.size() >= attr().producedUnitQueueSize)
		return ACCESSIBLE;

	ParameterSet cost = requestedUnit->installValue;
	cost *= attr().producedUnits[number].number;
	if(!requestResource(cost, NEED_RESOURCE_SILENT_CHECK))
		return ACCESSIBLE;

	if(!player()->accessibleByBuildings(requestedUnit))
		return ACCESSIBLE;

	return CAN_START;
}

bool UnitActing::startProduction(const AttributeBase* producedUnit, UnitSquad* shippedSquad, int counter, bool restartFromQueue)
{
	//xassert(!producedUnit_);

	if(currentState() > CHAIN_PRODUCTION && currentState() != CHAIN_OPEN  && currentState() != CHAIN_CLOSE)
		return false;

	int freeUnits = player()->checkUnitNumber(producedUnit);
	if(!shippedSquad){
		if(freeUnits > 0 && producedUnit->isLegionary()){
			int freePlaces = player()->unitNumberMaxReal(producedUnit->unitNumberMaxType);
			int residue = freePlaces % freeUnits;
			if(producedUnit->accountingNumber)
				freeUnits += (residue + (freeUnits - 1)*safe_cast<const AttributeLegionary*>(producedUnit)->squad->accountingNumber) / producedUnit->accountingNumber;
			else
				freeUnits = 100;
		}
	}
	else
		if(producedUnit->accountingNumber)
			freeUnits += freeUnits*safe_cast<const AttributeLegionary*>(producedUnit)->squad->accountingNumber / producedUnit->accountingNumber; 
		else
			freeUnits = 100;

	counter = min(counter, freeUnits);

	if(!counter){
		if(player()->active())
			UI_Dispatcher::instance().sendMessage(UI_MESSAGE_UNIT_LIMIT_REACHED);
		return false;
	}

	if(!player()->accessible(producedUnit))
		return false;

	if(!restartFromQueue){
		if(producedQueue_.size() >= attr().producedUnitQueueSize)
			return false;
		
		ParameterSet orderValue = producedUnit->installValue;
		orderValue *= counter;
		if(!requestResource(orderValue, NEED_RESOURCE_TO_PRODUCE_UNIT)){
			if(player()->active())
				UI_Dispatcher::instance().sendMessage(UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_BUILDING);
			return false;
		}
		subResource(orderValue);

		AttributeBase::ProducedUnitsVector::const_iterator i;
		FOR_EACH(attr().producedUnits, i)
			if(i->second.unit == producedUnit)
				break;

		producedQueue_.push_back(ProduceItem(PRODUCE_UNIT, i->first));

		if(isProducing() || producedQueue_.size() > 1)
			return true;
	}

	if(!shippedSquad && producedUnit->isLegionary()){
		player()->reserveUnitNumber(safe_cast<const AttributeLegionary*>(producedUnit)->squad, 1);
		squadReserved_ = true;
	}
	shippedSquad_ = shippedSquad;
	teleportProduction_ = shippedSquad != 0;
	producedUnit_ = producedUnit;
	player()->reserveUnitNumber(producedUnit_, counter);
	if(shippedSquad_)
		shippedSquad_->reserveUnitNumber(producedUnit_, counter);
	productionCounter_ = counter;
	ParameterSet value = producedUnit_->creationValue;
	value *= productionCounter_;
	productionConsumer_.start(this, value, producedUnit_->creationTime*productionCounter_);
	player()->checkEvent(EventUnitAttributePlayer(Event::STARTED_PRODUCTION, producedUnit_, player()));
	makeStaticXY(STATIC_DUE_TO_PRODUCTION);
	return true;
}

void UnitActing::finishProduction()
{
	if(teleportProduction_ && shippedSquad_)
		shippedSquad_->cancelRequestedUnitProduction(this);
	finishState(StateOpen::instance());
	if(producedUnit_ && squadReserved_){
		player()->reserveUnitNumber(safe_cast<const AttributeLegionary*>(producedUnit_)->squad, -1);
		squadReserved_ = false;
	}
	shippedUnit_ = 0;
	wasProduced_ = false;
	if(shippedSquad_ && shippedSquad_->locked())
		shippedSquad_->setLocked(false);
	shippedSquad_ = 0;
	producedUnit_ = 0;
	productionCounter_ = 0;
	productionConsumer_.clear();
	producedParameter_ = 0;
	makeDynamicXY(STATIC_DUE_TO_PRODUCTION);
	callFinishProductionTimer_.stop();

	if(totalProductionCounter_ && !--totalProductionCounter_)
		Kill();

	if(!producedQueue_.empty()){
		producedQueue_.erase(producedQueue_.begin());
		if(!producedQueue_.empty()){
			ProduceItem item = producedQueue_.front();
			switch(item.type_) {
			case PRODUCE_INVALID:
				finishProduction();
				break;
			case PRODUCE_UNIT:
				if(!startProduction(attr().producedUnits[item.data_].unit, 0, attr().producedUnits[item.data_].number, true)){
					producedQueue_.insert(producedQueue_.begin(), ProduceItem());
					callFinishProductionTimer_.start(5000);
				}
				break;
			case PRODUCE_RESOURCE:
				startProductionParamater(attr().producedParameters[item.data_]);
				break;
			}
		}
	}
}

void UnitActing::startProductionParamater(const ProducedParameters& prm)
{
	productionParameterTimer_.start(prm.time*1000);
	producedParameter_ = &prm;
	player()->checkEvent(EventUnitAttributePlayer(Event::STARTED_PRODUCTION_PARAMETER, &attr(), player()));
}

class ObstacleCheck {
public:
	vector<UnitBase*> unitList_;

	void operator () (UnitBase* p) {
		if(p->attr().isBuilding() || (p->attr().isLegionary() && (safe_cast<UnitLegionary*>(p)->getStaticReasonXY() & UnitActing::STATIC_DUE_TO_UPGRADE)))
			unitList_.push_back(p);
	}

	bool check(Vect2f& point) {
		vector<UnitBase*>::iterator it;
		FOR_EACH(unitList_, it)
			if(point.distance2((*it)->position2D()) < sqr((*it)->radius() + 12.0f))
				return false;

		return true;
	}
};

Vect2f UnitActing::getFreePosition(const Vect2f& point) const
{
	PositionGeneratorSquad generator;
	generator.init(12.0f);
	
	ObstacleCheck oc;
	universe()->unitGrid.Scan(point.xi(), point.yi(), 150, oc);

	for(int i = 0; i < 50; i++) {
		Vect2f pointNew = point + generator.get();
		if(!pathFinder->checkImpassability(pointNew.xi(), pointNew.yi(), impassability()) && oc.check(pointNew))
			return pointNew;
	}

	return point;
}

void UnitActing::setPose(const Se3f& poseIn, bool initPose)
{
	__super::setPose(poseIn, initPose);

	if(initPose){
		if(currentState() != CHAIN_BIRTH_IN_AIR)
			for(int i = 0; i < transportSlots_.size(); i++)
				setAdditionalChain(CHAIN_SLOT_IS_EMPTY, i);
		if(shipmentPosition_.yi() < 0){
			Vect3f shipmentPosition(0.0f, 50.0f, 0.0f);
			pose().xformPoint(shipmentPosition);
			shipmentPosition_ = getFreePosition(shipmentPosition);
		}
	}
}

float UnitActing::angleZ() const
{
	return rigidBody()->angle();
}

bool UnitActing::checkInPathTracking(const UnitBase* tracker) const
{
	if(tracker->attr().isSquad())
		return attr().enablePathTracking;

	if(!tracker->rigidBody())
		return false;

	if(tracker->attr().isActing()) {
		if(tracker->radius() > radius())
			return false;

		if(rigidBody()->flyingMode() && safe_cast<const UnitActing*>(tracker)->rigidBody()->flyingMode()) {
			if(rigidBody()->prm().hoverMode != tracker->rigidBody()->prm().hoverMode)
				return false;
			return attr().enablePathTracking;
		}
		if(!rigidBody()->flyingMode() && !safe_cast<const UnitActing*>(tracker)->rigidBody()->flyingMode())
			return attr().enablePathTracking;
		return false;
	}
	if(!rigidBody()->flyingMode())
		return attr().enablePathTracking;
	return false;
}

void UnitActing::enableFlying(int flyingReason)
{
	flyingReason_ &= ~flyingReason;
	if(!flyingReason_)
		rigidBody()->setFlyingModeEnabled(rigidBody()->prm().flyingMode);
}

void UnitActing::disableFlying(int flyingReason, int time)
{
	flyingReason_ |= flyingReason;
	rigidBody()->setFlyingModeEnabled(false, time);
}

void UnitActing::makeStatic(int staticReason) 
{ 
	staticReason_ |= staticReason; 
	rigidBody()->makeStatic();
}

void UnitActing::makeDynamic(int staticReason) 
{ 
	staticReason_ &= ~staticReason; 
	if(!staticReason_ && rigidBody()->unmovable()){
		if(getSquadPoint())
			getSquadPoint()->updatePose(true);
		rigidBody()->makeDynamic();
	}
}

const UI_MinimapSymbol* UnitActing::minimapSymbol(bool permanent) const
{
	if(specialMinimapSymbolActivated_)
		return &attr().minimapSymbolSpecial_;

	if(!permanent && (getStaticReasonXY() & STATIC_DUE_TO_WAITING)){
		switch(attr().minimapSymbolType_){
		case UI_MINIMAP_SYMBOLTYPE_DEFAULT:
			return &player()->race()->minimapMark(UI_MINIMAP_SYMBOL_UNIT_WAITING);
		case UI_MINIMAP_SYMBOLTYPE_SELF:

			return &attr().minimapSymbolWaiting_;
		}
	}

	return __super::minimapSymbol(permanent);
}

const struct UI_ShowModeSprite* UnitActing::getSelectSprite() const
{
	if(getStaticReasonXY() & STATIC_DUE_TO_WAITING)
		return attr().getSelectSprite(AttributeBase::UI_STATE_TYPE_WAITING);
	return __super::getSelectSprite();
}

void UnitActing::showDebugInfo()
{
	__super::showDebugInfo();

	if(showDebugUnitBase.lodDistance_){
		XBuffer buf;
		float radius;
		if(rigidBody()){
			const Vect3f& bound = rigidBody()->extent();
			radius = (bound.x+bound.y+bound.z)/3;
		}else{
			sBox6f bound;
			model()->GetBoundBox(bound);
			Vect3f extent(bound.max);
			extent -= bound.min;
			extent *= 0.5f;
			radius = (extent.x+extent.y+extent.z)/3;
		}

		buf <= numLodDistance < "(" <= radius <")";
		show_text(position(), buf, Color4c::RED);
	}

	if(showDebugUnitReal.attackModes){
		XBuffer buf(1028,1);
		buf < getEnumNameAlt(unitState()) < "\n";
		buf < getEnumNameAlt(autoAttackMode()) < "\n";
		buf < getEnumNameAlt(walkAttackMode()) < "\n";
		buf < getEnumNameAlt(weaponMode()) < "\n";
		buf < getEnumNameAlt(autoTargetFilter());
		show_text(position(), buf, Color4c::GREEN);
	}

	if(showDebugUnitReal.directControlWeapon){
		XBuffer buf;
		buf < "directControWeaponID: " <= directControlWeaponID();
		show_text(position(), buf, Color4c::GREEN);
	}

	if(showDebugUnitReal.hearingRadius)
		show_vector(position(), hearingRadius(), Color4c::YELLOW);
	if(showDebugUnitReal.noiseRadius)
		show_vector(position(), noiseRadius(), Color4c::GREEN);
	if(showDebugUnitReal.noiseTarget && noiseTarget_){
		show_line(position(), noiseTarget_->position(), Color4c::GREEN);
		show_vector(noiseTarget_->position(), 5.f, Color4c::GREEN);
	}

	if(showDebugUnitReal.sightSector && !isEq(attr().sightSector, 2*M_PI, 0.01f)){
		float len = sightRadius();
		Color4c col = Color4c::GREEN; 
		Vect3f dir(len * sinf(-attr().sightSector/2.f), len * cosf(-attr().sightSector/2.f), 0);
		dir = pose().xformVect(dir);
		show_vector(position(), dir, col);

		dir = Vect3f(len * sinf(attr().sightSector/2.f), len * cosf(attr().sightSector/2.f), 0);
		dir = pose().xformVect(dir);
		show_vector(position(), dir, col);
	}

	if(showDebugUnitReal.target && targetUnit_)
		show_line(position(), targetUnit_->position(), Color4c::RED);

	if(showDebugWeapon.enable){
		WeaponSlots::iterator it;
		FOR_EACH(weaponSlots_, it)
			it->weapon()->showDebugInfo();

		if(showDebugWeapon.parameters){
			XBuffer buf(2048,1);
			buf < "WeaponParameters:\n";
			FOR_EACH(weaponSlots_, it){
				buf < it->weapon()->parameters().debugStr() < "\n";
			}

			show_text(position(), buf, Color4c::GREEN);
		}

		if(showDebugWeapon.damage){
			XBuffer buf(2048,1);
			buf < "Weapon damage:\n";
			FOR_EACH(weaponSlots_, it){
				buf < it->weapon()->damage().debugStr() < "\n";
			}

			show_text(position(), buf, Color4c::GREEN);
		}
	}

	if(showDebugUnitReal.producedUnitQueue){
		XBuffer msg;
		msg < "producedQueue: " <= producedQueue_.size();
		show_text(position(), msg, Color4c::GREEN);
	}

	if(showDebugUnitReal.transportSlots && !transportSlots_.empty()){
		XBuffer msg(256, 1);
		msg < "transportSlots: ";
		LegionariesLinks::const_iterator i;
		FOR_EACH(transportSlots_, i)
			msg <= (*i ? (*i)->unitID().index() : 0) < " ";
		show_text(position(), msg, Color4c::GREEN);
	}

	if(showDebugUnitReal.parametersParts){
		XBuffer msg;
		BodyParts::iterator bi;
		FOR_EACH(bodyParts_, bi)
			msg < bi->parameters.debugStr() < "\n";
		show_text(position(), msg, Color4c::GREEN);
	}
}

float UnitActing::productionParameterProgress() const
{
	if(!producedParameter_)
		return 0;
	return 1.f - (float)productionParameterTimer_.timeRest()/(float)(1000*producedParameter_->time);
}

void UnitActing::changeUnitOwner(Player* playerIn)
{
	interuptState(StateUpgrade::instance());

	setDirectControl(DIRECT_CONTROL_DISABLED);

	cancelProduction();

	if(!isUnderEditor() && !playerPrev_ && parameters_.restorePossessionRecoveryBack(parametersMax()))
		playerPrev_ = player();
	else 
		playerPrev_ = 0;

	parameters_.set(0, ParameterType::POSSESSION);
	ParameterSet possession = parametersMax();
	possession.mask(ParameterType::POSSESSION);
	parameters_ += possession;

	LegionariesLinks::iterator ui;
	FOR_EACH(transportSlots_, ui)
		if(UnitLegionary* transportSlot = *ui)
			if(transportSlot->player() != playerIn)
				transportSlot->clearTransport();
	
	stop();

	__super::changeUnitOwner(playerIn);

	FOR_EACH(transportSlots_, ui)
		if(UnitLegionary* transportSlot = *ui)
			transportSlot->updateTransport(this);

	RealsLinks::iterator dockedUnit(dockedUnits.begin());
	while(dockedUnit != dockedUnits.end()){
		if(UnitReal* unit = *dockedUnit){
			unit->updateDock(this);
			++dockedUnit;
		}
		else
			dockedUnit = dockedUnits.erase(dockedUnit);
	}

	clearAttackTarget();

	rigidBody()->setFieldFlag(1 << (20 + player()->playerID()));
}

class ClearRegionOp {
	Vect2f position_;
	float radius_;
public:

	ClearRegionOp(Vect2f& position, float radius):position_(position),radius_(radius) {}

	void operator () (UnitBase* p) 
	{
		if(p->attr().isLegionary())
			safe_cast<UnitLegionary*>(p)->formationUnit_.resolvePenetration(position_, radius_);
	}
};

void UnitActing::clearRegion(Vect2i position, float radius)
{
	ClearRegionOp clearOp(Vect2f(position), radius);
	universe()->unitGrid.Scan(position.x, position.y, round(radius), clearOp);
}

void UnitActing::setImpulseAbnormalState(const AbnormalStateAttribute& state, UnitBase* owner)
{
	impulseAbnormalState_ = state;
	impulseOwner_ = owner;
}

//=======================================================
void UnitActing::attackQuant()
{
	start_timer_auto();

	if(isUnderEditor())	return;

//  if(weapons().empty()) return;

	if(isFrozen())
		return;

	bool need_scan = false;


	if(targetUnit())
		hadTargetUnit_ = true;
	else if(hadTargetUnit_){
		need_scan = true;
		hadTargetUnit_ = false;
	}

	int targetID = targetUnit() ? targetUnit()->unitID().index() : 0;

	if(!aiScanTimer.busy() || need_scan){
		aiScanTimer.start(300 + logicRND(1500));
		targetController();
	}

	if(!noiseScanTimer_.busy()){
		noiseScanTimer_.start(300 + logicRND(1500));
		noiseTargetController();
	}
}

void UnitActing::targetController()
{
	switch(unitState()) {
	case AUTO_MODE:
		if(!isDefaultWeaponExist())
			break;

		if(canAutoAttack() && autoAttackMode() != ATTACK_MODE_DISABLE){
			MicroAI_Scaner aiScaner(this, autoAttackMode());
			universe()->unitGrid.Scan(position().xi(), position().yi(), max(sightRadius(), autoFireRadius()), aiScaner);
			targetUnit_ = aiScaner.processTargets();
		}
		else {
			targetUnit_ = 0;
			clearAutoAttackTargets();
		}

		break;

	case ATTACK_MODE:
		// ≈сли убили указанную цель - работаем в авто режиме.
		if(!fireTargetExist() || (targetUnit_ && !canAttackTarget(WeaponTarget(targetUnit_)))){
			setUnitState(AUTO_MODE);
			wayPointsClear();
			clearAttackTarget();

			if(canAutoAttack() && autoAttackMode() != ATTACK_MODE_DISABLE){
				MicroAI_Scaner aiScaner(this, autoAttackMode());
				universe()->unitGrid.Scan(position().xi(), position().yi(), max(sightRadius(), autoFireRadius()), aiScaner);
				targetUnit_ = aiScaner.processTargets();
			}
		}
		else {
			if(canAutoAttack() && autoAttackMode() != ATTACK_MODE_DISABLE){
				MicroAI_Scaner aiScaner(this, autoAttackMode());
				universe()->unitGrid.Scan(position().xi(), position().yi(), max(sightRadius(), autoFireRadius()), aiScaner);
				aiScaner.processTargets();
			}
			else
				clearAutoAttackTargets();
		}

		break;
			
	case MOVE_MODE:
		if(canAutoAttack()){
			if(autoAttackMode() != ATTACK_MODE_DISABLE){
				MicroAI_Scaner aiScaner(this, autoAttackMode());
				universe()->unitGrid.Scan(position().xi(), position().yi(), max(sightRadius(), autoFireRadius()), aiScaner);
				targetUnit_ = aiScaner.processTargets();
			}
			else {
				targetUnit_ = 0;
				clearAutoAttackTargets();
			}
		}
		else {
			targetUnit_ = 0;
			clearAutoAttackTargets();
		}

		break;
	}
}

void UnitActing::noiseTargetController()
{
	if(unitState() == AUTO_MODE && hearingRadius() > FLT_EPS){
		MicroAI_NoiseScaner noise_scaner(this);
		universe()->unitGrid.Scan(position().xi(), position().yi(), hearingRadius(), noise_scaner);
		noiseTarget_ = noise_scaner.target();
	}
}

bool UnitActing::canPutInTransport(const UnitBase* unit) const
{
	if(currentState() >= CHAIN_PRODUCTION)
		return false;

	if(unit->attr().isSquad()){
		const UnitSquad* squad = static_cast<const UnitSquad*>(unit);
		const LegionariesLinks& units = squad->units();
		LegionariesLinks::const_iterator ui;
		FOR_EACH(units, ui){
			if(canPutInTransport(*ui))
				return true;
		}
		return false;
	}
	else if(unit->attr().isLegionary()){
		const UnitLegionary* legioner = safe_cast<const UnitLegionary*>(unit);
		int counter = legioner->attr().transportSlotsRequired;
		for(int i = 0; i < transportSlots_.size(); i++)
			if(!transportSlots_[i] && attr().transportSlots[i].check(legioner->attr().formationType, legioner->parameters()) && !--counter)
				return true;
		return false;
	}
	else
		return false;
}

void UnitActing::putUnitInTransport(UnitLegionary* unit)
{
	int counter = unit->attr().transportSlotsRequired;
	for(int i = 0; i < transportSlots_.size(); i++){
		if(!transportSlots_[i] && attr().transportSlots[i].check(unit->attr().formationType, unit->parameters())){
			transportSlots_[i] = unit;
			if(counter == unit->attr().transportSlotsRequired){
				unit->setTransport(this, i); 
				if(!(currentState() & CHAIN_TRANSPORT) || currentState() == CHAIN_CLOSE_FOR_LANDING){
					if(rigidBody()->prm().flyingMode)
						startState(StateMoveToCargo::instance());
					else
						startState(StateOpenForLanding::instance());
				}
			}
			if(!--counter)
				break;
		}
	}
}

bool UnitActing::putInTransport(UnitSquad* squad)
{
	if(!canPutInTransport(squad))
		return false;

	squad->clearOrders();

	bool partlyLoaded = false;
	const LegionariesLinks& units = squad->units();
	LegionariesLinks::const_iterator ui;
	FOR_EACH(units, ui){
		if(!canPutInTransport(*ui)){
			partlyLoaded = true;
			continue;
		}
		putUnitInTransport(*ui);
	}

	if(partlyLoaded){
		UnitSquad* squadIn = safe_cast<UnitSquad*>(player()->buildUnit(&squad->attr()));
		squadIn->setPose(pose(), true);
		for(;;){
			const LegionariesLinks& units = squad->units();
			LegionariesLinks::const_iterator ui;
			FOR_EACH(units, ui){
				UnitLegionary* unit = *ui;
				if(unit->isMovingToTransport()){
					squad->removeUnit(unit);
					squadIn->addUnit(unit);
					break;
				}
			}
			if(ui == units.end())
				break;
		}
	}
	return true;
}

bool UnitActing::waitingForPassenger() const
{
	LegionariesLinks::const_iterator i;
	FOR_EACH(transportSlots_, i)
		if(*i && !(*i)->isDocked())
			return true;
	return false;
}

void UnitActing::putOutTransport(int index)
{
	if(index == -1){
		LegionariesLinks::iterator ui;
		FOR_EACH(transportSlots_, ui)
			if(*ui)
				(*ui)->clearTransport();
	}
	else if(index < transportSlots_.size() && transportSlots_[index])
		transportSlots_[index]->clearTransport();
}

int UnitActing::findMainUnitInTransport()
{
	int priority = -1;
	LegionariesLinks::iterator ui, mainUnit = transportSlots_.end();
	FOR_EACH(transportSlots_, ui){
		if(*ui && (*ui)->isDocked()){
			if((*ui)->directControlPrev()){
				mainUnit = ui;
				break;
			}
			if((*ui)->selectionListPriority() > priority){
				priority = (*ui)->selectionListPriority();
				mainUnit = ui;
			}
		}
	}
	if(mainUnit != transportSlots_.end()){
		if(!(*mainUnit)->directControlPrev())
			(*mainUnit)->setDirectControlPrev(true);
		return mainUnit - transportSlots_.begin();
	}
	return -1;
}

void UnitActing::detachFromTransportSlot(int index, int slotsNumber) 
{ 
	UnitLink<UnitLegionary> unit(transportSlots_[index]);
	if(unit && unit->inTransport()){
		float angle = 2 * M_PI * index / attr().transportSlots.size();
		Vect2f delta(sinf(angle), cosf(angle));
		delta *= (unit->radius() + radius()) / (delta.norm() + 0.01f);
		Se3f unitPose(pose());
		unitPose.trans().x += delta.x;
		unitPose.trans().y += delta.y;
		unit->setPose(unitPose, true);
	}
	if(cargo_ == unit)
		cargo_ = 0;
	for(int i = index; i < index + slotsNumber; ++i){
		transportSlots_[i] = 0; 
		stopAdditionalChain(CHAIN_CARGO_LOADED, i);
		setAdditionalChain(CHAIN_SLOT_IS_EMPTY, i);
	}
}

void UnitActing::resetCargo() 
{ 
	cargo_ = 0; 
}

bool UnitActing::transportEmpty() const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(transportSlots_, ui)
		if(*ui && (*ui)->inTransport())
			return false;
	return true;
}

bool UnitActing::isInTransport(const AttributeBase* attribute) const
{
	LegionariesLinks::const_iterator ui;
	FOR_EACH(transportSlots_, ui){
		if(*ui && (*ui)->inTransport() && &(*ui)->attr() == attribute)
			return true;
	}

	return false;
}

bool UnitActing::traceFireTarget() const
{
	if(!fireTargetExist())
		return false;

	bool need_ground_trace = true;
	bool need_units_trace = true;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		if(WeaponBase* p = it->weapon()){
			const WeaponPrm* prm = p->weaponPrm();
			if((!prm->needSurfaceTrace() || !prm->needUnitTrace()) && !prm->disableOwnerMove() && p->canAttack(fireTarget())){
				if(!prm->needSurfaceTrace())
					need_ground_trace = false;
				if(!prm->needUnitTrace())
					need_units_trace = false;
			}
		}
	}

	Vect3f start_pos = position();
	start_pos.z += height()/5.f;
	Vect3f pos;

	if(need_ground_trace){
		if(!weapon_helpers::traceGround(start_pos, firePosition(), pos))
			return false;
	}

	if(need_units_trace){
		if(!weapon_helpers::traceUnit(start_pos, firePosition(), pos, this, targetUnit_))
			return false;
	}

	return true;
}

void UnitActing::cancelUnitProduction(ProduceItem& item)
{
	switch(item.type_){
		case PRODUCE_UNIT: {
			const AttributeBase::ProducedUnits& producedUnits = attr().producedUnits[item.data_];
			ParameterSet cost = producedUnits.unit->installValue;
			cost *= producedUnits.number;
			player()->addResource(cost);
			break;
		} 
		case PRODUCE_RESOURCE: {
			const ProducedParameters& prm = attr().producedParameters[item.data_];
			player()->addResource(prm.cost);
			player()->removeUniqueParameter(prm.signalVariable.c_str());
			break;
	   }
	}
}

void UnitActing::cancelProduction()
{
	if(teleportProduction_ && shippedSquad_)
		shippedSquad_->cancelRequestedUnitProduction(this);

	if(producedUnit_){
		player()->reserveUnitNumber(producedUnit_, -productionCounter_);
		if(shippedSquad_)
			shippedSquad_->reserveUnitNumber(producedUnit_, -productionCounter_);

		if(squadReserved_){
			player()->reserveUnitNumber(safe_cast<const AttributeLegionary*>(producedUnit_)->squad, -1);
			squadReserved_ = false;
		}

		producedUnit_ = 0;
	}
	interuptState(StateOpen::instance());
	shippedUnit_ = 0;
	wasProduced_ = false;
	shippedSquad_ = 0;
	productionCounter_ = 0;
	productionConsumer_.clear();
	producedParameter_ = 0;
	makeDynamicXY(STATIC_DUE_TO_PRODUCTION);

	ProducedQueue::iterator item;
	FOR_EACH(producedQueue_, item)
		cancelUnitProduction(*item);
	producedQueue_.clear();
}

Accessibility UnitActing::canUpgrade(int upgradeNumber, RequestResourceType triggerAction) const
{
	if(!attr().upgrades.exists(upgradeNumber))
		return DISABLED;

	const AttributeBase::Upgrade& upgrade = attr().upgrades[upgradeNumber];
	if(!parameters().above(upgrade.accessParameters))
		return DISABLED;

	const AttributeBase* attribute = upgrade.upgrade;

	if(!requestResource(attribute->accessValue, triggerAction))
		return DISABLED;

	if(currentState() > CHAIN_PRODUCTION && currentState() != CHAIN_OPEN  && currentState() != CHAIN_CLOSE)
		if(isUpgrading())
			return ACCESSIBLE;
		else
			return DISABLED;

	if(attr().isLegionary() && safe_cast<const UnitLegionary*>(this)->formationUnit_.penetrationFound())
		return ACCESSIBLE;

	if(!player()->checkUnitNumber(attribute, &attr()))
		return ACCESSIBLE;

	if(!player()->accessibleByBuildings(attribute))
		return ACCESSIBLE;

	if(!requestResource(attr().upgrades[upgradeNumber].upgradeValue, triggerAction))
		return ACCESSIBLE;

	return CAN_START;
}

bool UnitActing::upgrade(int upgradeNumber)
{
	if(canUpgrade(upgradeNumber, NEED_RESOURCE_TO_UPGRADE) != CAN_START)
		return false;

	subResource(attr().upgrades[upgradeNumber].upgradeValue);

	executedUpgradeIndex_ = upgradeNumber;
	reserveSquadNumber(executedUpgrade(), 1);
	upgradeTime_ = attr().upgrades[executedUpgradeIndex_].chainUpgradeTime;
	if(!executedUpgrade()->rigidBodyPrm->flyingMode || executedUpgrade()->rigidBodyPrm->hoverMode)
		disableFlying(FLYING_DUE_TO_UPGRADE, upgradeTime_);
	if(executedUpgrade()->isBuilding() && !attr().upgrades[executedUpgradeIndex_].built)
		finishUpgradeTime_ = int(executedUpgrade()->creationTime * 1000.0f);
	else
		finishUpgradeTime_ = attr().isLegionary() ? 
			executedUpgrade()->chainUpgradedFromLegionary : executedUpgrade()->chainUpgradedFromBuilding;
		
	upgradeTime_ += finishUpgradeTime_;
	setManualTarget(NULL);
	fireStop();
	cancelProduction();
	startState(StateUpgrade::instance());
	if(attr().upgradeStop){
		makeStaticXY(STATIC_DUE_TO_UPGRADE);
	}
	player()->checkEvent(EventUnitUnitAttributePlayer(Event::STARTED_UPGRADE, attr().upgrades[upgradeNumber].upgrade, this, player()));
	return true;
}

float UnitActing::upgradeProgres(int number, bool forFinishPhase) const
{
	if((number == -1) == ((forFinishPhase ? previousUpgradeIndex_ : executedUpgradeIndex_) == number))
		return 0.0f;

	int time;
	if(forFinishPhase && attr().isBuilding() && !(currentState() & CHAIN_IS_UPGRADED))
		time = int((1.0f - safe_cast<const UnitBuilding*>(this)->constructionProgress()) * finishUpgradeTime_);
	else{
		time = chainDelayTimer.timeRest();
		if(!forFinishPhase)
			time += finishUpgradeTime_;
	}
	
	return upgradeTime_ ? clamp(1.0f - float(time) / float(upgradeTime_), 0.0f, 1.0f) : 1.0f;
}

void UnitActing::setUpgradeProgresParameters(int index, int time, int finishTime)
{
	previousUpgradeIndex_ = index;
	upgradeTime_ = time;
	finishUpgradeTime_ = finishTime;
}

void UnitActing::finishUpgrade()
{
	if(!executedUpgrade())
		return;

	reserveSquadNumber(executedUpgrade(), -1);
	UnitActing* unit = safe_cast<UnitActing*>(player()->buildUnit(executedUpgrade()));
	setRegisteredInPlayerStatistics(false);
	if(!transportSlots_.empty()){
		if(isDirectControl())
			findMainUnitInTransport();

		for(int i = 0; i < transportSlots_.size(); ++i){
			UnitLegionary* unitCargo = transportSlots_[i];
			if(unitCargo && !attr().transportSlots[i].destroyWithTransport){
				unitCargo->clearTransport();
				if(unit->canPutInTransport(unitCargo)){
					unit->putUnitInTransport(unitCargo);
					unitCargo->startState(StateInTransport::instance());
				}
			}
		}
	}

	if(unit->attr().inheritHealthArmor){
		ParameterSet prm = unit->parameters();
		prm.scaleType(ParameterType::HEALTH, parameters().health()/parametersMax().health());
		prm.scaleType(ParameterType::ARMOR, parameters().armor()/parametersMax().armor());
		unit->setParameters(prm);
	}

	if(executedUpgrade()->isBuilding() && !attr().upgrades[executedUpgradeIndex_].built){
		safe_cast<UnitBuilding*>(unit)->startConstruction(true);
		unit->setUpgradeProgresParameters(executedUpgradeIndex_, upgradeTime_, finishUpgradeTime_);
	}
	else{
		unit->startState(StateIsUpgraded::instance());
		unit->setUpgradeProgresParameters(executedUpgradeIndex_, upgradeTime_);
		if(attr().isLegionary())
			unit->setChainByHealthWithTimer(CHAIN_UPGRADED_FROM_LEGIONARY, unit->attr().chainUpgradedFromLegionary);
		else if(attr().isBuilding())
			unit->setChainByHealthWithTimer(CHAIN_UPGRADED_FROM_BUILDING, unit->attr().chainUpgradedFromBuilding);
		if(unit->attr().upgradeStop){
			unit->disableFlying(FLYING_DUE_TO_UPGRADE);
			unit->makeStaticXY(STATIC_DUE_TO_UPGRADE);
		}
		if(unit->rigidBody()->prm().flyingMode && !unit->rigidBody()->prm().hoverMode)
			unit->rigidBody()->setFlyingHeightCurrent(rigidBody()->flyingHeightCurrentWithDelta());
	}
	
	unit->setPose(pose(), true);
	unit->setLabel(label());

	if(executedUpgrade()->isLegionary()){
		UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
		legionary->setLevel(attr().upgrades[executedUpgradeIndex_].level, true);
		if(attr().isLegionary()){
			UnitSquad* squad = safe_cast<UnitLegionary*>(this)->squad();
			if(!squad->checkFormationConditions(legionary->attr().formationType)){
				UnitSquad* squadForUpgrade = squad->squadForUpgrade();
				if(!squadForUpgrade || !squadForUpgrade->checkFormationConditions(legionary->attr().formationType)){
					squad->setSquadForUpgrade(squadForUpgrade = safe_cast<UnitSquad*>(player()->buildUnit(&*legionary->attr().squad)));
					squadForUpgrade->setPose(unit->pose(), true);
					squadForUpgrade->addUnit(legionary);
					if(!squad->wayPointsEmpty())
						squadForUpgrade->addWayPointS(squad->wayPoint());
				}else
					squadForUpgrade->addUnit(legionary);
			}else
				 squad->addUnit(legionary);
		}else{
			UnitSquad* squad = safe_cast<UnitSquad*>(player()->buildUnit(&*legionary->attr().squad));
			squad->setPose(unit->pose(), true);
			squad->addUnit(legionary);
		}
	}
	
	if(selected()){
		universe()->changeSelection(this, unit);
		unit->setSelected(true); // убирает мигание селекта
		if(cameraManager->isVisible(position()))
			universe()->addVisibleUnit(unit);
	}

	if(usedByTrigger())
		unit->setUsedByTrigger(10);
	
	if(DirectControlMode directControlMode = directControl()){
		setDirectControl(DIRECT_CONTROL_DISABLED);
		unit->setDirectControl(directControlMode);
	}
	
	unit->setBeforeUpgrade(&attr());
	executedUpgradeIndex_ = -1;
	upgradeTime_ = 0;
	finishUpgradeTime_ = 0;
	hide(HIDE_BY_UPGRADE, true);
	Kill(); 
	unit->setPose(pose(), true); // ƒл€ восстановлени€ фундамента здани€
	if(attr().isBuilding())
		unit->setShipmentPosition(shipmentPosition());
}

void UnitActing::reserveSquadNumber(const AttributeBase* attr, int count)
{
	//player()->reserveUnitNumber(attr, count);
	//if(attr->isLegionary() && (!getSquadPoint() || getSquadPoint()->getUnitReal() == this))
	//	player()->reserveUnitNumber(safe_cast<const AttributeLegionary*>(attr)->squad, count);
}

void UnitActing::cancelUpgrade()
{
	makeDynamicXY(STATIC_DUE_TO_UPGRADE);
	enableFlying(FLYING_DUE_TO_UPGRADE);
	reserveSquadNumber(executedUpgrade(), -1);
	executedUpgradeIndex_ = -1;
	upgradeTime_ = 0;
	finishUpgradeTime_ = 0;
}

void UnitActing::mapUpdate(float x0,float y0,float x1,float y1)
{
	if(!rigidBody()->isBoxMode() && !rigidBody()->isFrozen())
		__super::mapUpdate(x0, y0, x1, y1);
}

void UnitActing::collision(UnitBase* unit, const ContactInfo& contactInfo)
{
	if(!rigidBody()->isBoxMode())
		__super::collision(unit, contactInfo);
}

void UnitActing::setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo)
{
	if(damage.empty() || isUnderEditor())
		return;

#ifndef _FINAL_VERSION_
	if(DebugPrm::instance().debugDisableDamage)
		return;
#endif

	ParameterSet armorDamage = damage;
	
	if(attr().armorFactors.used() && contactInfo){
		Vect3f collisionPoint = contactInfo->collisionPoint(this);
		pose().invXformPoint(collisionPoint);
		armorDamage *= attr().armorFactors.factor(collisionPoint);
	}

	int partIndex = contactInfo ? contactInfo->bodyPart(this) : -1;
	ParameterSet& parameters = partIndex != -1 ? bodyParts_[partIndex].parameters : parameters_;

	float prevHealth = parameters.health();
	float prevArmor = parameters.armor();
	bool wasPossessed = attr().canBeCaptured && parameters.possession() > FLT_EPS;

	ParameterSet healthDamage = armorDamage;
	healthDamage.set(0, ParameterType::ARMOR);
	armorDamage.sub(healthDamage);
	parameters.subClamped(armorDamage);
	
	ParameterSet armor;
	armor.setArmor(parameters, parametersMax());
	healthDamage.subPositiveOnly(armor);
	parameters.subClamped(healthDamage);
	
	parameters.clamp(parametersMax()); // ≈сли были отрицательные величины - лечение

	if(agressor && agressor->attr().isProjectile())
		agressor = agressor->ignoredUnit();

	float healthMax = parametersMax().health();
	if(prevHealth > 1.f && parameters.health() < 1.f){
		unregisterInPlayerStatistics(agressor);
		//xassert(agressor && "Ќе установлен юнит дл€ передачи параметров за гибель");
		if(agressor){
			ParameterArithmetics arithmetics = attr().deathGainArithmetics;
			UnitActing* unit = safe_cast<UnitActing*>(agressor);
			ShowChangeParametersController doShowUnit(this, unit->parameters(), ShowChangeParametersController::VALUES);
			ShowChangeParametersController doShowPlayer(unit, unit->player()->resource(), ShowChangeParametersController::VALUES);
			unit->deathGainMultiplicator(arithmetics);
			unit->applyParameterArithmetics(arithmetics);
			doShowUnit.update(unit->parameters());
			doShowPlayer.update(unit->player()->resource());
		}

		explode();
		Kill();
	}
	else 
		if(prevHealth < healthMax - healthMax*0.001f && parameters.health() > healthMax - healthMax*0.001f)
			player()->checkEvent(EventUnitPlayer(Event::COMPLETE_CURE, this, player()));

	if(!eventAttackTimer_.busy() && (prevHealth > parameters.health() || prevArmor > parameters.armor())){
		eventAttackTimer_.start(logicRND(5000));
		aiScanTimer.stop();
		player()->checkEvent(EventUnitMyUnitEnemy(Event::ATTACK_OBJECT, this, agressor));
		if(agressor)
			agressor->player()->checkEvent(EventUnitMyUnitEnemy(Event::ATTACK_OBJECT, this, agressor));
	}

	if(wasPossessed && parameters.possession() < FLT_EPS){
		//xassert(agressor && "Ќе установлен юнит дл€ оружи€ захвата");
		if(agressor && player() != agressor->player()){
			player()->checkEvent(EventUnitMyUnitEnemy(Event::CAPTURE_UNIT, this, agressor));
			agressor->player()->checkEvent(EventUnitMyUnitEnemy(Event::CAPTURE_UNIT, this, agressor));
			changeUnitOwner(agressor->player());
		}
		else
			parameters += healthDamage;
	}
}

bool UnitActing::requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const
{
	BodyParts::const_iterator i;
	FOR_EACH(bodyParts_, i)
		if(i->parameters.above(resource))
			return true;
	return __super::requestResource(resource, requestResourceType);
}

void UnitActing::subResource(const ParameterSet& resource)
{
	__super::subResource(resource);
	BodyParts::iterator i;
	FOR_EACH(bodyParts_, i)
		if(i->parameters.subClamped(resource))
			i->putOff();
}

bool UnitActing::accessibleParameter(const ProducedParameters& parameter) const
{
	if(!requestResource(parameter.cost, NEED_RESOURCE_TO_PRODUCE_PARAMETER))
		return false;
	if(!requestResource(parameter.accessValue, NEED_RESOURCE_TO_ACCESS_PARAMETER))
		return false;
	return player()->accessibleByBuildings(parameter);
}

bool UnitActing::putOnItem(const InventoryItem& inventoryItem)
{
	BodyParts::iterator i;
	FOR_EACH(bodyParts_, i){
		BodyPart& part = *i;
		BodyPartAttribute::Garments::const_iterator j;
		FOR_EACH(part.partAttr.possibleGarments, j)
			if(&*j->item == inventoryItem.attribute()){
				part.putOn(inventoryItem, *j); 
				updatePart(part);
				return true;
			}
	}
	return false;
}

void UnitActing::putOffItem(InventoryItem& inventoryItem)
{
	BodyParts::iterator i;
	FOR_EACH(bodyParts_, i){
		BodyPart& part = *i;
		if(part.item == inventoryItem.attribute()){
			part.putOff(inventoryItem);
			updatePart(part);
			return;
		}
	}
}

void UnitActing::updatePart(const BodyPart& part)
{
	streamLogicCommand.set(fCommandSetVisibilityGroupOfSet, model()) << part.visibilityGroup << part.visibilitySet;
	if(modelLogic())
		modelLogic()->SetVisibilityGroup(VisibilityGroupIndex(part.visibilityGroup), VisibilitySetIndex(part.visibilitySet));	
}

UnitActing::BodyPart::BodyPart(const BodyPartAttribute& attr)
: partAttr(attr)
{
	visibilitySet = partAttr.visibilitySet();
	visibilityGroup = partAttr.defaultGarment;
}

void UnitActing::BodyPart::putOn(const InventoryItem& inventoryItem, const BodyPartAttribute::Garment& garment)
{
	item = garment.item;
	parameters = inventoryItem.parameters();
	visibilityGroup = garment.visibilityGroup;
}

void UnitActing::BodyPart::putOff()
{
	visibilityGroup = partAttr.defaultGarment;
}

void UnitActing::BodyPart::putOff(InventoryItem& inventoryItem)
{
	putOff();
	inventoryItem.setParameters(parameters);
//	parameters = partAttr.parametersInitial;
}

void UnitActing::BodyPart::serialize(Archive& ar)
{
	ar.serialize(parameters, "parameters", 0);
	ar.serialize(item, "item", 0);
	ar.serialize(visibilitySet, "visibilitySet", 0);
	ar.serialize(visibilityGroup, "visibilityGroup", 0);
}

WeaponAnimationType UnitActing::getWeaponAnimationType(const WeaponSlot& weaponSlot) const
{
	if(weaponSlot.attribute()->externalAnimationSettings() && !weaponSlots_[weaponSlot.attribute()->animationSlotID()].isEmpty())
		return weaponSlots_[weaponSlot.attribute()->animationSlotID()].weapon()->aimControllerPrm().alternativeWeaponAnimationType();
	return weaponSlot.weapon()->weaponPrm()->animationType();
}

bool UnitActing::weaponChainQuant(MovementState state)
{
	weaponAnimationMode_ = false;
	vector<bool> activeAnimationGroups;
	activeAnimationGroups.resize(chainControllers_.size(), false);
	vector<const AnimationChain*> additionalChains;
	additionalChains.resize(chainControllers_.size(), 0);
	WeaponSlots::const_iterator it;
	const AnimationChain* defaultChain = 0;
	MovementState mstate = getMovementState();
	FOR_EACH(weaponSlots_, it){
		WeaponBase* weapon = it->weapon();
		WeaponAnimationType weaponAnimationType = getWeaponAnimationType(*it);
		switch(weapon->animationMode()){
		case WEAPON_ANIMATION_AIM:{
			const AnimationChain* chain = findChain(CHAIN_AIM, mstate, -1, weaponAnimationType);
			if(chain){
				if(chain->animationGroup() >= 0){
					activeAnimationGroups[chain->animationGroup()] = true;
					if(!additionalChains[chain->animationGroup()])
						additionalChains[chain->animationGroup()] = chain;
				}else if(!defaultChain)
					defaultChain = chain;
			}
			weaponAnimationMode_ = true;
			break;}
		case WEAPON_ANIMATION_RELOAD:{
			const AnimationChain* chain = findChain(weapon->reloadFromInventory() ? 
				CHAIN_RELOAD_INVENTORY : CHAIN_RELOAD, mstate, -1, weaponAnimationType);
			if(chain){
				if(chain->animationGroup() >= 0){
					activeAnimationGroups[chain->animationGroup()] = true;
					if(!additionalChains[chain->animationGroup()] 
					|| additionalChains[chain->animationGroup()]->chainID != CHAIN_FIRE)
						additionalChains[chain->animationGroup()] = chain;
				}else if(!defaultChain || defaultChain->chainID == CHAIN_AIM)
					defaultChain = chain;
			}
			weaponAnimationMode_ = true;
			break;}
		case WEAPON_ANIMATION_FIRE:{
			const AnimationChain* chain = findChain(CHAIN_FIRE, mstate, -1, weaponAnimationType);
			if(chain){
				if(chain->animationGroup() >= 0){
					activeAnimationGroups[chain->animationGroup()] = true;
					additionalChains[chain->animationGroup()] = chain;
				}else if(!defaultChain || defaultChain->chainID != CHAIN_FIRE)
					defaultChain = chain;
			}
			weaponAnimationMode_ = true;
			break;}
		}
	}
	if(defaultChain){
		setChain(defaultChain, mstate);
		return true;
	}
	if(weaponAnimationMode_){
		vector<const AnimationChain*>::iterator it;	
		FOR_EACH(additionalChains, it)
			setChain(*it, mstate, true);
		setChainByHealthExcludeGroups(CHAIN_MOVEMENTS, activeAnimationGroups);
		return true;
	}
	return false;
}

void UnitActing::initWeapon()
{
	const WeaponSlotAttributes* weapon_attributes = &attr().weaponAttributes;

	if(model()){
		int id = 0;
		weaponSlots_.resize(weapon_attributes->size());
		for(WeaponSlotAttributes::const_iterator ita = weapon_attributes->begin(); ita != weapon_attributes->end(); ++ita){
			weaponSlots_[id].setID(id);
			weaponSlots_[id].init(this, &ita->second);
			id++;
		}

		attachSmart(model());

		WeaponSlots::iterator it;
		FOR_EACH(weaponSlots_, it){
			WeaponSlots::const_iterator it1;
			FOR_EACH(weaponSlots_, it1)
				it->setParentSlot(&*it1);
		}
	}
}

inline bool UnitActing::fireWeaponModeCheck(const WeaponBase* weapon) const
{
	start_timer_auto();

	if(selectedWeaponID_ && weapon->weaponPrm()->ID() == selectedWeaponID_)
		return true;

	switch(weaponMode()){
	case SHORT_RANGE:
		return weapon->isShortRange();
	case LONG_RANGE:
		return weapon->isLongRange();
	case ANY_RANGE: // ¬ этом режиме оружие считаетс€ недоступным если перезарежаетс€.
		return !weapon->weaponPrm()->clearTargetOnLoading() || !weapon->isLoading();
	}

	return true;
}

bool UnitActing::fireRequest()
{
	if(isFrozen() || isUpgrading())
		return false;

	if(isAutoAttackForced())
		return fireRequestAuto();

	if(specialTargetPointEnable || specialTargetUnit_) {
		WeaponTarget target;

		if(specialTargetPointEnable)
			target = WeaponTarget(specialTargetPoint_, selectedWeaponID_);
		else if(specialTargetUnit_)
			target = WeaponTarget(specialTargetUnit_, selectedWeaponID_);

		return fireRequest(target);
	}
	else {
		if(isDirectControl() || (isSyndicateControl() && attr().syndicateControlAimEnabled)){
			if(targetPointEnable){
				WeaponTarget target = WeaponTarget(targetPoint_, 0);
				return fireRequest(target);
			}
			else {
				if(unitState() == ATTACK_MODE && targetUnit_){
					WeaponTarget target = WeaponTarget(targetUnit_);
					return fireRequest(target);
				}
			}
		}
		else if(targetPointEnable){
			WeaponTarget target = WeaponTarget(targetPoint_, 0);

			if(fireDistanceCheck(target))
				return fireRequest(target);
			else {
				if(fireRequest(target, true))
					return true;

				return fireRequestAuto();
			}
		}
		else {
			if(unitState() == ATTACK_MODE && targetUnit_){
				WeaponTarget target = WeaponTarget(targetUnit_);

				if(fireDistanceCheck(target))
					return fireRequest(target);
				else {
					if(fireRequest(target, true))
						return true;

					return fireRequestAuto();
				}
			}
			else 
				return fireRequestAuto();
		}
	}

	return false;
}

bool UnitActing::fireRequest(WeaponTarget& target, bool no_movement_weapons_only)
{
		bool ret = false;

	int group_priority = 0;
	bool skip_group = false;
	const WeaponGroupType* group = 0;

	UnitInterface* target_unit = target.unit();

	for(Weapons::const_iterator it = weapons_.begin(); it != weapons_.end(); ++it){
		WeaponBase* weapon = *it;

		if(!no_movement_weapons_only || weapon->weaponPrm()->disableOwnerMove()){
			if(group != weapon->groupType()){
				group_priority = 0;
				skip_group = false;
				group = weapon->groupType();
			}

			if(!skip_group || weapon->priority() == group_priority){
				if(isDirectControl()){
					switch(weapon->weaponPrm()->directControlMode()){
					case WEAPON_DIRECT_CONTROL_NORMAL:
						if(!target.weaponID() && weapon->isEnabled()){
							target.setWeaponID(weapon->weaponPrm()->ID());
							if(weapon->canAttack(target)){
//								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(target)){
//									skip_group = true;
//									group_priority = weapon->priority();
//								}
								if(weapon->fireRequest(target, !(directControlFireMode_ & DIRECT_CONTROL_PRIMARY_WEAPON)))
									ret = true;
							}
							else {
								WeaponTarget target1(target.position(), weapon->weaponPrm()->ID());
								if(weapon->canAttack(target1)){
//									if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(target1)){
//										skip_group = true;
//										group_priority = weapon->priority();
//									}
									if(weapon->fireRequest(target1, !(directControlFireMode_ & DIRECT_CONTROL_PRIMARY_WEAPON)))
										ret = true;
								}
							}
							target.setWeaponID(0);
						}
						else
							weapon->setAutoTarget(0);
						break;
					case WEAPON_DIRECT_CONTROL_ALTERNATE:
						if(!target.weaponID() && weapon->isEnabled() && weapon->weaponPrm()->ID() == directControlWeaponID()){
							target.setWeaponID(weapon->weaponPrm()->ID());
							if(weapon->canAttack(target)){
//								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(target)){
//									skip_group = true;
//									group_priority = weapon->priority();
//								}
								if(weapon->fireRequest(target, !(directControlFireMode_ & DIRECT_CONTROL_SECONDARY_WEAPON)))
									ret = true;
							}
							target.setWeaponID(0);
						}
						else
							weapon->setAutoTarget(0);
						break;
					case WEAPON_DIRECT_CONTROL_AUTO:
						if(UnitInterface* unit = weapon->autoTarget()){
							WeaponTarget trg(unit);		
							if(weapon->canAttack(trg)){
								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(trg)){
									skip_group = true;
									group_priority = weapon->priority();
								}
								if(weapon->fireRequest(trg))
									ret = true;
							}
							else
								weapon->setAutoTarget(0);
						}
						break;
					case WEAPON_DIRECT_CONTROL_DISABLE:
						weapon->setAutoTarget(0);
						break;
					}
				}
				else if(isSyndicateControl()){
					bool aim_only = attr().syndicateControlAimEnabled && !(directControlFireMode_ & DIRECT_CONTROL_SECONDARY_WEAPON);
					switch(weapon->weaponPrm()->syndicateControlMode()){
					case WEAPON_SYNDICATE_CONTROL_NORMAL:
						if(weapon->isEnabled() && fireWeaponModeCheck(weapon)){
							if(weapon->canAttack(target)){
								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(target)){
									skip_group = true;
									group_priority = weapon->priority();
								}
								if(weapon->fireRequest(target)){
									if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY){
										skip_group = true;
										group_priority = weapon->priority();
									}
									ret = true;
								}
							}
						}
						break;
					case WEAPON_SYNDICATE_CONTROL_FORCE:
						if(weapon->isEnabled() && fireWeaponModeCheck(weapon)){
							if(weapon->canAttack(target)){
								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(target)){
									skip_group = true;
									group_priority = weapon->priority();
								}
								if(target.unit()) aim_only = false;
								if(weapon->fireRequest(target, aim_only)){
									if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY){
										skip_group = true;
										group_priority = weapon->priority();
									}
									ret = true;
								}
							}
						}
						break;
					case WEAPON_SYNDICATE_CONTROL_AUTO:
						if(UnitInterface* unit = weapon->autoTarget()){
							WeaponTarget trg(unit);		
							if(weapon->canAttack(trg)){
								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(trg)){
									skip_group = true;
									group_priority = weapon->priority();
								}
								if(weapon->fireRequest(trg))
									ret = true;
							}
							else
								weapon->setAutoTarget(0);
						}
						break;
					case WEAPON_SYNDICATE_CONTROL_DISABLE:
						weapon->setAutoTarget(0);
						break;
					}
				}
				else {
					if(weapon->isEnabled() && fireWeaponModeCheck(weapon)){
						if(weapon->canAttack(target)){
							if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(target)){
								skip_group = true;
								group_priority = weapon->priority();
							}
							if(weapon->fireRequest(target)){
								if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY){
									skip_group = true;
									group_priority = weapon->priority();
								}
								ret = true;
							}
						}
						else {
							if(UnitInterface* unit = weapon->autoTarget()){
								if(unit == target_unit || (group->shootingMode() != WEAPON_GROUP_MODE_PRIORITY
								&& weapon->weaponPrm()->canShootMoving())){
									WeaponTarget trg(unit);		
									if(weapon->canAttack(trg)){
										if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(trg)){
											skip_group = true;
											group_priority = weapon->priority();
										}
										if(weapon->fireRequest(trg)){
											if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY){
												skip_group = true;
												group_priority = weapon->priority();
											}
											ret = true;
										}
									}
									else
										weapon->setAutoTarget(0);
								}
							}
						}
					}
					else
						weapon->setAutoTarget(0);
				}
			}
		}
	}

	return ret;
}

bool UnitActing::fireRequestAuto()
{
	bool ret = false;

	bool skip_group = false;
	const WeaponGroupType* group = 0;

	for(Weapons::const_iterator it = weapons_.begin(); it != weapons_.end(); ++it){
		WeaponBase* weapon = *it;

		if(group != weapon->groupType()){
			skip_group = false;
			group = weapon->groupType();
		}

		if(!skip_group){
			bool can_auto_shoot = !isDirectControl() || weapon->weaponPrm()->directControlMode() == WEAPON_DIRECT_CONTROL_AUTO;
			bool forced_disable = isDirectControl() && weapon->weaponPrm()->directControlMode() == WEAPON_DIRECT_CONTROL_DISABLE;

			if(weapon->isEnabled() && fireWeaponModeCheck(weapon) && !forced_disable && can_auto_shoot){
				if(UnitInterface* unit = weapon->autoTarget()){
					WeaponTarget trg(unit);
					if(weapon->canAttack(trg)){
						if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY && weapon->fireDistanceCheck(trg))
							skip_group = true;
						if(weapon->fireRequest(trg)){
							if(group->shootingMode() == WEAPON_GROUP_MODE_PRIORITY)
								skip_group = true;
							ret = true;
						}
					}
					else
						weapon->setAutoTarget(0);
				}
			}
			else 
				weapon->setAutoTarget(0);
		}
	}
	return ret;
}

bool UnitActing::fireCheck(WeaponTarget& target) const
{
	bool ret = false;
	for(WeaponSlots::const_iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it){
		WeaponBase* weapon = it->weapon();
		if(weapon->isEnabled() && weapon->isLoaded() && weapon->fireDistanceCheck(target.position()) && weapon->canAttack(target)){
			if(fireWeaponModeCheck(weapon) && weapon->checkFinalCost())
				return true;
		}
	}

	return false;
}

bool UnitActing::canFire(int weaponID, RequestResourceType triggerAction) const
{
	if(isUpgrading())
		return false;

	if(const WeaponBase* weapon = findWeapon(weaponID))
		if(weapon->isEnabled() && weapon->isLoaded() || (weapon->isLoading() && weapon->canFireMinTime())){
			if(!weapon->checkMovement())
				return false;
			
			if(((weapon->weaponPrm()->fireCostAtOnce() && weapon->canFire(triggerAction)) ||
				(!weapon->weaponPrm()->fireCostAtOnce() && weapon->canFireMinTime(triggerAction))) && weapon->checkFinalCost())
					return true;
		}

		return false;
}

bool UnitActing::fireDistanceCheck(const WeaponTarget& target, bool check_fow) const
{
	for(WeaponSlots::const_iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it){
		WeaponBase* weapon = it->weapon();
		if(weapon->isEnabled() && weapon->hasAmmo() && weapon->canAttack(target) && (!check_fow || weapon->checkFogOfWar(target))){
			if(weapon->checkTargetMode(target) && fireWeaponModeCheck(weapon) && !weapon->weaponPrm()->disableOwnerMove() && weapon->fireDistanceCheck(target, unitState() == ATTACK_MODE))
				return true;
		}
	}

	return false;
}

WeaponTarget UnitActing::fireTarget() const
{
	if(specialTargetPointEnable || specialTargetUnit_)
		return specialTargetPointEnable ? WeaponTarget(specialTargetPoint_, selectedWeaponID_) : WeaponTarget(specialTargetUnit_, selectedWeaponID_);	

	return targetPointEnable ? WeaponTarget(targetPoint_, selectedWeaponID_) : WeaponTarget(targetUnit_, selectedWeaponID_);
}

void UnitActing::updateTargetFromMainUnit(const UnitActing* mainUnit)
{
	selectWeapon(mainUnit->selectedWeaponID());
	if(mainUnit->specialTargetPointEnable)
		setTargetPoint(mainUnit->specialTargetPoint_);
	else if(mainUnit->specialTargetUnit_)
		setManualTarget(mainUnit->specialTargetUnit_);
	else if(mainUnit->targetPointEnable)
		setTargetPoint(mainUnit->targetPoint_);
	else
		setManualTarget(mainUnit->targetUnit_);
}

Rangef UnitActing::fireDistance(const WeaponTarget& target) const
{
	float r_min = FLT_INF;
	float r_max = 0.f;

	for(WeaponSlots::const_iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it){
		WeaponBase* weapon = it->weapon();
		if(weapon->isEnabled() && weapon->checkTargetMode(target) && weapon->hasAmmo() && weapon->canAttack(target) && fireWeaponModeCheck(weapon)){
			if(!weapon->weaponPrm()->disableOwnerMove()){
				r_min = min(r_min, weapon->fireRadiusMin());
				r_max = max(r_max, weapon->fireRadius());
			}
		}
	}

	return Rangef(r_min, r_max);
}

void fCommandReleaseWeapon(XBuffer& stream);

void UnitActing::weaponQuant()
{
	start_timer_auto();

	if(weaponSlots_.empty() || isUnderEditor()) 
		return;

	WeaponSlots::iterator it;
	FOR_EACH(weaponSlots_, it)
		it->upgradeWeapon();

	if(!isConstructed() || !isConnected() || (isDocked() && !inTransport()) || (currentState() & (CHAIN_FALL | CHAIN_RISE | CHAIN_FROZEN))){
		FOR_EACH(weaponSlots_, it){
			it->weapon()->enable(false);
			it->weapon()->setAccessible(false);
		}
	}
	else {
		FOR_EACH(weaponSlots_, it){
			if(!it->weapon()->checkTerrain()){
				it->weapon()->enable(false);
				it->weapon()->setAccessible(false);
			}
			else {
				bool accessible = weaponAccessible(it->weapon()->weaponPrm());
				it->weapon()->enable(accessible);
				it->weapon()->setAccessible(accessible);
			}
		}
	}

	if(fireRequest()){
		if(!fireRequestTimer_.busy()){
			fireRequestTimer_.start(logicRND(5000));
			player()->checkEvent(EventUnitPlayerInt(Event::UNIT_ATTACKING, this, player(), selectedWeaponID_));;
		}
	}

	FOR_EACH(weaponSlots_, it){
		WeaponBase* weapon = it->weapon();
		weapon->preQuant();
	}

	weaponRotation_.quant();

	FOR_EACH(weaponSlots_, it){
		WeaponBase* weapon = it->weapon();
		weapon->aimUpdate();
	}

	FOR_EACH(weaponSlots_, it)
		it->aimUpdate();

	FOR_EACH(weaponSlots_, it){
		WeaponBase* weapon = it->weapon();
		weapon->moveQuant();
	}

	// «апретить движение если стрел€ет непрерываемым оружием.
	makeDynamicXY(STATIC_DUE_TO_WEAPON);
	if(!isDirectControl())
		for(WeaponSlots::const_iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it){
			WeaponBase* weapon = it->weapon();
			if(weapon->isActive() && (!weapon->weaponPrm()->isInterrupt() && !weapon->weaponPrm()->canShootMoving()))
				makeStaticXY(STATIC_DUE_TO_WEAPON);
		}

	FOR_EACH(weaponSlots_, it)
		it->weapon()->fireEvent();

//	if(targetUnit() && !targetUnit()->alive())
//		targetController();
}

void UnitActing::weaponPostQuant()
{
	if(isUnderEditor())
		return;

	WeaponSlots::iterator it;

	FOR_EACH(weaponSlots_, it){
		it->weapon()->quant();
		it->weapon()->endQuant();
	}

	if(model()){
		WeaponSlots::iterator it;
		FOR_EACH(weaponSlots_, it)
			it->weapon()->interpolationQuant();
	}
}

bool UnitActing::removeWeaponFromSlot(int slot_index)
{
	if(slot_index >= 0 && slot_index < weaponSlots_.size())
		return weaponSlots_[slot_index].createWeapon(0);

	return false;
}

bool UnitActing::replaceWeaponInSlot(int slot_index, InventoryItem& item)
{
	if(slot_index >= 0 && slot_index < weaponSlots_.size())
		if(weaponSlots_[slot_index].createWeapon(item.attribute()->weaponReference)){
			weaponSlots_[slot_index].weapon()->setParameters(item.parameters());
			return true;
		}

	return false;
}

bool UnitActing::weaponAccessible(const WeaponPrm* weapon) const
{
	if(!requestResource(weapon->accessValue(), NEED_RESOURCE_SILENT_CHECK))//NEED_RESOURCE_TO_ACCESS_WEAPON))
		return false;

	return player()->weaponAccessible(weapon);
}

class WeaponSortOp
{
public:
	bool operator() (const WeaponBase* lh, const WeaponBase* rh) const { return *lh > *rh; }
};

bool UnitActing::addWeapon(WeaponBase* weapon)
{
	MTL();

	Weapons::const_iterator it = std::find(weapons_.begin(), weapons_.end(), weapon);
	if(it != weapons_.end())
		return false;

	weapons_.push_back(weapon);
	std::sort(weapons_.begin(), weapons_.end(), WeaponSortOp());

	return true;
}

bool UnitActing::removeWeapon(WeaponBase* weapon)
{
	MTL();

	Weapons::iterator it = std::find(weapons_.begin(), weapons_.end(), weapon);
	if(it != weapons_.end()){
		weapons_.erase(it);
		return true;
	}

	return false;
}

UnitInterface* UnitActing::fireUnit() const
{
	return specialTargetUnit_ ? specialTargetUnit_ : targetUnit();
}

Vect3f UnitActing::firePosition() const
{
	if(specialTargetPointEnable || specialTargetUnit_)
		return specialTargetPointEnable ? specialTargetPoint_ : specialTargetUnit_->position();

	return targetPointEnable ? targetPoint_ : targetUnit()->position();
}

Vect3f UnitActing::specialFirePosition() const
{
	if(specialTargetPointEnable)
		return specialTargetPoint_;
	else if(specialTargetUnit_)
		return specialTargetUnit_->position();

	return position(); // сюда приходить не должно
}

bool UnitActing::fireDistanceCheck() const
{
	if(fireTargetExist())
		return fireDistanceCheck(fireTarget());

	return false;
}

bool UnitActing::fireTargetExist() const
{
	if(specialTargetPointEnable || specialTargetUnit_) return true;
	return targetPointEnable || targetUnit();
}

bool UnitActing::fireTargetDocked() const
{
	if(specialTargetUnit_) 
		return specialTargetUnit_->isDocked();
	if(targetUnit()) 
		return targetUnit()->isDocked();
	return false;
}

bool UnitActing::isFireTarget(const UnitInterface* unit) const
{
	if(specialTargetUnit_ == unit) return true;
	if(targetUnit_ == unit) return true;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		if(it->weapon()->autoTarget() == unit)
			return true;
	}

	return false;
}

bool UnitActing::isAttackedFireTarget(const UnitInterface* unit) const
{
	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		if(it->weapon()->fireTarget() == unit)
			return true;
	}

	return false;
}

bool UnitActing::specialFireTargetExist() const
{
	if(specialTargetPointEnable || specialTargetUnit_)
		return true;
	return false;
}

UnitInterface* UnitActing::attackTarget(UnitInterface* unit, int weapon_id, bool moveToTarget)
{
	if(!canAttackTarget(WeaponTarget(unit, weapon_id)))
		return 0;
	if(moveToTarget)
		setUnitState(ATTACK_MODE);
	return unit;
}

void UnitActing::setManualTarget(UnitInterface* unit, bool moveToTarget)
{
	if(isAutoAttackForced())
		return;

	if(!isDirectControl()){
		directControlFireMode_ &= ~SYNDICAT_CONTROL_AIM_WEAPON;
		LegionariesLinks::const_iterator ui;
		FOR_EACH(transportSlots_, ui){
			UnitLegionary* legionary = *ui;
			if(legionary && legionary->canFireInTransport())
				legionary->setManualTarget(unit);
		}
	}

	if(isUpgrading())
		return;

	if(unitState() == TRIGGER_MODE)
		return;

	if(selectedWeaponID_) {
		specialTargetPointEnable = false;
		specialTargetUnit_ = unit ? attackTarget(unit, selectedWeaponID_, moveToTarget) : 0;
	} else {
		if(!isDefaultWeaponExist())
			return;

		specialTargetUnit_ = 0;
		specialTargetPoint_ = Vect3f::ZERO;
		specialTargetPointEnable = false;

		targetPointEnable = false;
		targetUnit_ = unit ? attackTarget(unit, 0, moveToTarget) : 0;
	}
}

void UnitActing::clearAttackTarget(int weaponID)
{
	selectWeapon(0);

	specialTargetUnit_ = 0;
	specialTargetPoint_ = Vect3f::ZERO;
	specialTargetPointEnable = false;

	targetUnit_ = 0;
	targetPoint_ = Vect3f::ZERO;
	targetPointEnable = false;

	clearAutoAttackTargets();
}

void UnitActing::clearAutoAttackTargets()
{
	for(WeaponSlots::iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it)
		it->weapon()->setAutoTarget(0);
}

void UnitActing::fireStop(int weaponID)
{
	if(weaponID == -1){
		WeaponSlots::iterator it;
		FOR_EACH(weaponSlots_, it)
			it->weapon()->switchOff();

		selectWeapon(0);
		setPreSelectedWeaponID(0);
	}
	else {
		if(WeaponBase* weapon = findWeapon(weaponID))
			weapon->switchOff();

		if(weaponID == selectedWeaponID())
			selectWeapon(0);

		if(weaponID == preSelectedWeaponID())
			setPreSelectedWeaponID(0);
	}
}

void UnitActing::applyPickedItem(UnitBase* unit)
{
	applyParameterArithmetics(unit->attr().parametersArithmetics);
	unit->Kill();
	player()->checkEvent(Event(Event::PICK_ITEM));
}

void UnitActing::applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics)
{
	__super::applyParameterArithmeticsImpl(arithmetics);

	WeaponSlots::iterator it;
	FOR_EACH(weaponSlots_, it)
		it->weapon()->applyParameterArithmetics(arithmetics);
}

bool UnitActing::canAttackTarget(const WeaponTarget& target, bool check_fow) const
{
	if(!isDirectControl()){
		LegionariesLinks::const_iterator ui;
		FOR_EACH(transportSlots_, ui){
			UnitLegionary* legionary = *ui;
			if(legionary && legionary->canFireInTransport() && legionary->canAttackTarget(target, check_fow))
				return true;
		}
	}

	if(const UnitInterface* unit = target.unit()){
		if(unit->isUnseen() && isEnemy(unit))
			return false;
	}

	if(weaponSlots_.empty())
		return false;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		if(it->weapon()->isAccessible() && fireWeaponModeCheck(it->weapon()) &&
			it->weapon()->checkTargetMode(target) && it->weapon()->canAttack(target) &&
			(!check_fow || it->weapon()->checkFogOfWar(target)))
				return true;
	}

	return false;
}

bool UnitActing::canAutoAttackTarget(const WeaponTarget& target) const
{
	start_timer_auto();

	if(weaponSlots_.empty())
		return false;

	if(const UnitInterface* unit = target.unit()){
		if(unit->isUnseen() && isEnemy(unit))
			return false;
	}

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(it->weapon()->isAccessible() && fireWeaponModeCheck(weapon) && weapon->canAutoFire() && /*weapon->checkTargetMode(target) &&*/ weapon->canAttack(target, true))
			return true;
	}

	return false;
}

bool UnitActing::isAutoAttackForced()
{
	if(const UnitSquad* squad = getSquadPoint())
		return squad->attr().forceUnitsAutoAttack && this != squad->getUnitReal();

	return false;
}

bool UnitActing::isAutoAttackDisabled()
{
	if(const UnitSquad* squad = getSquadPoint())
		return squad->attr().disableMainUnitAutoAttack && this == squad->getUnitReal();

	return false;
}

void UnitActing::selectWeapon(int weapon_id)
{ 
	if(!isDirectControl()){
		LegionariesLinks::const_iterator ui;
		FOR_EACH(transportSlots_, ui){
			UnitLegionary* legionary = *ui;
			if(legionary && legionary->canFireInTransport())
				legionary->selectWeapon(weapon_id);
		}
	}
	selectedWeaponID_ = weapon_id; 
}

const WeaponPrm* UnitActing::selectedWeapon() const
{
	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it)
		if(!selectedWeaponID_ || selectedWeaponID_ == it->weapon()->weaponPrm()->ID())
			return it->weapon()->weaponPrm();
	return 0;
}

float UnitActing::minFireRadiusOfWeapon(int weapon_id) const
{
	float r = 0;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it)
		if(weapon_id == it->weapon()->weaponPrm()->ID())
			r = it->weapon()->fireRadiusMin();

	return r;

}

float UnitActing::fireRadiusOfWeapon(int weapon_id) const
{
	float r = 0;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && ((!weapon_id && weapon->canAutoFire()) || weapon_id == weapon->weaponPrm()->ID()))
			r = max(r, weapon->fireRadius());
	}

	return r;
}

float UnitActing::fireDispersionRadius() const
{
	float r = 0;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && ((!selectedWeaponID_ && weapon->canAutoFire()) || selectedWeaponID_ == weapon->weaponPrm()->ID()))
			r = max(r, weapon->fireRadius() + weapon->fireDispersionRadius());
	}

	return r;
}

float UnitActing::fireRadius() const
{
	float r = 0;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && ((!selectedWeaponID_ && weapon->canAutoFire()) || selectedWeaponID_ == weapon->weaponPrm()->ID()))
			r = max(r, weapon->fireRadius());
	}

	return r;
}

float UnitActing::fireRadiusMin() const
{
	float r = fireRadius();

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && ((!selectedWeaponID_ && weapon->canAutoFire()) || selectedWeaponID_ == weapon->weaponPrm()->ID()))
			r = min(r, weapon->fireRadiusMin());
	}

	return r;
}

float UnitActing::autoFireRadius() const
{
	float r = 0;

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && weapon->canAutoFire())
			r = max(r, weapon->fireRadius());
	}

	return r;
}

float UnitActing::autoFireRadiusMin() const
{
	float r = autoFireRadius();

	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it){
		const WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && weapon->canAutoFire())
			r = min(r, weapon->fireRadiusMin());
	}

	return r;
}

void UnitActing::setTargetPoint(const Vect3f& point, bool aim_only, bool moveToTarget)
{
	if(isAutoAttackForced())
		return;

	if(!isDirectControl()){
		LegionariesLinks::const_iterator ui;
		FOR_EACH(transportSlots_, ui){
			UnitLegionary* legionary = *ui;
			if(legionary && legionary->canFireInTransport())
				legionary->setTargetPoint(point);
		}
	}

	if(isUpgrading())
		return;

	if(selectedWeaponID_ && !findWeapon(selectedWeaponID_))
		return;

	if(unitState() == TRIGGER_MODE)
		return;

	if(!aim_only){
		if(moveToTarget){
			stop();
			setUnitState(ATTACK_MODE);
		}
		directControlFireMode_ &= ~SYNDICAT_CONTROL_AIM_WEAPON;
	}else
		directControlFireMode_ |= SYNDICAT_CONTROL_AIM_WEAPON;
	
	if(selectedWeaponID_) {
		specialTargetUnit_ = 0;
		specialTargetPoint_ = point;
		specialTargetPointEnable = true;
	} else {
		specialTargetUnit_ = 0;
		specialTargetPoint_ = Vect3f::ZERO;
		specialTargetPointEnable = false;

		targetUnit_ = 0;
		targetPoint_ = point;
		targetPointEnable = true;
	}
}

bool UnitActing::isDefaultWeaponExist()
{
	for(WeaponSlots::iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it){
		WeaponBase* weapon = it->weapon();
		if(fireWeaponModeCheck(weapon) && weapon->weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_DEFAULT)
			return true;
	}

	return false;
}

bool UnitActing::isInSightSector(const UnitBase* target) const
{
	start_timer_auto();

	Vect3f dr = target->position() - position();
	dr.z = 0.f;

	float dr_norm = dr.norm();

	if(dr_norm < target->radius())
		return true;

	Vect3f temp_localwpoint;
	Se3f tmp_pose = pose();
	QuatF r2(M_PI_2, Vect3f::K);
	tmp_pose.rot().postmult(r2);
	tmp_pose.invXformPoint(target->position(), temp_localwpoint);
	float angle = atan2(temp_localwpoint.y, temp_localwpoint.x);

	float angle_size = asin(target->radius()/dr_norm);

	return fabs(angle) < attr().sightSector/2.f + angle_size;
}

void UnitActing::setDirectControl(DirectControlMode mode, bool setFoceMainUnit)
{
	if(directControl()){
		setActiveDirectControl(DIRECT_CONTROL_DISABLED);
		directControlFireMode_ = 0;
		clearAttackTarget();
		LegionariesLinks::iterator ui;
		FOR_EACH(transportSlots_, ui)
			if(UnitLegionary* unit = *ui)
				if(unit->directControlPrev())
					unit->setDirectControlPrev(false);
		if(isDirectControl() && attr().isLegionary())
			safe_cast<UnitLegionary*>(this)->formationUnit_.disableManualMode();
		if(isSyndicateControl() && attr().isLegionary() && attr().disablePathTrackingInSyndicateControl && safe_cast<UnitLegionary*>(this)->squad())
			safe_cast<UnitLegionary*>(this)->squad()->setDisablePathTracking(false);
	}
	if(mode){
		if(selectingTeamIndex_ == player()->teamIndex())
			setActiveDirectControl(mode);
		wayPointsClear();
		if(mode & DIRECT_CONTROL_ENABLED){
			selectWeapon(0);
			if(attr().isLegionary())
				safe_cast<UnitLegionary*>(this)->formationUnit_.enableManualMode();
			directControlFireMode_ = 0;
			if(directControlWeaponSlot_ >= 0 && directControlWeaponSlot_ < weaponSlots_.size()){
				const WeaponBase* weapon = weaponSlots_[directControlWeaponSlot_].weapon();
				if(!weapon->isEnabled() || weapon->weaponPrm()->directControlMode() != WEAPON_DIRECT_CONTROL_ALTERNATE)
					changeDirectControlWeapon(1);
			}else
				changeDirectControlWeapon(1);
		}
		if(setFoceMainUnit && attr().isLegionary()){
			UnitLegionary* unitLegion = safe_cast<UnitLegionary*>(this);
			unitLegion->squad()->findAndSetForceMainUnit(unitLegion);
		}
		if(mode & SYNDICATE_CONTROL_ENABLED && attr().isLegionary() && attr().disablePathTrackingInSyndicateControl)
			safe_cast<UnitLegionary*>(this)->squad()->setDisablePathTracking(true);
	}
	directKeysCommand = UnitCommand(COMMAND_ID_DIRECT_KEYS, 0, Vect3f::ZERO, 0);
	directControl_ = mode;
}

void UnitActing::executeDirectKeys(const UnitCommand& command)
{
	if(isDirectControl()){
		if(command.unit())
			setManualTarget(command.unit());
		else
			setTargetPoint(command.position());

		int keys = command.commandData();

		int fire_mode = 0;
		if(keys & DIRECT_KEY_MOUSE_LBUTTON)
			fire_mode |= DIRECT_CONTROL_PRIMARY_WEAPON;
		if(keys & DIRECT_KEY_MOUSE_RBUTTON)
			fire_mode |= DIRECT_CONTROL_SECONDARY_WEAPON;
		directControlFireMode_ = fire_mode;
	}
	else if(isSyndicateControl()){
		int keys = command.commandData();

		if(keys & DIRECT_KEY_MOUSE_RBUTTON)
			directControlFireMode_ |= DIRECT_CONTROL_SECONDARY_WEAPON;
		else
			directControlFireMode_ &= ~DIRECT_CONTROL_SECONDARY_WEAPON;
	}
}

bool UnitActing::changeDirectControlWeapon(int direction)
{
	int last_weapon_id = directControlWeaponID();

	for(int i = 0; i < weaponSlots_.size(); i++){
		int slot_id = (direction >= 0) ? i + directControlWeaponSlot_ + 1 : -i + directControlWeaponSlot_ - 1;
		if(slot_id < 0) slot_id += weaponSlots_.size();
		if(slot_id >= weaponSlots_.size()) slot_id -= weaponSlots_.size(); 

		const WeaponBase* weapon = weaponSlots_[slot_id].weapon();
		if(weapon->isEnabled() && weapon->weaponPrm()->ID() != last_weapon_id && weapon->weaponPrm()->directControlMode() == WEAPON_DIRECT_CONTROL_ALTERNATE){
			directControlWeaponSlot_ = slot_id;
			return true;
		}
	}

	return false;
}

int UnitActing::directControlWeaponID() const
{
	if(directControlWeaponSlot_ >= 0 && directControlWeaponSlot_ < weaponSlots_.size()){
		const WeaponBase* weapon = weaponSlots_[directControlWeaponSlot_].weapon();
		if(weapon->weaponPrm()->directControlMode() == WEAPON_DIRECT_CONTROL_ALTERNATE)
			return weapon->weaponPrm()->ID();
	}

	return 0;
}

const Vect3f& UnitActing::directControlOffset() const
{
	if(rigidBody()->onDeepWater())
		return attr().directControlOffsetWater;
	return attr().directControlOffset;
}

void UnitActing::setWalkAttackMode(WalkAttackMode mode)
{
	if(unitState() != ATTACK_MODE)
		clearAttackTarget();
	else
		clearAutoAttackTargets();

	makeDynamicXY(STATIC_DUE_TO_ATTACK);
	attackMode_.setWalkAttackMode(mode);
}

void UnitActing::setAutoAttackMode(AutoAttackMode mode)
{
	if(unitState() != ATTACK_MODE)
		clearAttackTarget();
	else
		clearAutoAttackTargets();

	if(unitState() == AUTO_MODE)
		stop();

	attackMode_.setAutoAttackMode(mode);
}

void UnitActing::setWeaponMode(WeaponMode mode)
{
	if(unitState() != ATTACK_MODE)
		clearAttackTarget();
	else
		clearAutoAttackTargets();

	if(unitState() == AUTO_MODE)
		stop();

	attackMode_.setWeaponMode(mode);
}

void UnitActing::setAutoTargetFilter(AutoTargetFilter filter)
{
	if(unitState() != ATTACK_MODE)
		clearAttackTarget();
	else
		clearAutoAttackTargets();

	if(unitState() == AUTO_MODE)
		stop();

	attackMode_.setAutoTargetFilter(filter);
}

const AttackModeAttribute& UnitActing::attackModeAttr() const
{
	return attr().hasAutomaticAttackMode ? attr().attackModeAttribute : player()->attackMode();
}

float UnitActing::targetRadius()
{
	if(specialTargetUnit_)
		return specialTargetUnit_->radius();

	if(targetUnit_)
		return targetUnit_->radius();

	return 0.0f;
}

float UnitActing::weaponChargeLevel(int weapon_id) const
{
	if(weapon_id){
		WeaponSlots::const_iterator it;
		FOR_EACH(weaponSlots_, it){
			if(it->weapon()->weaponPrm()->ID() == weapon_id)
				return it->weapon()->chargeLevel();
		}
	}
	else {
		float charge = 0.f;
		int sz = 0;

		WeaponSlots::const_iterator it;
		FOR_EACH(weaponSlots_, it){
			charge += it->weapon()->chargeLevel();
			sz++;
		}

		if(sz)
			return charge / float(sz);
	}

	return 0.f;
}

bool UnitActing::hasWeapon(int weapon_id) const
{
	if(weapon_id){
		WeaponSlots::const_iterator it;
		FOR_EACH(weaponSlots_, it){
			const WeaponBase* weapon = it->weapon();
			if(weapon->isEnabled() && weapon->weaponPrm()->ID() == weapon_id)
				return true;
		}

		return false;
	}

	return !weaponSlots_.empty();
}

WeaponBase* UnitActing::findWeapon(int weapon_id) const
{
	for(WeaponSlots::const_iterator it = weaponSlots_.begin(); it != weaponSlots_.end(); ++it){
		if(it->weapon()->weaponPrm()->ID() == weapon_id)
			return it->weapon();
	}

	return 0;
}

const WeaponBase* UnitActing::findWeapon(const char* label) const
{
	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it)
		if(!strcmp(it->weapon()->weaponPrm()->label(), label))
			return it->weapon();

	return 0;
}

void UnitActing::wayPointsClear() 
{ 
	if(UnitSquad* squad = getSquadPoint()) 
		squad->wayPointsClear(); 
}

void UnitActing::stop()
{
	wayPointsClear();

	if(unitState() == TRIGGER_MODE)
		return;

	setUnitState(AUTO_MODE);
}

void UnitActing::showCircles(const Vect2f& pos) const
{
	universe()->circleManager()->addCircle(pos, sightRadius(), attr().signRadiusCircle);
	universe()->circleManager()->addCircle(pos, noiseRadius(), attr().noiseRadiusCircle);
	universe()->circleManager()->addCircle(pos, hearingRadius(), attr().hearingRadiusCircle);
	universe()->circleManager()->addCircle(pos, radius() + fireRadius(), attr().fireRadiusCircle);
	universe()->circleManager()->addCircle(pos, radius() + fireRadiusMin(), attr().fireMinRadiusCircle);
	universe()->circleManager()->addCircle(pos, radius() + fireDispersionRadius(), attr().fireDispRadiusCircle);

	if(!isDirectControl())
		showSelect(pos);
}

void UnitActing::showEditor()
{
	__super::showEditor();

	if(selected()) 
		showCircles(position2D());
}

void UnitActing::attachUnit(UnitReal* unit)
{
	dockedUnits.push_back(unit);
}

void UnitActing::detachUnit(UnitReal* unit)
{
	RealsLinks::iterator i = find(dockedUnits.begin(), dockedUnits.end(), unit);
	if(i != dockedUnits.end())
		dockedUnits.erase(i);
}

bool UnitActing::isWorking() const 
{ 
	return unitState() != UnitReal::AUTO_MODE || isProducing() || isUpgrading() || isDocked(); 
}
