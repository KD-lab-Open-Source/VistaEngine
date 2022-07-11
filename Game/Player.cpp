#include "StdAfx.h"

#include "Region.h"
#include "RenderObjects.h"
#include "CameraManager.h"
#include "Universe.h"
#include "Triggers.h"
#include "SoundApp.h"
#include "UnitActing.h"
#include "IronLegion.h"
#include "IronBullet.h"
#include "Nature.h"
#include "Squad.h"
#include "IronBuilding.h"
#include "ExternalShow.h"
#include "Serialization.h"
#include "ResourceSelector.h"
#include "Installer.h"
#include "EventParameters.h"
#include "GameCommands.h"
#include "..\Environment\Environment.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\UserInterface\UI_CustomControls.h"

BEGIN_ENUM_DESCRIPTOR(AuxPlayerType, "AuxPlayerType")
REGISTER_ENUM(AUX_PLAYER_TYPE_ORDINARY_PLAYER, "Обычный игрок");
REGISTER_ENUM(AUX_PLAYER_TYPE_COMMON_FRIEND, "Общий союзник");
REGISTER_ENUM(AUX_PLAYER_TYPE_COMMON_ENEMY, "Общий враг");
END_ENUM_DESCRIPTOR(AuxPlayerType)

BEGIN_ENUM_DESCRIPTOR(RequestResourceType, "RequestResourceType")
REGISTER_ENUM(NEED_RESOURCE_TO_ACCESS_UNIT_OR_BUILDING, "Доступность юнита или здания");
REGISTER_ENUM(NEED_RESOURCE_TO_INSTALL_BUILDING, "Установка здания");
REGISTER_ENUM(NEED_RESOURCE_TO_BUILD_BUILDING, "Строительство здания");
REGISTER_ENUM(NEED_RESOURCE_TO_PRODUCE_UNIT, "Производство юнита");
REGISTER_ENUM(NEED_RESOURCE_TO_ACCESS_PARAMETER, "Доступность параметра");
REGISTER_ENUM(NEED_RESOURCE_TO_PRODUCE_PARAMETER, "Производство параметра");
REGISTER_ENUM(NEED_RESOURCE_TO_UPGRADE, "Апгрейд");
REGISTER_ENUM(NEED_RESOURCE_TO_MOVE, "Движение юнита");
REGISTER_ENUM(NEED_RESOURCE_TO_ACCESS_WEAPON, "Доступность оружия - выключен");
REGISTER_ENUM(NEED_RESOURCE_TO_FIRE, "Стрельба оружием");
END_ENUM_DESCRIPTOR(RequestResourceType)

//-------------------------------------
Player::Player(const PlayerData& playerData) 
: fogOfWarMap_(0)
, unitNumberManager_(playerData.race->unitNumbers)
{
	playerID_ = playerData.playerID;
	teamIndex_ = playerData.teamIndex;
	teamNames_[teamIndex_] = playerData.name();
	teamMode_ = false;
	name_ = playerData.name();
	shuffleIndex_ = playerData.shuffleIndex != -1 ? playerData.shuffleIndex : playerID_;
	clan_ = playerData.clan != -1 ? playerData.clan : 64 + playerID_;
	isWorld_ = playerData.realPlayerType == REAL_PLAYER_TYPE_WORLD;
	auxPlayerType_ = AUX_PLAYER_TYPE_ORDINARY_PLAYER;
	realPlayerType_ = playerData.realPlayerType;

	controlEnabled_ = true;
	triggersDisabled_ = false;

	setColorIndex(playerData.colorIndex);
	setSignIndex(playerData.signIndex);
	setSilhouetteColorIndex(playerData.silhouetteColorIndex);
	setUnderwaterColorIndex(playerData.underwaterColorIndex);
	difficulty_ = playerData.difficulty;
	race_ = playerData.race;

	hasAttackMode_ = false;
	attackMode_ = race_->attackModeAttr();

	legionariesAndBuildings_ = 0;
	unitNumberReserved_ = 0;

	active_ = false;
	if(playerData.realPlayerType == REAL_PLAYER_TYPE_AI)
		isAI_ = true;
	else{
		isAI_ = false;
		if(!isUnderEditor())
			difficulty_ = GlobalAttributes::instance().assistantDifficulty;
	}

	isDefeat_ = false;
	isWin_ = false;

	AttributeLibrary::Map::const_iterator mi;
	FOR_EACH(AttributeLibrary::instance().map(), mi){
		const AttributeBase* attribute = mi->get();
		xassert(attribute);
		if(attribute){
			attributeCacheMap_.insert(AttributeCacheMap::value_type(
				attribute->libraryKey(), AttributeCache(*attribute)));
		}
	}

	for(int i = 0; i < AttributeProjectileTable::instance().size(); i++){
		const AttributeProjectile& projectile = AttributeProjectileTable::instance()[i];
		attributeCacheMap_.insert(AttributeCacheMap::value_type(
			projectile.libraryKey(), AttributeCache(projectile)));
	}

	weaponPrmCache_.resize(WeaponPrmLibrary::instance().strings().size() + 1);
	WeaponPrmLibrary::Strings::const_iterator wmi;
	FOR_EACH(WeaponPrmLibrary::instance().strings(), wmi){
		const WeaponPrm* prm = wmi->get();
		xassert(prm);
		if(prm){
			weaponPrmCache_[prm->ID()].set(prm);
			prm->initCache(weaponPrmCache_[prm->ID()]);
		}
	}

	shootKeyDown_ = false;
	shootPosition_[0] = shootPosition_[1] = Vect3f::ZERO;
	shootPositionUpdateTime_ = 0;
	shootFailed_ = false;

	centerPositionUpdateTime_ = 0;
	centerPosition_ = Vect2f::ZERO;
	maxDistance_ = 0.f;

	aiUpdateIndex_ = 0;
}

void Player::addCooperativePlayer(const PlayerData& playerData)
{
	teamMode_ = true;
	teamNames_[playerData.teamIndex] = playerData.name();
	//cooperativeNames_[cooperativeIdx] = playerName;
}

