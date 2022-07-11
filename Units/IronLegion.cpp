#include "StdAfx.h"
#include "Universe.h"

#include "Squad.h"
#include "Sound.h"
#include "Triggers.h"
#include "terra.h"
#include "Dictionary.h"
#include "Serialization.h"
#include "IronBuilding.h"
#include "UnitItemInventory.h"
#include "UnitItemResource.h"
#include "RangedWrapper.h"
#include "..\Game\CameraManager.h"
#include "..\UserInterface\UI_Render.h"
#include "..\Environment\Environment.h"
#include "TypeLibraryImpl.h"
#include "..\Render\src\Grass.h"
#include "Console.h"
#include "..\Water\Water.h"
#include "..\Water\WaterWalking.h"
#include "RenderObjects.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\UserInterface\SelectManager.h"

int targetEventTime = 1000;

float squad_speed_correction_relaxation_time_inv = 0.25f;
float squad_speed_factor_modulation = 0.25f;

UNIT_LINK_GET(UnitLegionary)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UnitLegionary, ResourcerMode, "ResourcerMode")
REGISTER_ENUM_ENCLOSED(UnitLegionary, RESOURCER_IDLE, "RESOURCER_IDLE")
REGISTER_ENUM_ENCLOSED(UnitLegionary, RESOURCER_FINDING, "RESOURCER_FINDING")
REGISTER_ENUM_ENCLOSED(UnitLegionary, RESOURCER_PICKING, "RESOURCER_PICKING")
REGISTER_ENUM_ENCLOSED(UnitLegionary, RESOURCER_RETURNING, "RESOURCER_RETURNING")
END_ENUM_DESCRIPTOR_ENCLOSED(UnitLegionary, ResourcerMode)

REGISTER_CLASS(AttributeBase, AttributeLegionary, "Юнит");
REGISTER_CLASS(UnitBase, UnitLegionary, "Юнит")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_LEGIONARY, UnitLegionary)

WRAP_LIBRARY(UnitFormationTypes, "UnitFormationType", "Типы юнитов в формациях", "Scripts\\Content\\UnitFormationType", 0, true);

void UnitFormationType::serialize(Archive& ar)
{
	StringTableBase::serialize(ar); 
	ar.serialize(radius_, "radius", "Радиус");
	ar.serialize(color_, "color", "Цвет");
}

AttributeLegionary::Level::Level() 
{
	deathGainFactor = 1.f;
}

void AttributeLegionary::Level::serialize(Archive& ar)
{
	ar.serialize(parameters, "parameters", "Необходимые параметры");
	ar.serialize(arithmetics, "arithmetics", "Арифметика, срабатывающая по достижении уровня");
	ar.serialize(deathGainFactor, "deathGainFactor", "Коэффициент параметров за гибель");
	ar.serialize(deathGainTypesFactors, "deathGainTypesFactors", "Список дополнительных коэффициентов по типам");
	ar.serialize(deathGainTypesFactorDirectControl, "deathGainTypesFactorDirectControl", "Коэффициент параметров за гибель при прямом управлении");
	
	ar.serialize(levelEffect, "levelUpEffect", "Постоянный эффект уровня");
	ar.serialize(sprite, "sprite", "Значок уровня");

	ar.serialize(levelUpEffect, "levelEffect", "Эффект при получении уровня");
	ar.serialize(levelUpSound, "LevelUpSound", "Звук при получении уровня");
}

TransportSlot::TransportSlot()
{
	canFire = false;
	destroyWithTransport = false;
	visible = false;
	requiredForMovement = false;
}

void TransportSlot::serialize(Archive& ar)
{
	ar.serialize(types, "types", "Типы юнитов");
	ar.serialize(canFire, "canFire", "Может стрелять");
	ar.serialize(destroyWithTransport, "destroyWithTransport", "Погибает вместе с транспортом");
	ar.serialize(visible, "visible", "Видимый");
	ar.serialize(requiredForMovement, "requiredForMovement", "Необходимый для движения");
	ar.serialize(node, "node", "Имя логического узла модели для привязки");
}

bool TransportSlot::checkType(UnitFormationTypeReference type) const
{
	UnitFormationTypeReferences::const_iterator i;
	FOR_EACH(types, i)
		if(*i == type)
			return true;
	return false;
}

PlumeNode::PlumeNode() 
: radius_(15.0f)
{}

void PlumeNode::serialize(Archive& ar)
{
	ar.serialize(node_, "node", "&Имя");
	ar.serialize(radius_, "radius", "&Радиус");
}

AttributeLegionary::AttributeLegionary() 
{
	unitAttackClass = ATTACK_CLASS_LIGHT;
	collisionGroup = COLLISION_GROUP_REAL;
	rigidBodyPrm = RigidBodyPrmReference("Unit Water Ground");

	unitClass_ = UNIT_CLASS_LEGIONARY;

	resourcer = false;
	constructionProgressPercent = 0;

	sprayEffectScale = 1.0f;
	sprayEffectOnWater = false;

	grassTrack = false;

	autoFindTransport = false;
	
	directControlSightRadiusFactor = 1;

	//if(AttributeSquadTable::instance().empty())
	//	AttributeSquadTable::instance().add("Squad");
	//squad = AttributeSquadReference(AttributeSquadTable::instance()[0].c_str());
}

