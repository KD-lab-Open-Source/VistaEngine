#include "StdAfx.h"
#include "CameraManager.h"
#include "Universe.h"
#include "IronBuilding.h"
#include "Sound.h"
#include "ScanPoly.h"
#include "Triggers.h"
#include "terra.h"
#include "RenderObjects.h"
#include "Serialization.h"
#include "UnitEnvironment.h"
#include "..\Water\Water.h"
#include "..\Environment\Environment.h"
#include "..\Render\src\FogOfWar.h"
#include "PFTrap.h"
#include "RangedWrapper.h"
#include "..\Physics\CD\CD2D.h"

REGISTER_CLASS(AttributeBase, AttributeBuilding, "Здание");
REGISTER_CLASS(UnitBase, UnitBuilding, "Здание")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_BUILDING, UnitBuilding)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeBuilding, PlacementMode, "PlacementMode")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_GROUND_OR_WATER, "Устанавливать на поверхность или воду")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_GROUND, "Устанавливать на поверхность")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_GROUND_DELTA, "Устанавливать на поверхность с дельтой")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_GROUND_DELTA_HEIGTH, "Устанавливать на поверхность с дельтой на определенной высоте")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_WATER_DEPTH, "Устанавливать на воду с определенной глубиной (и глубже)")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_GROUND_OR_UNDERWATER, "Устанавливать на поверхность, можно под водой")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, PLACE_ON_GROUND_DELTA_UNDERWATER, "Устанавливать на поверхность с дельтой, здания остаются под водой")
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeBuilding, PlacementMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeBuilding, InteractionType, "InteractionType")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_NORMAL, "Неразрушаемое здание")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_TREE, "Дерево")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_FENCE, "Забор")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_BARN, "Сарай")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_BUILDING, "Здание")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_PHANTOM, "Фантом")
REGISTER_ENUM_ENCLOSED(AttributeBuilding, INTERACTION_BIG_BUILDING, "Большое здание")
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeBuilding, InteractionType)

BEGIN_ENUM_DESCRIPTOR(BuildingStatus, "BuildingStatus")
REGISTER_ENUM(BUILDING_STATUS_CONSTRUCTED, "Построено")
REGISTER_ENUM(BUILDING_STATUS_PLUGGED_IN, "Включен в интерфейсе")
REGISTER_ENUM(BUILDING_STATUS_CONNECTED, "Здание подключено: Я-Т посредством воздушных связей, остальные - через зеропласт")
REGISTER_ENUM(BUILDING_STATUS_ENABLED, "Разрешено деревом развития")
REGISTER_ENUM(BUILDING_STATUS_POWERED, "Получает энергию, не выводится иконка отсутствия энергии")
REGISTER_ENUM(BUILDING_STATUS_UPGRADING, "Апгрейдится в данный момент")
REGISTER_ENUM(BUILDING_STATUS_MOUNTED, "Разложено")
REGISTER_ENUM(BUILDING_STATUS_HOLD_CONSTRUCTION, "Остановлено строительство")
END_ENUM_DESCRIPTOR(BuildingStatus)

AttributeBuilding::AttributeBuilding() 
{
	unitClass_ = UNIT_CLASS_BUILDING;
	unitAttackClass = ATTACK_CLASS_BUILDING;
	rigidBodyPrm = RigidBodyPrmReference("Building");

	interactionType = INTERACTION_NORMAL;
	placementMode = PLACE_ON_GROUND;
	placementDeltaHeight = 0;
	placementHeight = 0;
	checkUndestructability = false;
	analyzeTerrain = false;
	deviationCosMin = 0;

	installerLight = false;

	cancelConstructionTime = 1;
	
	killAfterToolzerFinished = false;

	teleport = false;
	teleportationTime = 0;

	includeBase = true;
}