Player::~Player()
{
	xassert(units_.empty()); // Юниты удаляются в деструкторе Universe
}

void Player::getPlayerData(PlayerDataEdit& playerData) const
{
	playerData.playerID = playerID();

	playerData.hasAutomaticAttackMode = hasAttackMode_;

	if(hasAttackMode_)
		playerData.attackMode = attackMode_;

	playerData.shuffleIndex = shuffleIndex_;
	playerData.setName(name());
	playerData.race = race();
	playerData.colorIndex = colorIndex();
	playerData.signIndex = signIndex();
	playerData.silhouetteColorIndex = silhouetteColorIndex();
	playerData.underwaterColorIndex = underwaterColorIndex();
	playerData.clan = clan();
	playerData.difficulty = difficulty();
	playerData.auxPlayerType = auxPlayerType();
	playerData.auxPlayerType = auxPlayerType();
	playerData.triggerChainNames = triggerChainNames_;
	playerData.startCamera = startCamera_;
	playerData.triggerChainNames.sub(race()->scenarioTriggers);
    playerData.realPlayerType = realPlayerType_;
}

void Player::setPlayerData(const PlayerDataEdit& playerData)
{
	playerID_ = playerData.playerID;

	hasAttackMode_ = playerData.hasAutomaticAttackMode;

	if(playerData.hasAutomaticAttackMode)
		attackMode_ = playerData.attackMode;

	shuffleIndex_ = playerData.shuffleIndex;
	isWorld_ = playerData.realPlayerType == REAL_PLAYER_TYPE_WORLD;
	clan_ = playerData.clan != -1 ? playerData.clan : playerID_;
	name_ = playerData.name();
	setColorIndex(playerData.colorIndex);
	setSignIndex(playerData.signIndex);
	setSilhouetteColorIndex(playerData.silhouetteColorIndex);
	setUnderwaterColorIndex(playerData.underwaterColorIndex);
	race_ = playerData.race;
	difficulty_ = playerData.difficulty;
	auxPlayerType_ = playerData.auxPlayerType;
	triggerChainNames_ = playerData.triggerChainNames;
	startCamera_ = playerData.startCamera;
	triggerChainNames_.sub(race()->scenarioTriggers);
    realPlayerType_ = playerData.realPlayerType;
}

void Player::initFogOfWarMap()
{
	//if(!isWorld() &&  auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER)
	fogOfWarMap_ = universe()->createFogOfWarMap(clan());
}

PlayerDataEdit::PlayerDataEdit()
{
	auxPlayerType = AUX_PLAYER_TYPE_ORDINARY_PLAYER;
	hasAutomaticAttackMode = false;
}

void PlayerDataEdit::serialize(Archive& ar) 
{
	ar.serialize(auxPlayerType, "auxPlayerType", "Тип игрока");
	PlayerData::serialize(ar);

	ar.serialize(hasAutomaticAttackMode, "hasAutomaticAttackMode", "Собственные настройки режимов атаки");
	if(hasAutomaticAttackMode)
		ar.serialize(attackMode, "attackMode", "Настройки режимов атаки");

	ar.serialize(startCamera, "startCamera", "Камера при старте");
	ar.serialize(triggerChainNames, "triggerChainNames", "Триггера для сингла");
}

void TriggerChainName::serialize(Archive& ar)
{
	ar.serialize(ResourceSelector(*this, ResourceSelector::TRIGGER_OPTIONS), "name", "&Имя");
	ar.serialize(restartAlways, "restartAlways", "&Перезапускать в игровом сейве");
}	

string TriggerChainName::shortName() const
{
	int pos = rfind('\\');
	return transliterate((pos == string::npos ? c_str() : string(*this, pos + 1, size() - pos - 1)).c_str());
}

void TriggerChainNames::sub(const TriggerChainNames& commonTriggers)
{
	for(iterator i = begin(); i != end();)
		if(find(commonTriggers.begin(), commonTriggers.end(), *i) != commonTriggers.end())
			i = erase(i);
		else
			++i;
}

bool TriggerChainNames::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(static_cast<vector<TriggerChainName>&>(*this), name, nameAlt);
}

bool CameraSplineName::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit() && ar.isOutput() && cameraManager){
		ComboListString comboListString(cameraManager->splinesComboList().c_str(), c_str());
		return ar.serialize(comboListString, name, nameAlt);
	}
	else
		return ar.serialize(static_cast<string&>(*this), name, nameAlt);
}