void AttributeLegionary::serialize(Archive& ar)
{
    __super::serialize(ar);

	ar.openBlock("Physics", "Движение");
		ar.serialize(environmentDestruction, "environmentDestruction", "Разрушение объектов окружения");
		ar.serialize(rigidBodyPrm, "rigidBodyPrm", "Тип движения");
		ar.serialize(mass, "mass", "Масса");
		ar.serialize(sightRadiusNightFactor, "sightRadiusNightFactor", "Коэффициент для радиуса видимости ночью");
		ar.serialize(dieStop, "dieStop", "Останавливать при смерти");
		ar.serialize(upgradeStop, "upgradeStop", "Останавливать при Upgrade-е");
		ar.serialize(forwardVelocity, "forwardVelocity", 0);
		ar.serialize(enableRotationMode, "enableRotationMode", "Разворачивать на месте");
		ar.serialize(RangedWrapperf(forwardVelocityRunFactor, 0, 5, 0.1f), "forwardVelocityRunFactor",  "Коэффициент скорости движения при беге");
		ar.serialize(RangedWrapperf(sideMoveVelocityFactor, 0, 5, 0.1f), "sideMoveVelocityFactor",  "Коэффициент скорости движения в сторону");
		ar.serialize(RangedWrapperf(backMoveVelocityFactor, 0, 5, 0.1f), "backMoveVelocityFactor",  "Коэффициент скорости движения назад");
		ar.serialize(RangedWrapperf(angleRotationMode, 1, 3600, 1), "angleRotationMode",  "Скорость разворота на месте");
		ar.serialize(RangedWrapperf(angleSwimRotationMode, 1, 3600, 1), "angleSwimRotationMode",  "Скорость разворота на месте при плавании");
		ar.serialize(RangedWrapperf(pathTrackingAngle, 2, 50, 1), "pathTrackingAngle",  "Угол поворота ( 5 - минимальное значение для наземных )");
		ar.serializeArray(velocityByTerrainFactors, "velocityByTerrainFactors", "Коэффициенты скорости движения для типов поверхности");
		ar.serialize(waterVelocityFactor, "waterVelocityFactor", "Коэффициент скорости движения для воды");
		ar.serialize(resourceVelocityFactor, "resourceVelocityFactor", "Коэффициент скорости движения с ресурсом");
		ar.serialize(RangedWrapperf(waterFastVelocityFactor, 0, 5, 0.1f), "waterFastVelocityFactor",  "Коэффициент скорости быстрого движения для воды");
		ar.serialize(manualModeVelocityFactor, "manualModeVelocityFactor", "Коэффициент скорости движения в прямом управлении");
		ar.serialize(flyingHeight, "flyingHeight", "Высота для летающих юнитов");
		ar.serialize(waterLevel, "waterLevel", "Глубина для плавующих юнитов");
		ar.serialize(targetFlyRadius, "targetFlyRadius", "Радиус облета цели для летных юнитов");
		ar.serialize(useLocalPTStopRadius, "useLocalPTStopRadius", "Собственные настройки радиуса остановки");
		if(useLocalPTStopRadius)
			ar.serialize(localPTStopRadius, "localPTStopRadius", "Радиус остановки");
		ar.serialize(waterWeight_, "waterWeight_", "Коэффициент воздействия течения воды [0..100]");
		ar.serialize(enablePathFind, "enablePathFind", "Влючить объезд зон непроходимости");
		ar.serialize(enablePathTracking, "enablePathTracking", "Включить объезд юнитов");
		ar.serialize(enableVerticalFactor, "enableVerticalFactor", "Влючить замедление/ускорение на горах");
		ar.serialize(rotToTarget, "rotToTarget", "Поворачивать к цели при атаке");
		ar.serialize(ptSideMove, "ptSideMove", "Двигатся боком/задом при прямом управлении");
		ar.serialize(wheelList, "wheelList", "Колеса");

		if(!ar.serialize(traceInfos, "traceInfos", "следы")){ // CONVERSION 25.07.2006
			TerToolCtrl ctrl;
			ar.serialize(ctrl, "traceTerTool", "след");
			TraceInfo inf;
			inf.traceTerTool_ = ctrl;
			traceInfos.push_back(inf);
		}

		ar.serialize(traceNodes, "TraceNodes", "Объекты для привязки следов");
		ar.serialize(grassTrack, "grassTrack", "Оставлять след на траве");

		ar.serialize(formationType, "formationType", "Тип юнита в формации");
		ar.serialize(accountingNumber, "accountingNumber", "Число, учитываемое в максимальном количестве юнитов");
		ar.serialize(squad, "Squad", "Тип сквада");

		ar.serialize(autoFindTransport, "autoFindTransport", "Автоматическая посадка в транспорт");
		ar.serialize(directControlSightRadiusFactor, "directControlSightRadiusFactor", "Коэффициент радиуса видимости при прямом управлении");

		ar.serialize(runningConsumption, "runningConsumption", "Расход на бег");

		ar.serialize(plumeNodes, "plumeNodes", "Объекты для водяного шлейфа");
		sprayEffect.serialize(ar, "sprayEffect", "Эффект брызг на воде");
		ar.serialize(sprayEffectOnWater, "sprayEffectOnWater", "Вывод эффекта на поверхности воды");
		ar.serialize(sprayEffectScale, "sprayEffectScale", "Масштаб эффекта брызг на воде");
		if(fabs(sprayEffectScale) < FLT_EPS) 
			sprayEffectScale = 1.f;

		ar.serialize(gripNode, "GripNode", "Объект для оружия - захвата");
	ar.closeBlock();

    ar.serialize(levels, "levels", "Уровни развития");

	ar.openBlock("Construction", "Строительство зданий");
		ar.serialize(constructedBuildings, "constructedBuildings", "Создаваемые здания");
		ar.serialize(constructionPower, "constructionPower", "Производительность в секунду");
		ar.serialize(constructionCost, "constructionCost", "Стоимость в секунду");
		ar.serialize(constructionProgressPercent, "constructionProgressPercent", "Коэффициент увеличения стоимости строительства с увеличением числа строителей");
	ar.closeBlock();

	ar.serialize(pickedItems, "pickedItems", "Подбираемые предметы");
	ar.openBlock("Resourcer", "Ресурсодобыча");
    ar.serialize(resourcer, "resourcer", "Сборщик ресурсов");
	ar.serialize(resourcerCapacities, "resourcerCapacities", "Емкости ресурсодобытчика в порядке цепочек анимации");
	ar.serialize(resourceCollectors, "resourceCollectors", "Здания для складирования ресурсов");
	ar.closeBlock();

	collisionGroup = COLLISION_GROUP_REAL;

	if(ar.isOutput()){
		int i;
		for(i = 0; i < squad->numbers.size(); i++)
			if(squad->numbers[i].type == formationType) 
				break;
		if(i == squad->numbers.size() && !modelName.empty()) 
			kdError(0, XBuffer() < TRANSLATE("Юнит не влазит в сквад:") < libraryKey());
	}
	else{
		xassert(squad);
		//if(!squad)
		//	squad = AttributeSquadReference(AttributeSquadTable::instance()[0].c_str());
	}
}


float AttributeLegionary::radius() const
{
	return formationType->radius();
}

bool AttributeLegionary::canPick(const UnitBase* item) const
{
	AttributeItemReferences::const_iterator i;
	FOR_EACH(pickedItems, i)
		if(&item->attr() == *i)
			return true;
	return false;
}

bool AttributeLegionary::canPickResource(const ParameterTypeReference& type) const
{
	vector<ParameterCustom>::const_iterator i;
	FOR_EACH(resourcerCapacities, i)
		if(i->contain(type))
			return true;
	return false;
}

bool AttributeLegionary::canBuild(const UnitBase* unit) const
{
	AttributeBuildingReferences::const_iterator i;
	FOR_EACH(constructedBuildings, i)
		if(&unit->attr() == *i)
			return true;
	return false;
}