void AttributeBuilding::serialize(Archive& ar) 
{
	__super::serialize(ar);
	
	ar.serialize(enablePathTracking, "enablePathTracking", "Включить объезд юнитами");
	ar.serialize(interactionType, "interactionType", "Тип взаимодействия");

	ar.openBlock("placement", "Установка на поверхность");
		if(!ar.serialize(basementExtent, "basementExtent", "Размер площадки (половина от центра)")){
			ar.serialize(basementExtent, "BasementMax", 0);
			if(basementExtent.eq(Vect2f::ZERO))
				basementExtent =  boundBox.max;
		}

		ar.serialize(placementMode, "placementMode", "Способ установки");
		if(placementMode == PLACE_ON_GROUND_DELTA || placementMode == PLACE_ON_GROUND_DELTA_HEIGTH || placementMode == PLACE_ON_GROUND_DELTA_UNDERWATER)
			ar.serialize(placementDeltaHeight, "placementDeltaHeight", "Дельта поверхности в месте установки");
		if(placementMode == PLACE_ON_GROUND_DELTA_HEIGTH)
			ar.serialize(placementHeight, "placementHeight", "Высота поверхности в месте установки");
		if(placementMode == PLACE_ON_WATER_DEPTH)
			ar.serialize(placementHeight, "placementHeight", "Глубина воды в месте установки");
		if(placementMode != PLACE_ON_WATER_DEPTH)
			ar.serialize(checkUndestructability, "checkUndestructability", "Не устанавливать на некопаемости");

		ar.serialize(analyzeTerrain, "analyzeTerrain", "Анализировать изменения поверхности под зданием каждый квант");
		if(analyzeTerrain){
			int angle = round(R2G(acosf(deviationCosMin)));
			ar.serialize(angle, "angleMax", "Максимальное отклонение, градусы");
			deviationCosMin = cosf(G2R(angle));
		}

		ar.serialize(RangedWrapperi(cancelConstructionTime, 0, 10000), "cancelConstructionTime", "Максимальное время, которое недостроенное здание ожидает строителя, секунды");
		cancelConstructionTime = clamp(cancelConstructionTime, 60, 10000);
		ar.serialize(placementZone, "placementZone", "Зона установки");
		ar.serialize(producedPlacementZone, "producedPlacementZone", "Создаваемая зона");
		ar.serialize(producedPlacementZoneRadius, "producedPlacementZoneRadius", "Радиус создаваемой зоны");
		if(ar.isInput() && producedPlacementZoneRadiusMax_ < producedPlacementZoneRadius)
			producedPlacementZoneRadiusMax_ = producedPlacementZoneRadius;

		ar.serialize(toolzer, "toolzer", "Тулзер");
		ar.serialize(killAfterToolzerFinished, "killAfterToolzerFinished", "Уничтожать объект после отработки тулзера");
		ar.serialize(formationType, "formationType", "Тип юнита в формации для задания максимального количества юнитов");
		ar.serialize(accountingNumber, "accountingNumber", "Число, учитываемое в максимальном количестве юнитов");
		ar.serialize(mass, "mass", "Масса");
		ar.serialize(sightRadiusNightFactor, "sightRadiusNightFactor", "Коэффициент для радиуса видимости ночью");
	ar.closeBlock();

	ar.serialize(teleport, "teleport", "Телепорт");
	if(teleport){
		ar.serialize(teleportedUnits, "teleportedUnits", "Телепортируемые юниты");
		ar.serialize(teleportationTime, "teleportationTime", "Время телепортации, секунды");
	}

	ar.serialize(includeBase, "includeBase", "Входит в базу");

	ar.serialize(iconDistanceFactor, "iconDistanceFactor", 0);

	if(ar.isInput()){
		//unitAttackClass = ATTACK_CLASS_BUILDING;
		collisionGroup = interactionType != INTERACTION_PHANTOM ? COLLISION_GROUP_REAL : 0;
		rotToTarget = false;
		if(placementMode == PLACE_ON_GROUND_OR_WATER || placementMode == PLACE_ON_WATER_DEPTH)
			rigidBodyPrm = RigidBodyPrmReference("Building Water");
		else
			rigidBodyPrm = RigidBodyPrmReference("Building");
	}
}