STARFORCE_API void Player::serialize(Archive& ar) 
{
	SECUROM_MARKER_HIGH_SECURITY_ON(4);

	ar.serialize(auxPlayerType_, "auxPlayerType", 0);
	if(auxPlayerType_ != AUX_PLAYER_TYPE_ORDINARY_PLAYER){
		ar.serialize(race_, "race", 0);
		clan_ = 64 + playerID_;
		ar.serialize(colorIndex_, "colorIndex", 0);
		ar.serialize(silhouetteColorIndex_, "silhouetteColorIndex", 0);
		ar.serialize(underwaterColorIndex_, "underwaterColorIndex", 0);
		ar.serialize(signIndex_, "signIndex", 0);
		if(ar.isInput()){
			setColorIndex(colorIndex_);
			setSignIndex(signIndex_);
			setSilhouetteColorIndex(silhouetteColorIndex_);
			setUnderwaterColorIndex(underwaterColorIndex_);
		}
	}

	if(ar.isOutput() && !universe()->userSave()) // !!! CONVERSION
		triggerChainNames_.sub(race()->scenarioTriggers);

	ar.serialize(triggerChainNames_, "TriggerChainNames", 0);
	ar.serialize(hasAttackMode_, "hasAttackMode", 0);
	if(hasAttackMode_)
		ar.serialize(attackMode_, "attackMode", 0);

	if(ar.isInput()){
		resource_ = race()->initialResource;
		resourceCapacity_ = race()->resourceCapacity;
		if(auxPlayerType_ == AUX_PLAYER_TYPE_ORDINARY_PLAYER && !isWorld())
			race_->setUsed(unitColor(), unitSign());
	}

	if(universe()->userSave()){
		ar.serialize(resource_, "resource", 0);
		resourcePrev_ = resource_;
		
		if(active())
			if(ar.isOutput()){
				CameraCoordinate coordinate = cameraManager->coordinate();
				ar.serialize(coordinate, "currentCamera", 0);
			}
			else{
				CameraCoordinate coordinate;
				if(ar.serialize(coordinate, "currentCamera", 0))
					cameraManager->setCoordinate(coordinate);
			}
	}
	else{
		resourcePrev_ = resource_ = race()->initialResource;
		
		ar.serialize(startCamera_, "startCamera", 0);
		
		if(ar.isInput()){
			if(isWorld()){
				const TriggerChainNames& worldTriggers = GlobalAttributes::instance().worldTriggers;
				triggerChainNames_.sub(worldTriggers);
				triggerChainNames_.insert(triggerChainNames_.end(), worldTriggers.begin(), worldTriggers.end());
			}
			else if(auxPlayerType_ == AUX_PLAYER_TYPE_ORDINARY_PLAYER){
				race()->setCommonTriggers(triggerChainNames_, universe()->gameType());
				if(active() && !startCamera_.empty()){
					if(const CameraSpline* spline = cameraManager->findSpline(startCamera_.c_str())){
						cameraManager->loadPath(*spline, false);
						cameraManager->startReplayPath(spline->stepDuration(), 1);
					}
				}
			}
		}
	}

	UnitCreatorBase::setPlayer(this);
	if(ar.isInput()){
		if(!universe()->isRandomScenario() || auxPlayerType_ != AUX_PLAYER_TYPE_ORDINARY_PLAYER || isWorld() ) { 
			UnitList units; // addUnit in ObjectCreator
			ar.serialize(units, "units", 0);
			UnitList::iterator unit;
			FOR_EACH(units, unit)
				if(unit->unit())
					unit->unit()->setPose(unit->unit()->pose(), true);
		}
	}
	else{
		UnitList::iterator ui;
		UnitList tempUnits;
		FOR_EACH(units_, ui){
			UnitBase* unit(*ui);
			if(!unit->auxiliary() && unit->player() == this && (!unit->attr().isProjectile() || (universe()->userSave() && !unit->isDocked())))
				tempUnits.push_back(unit);
		}
		ar.serialize(tempUnits, "units", 0);
	}
	
	if(universe()->userSave())
		ar.serialize(resourceCapacity_, "resourceCapacity", 0); // Считается по зданиям, но может измениться по арифметике

	if(ar.isInput())
		Universe::loadProgressUpdate();

	TriggerChains::iterator saveIter = triggerChains_.begin();
	TriggerChainNames::const_iterator ti;
	FOR_EACH(triggerChainNames_, ti){
		if(ar.isInput()){
			triggerChains_.push_back(TriggerChain());
			if(ti->restartAlways || !ar.serialize(triggerChains_.back(), ti->shortName().c_str(), 0))
				triggerChains_.back().load(ti->c_str());
			triggerChains_.back().setAIPlayerAndTriggerChain(this);
		}
		else if(universe()->userSave()){
			if(!ti->restartAlways)
				ar.serialize(*saveIter, ti->shortName().c_str(), 0);
			++saveIter;
		}
	}

	if(universe()->userSave()){
		ar.serialize(isDefeat_, "isDefeat", 0);
		ar.serialize(attributeCacheMap_, "attributeCacheMap", 0);
		ar.serialize(weaponPrmCache_, "weaponPrmCache", 0);
		ar.serialize(intVariables_, "intVariables", 0);
		ar.serialize(playerStatistics_, "playerStatistics", 0); 
	}

	resourceDelta_ = resourcePrev_;
	resourceDelta_.set(0.f);

	if(ar.isInput())
		Universe::loadProgressUpdate();

	SECUROM_MARKER_HIGH_SECURITY_OFF(4);
}

void Player::relaxLoading()
{
	MTG();
	MTL();
	UnitList::iterator u_it;
	FOR_EACH(units_, u_it)
		(*u_it)->relaxLoading();
}

void Player::centerPositionQuant()
{
	++centerPositionUpdateTime_;

	if(centerPositionUpdateTime_ == 5){
		
		centerPositionUpdateTime_ = 0;

		RealUnits::const_iterator i;
		int counter = 0;
		centerPosition_ = Vect2f::ZERO;
		FOR_EACH(realUnits_, i)
			if((*i)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*i)->attr()).includeBase){
				++counter;
				centerPosition_ += (*i)->position2D();
			}

		if(counter)
			centerPosition_ /= counter;

		maxDistance_ = 0.f;
		
		FOR_EACH(realUnits_, i)
			if((*i)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*i)->attr()).includeBase){
				float dist2 = centerPosition_.distance2((*i)->position2D());
				if(dist2 > maxDistance_)
					maxDistance_ = dist2;
			}

		maxDistance_ = sqrt(maxDistance_);

	}
}