int AttributeLegionary::resourcerCapacity(const ParameterSet& itemParameters) const
{
	vector<ParameterCustom>::const_iterator i;
	FOR_EACH(resourcerCapacities, i)
		if(itemParameters.contain(*i))
			return i - resourcerCapacities.begin();
	return -1;
}

UnitLegionary::UnitLegionary(const UnitTemplate& data) 
: UnitActing(data)
{
	inSquad_ = false;
	onWater = false;
	selectAble_ = true;
	isMovingToTransport_ = false;
	autoFindTransport_ = attr().autoFindTransport;
	
	requestStatus_ = 0;

	squad_ = NULL;
	fireStatus_ = 0;

	manualAttackTarget_ = false;
	hasAttackPosition_ = false;
	attackPosition_ = Vect3f(0,0,0);
	
	level_ = -1;
	checkLevel(false);

	//setRadius(attr().formationType->radius());
	//rigidBody()->setRadius(attr().formationType->radius());

	resourcerMode_ = RESOURCER_IDLE;
	
	transportSlotIndex_ = 0;

	canFireInTransport_ =false;

	directControlPrev_ = false;

	nearConstructedBuilding_ = false;

	resourcerCapacityIndex_ = 0;

	if(attr().hasInventory()){
		inventory_.reserve(attr().inventories.size());
		for(UI_InventoryReferences::const_iterator it = attr().inventories.begin(); it != attr().inventories.end(); ++it)
			inventory_.add(it->control());
	}
	
	inventory_.setOwner(this);
/*
	if(!attr().traceTerTool.isEmpty()){
		if(!attr().traceNodes.empty()){
			traceControllers_.reserve(attr().traceNodes.size());

			bool wheelNode = !attr().wheelList.empty();
			for(int i = 0; i < attr().traceNodes.size(); i++)
				traceControllers_.push_back(TraceController(this, attr().traceTerTool, (int)attr().traceNodes[i], wheelNode));
		}
		else
			traceControllers_.push_back(TraceController(this, attr().traceTerTool));
	}*/
	if(!attr().traceInfos.empty()){
		if(!attr().traceNodes.empty()){
			traceControllers_.reserve(attr().traceNodes.size() * attr().traceInfos.size());

			bool wheelNode = !attr().wheelList.empty();
			for(int i = 0; i < attr().traceNodes.size(); i++){
				for(int j = 0; j < attr().traceInfos.size(); j++){
					traceControllers_.push_back(TraceController(this, attr().traceInfos[j].traceTerTool_, attr().traceInfos[j].surfaceKind_, (int)attr().traceNodes[i], wheelNode));
				}
			}
		}
		else {
			traceControllers_.reserve(attr().traceInfos.size());
			for(int j = 0; j < attr().traceInfos.size(); j++)
				traceControllers_.push_back(TraceController(this, attr().traceInfos[j].traceTerTool_, attr().traceInfos[j].surfaceKind_));
		}
	}
	traceStarted_ = false;

	posibleStates_ = UnitLegionaryPosibleStates::instance();

	waterPlume = NULL;

	resourceCollectorI_ = resourceCollectorJ_ = 0;
	resourceCollectorDistance_ = 0;
}

UnitLegionary::~UnitLegionary()
{
	RELEASE(waterPlume);
}

void UnitLegionary::setSquad(UnitSquad* squad)
{
	squad_ = squad;
}

float UnitLegionary::formationRadius() const
{
	return attr().formationType->radius();
}

void fCommandDownGrass(XBuffer& stream)
{
	Vect3f pos;
	float r;
	stream.read(pos);
	stream.read(r);
	environment->grass()->DownGrass(pos, r);
}

void fCommandUpdateWaterPlum(XBuffer& stream)
{
	cWaterPlume* waterPlume;
	Vect3f position;
	stream.read(waterPlume);
	stream.read(position);
	waterPlume->setPosition(position);
	float water_z = environment->water()->GetZFast(position.x, position.y);
	bool verifyZ(waterPlume->verifyZ());
	float rate;
	stream.read(rate);
	waterPlume->setRate(rate);
	cWaterPlume::NodeContainer::iterator nd;
	FOR_EACH(waterPlume->nodes(), nd){
		stream.read(position);
		nd->update(position, verifyZ && position.z > water_z, water_z, waterPlume->unDirPartls());
	}
}

