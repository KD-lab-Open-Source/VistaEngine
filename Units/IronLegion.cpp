#include "StdAfx.h"
#include "Universe.h"

#include "Squad.h"
#include "Sound.h"
#include "Triggers.h"
#include "vmap.h"
#include "Serialization\Dictionary.h"
#include "Serialization\Serialization.h"
#include "IronBuilding.h"
#include "UnitItemInventory.h"
#include "UnitItemResource.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\RadianWrapper.h"
#include "Serialization\SerializationFactory.h"
#include "Game\CameraManager.h"
#include "UserInterface\UI_Render.h"
#include "Environment\Environment.h"
#include "Serialization\StringTableImpl.h"
#include "Render\src\Grass.h"
#include "Console.h"
#include "Water\Water.h"
#include "Water\WaterWalking.h"
#include "RenderObjects.h"
#include "UserInterface\UserInterface.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\SelectManager.h"
#include "VistaRender\FieldOfView.h"
#include "AI\PFTrap.h"
#include "Terra\TerrainType.h"

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

DECLARE_SEGMENT(UnitLegionary)
REGISTER_CLASS(AttributeBase, AttributeLegionary, "Юнит");
REGISTER_CLASS(UnitBase, UnitLegionary, "Юнит")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_LEGIONARY, UnitLegionary)

WRAP_LIBRARY(UnitFormationTypes, "UnitFormationType", "Типы юнитов в формациях", "Scripts\\Content\\UnitFormationType", 0, LIBRARY_EDITABLE | LIBRARY_IN_PLACE);

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
	dockWhenLanding = false;
	requiredForMovement = false;
}

void TransportSlot::serialize(Archive& ar)
{
	ar.serialize(types, "types", "Типы юнитов");
	ar.serialize(necessaryParameters, "necessaryParameters", "Необходимые параметры юнита");
	ar.serialize(canFire, "canFire", "Может стрелять");
	ar.serialize(destroyWithTransport, "destroyWithTransport", "Погибает вместе с транспортом");
	ar.serialize(visible, "visible", "Видимый");
	ar.serialize(requiredForMovement, "requiredForMovement", "Необходимый для движения");
	ar.serialize(node, "node", "Имя логического узла модели для привязки");
	ar.serialize(dockWhenLanding, "dockWhenLanding", "Линковать при посадке");
}

