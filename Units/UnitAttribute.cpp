
#include "stdafx.h"

#include "Sound.h"
#include "UnitAttribute.h"
#include "GlobalAttributes.h"
#include "GameOptions.h"
#include "SoundScript.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\CommonLocText.h"
#include "..\UserInterface\Controls.h"
#include "..\UserInterface\UI_CustomControls.h"
#include "..\Render\inc\IVisGeneric.h"
#include "RenderObjects.h"
#include "BaseUnit.h"
#include "Dictionary.h"
#include "Serialization.h"
#include "RangedWrapper.h"
#include "ResourceSelector.h"
#include "XPrmArchive.h"
#include "TypeLibraryImpl.h"
#include "..\Util\Console.h"
#include "..\Util\MillisecondsWrapper.h"
#include "..\3dx\Lib3dx.h"
#include "SoundTrack.h"
#include "NetPlayer.h"

#include "UnitItemResource.h"
#include "UnitItemInventory.h"
#include "AttributeSquad.h"
#include "IronBullet.h"
#include "TextDB.h"
#include "..\TriggerEditor\TriggerExport.h"
#include "Starforce.h"

template<>
struct PairSerializationTraits<pair<int, AttributeBase::Upgrade> >
{
	static const char* firstName() { return "&Номер апгрейда"; }
	static const char* secondName() { return "&Апгрейд"; }
};

template<>
struct PairSerializationTraits<pair<int, AttributeBase::ProducedUnits> >
{
	static const char* firstName() { return "&Номер производства"; }
	static const char* secondName() { return "&Производство"; }
};

template<>
struct PairSerializationTraits<pair<int, ProducedParameters> >
{
	static const char* firstName() { return "&Номер производства"; }
	static const char* secondName() { return "&Производство"; }
};

template<class Map>
void fixIntMap(Map& map)
{
	bool needSort = false;
	for(Map::iterator i = map.begin(); i != map.end(); ++i)
		for(Map::iterator j = map.begin(); j != i; ++j)
			if(i->first == j->first){
				int max = 0;
				for(Map::iterator k = map.begin(); k != map.end(); ++k)
					if(max < k->first)
						max = k->first;
				i->first = max + 1;
				needSort = true;
			}
	if(needSort)
		map.sort();
}

//////////////////////////////////////////////////////
RandomGenerator effectRND(time(0));

REGISTER_CLASS(AttributeBase, AttributeBase, "Базовые свойства")

WRAP_LIBRARY(AttributeLibrary, "AttributeLibrary", "Юниты", "Scripts\\Content\\AttributeLibrary", 3, true);
WRAP_LIBRARY(AuxAttributeLibrary, "AuxAttributeLibrary", "AuxAttributeLibrary", "Scripts\\Engine\\AuxAttributeLibrary", 0, false);

WRAP_LIBRARY(RigidBodyPrmLibrary, "RigidBodyPrmLibrary", "RigidBodyPrmLibrary", "Scripts\\Engine\\RigidBodyPrmLibrary", 0, false);

WRAP_LIBRARY(RaceTable, "RaceTable", "Расы", "Scripts\\Content\\RaceTable", 0, true);

WRAP_LIBRARY(UnitNameTable, "UnitName", "Названия юнитов", "Scripts\\Content\\UnitName", 0, false);

WRAP_LIBRARY(BodyPartTypeTable, "BodyPartType", "Типы частей тела", "Scripts\\Content\\BodyPartType", 0, false);

WRAP_LIBRARY(DifficultyTable, "DifficultyTable", "Уровни сложности", "Scripts\\Content\\DifficultyTable", 0, false);

REGISTER_CLASS(EffectContainer, EffectContainer, "Эффект");

WRAP_LIBRARY(EffectLibrary, "EffectContainerLibrary", "Эффекты", "Scripts\\Content\\EffectContainerLibrary", 0, true);

WRAP_LIBRARY(PlacementZoneTable, "PlacementZone", "Зоны установки (первая зона должна быть \"Нет зоны\"!)", "Scripts\\Content\\PlacementZone", 1, true);

BEGIN_ENUM_DESCRIPTOR(AttributeType, "AttributeType");
REGISTER_ENUM(ATTRIBUTE_NONE, "ATTRIBUTE_NONE");
REGISTER_ENUM(ATTRIBUTE_LIBRARY, "ATTRIBUTE_LIBRARY");
REGISTER_ENUM(ATTRIBUTE_AUX_LIBRARY, "ATTRIBUTE_AUX_LIBRARY");
REGISTER_ENUM(ATTRIBUTE_SQUAD, "ATTRIBUTE_SQUAD");
REGISTER_ENUM(ATTRIBUTE_PROJECTILE, "ATTRIBUTE_PROJECTILE");
END_ENUM_DESCRIPTOR(AttributeType);

BEGIN_ENUM_DESCRIPTOR(FOWVisibleMode, "FOWVisibleMode");
REGISTER_ENUM(FVM_ALLWAYS,"Всегда видим");
REGISTER_ENUM(FVM_HISTORY_TRACK,"Виден как в последний раз");
REGISTER_ENUM(FVM_NO_FOG,"Виден в видимой зоне");
END_ENUM_DESCRIPTOR(FOWVisibleMode);

BEGIN_ENUM_DESCRIPTOR(AuxAttributeID, "Служебные аттрибуты")
REGISTER_ENUM(AUX_ATTRIBUTE_NONE, "Никто")
REGISTER_ENUM(AUX_ATTRIBUTE_ENVIRONMENT, "Объект окружения")
REGISTER_ENUM(AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE, "Простой объект окружения")
REGISTER_ENUM(AUX_ATTRIBUTE_DETONATOR, "Детонатор")
REGISTER_ENUM(AUX_ATTRIBUTE_ZONE, "Зона")
END_ENUM_DESCRIPTOR(AuxAttributeID)

BEGIN_ENUM_DESCRIPTOR(AttackClass, "Класс юнитов")
REGISTER_ENUM(ATTACK_CLASS_IGNORE, "Никто")
REGISTER_ENUM(ATTACK_CLASS_LIGHT, "Легкий")
REGISTER_ENUM(ATTACK_CLASS_MEDIUM, "Средний")
REGISTER_ENUM(ATTACK_CLASS_HEAVY, "Тяжелый")
REGISTER_ENUM(ATTACK_CLASS_AIR, "Воздушный")
REGISTER_ENUM(ATTACK_CLASS_AIR_MEDIUM, "Воздушный средний")
REGISTER_ENUM(ATTACK_CLASS_AIR_HEAVY, "Воздушный тяжелый")
REGISTER_ENUM(ATTACK_CLASS_UNDERGROUND, "Подземный")
REGISTER_ENUM(ATTACK_CLASS_BUILDING, "Здание")
REGISTER_ENUM(ATTACK_CLASS_MISSILE, "Снаряд")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BUSH, "Декорация куст")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_TREE, "Декорация дерево")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_FENCE, "Декорация забор")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_FENCE2, "Декорация неразрушаемый забор")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BARN, "Декорация сарай")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BUILDING, "Декорация здание")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BRIDGE, "Декорация мост")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_STONE, "Декорация камень")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_INDESTRUCTIBLE, "Декорация неразрушаемое строение")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BIG_BUILDING, "Декорация большое здание")
REGISTER_ENUM(ATTACK_CLASS_TERRAIN_SOFT, "Земля копаемая")
REGISTER_ENUM(ATTACK_CLASS_TERRAIN_HARD, "Земля некопаемая")
REGISTER_ENUM(ATTACK_CLASS_WATER, "Вода")
REGISTER_ENUM(ATTACK_CLASS_WATER_LOW, "Вода относительная")
REGISTER_ENUM(ATTACK_CLASS_ICE, "Лёд")
//REGISTER_ENUM(ATTACK_CLASS_ALL, "Все")
END_ENUM_DESCRIPTOR(AttackClass)


BEGIN_ENUM_DESCRIPTOR(ChainID, "ChainID")
REGISTER_ENUM(CHAIN_STAND, "Стоять")
REGISTER_ENUM(CHAIN_WALK, "Идти")
REGISTER_ENUM(CHAIN_RUN, "Бежать")
REGISTER_ENUM(CHAIN_MOVEMENTS, "Двигаться");
REGISTER_ENUM(CHAIN_BUILDING_STAND, "Стоять для зданий")
REGISTER_ENUM(CHAIN_ATTACK, "Атаковать")
REGISTER_ENUM(CHAIN_FIRE, "Стрелять")
REGISTER_ENUM(CHAIN_FIRE_WALKING, "Стрелять на ходу")
REGISTER_ENUM(CHAIN_FIRE_RUNNING, "Стрелять на бегу")
REGISTER_ENUM(CHAIN_AIM, "Целиться")
REGISTER_ENUM(CHAIN_AIM_WALKING, "Целиться на ходу")
REGISTER_ENUM(CHAIN_AIM_RUNNING, "Целиться на бегу")

REGISTER_ENUM(CHAIN_TURN, "Поворот на месте")
REGISTER_ENUM(CHAIN_TRANSITION, "Переход на другой тип поверхности")
REGISTER_ENUM(CHAIN_GO_WALK, "Переход стоять -> идти")
REGISTER_ENUM(CHAIN_STOP_WALK, "Переход идти -> стоять")
REGISTER_ENUM(CHAIN_GO_RUN, "Переход идти -> бежать")
REGISTER_ENUM(CHAIN_STOP_RUN, "Переход бежать -> идти")

REGISTER_ENUM(CHAIN_BIRTH, "Рождение")
REGISTER_ENUM(CHAIN_BIRTH_IN_AIR, "Рождение в небе");
REGISTER_ENUM(CHAIN_DEATH, "Смерть")
REGISTER_ENUM(CHAIN_FALL, "Падать")
REGISTER_ENUM(CHAIN_RISE, "Вставать")
REGISTER_ENUM(CHAIN_LANDING, "Загужаться в транспорт")
REGISTER_ENUM(CHAIN_IN_TRANSPORT, "Сидеть в транспорте")
REGISTER_ENUM(CHAIN_WORK, "Работать")
REGISTER_ENUM(CHAIN_PICKING, "Собирать ресурс")
REGISTER_ENUM(CHAIN_STAND_WITH_RESOURCE, "Стоять с полным ресурсом")
REGISTER_ENUM(CHAIN_WALK_WITH_RESOURCE, "Идти с полным ресурсом")
REGISTER_ENUM(CHAIN_GIVE_RESOURCE, "Отдавать ресурс (предметы и сборщики)")
REGISTER_ENUM(CHAIN_BUILD, "Строить (для юнитов)")
REGISTER_ENUM(CHAIN_BE_BUILT, "Строиться (для зданий)")
REGISTER_ENUM(CHAIN_UPGRADE, "Апгрейд")
REGISTER_ENUM(CHAIN_CONSTRUCTION, "Строится или Апгрейд");
REGISTER_ENUM(CHAIN_PRODUCTION, "Производство")
REGISTER_ENUM(CHAIN_OPEN, "Открыть")
REGISTER_ENUM(CHAIN_CLOSE, "Закрыть")
REGISTER_ENUM(CHAIN_HOLOGRAM, "Голограмма здания")
REGISTER_ENUM(CHAIN_DISCONNECT, "Здание отключено")
REGISTER_ENUM(CHAIN_UNINSTALL, "Демонтаж (продажа) здания")
REGISTER_ENUM(CHAIN_MOVE, "Шевелиться")
REGISTER_ENUM(CHAIN_TRIGGER, "Цепочка триггера")
REGISTER_ENUM(CHAIN_FLY_DOWN, "Спускаться с высоты")
REGISTER_ENUM(CHAIN_FLY_UP, "Подниматься на высоту")
REGISTER_ENUM(CHAIN_WEAPON_GRIP, "Захваченный")
REGISTER_ENUM(CHAIN_NIGHT, "Ночная цепочка")
REGISTER_ENUM(CHAIN_NONE, "CHAIN_NONE")
REGISTER_ENUM(CHAIN_OPEN_FOR_LANDING, "Посадка в транспорт открыть")
REGISTER_ENUM(CHAIN_CLOSE_FOR_LANDING, "Посадка в транспорт закрыть")
REGISTER_ENUM(CHAIN_LAND_TO_LOAD, "Посадка для погрузки")
REGISTER_ENUM(CHAIN_MOVE_TO_CARGO, "Двигаться к пасажиру")
REGISTER_ENUM(CHAIN_CARGO_LOADED, "Груз погружен")
REGISTER_ENUM(CHAIN_SLOT_IS_EMPTY, "Слот пуст")
REGISTER_ENUM(CHAIN_IS_UPGRADED, "Апгрейд завершен")
REGISTER_ENUM(CHAIN_UPGRADED_FROM_BUILDING, "Апгрейд из здания")
REGISTER_ENUM(CHAIN_UPGRADED_FROM_LEGIONARY, "Апгрейд из юнита")
REGISTER_ENUM(CHAIN_TELEPORTING, "Телепортация")
END_ENUM_DESCRIPTOR(ChainID)