bool AttributeBuilding::canTeleportate(const UnitBase* unit) const
{
	AttributeUnitReferences::const_iterator i;
	FOR_EACH(teleportedUnits, i)
		if(&unit->attr() == *i)
			return true;
	return false;
}

UnitBuilding::UnitBuilding(const UnitTemplate& data) 
:	UnitActing(data)
{
	setBuildingStatus(BUILDING_STATUS_PLUGGED_IN | BUILDING_STATUS_CONSTRUCTED);
	if(!attr().analyzeTerrain)
		makeStatic(STATIC_DUE_TO_BUILDING);
	
	basementInstalled_ = false;

	buildersCounter_ = 0;
	basementInstallPosition_ = Vect2f::ZERO;
	basementInstallAngle_ = 0;

	constructor_ = 0;

	posibleStates_ = UnitBuildingPosibleStates::instance();

	teleportID_ = 0;

	toolzerFinished_ = false;

	resourceCapacityAdded_ = false;
}

void UnitBuilding::setConstructor(UnitLegionary* unit)
{
	constructor_ = unit;
}

UnitBuilding::~UnitBuilding()
{
	xassert(!basementInstalled_);
}

void UnitBuilding::Kill()
{
	if(resourceCapacityAdded_){
		resourceCapacityAdded_ = false;
		player()->subClampedResourceCapacity(attr().resourceCapacity);
	}

	__super::Kill();

	uninstallBasement();
}	

void UnitBuilding::setPose(const Se3f& poseIn, bool initPose)
{
	//Se3f poseZ = poseIn;
	//poseZ.trans().z = attr().buildingPositionZ(poseZ.trans());
	//__super::setPose(poseZ, false);
	__super::setPose(poseIn, initPose);

	if(basementInstallPosition_.distance2(position2D()) > 1.f || initPose){
		uninstallBasement();
		installBasement();
	}

	if(initPose){
		clearRegion(Vect2i(position().xi(), position().yi()), rigidBody()->radius());
		if(!toolzerFinished_ && !attr().toolzer.isEmpty()){
			toolzer_ = attr().toolzer;
			toolzer_.start(pose(), attr().boundScale);
			makeDynamic(STATIC_DUE_TO_BUILDING);
		}
	}
}

void UnitBuilding::Quant()
{
	start_timer_auto();
	
	__super::Quant();
	if(!alive())
		return;

	if(!toolzerFinished_)
		toolzerFinished_ = toolzer_.isFinished();

	if(attr().checkPlacementZone(position2D(), player(), 0))
		setBuildingStatus(buildingStatus() | BUILDING_STATUS_CONNECTED);
	else
		setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_CONNECTED);

	if(isConstructed()){
		ParameterSet recovery;
		recovery.setRecovery(parameters());
		parameters_.scaleAdd(recovery, logicPeriodSeconds);
		parameters_.clamp(parametersMax());

		if(attr().automaticProduction && !producedUnit_)
			executeCommand(UnitCommand(COMMAND_ID_PRODUCE, 0));
	}
	else if(toolzerFinished_){
 		if(attr().killAfterToolzerFinished)
			Kill();
		if(!attr().needBuilders)
			addResourceToBuild(buildConsumer_.delta());
		if(buildingStatus() & BUILDING_STATUS_HOLD_CONSTRUCTION){
			cancelConstructionTimer_.start(attr().cancelConstructionTime*1000);
			constructionInProgressTimer_.start(500);
		}
		if(!cancelConstructionTimer_ && currentState() != CHAIN_UNINSTALL){
			player()->addResource(attr().cancelConstructionValue);
			ShowChangeParametersController doShow(this, attr().cancelConstructionValue);
			startState(StateUninstal::instance());
		}
	}

	if(toolzerFinished_ && currentState() != CHAIN_BIRTH_IN_AIR){
		if(!attr().analyzeTerrain)
			makeStatic(STATIC_DUE_TO_BUILDING);

		if(rigidBody()->rotation().zcol().z < attr().deviationCosMin){
			ParameterSet damage(parameters());
			damage.set(0);
			damage.set(1e+10, ParameterType::HEALTH);
			damage.set(1e+10, ParameterType::ARMOR);
			setDamage(damage, 0);
		}
	}