void UnitLegionary::Quant()
{
	start_timer_auto();

	if(!alive()){
		UnitReal::Quant();
		return;
	}

#ifndef _FINAL_VERSION_
	if(!squad_){
		if(isUnderEditor()){
			squad_ = safe_cast<UnitSquad*>(player()->buildUnit(&*attr().squad));
			squad_->setPose(pose(), true);
			squad_->addUnit(this, false);
		}
		xassertStr(squad_ && "У юнита нет сквада, он будет удален", attr().libraryKey());
		if(!squad_){
			Kill();
			return;
		}
	}
#endif

	velocityProcessing();

	__super::Quant();

	if(isFrozen()){
		if(rigidBody()->colliding() & (RigidBodyBase::GROUND_COLLIDING | RigidBodyBase::WATER_COLLIDING))
			rigidBody()->makeFrozen(rigidBody()->onIce);
		if(!rigidBody()->isBoxMode()){
			rigidBody()->enableAngularEvolve(false);
			rigidBody()->setWaterAnalysis(false);
			rigidBody()->enableBoxMode();
		}
	}else
		rigidBody()->unFreeze();
		
	if(!alive())
		return;

	ParameterSet recovery;
	recovery.setRecovery(parameters());
	parameters_.scaleAdd(recovery, logicPeriodSeconds);
	parameters_.clamp(parametersMax());

	checkLevel(true);

	if(targetUnit_){
		targetUnit_->setAimed();
		if(!targetEventTimer_){
			targetEventTimer_.start(targetEventTime + logicRND(targetEventTime));
			player()->checkEvent(EventUnitMyUnitEnemy(Event::AIM_AT_OBJECT, targetUnit_, this));
			targetUnit_->player()->checkEvent(EventUnitMyUnitEnemy(Event::AIM_AT_OBJECT, targetUnit_, this));
		}
	}

	if(!rigidBody()->onWater && !rigidBody()->isBoxMode() && isMoving() && !rigidBody()->onSecondMap){
		TraceControllers::iterator it;
		if(!traceStarted_){
			traceStarted_ = true;
			FOR_EACH(traceControllers_, it)
				it->start();
		}
		FOR_EACH(traceControllers_, it)
			it->update();
	}
	else
		traceStarted_ = false;

	bool onWaterNew((rigidBody()->onLowWater || rigidBody()->onWater) && !rigidBody()->onDeepWater && !hiddenGraphic());
	if(onWater != onWaterNew){
		if(!onWater){
			if(modelLogic()){
				waterPlume = new cWaterPlume(this, environment->waterPlumeAtribute());
				waterPlume->SetScene(terScene);
				attachSmart(waterPlume);
			}
		}else 
			RELEASE(waterPlume);
		onWater = onWaterNew;
	}
	if(onWater){
		streamLogicCommand.set(fCommandUpdateWaterPlum) << waterPlume << position() << (rigidBody()->unitMove() ? 
			1.0f / (rigidBody()->forwardVelocity() + 1.0f) : 0.0f);
		cWaterPlume::NodeContainer::iterator nd;
		FOR_EACH(waterPlume->nodes(), nd)
			streamLogicCommand << modelLogicNodePosition(nd->index).trans();
	}

	if(attr().grassTrack&&isMoving())
		streamLogicCommand.set(fCommandDownGrass) << rigidBody()->position() << rigidBody()->extent().x;

	resourcerQuant();

	if(autoFindTransport_ && !transportFindingPause_){
		transportFindingPause_.start(logicRND(2000));
		setTransportAuto();
	}
	if(constructedBuilding_){
		UnitBuilding* building = safe_cast<UnitBuilding*>(&*constructedBuilding_);
		if(!building->constructor())
			building->setConstructor(this);
		int buildersCounter = building->buildersCounter();
		if(buildersCounter){
			ParameterSet cost = attr().constructionCost;
			cost *= logicPeriodSeconds*(1.f + (buildersCounter - 1)*attr().constructionProgressPercent/100.f)*building->constructionsSpeedFactor();
			if(requestResource(cost, NEED_RESOURCE_TO_BUILD_BUILDING)){
				if(nearUnit(constructedBuilding_))
					subResource(cost);
			}
			else
			{
				building->setConstructor(0);
            	constructedBuilding_ = 0;
			}
		}
	}

	if(constructedBuilding_ && position2D().distance2(constructedBuilding_->position2D()) > 
		sqr(radius() + constructedBuilding_->radius())	&& nearUnit(constructedBuilding_)){
		nearConstructedBuilding_ = true;
		ParameterSet delta = attr().constructionPower;
		delta *= logicPeriodSeconds*parameters().findByType(ParameterType::RESOURCE_PRODUCTIVITY_FACTOR, 1);
		UnitBuilding* building = safe_cast<UnitBuilding*>(&*constructedBuilding_);
		if(building->isConstructed() || building->addResourceToBuild(delta)){
			constructedBuilding_ = 0;
			building->setConstructor(0);
		}
	}
	else{
		nearConstructedBuilding_ = false;
		if(!constructedBuilding_ && !constructedBuildings_.empty()){
			constructedBuilding_ = constructedBuildings_.front();
			constructedBuildings_.erase(constructedBuildings_.begin());
		}
	}

	if(itemToPick_){
		// подбор предмета
		if(nearUnit(itemToPick_)){
			if(putToInventory(itemToPick_))
				itemToPick_->Kill();

			itemToPick_ = 0;
		}
	}

	if(manualRunMode && rigidBody()->unitMove() && !attr().runningConsumption.empty()){
		ParameterSet runningConsumption = attr().runningConsumption;
		runningConsumption *= logicPeriodSeconds;
		if(requestResource(runningConsumption, NEED_RESOURCE_TO_MOVE))
			subResource(runningConsumption);
		else
			squad()->executeCommand(UnitCommand(COMMAND_ID_STOP_RUN, (class UnitInterface*)0, 0));
	}

	if(selected()){
		CommandList::const_iterator cmd;
		FOR_EACH(squad()->suspendCommandList(), cmd)
			if(cmd->commandID() == COMMAND_ID_ATTACK)
				if(const WeaponBase* weapon = findWeapon(cmd->commandData()))
					if(weapon->weaponPrm()->hasTargetMark())
						UI_LogicDispatcher::instance().addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, 
						Se3f(QuatF::ID, cmd->position()), &weapon->weaponPrm()->targetMark(), this, cmd->unit()));
	}
}

void UnitLegionary::Kill()
{
	if(dead())
		return;

	if(squad_)
		squad_->removeUnit(this);

	__super::Kill();

	RELEASE(waterPlume);

	TraceControllers::iterator ti;
	FOR_EACH(traceControllers_, ti)
		ti->stop();
}

void UnitLegionary::executeCommand(const UnitCommand& command)
{
	switch(command.commandID()){
	case COMMAND_ID_POINT:
	case COMMAND_ID_OBJECT:
	case COMMAND_ID_STOP: 
		itemToPick_ = 0;
		break;
	case COMMAND_ID_ITEM_REMOVE:
		inventory_.removeItem(command.commandData());
		break;
	case COMMAND_ID_ITEM_DROP:
		if(const InventoryItem* item = inventory_.getItem(command.commandData())){
			UnitBase* p = player()->buildUnit(item->attribute());
			p->setPose(pose(), true);
			safe_cast<UnitInterface*>(p)->setParameters(item->parameters());
			inventory_.removeItem(command.commandData());
		}
		break;
	case COMMAND_ID_ITEM_ACTIVATE:
		if(const InventoryItem* item = inventory_.getItem(command.commandData())){
			inventory_.removeItem(command.commandData());
		}
		break;
	case COMMAND_ID_ITEM_TAKE:
		inventory_.takeItem(command.commandData());
		break;
	case COMMAND_ID_ITEM_RETURN:
		inventory_.returnItem(command.commandData());
		break;
	case COMMAND_ID_ITEM_TAKEN_DROP:
		if(inventory_.takenItem()()){
			UnitBase* p = player()->buildUnit(inventory_.takenItem().attribute());
			p->setPose(Se3f(MatXf(Mat3f::ID, command.position())), true);
			safe_cast<UnitInterface*>(p)->setParameters(inventory_.takenItem().parameters());
			inventory_.removeTakenItem();
		}
		break;

	case COMMAND_ID_GO_RUN:
		manualRunMode = true;
		goToRun();
		break;
	case COMMAND_ID_STOP_RUN:
		manualRunMode = false;
		stopToRun();
		break;
	case COMMAND_ID_DIRECT_PUT_IN_TRANSPORT:
		UnitInterface* unit = command.unit();
		if(unit && safe_cast<UnitActing*>(unit->getUnitReal())->putInTransport(squad()))
			startState(StateLanding::instance());
		break;	
	}

	__super::executeCommand(command);
}