BEGIN_ENUM_DESCRIPTOR(MovementStateID, "MovementStateID")
REGISTER_ENUM(MOVEMENT_STATE_LEFT, "Влево")
REGISTER_ENUM(MOVEMENT_STATE_RIGHT, "Вправо")
REGISTER_ENUM(MOVEMENT_STATE_FORWARD, "Вперед")
REGISTER_ENUM(MOVEMENT_STATE_BACKWARD, "Назад")
REGISTER_ENUM(MOVEMENT_STATE_ON_GROUND, "На земле")
REGISTER_ENUM(MOVEMENT_STATE_ON_LOW_WATER, "На неглубокой воде")
REGISTER_ENUM(MOVEMENT_STATE_ON_WATER, "На воде")
REGISTER_ENUM(MOVEMENT_STATE_ALL_SIDES, "Во все стороны")
REGISTER_ENUM(MOVEMENT_STATE_ALL_SURFACES, "На любой поверхности")
END_ENUM_DESCRIPTOR(MovementStateID)

BEGIN_ENUM_DESCRIPTOR(ExcludeCollision, "ExcludeCollision")
REGISTER_ENUM(EXCLUDE_COLLISION_BULLET, "EXCLUDE_COLLISION_BULLET")
REGISTER_ENUM(EXCLUDE_COLLISION_ENVIRONMENT, "EXCLUDE_COLLISION_ENVIRONMENT")
END_ENUM_DESCRIPTOR(ExcludeCollision)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, ProductionRequirement, "ProductionRequirement")
REGISTER_ENUM_ENCLOSED(AttributeBase, PRODUCE_EVERYWHERE, "Производить везде");
REGISTER_ENUM_ENCLOSED(AttributeBase, PRODUCE_ON_WATER, "Производить на воде");
REGISTER_ENUM_ENCLOSED(AttributeBase, PRODUCE_ON_TERRAIN, "Производить на земле");
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, ProductionRequirement)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, AttackTargetNotificationMode, "AttackTargetNotificationMode")
REGISTER_ENUM_ENCLOSED(AttributeBase, TARGET_NOTIFY_SQUAD, "оповещать свой сквад");
REGISTER_ENUM_ENCLOSED(AttributeBase, TARGET_NOTIFY_ALL, "оповещать всех в заданном радиусе");
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, AttackTargetNotificationMode)

BEGIN_ENUM_DESCRIPTOR(UnitClass, "UnitClass")
REGISTER_ENUM(UNIT_CLASS_NONE, "UNIT_CLASS_NONE")
REGISTER_ENUM(UNIT_CLASS_ITEM_RESOURCE, "UNIT_CLASS_ITEM_RESOURCE")
REGISTER_ENUM(UNIT_CLASS_ITEM_INVENTORY, "UNIT_CLASS_ITEM_INVENTORY")
REGISTER_ENUM(UNIT_CLASS_REAL, "UNIT_CLASS_REAL")
REGISTER_ENUM(UNIT_CLASS_PROJECTILE, "UNIT_CLASS_PROJECTILE")
REGISTER_ENUM(UNIT_CLASS_PROJECTILE_BULLET, "UNIT_CLASS_PROJECTILE_BULLET")
REGISTER_ENUM(UNIT_CLASS_PROJECTILE_MISSILE, "UNIT_CLASS_PROJECTILE_MISSILE")
REGISTER_ENUM(UNIT_CLASS_ENVIRONMENT, "UNIT_CLASS_ENVIRONMENT")
REGISTER_ENUM(UNIT_CLASS_ENVIRONMENT_SIMPLE, "UNIT_CLASS_ENVIRONMENT_SIMPLE")
REGISTER_ENUM(UNIT_CLASS_ZONE, "UNIT_CLASS_ZONE")
REGISTER_ENUM(UNIT_CLASS_EFFECT, "UNIT_CLASS_EFFECT")
REGISTER_ENUM(UNIT_CLASS_SQUAD, "UNIT_CLASS_SQUAD")
REGISTER_ENUM(UNIT_CLASS_BUILDING, "UNIT_CLASS_BUILDING")
REGISTER_ENUM(UNIT_CLASS_LEGIONARY, "UNIT_CLASS_LEGIONARY")
REGISTER_ENUM(UNIT_CLASS_TRAIL, "UNIT_CLASS_TRAIL")
REGISTER_ENUM(UNIT_CLASS_MAX, "UNIT_CLASS_MAX")
END_ENUM_DESCRIPTOR(UnitClass)

BEGIN_ENUM_DESCRIPTOR(CollisionGroupID, "CollisionGroupID")
REGISTER_ENUM(COLLISION_GROUP_ACTIVE_COLLIDER, "COLLISION_GROUP_ACTIVE_COLLIDER");
REGISTER_ENUM(COLLISION_GROUP_COLLIDER, "COLLISION_GROUP_COLLIDER");
REGISTER_ENUM(COLLISION_GROUP_REAL, "COLLISION_GROUP_REAL");
REGISTER_ENUM(COLLISION_GROUP_GROUND_COLLIDER, "COLLISION_GROUP_GROUND_COLLIDER");
END_ENUM_DESCRIPTOR(CollisionGroupID)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(EffectAttribute, WaterPlacementMode, "EffectAttribute::WaterPlacementMode")
REGISTER_ENUM_ENCLOSED(EffectAttribute, WATER_BOTTOM, "ставить на дно")
REGISTER_ENUM_ENCLOSED(EffectAttribute, WATER_SURFACE, "ставить на поверхность воды")
END_ENUM_DESCRIPTOR_ENCLOSED(EffectAttribute, WaterPlacementMode)

BEGIN_ENUM_DESCRIPTOR(ShadowType, "ShadowType")
REGISTER_ENUM(SHADOW_CIRCLE, "круглая тень");
REGISTER_ENUM(SHADOW_REAL, "реальная тень");
REGISTER_ENUM(SHADOW_DISABLED, "нет тень");
END_ENUM_DESCRIPTOR(ShadowType)

BEGIN_ENUM_DESCRIPTOR(SoundSurfKind, "SoundSurfKind")
REGISTER_ENUM(SOUND_SURF_ALL, "Все типы поверхности");
REGISTER_ENUM(SOUND_SURF_KIND1, "Поверхность 1 рода");
REGISTER_ENUM(SOUND_SURF_KIND2, "Поверхность 2 рода");
REGISTER_ENUM(SOUND_SURF_KIND3, "Поверхность 3 рода");
REGISTER_ENUM(SOUND_SURF_KIND4, "Поверхность 4 рода");
END_ENUM_DESCRIPTOR(SoundSurfKind)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(BodyPartAttribute, Functionality, "BodyPartAttribute::Functionality")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, LIFE, "Жизнь")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, MOVEMENT, "Движение")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, PRODUCTION, "Производство")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, UPGRADE, "Апгрейд")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, FIRE, "Стрельба")
END_ENUM_DESCRIPTOR_ENCLOSED(BodyPartAttribute, Functionality)

///////////////////////////////////////////////
float AttributeBase::producedPlacementZoneRadiusMax_ = 0;

cScene* AttributeBase::scene_;
cObject3dx* AttributeBase::model_;
cObject3dx* AttributeBase::logicModel_;
const char* AttributeBase::currentLibraryKey_;

AttributeBase::AttributeBase() : modelName("")
{ 
	unitClass_ = UNIT_CLASS_NONE;

	showSilhouette = false;
	hideByDistance = false;
	boundScale = 1;
	boundRadius = 0;
	boundHeight = 0;
	selectBySphere = true;

    internal = false;

	//parameters.set(1);

	unitAttackClass = ATTACK_CLASS_IGNORE;
//	attackClass = ATTACK_CLASS_IGNORE;
	excludeFromAutoAttack = false;

	hasAutomaticAttackMode = false;
	attackTargetNotificationMode = TARGET_NOTIFY_SQUAD;
	attackTargetNotificationRadius = 1.f;

	basementExtent = Vect2f::ZERO;

	excludeCollision = 0;
	collisionGroup = 0;
	environmentDestruction = 0;

	producedPlacementZoneRadius = 0;

	automaticProduction = false;
	totalProductionNumber = 0;

	enablePathFind = true;
	enableVerticalFactor = false;

	iconDistanceFactor = 1.1f;
	
	mass = 1;
	forwardVelocity = 0;
	forwardVelocityRunFactor = 1;
	sideMoveVelocityFactor = 1.0f;
	backMoveVelocityFactor = 1.0f;

	for(int i = 0; i < 4; i++)
		velocityByTerrainFactors[i] = 1;
	flyingHeight = 0;
	waterLevel = 0;
	targetFlyRadius = 200;

	sightRadiusNightFactor = 1.f;

	radius_ = 0;
	boundBox.min = Vect3f::ZERO;
	boundBox.max = Vect3f::ZERO;
	waterWeight_ = 0;
	waterVelocityFactor = 1.0f;
	resourceVelocityFactor = 1.0f;
	waterFastVelocityFactor = 1.0f;
	manualModeVelocityFactor = 1.0f;
	enablePathTracking = true;
	pathTrackingAngle = 30;
	dieStop = true;
	upgradeStop = true;

	minimapSymbolType_ = UI_MINIMAP_SYMBOLTYPE_DEFAULT;
	minimapScale_ = 1.f;
	hasPermanentSymbol_ = false;
	dockNodeNumber = 0;
	creationTime = 60;
	transparent_mode = false;
	fow_mode = FVM_ALLWAYS;

	selectCircleRelativeRadius = 1.0f; // По умолчанию граф. радиус размером с radius()
	selectRadius = 0;
	selectCircleColor.set(0, 255, 0, 255);
	fireRadiusCircle.color = sColor4c(150, 0, 0, 0);
	fireMinRadiusCircle.color = sColor4c(0, 100, 0, 0);
	signRadiusCircle.color = sColor4c(0, 0, 150, 0);

	enableRotationMode = true;
	angleRotationMode = 100;
	angleSwimRotationMode = 100;

	needBuilders = false;
	inheritHealthArmor = false;
	accountingNumber = 1;
	upgradeAutomatically = false;
	produceParametersAutomatically = false;
	canBeCaptured = false;
	putInIdleList = false;
	isHero = false;
	isStrategicPoint = false;
	accountInCondition = false;

	transportSlotShowEvent = SHOW_AT_HOVER_OR_SELECT;

	transportLoadRadius = 10;
	transportLoadDirectControlRadius = 16.0f;
	checkRequirementForMovement = false;
	transportSlotsRequired = 1;

	rotToTarget = true;
	ptSideMove = false;

	productionRequirement = PRODUCE_EVERYWHERE;
	productionNightFactor = 1;

	producedUnitQueueSize = 5;

	selectionListPriority = 0;

	initialHeightUIParam = 40;
	
	contactWeight = 0;

	chainTransitionTime = 1000;
	chainLandingTime = 1000;
	chainGiveResourceTime = 1000;
	chainRiseTime = 1000;
	//chainGoToRunTime = 1000;
	//chainStopToRunTime = 1000;
	chainOpenTime = 1000;
	chainCloseTime = 1000;
	chainBirthTime = 1000;
	chainOpenForLandingTime = 1000;
	chainCloseForLandingTime = 1000;
	chainFlyDownTime = 1000;
	chainUpgradedFromBuilding = 1000;
	chainUpgradedFromLegionary = 1000;
	chainUninsatalTime = 1000;

	invisible = false;
	canChangeVisibility = false;
	transparenceDiffuseForAlien.color.set(255, 255, 255, 6);
	transparenceDiffuseForClan.color.set(255, 255, 255, 64);

	lodDistance=OBJECT_LOD_DEFAULT;

	useLocalPTStopRadius = false;
	localPTStopRadius = GlobalAttributes::instance().pathTrackingStopRadius;

	defaultDirectControlEnabled = false;
	directControlOffset.set(-100, 0, 10);
	directControlOffsetWater.set(-100, 0, 10);
	directControlThetaMin = 0.0f;
	directControlThetaMax = M_PI;
}