//	if(player()->GetEvolutionBuildingData(Attribute.ID).Enabled)
		setBuildingStatus(buildingStatus() | BUILDING_STATUS_ENABLED);
//	else
//		setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_ENABLED);

	if(buildingStatus() & BUILDING_STATUS_PLUGGED_IN)
		setBuildingStatus(buildingStatus() | BUILDING_STATUS_POWERED);
	else
		setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_POWERED);

	int flags = BUILDING_STATUS_CONNECTED | BUILDING_STATUS_POWERED | BUILDING_STATUS_ENABLED;
	if((buildingStatus() & flags) == flags && buildingStatus() & (BUILDING_STATUS_CONSTRUCTED | BUILDING_STATUS_UPGRADING)){
		setBuildingStatus(buildingStatus() | BUILDING_STATUS_MOUNTED);
	}
	else
		setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_MOUNTED);

	buildersCounter_ = 0;

	if((isConstructed() && isConnected()) != resourceCapacityAdded_){
		if(resourceCapacityAdded_){
			resourceCapacityAdded_ = false;
			player()->subClampedResourceCapacity(attr().resourceCapacity);
		}else{
			resourceCapacityAdded_ = true;
			player()->addResourceCapacity(attr().resourceCapacity);
		}
	}
}

void UnitBuilding::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(buildingStatus_, "buildingStatus", 0);
	if(!isConstructed()){
		ar.serialize(constructor_, "constructor", 0);
		if(universe()->userSave()){
			ar.serialize(buildConsumer_, "buildConsumer", 0);
			ar.serialize(cancelConstructionTimer_, "cancelConstructionTimer", 0);
			ar.serialize(constructionInProgressTimer_, "constructionInProgressTimer", 0);
		}
	}

	if(attr().teleport)
		ar.serialize(teleportID_, "teleportID", "Номер телепорта для пары");
	
	if(universe()->userSave()){
		ar.serialize(toolzerFinished_, "toolzerFinished", 0);
		ar.serialize(resourceCapacityAdded_, "resourceCapacityAdded", 0);
	}
}

void UnitBuilding::startConstruction(bool afterUpgrade)
{
	setBuildingStatus(buildingStatus() & ~(BUILDING_STATUS_MOUNTED | BUILDING_STATUS_CONSTRUCTED));
	if(afterUpgrade)
		setBuildingStatus(buildingStatus() | BUILDING_STATUS_UPGRADING);
	else
		player()->checkEvent(EventUnitPlayer(Event::STARTED_BUILDING, this, player())); 
	cancelConstructionTimer_.start(attr().cancelConstructionTime*1000);
	constructionInProgressTimer_.start(500);

	stopPermanentEffects();
	
	buildConsumer_.start(this, attr().creationValue, attr().creationTime);
	addResourceToBuild(buildConsumer_.delta());
	buildersCounter_ = 0;
}

float UnitBuilding::constructionsSpeedFactor() const 
{
	return rigidBody()->onWater ?  parameters().findByType(ParameterType::CONSTRUCTION_TIME_FACTOR_ON_WATER, 1) : 1;
}