bool TransportSlot::check(UnitFormationTypeReference type, const ParameterSet& parameters) const
{
	if(!parameters.above(necessaryParameters))
		return false;
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

	itemDropRadius = 0.f;

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

	excludeCollision = EXCLUDE_COLLISION_LEGIONARY;

	if(ar.openBlock("Physics", "Движение")){
		ar.serialize(environmentDestruction, "environmentDestruction", "Разрушение объектов окружения");
		ar.serialize(rigidBodyPrm, "rigidBodyPrm", "Тип движения");
		ar.serialize(mass, "mass", "Масса");
		ar.serialize(RadianWrapper(sightSector), "sightSector", "Сектор обзора");
		ar.serialize(sightSectorColorIndex, "sightSectorColorIndex", "Номер цвета сектора обзора");
		ar.serialize(showSightSector, "showSightSector", "Показывать сектор обзора");
		ar.serialize(sightRadiusNightFactor, "sightRadiusNightFactor", "Коэффициент для радиуса видимости ночью");
		ar.serialize(sightFogOfWarFactor, "sightFogOfWarFactor", "Коэффициент для радиуса открытия тумана войны");
		ar.serialize(upgradeStop, "upgradeStop", "Останавливать при Upgrade-е");
		ar.serialize(forwardVelocity, "forwardVelocity", 0);
		ar.serialize(enableRotationMode, "enableRotationMode", "Разворачивать на месте");
		ar.serialize(RangedWrapperf(forwardVelocityRunFactor, 0, 5, 0.1f), "forwardVelocityRunFactor",  "Коэффициент скорости движения при беге");
		ar.serialize(RangedWrapperf(sideMoveVelocityFactor, 0, 5, 0.1f), "sideMoveVelocityFactor",  "Коэффициент скорости движения в сторону");
		ar.serialize(RangedWrapperf(backMoveVelocityFactor, 0, 5, 0.1f), "backMoveVelocityFactor",  "Коэффициент скорости движения назад");
		ar.serialize(RangedWrapperf(angleRotationMode, 1, 3600, 1), "angleRotationMode",  "Скорость разворота на месте");
		ar.serialize(RangedWrapperf(angleSwimRotationMode, 1, 3600, 1), "angleSwimRotationMode",  "Скорость разворота на месте при плавании");
		ar.serialize(RangedWrapperf(pathTrackingAngle, 2, 180, 1), "pathTrackingAngle",  "Угол поворота ( 5 - минимальное значение для наземных )");
		if(ar.openBlock("velocityFactorsByTerrain", "Коэффициенты скорости движения для типов поверхности")){
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[0], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain0", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE0));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[1], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain1", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE1));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[2], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain2", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE2));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[3], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain3", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE3));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[4], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain4", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE4));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[5], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain5", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE5));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[6], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain6", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE6));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[7], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain7", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE7));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[8], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain8", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE8));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[9], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain9", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE9));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[10], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain10", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE10));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[11], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain11", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE11));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[12], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain12", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE12));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[13], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain13", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE13));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[14], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain14", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE14));
			ar.serialize(RangedWrapperf(velocityFactorsByTerrain[15], 0.0f, 1.0f, 0.1f), "velocityFactorsByTerrain15", TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE15));
			if(!ar.isEdit() && ar.isOutput()){
				impassability = 0;
				for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
					if(velocityFactorsByTerrain[i] < 1.0e-6f)
                        impassability |= 1 << i;
			}
			ar.serialize(impassability, "impassibility", 0);
			ar.closeBlock();
		}			
		ar.serialize(sinkInLava, "sinkInLava", "Тонет в лаве");
		ar.serialize(moveSinkVelocity, "moveSinkVelosity", "Скорость погружения когда движется");
		ar.serialize(standSinkVelocity, "standSinkVelosity", "Скорость погружения когда стоит");
		ar.serialize(waterVelocityFactor, "waterVelocityFactor", "Коэффициент скорости движения для воды");
		ar.serialize(resourceVelocityFactor, "resourceVelocityFactor", "Коэффициент скорости движения с ресурсом");
		ar.serialize(RangedWrapperf(waterFastVelocityFactor, 0, 5, 0.1f), "waterFastVelocityFactor",  "Коэффициент скорости быстрого движения для воды");
		ar.serialize(manualModeVelocityFactor, "manualModeVelocityFactor", "Коэффициент скорости движения в прямом управлении");
		ar.serialize(flyingHeight, "flyingHeight", "Высота для летающих юнитов");
		ar.serialize(RangedWrapperf(flyingHeightDeltaFactor, 0.0f, 1.0f), "flyingHeightDeltaFactor", "Относительная амплитуда колебаний высоты для летающих юнитов");
		ar.serialize(flyingHeightDeltaPeriod, "flyingHeightDeltaPeriod", "Период колебаний высоты для летающих юнитов");
		ar.serialize(RadianWrapper(additionalHorizontalRot), "additionalHorizontalRot", "Максимальный угол наклолна при повороте");
		ar.serialize(waterLevel, "waterLevel", "Глубина для плавующих юнитов");
		ar.serialize(useLocalPTStopRadius, "useLocalPTStopRadius", "Собственные настройки радиуса остановки");
		if(useLocalPTStopRadius)
			ar.serialize(localPTStopRadius, "localPTStopRadius", "Радиус остановки");
		ar.serialize(waterWeight_, "waterWeight_", "Коэффициент воздействия течения воды [0..100]");
		ar.serialize(enablePathTracking, "enablePathTracking", "Включить объезд юнитов");
		ar.serialize(enableVerticalFactor, "enableVerticalFactor", "Влючить замедление/ускорение на горах");
		ar.serialize(rotToTarget, "rotToTarget", "Поворачивать к цели при атаке");
		ar.serialize(rotToNoiseTarget, "rotToNoiseTarget", "Поворачивать к источнику шума");
		ar.serialize(ptSideMove, "ptSideMove", "Двигатся боком/задом при прямом управлении");
		ar.serialize(realSuspension, "realSuspension", "Честная подвеска");
		if(!ar.isEdit() && ar.isOutput() && !rigidBodyCarPrm.empty())
			rigidBodyCarPrm.update(logicModel(), boundBox.center());
		ar.serialize(rigidBodyCarPrm, "rigidBodyCarPrm", "Подвеска");
		
		ar.serialize(wheelList, "wheelList", "Колеса");

		ar.serialize(traceInfos, "traceInfos", "следы");

		ar.serialize(traceNodes, "TraceNodes", "Объекты для привязки следов");
		ar.serialize(grassTrack, "grassTrack", "Оставлять след на траве");

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
	}

    ar.serialize(levels, "levels", "Уровни развития");

	if(ar.openBlock("Construction", "Строительство зданий")){
		ar.serialize(constructedBuildings, "constructedBuildings", "Создаваемые здания");
		ar.serialize(constructionPower, "constructionPower", "Производительность в секунду");
		ar.serialize(constructionCost, "constructionCost", "Стоимость в секунду");
		ar.serialize(constructionProgressPercent, "constructionProgressPercent", "Коэффициент увеличения стоимости строительства с увеличением числа строителей");
		ar.closeBlock();
	}

	ar.serialize(itemDropRadius, "itemDropRadius", "Радиус выбрасывания предмета из инвентаря");

	ar.serialize(pickedItems, "pickedItems", "Подбираемые предметы");
	if(ar.openBlock("Resourcer", "Ресурсодобыча")){
		ar.serialize(resourcer, "resourcer", "Сборщик ресурсов");
		ar.serialize(resourcerCapacities, "resourcerCapacities", "Емкости ресурсодобытчика в порядке цепочек анимации");
		ar.serialize(resourceCollectors, "resourceCollectors", "Здания для складирования ресурсов");
		ar.closeBlock();
	}

	collisionGroup = COLLISION_GROUP_REAL;
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
	, formationUnit_(rigidBody())
	, formationController_(0, true)
{
	staticReasonXY_ = 0;
	isForceMainUnit_ = false;
	onWater = false;
	selectAble_ = true;
	isMovingToTransport_ = false;
	autoFindTransport_ = attr().autoFindTransport;
	
	requestStatus_ = 0;

	squad_ = NULL;
	fireStatus_ = 0;

	manualAttackTarget_ = false;
	hasAttackPosition_ = false;
	attackPosition_ = Vect3f::ZERO;
	
	manualModeVelocityFactor = 1.0f;
	
	manualMovementMode = MODE_WALK;

	level_ = -1;
	checkLevel(GlobalAttributes::instance().applyArithmeticsOnCreation);

	resourcerMode_ = RESOURCER_IDLE;
	
	transportSlotIndex_ = 0;

	canFireInTransport_ =false;

	directControlPrev_ = false;

	nearConstructedBuilding_ = false;

	resourcerCapacityIndex_ = 0;

	if(attr().hasInventory()){
		inventory_.reserve(attr().inventories.size());
		for(UI_InventoryReferences::const_iterator it = attr().inventories.begin(); it != attr().inventories.end(); ++it){
			inventory_.add(it->control());
		}
	}
	
	inventory_.setOwner(this);

	for(AttributeItemInventoryReferences::const_iterator itm = attr().equipment.begin(); itm != attr().equipment.end(); ++itm){
		if(const AttributeBase* attr = *itm)
			putToInventory(InventoryItem(*safe_cast<const AttributeItemInventory*>(attr)));
	}

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
					//traceControllers_.push_back(TraceController(this, attr().traceInfos[j].traceTerTool_, attr().traceInfos[j].surfaceKind_, (int)attr().traceNodes[i], wheelNode));
					traceControllers_.push_back(TraceController(this, attr().traceInfos[j], (int)attr().traceNodes[i], wheelNode));
				}
			}
		}
		else {
			traceControllers_.reserve(attr().traceInfos.size());
			for(int j = 0; j < attr().traceInfos.size(); j++)
				//traceControllers_.push_back(TraceController(this, attr().traceInfos[j].traceTerTool_, attr().traceInfos[j].surfaceKind_));
				traceControllers_.push_back(TraceController(this, attr().traceInfos[j]));
		}
	}
	traceStarted_ = false;

	stateController_.initialize(this, UnitLegionaryPosibleStates::instance());

	waterPlume = NULL;

	resourceCollectorI_ = resourceCollectorJ_ = 0;
	resourceCollectorDistance_ = 0;

	moveToCursor_ = false;

	formationUnit_.setOwnerUnit(this);

	formationController_.addUnit(&formationUnit_);
	formationController_.setMainUnit(&formationUnit_);
}