struct FindItemOp
{
	FindItemOp(UnitLegionary* resourcer) : resourcer_(resourcer), distMin_(FLT_INF), item_(0)
	{}

	void operator()(const UnitBase* p)
	{
		if(p->alive() && !p->isUnseen() && p->attr().isResourceItem() && resourcer_->attr().resourcerCapacity(safe_cast<const UnitInterface*>(p)->parameters()) != -1){
			float dist = p->position2D().distance2(resourcer_->position2D());
			if(distMin_ > dist){
				distMin_ = dist;
				item_ = const_cast<UnitReal*>(safe_cast<const UnitReal*>(p));
			}
		}
	}

	UnitReal* item() const { return item_; }

private:
	float distMin_;
	UnitLegionary* resourcer_;
	UnitReal* item_;
};

bool UnitLegionary::setResourceItem(UnitInterface* resourceItem)
{
	if(!resourceItem)
		return false;
	
	AttributeItemReferences::const_iterator i;
	FOR_EACH(attr().pickedItems, i)
		if(*i == &resourceItem->attr())
			return true;
	
	int index = attr().resourcerCapacity(resourceItem->parameters());
	if(index == -1 || resourcePickingConsumer_.started() && resourcerCapacityIndex_ != index)
		return false;
	resourcerCapacityIndex_ = index;

	resourcerCapacity_ = attr().resourcerCapacities[resourcerCapacityIndex_];
	resourceItem_ = safe_cast<UnitReal*>(resourceItem);
	if(!resourceItem_)
		return false;
	resourcerMode_ = RESOURCER_FINDING;

	if(!resourcePickingConsumer_.started()){
		float time = parameters().findByType(ParameterType::RESOURCE_PICKING_TIME, 20);
		resourcerCapacity_ *= parameters().findByType(ParameterType::RESOURCE_PRODUCTIVITY_FACTOR, 1);
		resourcePickingConsumer_.start(resourceItem_, resourcerCapacity_, time);
	}
	return true;
}

void UnitLegionary::resourcerQuant()
{
	switch(resourcerMode_){
	case RESOURCER_FINDING:
		if(resourceItem_ && !resourceItem_->isUnseen()){
			if(nearUnit(resourceItem_)){
				resourcerMode_ = RESOURCER_PICKING;
			}
		}
		else if(!resourceFindingPause_){
			resourceFindingPause_.start(logicRND(3000));
			FindItemOp op(this);
			universe()->unitGrid.Scan(round(position().x), round(position().y), sightRadius(), op);
			if(!setResourceItem(op.item())){
				resourceItem_ = 0;
				resourcerMode_ = RESOURCER_IDLE;
			}
		}
		break;

	case RESOURCER_PICKING:
		if(resourceItem_){
			if(resourceItem_->parameters().zero(resourcerCapacity_)){
				resourceItem_->Kill();
				resourceItem_ = 0;
				resourcerMode_ = RESOURCER_IDLE;
			}
			else{
				ParameterSet delta = resourcePickingConsumer_.delta();
				delta.clamp(resourceItem_->parameters());
				resourceItem_->getParameters().subClamped(delta);
				if(resourcePickingConsumer_.addQuant(delta)){
					resourcerMode_ = RESOURCER_RETURNING;
					resourceCollector_ = 0;
					resourceCollectorI_ = resourceCollectorJ_ = 0;
					resourceCollectorDistance_ = FLT_INF;
				}
			}
		}
		else
			resourcerMode_ = RESOURCER_IDLE;
		break;

	case RESOURCER_RETURNING:
		if(!resourceCollector_ || resourceCollectorI_ || resourceCollectorJ_){
			interuptState(StateGiveResource::instance());
			if(resourceCollectorI_ < attr().resourceCollectors.size()){
				const RealUnits& collectors = player()->realUnits(attr().resourceCollectors[resourceCollectorI_]);
				if(resourceCollectorJ_ < collectors.size()){
					UnitReal* collector = collectors[resourceCollectorJ_++];
					float dist = getPathFinderDistance(collector);
					if(collector->alive() && resourceCollectorDistance_ > dist){
						resourceCollectorDistance_ = dist;
						resourceCollector_ = collector;
					}
				}
				else{
					resourceCollectorI_++;
					resourceCollectorJ_ = 0;
				}
			}
			else{
				resourceCollectorI_ = 0;
				resourceCollectorJ_ = 0;
				resourceCollectorDistance_ = FLT_INF;
				if(resourceCollector_)
					setWayPoint(resourceCollector_->position2D());
				else{
					int collectors = 0;
					AttributeReferences::const_iterator i;
					FOR_EACH(attr().resourceCollectors, i)
						collectors += player()->countUnits(*i);
					if(!collectors){
						resourceItem_ = 0;
						resourcerMode_ = RESOURCER_IDLE;
					}
				}
			}
		}
		break;
	}
}

bool UnitLegionary::canAutoMove() const
{
	if(squad_){
		UnitLegionary* leader = safe_cast<UnitLegionary*>(squad_->getUnitReal());
		
		if(this == leader)
			return true;
		
		if(leader->getUnitState() == UnitReal::AUTO_MODE && leader->fireTargetExist())
			return true;

		if(leader->getUnitState() == UnitReal::ATTACK_MODE && leader->fireTargetExist() && (!canAttackTarget(leader->fireTarget())))
			return true;

		return false;
	}

	return true;
}

void UnitLegionary::setTransport(UnitActing* transport, int transportSlotIndex)
{
	transport_ = transport;
	transportSlotIndex_ = transportSlotIndex;
	isMovingToTransport_ = true;
}

class TransportAutoFindOp
{
public:
	TransportAutoFindOp(UnitLegionary* owner) : owner_(owner->squad()) { }

	bool operator ()(UnitBase* unit)
	{
		if(unit->attr().isTransport()){
			UnitActing* transport = safe_cast<UnitActing*>(unit);
			if(transport->putInTransport(owner_))
				return false;
		}

		return true;
	}

private:
	UnitSquad* owner_;
};

bool UnitLegionary::setTransportAuto()
{
	if(inTransport() || isMovingToTransport())
		return false;

	TransportAutoFindOp find_op(this);

	return universe()->unitGrid.ConditionScan(position().xi(), position().yi(), sightRadius(), find_op);
}

UnitActing* UnitLegionary::transport() const
{
	if(inTransport() || isMovingToTransport())
		return transport_;
	else
		return 0;
}

void UnitLegionary::clearTransport()
{
	if(!interuptState(StateLanding::instance()) && !finishState(StateInTransport::instance()) && transport_)
		putUnitOutTransport();
}