bool UnitBuilding::addResourceToBuild(const ParameterSet& resource)
{
	buildersCounter_++;
	float progressOld = buildConsumer_.progress();
	float progressReal = parameters().minFraction(parametersMax());
	if(progressOld > progressReal + FLT_EPS)
		buildConsumer_.setProgress(progressReal);

	if(!isConstructed()){
		if(buildConsumer_.addQuant(resource, constructionsSpeedFactor())){
			if(buildingStatus() & ~BUILDING_STATUS_UPGRADING){
				player()->checkEvent(EventUnitPlayer(Event::COMPLETE_BUILDING, this, player()));
				setRegisteredInPlayerStatistics(true);
			}
			setBuildingStatus((buildingStatus() | BUILDING_STATUS_CONSTRUCTED) & ~BUILDING_STATUS_UPGRADING);
			setUpgradeProgresParameters();
			parameters_ = parametersMax();
			stopPermanentEffects();
			startPermanentEffects();
			return true;
		}
	}
	parameters_ = parametersMax();
	float progress = buildConsumer_.progress();
	parameters_.scaleByProgress(progress);
	
	if(progress > progressOld + FLT_EPS){
		cancelConstructionTimer_.start(attr().cancelConstructionTime*1000);
		constructionInProgressTimer_.start(500);
	}
	return false;
}

void UnitBuilding::executeCommand(const UnitCommand& command)
{
	switch(command.commandID()){

		case COMMAND_ID_SELF_ATTACK_MODE:
			setAutoAttackMode((AutoAttackMode)command.commandData());
			break;
		case COMMAND_ID_WALK_ATTACK_MODE:
			setWalkAttackMode((WalkAttackMode)command.commandData());
			break;
		case COMMAND_ID_WEAPON_MODE:
			setWeaponMode((WeaponMode)command.commandData());
			break;
		case COMMAND_ID_AUTO_TARGET_FILTER:
			setAutoTargetFilter((AutoTargetFilter)command.commandData());
			break;

		case COMMAND_ID_PRODUCE:
		case COMMAND_ID_PRODUCE_PARAMETER:
			if(!isConstructed())
				return;
			break;

		case COMMAND_ID_ATTACK:
			selectWeapon(command.commandData());
			setTargetPoint(command.position());
			break;
		case COMMAND_ID_STOP:
			fireStop();
			setManualTarget(NULL);
			selectWeapon(-1);
			setManualTarget(NULL);
			break;

		case COMMAND_ID_UNINSTALL:
			uninstall();
			break;
		case COMMAND_ID_POWER_ON:
			if(isConstructed())
				setBuildingStatus(buildingStatus() | BUILDING_STATUS_PLUGGED_IN);
			break;
		case COMMAND_ID_POWER_OFF:
			if(isConstructed())
				setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_PLUGGED_IN);
			break;

		case COMMAND_ID_HOLD_CONSTRUCTION:
			setBuildingStatus(buildingStatus() | BUILDING_STATUS_HOLD_CONSTRUCTION);
			break;
		case COMMAND_ID_CONTINUE_CONSTRUCTION:
			setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_HOLD_CONSTRUCTION);
			break;

		case COMMAND_ID_AUTO_TRANSPORT_FIND:
			toggleAutoFindTransport(command.commandData());
			break;

	}

	__super::executeCommand(command);
}

void UnitBuilding::uninstall()
{
	player()->checkEvent(EventUnitPlayer(Event::SOLD_BUILDING, this, player()));
	ParameterSet delta = attr().uninstallValue;
	delta *= health();
	player()->addResource(delta);
	ShowChangeParametersController doShow(this, delta);
	startState(StateUninstal::instance());
}

bool UnitBuilding::checkInPathTracking(const UnitBase* tracker) const
{
	if(tracker->rigidBody() && tracker->rigidBody()->isUnit() && safe_cast<RigidBodyUnit*>(tracker->rigidBody())->flyingMode()) {
		Vect3f trackerPoint(tracker->rigidBody()->extent());
		trackerPoint.negate();
		tracker->rigidBody()->orientation().xform(trackerPoint);
		trackerPoint.add(tracker->rigidBody()->centreOfGravity());
		Vect3f buildingPoint(rigidBody()->extent());
		rigidBody()->orientation().xform(buildingPoint);
		buildingPoint.add(rigidBody()->centreOfGravity());
		return buildingPoint.z > trackerPoint.z;
	}

	if(player()->clan() == tracker->player()->clan())
		return true;

	if(attr().interactionType & tracker->attr().environmentDestruction)
		return false;

	return __super::checkInPathTracking(tracker);
}