UnitLegionary::~UnitLegionary()
{
	RELEASE(waterPlume);
}

void UnitLegionary::setSquad(UnitSquad* p)
{
	squad_ = p;
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
	float water_z = environment->water()->GetZFast(position.xi(), position.yi());
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

void UnitLegionary::setPose(const Se3f& poseIn, bool initPose)
{
	__super::setPose(poseIn, initPose);

	if(initPose){
		rigidBody()->setSinkParameters(attr().sinkInLava && water && water->isLava(), attr().moveSinkVelocity, attr().standSinkVelocity);
		formationUnit_.initPose();
		if(squad())
			squad()->updatePose(true);
		formationController_.initPose(position2D(), angleZ());
	}
}

void UnitLegionary::relaxLoading()
{
	#ifndef _FINAL_VERSION_
	if(alive() && !squad()){
		if(isUnderEditor()){
			setSquad(safe_cast<UnitSquad*>(player()->buildUnit(&*attr().squad)));
			squad()->setPose(pose(), true);
			squad()->addUnit(this);
		}
		xassertStr(squad() && "У юнита нет сквада, он будет удален", attr().libraryKey());
		if(!squad()){
			Kill();
			return;
		}
	}
	#endif

	__super::relaxLoading();
}

void UnitLegionary::Quant()
{
	start_timer_auto();

	__super::Quant();

	formationController_.computePose();

	if(!formationController_.wayPointEmpty())
		formationController_.quant();

	isMoving_ = formationUnit_.unitMove();

	if(!alive()){
		if(!isFrozen())
			rigidBody()->unFreeze();
		return;
	}

	if(isFrozen()){
		if(!rigidBody()->isBoxMode()){
			rigidBody()->setWaterAnalysis(false);
			rigidBody()->enableBoxMode();
		}
	}else
		rigidBody()->unFreeze();
		
	if(rigidBody()->isSinking() && isEq(rigidBody()->flyingHeightCurrent(), -(rigidBody()->centreOfGravityLocal().z + rigidBody()->extent().z))){
		ParameterSet damage(parameters());
		damage.set(0);
		damage.set(1e+10, ParameterType::HEALTH);
		damage.set(1e+10, ParameterType::ARMOR);
		setDamage(damage, 0);
	}

	if(!alive())
		return;

	ParameterSet recovery;
	recovery.setRecovery(parameters());
	parameters_.scaleAdd(recovery, logicPeriodSeconds);
	parameters_.clamp(parametersMax());

	checkLevel(true);

	if(targetUnit_){
		targetUnit_->setAimed();
		if(!targetEventTimer_.busy()){
			targetEventTimer_.start(targetEventTime + logicRND(targetEventTime));
			player()->checkEvent(EventUnitMyUnitEnemy(Event::AIM_AT_OBJECT, targetUnit_, this));
			targetUnit_->player()->checkEvent(EventUnitMyUnitEnemy(Event::AIM_AT_OBJECT, targetUnit_, this));
		}
	}

	if(moveToCursor_){
		if(isSyndicateControl() && player()->shootKeyDown())
			squad()->addWayPointS(player()->shootPoint2D());
		else
			moveToCursor_ = false;
	}
	
	if(!rigidBody()->onWater() && !rigidBody()->isBoxMode() && isMoving()){
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

	bool onWaterNew((rigidBody()->onLowWater() || rigidBody()->onWater()) && !rigidBody()->onDeepWater() && !hiddenGraphic());
	if(onWater != onWaterNew){
		if(!onWater){
			if(modelLogic()){
				waterPlume = new cWaterPlume(this, environment->waterPlumeAtribute());
				waterPlume->setScene(terScene);
				attachSmart(waterPlume);
			}
		}else 
			RELEASE(waterPlume);
		onWater = onWaterNew;
	}
	if(onWater){
		streamLogicCommand.set(fCommandUpdateWaterPlum) << waterPlume << position() << (formationUnit_.unitMove() ? 
			1.0f / (rigidBody()->forwardVelocity() + 1.0f) : 0.0f);
		cWaterPlume::NodeContainer::iterator nd;
		FOR_EACH(waterPlume->nodes(), nd)
			streamLogicCommand << modelLogicNodePosition(nd->index).trans();
	}

	if(attr().grassTrack&&isMoving())
		streamLogicCommand.set(fCommandDownGrass) << rigidBody()->position() << rigidBody()->extent().x;

	resourcerQuant();

	if(autoFindTransport_ && !transportFindingPause_.busy()){
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

	if(itemToPick_ && currentState() != CHAIN_PICK_ITEM){
		// подбор предмета
		if(nearUnit(itemToPick_)){
			if(putToInventory(itemToPick_))
				itemToPick_->Kill();
			else
				itemToPick_->jump();
			itemToPick_ = 0;
			startState(StatePickItem::instance());
		}
	}

	if(runMode() && formationUnit_.unitMove() && !attr().runningConsumption.empty()){
		ParameterSet runningConsumption = attr().runningConsumption;
		runningConsumption *= logicPeriodSeconds;
		if(requestResource(runningConsumption, NEED_RESOURCE_TO_MOVE))
			subResource(runningConsumption);
		else
			squad()->executeCommand(UnitCommand(COMMAND_ID_CHANGE_MOVEMENT_MODE, (class UnitInterface*)0, MODE_WALK));
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

	if(squad()->attr().automaticJoin && squad()->unitsNumber() > 1 
	  && position2D().distance2(squad()->position2D()) > sqr(1.5f*squad()->attr().automaticJoinRadius)){
		if(squad()->getUnitReal() == this){
			const LegionariesLinks& units = squad()->units();
			while(units.size() > 1){
				UnitLegionary* unit = units.back();
				squad()->removeUnit(unit);
				UnitSquad* newSquad;
				unit->setSquad(newSquad = safe_cast<UnitSquad*>(player()->buildUnit(&*unit->attr().squad)));
				newSquad->setPose(unit->pose(), true);
				newSquad->addUnit(unit);
				if(squad()->waitingMode())
					newSquad->setWaitingMode(UnitSquad::WAITING_ALL);
			}
		}else{
			squad()->removeUnit(this);
			setSquad(safe_cast<UnitSquad*>(player()->buildUnit(&*attr().squad)));
			squad()->setPose(pose(), true);
			squad()->addUnit(this);
			if(squad()->waitingMode())
				squad()->setWaitingMode(UnitSquad::WAITING_ALL);
		}
	}
}

void UnitLegionary::explode()
{
	__super::explode();

	if(attr().dropInventoryItems)
		inventory_.dropItems();
}

void UnitLegionary::setCorpse()
{
	__super::setCorpse();

	if(deathAttr().explodeReference->enableRagDoll){
		rigidBody()->setWaterAnalysis(false);
		rigidBody()->enableBoxMode();
	}else if((deathAttr().explodeReference->animatedDeath && rigidBody()->onDeepWater()) || !rigidBody()->colliding()){
		if(rigidBody()->prm().hoverMode || rigidBody()->flyingMode()){
			rotationDeath = logicRNDinterval(-7, 7);
			rigidBody()->setGravityZ(-rigidBody()->prm().gravity / logicRNDfabsRndInterval(5.0f, 15.0f));
			rigidBody()->setWaterAnalysis(true);
			rigidBody()->setFriction(0.05f);
		}else if(rigidBody()->onDeepWater()){
			rigidBody()->setGravityZ(-10.0f);
			rigidBody()->setWaterAnalysis(false);
		}
		if(rigidBody()->prm().moveVertical || deathAttr().explodeReference->animatedDeath)
			rigidBody()->setAngularEvolve(false);
		rigidBody()->enableBoxMode();
		if(isDocked()){
			Vect3f velocity(0, rigidBody()->prm().forward_velocity_max, 0);
			dock()->pose().xformVect(velocity);
			rigidBody()->setVelocity(velocity);
		}
		if(!deathAttr().explodeReference->animatedDeath)
			setChainByHealth(CHAIN_DEATH);
	}else{
		if(rigidBody()->isBoxMode())
			rigidBody()->disableBoxMode();
		rigidBody()->setFlyingModeEnabled(false);
		makeStaticXY(STATIC_DUE_TO_DEATH);
	}
}

MovementState UnitLegionary::getMovementState()
{
	MovementState state = __super::getMovementState();

	// Выставляем направления движения
	if(!formationUnit_.manualMoving() && formationUnit_.getRotationMode() == FormationUnit::ROT_LEFT)
		state.state() |= ANIMATION_STATE_LEFT;
	else if(!formationUnit_.manualMoving() && formationUnit_.getRotationMode() == FormationUnit::ROT_RIGHT)
		state.state() |= ANIMATION_STATE_RIGHT;
	else if(formationUnit_.ptDirection() == FormationUnit::PT_DIR_FORWARD)
		state.state() |= ANIMATION_STATE_FORWARD;
	else if(formationUnit_.ptDirection() == FormationUnit::PT_DIR_BACK)
		state.state() |= ANIMATION_STATE_BACKWARD;
	else if(formationUnit_.ptDirection() == FormationUnit::PT_DIR_LEFT)
		state.state() |= ANIMATION_STATE_LEFT;
	else if(formationUnit_.ptDirection() == FormationUnit::PT_DIR_RIGHT)
		state.state() |= ANIMATION_STATE_RIGHT;
	else
		state.state() |= ANIMATION_STATE_ALL_SIDES;

	state.state() |= formationUnit_.movementMode();

	if(getStaticReasonXY() & STATIC_DUE_TO_WAITING)
		state.state() |= ANIMATION_STATE_WAIT;
	else if(formationUnit_.unitMove())
		state.state() |= ANIMATION_STATE_MOVE;
	else if(formationUnit_.unitRotate())
		state.state() |= ANIMATION_STATE_TURN;
	else 
		state.state() |= ANIMATION_STATE_STAND;
	
	if(unitState() == ATTACK_MODE)
		state.state() |= ANIMATION_STATE_ATTACK;

	return state;
}

WeaponAnimationType UnitLegionary::weaponAnimationType() const
{
	if(weaponSlots_.size() > 0 && !weaponSlots_.front().isEmpty())
		return getWeaponAnimationType(weaponSlots_.front());
	return WeaponAnimationType();
}

void UnitLegionary::setMovementChain()
{
	MovementState mstate = getMovementState();
	const AbnormalStateType* astate = abnormalStateType();
	if(movementStatePrev_ != mstate){
		if(movementStatePrev_ != MovementState::EMPTY){
			WeaponAnimationType weaponType = weaponAnimationType();
			if(bodyParts_.empty() || !weaponType || !(mainChain_ = attr().animationChainTransition(1.0f - health(), abnormalStateType(), movementStatePrev_, mstate, weaponType)))
				mainChain_ = attr().animationChainTransition(1.0f - health(), abnormalStateType(), movementStatePrev_, mstate);
			if(mainChain_){
				if(movementStatePrev_.state() & ANIMATION_STATE_WAIT)
					makeStaticXY(UnitActing::STATIC_DUE_TO_ANIMATION);
				movementChainDelayTimer_.start(attr().chainChangeMovementMode);
			}
		}
		movementStatePrev_ = mstate;
	}
	if(!movementChainDelayTimer_.busy()){
		makeDynamicXY(UnitActing::STATIC_DUE_TO_ANIMATION);
		if(bodyParts_.empty())
			mainChain_ = attr().animationChainByFactor(CHAIN_MOVEMENTS, 1.0f - health(), astate, mstate);
		else{
			WeaponAnimationType weaponType = weaponAnimationType();
			if(!weaponType || !(mainChain_ = attr().animationChain(CHAIN_MOVEMENTS, -1, astate, mstate, weaponType)))
				mainChain_ = attr().animationChain(CHAIN_MOVEMENTS, -1, astate, mstate);
		}
	}
	chainControllers_.setChain(mainChain_, mstate);
	modelLogicUpdated_ = false;
}

void UnitLegionary::Kill()
{
	if(dead())
		return;

	if(squad())
		squad()->removeUnit(this);

	directControlPrev_ = false;

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
		formationController_.addWayPointS(command.position());
		break;
	case COMMAND_ID_OBJECT:
	case COMMAND_ID_STOP: 
		interuptState(StatePickItem::instance());
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
			Vect3f delta = command.position() - position();

			if(delta.norm2() > sqr(attr().itemDropRadius))
				delta.normalize(attr().itemDropRadius);

			p->setPose(Se3f(MatXf(Mat3f::ID, position() + delta)), true);
			safe_cast<UnitInterface*>(p)->setParameters(inventory_.takenItem().parameters());
			inventory_.removeTakenItem();
		}
		break;
	case COMMAND_ID_ITEM_TAKEN_TRANSFER:
		if(inventory_.takenItem()()){
			UnitInterface* unit = command.unit();
			if(unit->putToInventory(inventory_.takenItem()))
				inventory_.removeTakenItem();
		}
		break;

	case COMMAND_ID_CHANGE_MOVEMENT_MODE:
		if(command.commandData() != MODE_RUN)
			manualMovementMode &= MOVEMENT_STATE_RUN;
		manualMovementMode |= command.commandData();
		formationUnit_.setMovementMode(manualMovementMode);
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
	if(unitState() == AUTO_MODE && autoAttackMode() == ATTACK_MODE_OFFENCE && targetUnit())
		return;

	switch(resourcerMode_){
	case RESOURCER_FINDING:
		if(resourceItem_ && !resourceItem_->isUnseen()){
			if(nearUnit(resourceItem_)){
				resourcerMode_ = RESOURCER_PICKING;
			}
		}
		else if(!resourceFindingPause_.busy()){
			resourceFindingPause_.start(logicRND(3000));
			FindItemOp op(this);
			universe()->unitGrid.Scan(round(position().x), round(position().y), int(sightRadius()), op);
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
					nearUnit(resourceCollector_);
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
	UnitLegionary* leader = squad()->getUnitReal();
		
	if(this == leader)
		return true;
	
	if(leader->unitState() == UnitReal::AUTO_MODE)
		return true;

	if(leader->unitState() == UnitReal::ATTACK_MODE && leader->fireTargetExist() && (!canAttackTarget(leader->fireTarget())))
		return true;

	return false;
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

	return universe()->unitGrid.ConditionScan(position().xi(), position().yi(), int(sightRadius()), find_op);
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
	if(transport_->player() != player())
		transport_->changeUnitOwner(player());
	const TransportSlot& slot = transport_->attr().transportSlots[transportSlotIndex_];
	if(!slot.visible){
		hide(HIDE_BY_TRANSPORT, true);
		chainControllers_.clear();
	}
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
	if(!slot.dockWhenLanding)
		attachToDock(transport_, slot.node, false);
	transport_->applyParameterArithmetics(attr().additionToTransport);
	transport_->resetCargo();
	isMovingToTransport_ = false;
	transport_->updateSelectionListPriority();
}

void UnitLegionary::putUnitOutTransport()
{
	if(transport_){
		if(inTransport())
			transport_->applyParameterArithmetics(ParameterArithmetics(attr().additionToTransport, true));
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
	isMovingToTransport_ = false;
	if(squad())
		squad()->wayPointsClear();
	transport_ = 0;
}

void UnitLegionary::showDebugInfo()
{
	__super::showDebugInfo();

	formationUnit_.show();

	formationController_.showDebugInfo();

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
			attr().resourcerCapacities.front().showDebug(position(), Color4c::YELLOW);
	}

	if(showDebugLegionary.trace){
		TraceControllers::iterator it;
		FOR_EACH(traceControllers_, it)
			it->showDebugInfo();
	}

	if(showDebugLegionary.aimed && aimed())
		buf < "aimed\n";

	if(showDebugLegionary.usedByTrigger && usedByTrigger())
		buf < usedTriggerName() < "\n";

	if(buf.tell()){
		buf -= 2;
		show_text(position() + Vect3f(0, 0, 15), buf, Color4c::RED);
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
	if(isUnseen() && !player()->active() && universe()->activePlayer()->clan() != player()->clan())
		return;

	__super::graphQuant(dt);

	if(unvisible())
		return;

	if(runMode() && alive() && !isDirectControl())
		player()->race()->runModeSprite().draw(interpolatedPose().trans(), attr().initialHeightUIParam, player()->unitColor());

	if(selected() && level() >= 0)
		attr().levels[level()].sprite.draw(interpolatedPose().trans(), attr().initialHeightUIParam, player()->unitColor());

	if(attr().showSightSector && squad() && squad()->attr().showSightSector)
		environment->fieldOfViewMap()->add(position(), angleZ(), sightRadius(), attr().sightSector, attr().sightSectorColorIndex);
}

void UnitLegionary::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(squad_, "squad", 0);

	ar.serialize(isForceMainUnit_, "isForceMainUnit", 0);

	int level = level_;
	ar.serialize(level, "level", 0);
	if(ar.isInput())
		setLevel(level, false);

	if(universe()->userSave()){
		int staticReasonXY(staticReasonXY_);
		ar.serialize(staticReasonXY, "staticReasonXY", 0);
		if(ar.isInput()&& staticReasonXY)
			makeStaticXY(staticReasonXY);
		
		ar.serialize(movementStatePrev_, "movementChainDelayTimer", 0);
		ar.serialize(movementChainDelayTimer_, "movementChainDelayTimer", 0);

		ar.serialize(transport_, "transport", 0);
		ar.serialize(transportSlotIndex_, "transportSlot", 0);
		ar.serialize(canFireInTransport_, "canFireInTransport", 0);
		ar.serialize(isMovingToTransport_, "isMovingToTransport", 0);
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

		ar.serialize(inventory_, "inventory", 0);

		ar.serialize(formationController_, "formationController", 0);
	}
	ar.serialize(manualMovementMode, "manualMovementMode", "Режим движения");
	if(ar.isInput()){
		if(ar.isEdit()){
			int moveMode = manualMovementMode & ~MODE_RUN;
			if(moveMode != MODE_WALK && moveMode != MODE_GRABBLE && moveMode != MODE_CRAWL)
				manualMovementMode = MODE_WALK | (manualMovementMode & MODE_RUN);
		}
		formationUnit_.setMovementMode(manualMovementMode);
	}

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
		applyPickedItem(unit);
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

void UnitLegionary::setSelected(bool selected)
{
	__super::setSelected(selected);

	if(!selected && inventory_.takenItem()()){
		if(MT_IS_GRAPH())
			sendCommand(UnitCommand(COMMAND_ID_ITEM_RETURN, -1));
		else
			inventory_.returnItem();
	}
}

void UnitLegionary::clearOrders()
{
	bool allSlotsEmpty = true;
	for(int i = 0; i < transportSlots().size(); i++)
		if(UnitLegionary* transportSlot = transportSlots()[i]){
			if(!transportSlot->isDocked() && (transportSlot->player() == player()))
				transportSlot->clearTransport();
			else
				allSlotsEmpty = false;
		}
	if(allSlotsEmpty)
		interuptState(StateOpenForLanding::instance());
	interuptState(StatePickItem::instance());
	itemToPick_ = 0;
	squad()->wayPointsClear();
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
}

void UnitLegionary::changeUnitOwner(Player* playerIn)
{
	if(player() == playerIn)
		return;

	clearOrders();

	if(squad()){
		squad()->removeUnit(this);
		__super::changeUnitOwner(playerIn);
		setSquad(safe_cast<UnitSquad*>(playerIn->buildUnit(&*attr().squad)));
		squad()->setPose(pose(), true);
		squad()->addUnit(this);
		setSelected(false);
	}
	else
		__super::changeUnitOwner(playerIn);
}

void UnitLegionary::makeStatic(int staticReason) 
{ 
	__super::makeStatic(staticReason);
	makeStaticXY(staticReason);
}

void UnitLegionary::makeDynamic(int staticReason) 
{ 
	__super::makeDynamic(staticReason);
	makeDynamicXY(staticReason);
}

void UnitLegionary::makeStaticXY(int staticReason) 
{ 
	staticReasonXY_ |= staticReason; 
	formationUnit_.makeStatic();
}

void UnitLegionary::makeDynamicXY(int staticReason) 
{ 
	staticReasonXY_ &= ~staticReason; 
	if(!staticReasonXY_ && formationUnit_.unmovable()){
		formationUnit_.makeDynamic();
		if(squad())
			squad()->updatePose(true);
	}
}

void UnitLegionary::setForceMainUnit(bool isForceMainUnit) 
{ 
	if(isForceMainUnit_ != isForceMainUnit){
		isForceMainUnit_ = isForceMainUnit; 
		if(UnitActing* transport = transport_)
			transport->updateSelectionListPriority();
	}
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

bool UnitLegionary::setItemToPick(UnitBase* item, bool checkInventory)
{
	if(!item->attr().isInventoryItem()) 
		return false;

	UnitItemInventory* p = safe_cast<UnitItemInventory*>(item);

	if(checkInventory && !canPutToInventory(p))
		return false;

	itemToPick_ = p;
	return true;
}

bool UnitLegionary::uniform(const UnitReal* unit) const
{
	return !unit || &attr() == &unit->attr() && level() == safe_cast<const UnitLegionary*>(unit)->level() && directControl() == safe_cast<const UnitActing*>(unit)->directControl();
}

bool UnitLegionary::prior(const UnitInterface* unit) const
{
	if(unit->attr().isLegionary()){
		if(directControl() != safe_cast<const UnitActing*>(unit)->directControl())
			return directControl() > safe_cast<const UnitActing*>(unit)->directControl();

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

bool UnitLegionary::putToInventory(const InventoryItem& item)
{
	return inventory_.putItem(item);
}

bool UnitLegionary::reloadInventoryWeapon(int slot_index)
{
	return inventory_.reloadWeapon(slot_index);
}

bool UnitLegionary::removeInventoryWeapon(int slot_index)
{
	return inventory_.removeWeapon(slot_index);
}

bool UnitLegionary::updateInventoryWeapon(int slot_index)
{
	return inventory_.updateWeapon(slot_index);
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

float UnitLegionary::forwardVelocity(bool moveback)
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
		return factor * (formationUnit_.isRunMode() ? forwardVelocity_ * attr().forwardVelocityRunFactor : forwardVelocity_);

	if(formationUnit_.ptDirection() == FormationUnit::PT_DIR_LEFT || formationUnit_.ptDirection() == FormationUnit::PT_DIR_RIGHT)
		factor *= attr().sideMoveVelocityFactor;
	if(formationUnit_.ptDirection() == FormationUnit::PT_DIR_BACK)
		factor *= attr().backMoveVelocityFactor;

	if(rigidBody()->onWater())
		return factor * forwardVelocity_ * (formationUnit_.isRunMode() ? attr().waterFastVelocityFactor : attr().waterVelocityFactor);
	else{
		return factor * formationUnit_.computeTerrainVelocityFactor(moveback) * forwardVelocity_ * (formationUnit_.isRunMode() ? attr().forwardVelocityRunFactor : 1.0f);
	}

}

bool UnitLegionary::setState(UnitLegionary* unit)
{
	if(unit->targetPointEnable && unit->unitState() == UnitReal::ATTACK_MODE){
		selectWeapon(0);
		setTargetPoint(unit->targetPoint_);
		return true;
	}
	else if(unit->targetUnit_ && unit->unitState() == UnitReal::ATTACK_MODE){
		selectWeapon(0);
		setManualTarget(unit->targetUnit_);
		return true;
	}

	return false;
}

void UnitLegionary::giveResource()
{
	player()->addResource(resourcerCapacity_, true);
	resourcerMode_ = RESOURCER_FINDING;
	resourcePickingConsumer_.clear();
	ShowChangeParametersController doShow(resourceCollector_, resourcerCapacity_);
	setResourceItem(resourceItem_);
}

void UnitLegionary::enableWaitingMode()
{
	if(!(getStaticReasonXY() & STATIC_DUE_TO_WAITING)){
		makeStaticXY(STATIC_DUE_TO_WAITING);
		startEffect(&squad()->attr().waitingUnitEffect);
	}
}

void UnitLegionary::disableWaitingMode()
{
	if(getStaticReasonXY() & STATIC_DUE_TO_WAITING){
		stopEffect(&squad()->attr().waitingUnitEffect);
		makeDynamicXY(STATIC_DUE_TO_WAITING);
	}
}

bool UnitLegionary::nearUnit(const UnitBase* unit, float radiusMin)
{
	formationUnit_.setIgnoreUnit(unit);
	if(position2D().distance2(unit->position2D()) < sqr(radius() + unit->radius() + radiusMin)){
		formationUnit_.setIgnoreUnit(0);
		if(unitState() != TRIGGER_MODE)
			setUnitState(AUTO_MODE);
		formationUnit_.clearPersonalWayPoint();
		if(!attr().rotToTarget)
			return true;
		return formationUnit_.rot2point(unit->position2D());
	}
	else
		formationController_.addWayPointS(unit->position2D());
	return false;
}

bool UnitLegionary::moveToTransport()
{
	const TransportSlot& slot = transport()->attr().transportSlots[transportSlotIndex()];
	if(slot.dockWhenLanding){
		Vect2f point = transport()->modelLogicNodePosition(slot.node).trans();
		formationUnit_.setIgnoreUnit(transport());
		if(formationUnit_.isPointReached(point)){
			formationUnit_.setIgnoreUnit(0);
			if(unitState() != TRIGGER_MODE)
				setUnitState(AUTO_MODE);
			formationUnit_.clearPersonalWayPoint();
			if(!attr().rotToTarget)
				return true;
			return formationUnit_.rot2point(point);
		}
		else
			formationController_.addWayPointS(point);
		return false;
	}
	return nearUnit(transport(), 0.0f);
}

bool UnitLegionary::wayPointsEmpty()
{
	return formationController_.wayPointEmpty() || formationUnit_.isPointReached(formationController_.lastWayPoint());
}

float UnitLegionary::getPathFinderDistance(const UnitBase* unit_) const
{
	PathFinder::PFTile flags = 0;

	if(rigidBody()->prm().groundPass)
		flags |= PathFinder::GROUND_FLAG;

	if(rigidBody()->prm().waterPass)
		flags |= PathFinder::WATER_FLAG;

	if(rigidBody()->prm().fieldPass)
		flags |= PathFinder::FIELD_FLAG;
	else
		flags |= 1 << (20 + player()->playerID());

	flags |= int(attr().environmentDestruction);

	vector<Vect2f> points_;
	bool pathFind_ = pathFinder->findPath(position2D(), unit_->position2D(), flags, impassability(), attr().velocityFactorsByTerrain, points_);

	if(points_.size() < 2)
		return position2D().distance(unit_->position2D());

	float retDist = sqrtf(sqr(position2D().x - points_[0].x) + sqr(position2D().y - points_[0].y));
	for(int i =1; i< points_.size(); i++)
		retDist += sqrtf(float(sqr(points_[i].x - points_[i-1].x) + sqr(points_[i].y - points_[i-1].y)));

	return retDist;
}

UnitInterface* UnitLegionary::attackTarget(UnitInterface* unit, int weapon_id, bool moveToTarget)
{
	if(UnitActing::attackTarget(unit, weapon_id, moveToTarget))
		return unit;
	if(moveToTarget && squad()->getUnitReal() == this)
		squad()->addWayPointS(unit->position2D());
	return 0;
}

void UnitLegionary::executeDirectKeys(const UnitCommand& command)
{
	__super::executeDirectKeys(command);

	if(isDirectControl()){
		manualModeVelocityFactor = attr().manualModeVelocityFactor;
		
		int keys = command.commandData();

		if(attr().rotToTarget){
			Vect2f viewPoint = command.position();

			if(attr().ptSideMove && (keys & DIRECT_KEY_STRAFE_LEFT)) {
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_LEFT);
				formationUnit_.manualMove(manualModeVelocityFactor);
				keys = 0;
			}

			if(attr().ptSideMove && (keys & DIRECT_KEY_STRAFE_RIGHT)) {
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_RIGHT);
				formationUnit_.manualMove(manualModeVelocityFactor);
				keys = 0;
			}

			keys &= DIRECT_KEY_MOVE_FORWARD | DIRECT_KEY_MOVE_BACKWARD |
				DIRECT_KEY_TURN_LEFT | DIRECT_KEY_TURN_RIGHT;

			if(keys){
				float vpAngle = 0;
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_FORWARD);
				switch(keys){
				case DIRECT_KEY_TURN_LEFT:
					if(attr().ptSideMove)
						formationUnit_.setPtDirection(FormationUnit::PT_DIR_LEFT);
					else
						vpAngle = -M_PI_2;
					break;
				case DIRECT_KEY_TURN_RIGHT:
					if(attr().ptSideMove)
						formationUnit_.setPtDirection(FormationUnit::PT_DIR_RIGHT);
					else
						vpAngle = M_PI_2;
					break;
				case DIRECT_KEY_MOVE_BACKWARD:
					if(attr().ptSideMove)
						formationUnit_.setPtDirection(FormationUnit::PT_DIR_BACK);
					else
						vpAngle = M_PI;
					break;
				case DIRECT_KEY_TURN_LEFT + DIRECT_KEY_MOVE_FORWARD:
					vpAngle = -M_PI_4;
					break;
				case DIRECT_KEY_TURN_RIGHT + DIRECT_KEY_MOVE_FORWARD:
					vpAngle = M_PI_4;
					break;
				case DIRECT_KEY_TURN_LEFT + DIRECT_KEY_MOVE_BACKWARD:
					if(attr().ptSideMove){
						formationUnit_.setPtDirection(FormationUnit::PT_DIR_BACK);
						vpAngle = M_PI_4;
					}else
						vpAngle = -3.0f*M_PI_4;
					break;
				case DIRECT_KEY_TURN_RIGHT + DIRECT_KEY_MOVE_BACKWARD:
					if(attr().ptSideMove){
						formationUnit_.setPtDirection(FormationUnit::PT_DIR_BACK);
						vpAngle = -M_PI_4;
					}else
						vpAngle = 3.0f*M_PI_4;
					break;
				}

				Vect2f dir = viewPoint - position2D();
				Mat2f tq(vpAngle);
				dir *= tq;
				viewPoint = position2D() + dir;
				formationUnit_.manualMove(manualModeVelocityFactor);
			}

			formationUnit_.setManualViewPoint(viewPoint);
		}
		else {
			if(!formationUnit_.unitMove())
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_FORWARD);

			if(attr().ptSideMove && (keys & DIRECT_KEY_STRAFE_LEFT)) {
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_LEFT);
				formationUnit_.manualMove(manualModeVelocityFactor);
				return;
			}

			if(attr().ptSideMove && (keys & DIRECT_KEY_STRAFE_RIGHT)) {
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_RIGHT);
				formationUnit_.manualMove(manualModeVelocityFactor);
				return;
			}

			if(attr().ptSideMove && (keys & DIRECT_KEY_MOVE_BACKWARD)) {
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_BACK);
				formationUnit_.manualMove(manualModeVelocityFactor);
				return;
			}

			if(keys & DIRECT_KEY_MOVE_FORWARD){
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_FORWARD);
				formationUnit_.manualMove(manualModeVelocityFactor);
			}

			if(keys & DIRECT_KEY_MOVE_BACKWARD){
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_FORWARD);
				formationUnit_.manualMove(manualModeVelocityFactor, true);
			}

			if(keys & DIRECT_KEY_TURN_LEFT){
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_FORWARD);
				formationUnit_.manualTurn(manualModeVelocityFactor);
				if(!(keys & (DIRECT_KEY_MOVE_FORWARD | DIRECT_KEY_MOVE_BACKWARD))){
					formationUnit_.manualMove(manualModeVelocityFactor);
					manualModeVelocityFactor *= 0.5f;
				}
			}

			if(keys & DIRECT_KEY_TURN_RIGHT){
				formationUnit_.setPtDirection(FormationUnit::PT_DIR_FORWARD);
				formationUnit_.manualTurn(-manualModeVelocityFactor);
				if(!(keys & (DIRECT_KEY_MOVE_FORWARD | DIRECT_KEY_MOVE_BACKWARD))){
					formationUnit_.manualMove(manualModeVelocityFactor);
					manualModeVelocityFactor *= 0.5f;
				}
			}
		}
	}
}