STARFORCE_API void Player::createRandomScenario(const Vect2f& location)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(5);

	const AttributeUnitOrBuildingReferences& initialUnits = race()->initialUnits;
	if(!initialUnits.empty()){
		if(initialUnits.size() == 1){
			UnitBase* unit = buildUnit(initialUnits.front());
			unit->setPose(Se3f(QuatF::ID, Vect3f(location, 0)), true);
			if(unit->attr().isLegionary()){
				UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
				UnitSquad* squad = safe_cast<UnitSquad*>(buildUnit(&*legionary->attr().squad));
				squad->setPose(unit->pose(), true);
				squad->addUnit(legionary, false);
			}
		}
		else{
			float radiusMax = 0;
			AttributeUnitOrBuildingReferences::const_iterator i, imax;
			FOR_EACH(initialUnits, i)
				if(radiusMax < (*i)->radius()){
					radiusMax = (*i)->radius();
					imax = i;
				}
			float angle = 0;
			float deltaAngle = 2*M_PI/initialUnits.size();
			FOR_EACH(initialUnits, i){
				UnitBase* unit = buildUnit(*i);
				Vect2f poseShifted = i != imax ? location + Vect2f(cosf(angle), sinf(angle))*radiusMax : location;
				unit->setPose(Se3f(QuatF::ID, Vect3f(poseShifted, 0)), true);
				if(unit->attr().isLegionary()){
					UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
					UnitSquad* squad = safe_cast<UnitSquad*>(buildUnit(&*legionary->attr().squad));
					squad->setPose(unit->pose(), true);
					squad->addUnit(legionary, false);
				}
				angle += deltaAngle;
			}
		}
	}
	if(active()){
		cameraManager->reset();
		Vect2f delta = Vect2f(vMap.H_SIZE/2, vMap.V_SIZE/2) - location;
		float psi = atan2f(delta.y, delta.x);
		const CameraRestriction& restriction = GlobalAttributes::instance().cameraRestriction;
		cameraManager->setCoordinate(CameraCoordinate(location, psi, restriction.CAMERA_THETA_DEFAULT, restriction.CAMERA_ZOOM_DEFAULT));
	}

	SECUROM_MARKER_HIGH_SECURITY_OFF(5);
}

void Player::setColorIndex(int colorIndex)
{
	colorIndex_ = min(colorIndex, GlobalAttributes::instance().playerAllowedColorSize());
	unitColor_ = GlobalAttributes::instance().playerColors[colorIndex_];
}

void Player::setUnderwaterColorIndex(int underwaterColorIndex)
{
	underwaterColorIndex_ = min(underwaterColorIndex, GlobalAttributes::instance().playerAllowedUnderwaterColorSize());
	underwaterColor_ = GlobalAttributes::instance().underwaterColors[underwaterColorIndex_];
}

void Player::setSilhouetteColorIndex(int silhouetteColorIndex) 
{ 
	silhouetteColorIndex_ = min(silhouetteColorIndex, GlobalAttributes::instance().playerAllowedSilhouetteSize());
}

void Player::setSignIndex(int signIndex)
{
	signIndex_ = min(signIndex, GlobalAttributes::instance().playerAllowedSignSize());
	unitSign_ = (signIndex < 0 ? "" : GlobalAttributes::instance().playerSigns[signIndex_].unitTexture);
}

void Player::setAI(bool isAI)
{
	if(isWorld())
		return;
	isAI_ = isAI;
}

Vect2f Player::shootPoint2D() const
{
	Vect3f current;
	current.interpolate(shootPosition_[0], shootPosition_[1], clamp((float)shootPositionUpdateTime_ / universe()->directShootInterpotatePeriod(), 0.f, 1.f));
	return reinterpret_cast<const Vect2f&>(current);
}

void Player::Quant()
{
	MTL();
	
	if(isWorld())
		resource_ = ParameterSet();

	const int ai_update_per_quant = 3;
	int skip_count = 0;
	for(int i = 0; i < ai_update_per_quant; i++){
		UnitActing* unit = 0;
		while(!unit){
			const RealUnits& unitList = realUnits();

			if(!unitList.empty()){
				if(aiUpdateIndex_ >= unitList.size()) aiUpdateIndex_ = 0;

				UnitReal* unitReal = unitList[aiUpdateIndex_];
				if(!unitReal->alive() || !unitReal->attr().isActing()){
					aiUpdateIndex_++;
					if(++skip_count >= unitList.size())
						break;
				}
				else {
					unit = safe_cast<UnitActing*>(unitReal);
					if(!unit->needAiUpdate()){
						unit = 0;
						if(++skip_count >= unitList.size())
							break;
					}

					aiUpdateIndex_++;
				}
			}
			else
				break;
		}

		if(unit)
			unit->toggleAiUpdate();
		else
			break;
	}

	for(int i = 0; i < units_.size();){
		UnitBase* unit = units_[i];
		if(unit->player() == this){
			if(!unit->dead()){
				unit->Quant();
				if(unit->player() == this){
					if(!unit->alive() && unit->isRegisteredInRealUnits())
						removeFromRealUnits(safe_cast<UnitObjective*>(unit));
					++i;
					continue;
				}
			}else{
				if(unit->deadQuant()){
					++i;
					continue;
				}
				unit->removeFromUnitGrid();
				removeUnit(unit);
			}
		}
		CUNITS_LOCK(this);
		units_.erase(units_.begin() + i);
	}
	
	if(active()){
		streamLogicCommand.set(fCommandResetIdleUnits);
		RealUnitsMap::iterator mi;
		FOR_EACH(realUnitsMap_, mi)
			if(mi->first && mi->first->putInIdleList){
				RealUnits::iterator ui;
				FOR_EACH(mi->second, ui)
					if(!(*ui)->isWorking())
						streamLogicCommand.set(fCommandAddIdleUnits) << safe_cast<UnitActing*>(*ui);
			}
	}

	++shootPositionUpdateTime_;

	calculateResourceDelta();
	
	centerPositionQuant();
}

void Player::interfaceToggled()
{
	UnitList::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->updateEffects();
}

void Player::triggerQuant(bool pause) 
{ 
	if(triggersDisabled())
		return;

	TriggerChains::iterator i;
	FOR_EACH(triggerChains_, i)
		i->quant(pause);
}

void Player::checkEvent(const Event& event)
{ 
	start_timer_auto();

	if(!isWin() && !isDefeat())
		playerStatistics_.registerEvent(event, this);

	if(active())
		minimap().checkEvent(event);

	if(triggersDisabled())
		return;

	TriggerChains::iterator i;
	FOR_EACH(triggerChains_, i)
		i->checkEvent(event); 
}

void Player::calculateResourceDelta()
{
	const float tau = 0.05f; // параметр сглаживания для resourceDelta_

	ParameterSet resourceDelta;
	resourceDelta = resource_;
	resourceDelta.sub(resourcePrev_);
	ParameterSet resourceTmp = resourceDelta_;
	resourceTmp.set(0.f);
	resourceTmp.scaleAdd(resourceDelta_, 1 - tau);
	resourceTmp.scaleAdd(resourceDelta, tau);
	resourceDelta_ = resourceTmp;
	resourcePrev_ = resource_;
}

