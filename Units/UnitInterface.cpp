#include "StdAfx.h"
#include "UnitInterface.h"
#include "Squad.h"
#include "Player.h"
#include "Universe.h"
#include "CameraManager.h"
#include "EventParameters.h"
#include "GameCommands.h"
#include "..\Environment\Environment.h"
#include "..\Water\SkyObject.h"

UNIT_LINK_GET(UnitInterface)
UNIT_LINK_GET(const UnitInterface)

UnitInterface::UnitInterface(const UnitTemplate& data)
: UnitBase(data),
  AttributeCache(*data.attribute())	
{
	const AttributeCache* cache = player()->attributeCache(&attr());
	if(cache)
		static_cast<AttributeCache&>(*this) = *cache;

	usedByTrigger_ = false;
	showAISign_ = false;
	reason_ = REASON_DEFAULT;
}

UnitInterface::~UnitInterface()
{
}

void UnitInterface::sendCommand(const UnitCommand& command)
{
	switch(command.commandID()) {
	// исполняются непосредственно, на повторяемость не влияет
	case COMMAND_ID_CAMERA_FOCUS:
		if(player()->active())
			executeCommand(command);
		return;
	}

	MTG();
	if(player()->controlEnabled())
		universe()->sendCommand(netCommand4G_UnitCommand(unitID().index(), command));
}

void UnitInterface::receiveCommand(const UnitCommand& command)
{
	if(command.isUnitValid())
		if(command.shiftModifier() && isSuspendCommand(command.commandID())){
			if(canSuspendCommand(command))
				suspendCommandList_.push_back(command);
		}
		else if(!command.shiftModifier() && isSuspendCommand(command.commandID())){
			suspendCommandList_.clear();
			executeCommand(command);
		}
		else
			executeCommand(command);
}

void UnitInterface::Quant()
{
	__super::Quant();

	if(dead())
		return;

	if(!suspendCommandList_.empty() && !isSuspendCommandWork()) {
		UnitCommand command = suspendCommandList_.front();
		suspendCommandList_.erase(suspendCommandList_.begin());
		if(command.isUnitValid())
			executeCommand(command);
	}

	if(usedByTrigger_ && usedByTriggerTimer_.was_started()){

		bool interrupt = false;
		switch(reason_){
			case REASON_DEFAULT:
				break;
			case REASON_ATTACK:
				if(getUnitReal()->getUnitState() != UnitReal::ATTACK_MODE)
					interrupt = true;
				break;
			case REASON_MOVE:
				if(getUnitReal()->getUnitState() != UnitReal::MOVE_MODE)
					interrupt = true;
		}

		if(usedByTriggerTimer_() || interrupt){
			usedByTriggerTimer_.stop();
			reason_ = REASON_DEFAULT;
			executeCommand(UnitCommand(COMMAND_ID_STOP));
			setUsedByTrigger(false);
		}
	}

    if(showAISign_ && !isWorking()){
		showAISign_ = false;
		if(player()->active())
			stopEffect(&player()->race()->workForAIEffect());
	}
}

void UnitInterface::executeCommand(const UnitCommand& command)
{
	switch(command.commandID()){
	case COMMAND_ID_EXPLODE_UNIT_DEBUG: {
			MTL();
			ParameterSet damage(parameters());
			damage.set(0);
			damage.set(DebugPrm::instance().debugDamage, ParameterType::HEALTH);
			damage.set(DebugPrm::instance().debugDamageArmor, ParameterType::ARMOR);
			setDamage(damage, 0);
		} break;
	case COMMAND_ID_EXPLODE_UNIT: {
			ParameterSet damage(parameters());
			damage.set(0);
			damage.set(1e+6f, ParameterType::HEALTH);
			damage.set(1e+6f, ParameterType::ARMOR);
			setDamage(damage, 0);
		} break;
	case COMMAND_ID_KILL_UNIT: {
			Kill();
		} break;
	case COMMAND_ID_CAMERA_FOCUS:{
		CameraCoordinate coord(	position2D(), 
								cameraManager->coordinate().psi(),
								cameraManager->coordinate().theta(),
								cameraManager->coordinate().distance());
		cameraManager->setCoordinate(coord);
		break;}
	default:
		MTL();
		break;
	}
}

const InventorySet* UnitInterface::inventory() const
{
	if(UnitLegionary* unit = dynamic_cast<UnitLegionary*>(const_cast<UnitInterface*>(this)->getUnitReal()))
		return unit->getInventory();
	
	return 0;
}

float UnitInterface::health() const 
{ 
	float maxHealth = parametersMax().health();
	return maxHealth ? clamp(parameters().health()/maxHealth, 0, 1) : 1; 
}

float UnitInterface::sightRadius() const
{
	float nightFactor = environment->environmentTime()->IsDay() ? 1.f : attr().sightRadiusNightFactor;
	return min(parameters().findByType(ParameterType::SIGHT_RADIUS, 300)*nightFactor, float(FogOfWarMap::maxSightRadius()));
}

void UnitInterface::serialize(Archive& ar)
{
	UnitBase::serialize(ar);
	if(universe()->userSave()){
		AttributeCache::serialize(ar);
		ar.serialize(suspendCommandList_, "suspendCommandList", 0);
	}
}

bool UnitInterface::requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const
{
	if(!resource.below(parameters(), player()->resource())){
		if(requestResourceType){
			ParameterSet delta = resource;
			delta.subClamped(parameters());
			delta.subClamped(player()->resource());
			// Caution graph quant
			const_cast<Player*>(player())->checkEvent(EventParameters(Event::LACK_OF_RESOURCE, delta, requestResourceType));
		}
		return false;
	}
	return true;

}

void UnitInterface::subResource(const ParameterSet& resource)
{
	parameters_.subClamped(resource);
	player()->subResource(resource);
}

void UnitInterface::setShowAISign(bool showAISign)
{
	if(!showAISign_ && showAISign && universe()->interfaceEnabled()){
		showAISign_ = true;
		if(player()->active())
			startEffect(&player()->race()->workForAIEffect());
	}
	if(showAISign_ && !showAISign){
		showAISign_ = false;
		if(player()->active())
			stopEffect(&player()->race()->workForAIEffect());
	}
}

void UnitInterface::setUsedByTrigger(bool usedByTrigger)
{
	if(usedByTrigger && !showAISign_ && universe()->interfaceEnabled()){
		showAISign_ = true;
		if(player()->active())
			startEffect(&player()->race()->workForAIEffect());
	}
	usedByTrigger_ = usedByTrigger;
}

void UnitInterface::startUsedByTrigger(int time, UsedByTriggerReason reason)
{
	setUsedByTrigger(true);
	usedByTriggerTimer_.start(time);
	reason_ = reason;
}

bool UnitInterface::uniform(const UnitReal* unit) const
{
	return !unit || &attr() == &unit->attr();
}

bool UnitInterface::prior(const UnitInterface* unit) const
{
	int pr1 = selectionListPriority();
	int pr2 =unit->selectionListPriority();

	return pr1 == pr2 ? &attr() < &unit->getUnitReal()->attr() : pr1 > pr2;
}