void AttributeBase::serialize(Archive& ar) 
{
	if(ar.isOutput()){
		createModel(modelName.c_str());
		if(!ar.isEdit()){	// Сложные расчеты - только перед записью
			refreshChains(ar.isEdit() && isUnderEditor());
			initGeometryAttribute();
			producedThisFactories.clear();
			AttributeLibrary::Map::const_iterator mi;
			FOR_EACH(AttributeLibrary::instance().map(), mi){
				const AttributeBase* attribute = mi->get();
				if(attribute){
					AttributeBase::ProducedUnitsVector::const_iterator ui;
					FOR_EACH(attribute->producedUnits, ui)
						if(ui->second.unit == this){
							producedThisFactories.push_back(attribute);
							break;
						}
				}
			}
		}
	}
				
	if(!isSquad() && !isProjectile()){
		if(ar.isOutput())
			libraryKey_ = AttributeReference(this).c_str();
		ar.serialize(libraryKey_, "libraryKey", 0);
	}

	setCurrentLibraryKey(libraryKey());

	ar.serialize(unitClass_, "unitClass", 0);
	ar.serialize(ModelSelector(modelName), "modelName", "Имя модели");
	ar.serialize(boundHeight, "boundHeight", "Высота модели (по баунду)");
	if(isBuilding() || isResourceItem() || isInventoryItem())
		ar.serialize(radius_, "radius", "Логический радиус");
	ar.serialize(boundScale, "boundScale", 0);
	ar.serialize(boundRadius, "boundRadius", 0);

	if(!ar.isEdit() || GlobalAttributes::instance().enableSilhouettes)
		ar.serialize(showSilhouette, "showSilhouette", "Выводить силуэт");
	ar.serialize(hideByDistance, "hideByDistance", "Исчезает при удалении");

	ar.serialize(transparent_mode, "mode_transparent", "Становится прозрачным если позади юнит");
	ar.serialize(fow_mode, "fow_mode", "Режим видимости при тумане войны");
	
	ShadowType shadowType = modelShadow.shadowType();
	ar.serialize(shadowType, "shadowType", "Тип тени");
	if(shadowType == SHADOW_CIRCLE){
		float shadowRadiusFactor = modelShadow.shadowRadiusRelative() * boundScale;
		ar.serialize(shadowRadiusFactor, "shadowRadiusFactor", "Радиус тени (коэффициент от радиуса объекта)");
		shadowRadiusFactor *= boundScale;
		modelShadow.set(shadowType, shadowRadiusFactor);
	}else
		modelShadow.setType(shadowType);
	
	ar.serialize(permanentEffects, "permanentEffects", "постоянные эффекты");
	
	ar.serialize(animationChains, "animationChains", 0);
	
	if((isBuilding() || isLegionary())){
		ar.serialize(parametersInitial, "parametersInitial", "Личные (начальные) параметры юнита");
		if(parametersInitial.possession())
			canBeCaptured = true;

		ar.serialize(bodyParts, "bodyParts", "Части тела");
		
		if(!ar.isEdit() && ar.isOutput() && !bodyParts.empty())
			RigidBodyModelPrmBuilder(rigidBodyModelPrm, bodyParts, model());
		
		ar.serialize(rigidBodyModelPrm, "rigidBodyModelPrm", 0);

		ar.openBlock("cost", "Стоимость");
			ar.serialize(dockNodeNumber, "dockNodeNumber", "Номер ноды у завода");
			ar.serialize(creationTime, "creationTime", "Время производства, секунды");
			ar.serialize(creationValue, "creationValue", "Стоимость производства");
			ar.serialize(installValue, "installValue", "Стоимость начального заказа");
			if(isBuilding()){
				ar.serialize(cancelConstructionValue, "cancelConstructionValue", "Возвращаемое от недостроенного здания");
				ar.serialize(uninstallValue, "uninstallValue", "Возвращаемое после деинсталляции");
				ar.serialize(needBuilders, "needBuilders", "Необходимы строители");
			}
			ar.serialize(accessValue, "accessValue", "Необходимые параметры для производства и апгрейда");
			ar.serialize(accessBuildingsList, "accessBuildingsList", "Необходимые строения для производства и апгрейда");
			ar.serialize(inheritHealthArmor, "inheritHealthArmor", "Наследовать здоровье и броню при апгрейде");
		ar.closeBlock();

		ar.serialize(upgrades, "upgrades", "Апгрейды");
		if(ar.isInput()){
			fixIntMap(upgrades);
			Upgrades::iterator iu;
			FOR_EACH(upgrades, iu)
				if(iu->second.chainUpgradeNumber < 0)
					iu->second.chainUpgradeNumber = iu->first;
		}

		if(ar.isInput()){
			upgradeAutomatically = false;
			Upgrades::iterator i;
			FOR_EACH(upgrades, i)
				upgradeAutomatically |= i->second.automatic;
		}
		
		ar.openBlock("transport", "Транспорт");
			ar.serialize(transportSlots, "transportSlots", "Слоты");
			ar.serialize(transportLoadRadius, "transportLoadRadius", "Радиус подбора для летных юнитов");
			ar.serialize(transportLoadDirectControlRadius, "transportLoadDirectControlRadius", "Радиус подбора в прямом управлении");
			if(ar.isInput()){
				checkRequirementForMovement = false;
				TransportSlots::iterator i;
				FOR_EACH(transportSlots, i)
					checkRequirementForMovement |= i->requiredForMovement;
			}
			if(isLegionary()){
				ar.serialize(additionToTransport, "additionToTransport", "Параметры, добавляемые транспорту");
				ar.serialize(transportSlotsRequired, "transportSlotsRequired", "Необходимое количество слотов для размещения в транспорте");
			}
		ar.closeBlock();
	}
	else if(isResourceItem()){
		ar.openBlock("parameters", "Параметры");
		parametersArithmetics.serialize(ar);
		ar.closeBlock();
	}
	
	if(isObjective()){
		ar.openBlock("Interface", "Интерфейс");

		ar.serialize(tipsName, "tipsName", "Имя юнита для интерфейса (Лок)");
		
		ar.serialize(interfaceName_, "interfaceNames", "Краткое описание для интерфейса (Лок)");
		ar.serialize(interfaceDescription_, "interfaceDescriptions", "Полное описание для интерфейса (Лок)");

		ar.openBlock("minimap", "Обозначение на миникарте");
		ar.serialize(minimapScale_, "minimapScale", "относительный масштаб юнита для отметки на миникарте");
		ar.serialize(minimapSymbolType_, "symbolType", "тип пометки");
		if(minimapSymbolType_ == UI_MINIMAP_SYMBOLTYPE_SELF){
			///  CONVERSION 2006-11-7
			if(!ar.serialize(minimapSymbol_, "minimapSymbol", "Собственный символ")){
				minimapSymbol_.serialize(ar);
			}
			/// ^^^^^
		}
		ar.serialize(hasPermanentSymbol_, "hasPermanentSymbol", "Выводить постоянный символ");
		if(hasPermanentSymbol_)
			ar.serialize(minimapPermanentSymbol_, "minimapPermamentSymbol", "Постоянный символ");
		ar.closeBlock();

		ar.serialize(isHero, "isHero", "Герой (для статистики)");
		ar.serialize(isStrategicPoint, "isStrategicPoint", "Стратегическая точка (для статистики)");
		ar.serialize(accountInCondition, "accountInCondition", "Учитывать в условии 'У игрока не осталось дееспособных юнитов'");

		ar.serialize(inventories, "inventories", "Инвентарь");

		ar.serialize(selectBySphere, "selectBySphere", "Селектить по описанной сфере");
		ar.serialize(selectionCursor_, "selection_cursor", "Курсор выбора");
		selectionCursorProxy_ = selectionCursor_;
		ar.serialize(selectionListPriority, "selectionListPriority", "Приоритет в списке селекта");

		ar.serialize(initialHeightUIParam, "initialHeightUIParam", "высота юнита для вывода значений");
		if(isTransport()){
			ar.openBlock("TransportSlots", "Визуализация транспортных слотов");
			ar.serialize(transportSlotShowEvent, "transportSlotShowEvent", "Когда показывать");
			if(transportSlotShowEvent == SHOW_AT_PARAMETER_INCREASE || transportSlotShowEvent == SHOW_AT_PARAMETER_DECREASE)
				transportSlotShowEvent = SHOW_ALWAYS;
			ar.serialize(transportSlotEmpty, "transportSlotEmpty", "Пустой слот");
			ar.serialize(transportSlotFill, "transportSlotFill", "Заполненный слот");
			ar.closeBlock();
		}

		ParameterShowSetting::possibleParameters_ = &parametersInitial;
		ar.serialize(parameterShowSettings, "parameterShowSettings", "Выводимые параметры");
		ar.serialize(showChangeParameterSettings, "showChangeParameterSettings", "Визуализация изменения общих параметров");

		ar.openBlock("selection", "При селекте");
			ar.serialize(selectCircleRelativeRadius, "selectCircleRelativeRadius", "Относительный радиус окружности");
			ar.serialize(selectRadius, "selectRadius", 0);
			ar.serialize(selectCircleColor, "selectCircleColor", "Цвет окружности");
			ar.serialize(fireRadiusCircle, "fireRadiusCircle", "Кружок максимального радиуса атаки");
			ar.serialize(fireMinRadiusCircle, "fireMinRadiusCircle", "Кружок минимального радиуса атаки");
			ar.serialize(signRadiusCircle, "signRadiusCircle", "Кружок радиуса видимости");
		ar.closeBlock();

		ar.serialize(interfaceTV, "interfaceTV", "ИнтерфейсТВ");

		ar.closeBlock();
	}
	
	if(isActing()){
		ar.serialize(weaponAttributes, "weaponAttributes", "Оружие");

		ar.openBlock("attack", "Атака");
		bool no_conversion = ar.serialize(hasAutomaticAttackMode, "hasAutomaticAttackMode", "Собственные настройки режимов атаки");
		if(hasAutomaticAttackMode)
			ar.serialize(attackModeAttribute, "attackModeAttribute", "Настройки режимов атаки");

		ar.serialize(attackTargetNotificationMode, "attackTargetNotificationMode", "Режим оповещения о замеченных врагах");

		if(attackTargetNotificationMode & TARGET_NOTIFY_ALL)
			ar.serialize(attackTargetNotificationRadius, "attackTargetNotificationRadius", "Радиус оповещения о замеченных врагах (относительно радиуса видимости)");

		ar.closeBlock();

		if(!no_conversion){ // conversion 24.08
			ar.serialize(hasAutomaticAttackMode, "hasAutomaticAttackMode", "Собственные настройки режимов атаки");
			if(hasAutomaticAttackMode)
				ar.serialize(attackModeAttribute, "attackModeAttribute", "Настройки режимов атаки");
		}

		ar.openBlock("directControl", "Прямое управление");
			if(GlobalAttributes::instance().directControlMode)
				ar.serialize(defaultDirectControlEnabled, "defaultDirectControlEnabled", "Включать прямое управление по умолчанию");
			ar.serialize(directControlNode, "directControlNode", "Узел для линковки камеры");
			ar.serialize(directControlOffset, "directControlOffset", "Смещение камеры");
			if(!ar.serialize(directControlOffsetWater, "directControlOffsetWater", "Смещение камеры для плавания"))
				directControlOffsetWater = directControlOffset;
			float tmp = directControlThetaMin / M_PI * 180.f;
			ar.serialize(tmp, "directControlThetaMin", "Минимальный угол");
			directControlThetaMin = clamp(tmp / 180.f * M_PI, 0.f, M_PI) ;
			tmp = directControlThetaMax / M_PI * 180.f;
			ar.serialize(tmp, "directControlThetaMax", "Максимальный угол");
			directControlThetaMax = clamp(tmp / 180.f * M_PI, directControlThetaMin, M_PI) ;
		ar.closeBlock();
	}

	if(!isLegionary() && !isProjectile()){ // Антоним папки "движение"
		ar.serialize(rigidBodyPrm, "rigidBodyPrm", 0);
	}

	if(isObjective() || isProjectile()){
		ar.openBlock("Death", "Гибель");
			ar.serialize(waterEffect, "waterEffect", "воздействие от воды");
			ar.serialize(lavaEffect, "lavaEffect", "воздействие от лавы");
			ar.serialize(iceEffect, "iceEffect", "воздействие от заморозки");
			ar.serialize(earthEffect, "earthEffect", "воздействие от земли");
   			harmAttr.serialize(ar);
			ar.serialize(contactWeight, "contactWeight", "Сила воздействия при контакте");

			if(isBuilding() || isLegionary()){
				ar.serialize(unitAttackClass, "unitAttackClass", "класс атаки юнита");
				if(unitAttackClass == ATTACK_CLASS_ENVIRONMENT_BIG_BUILDING)
					unitAttackClass = ATTACK_CLASS_LIGHT;

				ar.serialize(excludeFromAutoAttack, "excludeFromAutoAttack", "Исключить из автоматичекского поиска целей для атаки");
				
				ar.serialize(leavingItems, "leavingItems", "Оставляемые предметы");
				ar.serialize(deathGainArithmetics, "deathGainArithmetics", "Арифметика за гибель");

				if(isLegionary())
					ar.serialize(armorFactors, "armorFactors", "Коэффициенты брони");
			}
		ar.closeBlock();
	}

	if(isActing()){
		ar.openBlock("Production", "Производство");

		ar.serialize(producedUnits, "producedUnits", "Производимые юниты");
		if(ar.isInput())
			fixIntMap(producedUnits);

		ar.serialize(producedUnitQueueSize, "producedUnitQueueSize", "Максимальная длина очереди");
		if(!ar.serialize(dockNodes, "dockNodes", "Имя дока")){ // conversion 15.09
			Logic3dxNode dockNode;
			ar.serialize(dockNode, "dockNode", "Имя дока");
			dockNodes.push_back(dockNode);
		}
				
		if(isBuilding()){
			ar.serialize(automaticProduction, "automaticProduction", "Автоматическое производство юнитов");
			ar.serialize(totalProductionNumber, "totalProductionNumber", "Максимальное количество произведенных юнитов или параметров");
		}

		productivity *= 1.f/logicPeriodSeconds;
		ar.serialize(productivity, "productivity", "Ресурсодобытчик - производительность в секунду");
		productivity *= logicPeriodSeconds;
		ar.serialize(productivityTotal, "productivityTotal", "Ресурсодобытчик - максимальная производительность");
		ar.serialize(productionRequirement, "productionRequirement", "Требования для производительности ресурса");
		ar.serialize(productionNightFactor, "productionNightFactor", "Коэффициент ночью");

		ar.serialize(producedParameters, "producedParameters", "Производимые параметры");
		if(ar.isInput())
			fixIntMap(producedParameters);

		if(ar.isInput()){
			produceParametersAutomatically = false;
			ProducedParametersList::iterator i;
			FOR_EACH(producedParameters, i)
				produceParametersAutomatically |= i->second.automatic;
		}

        		
		ar.serialize(resourceCapacity, "resourceCapacity", "Емкость для ресурса");
		ar.serialize(putInIdleList, "putInIdleList", "Помещать в список бездействующих юнитов");

		ar.closeBlock();
	}

	ar.serialize(excludeCollision, "excludeCollision", 0);
	ar.serialize(collisionGroup, "collisionGroup", 0);

	ar.serialize(internal, "internal", 0);

	ar.serialize(boundBox.min, "boundBoxMin", 0);
	ar.serialize(boundBox.max, "boundBoxMax", 0);

	ar.serialize(producedThisFactories, "producedThisFactories", 0);

	if(isObjective()){
		ar.openBlock("chainTimes", "Времена цепочек анимаций");
			ar.serialize(MillisecondsWrapper(chainTransitionTime), "chainTransitionTime", "Переходы на другую поверхность");
			//ar.serialize(chainGoToRunTime, "chainGoToRunTime", "Переход на бег");
			//ar.serialize(chainStopToRunTime, "chainStopToRunTime", "Переход на шаг");
			if(isLegionary())
				ar.serialize(MillisecondsWrapper(chainLandingTime), "chainLandingTime", "Садится в транспорт");
			if(isResourceItem() || isInventoryItem())
				ar.serialize(MillisecondsWrapper(chainGiveResourceTime), "chainGiveResourceTime", "Отдавать ресурс");
			if(isActing()){
				ar.serialize(MillisecondsWrapper(chainRiseTime), "chainRiseTime", "Вставать");
				ar.serialize(MillisecondsWrapper(chainOpenTime), "chainOpenTime", "Открыть");
				ar.serialize(MillisecondsWrapper(chainCloseTime), "chainCloseTime", "Закрыть");
				ar.serialize(MillisecondsWrapper(chainBirthTime), "chainBirthTime", "Рождение");
				ar.serialize(MillisecondsWrapper(chainOpenForLandingTime), "chainOpenForLandingTime", "Открыть для посадки");
				ar.serialize(MillisecondsWrapper(chainCloseForLandingTime), "chainCloseForLandingTime", "Закрыть для посадки");
				ar.serialize(MillisecondsWrapper(chainFlyDownTime), "chainFlyDownTime", "Спускаться с высоты");

				if(ar.isInput()){
					ar.serialize(MillisecondsWrapper(chainUpgradedFromBuilding), "chainIsUpgraded", "Апгрейд из здания");
					ar.serialize(MillisecondsWrapper(chainUpgradedFromBuilding), "chainUpgradedFromBuilding", "Апгрейд из здания");
				}
				else{
					ar.serialize(MillisecondsWrapper(chainUpgradedFromBuilding), "chainUpgradedFromBuilding", "Апгрейд из здания");
				}
				
				ar.serialize(MillisecondsWrapper(chainUpgradedFromLegionary), "chainUpgradedFromLegionary", "Апгрейд из юнита");
			}
			if(isBuilding())
				ar.serialize(MillisecondsWrapper(chainUninsatalTime), "chainUninsatalTime", "Демонтаж здания");
		ar.closeBlock();
	}

	if(isActing()){
		ar.openBlock("Invisibility", "Невидимость");
		ar.serialize(invisible, "invisible", "Юнит невидим");
		ar.serialize(canChangeVisibility, "canChangeVisibility", "Юнит может менять режим видимости");
		ar.serialize(transparenceDiffuseForAlien, "invisibleColorForAlien", "Видимость для врагов");
		ar.serialize(transparenceDiffuseForClan, "invisibleColorForClan", "Видимость для своих");
		ar.closeBlock();
	}

	ar.serialize(lodDistance,"distanceLod","ЛОД: Дистанция переключения");
}