void Player::addResource(const ParameterSet& resource, bool registerEvent) 
{ 
	if(registerEvent)
		playerStatistics_.registerEvent(EventParameters(Event::ADD_RESOURCE, resource), this);
	resource_ += resource; 
	resource_.clamp(resourceCapacity());
	unitNumberManager_.add(resource);
}

void Player::subResource(const ParameterSet& resource) 
{ 
	playerStatistics_.registerEvent(EventParameters(Event::SUB_RESOURCE, resource), this);
	resource_.subClamped(resource);
	unitNumberManager_.sub(resource);
}

bool Player::requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const
{ 
	if(!resource_.above(resource)){
		if(requestResourceType){
			ParameterSet delta = resource;
			delta.subClamped(resource_);
			const_cast<Player*>(this)->checkEvent(EventParameters(Event::LACK_OF_RESOURCE, delta, requestResourceType));
		}
		return false;
	}
	return true;
}

void Player::showEditor()
{
	UnitList::iterator ui;
	FOR_EACH(units_, ui)
		(*ui)->showEditor();
}


//----------------------------------
UnitBase* Player::buildUnit(const AttributeBase* attr)
{
	start_timer_auto();
	UnitBase* p = UnitBase::create(UnitTemplate(attr, this));
	return p;
}

void Player::addUnit(UnitBase* unit)
{
	CUNITS_LOCK(this);

	unit->setPlayer(this);
	units_.push_back(unit);

	if(unit->attr().isObjective()){
		UnitObjective* unitObjective(safe_cast<UnitObjective*>(unit));
		xassert(!unitObjective->isRegisteredInRealUnits());
		unitObjective->setRegisteredInRealUnits(true);
		realUnits_.push_back(unitObjective);
		realUnitsMap_[&unit->attr()].push_back(unitObjective);
		if(!unit->attr().isBuilding())
			addResourceCapacity(unit->attr().resourceCapacity);
		legionariesAndBuildings_ += unit->attr().accountingNumber;
	}

	if(unit->attr().isSquad())
		squads.push_back(safe_cast<UnitSquad*>(unit));
}

void Player::removeFromRealUnits(UnitObjective* unit)
{
	CUNITS_LOCK(this);

	realUnits_.erase(unit);

	RealUnits& list = realUnitsMap_[&unit->attr()];
	list.erase(unit);

	universe()->checkEvent(EventUnitPlayer(Event::DESTROY_OBJECT, unit, this));
	unit->setRegisteredInRealUnits(false);
}

void Player::removeUnit(UnitBase* unit)
{
	CUNITS_LOCK(this);

	if(unit->isRegisteredInRealUnits())
		removeFromRealUnits(safe_cast<UnitObjective*>(unit));
	
	if(unit->attr().isSquad())
		squads.erase(safe_cast<UnitSquad*>(unit));

	if(unit->dead() && !unit->placedIntoDeleteList()){
		unit->setPlacedIntoDeleteList();
		universe()->deleteUnit(unit); 
	}
}

void Player::removeObjectiveUnit(UnitObjective* unit)
{
	legionariesAndBuildings_ -= unit->attr().accountingNumber;

	if(!unit->attr().isBuilding())
		subClampedResourceCapacity(unit->attr().resourceCapacity);
}

void Player::subClampedResourceCapacity(ParameterSet capacity)
{
	resourceCapacity_.subClamped(capacity);
	resource_.clamp(resourceCapacity());
}

void Player::killAllUnits()
{
	MTL();
	UnitList::iterator ui;
	FOR_EACH(units_, ui){
		UnitBase* unit(*ui);
		if(!unit->attr().isReal() || !safe_cast<UnitReal*>(unit)->finishState(StateDeath::instance()))
			unit->Kill();
		unit->deadQuant();
		unit->deadQuant();
	}
}

//---------------------------------------------------------
void Player::sendCommand(const UnitCommand& command)
{
	MTG();
	if(controlEnabled())
 		universe()->sendCommand(netCommand4G_PlayerCommand(playerID(), command));
}

void Player::executeCommand(const UnitCommand& command)
{
	MTL();
	switch(command.commandID()){
	case COMMAND_ID_BUILDING_START: {
		UnitBuilding* building = buildStructure(command.attribute(), command.position(), false);
		if(building){
			UnitBase* unit = command.unit();
			if(unit && unit->attr().isSquad() && command.attribute()->needBuilders)
				safe_cast<UnitSquad*>(unit)->setConstructedBuilding(building, command.shiftModifier());
		}
		} 
		break; 
	case COMMAND_ID_DIRECT_SHOOT:
		shootKeyDown_ = command.commandData();
		shootPosition_[0] = shootPosition_[1] = command.position();
		shootPositionUpdateTime_ = 0;
		break;
	case COMMAND_ID_DIRECT_SHOOT_MOUSE:
		shootPosition_[0] = shootPosition_[1];
		shootPosition_[1] = command.position();
		shootPositionUpdateTime_ = 0;
		break;
	}
}

//-------------------------------------------
UnitReal* Player::findUnit(const AttributeBase* attr)
{
	MTL();
	RealUnits::iterator ui;
	FOR_EACH(realUnits_,ui)
		if(&(*ui)->attr() == attr)
			return (*ui);
		
	return 0;
}