void UnitBuilding::showDebugInfo()
{
	__super::showDebugInfo();
	
	if(showDebugBuilding.status){
		string str;
		if(buildingStatus() & BUILDING_STATUS_CONSTRUCTED)
			str += "Cs ";
		if(buildingStatus() & BUILDING_STATUS_PLUGGED_IN)
			str += "Pg ";
		if(buildingStatus() & BUILDING_STATUS_CONNECTED)
			str += "Cn ";
		if(buildingStatus() & BUILDING_STATUS_POWERED)
			str += "Pw ";
		if(buildingStatus() & BUILDING_STATUS_ENABLED)
			str += "En ";
		if(buildingStatus() & BUILDING_STATUS_UPGRADING)
			str += "Up ";
		if(buildingStatus() & BUILDING_STATUS_MOUNTED)
			str += "Mo ";
		if(buildingStatus() & BUILDING_STATUS_HOLD_CONSTRUCTION)
			str += "Hl ";
		
		show_text(position(), str.c_str(), CYAN);
	}

	if(showDebugBuilding.basement){
		Vect2i points[4];
		attr().calcBasementPoints(angleZ(), position2D(), points);
		Vect3f p0(points[3].x, points[3].y, position().z);
		for(int i = 0; i < 4; i++){
			Vect3f p(points[i].x, points[i].y, position().z);
			show_line(p, p0, YELLOW);
			p0 = p;
		}
	}
	
	if(show_environment_type)
		show_text(position(), getEnumName(attr().interactionType), CYAN);
}

void UnitBuilding::collision(UnitBase* unit, const ContactInfo& contactInfo)
{
	if(player()->clan() != unit->player()->clan() && (unit->attr().environmentDestruction & attr().interactionType & (ENVIRONMENT_BARN | ENVIRONMENT_BUILDING | ENVIRONMENT_BIG_BUILDING))){
		ParameterSet damage(parameters());
		damage.set(0);
		damage.set(1e+10, ParameterType::HEALTH);
		damage.set(1e+10, ParameterType::ARMOR);
		setDamage(damage, 0);
	}
	if(player()->clan() != unit->player()->clan() && (unit->attr().environmentDestruction & attr().interactionType & (ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2 | ENVIRONMENT_TREE))){
		if(!rigidBody()->isBoxMode()){
			rigidBody()->enableBoxMode();
			rigidBody()->startFall(position());
		}
		return;
	}
	__super::collision(unit, contactInfo);
}

void UnitBuilding::graphQuant(float dt)
{
	if(hiddenGraphic())
		return;

	__super::graphQuant(dt);
	
	if(isConstructed() && attr().iconDistanceFactor && !hiddenGraphic()){
		if(buildingStatus() & BUILDING_STATUS_CONNECTED){
			int flag = BUILDING_STATUS_POWERED | BUILDING_STATUS_ENABLED;
			if((buildingStatus() & flag) != flag){
				//MatXf m = avatar()->matrix();
				//m.trans().z += radius()*attr().iconDistanceFactor;
				//energy_icon_.show(m.trans());
			}
		}
		else{
			//MatXf m = avatar()->matrix();
			//m.trans().z += radius()*attr().iconDistanceFactor;
			//connection_icon_.show(m.trans());
		}
	}
}

void UnitBuilding::changeUnitOwner(Player* playerNew)
{
	if(resourceCapacityAdded_){
		resourceCapacityAdded_ = false;
		player()->subClampedResourceCapacity(attr().resourceCapacity);
	}

	__super::changeUnitOwner(playerNew);

	setBuildingStatus(buildingStatus() & ~BUILDING_STATUS_HOLD_CONSTRUCTION);
}