void UnitLegionary::updateWayPointsForNewUnit(UnitLegionary* leader)
{
	if(setState(leader))
		return;
	startMoving();
}

void UnitLegionary::startMoving()
{
	if(unitState() == TRIGGER_MODE) 
		return;
	formationUnit_.disableRotationMode();
	setUnitState(MOVE_MODE);
	makeDynamicXY(STATIC_DUE_TO_ATTACK);
	if(isSyndicateControl())
		moveToCursor_ = true;
	return;
}

bool UnitLegionary::canMoveToFireTarget(float& fireRadius)
{
	WeaponTarget target = fireTarget();
	Rangef attackRange = fireDistance(target);
	if(attackRange.minimum() == FLT_INF)
		return false;
	if(fireDistanceCheck(target)){
		if(unitState() != UnitReal::AUTO_MODE && !player()->isAI())
			return false;
		if(!attackTargetUnreachable() || traceFireTarget()){
			setAttackTargetUnreachable(false);
			return false;
		}
		fireRadius = attackRange.minimum() + radius() + targetRadius() + rigidBody()->prm().is_point_reached_radius_max;
		return true;
	}
	Vect2f firePos = firePosition();
	if(position2D().distance2(firePos) < sqr(attackRange.minimum() + radius() + targetRadius())){
		fireRadius = attackRange.minimum() + radius() + targetRadius() + rigidBody()->prm().is_point_reached_radius_max;
		return true;
	}
	int radiusAim = target.unit() ? target.unit()->radius() : 1;
	if(unitState() == UnitReal::AUTO_MODE && attackModeAttr().targetInsideSightRadius() 
	&& player()->fogOfWarMap() && !player()->fogOfWarMap()->checkFogStateInCircle(Vect2i(firePos), radiusAim))
		return false;
	fireRadius = attackRange.maximum() + radius() + targetRadius() - rigidBody()->prm().is_point_reached_radius_max;
	return true;
}