UnitReal* Player::findUnit(const AttributeBase* attr, const Vect2f& nearPosition, float distanceMin, const ConstructionState& state, bool onlyVisible)
{
	MTL();
	UnitReal* bestUnit = 0;
	float dist, bestDist = FLT_INF;
	RealUnits::iterator ui;
	if(attr){
		FOR_EACH(realUnits_,ui)
			if(&(*ui)->attr() == attr && bestDist > (dist = nearPosition.distance2((*ui)->position2D())) && dist > sqr(distanceMin)){
				if(onlyVisible && (*ui)->isUnseen())
					continue;
				if((state & CONSTRUCTED) && (state & CONSTRUCTING) ||
				   (state & CONSTRUCTED) && (*ui)->isConstructed() ||
				   (state & CONSTRUCTING) && !(*ui)->isConstructed()){
					bestDist = dist;
					bestUnit = *ui;
				}
			}
	}
	else{
		///xassert(0);
		FOR_EACH(realUnits_,ui)
			if(!(*ui)->attr().internal && bestDist > (dist = nearPosition.distance2((*ui)->position2D())) && dist > sqr(distanceMin)){
				if(onlyVisible && (*ui)->isUnseen())
					continue;
				if((state & CONSTRUCTED) && (state & CONSTRUCTING) ||
				   (state & CONSTRUCTED) && (*ui)->isConstructed() ||
				   (state & CONSTRUCTING) && !(*ui)->isConstructed()){
					bestDist = dist;
					bestUnit = *ui;
				}
			}
	}

	return bestUnit;
}

UnitReal* Player::findUnitByLabel(const char* label)
{
	MTAuto lock(UnitsLock());
	RealUnits::iterator ui;
	FOR_EACH(realUnits_,ui)
		if(!strcmp((*ui)->label(), label))
			return (*ui);
		
		return 0;
}


//----------------------------------------------

void Player::refreshAttribute()
{
	MTL();
	
	UnitList::iterator ui;
	FOR_EACH(units_,ui)
		(*ui)->refreshAttribute();
}


//-----------------------------------------------

void Player::showDebugInfo()
{
	if(showDebugPlayer.resource && active()){
		Vect3f dir, pos;
		cameraManager->GetCamera()->GetWorldRay(Vect2f(-0.4f, -0.495f), pos, dir);
		resource_.showDebug(pos + dir*300, CYAN);
	}
		
	UnitList::iterator ui;
	FOR_EACH(units_,ui)
		if(!(*ui)->dead())
			if(!showDebugPlayer.showSelectedOnly || (*ui)->selected())
				(*ui)->showDebugInfo();

	if(showDebugPlayer.showStatistic && active())
		playerStatistics_.showDebugInfo();
}

void Player::setActivePlayer(int cooperativeIndex)
{
	active_ = 1;
	name_ = teamNames_[teamIndex_ = cooperativeIndex];
}

void Player::SetDeactivePlayer()
{
	active_ = 0;
}

//--------------------------------

void Player::updateSkinColor()
{
	MTAuto lock(UnitsLock());

	UnitList::iterator ui;
	FOR_EACH(units_,ui)
		(*ui)->updateSkinColor();
}

//--------------------------------------------------
struct terMissileTestOperator
{
	//float Radius;
	Vect3f Position;

 	class RigidBodyBase* SourcePoint;
 	class RigidBodyBase* TargetPoint;
 	class RigidBodyBase* rigidBody;

 	class RigidBodyBase* HitPoint;
 	
	terMissileTestOperator(class RigidBodyBase* object,class RigidBodyBase* source,class RigidBodyBase* target)
	{
		rigidBody = object;
		SourcePoint = source;
		TargetPoint = target;

		Position = rigidBody->position();
		//Radius = rigidBody->boundRadius();

		HitPoint = NULL;
	}

	int operator()(const UnitBase* p)
	{
		if(p->alive() && (p->collisionGroup() & COLLISION_GROUP_REAL)){
			RigidBodyBase* b = p->rigidBody();
			if(b != SourcePoint && b->prm().unit_type == RigidBodyPrm::UNIT){
				ContactInfo contactInfo;
				if(rigidBody->bodyCollision(b, contactInfo)){
					HitPoint = b;
					if(b == TargetPoint)
						return 0;
				}
			}
		}
		return 1;
	}
};

//-----------------------------------------------------------
int Player::countUnits(const AttributeBase* attribute) const
{
	int count = 0;
	const RealUnits& list = realUnits(attribute);
	RealUnits::const_iterator ui;
	FOR_EACH(list, ui)
		if((*ui)->alive())
			++count;
	return count;
}

int Player::countUnitsConstructed(const AttributeBase* attribute) const
{
	int count = 0;
	const RealUnits& list = realUnits(attribute);
	RealUnits::const_iterator ui;
	FOR_EACH(list, ui)
		if((*ui)->alive() && (*ui)->isConstructed())
			++count;
	return count;
}

int Player::countSquads(const AttributeBase* attribute) const
{
	int count = 0;
	SquadList::const_iterator si;
	if (!squads.empty())
		FOR_EACH(squads, si)
			if (&(*si)->attr() == attribute)
				++count;

	return count;
}

bool Player::isEnemy(const Player* player) const 
{ 
	if(player->auxPlayerType() == AUX_PLAYER_TYPE_COMMON_FRIEND || auxPlayerType() == AUX_PLAYER_TYPE_COMMON_FRIEND)
		return false;
	if((player->auxPlayerType() == AUX_PLAYER_TYPE_COMMON_ENEMY || player->isWorld()) && player != this)
		return true; 
	return clan() != player->clan();
}

bool Player::isEnemy(const UnitBase* unit) const
{
	return isEnemy(unit->player()) && !unit->attr().isProjectile();
}

TriggerChain* Player::getStrategyToEdit()
{
	vector<const char*> items;
	TriggerChains::iterator it;
	FOR_EACH(triggerChains_, it)
		items.push_back(!it->name.empty() ? it->name.c_str() : "xxx");

	items.push_back("GlobalTrigger.scr");

	int i = popupMenuIndex(items);
	if(i == -1)
		return 0;
	if(i == triggerChains_.size())
		return 0;
	it = triggerChains_.begin();
	while(i--)
		++it;
	return &*it;
}

const AttributeCache* Player::attributeCache(const AttributeBase* attribute) const
{
	AttributeCacheMap::const_iterator i = attributeCacheMap_.find(attribute->libraryKey());
	if(i != attributeCacheMap_.end())
		return &i->second;
	else
		return 0;
}

