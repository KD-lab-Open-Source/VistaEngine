#include "StdAfx.h"
#include "UnitInterface.h"
#include "Squad.h"
#include "Player.h"
#include "Universe.h"
#include "CameraManager.h"
#include "EventParameters.h"
#include "GameCommands.h"
#include "Environment\Environment.h"
#include "Water\SkyObject.h"
#include "Water\CircleManager.h"
#include "UserInterface\UI_Logic.h"
#include "Game\GameOptions.h"

UNIT_LINK_GET(UnitInterface)
UNIT_LINK_GET(const UnitInterface)

UnitInterface::UnitInterface(const UnitTemplate& data)
: UnitBase(data),
  AttributeCache(*data.attribute())	
{
	const AttributeCache* cache = player()->attributeCache(&attr());
	if(cache)
		static_cast<AttributeCache&>(*this) = *cache;

	usedByTriggerPriority_ = 0;
	usedByTriggerAction_ = 0;
	usedByTriggerResetCounter_ = 0;

	selectingTeamIndex_ = -1;
	hoverTimer_ = 0;
}

UnitInterface::~UnitInterface()
{
}

void UnitInterface::applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics)
{
	applyArithmetics(arithmetics);
}

void UnitInterface::sendCommand(const UnitCommand& command)
{
	switch(command.commandID()) {
	// исполняются непосредственно, на повторяемость не влияет
	case COMMAND_ID_CAMERA_FOCUS:
	case COMMAND_ID_SELECT_SELF:
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

void UnitInterface::graphQuant(float dt)
{
	if(!attr().isInventoryItem() && cameraManager->eyePosition().distance2(interpolatedPose().trans()) < universe()->maxSptiteVisibleDistance())
	{
		bool needSprite = false;

		if(!attr().showSpriteForUnvisible)
			needSprite = true;
		else if(model())
			needSprite = model()->unvisible();
		else if(attr().isSquad()){
			const LegionariesLinks& units = getSquadPoint()->units();
			LegionariesLinks::const_iterator it;
			FOR_EACH(units, it)
				if((*it)->model() && (*it)->model()->unvisible()){
					needSprite = true;
					break;
				}
		}

		if(needSprite){
			bool connected = (attr().isObjective() ? safe_cast<UnitObjective*>(this)->isConnected() : true);
			const AttributeBase::SelectSprite& sprites = attr().selectSprites[connected ? AttributeBase::ORDINARY : AttributeBase::UNPOWERED];
			const UI_UnitSprite* sprite = 0;
			if(player() == universe()->activePlayer()){
				sprite = signHovered() && attr().selectBySprite ? &sprites.selectSpriteHover : (selected() ? &sprites.selectSpriteSelected : &sprites.selectSpriteNormal);
				if(sprite->isEmpty())
					sprite = &sprites.selectSpriteNormal;
			}
			else if(sprites.showSelectSpritesForOthers){
				if(sprites.ownSelectSpritesForOthers)
					sprite = signHovered() && attr().selectBySprite 
					&& !sprites.unitSpriteForOthersHovered.isEmpty() ? &sprites.unitSpriteForOthersHovered : &sprites.unitSpriteForOthers;
				else
					sprite = signHovered() && attr().selectBySprite 
					&& !player()->race()->squadSpriteForOthersHovered().isEmpty() ? &player()->race()->squadSpriteForOthersHovered() : &player()->race()->squadSpriteForOthers();
			}
			Recti pos;
			if(sprite 
				&& sprite->draw(interpolatedPose().trans(), 
								attr().showSpriteForUnvisible ? 0.f : attr().initialHeightUIParam, 
								player()->unitColor(), 
								Vect2f::ZERO, 
								GameOptions::instance().getFloat(OPTION_UNIT_SPRITE_SCALE),
								&pos)
				&& attr().selectBySprite)
			{
				UI_LogicDispatcher::instance().addSquadSelectSign(pos, this);
			}
		}
	}

	if(hoverTimer_ > 0)
		--hoverTimer_;
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

	if(usedByTrigger() && (!isWorking() || usedByTriggerTimer_.finished()) && usedByTriggerResetCounter_++)
		setUsedByTrigger(0);
}

void UnitInterface::setPose(const Se3f& pose, bool initPose)
{
	UnitBase::setPose(pose, initPose);

	if(initPose && isConstructed() && (!attr().isProjectile() || !isDocked())){
		stopPermanentEffects();
		startPermanentEffects();
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
	case COMMAND_ID_CAMERA_FOCUS:{
		CameraCoordinate coord(position2D(), 
							   cameraManager->coordinate().psi(),
							   cameraManager->coordinate().theta(),
							   cameraManager->coordinate().fi(),
							   cameraManager->coordinate().distance(), 
							   cameraManager->coordinate().focus());
		cameraManager->setCoordinate(coord);
		break;}
	case COMMAND_ID_CAMERA_MOVE:
		if(player()->active()){
			CameraCoordinate coord(command.position(), 
				command.angleZ(),
				cameraManager->coordinate().theta(),
				cameraManager->coordinate().fi(),
				cameraManager->coordinate().distance(), 
				cameraManager->coordinate().focus());
			cameraManager->setCoordinate(coord);
		}
		break;
	case COMMAND_ID_SELECT_SELF:
		universe()->select(this, command.shiftModifier());
		break;

	case COMMAND_ID_UNIT_SELECTED:
		if(command.shiftModifier() && player()->active())
			player()->checkEvent(EventUnitPlayer(Event::SELECT_UNIT, this , player()));
		if(player()->active())
			selectingTeamIndex_ = command.commandData();
		break;

	case COMMAND_ID_UNIT_DESELECTED:
		if(player()->active())
			selectingTeamIndex_ = -1;
		break;

	default:
		MTL();
		break;
	}
}

const InventorySet* UnitInterface::inventory() const
{
	const UnitReal* unit = getUnitReal();
	if(unit->attr().isLegionary())
		return safe_cast<const UnitLegionary*>(unit)->getInventory();
	
	return 0;
}

float UnitInterface::health() const 
{ 
	float maxHealth = parametersMax().health();
	return maxHealth ? clamp(parameters().health()/maxHealth, 0.0f, 1.0f) : 1.0f; 
}

float UnitInterface::sightRadius() const
{
	float nightFactor = environment->environmentTime()->isDay() ? 1.f : attr().sightRadiusNightFactor;
	return min(parameters().findByType(ParameterType::SIGHT_RADIUS, 300)*nightFactor, float(FogOfWarMap::maxSightRadius()));
}

float UnitInterface::noiseRadius() const
{
	return parameters().findByType(ParameterType::NOISE_RADIUS, 0);
}

float UnitInterface::hearingRadius() const
{
	return parameters().findByType(ParameterType::HEARING_RADIUS, 100);
}

void UnitInterface::showSelect(const Vect2f& pos) const
{
	if(!attr().showSelectRadius)
		return;

	float selectRadius = attr().selectRadius > 1.f ? attr().selectRadius : radius();
	universe()->circleManager()->addCircle(pos, selectRadius, player()->race()->circle);
	if(player()->teamMode() && (selectingTeamIndex_ != -1) && (selectingTeamIndex_ != player()->teamIndex()))
		universe()->circleManager()->addCircle(pos, selectRadius * 0.9f, player()->race()->circleTeam);
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

void UnitInterface::setUsedByTrigger(int priority, const void* action, int time)
{
	if(priority){
		if(universe()->interfaceEnabled() && player()->active())
			startEffect(&player()->race()->workForAIEffect());
		
		XBuffer buffer;
		buffer < "Предыдущий триггер: " < usedTriggerName_.c_str() < ", новый: " < Trigger::currentTriggerName();
		xassertStr((!usedByTriggerPriority_ || priority < usedByTriggerPriority_) && "Повторное включение usedByTrigger в триггере", buffer);
		usedTriggerName_ = Trigger::currentTriggerName();
		usedByTriggerPriority_ = priority;
		usedByTriggerAction_ = action;
		usedByTriggerResetCounter_ = 0;
		if(time)
			usedByTriggerTimer_.start(time);
	}
	else{
		if(universe()->interfaceEnabled() && player()->active())
			stopEffect(&player()->race()->workForAIEffect());

		xassertStr((usedByTriggerPriority_ || action && action != usedByTriggerAction_) && "Повторное выключение usedByTrigger в триггере", Trigger::currentTriggerName());
		usedByTriggerPriority_ = 0;
		usedByTriggerAction_ = 0;
		usedByTriggerTimer_.stop();
	}
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