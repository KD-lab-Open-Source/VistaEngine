#include "StdAfx.h"
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
#include "Serialization\Serialization.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\SerializationFactory.h"
#include "Installer.h"
#include "EventParameters.h"
#include "GameCommands.h"
#include "Environment\Environment.h"
#include "Environment\SourceManager.h"
#include "Environment\Anchor.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\UI_Minimap.h"
#include "Serialization\StringTable.h"
#include "Terra\vMap.h"
#include "VistaRender\Field.h"
#include "SurMap5\UniverseObjectAction.h"
#include "FileUtils\FileUtils.h"
#include "WBuffer.h"
#include "UnicodeConverter.h"
#include "Render\src\Scene.h"

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

class PlayerFactoryArg0 
{
protected:
	Player* createArg() { // заглушка - игроки создаются addPlayer
		xassert(0);
		return 0;
	}
};

typedef SerializationFactory<Player, PlayerFactoryArg0> PlayerSerializationFactory;

template<>
struct FactorySelector<Player>
{
	typedef PlayerSerializationFactory Factory;
};


//-------------------------------------
Player::Player(const PlayerData& playerData) 
: fogOfWarMap_(0)
{
	playerID_ = playerData.playerID;
	teamIndex_ = playerData.teamIndex;
	a2w(teamNames_[teamIndex_], playerData.name());
	teamMode_ = false;
	a2w(name_, playerData.name());
	shuffleIndex_ = playerData.shuffleIndex != -1 ? playerData.shuffleIndex : playerID_;
	clan_ = playerData.clan != -1 ? playerData.clan : 64 + playerID_;
	isWorld_ = playerData.realPlayerType == REAL_PLAYER_TYPE_WORLD;
	auxPlayerType_ = AUX_PLAYER_TYPE_ORDINARY_PLAYER;
	realPlayerType_ = playerData.realPlayerType;

	playerUnit_ = 0;

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

	unitNumber_ = 0;
	unitNumberReserved_ = 0;

	scores_ = 0;

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

	for(int i = 0; i < AttributeSquadTable::instance().size(); i++){
		const AttributeSquad& attrSquad = AttributeSquadTable::instance()[i];
		squadsTypeMap_[&attrSquad];
		attributeCacheMap_.insert(AttributeCacheMap::value_type(
			attrSquad.libraryKey(), AttributeCache(attrSquad)));
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
	shootPositionUpdateTime_ = universe()->quantCounter();
	shootPositionNeedUpdate_ = 0;
	shootFailed_ = false;

	centerPositionUpdateTime_ = 0;
	centerPosition_ = Vect2f::ZERO;
	maxDistance_ = 0.f;

	aiUpdateIndex_ = 0;

	if(GlobalAttributes::instance().enableFieldDispatcher){
		fieldDispatcher_ = new FieldDispatcher(vMap.H_SIZE, vMap.V_SIZE);
		terScene->AttachObj(fieldDispatcher_);
	}
	else
		fieldDispatcher_ = 0;
}

void Player::addCooperativePlayer(const PlayerData& playerData)
{
	teamMode_ = true;
	a2w(teamNames_[playerData.teamIndex], playerData.name());
	//cooperativeNames_[cooperativeIdx] = playerName;
}

Player::~Player()
{
	xassert(units_.empty()); // Юниты удаляются в деструкторе Universe
	if(fieldDispatcher_)
		fieldDispatcher_->Release();
}

void Player::getPlayerData(PlayerDataEdit& playerData) const
{
	playerData.playerID = playerID();

	playerData.hasAutomaticAttackMode = hasAttackMode_;

	if(hasAttackMode_)
		playerData.attackMode = attackMode_;

	playerData.shuffleIndex = shuffleIndex_;
	playerData.setName(w2a(name()).c_str());
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
	a2w(name_, playerData.name());
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
	static ResourceSelector::Options triggerOptions("*.scr", "Scripts\\Content\\Triggers", "Trigger Chain Name", false);
	ar.serialize(ResourceSelector(*this, triggerOptions), "name", "&Имя");
	ar.serialize(restartAlways, "restartAlways", "&Перезапускать в игровом сейве");
}	

string TriggerChainName::shortName() const
{
	return transliterate(extractFileName(c_str()).c_str());
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
	if(ar.isEdit() && cameraManager){
		ComboListString comboListString(cameraManager->splinesComboList().c_str(), c_str());
		bool result = ar.serialize(comboListString, name, nameAlt);
		if(ar.isInput())
			static_cast<string&>(*this) = comboListString;
		return result;
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
		unitNumberByType_ = resourceCapacity_;
		unitNumberByType_.set(0);
		unitNumberReservedByType_ = resourceCapacity_;
		unitNumberReservedByType_.set(0);
		if(auxPlayerType_ == AUX_PLAYER_TYPE_ORDINARY_PLAYER && !isWorld())
			race_->setUsed(unitColor(), unitSign());
	}

	if(universe()->userSave()){
		ar.serialize(resource_, "resource", 0);
		resourcePrev_ = resource_;

		ar.serialize(showAssemblyCommandTimer_, "showAssemblyCommandTimer", 0);

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
		playerStatistics_.registerEvent(EventParameters(Event::ADD_RESOURCE, resource_), this);		

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

	ar.serialize(assemblyPosition_, "assemblyPosition", 0);

	UnitSerializationFactory::setPlayer(this);
	if(ar.isInput()){
		if(!universe()->isRandomScenario() || auxPlayerType_ != AUX_PLAYER_TYPE_ORDINARY_PLAYER || isWorld() ) { 
			UnitList units; // addUnit in ObjectCreator
			ar.serialize(units, "units", 0);
			UnitList::iterator unit;
			if(universe()->universeObjectAction){
				UniverseObjectAction& action = *universe()->universeObjectAction;
				FOR_EACH(units, unit){
		            UnitBase* iunit = *unit;
					if(iunit && !iunit->auxiliary() && !iunit->attr().isSquad())
						action(*iunit);
				}
			}
			FOR_EACH(units, unit)
				if(unit->unit())
					unit->unit()->setPose(unit->unit()->pose(), true);
		}

		ar.serialize(playerUnit_, "playerUnit", 0);
		if(race()->playerUnitAttribute() && !playerUnit_){
			playerUnit_ = static_cast<UnitPlayer*>(buildUnit(race()->playerUnitAttribute()));
			playerUnit_->setPose(Se3f(QuatF::ID, To3D(Vect2f(vMap.H_SIZE/2, vMap.V_SIZE/2))), true);
		}
	}
	else{
		UnitList::iterator ui;
		UnitList tempUnits;
		FOR_EACH(units_, ui){
			UnitBase* unit(*ui);
			if(!unit->auxiliary() && unit->player() == this && (!unit->attr().isProjectile() || (universe()->userSave() && !unit->isDocked())) && (!unit->attr().isPlayer() || unit == playerUnit_))
				tempUnits.push_back(unit);
		}
		ar.serialize(tempUnits, "units", 0);
		ar.serialize(playerUnit_, "playerUnit", 0);
	}
	
	if(universe()->userSave())
		ar.serialize(resourceCapacity_, "resourceCapacity", 0); // Считается по зданиям, но может измениться по арифметике

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

	SECUROM_MARKER_HIGH_SECURITY_OFF(4);
}

void Player::relaxLoading()
{
	MTG();
	MTL();

	if(isUnderEditor()){
		if(assemblyPosition_ && !universe()->userSave()){
			sourceManager->removeAnchor(assemblyPosition_);
			assemblyPosition_ = 0;
		}
	}
	else if(!assemblyPosition_){
		Anchor* anchor = sourceManager->addAnchor(&race()->anchorForAssemblyPoint());
		anchor->setLabel(XBuffer() < "Player" <= playerID_ < "_AssemblyPosition");
		anchor->enable();
		assemblyPosition_ = anchor;
		xassert(assemblyPosition_);
	}

	for(int i = 0; i < units_.size(); i++)
		units_[i]->relaxLoading();
}

void Player::centerPositionQuant()
{
	if(!centerPositionUpdateTime_--){
		centerPositionUpdateTime_ = 5;

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
				squad->addUnit(legionary);
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
					squad->addUnit(legionary);
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
		cameraManager->setCoordinate(CameraCoordinate(location, psi, restriction.thetaDefault, 0, restriction.zoomDefault));
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

bool Player::shootPointValid() const
{
	return universe()->quantCounter() - shootPositionUpdateTime_ <= universe()->directShootInterpotatePeriod();
}

Vect2f Player::shootPoint2D() const
{
	Vect3f current;
	return reinterpret_cast<const Vect2f&>(current.interpolate(shootPosition_[0], shootPosition_[1], 
		clamp((float)(universe()->quantCounter() - shootPositionUpdateTime_) / universe()->directShootInterpotatePeriod(), 0.f, 1.f)));
}

bool Player::updateShootPoint() const
{
	return shootPositionNeedUpdate_ > universe()->quantCounter();
}

void Player::setUpdateShootPoint()
{
	shootPositionNeedUpdate_ = universe()->quantCounter() + 2;
}

void Player::Quant()
{
	MTL();
	
	if(isWorld())
		resource_ = ParameterSet();
	
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

	calculateResourceDelta();
	
	centerPositionQuant();

	if(assemblyPosition_){
		if(showAssemblyCommandTimer_.busy() && assemblyPosition_->selected()){
			UI_LogicDispatcher::instance().addMark(
				UI_MarkObjectInfo(UI_MARK_ASSEMBLY_POINT, assemblyPosition_->pose(), &race()->orderMark(UI_CLICK_MARK_ASSEMBLY_POINT), assemblyPosition_));
		}
		else
			assemblyPosition_->setSelected(false);
	}
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
}

void Player::subResource(const ParameterSet& resource) 
{ 
	playerStatistics_.registerEvent(EventParameters(Event::SUB_RESOURCE, resource), this);
	resource_.subClamped(resource);
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
	UnitList unitsTmp;
	unitsTmp.swap(units_);

	UnitList::iterator ui;
	FOR_EACH(unitsTmp, ui)
		(*ui)->showEditor();

	unitsTmp.insert(unitsTmp.end(), units_.begin(), units_.end());
	unitsTmp.swap(units_);
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
	}

	if(unit->attr().isSquad()){
		squads_.push_back(safe_cast<UnitSquad*>(unit));
		squadsTypeMap_[&safe_cast<UnitSquad*>(unit)->attr()].push_back(safe_cast<UnitSquad*>(unit));
	}

	if(unit->attr().isObjective() || unit->attr().isSquad()){
		unitNumber_ += unit->attr().accountingNumber;
		unitNumberByType_.addByIndex(unit->attr().accountingNumber, unit->attr().unitNumberMaxType.key());
	}
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
	
	if(unit->attr().isSquad()){
		squads_.erase(safe_cast<UnitSquad*>(unit));
		squadsTypeMap_[&safe_cast<UnitSquad*>(unit)->attr()].erase(safe_cast<UnitSquad*>(unit));
	}

	if(unit->dead() && !unit->placedIntoDeleteList()){
		unit->setPlacedIntoDeleteList();
		universe()->deleteUnit(unit); 
	}
}

void Player::removeUnitAccount(UnitInterface* unit)
{
	const AttributeBase& attr = unit->attr();
	unitNumber_ -= attr.accountingNumber;
	unitNumberByType_.addByIndex(-attr.accountingNumber, attr.unitNumberMaxType.key());

	if(!attr.isBuilding() && !attr.isSquad())
		subClampedResourceCapacity(attr.resourceCapacity);
}

void Player::subClampedResourceCapacity(const ParameterSet& capacity)
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

void Player::directShootCommand(const Vect3f& pos, unsigned int data)
{
	shootKeyDown_ = data;
	shootPosition_[0] = shootPosition_[1] = pos;
	shootPositionUpdateTime_ = universe()->quantCounter();
}

void Player::directShootCommandMouse(const Vect3f& pos)
{
	shootPosition_[0] = shootPosition_[1];
	shootPosition_[1] = pos;
	shootPositionUpdateTime_ = universe()->quantCounter();
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
		if(!isUsedByTriggerAttack())
			directShootCommand(command.position(), command.commandData());
		break;
	case COMMAND_ID_DIRECT_SHOOT_MOUSE:
		if(!isUsedByTriggerAttack())
			directShootCommandMouse(command.position());
		break;
	case COMMAND_ID_DIRECT_MOUSE_SET:
		if(!isUsedByTriggerAttack())
			directShootCommand(command.position(), shootKeyDown_);
		break;
	case COMMAND_ID_ASSEMBLY_POINT:
		showAssemblyCommand(command.position(), command.commandData());
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

//-----------------------------------------------

void Player::showDebugInfo()
{
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

int Player::countSquads(const AttributeSquad* attribute) const
{
	MTL();
	xassert(squadsTypeMap_.exists(safe_cast<const AttributeSquad*>(attribute)));
	return squadsTypeMap_[safe_cast<const AttributeSquad*>(attribute)].size();
}

bool Player::isEnemy(const Player* player) const 
{ 
	//if(player->isWorld()) // всем друг
	//	return false;
	if(player->auxPlayerType() == AUX_PLAYER_TYPE_COMMON_FRIEND || auxPlayerType() == AUX_PLAYER_TYPE_COMMON_FRIEND)
		return false;
	if((player->auxPlayerType() == AUX_PLAYER_TYPE_COMMON_ENEMY || player->isWorld())  && player != this)
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
	if(!attribute)
		return 0;
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
				RealUnits::iterator ui;
				FOR_EACH(realUnits_, ui)
					if(&(*ui)->attr() == arithmetics.attribute)
						(*ui)->applyParameterArithmeticsImpl(arithmetics);
			}
			if(arithmetics.creationType & ArithmeticsData::NEW){
				AttributeCache* cache = const_cast<AttributeCache*>(attributeCache(arithmetics.attribute));
				if(cache)
					cache->applyArithmetics(arithmetics);
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
		}

		if(arithmetics.unitType & ArithmeticsData::PLAYER_CAPACITY){
			arithmetics.apply(resourceCapacity_);
			resource_.clamp(resourceCapacity());
		}
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

void Player::printAccessible(WBuffer& buffer, const AccessBuildingsList& buildingsList, const wchar_t* enabledColor, const wchar_t* disabledColor) const
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
		buffer < (thereIs ? enabledColor : disabledColor) < j0->building->tipsName.c_str() < L", ";
	}
	if(buffer.tell()){
		buffer -= 2;
		buffer < L'\0';
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

const wchar_t* Player::printWeaponAccessible(WBuffer& buffer, const WeaponPrm* weapon, const wchar_t* enabledColor, const wchar_t* disabledColor) const
{
	buffer.init();
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
		buffer < (thereIs ? enabledColor : disabledColor) < i->front().building->tipsName.c_str() < L", ";
	}
	if(buffer.tell()){
		buffer -= 2;
		buffer < L'\0';
	}
	return buffer.c_str();
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

UnitActing* Player::findFreeFactory(const AttributeBase* unitAttribute, int priority)
{
	if(unitAttribute){
		AttributeReferences::const_iterator i;
		FOR_EACH(unitAttribute->producedThisFactories, i){
			const RealUnits& units = realUnits(*i);
			RealUnits::const_iterator ui;
			FOR_EACH(units, ui)
				if((*ui)->attr().isActing()){
					UnitActing* unit = safe_cast<UnitActing*>(*ui);
					if(priority && unit->usedByTrigger(priority))
						continue;
					if(!unit->isProducing() && !unit->isUpgrading() && unit->isConstructed() && unit->isConnected())
						return unit;
				}
		}
	}
	return 0;
}

Accessibility Player::canBuildStructure(const AttributeBase* attr, const AttributeBase* attrSquad) const
{
	if(!attr)
		return DISABLED;

	if(!requestResource(attr->accessValue, NEED_RESOURCE_SILENT_CHECK))
		return DISABLED;

	if(!checkUnitNumber(attr, attrSquad))
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
		installer.SetBuildPosition(pos, pos.z);
		installer.environmentAnalysis(this);
		if(!installer.valid())
			return 0;
	}

	subResource(buildingAttr->installValue);

	UnitBuilding* n = safe_cast<UnitBuilding*>(buildUnit(buildingAttr));
	n->setRegisteredInPlayerStatistics(false);
	n->startConstruction();
	n->setPose(Se3f(QuatF(pos.z, Vect3f::K), pos), true);
	return n;
}

int Player::unitNumber(ParameterTypeReferenceZero type) const
{
	if(type.key() == -1)
		return unitNumber_;
	else
		return round(unitNumberByType_.findByIndex(type.key(), unitNumber_));
}

int Player::unitNumberMax(ParameterTypeReferenceZero type) const
{
	int unitMax = round(resource().maxByType(ParameterType::NUMBER_OF_UNITS));
	if(type.key() == -1)
		return unitMax;
	else
		return round(resource().findByIndex(type.key(), unitMax));
}

int Player::unitNumberMaxReal(ParameterTypeReferenceZero type) const
{
	if(type.key() == -1)
		return 1000;
	else
		return round(unitNumberMax(type) - unitNumber(type) - unitNumberReservedByType_.findByIndex(type.key(), unitNumberReserved_));
}

int Player::checkUnitNumber(const AttributeBase* attribute, const AttributeBase* upgradingAttribute) const
{
	if(attribute->isLegionary()){
		const AttributeBase* squad = safe_cast<const AttributeLegionary*>(attribute)->squad;
		int squadNumber = unitNumberMaxReal(squad->unitNumberMaxType);
		if(upgradingAttribute){
			if(upgradingAttribute->isLegionary()){
				const AttributeBase* upgradingSquad = safe_cast<const AttributeLegionary*>(upgradingAttribute)->squad;
				squadNumber += upgradingSquad->accountingNumber;
				if(squad->unitNumberMaxType == upgradingSquad->unitNumberMaxType && squad->accountingNumber == upgradingSquad->accountingNumber)
					squadNumber = 100;
			}
			else if(upgradingAttribute == squad) // Добавление в сквад
				squadNumber += squad->accountingNumber;
		}
		if(squadNumber < squad->accountingNumber)
			return 0;
	}

	if(!attribute->accountingNumber)
		return 100;

	int freeNumber = unitNumberMaxReal(attribute->unitNumberMaxType);
	if(upgradingAttribute){
		if(attribute->unitNumberMaxType == upgradingAttribute->unitNumberMaxType && attribute->accountingNumber == upgradingAttribute->accountingNumber)
			return 1;
		freeNumber += upgradingAttribute->accountingNumber;
	}
	freeNumber /= attribute->accountingNumber;
	return max(freeNumber, 0);
}

void Player::reserveUnitNumber(const AttributeBase* attribute, int counter)
{
	unitNumberReserved_ += counter*attribute->accountingNumber;
	unitNumberReservedByType_.addByIndex(counter*attribute->accountingNumber, attribute->unitNumberMaxType.key());
	xassert(unitNumberReservedByType_.findByIndex(attribute->unitNumberMaxType.key(), 0) >=0);
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

const SquadList& Player::squadList(const AttributeSquad* attribute) const
{
	xassert(squadsTypeMap_.exists(safe_cast<const AttributeSquad*>(attribute)));
	SquadsTypeMap::const_iterator it = squadsTypeMap_.find(attribute);
	if(it != squadsTypeMap_.end())
		return it->second;
	static SquadList staticSquadList;
	return staticSquadList;
}

void Player::setWin() 
{
	isWin_ = true;
	playerStatistics_.finalize(true);
}

void Player::setDefeat() 
{
	if(!isWin()){
		isDefeat_ = true;
		playerStatistics_.finalize(false);
	}
	setAI(false);
}

void Player::startTigger(const TriggerChainName& triggerChainName)
{
	triggerChainNames_.push_back(triggerChainName);
	triggerChains_.push_back(TriggerChain());
	triggerChains_.back().load(triggerChainName.c_str());
	triggerChains_.back().setAIPlayerAndTriggerChain(this);
}

const Anchor* Player::requestedAssemblyPosition() const
{
	if(const Anchor* anchor = assemblyPosition_)
		if(showAssemblyCommandTimer_.busy())
			return anchor;
	return 0;
}

void Player::showAssemblyCommand(const Vect3f& pos, unsigned int data)
{
	xassert(assemblyPosition_);

	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi)
		if((*pi)->clan() == clan() && (*pi) != this)
			(*pi)->checkEvent(EventUnitPlayer(Event::REQUEST_ASSEMBLY, 0, this));

	if(clan() != universe()->activePlayer()->clan())
		return;

	if(data){
		assemblyPosition_->setPose(Se3f(QuatF::ID, pos), true);
		assemblyPosition_->setSelected(true);
		showAssemblyCommandTimer_.start(GlobalAttributes::instance().assemblyCommandShowTime * 1000);
	}
	else {
		showAssemblyCommandTimer_.stop();
		assemblyPosition_->setSelected(false);
	}
}

void Player::drawDebug2D(XBuffer& buffer)
{
	if(showDebugPlayer.resource)
		buffer < "\nActive player resource:\n" < resource().debugStr();

	if(showDebugPlayer.resourceCapacity)
		buffer < "\nActive player capacity:\n" < resourceCapacity().debugStr();

	if(showDebugPlayer.unitNumber)
		buffer < "\nActive player unitNumber:\nОбщее: " <= unitNumber_ < "\n" < unitNumberByType_.debugStr();

	if(showDebugPlayer.unitNumberReserved)
		buffer < "\nActive player unitNumberReserved:\nОбщее: " <= unitNumberReserved_ < "\n" < unitNumberReservedByType_.debugStr();
}

void Player::startUsedByTriggerAttack(int time)
{
	usedByTriggerTimer_.start(time);
}

void UnitSerializer::serialize(Archive& ar)
{
	AttributeType attributeType = unit_ ? unit_->attr().attributeType() : ATTRIBUTE_NONE;
	ar.serialize(attributeType, "attributeType", 0);
	switch(attributeType){
		case ATTRIBUTE_LIBRARY: {
			AttributeReference attribute = unit_ ? &unit_->attr() : 0;
			ar.serialize(attribute, "attribute", 0);
			UnitSerializationFactory::setAttribute(attribute);
			xassertStr(ar.isOutput() || attribute, (string("Не найден юнит в библиотеке: ") + attribute.c_str()).c_str());
			break; }
		case ATTRIBUTE_AUX_LIBRARY: {
			AuxAttributeReference auxAttribute = AuxAttributeReference(unit_ ? &unit_->attr() : 0);
			ar.serialize(auxAttribute, "auxAttribute", 0);
			UnitSerializationFactory::setAttribute(auxAttribute);
			xassertStr(ar.isOutput() || auxAttribute, (string("Не найден юнит в библиотеке: ") + auxAttribute.c_str()).c_str());
			break; }
		case ATTRIBUTE_SQUAD: {
			AttributeSquadReference attributeSquad(unit_ ? safe_cast_ref<const AttributeSquad&>(unit_->attr()).c_str() : "");
			ar.serialize(attributeSquad, "attributeSquad", 0);
			UnitSerializationFactory::setAttribute(&*attributeSquad);
			break; }
		case ATTRIBUTE_PROJECTILE: {
			AttributeProjectileReference attributeProjectile(unit_ ? safe_cast_ref<const AttributeProjectile&>(unit_->attr()).c_str() : "");
			ar.serialize(attributeProjectile, "attributeProjectile", 0);
			UnitSerializationFactory::setAttribute(&*attributeProjectile);
			break; }
		case ATTRIBUTE_NONE: 
			xassert(0 && "Неизвестный юнит на мире");
			break; 
	}

	PolymorphicWrapper<UnitBase> unitBase = unit_;
	ar.serialize(unitBase, "unit", 0);
	unit_ = safe_cast<UnitBase*>(unitBase.get());
}