UnitReal* UnitBuilding::findTeleport() const
{
	const RealUnits& teleports = player()->realUnits(&attr());
	if(teleportID_){
		RealUnits::const_iterator ui;
		FOR_EACH(teleports, ui)
			if(safe_cast<UnitBuilding*>(*ui)->teleportID_ == teleportID_ && *ui != this)
				return *ui;
	}
	else if(teleports.size() > 1){
		int index = logicRND(teleports.size());
		if(this == teleports[index])
			index = (index + 1) % teleports.size();
		return teleports[index];
	}
	return 0;
}

//------------------------------------------
class ScanInstallLineOp
{
public:
	void operator()(int x1,int x2,int y)
	{
		unsigned short* buf = vMap.GABuf + vMap.offsetGBufC(0, y);
		while(x1 <= x2)
			*(buf + vMap.XCYCLG(x1++)) |= GRIDAT_BUILDING;
	}
};

void UnitBuilding::installBasement()
{
	if(alive() && !basementInstalled_){
		basementInstalled_ = true;
		Vect2i points[4];
		attr().calcBasementPoints(basementInstallAngle_ = rigidBody_->angleZ(), basementInstallPosition_ = position2D(), points);
		for(int i = 0; i < 4; i++)
			points[i] >>= kmGrid;
		scanPolyByLineOp(points, 4, ScanInstallLineOp());
	}
}


class ScanUninstallLineOp
{
public:
	void operator()(int x1,int x2,int y)
	{
		unsigned short* buf = vMap.GABuf + vMap.offsetGBufC(0,y);
		while(x1 <= x2)
			*(buf + vMap.XCYCLG(x1++)) &= ~(GRIDAT_BUILDING | GRIDAT_BASE_OF_BUILDING_CORRUPT);
	}
};

void UnitBuilding::uninstallBasement()
{
	if(basementInstalled_){
		basementInstalled_ = false;
		Vect2i points[4];
		attr().calcBasementPoints(basementInstallAngle_, basementInstallPosition_, points);
		for(int i = 0; i < 4; i++)
			points[i] >>= kmGrid;
		scanPolyByLineOp(points, 4, ScanUninstallLineOp());
	}
}

//------------------------------------------
class CheckPlacementZoneOp
{
public:
	CheckPlacementZoneOp(const AttributeBuilding& attribute, const Vect2f& position, Player* player, bool checkConnectionOnly) 
		: attribute_(attribute), position_(position), player_(player), valid_(false), placementPosition_(Vect2f::ZERO), checkConnectionOnly_(checkConnectionOnly)
	{}

	void operator()(UnitBase* p)
	{
		if((p->player() == player_ || p->player()->isWorld()) && 
			p->attr().producedPlacementZone == attribute_.placementZone){
			float dist2 = p->position2D().distance2(position_);
			if(dist2 < sqr(p->attr().producedPlacementZoneRadius)){
				if(checkConnectionOnly_)
					valid_ = true;
                else if(p->attr().isResourceItem()){ 
					if(!p->isUnseen()){
						placementPosition_ = p->position2D();
						if(player_->fogOfWarMap() && player_->fogOfWarMap()->getFogState(placementPosition_.xi(), placementPosition_.yi()) == FOGST_NONE)
							valid_ = true;
					}
				}
				else
					valid_ = true;
			}
		}
	}

	bool valid() const { return valid_; }
	Vect2f placementPosition() { return placementPosition_; }

private:
	Vect2f position_;
	Vect2f placementPosition_;
	const AttributeBuilding& attribute_;
	Player* player_;
	bool checkConnectionOnly_;
	bool valid_;
};

bool AttributeBuilding::checkPlacementZone(const Vect2f& position, Player* player, Vect2f* snapPosition_) const
{
	start_timer_auto();
	if(placementZone){
		CheckPlacementZoneOp op(*this, position, player, !snapPosition_);
		universe()->unitGrid.Scan(position, AttributeBase::producedPlacementZoneRadiusMax(), op);
		if(!op.valid())
			return false;
		if(snapPosition_)
			*snapPosition_ = op.placementPosition();
	}
	return true;
}