float AttributeBase::getPTStopRadius() const
{
	if(useLocalPTStopRadius)
		return localPTStopRadius;

	return GlobalAttributes::instance().pathTrackingStopRadius;
}

bool AttributeBase::createModel(const char* model_name)
{
	releaseModel();

	if(!strlen(model_name))
		return false;

	scene_ = gb_VisGeneric->CreateScene();
	if(!scene_)
		return false;

	model_ = scene_->CreateObject3dx(model_name);
	if(!model_)
		return false;

	logicModel_ = scene_->CreateLogic3dx(model_name);
	if(!logicModel_)
		return false;

	return true;
}

void AttributeBase::releaseModel()
{
	if(scene_){
		if(model_){
			model_->Release();
			model_ = 0;
		}

		if(logicModel_){
			logicModel_->Release();
			logicModel_ = 0;
		}

		scene_->Release();
		scene_ = 0;
	}
}

void AttributeBase::setModel(cObject3dx* model, cObject3dx* logicModel)
{
	model_ = model;
	logicModel_ = logicModel;
}

void AttributeBase::preload() const
{
	if(!modelName.empty()){
		const RaceProperty& race = *UnitAttributeID(libraryKey()).race();
		if(race.used() || ::isUnderEditor()){
			if(!race.skins().empty()){
				RaceProperty::Skins::const_iterator i;
				FOR_EACH(race.skins(), i)
					pLibrary3dx->PreloadElement(modelName.c_str(), i->skinColor, i->emblemName.c_str());
			}
			else
				pLibrary3dx->PreloadElement(modelName.c_str(), false);
			pLibrary3dx->PreloadElement(modelName.c_str(), true);
		}
		else{
			pLibrary3dx->Unload(modelName.c_str(), false);
			pLibrary3dx->Unload(modelName.c_str(), true);
		}
	}
}

bool AttributeBase::canProduce(const AttributeBase* attribute) const
{
	AttributeBase::ProducedUnitsVector::const_iterator i;
	FOR_EACH(producedUnits, i)
		if(i->second.unit == attribute)
			return true;
	return false;
}
	

struct AnimationChainLess
{
	bool operator()(const AnimationChain& chain1, const AnimationChain& chain2) const {
		return chain1.chainID != chain2.chainID ? chain1.chainID < chain2.chainID :
		  (chain1.abnormalStateMask != chain2.abnormalStateMask ? chain1.abnormalStateMask < chain2.abnormalStateMask :
		  (chain1.movementState != chain2.movementState ? chain1.movementState < chain2.movementState :
			chain1.counter < chain2.counter));
	}
};

struct AnimationChainLessEdit
{
	bool operator()(const AnimationChain& chain1, const AnimationChain& chain2) const {
		return chain1.priority != chain2.priority ? chain1.priority > chain2.priority :
		  (chain1.chainID != chain2.chainID ? chain1.chainID < chain2.chainID :
		  (chain1.abnormalStatePriority() != chain2.abnormalStatePriority() ? chain1.abnormalStatePriority() > chain2.abnormalStatePriority():
		  (chain1.movementState != chain2.movementState ? chain1.movementState < chain2.movementState :
			chain1.counter < chain2.counter)));
	}
};

void AttributeBase::refreshChains(bool editArchive)
{
	if(editArchive)
		std::sort(animationChains.begin(), animationChains.end(), AnimationChainLessEdit());
	else
		std::sort(animationChains.begin(), animationChains.end(), AnimationChainLess());

	AnimationChains::iterator ci, cj;
	FOR_EACH(animationChains, ci){
		int counter = 0;
		for(cj = animationChains.begin(); cj != ci; ++cj)
			if(ci->chainID == cj->chainID && ci->abnormalStateMask == cj->abnormalStateMask 
			  && ci->movementState == cj->movementState && ci->transitionToState == cj->transitionToState)
				++counter;
		ci->counter = counter;
	}
}

void AttributeBase::initGeometryAttribute()
{
    if(!strlen(modelName.c_str()) || !gb_VisGeneric)
		return;

	cObject3dx* logic = logicModel();
	if(!logic){
        logic = model();
		kdWarning("GAV", XBuffer() < TRANSLATE("Отсутствует логический баунд в моделе ") < modelName.c_str());
	}
	xassert(logic);
	logic->SetPosition(Se3f::ID);

	if(!boundHeight)
		boundHeight = boundRadius > 0 ? boundBox.max.z - boundBox.min.z : 20;

	//logic->SetScale(boundScale);
	//logic->Update();
	//boundRadius = logic->GetBoundRadius();

	logic->SetScale(1);
	logic->Update();
	logic->GetBoundBox(boundBox);
	boundScale = boundHeight/max(boundBox.max.z - boundBox.min.z, FLT_EPS);
	logic->SetScale(boundScale);
	logic->Update();

	boundRadius = logic->GetBoundRadius();
	logic->GetBoundBox(boundBox);

	Vect3f deltaBound = boundBox.max - boundBox.min;
	xassert_s(deltaBound.x > FLT_EPS && deltaBound.y > FLT_EPS && deltaBound.z > FLT_EPS && "Объект слишком маленький или не имеет логического баунда: ", modelName.c_str());
	float radiusMin = 3;
	for(int i = 0; i < 2; i++)
		if(deltaBound[i] < 2*radiusMin){
			boundBox.max[i] = radiusMin;
			boundBox.min[i] = -radiusMin;
		}
	if(deltaBound.z < 2*radiusMin)
		boundBox.max.z = 2*radiusMin - boundBox.min.z;

	selectRadius = selectCircleRelativeRadius*((const Vect2f&)boundBox.max).distance(boundBox.min)/2;
}

void AttributeBase::calcBasementPoints(float angle, const Vect2f& center, Vect2i points[4]) const 
{
	Mat2f m2(angle);
	Vect2f v0 = basementExtent;
	Vect2f v1 = basementExtent;
	v1.x = -v1.x;
	v0 *= m2;
	v1 *= m2;
	points[0] = -v0 + center;
	points[1] = -v1 + center;
	points[2] = v0 + center;
	points[3] = v1 + center;
}

const AnimationChain* AttributeBase::animationChain(ChainID chainID, int counter, const AbnormalStateType* astate, MovementState movementState) const
{
	AnimationChainsInterval interval = findAnimationChainInterval(chainID, astate, movementState);
	if(interval.first == animationChains.end())
		return 0;

	if(counter == -1)
		counter = logicRND(interval.second - interval.first);

	//xassertStr(interval.first + counter < interval.second && "Неправильная сборка анимации или ссылки оружия на анимацию", libraryKey());
	if(counter >= interval.second - interval.first)
		counter = interval.second - interval.first - 1;

	return &*(interval.first + counter);
}

const AnimationChain* AttributeBase::animationChainByFactor(ChainID chainID, float factor, const AbnormalStateType* astate, MovementState movementState) const
{
	AnimationChainsInterval interval = findAnimationChainInterval(chainID, astate, movementState);
	if(interval.first == animationChains.end())
		return 0;

	int numChains = interval.second - interval.first;
	return &*(interval.first + min(round(factor*numChains), numChains - 1));
}

const AnimationChain* AttributeBase::animationChainTransition(float factor, const AbnormalStateType* astate, MovementState stateFrom, MovementState stateTo) const
{
	AnimationChainsInterval interval = findTransitionChainInterval(astate, stateFrom, stateTo);
	if(interval.first == animationChains.end())
	return 0;

	int numChains = interval.second - interval.first;
	return &*(interval.first + min(round(factor*numChains), numChains - 1));
}

AnimationChainsInterval AttributeBase::findAnimationChainInterval(ChainID chainID, const AbnormalStateType* astate, MovementState movementState) const
{
	AnimationChains::const_iterator end = animationChains.end();
	AnimationChains::const_iterator i;
	FOR_EACH(animationChains, i)
		if(i->chainID == chainID){
			AnimationChains::const_iterator iAstateNone = end;
			for(;i != end; ++i){
				if(i->chainID != chainID){
					if(iAstateNone == end)
						return AnimationChainsInterval(end, end);
					else{
						i = end;
						break;
					}
				}
				if((i->movementState & movementState) == movementState){
					if(iAstateNone == end)
						iAstateNone = i;
					if(i->checkAbnormalState(astate))
						break;
				}
			}
			if(i != end){ // с состояниями
				AnimationChains::const_iterator begin = i;
				for(; i != end; ++i)
					if(i->chainID != chainID || !i->checkAbnormalState(astate) || (i->movementState & movementState) != movementState)
						break;
				return AnimationChainsInterval(begin, i);
			}
			else if(iAstateNone != end){ // без состояний
				AnimationChains::const_iterator begin = i = iAstateNone;
				for(; i != end; ++i)
					if(i->chainID != chainID || (i->movementState & movementState) != movementState)
						break;
				return AnimationChainsInterval(begin, i);
			}
			else
				return AnimationChainsInterval(end, end);
		}

	return AnimationChainsInterval(end, end);
}   