void UnitLegionary::putInTransport()
{
	const TransportSlot& slot = transport_->attr().transportSlots[transportSlotIndex_];
	if(!slot.visible)
		hide(HIDE_BY_TRANSPORT, true);
	canFireInTransport_ = slot.canFire;
	for(int i = transportSlotIndex_; i < transportSlotIndex_ + attr().transportSlotsRequired; ++i){
		transport_->stopAdditionalChain(CHAIN_SLOT_IS_EMPTY, i);
		transport_->setAdditionalChain(CHAIN_CARGO_LOADED, i);
	}
	if(DirectControlMode cargoDirectControl = directControl()){
		directControlPrev_ = true;
		bool selectUnit = activeDirectControl();
		setDirectControl(DIRECT_CONTROL_DISABLED);
		transport_->setDirectControl(cargoDirectControl);
		if(selectUnit){
			universe()->select(transport_, false);
			transport_->setActiveDirectControl(cargoDirectControl);
		}
	}else
		universe()->deselect(squad());
	attachToDock(transport_, slot.node, false);
	additionToTransportInv_ = attr().additionToTransport;
	additionToTransportInv_.setInvertOnApply();
	transport_->applyParameterArithmetics(additionToTransportInv_);
	transport_->resetCargo();
	isMovingToTransport_ = false;
	transport_->updateSelectionListPriority();
	if(transport_->player() != player())
		transport_->changeUnitOwner(player());
}

void UnitLegionary::putUnitOutTransport()
{
	if(transport_){
		if(inTransport()){
			additionToTransportInv_.setInverted();
			transport_->applyParameterArithmetics(additionToTransportInv_);
		}
		transport_->detachFromTransportSlot(transportSlotIndex_, attr().transportSlotsRequired);
		if(directControlPrev_){
			if(DirectControlMode transportDirectControl = transport_->directControl()){
				bool selectUnit = transport_->activeDirectControl();
				transport_->setDirectControl(DIRECT_CONTROL_DISABLED);
				setDirectControl(transportDirectControl);
				if(selectUnit){
					universe()->select(this, false);
					setActiveDirectControl(transportDirectControl);
				}
				
			}
		}
		transport_->updateSelectionListPriority();
	}else
		setDirectControl(DIRECT_CONTROL_DISABLED);	
	directControlPrev_ = false;
	hide(HIDE_BY_TRANSPORT, false);
	makeDynamic(STATIC_DUE_TO_SITTING_IN_TRANSPORT);
	isMovingToTransport_ = false;
	wayPoints_.clear();
	transport_ = 0;
}

void UnitLegionary::setInSquad()
{
	if(!inSquad()){
		inSquad_ = true;
		//getSquad()->addSquadMutationMolecula(atom);
	}
}

void UnitLegionary::showDebugInfo()
{
	__super::showDebugInfo();

	XBuffer buf;
	if(showDebugLegionary.level){
		buf < "Level: " <= level() < "\n";
	}

	if(showDebugLegionary.transport && transport_)
		buf < inTransport() ? "in transport\n" : "to transport\n";

	if(showDebugLegionary.invisibility && isUnseen()){
		buf < "Invisible\n";
	}

	if(attr().resourcer){
		if(showDebugLegionary.resourcerMode)
			buf < "resourcerMode: " < getEnumNameAlt(resourcerMode_) < "\n";
		if(showDebugLegionary.resourcerProgress){
			buf < "resourcePickingConsumer: " <= resourcePickingConsumer_.progress() < "\n";
		}
		if(showDebugLegionary.resourcerCapacity && !attr().resourcerCapacities.empty())
			attr().resourcerCapacities.front().showDebug(position(), YELLOW);
	}

	if(showDebugLegionary.trace){
		TraceControllers::iterator it;
		FOR_EACH(traceControllers_, it)
			it->showDebugInfo();
	}

	if(showDebugLegionary.aimed && aimed())
		buf < "aimed\n";
	
	if(buf.tell()){
		buf -= 2;
		show_text(position() + Vect3f(0, 0, 15), buf, RED);
	}
}

bool UnitLegionary::selectAble() const
{
	if(inTransport() || !selectAble_)
		return false;
	return __super::selectAble();
}

void UnitLegionary::graphQuant(float dt)
{
	if(hiddenGraphic())
		return;
	
	if(isUnseen() && !player()->active() && universe()->activePlayer()->clan() != player()->clan())
		return;
		
	__super::graphQuant(dt);

	if(selected() && level() >= 0)
		attr().levels[level()].sprite.draw(interpolatedPose().trans(), attr().initialHeightUIParam);
}

void UnitLegionary::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(squad_, "squad", 0);

	bool inSquad = inSquad_;
	ar.serialize(inSquad, "inSquad", 0);
	if(inSquad)
		setInSquad();

	int level = level_;
	ar.serialize(level, "level", 0);
	if(ar.isInput())
		setLevel(level, false);

	if(universe()->userSave()){
		ar.serialize(transport_, "transport", 0);
		ar.serialize(transportSlotIndex_, "transportSlot", 0);
		ar.serialize(canFireInTransport_, "canFireInTransport", 0);
		ar.serialize(isMovingToTransport_, "isMovingToTransport", 0);
		ar.serialize(additionToTransportInv_, "additionToTransportInv", 0);
		ar.serialize(transportFindingPause_, "transportFindingPause" , 0);
		ar.serialize(directControlPrev_, "directControlPrev", 0);

		ar.serialize(teleport_, "teleport", 0);
		ar.serialize(teleportTo_, "teleportTo", 0);
		
		ar.serialize(resourcerMode_, "resourcerMode", 0);
		if(resourcerMode_ != RESOURCER_IDLE){
			ar.serialize(resourceItem_, "resourceItem", 0);
			ar.serialize(resourcerCapacityIndex_, "resourcerCapacityIndex", 0);
			if(ar.isInput())
				resourcerCapacity_ = attr().resourcerCapacities[resourcerCapacityIndex_];
			ar.serialize(resourcerCapacity_, "resourcerCapacity", 0);
		}
	}

	ar.serialize(inventory_, "inventory", 0);

	ar.serialize(constructedBuilding_, "constructedBuilding", 0);
	if(constructedBuilding_)
		ar.serialize(nearConstructedBuilding_, "nearConstructedBuilding", 0);
	ar.serialize(constructedBuildings_, "constructedBuildings", 0);
}

void UnitLegionary::addParameters(const ParameterSet& parameters)
{
	parameters_ += parameters;
	checkLevel(true);
}