void UnitLegionary::clearAttackTarget(int weaponID)
{
	__super::clearAttackTarget(weaponID);

	makeDynamicXY(STATIC_DUE_TO_ATTACK);
	formationUnit_.disableRotationMode();
}

bool UnitLegionary::rotToFireTarget()
{
	if(formationUnit_.unitMove()){
		if(isSyndicateControl() && attr().rotToTarget && fireTargetExist() && !fireTargetDocked())
			formationUnit_.rot2pointMoving(firePosition());
		return false;
	}

	if(isDirectControl())
		return false;

	if(attr().rotToTarget && ((fireTargetExist() && isSyndicateControl()) || fireDistanceCheck()) && !fireTargetDocked())
		return formationUnit_.rot2point(firePosition());

	return false;
}

bool UnitLegionary::rotToNoiseTarget()
{
	if(isDirectControl())
		return false;

	if(attr().rotToNoiseTarget && noiseTarget_ && !fireTargetExist()){
		if(formationController_.wayPointEmpty())
			formationController_.setWayPoint(position2D());
		return formationUnit_.rot2point(noiseTarget_->position2D());
	}
	return false;
}

bool UnitLegionary::isWorking() const 
{ 
	return __super::isWorking() || resourcerMode_ != RESOURCER_IDLE || teleport_ || constructedBuilding_; 
}