AnimationChainsInterval AttributeBase::findTransitionChainInterval(const AbnormalStateType* astate, MovementState stateFrom, MovementState stateTo) const
{
	AnimationChains::const_iterator end = animationChains.end();
	AnimationChains::const_iterator i;
	FOR_EACH(animationChains, i)
		if(i->chainID == CHAIN_TRANSITION){
			AnimationChains::const_iterator iAstateNone = end;
			for(;i != end; ++i){
				if(i->chainID != CHAIN_TRANSITION){
					if(iAstateNone == end)
						return AnimationChainsInterval(end, end);
					else{
						i = end;
						break;
					}
				}
				if((i->movementState & stateFrom) == stateFrom && (i->transitionToState & stateTo) == stateTo){
					if(iAstateNone == end)
						iAstateNone = i;
					if(i->checkAbnormalState(astate))
						break;
				}
			}
			if(i != end){ // с состояниями
				AnimationChains::const_iterator begin = i;
				for(; i != end; ++i)
					if(i->chainID != CHAIN_TRANSITION || !i->checkAbnormalState(astate)
						|| (i->movementState & stateFrom) != stateFrom || (i->transitionToState & stateTo) != stateTo)
						break;
				return AnimationChainsInterval(begin, i);
			}
			else if(iAstateNone != end){ // без состояний
				AnimationChains::const_iterator begin = i = iAstateNone;
				for(; i != end; ++i)
					if(i->chainID != CHAIN_TRANSITION || (i->movementState & stateFrom) != stateFrom || (i->transitionToState & stateTo) != stateTo)
						break;
				return AnimationChainsInterval(begin, i);
			}
			else
				return AnimationChainsInterval(end, end);
		}

	return AnimationChainsInterval(end, end);
}

int AttributeBase::animationChainIndex(const AnimationChain* chain) const	
{ 
	AnimationChains::const_iterator it;
	FOR_EACH(animationChains, it)
		if(&(*it) == chain) 
			return it - animationChains.begin();
	return animationChains.size();
}

const AnimationChain* AttributeBase::animationChainByIndex(int index) const 
{
	if(index < animationChains.size())
		return &*(animationChains.begin() + index);
	return 0;
}

AttributeType AttributeBase::attributeType() const
{
	if(isEnvironment())
		return ATTRIBUTE_AUX_LIBRARY;

	if(isSquad())
		return ATTRIBUTE_SQUAD;

	if(isProjectile())
		return ATTRIBUTE_PROJECTILE;

	AttributeReference attribute = this;
	if(attribute)
		return ATTRIBUTE_LIBRARY;

	AuxAttributeReference auxAttribute = this;
	if(auxAttribute)
		return ATTRIBUTE_AUX_LIBRARY;

	xassert(0);
	return ATTRIBUTE_NONE;
}

void AttributeBase::TraceInfo::serialize(Archive& ar)
{
	ar.serialize(surfaceKind_, "surfaceKind", "Тип поверхности");
	ar.serialize(traceTerTool_, "traceTerTool", "След");
}

//--------------------------------------------
const char* AttributeBase::libraryKey() const
{
	return libraryKey_.c_str();
}

float AttributeBase::radius() const
{
	return radius_ ? radius_ : ((const Vect2f&)boundBox.max).distance(boundBox.min)/2;
}

RandomGenerator logicRnd;

AnimationChain::AnimationChain() 
{
	chainID = CHAIN_NONE;
	counter = 0;
	period = 2000; 
	animationAcceleration = 0;
	abnormalStateMask = 0;
	cycled = false;
	reversed = false;
	syncBySound = false;
	randomPhase = false;
	stopPermanentEffects = false;
	priority = 0;
	supportedByLogic = false;
	movementState = MOVEMENT_STATE_DEFAULT;
	transitionToState = MOVEMENT_STATE_DEFAULT;
}

void AnimationChain::serialize(Archive& ar) 
{
	if(ar.isOutput())
		setDefaultPriority();

	ar.serialize(chainID, "chainID", "&Идентификатор цепочки");
	if(chainID == CHAIN_IS_UPGRADED)
		chainID = CHAIN_UPGRADED_FROM_BUILDING;
	ar.serialize(movementState, "movementState", "&Движение");
	if(chainID == CHAIN_TRANSITION)
		ar.serialize(transitionToState, "transitionToState", "Переход в состояние");
	ar.serialize(counter, "counter", "&Номер");

	ar.serialize(chainIndex_, "ChainIndex", "&Имя цепочки в модели");
	ar.serialize(animationGroup_, "AnimationGroup", "&Имя анимационной группы");
	ar.serialize(visibilityGroup_, "VisibilityGroup", "&Имя группы видимости");

	if(ar.isInput() && !ar.isEdit()){
		string name;
		if(ar.serialize(name, "chainName", 0))
			chainIndex_.set(name.c_str());
		if(ar.serialize(name, "animationGroupName", 0))
			animationGroup_.set(name.c_str());
		if(ar.serialize(name, "visibilityGroupName", 0))
			visibilityGroup_.set(name.c_str());
	}

	ar.serialize(animationAcceleration, "animationAcceleration", "Ускорение анимации, %");
	ar.serialize(cycled, "cycled", "Зацикленная");
	ar.serialize(priority, "priority", "Приоритет (0 - самый низкий)");
	ar.serialize(supportedByLogic, "supportedByLogic", 0);
	ar.serialize(reversed, "reversed", "Проигрывать в обратную сторону");
	ar.serialize(syncBySound, "syncBySound", "Синхронизировать по звуку");
	ar.serialize(randomPhase, "randomPhase", "Устанавливать случайную фазу");
	ar.serialize(effects, "effects", "Спецэффекты");
	ar.serialize(stopPermanentEffects, "stopPermanentEffects", "Выключать постоянные эффекты");
	
	ar.serialize(soundReferences, "soundReferences", "Звуки");
	ar.serialize(soundMarkers, "soundMarkers", "Звуковые метки");

	ar.serialize(abnormalStateTypes, "abnormalStateTypes", "&Типы воздействия");

	if(ar.isOutput()){
		cObject3dx* model = AttributeBase::model();
		if(model)
			period = max(round(model->GetChain(chainIndex())->time*1000/max(1.f + animationAcceleration/100.f, 0.001f)), 100);
	}

	ar.serialize(period, "period", 0);
	float periodSeconds = period/1000.f;
	ar.serialize(periodSeconds, "periodSeconds", "Период анимации (для просмотра), секунды");

	if(ar.isInput()){
		abnormalStateMask = 0;
		AbnormalStateTypeReferences::const_iterator i;
		FOR_EACH(abnormalStateTypes, i)
			if(*i)
				abnormalStateMask |= (*i)->mask();
	}
}

void AnimationChain::setDefaultPriority()
{
	priority -= abnormalStatePriority();

	switch(chainID){
	case CHAIN_BUILD:
	case CHAIN_PICKING:	
	case CHAIN_STAND_WITH_RESOURCE:
	case CHAIN_WALK_WITH_RESOURCE:
	case CHAIN_GIVE_RESOURCE:
		priority = 20;
		break;

	case CHAIN_FIRE:
	case CHAIN_FIRE_WALKING:
	case CHAIN_FIRE_RUNNING:
	case CHAIN_AIM:
	case CHAIN_AIM_WALKING:
	case CHAIN_AIM_RUNNING:
		priority = 40;
		break;

	case CHAIN_LANDING:
	case CHAIN_IN_TRANSPORT:
	case CHAIN_OPEN_FOR_LANDING:
	case CHAIN_CLOSE_FOR_LANDING:
	case CHAIN_FLY_DOWN:
	case CHAIN_FLY_UP:
		priority = 60;
		break;

	case CHAIN_RISE:
	case CHAIN_TRANSITION:
		priority = 80;
		break;

	case CHAIN_PRODUCTION:
	case CHAIN_UPGRADE:
	case CHAIN_FALL:
	case CHAIN_WEAPON_GRIP:
	case CHAIN_HOLOGRAM:
	case CHAIN_BE_BUILT:
	case CHAIN_UPGRADED_FROM_BUILDING:
	case CHAIN_UPGRADED_FROM_LEGIONARY:
		priority = 80;
		supportedByLogic = true;
		break;

	case CHAIN_BIRTH:
	case CHAIN_DEATH:
	case CHAIN_OPEN:
	case CHAIN_CLOSE:
	case CHAIN_TRIGGER:
		priority = 100;
		break;
	}

	priority += abnormalStatePriority();
}

int AnimationChain::abnormalStatePriority() const
{
	int priority = 0;
	AbnormalStateTypeReferences::const_iterator i;
	FOR_EACH(abnormalStateTypes, i)
		if(*i && priority < (*i)->priority)
			priority = (*i)->priority;
	return priority;
}

bool AnimationChain::checkAbnormalState(const AbnormalStateType* astate) const
{
	if(!astate)
		return !abnormalStateMask;
	else 
		return abnormalStateMask & astate->mask();
}

bool AnimationChain::compare(const AnimationChain& rhs) const
{
	return chainID == rhs.chainID && movementState == rhs.movementState && abnormalStateMask == rhs.abnormalStateMask && transitionToState == rhs.transitionToState;
}

const char* AnimationChain::name() const
{
	static XBuffer buffer(1000, 1);
	buffer.init();
	buffer < getEnumNameAlt(chainID);

	if(!abnormalStateTypes.empty()){
		buffer < " {";
		AbnormalStateTypeReferences::const_iterator i;
		FOR_EACH(abnormalStateTypes, i)
			if(*i)
				buffer < (*i)->c_str() < ", ";
		buffer -= 2;
		buffer < "}";
	}

	buffer < " [" < getEnumDescriptor(MOVEMENT_STATE_RIGHT).nameAltCombination(movementState).c_str() < "]";

	if(chainID == CHAIN_TRANSITION)
		buffer < " -> [" < getEnumDescriptor(MOVEMENT_STATE_RIGHT).nameAltCombination(transitionToState).c_str() < "]";
	
	if(counter)
		buffer < " " <= counter;

	std::replace(buffer.buffer(), buffer.buffer() + buffer.size(), '|', '+');

	return buffer;
}

/////////////////////////////////////
void UnitNumber::serialize(Archive& ar)
{
	ar.serialize(type, "type", "&Тип юнита");
	ar.serialize(numberParameters, "numberParameters", "&Максимальное количество");
}


void UnitColor::serialize(Archive& ar)
{
	ar.serialize(color, "color", "Цвет");
	ar.serialize(isBrightColor, "isBrightColor", "Раскрасить ярко");
}

void UnitColorEffective::apply(cObject3dx* model, float phase)
{
	sColor4f clr(color);
	model->SetOpacity(1.f + (clr.a - 1.f) * phase);
	sColor4f reset(1.f, 1.f, 1.f, 0.f);
	if(isBrightColor){
		clr.a = phase * fill_ / 255.f;
		model->SetTextureLerpColor(clr);
		model->SetColorMaterial(&reset, &reset, 0);
	}
	else {
		model->SetTextureLerpColor(reset);
		model->SetColorMaterial(
			&sColor4f(clr.r, clr.g, clr.b, 0.2f * phase),
			&sColor4f(clr.r, clr.g, clr.b, 0.8f * phase),
			0);
	}
}


void UnitColorEffective::setColor(const UnitColor& clr, bool reset)
{	// цвета бывают яркие и не яркие
	// яркие цвета всегда действуют по одиночке (последний) и они более приоритетны, чем не яркие
	// альфа яркого цвета - это степень заливки текстуры модели
	// не яркие цвета складываются, альфа не яркого цвета это всегда прозрачность модели, независимо от типа других цветов
	// прозрачность берется наименьшая из действующих

	if(reset){
		color = clr.color;
		if(clr.isBrightColor){
			color.a = 255;
			isBrightColor = true;
			fill_ = clr.color.a;
		}
		else {
			isBrightColor = false;
			fill_ = 255;
		}
	}
	else {
		if(clr.isBrightColor){
			color.set(clr.color.r, clr.color.g, clr.color.b, color.a);
			fill_ = clr.color.a;
			isBrightColor = true;
		}
		else if(!isBrightColor){
			color.set(
				clamp(color.r + clr.color.r, 0, 255),
				clamp(color.g + clr.color.g, 0, 255),
				clamp(color.b + clr.color.b, 0, 255),
				min(color.a, clr.color.a));
		}
		else
			color.a = min(color.a, clr.color.a);
	}
}

void UnitColorEffective::setOpacity(float op, bool reset)
{
	if(reset) {
		color = UnitColor::ZERO.color;
		color.a = round(op * 255);
		fill_ = 255;
	}
	else {
		color.a = min(color.a, round(op * 255));
	}
}

//////////////////////////////////////////////////

const UI_MarkObjectAttribute& RaceProperty::orderMark(UI_ClickModeMarkID mark_id) const
{
	xassert(mark_id >= 0 && mark_id < UI_CLICK_MARK_SIZE);

	return orderMarks_[mark_id];
}

const UI_MinimapSymbol& RaceProperty::minimapMark(UI_MinimapSymbolID mark_id) const{
	xassert(mark_id >= 0 && mark_id < UI_MINIMAP_SYMBOL_MAX);

	return minimapMakrs_[mark_id];
}