float UnitLegionary::levelProgress() const
{
	if(level() + 1 >= attr().levels.size())
		return 0.f;

	if(level() > 0){
		ParameterSet need = attr().levels[level() + 1].parameters;
		ParameterSet is1 = parameters();
		ParameterSet is2 = player()->resource();
		need.subClamped(attr().levels[level()].parameters);
		is1.subClamped(attr().levels[level()].parameters);
		is2.subClamped(attr().levels[level()].parameters);
		return need.progress(is1, is2);
	}

	return attr().levels[level() + 1].parameters.progress(parameters(), player()->resource());
}

void UnitLegionary::checkLevel(bool applyArithmetics)
{
	for(;level_ + 1 < attr().levels.size(); ){
		const AttributeLegionary::Level& level = attr().levels[level_ + 1];
		if(level.parameters.below(parameters(), player()->resource()))
			setLevel(level_ + 1, applyArithmetics);
		else
			break;
	}
}

void UnitLegionary::setLevel(int newLevel, bool applyArithmetics)
{
	if(newLevel < 0 || newLevel >= attr().levels.size() || level_ == newLevel)
		return;

	const AttributeLegionary::Levels& levels = attr().levels;
	if(level_ >= 0)
		stopEffect(&levels[level_].levelEffect);

	const AttributeLegionary::Level& level = levels[level_ = newLevel];
	startEffect(&level.levelEffect);
	startEffect(&level.levelUpEffect, false, -1, 10*1000);
	startSoundEffect(level.levelUpSound);
	stopSoundEffect(level.levelUpSound);
	if(applyArithmetics)
		applyParameterArithmetics(level.arithmetics);
}

void UnitLegionary::deathGainMultiplicator(ParameterArithmetics& arithmetics) const
{
	if(level() < 0)
		return;
	
	arithmetics *= attr().levels[level()].deathGainFactor;
	arithmetics *= !isDirectControl() ? attr().levels[level()].deathGainTypesFactors : attr().levels[level()].deathGainTypesFactorDirectControl;
}

float UnitLegionary::sightRadius() const
{
	float radius = __super::sightRadius();
	if(isDirectControl()){
		radius *= attr().directControlSightRadiusFactor;
		radius = min(radius, float(FogOfWarMap::maxSightRadius()));
	}
	return radius;
}

void UnitLegionary::collision(UnitBase* unit, const ContactInfo& contactInfo)
{
	if(unit->alive() && unit->attr().isResourceItem() && attr().canPick(unit)){
		applyParameterArithmetics(unit->attr().parametersArithmetics);
		unit->Kill();
		return;
	}
	__super::collision(unit, contactInfo);
}

bool UnitLegionary::setConstructedBuilding(UnitReal* constructedBuilding, bool queued) 
{ 
	if(attr().canBuild(constructedBuilding)){
		if(queued && constructedBuilding_)
			constructedBuildings_.push_back(constructedBuilding);
		else
			constructedBuilding_ = constructedBuilding; 
		return true;
	}
	return false;
}

void UnitLegionary::clearOrders()
{
	interuptState(StateOpenForLanding::instance());
	wayPoints_.clear();
	resourceItem_ = 0;
	resourcerMode_ = RESOURCER_IDLE;
	resourcePickingConsumer_.clear();
	if(constructedBuilding_){
		UnitBuilding* building = safe_cast<UnitBuilding*>(&*constructedBuilding_);
		building->setConstructor(0);
	}
	constructedBuilding_ = 0;
	constructedBuildings_.clear();
	clearTransport();
	setTeleport(0, 0); 
//	rigidBody()->setTrail(0);
}

void UnitLegionary::changeUnitOwner(Player* playerIn)
{
	if(player() == playerIn)
		return;

	clearOrders();

	if(squad()){
		squad()->removeUnit(this);
		__super::changeUnitOwner(playerIn);
		squad_ = safe_cast<UnitSquad*>(playerIn->buildUnit(&*attr().squad));
		squad_->setPose(pose(), true);
		squad_->addUnit(this, false);
		setSelected(false);
	}
	else
		__super::changeUnitOwner(playerIn);
}

void UnitLegionary::applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics)
{
	__super::applyParameterArithmeticsImpl(arithmetics);
	if(squad() && !squad()->Empty() && squad()->getUnitReal() == this) // только один раз - только первый
		squad()->applyParameterArithmeticsImpl(arithmetics);
	checkLevel(true);
}

void UnitLegionary::setTeleport(UnitReal* teleport, UnitReal* teleportTo)
{
	if(!teleport_ && teleportTo_)
		return;
	teleport_ = teleport;
	teleportTo_ = teleportTo;
}

bool UnitLegionary::setItemToPick(UnitBase* item)
{
	if(!item->attr().isInventoryItem()) 
		return false;

	UnitItemInventory* p = safe_cast<UnitItemInventory*>(item);

	if(!canPutToInventory(p))
		return false;

	itemToPick_ = p;
	return true;
}

bool UnitLegionary::uniform(const UnitReal* unit) const
{
	return !unit || &attr() == &unit->attr() && level() == safe_cast<const UnitLegionary*>(unit)->level();
}

bool UnitLegionary::prior(const UnitInterface* unit) const
{
	if(unit->attr().isLegionary()){
		int pr1 = selectionListPriority();
		int pr2 =unit->selectionListPriority();

		if(pr1 == pr2){
			pr1 = level();
			pr2 =safe_cast<const UnitLegionary*>(unit)->level();

			return pr1 == pr2 ? &attr() < &unit->getUnitReal()->attr() : pr1 > pr2;
		}

		return pr1 > pr2;
	}

	return UnitInterface::prior(unit);
}


bool UnitLegionary::canPutToInventory(const UnitItemInventory* item) const
{
	return inventory_.canPutItem(item);
}

bool UnitLegionary::putToInventory(const UnitItemInventory* item)
{
	return inventory_.putItem(item);
}

bool UnitLegionary::canExtractResource(const UnitItemResource* item) const
{
	xassert(item);
	
	AttributeItemReferences::const_iterator i;
	FOR_EACH(attr().pickedItems, i)
		if(*i == &item->attr())
			return true;

	return attr().resourcerCapacity(item->parameters()) >= 0;
}

bool UnitLegionary::canBuild(const UnitReal* building)  const
{
	xassert(building);
	return attr().canBuild(building);
}

bool UnitLegionary::canRun() const
{
	ParameterSet runningConsumption = attr().runningConsumption;
	runningConsumption *= logicPeriodSeconds;
	return requestResource(runningConsumption, NEED_RESOURCE_SILENT_CHECK);
}