class CheckPlaceIsFreeOp
{
public:
	CheckPlaceIsFreeOp(const Vect2f& position, const Vect2f& extent, Mat2f orientation, PlacementZone placementZone, int clan) 
		: position_(position), extent_(extent), orientation_(orientation), placementZone_(placementZone), clan_(clan), valid_(true)
	{}

	void operator()(UnitBase* p)
	{
		if(((p->attr().isBuilding() || (p->attr().isLegionary() && !safe_cast<RigidBodyUnit*>(p->rigidBody())->flyingMode()) && p->player()->clan() != clan_)
			//|| (p->attr().isResourceItem() && placementZone_ != p->attr().producedPlacementZone) 
			|| (p->attr().isEnvironment() && safe_cast<UnitEnvironment*>(p)->checkInBuildingPlacement())) 
			&& penetrationCircleRectangle(p->radius(), p->position2D(), extent_, position_, orientation_))
			valid_ = false;
	}

	bool valid() const { return valid_; }

private:
	Vect2f position_;
	Vect2f extent_;
	Mat2f orientation_;
	PlacementZone placementZone_;
	int clan_;
	bool valid_;
};

bool AttributeBuilding::checkBuildingPosition(const Vect2f& position, const Mat2f& orientation, Player* player, bool checkUnits, Vect2f& snapPosition_) const
{
	if (player->fogOfWarMap() && player->fogOfWarMap()->getFogState((int) position.x, (int) position.y) == FOGST_FULL)
		return false;

	int zMin, zMax;
	vMap.findMinMaxInArea(position, boundRadius, zMin, zMax);

	int zWaterMin, zWaterMax;
	environment->water()->findMinMaxInArea(position, boundRadius, zWaterMin, zWaterMax);

	if(zWaterMax < environment->waterPathFindingHeight())
		zWaterMax = 0;

	switch(placementMode){
	case PLACE_ON_GROUND:
		if(zWaterMax)
			return false;
		break;
	case PLACE_ON_GROUND_DELTA:
	case PLACE_ON_GROUND_DELTA_UNDERWATER:
		if(zWaterMax || zMax - zMin > placementDeltaHeight)
			return false;
		break;
	case PLACE_ON_GROUND_DELTA_HEIGTH:
		if(zWaterMax || zMax - zMin > placementDeltaHeight 
		  || zMax - placementHeight > placementDeltaHeight
		  || placementHeight - zMin > placementDeltaHeight)
			return false;
		break;
	case PLACE_ON_WATER_DEPTH:
		if(!zWaterMin)
			return false;
		break;
	}

	if(checkUndestructability && vMap.checkUndestructability(position, boundRadius))
		return false;

	if(checkUnits){
		CheckPlaceIsFreeOp op(position, basementExtent, orientation, placementZone, player->clan());
		universe()->unitGrid.Scan(position, basementExtent.norm(), op);
		if(!op.valid())
			return false;
	}

	return checkPlacementZone(position, player, &snapPosition_);
}

float AttributeBuilding::buildingPositionZ(const Vect2f& position) const
{
	Vect3f normal;
	float z;
	switch(placementMode){
	case PLACE_ON_GROUND:
	case PLACE_ON_GROUND_DELTA:
	case PLACE_ON_GROUND_OR_UNDERWATER:
	case PLACE_ON_GROUND_DELTA_UNDERWATER:
		z = vMap.analyzeArea(position, boundRadius, normal);
		break;
	case PLACE_ON_GROUND_DELTA_HEIGTH:
		z = placementHeight;
		break;
	case PLACE_ON_GROUND_OR_WATER:
	case PLACE_ON_WATER_DEPTH: 
		z = environment->water()->analyzeArea(position, boundRadius, normal);
		break;
	default:
		xassert(0);
	}
	return z - boundBox.min.z;
}