//////////////////////////////////////////////////

AttackMode::AttackMode()
{
	autoAttackMode_ = ATTACK_MODE_DISABLE;
	autoTargetFilter_ = AUTO_ATTACK_ALL;
	walkAttackMode_ = WALK_NOT_ATTACK;
	weaponMode_ = ANY_RANGE;
}

void AttackMode::serialize(Archive& ar)
{
	ar.serialize(autoAttackMode_, "autoAttackMode", "Режим атаки");
	ar.serialize(autoTargetFilter_, "autoTargetFilter", "Режим автоматического выбора целей");
//	ar.serialize(walkAttackMode_, "walkAttackMode", "Режим атаки при движении");
	ar.serialize(weaponMode_, "weaponMode", "Режим оружия");
}

AttackModeAttribute::AttackModeAttribute()
{
	patrolRadius_ = 100.f;
	patrolStopTime_ = 1.f;
	patrolMovementMode_ = PATROL_MOVE_RANDOM;
	targetInsideSightRadius_ = false;
}

void AttackModeAttribute::serialize(Archive& ar)
{
	ar.serialize(attackMode_, "attackMode", "Начальные установки режимов атаки");
/*
	ar.serialize(patrolRadius_, "patrolRadius", "Радиус патрулирования");
	ar.serialize(patrolStopTime_, "patrolStopTime", "Время остановки при патрулировании, сек");
	ar.serialize(patrolMovementMode_, "patrolMovementMode", "Режим движения при патрулировании");
*/
	ar.serialize(targetInsideSightRadius_, "targetInsideSightRadius", "Терять цель при выходе из радиуса видимости");
}

//////////////////////////////////////////////////

RaceProperty::RaceProperty(const char* name) :
	StringTableBase(name),
	weaponUpgradeEffect_(false),
	workForAIEffect_(false)
{
	instrumentary_ = false;
	
	selectQuantityMax = 9;

	orderMarks_.resize(UI_CLICK_MARK_SIZE);
	minimapMakrs_.resize(UI_MINIMAP_SYMBOL_MAX);

	produceMultyAmount = 5;

	used_ = false;
	usedAlways_ = true;
}

void RaceProperty::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar); 
	ar.serialize(locName_, "locName", "Имя расы");
	ar.serialize(fileNameAddition_, "fileNameAddition", "Добавка к именам файлов");
	ar.serialize(instrumentary_, "instrumentary", "Служебная");
	ar.serialize(usedAlways_, "usedAlways", "Загружать всегда");

	ar.serialize(shipmentPositionMark_, "shipmentPositionMark", "Флажок точки сбора производимых юнитов");

	ar.openBlock("orderMarks", "Визуализация отдачи приказов");
	int i;
	for(i = 0; i < UI_CLICK_MARK_SIZE; i++)
		ar.serialize(orderMarks_[i], getEnumName(UI_ClickModeMarkID(i)), getEnumNameAlt(UI_ClickModeMarkID(i)));
	ar.closeBlock();

//	ar.serialize(unitAttackEffect_, "unitAttackEffect", "Визуализация атаки по юниту");
	ar.serialize(weaponUpgradeEffect_, "weaponUpgradeEffect", "Визуализация апгрейда оружия");

	ar.openBlock("minimapMarks", "Визуализация событий и юнитов на миникарте");
	for(i = 0; i < UI_MINIMAP_SYMBOL_MAX; i++)
		ar.serialize(minimapMakrs_[i], getEnumName(UI_MinimapSymbolID(i)), getEnumNameAlt(UI_MinimapSymbolID(i)));
	ar.closeBlock();
	ar.openBlock("controlAI", "Под управлением AI");
	ar.serialize(workForAISprite_, "workForAISprite", "Пометка над юнитом при AI");
	ar.serialize(workForAIEffect_, "workForAIEffect", "Эффект над юнитом при AI");
	ar.closeBlock();
	ar.serialize(runModeSprite_, "runModeSprite", "Пометка над юнитом при режиме бега");

	ar.serialize(initialResource, "initialResource", "Первоначальный ресурс");
	ar.serialize(resourceCapacity, "resourceCapacity", "Первоначальная емкость");
	ar.serialize(initialUnits, "initialUnits", "Первоначальный набор юнитов");
	
	ar.serialize(commonTriggers, "commonTriggers", 0);
	ar.serialize(scenarioTriggers, "scenarioTriggers", "Триггера для сингла");
	ar.serialize(battleTriggers, "battleTriggers", "Триггера для баттла");
	ar.serialize(multiplayerTriggers, "multiplayerTriggers", "Триггера для мультиплеера");
	if(ar.isInput() && !ar.isEdit()){
		if(scenarioTriggers.empty())
			scenarioTriggers = commonTriggers;
	}

	ar.serialize(selectQuantityMax, "selectQuantityMax", "Максимальное количество юнитов в селекте");
	ar.serialize(unitNumbers, "unitNumbers", "Максимальное количество юнитов по типам");
	
	ar.serialize(startConstructionSound, "startConstructionSound", "Звук на начало строительства здания");
	ar.serialize(unableToConstructSound, "unableToConstructSound", "Звук, когда здания нельзя установить");

	ar.serialize(attackModeAttribute_, "attackModeAttribute", "Настройки режимов атаки");

	ar.serialize(produceMultyAmount, "produceMultyAmount", "Количество заказываемых юнитов с шифтом");

	ar.serialize(circle, "circle", "Настройки визуализации селекта");
	ar.serialize(circleTeam, "circleTeam", "Настройки визуализации селекта напарников в командном режиме");

	selection_param.serialize(ar);

	ar.serialize(screenToPreload, "screenToPreload", "Экран для предзагрузки");
}

void RaceProperty::setUsed(sColor4c skinColor, const char* emblemName) const 
{
	used_ = true;
	skins_.push_back(Skin());
	skins_.back().skinColor = skinColor;
	skins_.back().emblemName = emblemName;
}

void RaceProperty::setUnused() const
{
	used_ = false;
	skins_.clear();
}

void RaceProperty::setCommonTriggers(TriggerChainNames& triggers, GameType gameType) const
{
	const TriggerChainNames& commonTriggers = gameType & GameType_Multiplayer ? multiplayerTriggers : 
		(gameType & GAME_TYPE_BATTLE ? battleTriggers : scenarioTriggers);
	triggers.sub(commonTriggers);
	triggers.insert(triggers.end(), commonTriggers.begin(), commonTriggers.end());
}

UnitAttributeID::UnitAttributeID() 
: unitName_("None") 
{}

UnitAttributeID::UnitAttributeID(const UnitName& unitName, const Race& race) 
: race_(race), 
unitName_(unitName)
{}

UnitAttributeID::UnitAttributeID(const char* str)
{
	string aName = str;
	if(!strcmp(str, "None")){
		*this = UnitAttributeID();
		return;
	}
	int pos = aName.find(",");
	if(pos == string::npos)
		return;
	string bName(&aName[pos + 2], aName.size() - pos - 2);
	aName.erase(pos, aName.size());
	if(aName.empty()){
		*this = UnitAttributeID();
		return;
	}
	*this = UnitAttributeID(UnitName(aName.c_str()), Race(bName.c_str()));
	if(!unitName_.key())
		*this = UnitAttributeID();
}

const char* UnitAttributeID::c_str() const 
{
	static string nameLogic, nameGraphics;
	string& name = MT_IS_LOGIC() ? nameLogic : nameGraphics;
	name = strcmp(unitName().c_str(), "None") ? string(unitName().c_str()) + ", " + race().c_str() : "None";
	return name.c_str();
}

void UnitAttributeID::serialize(Archive& ar) 
{
	ar.serialize(unitName_, "name", "&имя");
	ar.serialize(race_, "race", "&раса");
}

///////////////////////////////////////////////
void InterfaceTV::serialize(Archive& ar) 
{
	ar.serialize(radius_, "radius", "размер модели");
	ar.serialize(position_, "position", "смещение");
	ar.serialize(orientation_, "orientation", "поворот");
	ar.serialize(chain_, "chain", "анимационная цепочка");
}

//////////////////////////////////////////
void WeaponDamage::serialize(Archive& ar) 
{
	ar.serialize(static_cast<ParameterCustom&>(*this), "MainDamage", "Основные повреждения");
}

ArmorFactors::ArmorFactors() 
: front(1), back(1), left(1), right(1), top(1), used_(false)
{}

void ArmorFactors::serialize(Archive& ar)
{
	ar.serialize(front, "front", "Перед");
	ar.serialize(back, "back", "Зад");
	ar.serialize(left, "left", "Слева");
	ar.serialize(right, "right", "Справа");
	ar.serialize(top, "top", "Сверху");
	if(ar.isInput())
		used_ = fabs(front - 1.f) > FLT_EPS || fabs(back - 1.f) > FLT_EPS || fabs(left - 1.f) > FLT_EPS
			|| fabs(right - 1.f) > FLT_EPS || fabs(top - 1.f) < FLT_EPS;
}

float ArmorFactors::factor(const Vect3f& localPos) const
{
	float atz = atan2(localPos.z, sqrt(sqr(localPos.x)+sqr(localPos.y)));
	if(atz > M_PI/3.0f)
		return top;
	float at = atan2(localPos.y, localPos.x);
	if(fabs(at) < M_PI/4.0f)
		return right;
	if(fabs(at) > 3.0f*M_PI/4.0f)
		return left;
	if(localPos.y > 0)
		return front;
	if(localPos.y < 0)
		return back;
	return 1.0f;
}

////////////////////////////////////////
DifficultyPrm::DifficultyPrm(const char* name) 
: StringTableBase(name)
{
	aiDelay = 0; 
	triggerDelayFactor = 1;
	orderBuildingsDelay = 0;
	orderUnitsDelay = 0;
	orderParametersDelay = 0;
	upgradeUnitDelay = 0;
	simpleScanDelay = 0;
	eventScanDelay = 0;
	//attackBySpecialWeaponDelay = 0;
}

void DifficultyPrm::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar); 
	ar.serialize(locName, "locName", "Локализованное имя");
	ar.serialize(aiDelay, "aiDelay", "Задержка АИ");
	ar.serialize(triggerDelayFactor, "triggerDelayFactor", "Коэффициент триггера задержка");
	ar.serialize(orderBuildingsDelay, "orderBuildingsDelay", "Задержка строительства зданий");
	ar.serialize(orderUnitsDelay, "orderUnitsDelay", "Задержка производства юнитов");
	ar.serialize(orderParametersDelay, "orderParametersDelay", "Задержка производства параметров");
	ar.serialize(upgradeUnitDelay, "upgradeUnitDelay", "Задержка апгрейда юнита в здание");
	ar.serialize(simpleScanDelay, "simpleScanDelay", "Задержка для обычных контектсных условий");
	ar.serialize(eventScanDelay, "eventScanDelay", "Задержка для событийных контектсных условий");
	//ar.serialize(attackBySpecialWeaponDelay, "attackBySpecialWeaponDelay", "Задержка атаки специальным оружием поверхности");
}

// -----------------------------------------
std::string EffectContainer::texturesPath_;

EffectContainer::EffectContainer()
{
}

EffectContainer::~EffectContainer()
{
}

void EffectContainer::serialize(Archive& ar)
{
	ar.serialize(ModelSelector(fileName_, ModelSelector::EFFECT_OPTIONS), "fileName_", "имя файла");
}

EffectKey* EffectContainer::getEffect(float scale, sColor4c skin_color) const
{
	return gb_EffectLibrary->Get(fileName_.c_str(), scale, texturesPath_.empty() ? NULL : texturesPath_.c_str(), skin_color);
}

void EffectContainer::preloadLibrary()
{
	gb_EffectLibrary->preloadLibrary(fileName_.c_str(),texturesPath_.c_str());
}


EffectAttribute::EffectAttribute()
{
	isCycled_ = true;
	stopImmediately_ = false;

	bindOrientation_ = true;
	switchOffByInterface_ = legionColor_ = false;

	waterPlacementMode_ = WATER_BOTTOM;

	ignoreFogOfWar_ = false;
	ignoreInvisibility_ = false;

	switchOffUnderWater_ = switchOffUnderLava_ = switchOffByDay_ = switchOffOnIce_ = switchOnIce_ = false;

	scale_ = 1.0f;
}

EffectAttribute::EffectAttribute (const EffectReference& effectReference, bool isCycled)
{
	effectReference_ = effectReference;

	isCycled_ = isCycled;
	stopImmediately_ = false;

	bindOrientation_ = true;
	switchOffByInterface_ = legionColor_ = false;

	waterPlacementMode_ = WATER_BOTTOM;

	ignoreFogOfWar_ = false;
	ignoreInvisibility_ = false;

	switchOffUnderWater_ = switchOffUnderLava_ = switchOffByDay_ = switchOffOnIce_ = switchOnIce_ = false;

	scale_ = 1.0f;
}