void UnitLegionary::setUsedByTrigger(int priority, const void* action, int time)
{
	squad()->setUsedByTrigger(priority, action, time);
}

//---------------------------------

TraceController::TraceController(UnitReal* owner, const TerToolCtrl& ctrl,/* unsigned surface,*/ int nodeIndex, bool wheelNode) : owner_(owner),
	traceCtrl_(ctrl),
	//surfaceKind_(surface),
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

	//if(surfaceKind_ != (SURFACE_KIND_1 | SURFACE_KIND_2 | SURFACE_KIND_3 | SURFACE_KIND_4)){
	//	Vect3f pos = position().trans();
	//	if(!(1 << vMap.getSurKind(pos.xi(), pos.yi()) & surfaceKind_))
	//		need_update = false;
	//}

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
	Color4c col(128, 128, 128);
	
	MatXf X(position());
	Vect3f delta = X.rot().xcol();
	delta.normalize(5);
	show_vector(X.trans(), delta, v ? Color4c::RED : col);

	delta = X.rot().ycol();
	delta.normalize(5);
	show_vector(X.trans(), delta, v ? Color4c::BLUE : col);

	delta = X.rot().zcol();
	delta.normalize(5);
	show_vector(X.trans(), delta, v ? Color4c::GREEN : col);

#ifndef _FINAL_VERSION_
	show_vector(lastPosition_, 5, Color4c::GREEN);
#endif
}

const Se3f& TraceController::position()
{
	return owner_->modelLogicNodePosition(nodeIndex_);
}