const WeaponPrmCache* Player::weaponPrmCache(const WeaponPrm* prm) const
{
	xassert(prm->ID() >= 0 && prm->ID() < weaponPrmCache_.size());

	return &weaponPrmCache_[prm->ID()];
}

void Player::applyParameterArithmetics(const AttributeBase* attribute, const ParameterArithmetics& unitArithmetics)
{
	ParameterArithmetics::Data::const_iterator i;
	FOR_EACH(unitArithmetics.data, i){
		const ArithmeticsData& arithmetics = *i;
		if(arithmetics.unitType & ArithmeticsData::TAKEN_TYPE){
			if(arithmetics.creationType & ArithmeticsData::OLD){
				RealUnits::iterator ui;
				FOR_EACH(realUnits_, ui)
					if(&(*ui)->attr() == attribute)
						(*ui)->applyParameterArithmeticsImpl(arithmetics);
			}
			if(arithmetics.creationType & ArithmeticsData::NEW){
				AttributeCache* cache = const_cast<AttributeCache*>(attributeCache(attribute));
				if(cache)
					cache->applyArithmetics(arithmetics);
			}
		}
		else if(arithmetics.unitType & ArithmeticsData::CHOSEN_TYPES){
			if(arithmetics.creationType & ArithmeticsData::OLD){
				AttributeUnitOrBuildingReferences::const_iterator ai;
				FOR_EACH(arithmetics.attributes, ai){
					RealUnits::iterator ui;
					FOR_EACH(realUnits_, ui)
						if(&(*ui)->attr() == *ai)
							(*ui)->applyParameterArithmeticsImpl(arithmetics);
				}
			}
			if(arithmetics.creationType & ArithmeticsData::NEW){
				AttributeUnitOrBuildingReferences::const_iterator ai;
				FOR_EACH(arithmetics.attributes, ai){
					AttributeCache* cache = const_cast<AttributeCache*>(attributeCache(*ai));
					if(cache)
						cache->applyArithmetics(arithmetics);
				}
			}
		}
		else if(arithmetics.unitType & ArithmeticsData::ALL_TYPES){
			if(arithmetics.creationType & ArithmeticsData::OLD){
				RealUnits::iterator ui;
				FOR_EACH(realUnits_, ui)
					(*ui)->applyParameterArithmeticsImpl(arithmetics);
			}
			if(arithmetics.creationType & ArithmeticsData::NEW){
				AttributeCacheMap::iterator i;
				FOR_EACH(attributeCacheMap_, i)
					i->second.applyArithmetics(arithmetics);
				
				if(arithmetics.weaponType != ArithmeticsData::WEAPON_NONE){
					WeaponPrmCacheVector::iterator it;
					FOR_EACH(weaponPrmCache_, it)
						it->applyArithmetics(arithmetics);
				}
			}
		}

		if(arithmetics.unitType & ArithmeticsData::PLAYER){
			ParameterSet resourcePrev = resource_;
			arithmetics.apply(resource_);
			if(arithmetics.influenceInStatistic_ && arithmetics.operation != ArithmeticsData::SET)
				if(resource_.above(resourcePrev)){
					ParameterSet resources = resource_;
					resources.subClamped(resourcePrev);
					playerStatistics_.registerEvent(EventParameters(Event::ADD_RESOURCE, resources), this);
				}
				else
				{
					ParameterSet resources = resourcePrev;
					resources.subClamped(resource_);
					playerStatistics_.registerEvent(EventParameters(Event::SUB_RESOURCE, resources), this);
				}
			unitNumberManager_.apply(arithmetics);
		}

		if(arithmetics.unitType & ArithmeticsData::PLAYER_CAPACITY)
			arithmetics.apply(resourceCapacity_);
	}
}

bool Player::accessible(const AttributeBase* attribute) const
{
	if(!requestResource(attribute->accessValue, NEED_RESOURCE_TO_ACCESS_UNIT_OR_BUILDING))
		return false;
	return accessibleByBuildings(attribute);
}

bool Player::accessibleByBuildings(const AttributeBase* attribute) const
{
	AccessBuildingsList::const_iterator i;
	FOR_EACH(attribute->accessBuildingsList, i){
		bool thereIs = false;
		AccessBuildings::const_iterator j;
		FOR_EACH(*i, j){
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits(j->building), ui){
				if(!j->needConstructed || (*ui)->isConstructed()){
					thereIs = true;
					break;
				}
			}
		}
		if(!thereIs)
			return false;
	}
	return true;
}

void Player::printAccessible(XBuffer& buffer, const AccessBuildingsList& buildingsList, const char* enabledColor, const char* disabledColor) const
{
	AccessBuildingsList::const_iterator i;
	FOR_EACH(buildingsList, i){
		if(i->empty())
			continue;

		bool thereIs = false;
		AccessBuildings::const_iterator j0 = i->begin();
		AccessBuildings::const_iterator j;
		FOR_EACH(*i, j){
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits(j->building), ui){
				if(!j->needConstructed || (*ui)->isConstructed()){
					thereIs = true;
					j0 = j;
					break;
				}
			}
		}
		buffer < (thereIs ? enabledColor : disabledColor) < j0->building->tipsName.c_str() < ", ";
	}
	if(buffer.tell()){
		buffer -= 2;
		buffer < '\0';
	}
}

bool Player::accessibleByBuildings(const ProducedParameters& parameter) const
{
	AccessBuildingsList::const_iterator i;
	FOR_EACH(parameter.accessBuildingsList, i){
		bool thereIs = false;
		AccessBuildings::const_iterator j;
		FOR_EACH(*i, j){
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits(j->building), ui){
				if(!j->needConstructed || (*ui)->isConstructed()){
					thereIs = true;
					break;
				}
			}
		}
		if(!thereIs)
			return false;
	}
	return true;
}