void EffectAttribute::serialize(Archive& ar)
{
	ar.serialize(isCycled_, "isCycled", "зацикливать");
	ar.serialize(stopImmediately_, "stopImmediately", "Обрывать при окончании");
	ar.serialize(bindOrientation_, "bindOrientation", "ориентировать по объекту");
	ar.serialize(legionColor_, "legionColor", "окрашивать в цвет легиона");
	ar.serialize(switchOffByInterface_, "switchOffByInterface", "Гасить при отключении интерфейса");
	ar.serialize(switchOffUnderWater_, "switchOffUnderWater", "выключать в воде");
	ar.serialize(switchOffUnderLava_, "switchOffUnderLava", "выключать в лаве");
	ar.serialize(switchOffByDay_, "switchOffByDay", "включать ночью");
	ar.serialize(switchOffOnIce_, "switchOffOnIce", "выключать на льду");
	ar.serialize(switchOnIce_, "switchOnIce", "включать только на льду");
	ar.serialize(ignoreFogOfWar_, "ignoreFogOfWar", "виден в тумане войны");
	ar.serialize(ignoreInvisibility_, "ignoreInvisibility", "виден на невидимом юните");

	ar.serialize(waterPlacementMode_, "waterPlacementMode", "режим вывода на воде");

	ar.serialize(scale_, "scale", "масштаб");

	ar.serialize(effectReference_, "effectReference", "&эффект");
}

EffectAttributeAttachable::EffectAttributeAttachable(bool need_node_name) : EffectAttribute()
{
	scaleByModel_ = true;
	needNodeName_ = need_node_name;
	synchronizationWithModelAnimation_ = false;
	switchOffByAnimationChain_ = false;
}

EffectAttributeAttachable::EffectAttributeAttachable(const EffectReference& effectReference, bool isCycled) : EffectAttribute(effectReference, isCycled)
{
	scaleByModel_ = false;
	needNodeName_ = true;
	synchronizationWithModelAnimation_ = false;
	switchOffByAnimationChain_ = false;
}

void EffectAttributeAttachable::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(scaleByModel_, "scaleByModel", "масштабировать по размеру объекта");

	if (!isCycled_)
		ar.serialize(synchronizationWithModelAnimation_,"synchronizationWithModelAnimation","синхронизировать с анимацией");
	ar.serialize(switchOffByAnimationChain_, "switchOffByAnimationChain", "выключается анимацией как постоянный");
	if(needNodeName_)
		ar.serialize(node_, "node", "Узел привязки");
}

// -------------------------------------------
void AttributeBase::ProducedUnits::serialize(Archive& ar)
{
	xassertStr(!(ar.isOutput() && !ar.isEdit() && !unit) && "Пустой производимый юнит у ", currentLibraryKey());
	ar.serialize(unit, "unit", "&Юнит");
	ar.serialize(number, "number", "&Количество");
}

ProducedParameters::ProducedParameters() 
: time(60),
  automatic(false)
{
}

void ProducedParameters::serialize(Archive& ar)
{
	arithmetics.serialize(ar);
	ar.serialize(time, "time", "Время, секунды");
	ar.serialize(cost, "cost", "Стоимость");
	ar.serialize(accessValue, "accessValue", "Необходимые параметры для производства");
	ar.serialize(accessBuildingsList, "accessBuildingsList", "Необходимые строения для производства");
	ar.serialize(sprites_, "sprites", "Спрайты для очереди");
	ar.serialize(signalVariable, "signalVariable", "Имя сигнальной переменной (нужно задавать для различения уникальных параметров)");
	ar.serialize(automatic, "automatic", "Производить автоматически");
}

// -------------------------------------------

HarmAttribute::HarmAttribute()
{
}

const AbnormalStateEffect* HarmAttribute::abnormalStateEffect(const AbnormalStateType* type) const
{
	AbnormalStateEffects::const_iterator it = std::find(abnormalStateEffects.begin(), abnormalStateEffects.end(), type);
	if(it != abnormalStateEffects.end())
		return &*it;

	return 0;
}

void HarmAttribute::serializeAbnormalStateEffects(Archive& ar)
{
	ar.serialize(abnormalStateEffects, "abnormalStateEffects", "эффекты от воздействий");
#ifndef _FINAL_VERSION_
	AbnormalStateEffects::const_iterator it;
	FOR_EACH(abnormalStateEffects, it)
		if(it->effectAttribute().switchOffByInterface()){
			XBuffer buf;
			buf < "Юнит: " < AttributeBase::currentLibraryKey()
				< "\nЭффект: " < it->effectAttribute().effectReference().c_str()
				< "\nиз воздействия: \"" < it->typeRef().c_str() < "\""
				< "\nвыключается вместе с интерфейсом";
			xxassert(false, buf.c_str());
			kdError("Effects", buf.c_str());
		}
#endif
}

void HarmAttribute::serializeSources(Archive& ar)
{
	ar.serialize(deathAttribute_.sources, "sources", "Источники, остающиеся после гибели");
}

void HarmAttribute::serialize(Archive& ar)
{
	serializeAbnormalStateEffects(ar);

	deathAttribute_.serialize(ar);
}

const DeathAttribute& HarmAttribute::deathAttribute(const AbnormalStateType* type) const
{
	if(type){
		if(const AbnormalStateEffect* eff = abnormalStateEffect(type)){
			if(eff->hasDeathAttribute())
				return eff->deathAttribute();
		}
	}

	return deathAttribute_;
}

void HarmAttribute::serializeEnvironment(Archive& ar)
{
	serializeAbnormalStateEffects(ar);
	deathAttribute_.serialize(ar);
}

AttributeBase::Upgrade::Upgrade() 
: automatic(false), 
built(false), 
level(-1),
chainUpgradeNumber(0),
chainUpgradeTime(2000) 
{}

void AttributeBase::Upgrade::serialize(Archive& ar)
{
	xassertStr(!(ar.isOutput() && !ar.isEdit() && !upgrade) && "Пустой апгрейд у ", currentLibraryKey());
	ar.serialize(upgrade, "upgrade", "&Апгрейд");
	ar.serialize(automatic, "automatic", "&Автоматический");
	ar.serialize(upgradeValue, "upgradeValue", "Стоимость апгрейда");
	ar.serialize(accessParameters, "accessParameters", "Необходимые личные параметры");
	if(!ar.serialize(chainUpgradeNumber, "chainUpgradeNumber", "Номер цепочки анимации"))
		chainUpgradeNumber = -1;
	ar.serialize(MillisecondsWrapper(chainUpgradeTime), "chainUpgradeTime", "Время апгрейда");
	if(!ar.isEdit() || upgrade && upgrade->isBuilding())
		ar.serialize(built, "built", "Апгрейдить в достроенное здание");
	if(!ar.isEdit() || upgrade && upgrade->isLegionary())
		ar.serialize(level, "level", "Уровень юнита");
}

const ShowChangeSettings* AttributeBase::getShowChangeSettings(int idx) const
{
	ShowChangeParameterSettings::const_iterator sh_it;
	FOR_EACH(showChangeParameterSettings, sh_it)
		if(sh_it->typeIdx() == idx)
			return &(sh_it->changeSettings());
	return 0;
}

AttributeBase::RigidBodyModelPrmBuilder::RigidBodyModelPrmBuilder(RigidBodyModelPrm& modelPrm, BodyPartAttributes& bodyParts, cObject3dx* model) :
	modelPrm_(modelPrm),
	model_(model)
{
	bool baseNodeInited(false);
	modelPrm.resize(1);
	BodyPartAttributes::iterator bodyPart;
	int i = 0;
	FOR_EACH(bodyParts, bodyPart){
		RigidBodyModelPrm::iterator nodePrm;
		FOR_EACH(bodyPart->rigidBodyBodyPartPrm, nodePrm){
			nodePrm->bodyPartID = i;
			if(nodePrm->graphicNode == 0){
				*modelPrm.begin() = *nodePrm;
				baseNodeInited = true;
            }
			else
				modelPrm.push_back(*nodePrm);
		}
		++i;
	}
	if(!baseNodeInited){
		xxassert(bodyParts.empty() || modelPrm.size() == 1, "не задан базовый узел");
		modelPrm.clear();
		return;
	}
	for(RigidBodyModelPrm::iterator it = modelPrm.begin() + 1; it < modelPrm.end(); ++it){
		it->parent = findParentBody(it->graphicNode) - modelPrm.begin();
	}
}

RigidBodyModelPrm::const_iterator AttributeBase::RigidBodyModelPrmBuilder::findParentBody(int index) const
{
	index = model_->GetParentNumber(index);
	RigidBodyModelPrm::const_iterator it;
	FOR_EACH(modelPrm_, it){
		if(it->graphicNode == index)
			return it;
	}
	return findParentBody(index);
}

////////////////////////////////////////

PlacementZoneData::PlacementZoneData(const char* name) 
: StringTableBase(name)
{
	showRadius = 500;
}

void PlacementZoneData::serialize(Archive& ar)
{
	StringTableBase::serialize(ar);
	ar.serialize(circleColor, "circleColor", "Цвет кругов подключения");
	ar.serialize(showRadius, "showRadius", "Радиус, на котором показывать зоны подключения");
}


void loadAllLibraries(bool preload)
{
	initConditions();
	initActions();
	initActionsEnvironmental();
	initActionsSound();

	UnitNameTable::instance().add("None");

    DebugPrm::instance();
    SoundAttributeLibrary::instance();
    SoundTrackTable::instance();
    BodyPartTypeTable::instance();
    AbnormalStateTypeTable::instance();
    ParameterTypeTable::instance();
    ParameterGroupTable::instance();
    ParameterValueTable::instance();
    AttributeLibrary::instance();
    AuxAttributeLibrary::instance();
    AttributeSquadTable::instance();
    WeaponPrm::updateIdentifiers();
    WeaponGroupTypeTable::instance();
    WeaponPrmLibrary::instance();
    RaceTable::instance();
	DifficultyTable::instance();
    UnitNameTable::instance();
    AttributeProjectileTable::instance();
    ParameterFormulaTable::instance();
    SourcesLibrary::instance();	
    TerToolsLibrary::instance();	
    TextDB::instance();
    TextIdMap::instance();	

    // редактируесть через ComboBox:
	FormationPatterns::instance();
	UnitFormationTypes::instance();
	PlacementZoneTable::instance();

    // Для общей кучи перезагружаем все
	GlobalAttributes::instance();
	EffectLibrary::instance();

	UI_SpriteLibrary::instance();
    UI_TextureLibrary::instance();
	UI_ShowModeSpriteTable::instance();
	UI_MessageTypeLibrary::instance();
    UI_FontLibrary::instance();
    UI_CursorLibrary::instance();
	CommonLocText::instance();
	if(!isUnderEditor())
		VoiceAttribute::VoiceFile::loadVoiceFileDuration();
	UI_GlobalAttributes::instance();
	UI_Dispatcher::instance();
	UI_TextLibrary::instance();

	const int version = 2;
	if(GlobalAttributes::instance().version != version || check_command_line("update")){
		GlobalAttributes::instance().version = version;
		saveAllLibraries();
	}

	if(preload){
		AttributeLibrary::Map::const_iterator ai;
		FOR_EACH(AttributeLibrary::instance().map(), ai){
			if(ai->get())
				ai->get()->preload();
		}
	}

	if(check_command_line("save_all")){
		saveAllLibraries();
		ErrH.Exit();
	}
}

void saveAllLibraries()
{
	minimap().clearEvents();

	AttributeLibrary::Map& units = const_cast<AttributeLibrary::Map&>(AttributeLibrary::instance().map());
	AttributeLibrary::Map::iterator mi;
	FOR_EACH(units, mi)
		if(!mi->get()){
			mi = units.erase(mi);
			--mi;
		}

	WeaponPrmLibrary::Strings& weapons = const_cast<WeaponPrmLibrary::Strings&>(WeaponPrmLibrary::instance().strings());
	WeaponPrmLibrary::Strings::iterator wmi;
	FOR_EACH(weapons, wmi)
		if(!wmi->get()){
			wmi = weapons.erase(wmi);
			--wmi;
		}

    SoundAttributeLibrary::instance().saveLibrary();
    SoundTrackTable::instance().saveLibrary();
    BodyPartTypeTable::instance().saveLibrary();
    AbnormalStateTypeTable::instance().saveLibrary();
    ParameterTypeTable::instance().saveLibrary();
    ParameterGroupTable::instance().saveLibrary();
    ParameterValueTable::instance().saveLibrary();
    AttributeLibrary::instance().saveLibrary();
    AuxAttributeLibrary::instance().saveLibrary();
    AttributeSquadTable::instance().saveLibrary();
	WeaponGroupTypeTable::instance().saveLibrary();
    WeaponPrm::updateIdentifiers();
    WeaponPrmLibrary::instance().saveLibrary();
    RaceTable::instance().saveLibrary();
	DifficultyTable::instance().saveLibrary();
    UnitNameTable::instance().saveLibrary();
    AttributeProjectileTable::instance().saveLibrary();
    ParameterFormulaTable::instance().saveLibrary();
    SourcesLibrary::instance().saveLibrary();	
    TerToolsLibrary::instance().saveLibrary();	
    TextDB::instance().saveLibrary();
    TextIdMap::instance().saveLibrary();	

    // редактируесть через ComboBox:
	FormationPatterns::instance().saveLibrary();
	UnitFormationTypes::instance().saveLibrary();
	PlacementZoneTable::instance().saveLibrary();

    // Для общей кучи перезаписываем все
	GlobalAttributes::instance().saveLibrary();
	GameOptions::instance().saveLibrary();
	GameOptions::instance().savePresets();
	EffectLibrary::instance().saveLibrary();

	UI_SpriteLibrary::instance().saveLibrary();
    UI_TextureLibrary::instance().saveLibrary();
	UI_ShowModeSpriteTable::instance().saveLibrary();
	UI_MessageTypeLibrary::instance().saveLibrary();
    UI_FontLibrary::instance().saveLibrary();
    UI_CursorLibrary::instance().saveLibrary();
    UI_Dispatcher::instance().saveLibrary();
	UI_GlobalAttributes::instance().saveLibrary();
	UI_TextLibrary::instance().saveLibrary();
	CommonLocText::instance().saveLibrary();
	ControlManager::instance().saveLibrary();

	AttributeBase::releaseModel();

	//VoiceAttribute::VoiceFile::saveVoiceFileDurations();
}