/// может ли произвести кого-то для добычи из этого ресурса
bool UnitBuilding::canExtractResource(const UnitItemResource* item) const
{
	xassert(item);
	AttributeBase::ProducedUnitsVector::const_iterator it;
	FOR_EACH(attr().producedUnits, it)
		if(it->second.unit->isLegionary())
			if(safe_cast<const AttributeLegionary*>(it->second.unit.get())->resourcerCapacity(item->parameters()) >= 0)
				return true;
	return false;
}

/// может ли произвести кого-то для достройки здания
bool UnitBuilding::canBuild(const UnitReal* building) const
{
	xassert(building);
	AttributeBase::ProducedUnitsVector::const_iterator it;
	FOR_EACH(attr().producedUnits, it)
		if(it->second.unit->isLegionary())
			if(safe_cast<const AttributeLegionary*>(it->second.unit.get())->canBuild(building))
				return true;
	return false;
}

float UnitLegionary::forwardVelocity()
{
	float forwardVelocity_ = parameters().findByType(ParameterType::VELOCITY, attr().forwardVelocity);

	if(!forwardVelocity_)
		forwardVelocity_ = rigidBody()->prm().forward_velocity_max;

	float factor = 1.0f;
		
	if(resourcerMode_ == RESOURCER_RETURNING)
		factor = attr().resourceVelocityFactor;

	if(isDirectControl())
		factor *= manualModeVelocityFactor;
	
	if(rigidBody()->flyingMode())
		return factor * (rigidBody()->isRunMode() ? forwardVelocity_ * attr().forwardVelocityRunFactor : forwardVelocity_);

	if(rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_LEFT || rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_RIGHT)
		factor *= attr().sideMoveVelocityFactor;
	if(rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_BACK)
		factor *= attr().backMoveVelocityFactor;

	factor *= rigidBody()->verticalFactor();
	
	if(rigidBody()->onSecondMap)
		return (rigidBody()->isRunMode() ? factor * forwardVelocity_ * attr().forwardVelocityRunFactor : factor * forwardVelocity_);
	else if(rigidBody_->onWater)
		return (rigidBody()->isRunMode() ? factor * forwardVelocity_ * attr().waterFastVelocityFactor : factor * forwardVelocity_ * attr().waterVelocityFactor);
	else
		return (rigidBody()->isRunMode() ? factor * forwardVelocity_ * attr().velocityByTerrainFactors[vMap.getTerrainType((const Vect2f&)position(), radius())]*attr().forwardVelocityRunFactor : factor * forwardVelocity_ * attr().velocityByTerrainFactors[vMap.getTerrainType((const Vect2f&)position(), radius())]);

}

void UnitLegionary::velocityProcessing()
{
	if(rigidBody()->unitMove()){
		float velocity = 0;
		if(squad()->correctSpeed())
			velocity = squad()->getAvrVelocity();
		else
			velocity = forwardVelocity();

		rigidBody()->setForwardVelocity(velocity);
		forwardVelocityFactor_ = velocity / (parameters().findByType(ParameterType::VELOCITY, attr().forwardVelocity) + 1.e-5f);
		if(forwardVelocityFactor_ < FLT_EPS)
			forwardVelocityFactor_ = 0;
	}
}

bool UnitLegionary::setState( UnitLegionary* unit )
{
	if(unit->targetPointEnable && unit->getUnitState() == UnitReal::ATTACK_MODE){
		selectWeapon(0);
		setTargetPoint(unit->targetPoint_);
		return true;
	}
	else if(unit->targetUnit_ && unit->getUnitState() == UnitReal::ATTACK_MODE){
		selectWeapon(0);
		setManualTarget(unit->targetUnit_);
		return true;
	}

	return false;
}

void UnitLegionary::addWayPoint( const Vect2f& point, bool enableMoveMode )
{
	if(enableMoveMode)
		UnitReal::addWayPoint(point);
	else if (!(selectedWeaponID() && (unitState == UnitReal::ATTACK_MODE))) {
		Vect3f p2 = clampWorldPosition(To3D(point), radius());
		wayPoints_.push_back(p2);
	}
}

void UnitLegionary::giveResource()
{
	player()->addResource(resourcerCapacity_, true);
	resourcerMode_ = RESOURCER_FINDING;
	resourcePickingConsumer_.clear();
	ShowChangeParametersController doShow(resourceCollector_, resourcerCapacity_);
	setResourceItem(resourceItem_);
}

//---------------------------------

TraceController::TraceController(UnitReal* owner, const TerToolCtrl& ctrl, unsigned surface, int nodeIndex, bool wheelNode) : owner_(owner),
	traceCtrl_(ctrl),
	surfaceKind_(surface),
	nodeIndex_(max(nodeIndex, 0)), 
	wheelNode_(wheelNode)
{
#ifndef _FINAL_VERSION_
	lastPosition_ = Vect3f::ZERO;
#endif
}

void TraceController::start()
{
	traceCtrl_.start(position());
#ifndef _FINAL_VERSION_
	lastPosition_ = position().trans();
#endif
}

void TraceController::stop()
{
	traceCtrl_.stop();
}

void TraceController::update()
{
	bool need_update = false;

	if(nodeIndex_ && !wheelNode_){
		if(owner_->isModelLogicNodeVisible(nodeIndex_)){
			if(!isUpdated_){
				need_update = true;
				isUpdated_ = true;
			}
		}
		else
			isUpdated_ = false;
	}
	else
		need_update = true;

	if(surfaceKind_ != (SURFACE_KIND_1 | SURFACE_KIND_2 | SURFACE_KIND_3 | SURFACE_KIND_4)){
		Vect3f pos = position().trans();

		if(!(1 << vMap.getSurKind(pos.xi(), pos.yi()) & surfaceKind_))
			need_update = false;
	}

	if(need_update){
		traceCtrl_.setPosition(position());
#ifndef _FINAL_VERSION_
		lastPosition_ = position().trans();
#endif
	}
}

void TraceController::showDebugInfo()
{
	bool v = (!nodeIndex_ || owner_->isModelLogicNodeVisible(nodeIndex_));
	sColor4c col(128, 128, 128);
	
	MatXf X(position());
	Vect3f delta = X.rot().xcol();
	delta.normalize(5);
	show_vector(X.trans(), delta, v ? X_COLOR : col);

	delta = X.rot().ycol();
	delta.normalize(5);
	show_vector(X.trans(), delta, v ? Y_COLOR : col);

	delta = X.rot().zcol();
	delta.normalize(5);
	show_vector(X.trans(), delta, v ? Z_COLOR : col);

#ifndef _FINAL_VERSION_
	show_vector(lastPosition_, 5, GREEN);
#endif
}

const Se3f& TraceController::position()
{
	return owner_->modelLogicNodePosition(nodeIndex_);
}