bool Player::weaponAccessible(const WeaponPrm* weapon) const
{
	AccessBuildingsList::const_iterator i;
	FOR_EACH(weapon->accessBuildingsList(), i){
		bool thereIs = false;
		AccessBuildings::const_iterator j;
		FOR_EACH(*i, j){
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits(j->building), ui){
				if(!j->needConstructed || (*ui)->isConstructed()){
					thereIs = true;
					break;
				}
			}
		if(!thereIs)
			return false;
		}
	}
	return true;
}

string Player::printWeaponAccessible(const WeaponPrm* weapon, const char* enabledColor, const char* disabledColor) const
{
	XBuffer buffer(256, 1);
	AccessBuildingsList::const_iterator i;
	FOR_EACH(weapon->accessBuildingsList(), i){
		if(i->empty())
			continue;

		bool thereIs = true;
		AccessBuildings::const_iterator j;
		FOR_EACH(*i, j){
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits(j->building), ui){
				if(!j->needConstructed || (*ui)->isConstructed()){
					thereIs = true;
					break;
				}
			}
		}
		buffer < (thereIs ? enabledColor : disabledColor) < i->front().building->tipsName.c_str() < ", ";
	}
	if(buffer.tell()){
		buffer -= 2;
		buffer < '\0';
	}
	return buffer.buffer();
}

void Player::addUniqueParameter(const char* parameterName, int value)
{
	if(strlen(parameterName))
		intVariables_[parameterName] = value;
}

void Player::removeUniqueParameter(const char* parameterName)
{
	if(strlen(parameterName)){
		IntVariables::iterator i = intVariables().find(parameterName);
		if(i != intVariables_.end())
			intVariables_.erase(i);
	}
}

bool Player::checkUniqueParameter(const char* parameterName, int value) const
{
	if(!strlen(parameterName) || !intVariables_.exists(parameterName))
		return false;
	return intVariables_[parameterName] >= value;
}

UnitActing* Player::findFreeFactory(const AttributeBase* unitAttribute)
{
	if(unitAttribute){
		AttributeReferences::const_iterator i;
		FOR_EACH(unitAttribute->producedThisFactories, i){
			const RealUnits& units = realUnits(*i);
			RealUnits::const_iterator ui;
			FOR_EACH(units, ui)
				if((*ui)->attr().isActing()){
					UnitActing* unit = safe_cast<UnitActing*>(*ui);
					if(!unit->isProducing() && !unit->isUpgrading() && unit->isConstructed())
						return unit;
				}
		}
	}
	return 0;
}

Accessibility Player::canBuildStructure(const AttributeBase* attr) const
{
	if(!attr)
		return DISABLED;

	if(!requestResource(attr->accessValue, NEED_RESOURCE_SILENT_CHECK))
		return DISABLED;

	if(!checkUnitNumber(attr))
		return ACCESSIBLE;

	if(!accessibleByBuildings(attr))
		return ACCESSIBLE;

	if(!requestResource(attr->installValue, NEED_RESOURCE_SILENT_CHECK))
		return ACCESSIBLE;

	return CAN_START;
}

UnitBuilding* Player::buildStructure(const AttributeBase* buildingAttr, const Vect3f& pos, bool checkPosition)
{
	if(canBuildStructure(buildingAttr) != CAN_START){
		if(active())
			UI_LogicDispatcher::instance().sendBuildMessages(buildingAttr);
		return 0;
	}

	if(checkPosition){
		BuildingInstaller installer;
		installer.InitObject(safe_cast<const AttributeBuilding*>(buildingAttr), false);
		installer.SetBuildPosition(To3D(pos), pos.z);
		installer.environmentAnalysis(this);
		if(!installer.valid())
			return 0;
	}

	subResource(buildingAttr->installValue);

	UnitBuilding* n = safe_cast<UnitBuilding*>(buildUnit(buildingAttr));
	n->registerInPlayerStatistics();
	n->startConstruction();
	n->setPose(Se3f(QuatF(pos.z, Vect3f::K), pos), true);
	return n;
}

int Player::unitNumberMax() const
{
	return round(resource().findByType(ParameterType::NUMBER_OF_UNITS, 100));
}

int Player::checkUnitNumber(const AttributeBase* attribute, const AttributeBase* upgradingAttribute) const
{
	int freeNumber = 100;
	if(attribute->accountingNumber && (!upgradingAttribute || attribute->accountingNumber > upgradingAttribute->accountingNumber)){
		freeNumber = (unitNumberMax() - unitNumber() - unitNumberReserved_ + (upgradingAttribute ? upgradingAttribute->accountingNumber : 0))/attribute->accountingNumber;
		if(freeNumber <= 0)
			return 0;
	}

	int numberByType = unitNumberManager_.number(attribute->formationType);
	if(numberByType != -1){
		int counter = 0;
		RealUnits::const_iterator ui;
		FOR_EACH(realUnits_, ui)
			if((*ui)->attr().formationType == attribute->formationType)
				counter++;
		if(upgradingAttribute && upgradingAttribute->formationType == attribute->formationType)
			counter--;
		freeNumber = min(freeNumber, numberByType - counter);
		if(freeNumber <= 0)
			return 0;
	}
	return freeNumber;
}

void Player::reserveUnitNumber(const AttributeBase* attribute, int counter)
{
	unitNumberReserved_ += counter*attribute->accountingNumber;
	unitNumberManager_.reserve(attribute->formationType, counter);
}

void Player::CollisionQuant()
{
	UnitList::iterator ui;
	FOR_EACH(units_, ui){
		UnitBase* p = *ui;
		if(p->collisionGroup() & COLLISION_GROUP_ACTIVE_COLLIDER && p->alive())
			p->testRealCollision();
	}
}

const RealUnits& Player::realUnits(const AttributeBase* attribute) const 
{ 
	if(realUnitsMap_.exists(attribute))
		return realUnitsMap_[attribute]; 
	else{
		static RealUnits realUnits;
		return realUnits;
	}
} 

void Player::setWin() 
{
	isWin_ = true;
	playerStatistics_.finalize(true);
}

void Player::setDefeat() 
{
	isDefeat_ = true;
	playerStatistics_.finalize(false);
	setAI(false);
}