/////////////////////////////////////////////////////////////

AuxAttribute::AuxAttribute(AuxAttributeID auxAttributeID) 
: auxAttributeID_(auxAttributeID), 
BaseClass(getEnumName(auxAttributeID))
{}

AuxAttribute::AuxAttribute(const char* name) 
: auxAttributeID_(strlen(name) ? (AuxAttributeID)getEnumDescriptor(AUX_ATTRIBUTE_NONE).keyByName(name) : AUX_ATTRIBUTE_NONE), 
BaseClass(name)
{
}

void AuxAttribute::serialize(Archive& ar)
{
	StringTableBase::serialize(ar);
	if(!ar.isEdit() && ar.isInput()){ // CONVERSION 27.04.06
		if(ar.serialize(auxAttributeID_, "first", 0))
			setName(getEnumName(auxAttributeID_));
	}
	ar.serialize(auxAttributeID_, "auxAttributeID", "auxAttributeID");
	ar.serialize(type_, "second", "Значение");
}

AuxAttributeReference::AuxAttributeReference(const AttributeBase* attribute)
{
	AuxAttributeLibrary::Strings::const_iterator i;
	FOR_EACH(AuxAttributeLibrary::instance().strings(), i)
		if(i->get() == attribute)
			setKey(i->stringIndex());
}

AuxAttributeReference::AuxAttributeReference(AuxAttributeID auxAttributeID)
: BaseClass(auxAttributeID != AUX_ATTRIBUTE_NONE ? getEnumName(auxAttributeID) : "")
{}

bool AuxAttributeReference::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isInput()){
		AuxAttributeID attribute;
		bool nodeExists = ar.serialize(attribute, name, nameAlt);
		static_cast<BaseClass&>(*this) = BaseClass(getEnumName(attribute));
		return nodeExists;
	}
	else{
		AuxAttributeID attribute = (AuxAttributeID)getEnumDescriptor(AUX_ATTRIBUTE_NONE).keyByName(c_str());
		return ar.serialize(attribute, name, nameAlt);
	}
}

/////////////////////////////////////////////////////////////

UnitAttribute::UnitAttribute(UnitAttributeID unitAttributeID, AttributeBase* attribute) 
: unitAttributeID_(unitAttributeID), 
BaseClass(unitAttributeID.c_str(), attribute)
{}

UnitAttribute::UnitAttribute(const char* name) 
: unitAttributeID_(name), 
BaseClass(name)
{
}

void UnitAttribute::serialize(Archive& ar)
{
	if(ar.isEdit()){
		StringTableBase::serialize(ar);
		unitAttributeID_ = UnitAttributeID(c_str());
	}
	else { // CONVERSION 27.04.06
		ar.serialize(unitAttributeID_, "first", 0);
		setName(unitAttributeID_.c_str());
	}
	ar.serialize(type_, "second", "Значение");
}

void UnitAttribute::setKey(const UnitAttributeID& key)
{
	unitAttributeID_ = key;
	StringTableBase::setName(key.c_str());
}

void UnitAttribute::setName(const char* name)
{
	setKey(UnitAttributeID(name));
}

const char*	UnitAttribute::editorName() const
{
	return key().unitName().c_str();
}

bool UnitAttribute::editorAllowRename()
{
	return false;
}

void UnitAttribute::editorCreate(const char* name, const char* groupName)
{
	bool exists = UnitNameTable::instance().exists(name);
	const char* p = groupName;
	while(*p && *p != '\\')
		++p;
	if(p != groupName + strlen(groupName)){
		std::string race(groupName, p);
		if(!RaceTable::instance().exists(race.c_str())){
			xassertStr(0 && "Такой расы не существует", race.c_str());
			return;
		}

		if(!exists)
			UnitNameTable::instance().add(name);
		UnitAttributeID attributeID = UnitAttributeID(UnitName(name), Race(race.c_str()));
		++p;
		editorSetGroup(p);
		setKey(attributeID);
	}
}

std::string UnitAttribute::editorGroupName() const
{
	return type_ ? std::string(unitAttributeID_.race().c_str()) + "\\" + ClassCreatorFactory<AttributeBase>::instance().nameAlt(typeid(*type_).name()) : "";
}

const char* UnitAttribute::editorGroupsComboList()
{
	static std::string comboList;
	comboList = "";
	RaceTable::Strings::const_iterator it;
	const RaceTable::Strings& strings = RaceTable::instance().strings();
	FOR_EACH(strings, it){

		const RaceProperty& race = *it;
		std::string raceName = race.c_str();
		if(!raceName.empty()){
			const ComboStrings& names = ClassCreatorFactory<AttributeBase>::instance().comboStringsAlt();
			ComboStrings::const_iterator cit;
			FOR_EACH(names, cit){
				if(!comboList.empty())
					comboList += "|";
				comboList += raceName;
				comboList += "\\";
				comboList += cit->c_str();
			}
		}
	}
	return comboList.c_str();
}

AttributeReference::AttributeReference(const AttributeBase* attribute)
{
	AttributeLibrary::Strings::const_iterator i;
	FOR_EACH(AttributeLibrary::instance().strings(), i)
		if(i->get() == attribute)
			setKey(i->stringIndex());
	unitAttributeID_ = UnitAttributeID(c_str());
}

AttributeReference::AttributeReference(const char* attributeName)
: BaseClass(attributeName), 
unitAttributeID_(attributeName)
{}

const AttributeBase* AttributeReference::get() const 
{ 
	const StringTableBasePolymorphic<AttributeBase>* data = getInternal();
	if(data)
		return data->get();
	(BaseClass&)(*this) =  BaseClass(unitAttributeID_.c_str());
	data = getInternal();
	return data ? data->get() : 0;
}

bool AttributeReference::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit()){
		get(); // CONVERSION
		bool nodeExists = BaseClass::serialize(ar, name, nameAlt);
		if(ar.isInput())
			unitAttributeID_ = UnitAttributeID(c_str());
	}
	else{ // CONVERSION 27.04.06
		if(ar.openStruct(name, nameAlt, typeid(unitAttributeID_).name())){
			unitAttributeID_.serialize(ar);
			//static_cast<BaseClass&>(*this) = BaseClass(unitAttributeID_.c_str());
			get(); // CONVERSION
			ar.closeStruct(name);
			return true;
		}
		ar.closeStruct(name);
	}
	return false;
}

bool AttributeUnitReference::validForComboList(const AttributeBase& attr) const
{
	return &attr ? attr.isLegionary() : false;
}

bool AttributeBuildingReference::validForComboList(const AttributeBase& attr) const
{
	return &attr ? attr.isBuilding() : false;
}

bool AttributeUnitOrBuildingReference::validForComboList(const AttributeBase& attr) const
{
	return &attr ? attr.isLegionary() || attr.isBuilding() : false;
}

bool AttributeItemReference::validForComboList(const AttributeBase& attr) const
{
	return &attr ? attr.isResourceItem() || attr.isInventoryItem() : false;
}

bool AttributeItemResourceReference::validForComboList(const AttributeBase& attr) const
{
	return &attr ? attr.isResourceItem() : false;
}

bool AttributeItemInventoryReference::validForComboList(const AttributeBase& attr) const
{
	return &attr ? attr.isInventoryItem() : false;
}

///////////////////////////////////////////////////////////////////////
BodyPartAttribute::BodyPartAttribute()
{
	percent = 20;
}

void BodyPartAttribute::serialize(Archive& ar)
{
	ar.serialize(bodyPartType, "bodyPartType", "&Тип части тела");
	ar.serialize(functionality, "functionality", "Функционал");
	if(functionality & FIRE)
		ar.serialize(weapons, "weapons", "Влияет на оружие");
	ar.serialize(percent, "percent", "Процент от общего здоровья");
	ar.serialize(visibilitySet_, "visibilitySetName", "&Часть модели");
	VisibilityGroupOfSet::setVisibilitySet(visibilitySet_);
	ar.serialize(defaultGarment, "defaultGarment", "&Одежда по умолчанию");
	ar.serialize(possibleGarments, "possibleGarments", "Возможные одежды");
	ar.serialize(automaticGarments, "automaticGarments", "Автоматические одежды");
	ar.serialize(rigidBodyBodyPartPrm, "rigidBodyBodyPartPrm", "Физическая модель");
}

void BodyPartAttribute::Garment::serialize(Archive& ar)
{
	ar.serialize(visibilityGroup, "visibilityGroup", "Группа видимости");
	ar.serialize(item, "item", "Предмет");
}

void BodyPartAttribute::AutomaticGarment::serialize(Archive& ar)
{
	Garment::serialize(ar);
	ar.serialize(parameters, "parameters", "Требуемые параметры");
}

RigidBodyNodePrm::RigidBodyNodePrm() :
	parent(-1),
	bodyPartID(-1),
	upperLimits(0.0f, 0.0f, M_PI / 4.0f),
	lowerLimits(0.0f, 0.0f, M_PI / 4.0f),
	mass(1.0f)
{
}

void RigidBodyNodePrm::serialize(Archive& ar)
{
	ar.serialize(logicNode, "logicNode", "Логический узел");
	ar.serialize(graphicNode, "graphicNode", "Графический узел");
	upperLimits.set(R2G(upperLimits.x), R2G(upperLimits.y), R2G(upperLimits.z));
	lowerLimits.set(R2G(lowerLimits.x), R2G(lowerLimits.y), R2G(lowerLimits.z));
	ar.serialize(upperLimits, "upperLimits", "Верхний предел джоинта");
	ar.serialize(lowerLimits, "lowerLimits", "Нижний предел джоинта");
	upperLimits.set(G2R(upperLimits.x), G2R(upperLimits.y), G2R(upperLimits.z));
	lowerLimits.set(G2R(lowerLimits.x), G2R(lowerLimits.y), G2R(lowerLimits.z));
	ar.serialize(mass, "mass", "Масса");
	ar.serialize(parent, "parent", 0);
	ar.serialize(bodyPartID, "bodyPartID", 0);
}
void saveInterfaceLibraries()
{
	TextDB::instance().saveLibrary();
	TextIdMap::instance().saveLibrary();	
	UI_TextureLibrary::instance().saveLibrary();
	UI_ShowModeSpriteTable::instance().saveLibrary();
	UI_MessageTypeLibrary::instance().saveLibrary();
	UI_FontLibrary::instance().saveLibrary();
	UI_Dispatcher::instance().saveLibrary();
	UI_GlobalAttributes::instance().saveLibrary();
	CommonLocText::instance().saveLibrary();
}

ModelShadow::ModelShadow()
{
	shadowType_ = SHADOW_REAL;
	shadowRadiusRelative_ = 0.5f;
}

void ModelShadow::set(ShadowType shadowType, float shadowRadiusRelative)
{
	shadowType_ = shadowType;
	shadowRadiusRelative_ = shadowRadiusRelative;
}

void ModelShadow::setShadowType(c3dx* model, float radius) const
{
	model->SetCircleShadowParam(shadowRadiusRelative_ * radius, -1);
	switch (shadowType_){
		case SHADOW_CIRCLE:
			model->SetShadowType(c3dx::OST_SHADOW_CIRCLE);
			break;
		case SHADOW_REAL:
			model->SetShadowType(c3dx::OST_SHADOW_REAL);
			break;
		case SHADOW_DISABLED:
			model->SetShadowType(c3dx::OST_SHADOW_NONE);
			break;
	};
}

void ModelShadow::serialize(Archive& ar)
{
	ar.serialize(shadowType_, "shadowType", "Тип тени");
	if(shadowType_ == SHADOW_CIRCLE)
		ar.serialize(shadowRadiusRelative_, "shadowRadiusRelative", "Радиус тени (коэффициент от радиуса объекта)");
}

void ModelShadow::serializeForEditor(Archive& ar, float factor)
{
	ar.serialize(shadowType_, "shadowType", "Тип тени");
	if(shadowType_ == SHADOW_CIRCLE){
		float shadowRadiusRelative = shadowRadiusRelative_ * factor;
		ar.serialize(shadowRadiusRelative, "shadowRadiusRelative", "Радиус тени (коэффициент от радиуса объекта)");
		shadowRadiusRelative_ = shadowRadiusRelative / factor;
	}
}

