
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
	static const char* firstName() { return "&����� ��������"; }
	static const char* secondName() { return "&�������"; }
};

template<>
struct PairSerializationTraits<pair<int, AttributeBase::ProducedUnits> >
{
	static const char* firstName() { return "&����� ������������"; }
	static const char* secondName() { return "&������������"; }
};

template<>
struct PairSerializationTraits<pair<int, ProducedParameters> >
{
	static const char* firstName() { return "&����� ������������"; }
	static const char* secondName() { return "&������������"; }
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

REGISTER_CLASS(AttributeBase, AttributeBase, "������� ��������")

WRAP_LIBRARY(AttributeLibrary, "AttributeLibrary", "�����", "Scripts\\Content\\AttributeLibrary", 3, true);
WRAP_LIBRARY(AuxAttributeLibrary, "AuxAttributeLibrary", "AuxAttributeLibrary", "Scripts\\Engine\\AuxAttributeLibrary", 0, false);

WRAP_LIBRARY(RigidBodyPrmLibrary, "RigidBodyPrmLibrary", "RigidBodyPrmLibrary", "Scripts\\Engine\\RigidBodyPrmLibrary", 0, false);

WRAP_LIBRARY(RaceTable, "RaceTable", "����", "Scripts\\Content\\RaceTable", 0, true);

WRAP_LIBRARY(UnitNameTable, "UnitName", "�������� ������", "Scripts\\Content\\UnitName", 0, false);

WRAP_LIBRARY(BodyPartTypeTable, "BodyPartType", "���� ������ ����", "Scripts\\Content\\BodyPartType", 0, false);

WRAP_LIBRARY(DifficultyTable, "DifficultyTable", "������ ���������", "Scripts\\Content\\DifficultyTable", 0, false);

REGISTER_CLASS(EffectContainer, EffectContainer, "������");

WRAP_LIBRARY(EffectLibrary, "EffectContainerLibrary", "�������", "Scripts\\Content\\EffectContainerLibrary", 0, true);

WRAP_LIBRARY(PlacementZoneTable, "PlacementZone", "���� ��������� (������ ���� ������ ���� \"��� ����\"!)", "Scripts\\Content\\PlacementZone", 1, true);

BEGIN_ENUM_DESCRIPTOR(AttributeType, "AttributeType");
REGISTER_ENUM(ATTRIBUTE_NONE, "ATTRIBUTE_NONE");
REGISTER_ENUM(ATTRIBUTE_LIBRARY, "ATTRIBUTE_LIBRARY");
REGISTER_ENUM(ATTRIBUTE_AUX_LIBRARY, "ATTRIBUTE_AUX_LIBRARY");
REGISTER_ENUM(ATTRIBUTE_SQUAD, "ATTRIBUTE_SQUAD");
REGISTER_ENUM(ATTRIBUTE_PROJECTILE, "ATTRIBUTE_PROJECTILE");
END_ENUM_DESCRIPTOR(AttributeType);

BEGIN_ENUM_DESCRIPTOR(FOWVisibleMode, "FOWVisibleMode");
REGISTER_ENUM(FVM_ALLWAYS,"������ �����");
REGISTER_ENUM(FVM_HISTORY_TRACK,"����� ��� � ��������� ���");
REGISTER_ENUM(FVM_NO_FOG,"����� � ������� ����");
END_ENUM_DESCRIPTOR(FOWVisibleMode);

BEGIN_ENUM_DESCRIPTOR(AuxAttributeID, "��������� ���������")
REGISTER_ENUM(AUX_ATTRIBUTE_NONE, "�����")
REGISTER_ENUM(AUX_ATTRIBUTE_ENVIRONMENT, "������ ���������")
REGISTER_ENUM(AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE, "������� ������ ���������")
REGISTER_ENUM(AUX_ATTRIBUTE_DETONATOR, "���������")
REGISTER_ENUM(AUX_ATTRIBUTE_ZONE, "����")
END_ENUM_DESCRIPTOR(AuxAttributeID)

BEGIN_ENUM_DESCRIPTOR(AttackClass, "����� ������")
REGISTER_ENUM(ATTACK_CLASS_IGNORE, "�����")
REGISTER_ENUM(ATTACK_CLASS_LIGHT, "������")
REGISTER_ENUM(ATTACK_CLASS_MEDIUM, "�������")
REGISTER_ENUM(ATTACK_CLASS_HEAVY, "�������")
REGISTER_ENUM(ATTACK_CLASS_AIR, "���������")
REGISTER_ENUM(ATTACK_CLASS_AIR_MEDIUM, "��������� �������")
REGISTER_ENUM(ATTACK_CLASS_AIR_HEAVY, "��������� �������")
REGISTER_ENUM(ATTACK_CLASS_UNDERGROUND, "���������")
REGISTER_ENUM(ATTACK_CLASS_BUILDING, "������")
REGISTER_ENUM(ATTACK_CLASS_MISSILE, "������")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BUSH, "��������� ����")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_TREE, "��������� ������")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_FENCE, "��������� �����")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_FENCE2, "��������� ������������� �����")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BARN, "��������� �����")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BUILDING, "��������� ������")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BRIDGE, "��������� ����")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_STONE, "��������� ������")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_INDESTRUCTIBLE, "��������� ������������� ��������")
REGISTER_ENUM(ATTACK_CLASS_ENVIRONMENT_BIG_BUILDING, "��������� ������� ������")
REGISTER_ENUM(ATTACK_CLASS_TERRAIN_SOFT, "����� ��������")
REGISTER_ENUM(ATTACK_CLASS_TERRAIN_HARD, "����� ����������")
REGISTER_ENUM(ATTACK_CLASS_WATER, "����")
REGISTER_ENUM(ATTACK_CLASS_WATER_LOW, "���� �������������")
REGISTER_ENUM(ATTACK_CLASS_ICE, "˸�")
//REGISTER_ENUM(ATTACK_CLASS_ALL, "���")
END_ENUM_DESCRIPTOR(AttackClass)


BEGIN_ENUM_DESCRIPTOR(ChainID, "ChainID")
REGISTER_ENUM(CHAIN_STAND, "������")
REGISTER_ENUM(CHAIN_WALK, "����")
REGISTER_ENUM(CHAIN_RUN, "������")
REGISTER_ENUM(CHAIN_MOVEMENTS, "���������");
REGISTER_ENUM(CHAIN_BUILDING_STAND, "������ ��� ������")
REGISTER_ENUM(CHAIN_ATTACK, "���������")
REGISTER_ENUM(CHAIN_FIRE, "��������")
REGISTER_ENUM(CHAIN_FIRE_WALKING, "�������� �� ����")
REGISTER_ENUM(CHAIN_FIRE_RUNNING, "�������� �� ����")
REGISTER_ENUM(CHAIN_AIM, "��������")
REGISTER_ENUM(CHAIN_AIM_WALKING, "�������� �� ����")
REGISTER_ENUM(CHAIN_AIM_RUNNING, "�������� �� ����")

REGISTER_ENUM(CHAIN_TURN, "������� �� �����")
REGISTER_ENUM(CHAIN_TRANSITION, "������� �� ������ ��� �����������")
REGISTER_ENUM(CHAIN_GO_WALK, "������� ������ -> ����")
REGISTER_ENUM(CHAIN_STOP_WALK, "������� ���� -> ������")
REGISTER_ENUM(CHAIN_GO_RUN, "������� ���� -> ������")
REGISTER_ENUM(CHAIN_STOP_RUN, "������� ������ -> ����")

REGISTER_ENUM(CHAIN_BIRTH, "��������")
REGISTER_ENUM(CHAIN_BIRTH_IN_AIR, "�������� � ����");
REGISTER_ENUM(CHAIN_DEATH, "������")
REGISTER_ENUM(CHAIN_FALL, "������")
REGISTER_ENUM(CHAIN_RISE, "��������")
REGISTER_ENUM(CHAIN_LANDING, "���������� � ���������")
REGISTER_ENUM(CHAIN_IN_TRANSPORT, "������ � ����������")
REGISTER_ENUM(CHAIN_WORK, "��������")
REGISTER_ENUM(CHAIN_PICKING, "�������� ������")
REGISTER_ENUM(CHAIN_STAND_WITH_RESOURCE, "������ � ������ ��������")
REGISTER_ENUM(CHAIN_WALK_WITH_RESOURCE, "���� � ������ ��������")
REGISTER_ENUM(CHAIN_GIVE_RESOURCE, "�������� ������ (�������� � ��������)")
REGISTER_ENUM(CHAIN_BUILD, "������� (��� ������)")
REGISTER_ENUM(CHAIN_BE_BUILT, "��������� (��� ������)")
REGISTER_ENUM(CHAIN_UPGRADE, "�������")
REGISTER_ENUM(CHAIN_CONSTRUCTION, "�������� ��� �������");
REGISTER_ENUM(CHAIN_PRODUCTION, "������������")
REGISTER_ENUM(CHAIN_OPEN, "�������")
REGISTER_ENUM(CHAIN_CLOSE, "�������")
REGISTER_ENUM(CHAIN_HOLOGRAM, "���������� ������")
REGISTER_ENUM(CHAIN_DISCONNECT, "������ ���������")
REGISTER_ENUM(CHAIN_UNINSTALL, "�������� (�������) ������")
REGISTER_ENUM(CHAIN_MOVE, "����������")
REGISTER_ENUM(CHAIN_TRIGGER, "������� ��������")
REGISTER_ENUM(CHAIN_FLY_DOWN, "���������� � ������")
REGISTER_ENUM(CHAIN_FLY_UP, "����������� �� ������")
REGISTER_ENUM(CHAIN_WEAPON_GRIP, "�����������")
REGISTER_ENUM(CHAIN_NIGHT, "������ �������")
REGISTER_ENUM(CHAIN_NONE, "CHAIN_NONE")
REGISTER_ENUM(CHAIN_OPEN_FOR_LANDING, "������� � ��������� �������")
REGISTER_ENUM(CHAIN_CLOSE_FOR_LANDING, "������� � ��������� �������")
REGISTER_ENUM(CHAIN_LAND_TO_LOAD, "������� ��� ��������")
REGISTER_ENUM(CHAIN_MOVE_TO_CARGO, "��������� � ��������")
REGISTER_ENUM(CHAIN_CARGO_LOADED, "���� ��������")
REGISTER_ENUM(CHAIN_SLOT_IS_EMPTY, "���� ����")
REGISTER_ENUM(CHAIN_IS_UPGRADED, "������� ��������")
REGISTER_ENUM(CHAIN_UPGRADED_FROM_BUILDING, "������� �� ������")
REGISTER_ENUM(CHAIN_UPGRADED_FROM_LEGIONARY, "������� �� �����")
REGISTER_ENUM(CHAIN_TELEPORTING, "������������")
END_ENUM_DESCRIPTOR(ChainID)

BEGIN_ENUM_DESCRIPTOR(MovementStateID, "MovementStateID")
REGISTER_ENUM(MOVEMENT_STATE_LEFT, "�����")
REGISTER_ENUM(MOVEMENT_STATE_RIGHT, "������")
REGISTER_ENUM(MOVEMENT_STATE_FORWARD, "������")
REGISTER_ENUM(MOVEMENT_STATE_BACKWARD, "�����")
REGISTER_ENUM(MOVEMENT_STATE_ON_GROUND, "�� �����")
REGISTER_ENUM(MOVEMENT_STATE_ON_LOW_WATER, "�� ���������� ����")
REGISTER_ENUM(MOVEMENT_STATE_ON_WATER, "�� ����")
REGISTER_ENUM(MOVEMENT_STATE_ALL_SIDES, "�� ��� �������")
REGISTER_ENUM(MOVEMENT_STATE_ALL_SURFACES, "�� ����� �����������")
END_ENUM_DESCRIPTOR(MovementStateID)

BEGIN_ENUM_DESCRIPTOR(ExcludeCollision, "ExcludeCollision")
REGISTER_ENUM(EXCLUDE_COLLISION_BULLET, "EXCLUDE_COLLISION_BULLET")
REGISTER_ENUM(EXCLUDE_COLLISION_ENVIRONMENT, "EXCLUDE_COLLISION_ENVIRONMENT")
END_ENUM_DESCRIPTOR(ExcludeCollision)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, ProductionRequirement, "ProductionRequirement")
REGISTER_ENUM_ENCLOSED(AttributeBase, PRODUCE_EVERYWHERE, "����������� �����");
REGISTER_ENUM_ENCLOSED(AttributeBase, PRODUCE_ON_WATER, "����������� �� ����");
REGISTER_ENUM_ENCLOSED(AttributeBase, PRODUCE_ON_TERRAIN, "����������� �� �����");
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, ProductionRequirement)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeBase, AttackTargetNotificationMode, "AttackTargetNotificationMode")
REGISTER_ENUM_ENCLOSED(AttributeBase, TARGET_NOTIFY_SQUAD, "��������� ���� �����");
REGISTER_ENUM_ENCLOSED(AttributeBase, TARGET_NOTIFY_ALL, "��������� ���� � �������� �������");
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
REGISTER_ENUM_ENCLOSED(EffectAttribute, WATER_BOTTOM, "������� �� ���")
REGISTER_ENUM_ENCLOSED(EffectAttribute, WATER_SURFACE, "������� �� ����������� ����")
END_ENUM_DESCRIPTOR_ENCLOSED(EffectAttribute, WaterPlacementMode)

BEGIN_ENUM_DESCRIPTOR(ShadowType, "ShadowType")
REGISTER_ENUM(SHADOW_CIRCLE, "������� ����");
REGISTER_ENUM(SHADOW_REAL, "�������� ����");
REGISTER_ENUM(SHADOW_DISABLED, "��� ����");
END_ENUM_DESCRIPTOR(ShadowType)

BEGIN_ENUM_DESCRIPTOR(SoundSurfKind, "SoundSurfKind")
REGISTER_ENUM(SOUND_SURF_ALL, "��� ���� �����������");
REGISTER_ENUM(SOUND_SURF_KIND1, "����������� 1 ����");
REGISTER_ENUM(SOUND_SURF_KIND2, "����������� 2 ����");
REGISTER_ENUM(SOUND_SURF_KIND3, "����������� 3 ����");
REGISTER_ENUM(SOUND_SURF_KIND4, "����������� 4 ����");
END_ENUM_DESCRIPTOR(SoundSurfKind)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(BodyPartAttribute, Functionality, "BodyPartAttribute::Functionality")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, LIFE, "�����")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, MOVEMENT, "��������")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, PRODUCTION, "������������")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, UPGRADE, "�������")
REGISTER_ENUM_ENCLOSED(BodyPartAttribute, FIRE, "��������")
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

	selectCircleRelativeRadius = 1.0f; // �� ��������� ����. ������ �������� � radius()
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
		if(!ar.isEdit()){	// ������� ������� - ������ ����� �������
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
	ar.serialize(ModelSelector(modelName), "modelName", "��� ������");
	ar.serialize(boundHeight, "boundHeight", "������ ������ (�� ������)");
	if(isBuilding() || isResourceItem() || isInventoryItem())
		ar.serialize(radius_, "radius", "���������� ������");
	ar.serialize(boundScale, "boundScale", 0);
	ar.serialize(boundRadius, "boundRadius", 0);

	if(!ar.isEdit() || GlobalAttributes::instance().enableSilhouettes)
		ar.serialize(showSilhouette, "showSilhouette", "�������� ������");
	ar.serialize(hideByDistance, "hideByDistance", "�������� ��� ��������");

	ar.serialize(transparent_mode, "mode_transparent", "���������� ���������� ���� ������ ����");
	ar.serialize(fow_mode, "fow_mode", "����� ��������� ��� ������ �����");
	
	ShadowType shadowType = modelShadow.shadowType();
	ar.serialize(shadowType, "shadowType", "��� ����");
	if(shadowType == SHADOW_CIRCLE){
		float shadowRadiusFactor = modelShadow.shadowRadiusRelative() * boundScale;
		ar.serialize(shadowRadiusFactor, "shadowRadiusFactor", "������ ���� (����������� �� ������� �������)");
		shadowRadiusFactor *= boundScale;
		modelShadow.set(shadowType, shadowRadiusFactor);
	}else
		modelShadow.setType(shadowType);
	
	ar.serialize(permanentEffects, "permanentEffects", "���������� �������");
	
	ar.serialize(animationChains, "animationChains", 0);
	
	if((isBuilding() || isLegionary())){
		ar.serialize(parametersInitial, "parametersInitial", "������ (���������) ��������� �����");
		if(parametersInitial.possession())
			canBeCaptured = true;

		ar.serialize(bodyParts, "bodyParts", "����� ����");
		
		if(!ar.isEdit() && ar.isOutput() && !bodyParts.empty())
			RigidBodyModelPrmBuilder(rigidBodyModelPrm, bodyParts, model());
		
		ar.serialize(rigidBodyModelPrm, "rigidBodyModelPrm", 0);

		ar.openBlock("cost", "���������");
			ar.serialize(dockNodeNumber, "dockNodeNumber", "����� ���� � ������");
			ar.serialize(creationTime, "creationTime", "����� ������������, �������");
			ar.serialize(creationValue, "creationValue", "��������� ������������");
			ar.serialize(installValue, "installValue", "��������� ���������� ������");
			if(isBuilding()){
				ar.serialize(cancelConstructionValue, "cancelConstructionValue", "������������ �� �������������� ������");
				ar.serialize(uninstallValue, "uninstallValue", "������������ ����� �������������");
				ar.serialize(needBuilders, "needBuilders", "���������� ���������");
			}
			ar.serialize(accessValue, "accessValue", "����������� ��������� ��� ������������ � ��������");
			ar.serialize(accessBuildingsList, "accessBuildingsList", "����������� �������� ��� ������������ � ��������");
			ar.serialize(inheritHealthArmor, "inheritHealthArmor", "����������� �������� � ����� ��� ��������");
		ar.closeBlock();

		ar.serialize(upgrades, "upgrades", "��������");
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
		
		ar.openBlock("transport", "���������");
			ar.serialize(transportSlots, "transportSlots", "�����");
			ar.serialize(transportLoadRadius, "transportLoadRadius", "������ ������� ��� ������ ������");
			ar.serialize(transportLoadDirectControlRadius, "transportLoadDirectControlRadius", "������ ������� � ������ ����������");
			if(ar.isInput()){
				checkRequirementForMovement = false;
				TransportSlots::iterator i;
				FOR_EACH(transportSlots, i)
					checkRequirementForMovement |= i->requiredForMovement;
			}
			if(isLegionary()){
				ar.serialize(additionToTransport, "additionToTransport", "���������, ����������� ����������");
				ar.serialize(transportSlotsRequired, "transportSlotsRequired", "����������� ���������� ������ ��� ���������� � ����������");
			}
		ar.closeBlock();
	}
	else if(isResourceItem()){
		ar.openBlock("parameters", "���������");
		parametersArithmetics.serialize(ar);
		ar.closeBlock();
	}
	
	if(isObjective()){
		ar.openBlock("Interface", "���������");

		ar.serialize(tipsName, "tipsName", "��� ����� ��� ���������� (���)");
		
		ar.serialize(interfaceName_, "interfaceNames", "������� �������� ��� ���������� (���)");
		ar.serialize(interfaceDescription_, "interfaceDescriptions", "������ �������� ��� ���������� (���)");

		ar.openBlock("minimap", "����������� �� ���������");
		ar.serialize(minimapScale_, "minimapScale", "������������� ������� ����� ��� ������� �� ���������");
		ar.serialize(minimapSymbolType_, "symbolType", "��� �������");
		if(minimapSymbolType_ == UI_MINIMAP_SYMBOLTYPE_SELF){
			///  CONVERSION 2006-11-7
			if(!ar.serialize(minimapSymbol_, "minimapSymbol", "����������� ������")){
				minimapSymbol_.serialize(ar);
			}
			/// ^^^^^
		}
		ar.serialize(hasPermanentSymbol_, "hasPermanentSymbol", "�������� ���������� ������");
		if(hasPermanentSymbol_)
			ar.serialize(minimapPermanentSymbol_, "minimapPermamentSymbol", "���������� ������");
		ar.closeBlock();

		ar.serialize(isHero, "isHero", "����� (��� ����������)");
		ar.serialize(isStrategicPoint, "isStrategicPoint", "�������������� ����� (��� ����������)");
		ar.serialize(accountInCondition, "accountInCondition", "��������� � ������� '� ������ �� �������� ������������ ������'");

		ar.serialize(inventories, "inventories", "���������");

		ar.serialize(selectBySphere, "selectBySphere", "��������� �� ��������� �����");
		ar.serialize(selectionCursor_, "selection_cursor", "������ ������");
		selectionCursorProxy_ = selectionCursor_;
		ar.serialize(selectionListPriority, "selectionListPriority", "��������� � ������ �������");

		ar.serialize(initialHeightUIParam, "initialHeightUIParam", "������ ����� ��� ������ ��������");
		if(isTransport()){
			ar.openBlock("TransportSlots", "������������ ������������ ������");
			ar.serialize(transportSlotShowEvent, "transportSlotShowEvent", "����� ����������");
			if(transportSlotShowEvent == SHOW_AT_PARAMETER_INCREASE || transportSlotShowEvent == SHOW_AT_PARAMETER_DECREASE)
				transportSlotShowEvent = SHOW_ALWAYS;
			ar.serialize(transportSlotEmpty, "transportSlotEmpty", "������ ����");
			ar.serialize(transportSlotFill, "transportSlotFill", "����������� ����");
			ar.closeBlock();
		}

		ParameterShowSetting::possibleParameters_ = &parametersInitial;
		ar.serialize(parameterShowSettings, "parameterShowSettings", "��������� ���������");
		ar.serialize(showChangeParameterSettings, "showChangeParameterSettings", "������������ ��������� ����� ����������");

		ar.openBlock("selection", "��� �������");
			ar.serialize(selectCircleRelativeRadius, "selectCircleRelativeRadius", "������������� ������ ����������");
			ar.serialize(selectRadius, "selectRadius", 0);
			ar.serialize(selectCircleColor, "selectCircleColor", "���� ����������");
			ar.serialize(fireRadiusCircle, "fireRadiusCircle", "������ ������������� ������� �����");
			ar.serialize(fireMinRadiusCircle, "fireMinRadiusCircle", "������ ������������ ������� �����");
			ar.serialize(signRadiusCircle, "signRadiusCircle", "������ ������� ���������");
		ar.closeBlock();

		ar.serialize(interfaceTV, "interfaceTV", "�����������");

		ar.closeBlock();
	}
	
	if(isActing()){
		ar.serialize(weaponAttributes, "weaponAttributes", "������");

		ar.openBlock("attack", "�����");
		bool no_conversion = ar.serialize(hasAutomaticAttackMode, "hasAutomaticAttackMode", "����������� ��������� ������� �����");
		if(hasAutomaticAttackMode)
			ar.serialize(attackModeAttribute, "attackModeAttribute", "��������� ������� �����");

		ar.serialize(attackTargetNotificationMode, "attackTargetNotificationMode", "����� ���������� � ���������� ������");

		if(attackTargetNotificationMode & TARGET_NOTIFY_ALL)
			ar.serialize(attackTargetNotificationRadius, "attackTargetNotificationRadius", "������ ���������� � ���������� ������ (������������ ������� ���������)");

		ar.closeBlock();

		if(!no_conversion){ // conversion 24.08
			ar.serialize(hasAutomaticAttackMode, "hasAutomaticAttackMode", "����������� ��������� ������� �����");
			if(hasAutomaticAttackMode)
				ar.serialize(attackModeAttribute, "attackModeAttribute", "��������� ������� �����");
		}

		ar.openBlock("directControl", "������ ����������");
			if(GlobalAttributes::instance().directControlMode)
				ar.serialize(defaultDirectControlEnabled, "defaultDirectControlEnabled", "�������� ������ ���������� �� ���������");
			ar.serialize(directControlNode, "directControlNode", "���� ��� �������� ������");
			ar.serialize(directControlOffset, "directControlOffset", "�������� ������");
			if(!ar.serialize(directControlOffsetWater, "directControlOffsetWater", "�������� ������ ��� ��������"))
				directControlOffsetWater = directControlOffset;
			float tmp = directControlThetaMin / M_PI * 180.f;
			ar.serialize(tmp, "directControlThetaMin", "����������� ����");
			directControlThetaMin = clamp(tmp / 180.f * M_PI, 0.f, M_PI) ;
			tmp = directControlThetaMax / M_PI * 180.f;
			ar.serialize(tmp, "directControlThetaMax", "������������ ����");
			directControlThetaMax = clamp(tmp / 180.f * M_PI, directControlThetaMin, M_PI) ;
		ar.closeBlock();
	}

	if(!isLegionary() && !isProjectile()){ // ������� ����� "��������"
		ar.serialize(rigidBodyPrm, "rigidBodyPrm", 0);
	}

	if(isObjective() || isProjectile()){
		ar.openBlock("Death", "������");
			ar.serialize(waterEffect, "waterEffect", "����������� �� ����");
			ar.serialize(lavaEffect, "lavaEffect", "����������� �� ����");
			ar.serialize(iceEffect, "iceEffect", "����������� �� ���������");
			ar.serialize(earthEffect, "earthEffect", "����������� �� �����");
   			harmAttr.serialize(ar);
			ar.serialize(contactWeight, "contactWeight", "���� ����������� ��� ��������");

			if(isBuilding() || isLegionary()){
				ar.serialize(unitAttackClass, "unitAttackClass", "����� ����� �����");
				if(unitAttackClass == ATTACK_CLASS_ENVIRONMENT_BIG_BUILDING)
					unitAttackClass = ATTACK_CLASS_LIGHT;

				ar.serialize(excludeFromAutoAttack, "excludeFromAutoAttack", "��������� �� ���������������� ������ ����� ��� �����");
				
				ar.serialize(leavingItems, "leavingItems", "����������� ��������");
				ar.serialize(deathGainArithmetics, "deathGainArithmetics", "���������� �� ������");

				if(isLegionary())
					ar.serialize(armorFactors, "armorFactors", "������������ �����");
			}
		ar.closeBlock();
	}

	if(isActing()){
		ar.openBlock("Production", "������������");

		ar.serialize(producedUnits, "producedUnits", "������������ �����");
		if(ar.isInput())
			fixIntMap(producedUnits);

		ar.serialize(producedUnitQueueSize, "producedUnitQueueSize", "������������ ����� �������");
		if(!ar.serialize(dockNodes, "dockNodes", "��� ����")){ // conversion 15.09
			Logic3dxNode dockNode;
			ar.serialize(dockNode, "dockNode", "��� ����");
			dockNodes.push_back(dockNode);
		}
				
		if(isBuilding()){
			ar.serialize(automaticProduction, "automaticProduction", "�������������� ������������ ������");
			ar.serialize(totalProductionNumber, "totalProductionNumber", "������������ ���������� ������������� ������ ��� ����������");
		}

		productivity *= 1.f/logicPeriodSeconds;
		ar.serialize(productivity, "productivity", "��������������� - ������������������ � �������");
		productivity *= logicPeriodSeconds;
		ar.serialize(productivityTotal, "productivityTotal", "��������������� - ������������ ������������������");
		ar.serialize(productionRequirement, "productionRequirement", "���������� ��� ������������������ �������");
		ar.serialize(productionNightFactor, "productionNightFactor", "����������� �����");

		ar.serialize(producedParameters, "producedParameters", "������������ ���������");
		if(ar.isInput())
			fixIntMap(producedParameters);

		if(ar.isInput()){
			produceParametersAutomatically = false;
			ProducedParametersList::iterator i;
			FOR_EACH(producedParameters, i)
				produceParametersAutomatically |= i->second.automatic;
		}

        		
		ar.serialize(resourceCapacity, "resourceCapacity", "������� ��� �������");
		ar.serialize(putInIdleList, "putInIdleList", "�������� � ������ �������������� ������");

		ar.closeBlock();
	}

	ar.serialize(excludeCollision, "excludeCollision", 0);
	ar.serialize(collisionGroup, "collisionGroup", 0);

	ar.serialize(internal, "internal", 0);

	ar.serialize(boundBox.min, "boundBoxMin", 0);
	ar.serialize(boundBox.max, "boundBoxMax", 0);

	ar.serialize(producedThisFactories, "producedThisFactories", 0);

	if(isObjective()){
		ar.openBlock("chainTimes", "������� ������� ��������");
			ar.serialize(MillisecondsWrapper(chainTransitionTime), "chainTransitionTime", "�������� �� ������ �����������");
			//ar.serialize(chainGoToRunTime, "chainGoToRunTime", "������� �� ���");
			//ar.serialize(chainStopToRunTime, "chainStopToRunTime", "������� �� ���");
			if(isLegionary())
				ar.serialize(MillisecondsWrapper(chainLandingTime), "chainLandingTime", "������� � ���������");
			if(isResourceItem() || isInventoryItem())
				ar.serialize(MillisecondsWrapper(chainGiveResourceTime), "chainGiveResourceTime", "�������� ������");
			if(isActing()){
				ar.serialize(MillisecondsWrapper(chainRiseTime), "chainRiseTime", "��������");
				ar.serialize(MillisecondsWrapper(chainOpenTime), "chainOpenTime", "�������");
				ar.serialize(MillisecondsWrapper(chainCloseTime), "chainCloseTime", "�������");
				ar.serialize(MillisecondsWrapper(chainBirthTime), "chainBirthTime", "��������");
				ar.serialize(MillisecondsWrapper(chainOpenForLandingTime), "chainOpenForLandingTime", "������� ��� �������");
				ar.serialize(MillisecondsWrapper(chainCloseForLandingTime), "chainCloseForLandingTime", "������� ��� �������");
				ar.serialize(MillisecondsWrapper(chainFlyDownTime), "chainFlyDownTime", "���������� � ������");

				if(ar.isInput()){
					ar.serialize(MillisecondsWrapper(chainUpgradedFromBuilding), "chainIsUpgraded", "������� �� ������");
					ar.serialize(MillisecondsWrapper(chainUpgradedFromBuilding), "chainUpgradedFromBuilding", "������� �� ������");
				}
				else{
					ar.serialize(MillisecondsWrapper(chainUpgradedFromBuilding), "chainUpgradedFromBuilding", "������� �� ������");
				}
				
				ar.serialize(MillisecondsWrapper(chainUpgradedFromLegionary), "chainUpgradedFromLegionary", "������� �� �����");
			}
			if(isBuilding())
				ar.serialize(MillisecondsWrapper(chainUninsatalTime), "chainUninsatalTime", "�������� ������");
		ar.closeBlock();
	}

	if(isActing()){
		ar.openBlock("Invisibility", "�����������");
		ar.serialize(invisible, "invisible", "���� �������");
		ar.serialize(canChangeVisibility, "canChangeVisibility", "���� ����� ������ ����� ���������");
		ar.serialize(transparenceDiffuseForAlien, "invisibleColorForAlien", "��������� ��� ������");
		ar.serialize(transparenceDiffuseForClan, "invisibleColorForClan", "��������� ��� �����");
		ar.closeBlock();
	}

	ar.serialize(lodDistance,"distanceLod","���: ��������� ������������");
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
		// @Hallkezz
	try{
		if(!strlen(modelName.c_str()) || !gb_VisGeneric)
			return;

		cObject3dx* logic = logicModel();
		if(!logic){
			logic = model();
			kdWarning("GAV", XBuffer() < TRANSLATE("����������� ���������� ����� � ������ ") < modelName.c_str());
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
		xassert_s(deltaBound.x > FLT_EPS && deltaBound.y > FLT_EPS && deltaBound.z > FLT_EPS && "������ ������� ��������� ��� �� ����� ����������� ������: ", modelName.c_str());
		float radiusMin = 3;
		for(int i = 0; i < 2; i++)
			if(deltaBound[i] < 2*radiusMin){
				boundBox.max[i] = radiusMin;
				boundBox.min[i] = -radiusMin;
			}
		if(deltaBound.z < 2*radiusMin)
			boundBox.max.z = 2*radiusMin - boundBox.min.z;

		selectRadius = selectCircleRelativeRadius*((const Vect2f&)boundBox.max).distance(boundBox.min)/2;
		} catch (...) {
	}
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

	//xassertStr(interval.first + counter < interval.second && "������������ ������ �������� ��� ������ ������ �� ��������", libraryKey());
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
			if(i != end){ // � �����������
				AnimationChains::const_iterator begin = i;
				for(; i != end; ++i)
					if(i->chainID != chainID || !i->checkAbnormalState(astate) || (i->movementState & movementState) != movementState)
						break;
				return AnimationChainsInterval(begin, i);
			}
			else if(iAstateNone != end){ // ��� ���������
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
			if(i != end){ // � �����������
				AnimationChains::const_iterator begin = i;
				for(; i != end; ++i)
					if(i->chainID != CHAIN_TRANSITION || !i->checkAbnormalState(astate)
						|| (i->movementState & stateFrom) != stateFrom || (i->transitionToState & stateTo) != stateTo)
						break;
				return AnimationChainsInterval(begin, i);
			}
			else if(iAstateNone != end){ // ��� ���������
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
	ar.serialize(surfaceKind_, "surfaceKind", "��� �����������");
	ar.serialize(traceTerTool_, "traceTerTool", "����");
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

	ar.serialize(chainID, "chainID", "&������������� �������");
	if(chainID == CHAIN_IS_UPGRADED)
		chainID = CHAIN_UPGRADED_FROM_BUILDING;
	ar.serialize(movementState, "movementState", "&��������");
	if(chainID == CHAIN_TRANSITION)
		ar.serialize(transitionToState, "transitionToState", "������� � ���������");
	ar.serialize(counter, "counter", "&�����");

	ar.serialize(chainIndex_, "ChainIndex", "&��� ������� � ������");
	ar.serialize(animationGroup_, "AnimationGroup", "&��� ������������ ������");
	ar.serialize(visibilityGroup_, "VisibilityGroup", "&��� ������ ���������");

	if(ar.isInput() && !ar.isEdit()){
		string name;
		if(ar.serialize(name, "chainName", 0))
			chainIndex_.set(name.c_str());
		if(ar.serialize(name, "animationGroupName", 0))
			animationGroup_.set(name.c_str());
		if(ar.serialize(name, "visibilityGroupName", 0))
			visibilityGroup_.set(name.c_str());
	}

	ar.serialize(animationAcceleration, "animationAcceleration", "��������� ��������, %");
	ar.serialize(cycled, "cycled", "�����������");
	ar.serialize(priority, "priority", "��������� (0 - ����� ������)");
	ar.serialize(supportedByLogic, "supportedByLogic", 0);
	ar.serialize(reversed, "reversed", "����������� � �������� �������");
	ar.serialize(syncBySound, "syncBySound", "���������������� �� �����");
	ar.serialize(randomPhase, "randomPhase", "������������� ��������� ����");
	ar.serialize(effects, "effects", "�����������");
	ar.serialize(stopPermanentEffects, "stopPermanentEffects", "��������� ���������� �������");
	
	ar.serialize(soundReferences, "soundReferences", "�����");
	ar.serialize(soundMarkers, "soundMarkers", "�������� �����");

	ar.serialize(abnormalStateTypes, "abnormalStateTypes", "&���� �����������");

	if(ar.isOutput()){
		cObject3dx* model = AttributeBase::model();
		if(model)
			period = max(round(model->GetChain(chainIndex())->time*1000/max(1.f + animationAcceleration/100.f, 0.001f)), 100);
	}

	ar.serialize(period, "period", 0);
	float periodSeconds = period/1000.f;
	ar.serialize(periodSeconds, "periodSeconds", "������ �������� (��� ���������), �������");

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
	ar.serialize(type, "type", "&��� �����");
	ar.serialize(numberParameters, "numberParameters", "&������������ ����������");
}


void UnitColor::serialize(Archive& ar)
{
	ar.serialize(color, "color", "����");
	ar.serialize(isBrightColor, "isBrightColor", "���������� ����");
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
{	// ����� ������ ����� � �� �����
	// ����� ����� ������ ��������� �� �������� (���������) � ��� ����� �����������, ��� �� �����
	// ����� ������ ����� - ��� ������� ������� �������� ������
	// �� ����� ����� ������������, ����� �� ������ ����� ��� ������ ������������ ������, ���������� �� ���� ������ ������
	// ������������ ������� ���������� �� �����������

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
	ar.serialize(autoAttackMode_, "autoAttackMode", "����� �����");
	ar.serialize(autoTargetFilter_, "autoTargetFilter", "����� ��������������� ������ �����");
//	ar.serialize(walkAttackMode_, "walkAttackMode", "����� ����� ��� ��������");
	ar.serialize(weaponMode_, "weaponMode", "����� ������");
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
	ar.serialize(attackMode_, "attackMode", "��������� ��������� ������� �����");
/*
	ar.serialize(patrolRadius_, "patrolRadius", "������ ��������������");
	ar.serialize(patrolStopTime_, "patrolStopTime", "����� ��������� ��� ��������������, ���");
	ar.serialize(patrolMovementMode_, "patrolMovementMode", "����� �������� ��� ��������������");
*/
	ar.serialize(targetInsideSightRadius_, "targetInsideSightRadius", "������ ���� ��� ������ �� ������� ���������");
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
	ar.serialize(locName_, "locName", "��� ����");
	ar.serialize(fileNameAddition_, "fileNameAddition", "������� � ������ ������");
	ar.serialize(instrumentary_, "instrumentary", "���������");
	ar.serialize(usedAlways_, "usedAlways", "��������� ������");

	ar.serialize(shipmentPositionMark_, "shipmentPositionMark", "������ ����� ����� ������������ ������");

	ar.openBlock("orderMarks", "������������ ������ ��������");
	int i;
	for(i = 0; i < UI_CLICK_MARK_SIZE; i++)
		ar.serialize(orderMarks_[i], getEnumName(UI_ClickModeMarkID(i)), getEnumNameAlt(UI_ClickModeMarkID(i)));
	ar.closeBlock();

//	ar.serialize(unitAttackEffect_, "unitAttackEffect", "������������ ����� �� �����");
	ar.serialize(weaponUpgradeEffect_, "weaponUpgradeEffect", "������������ �������� ������");

	ar.openBlock("minimapMarks", "������������ ������� � ������ �� ���������");
	for(i = 0; i < UI_MINIMAP_SYMBOL_MAX; i++)
		ar.serialize(minimapMakrs_[i], getEnumName(UI_MinimapSymbolID(i)), getEnumNameAlt(UI_MinimapSymbolID(i)));
	ar.closeBlock();
	ar.openBlock("controlAI", "��� ����������� AI");
	ar.serialize(workForAISprite_, "workForAISprite", "������� ��� ������ ��� AI");
	ar.serialize(workForAIEffect_, "workForAIEffect", "������ ��� ������ ��� AI");
	ar.closeBlock();
	ar.serialize(runModeSprite_, "runModeSprite", "������� ��� ������ ��� ������ ����");

	ar.serialize(initialResource, "initialResource", "�������������� ������");
	ar.serialize(resourceCapacity, "resourceCapacity", "�������������� �������");
	ar.serialize(initialUnits, "initialUnits", "�������������� ����� ������");
	
	ar.serialize(commonTriggers, "commonTriggers", 0);
	ar.serialize(scenarioTriggers, "scenarioTriggers", "�������� ��� ������");
	ar.serialize(battleTriggers, "battleTriggers", "�������� ��� ������");
	ar.serialize(multiplayerTriggers, "multiplayerTriggers", "�������� ��� ������������");
	if(ar.isInput() && !ar.isEdit()){
		if(scenarioTriggers.empty())
			scenarioTriggers = commonTriggers;
	}

	ar.serialize(selectQuantityMax, "selectQuantityMax", "������������ ���������� ������ � �������");
	ar.serialize(unitNumbers, "unitNumbers", "������������ ���������� ������ �� �����");
	
	ar.serialize(startConstructionSound, "startConstructionSound", "���� �� ������ ������������� ������");
	ar.serialize(unableToConstructSound, "unableToConstructSound", "����, ����� ������ ������ ����������");

	ar.serialize(attackModeAttribute_, "attackModeAttribute", "��������� ������� �����");

	ar.serialize(produceMultyAmount, "produceMultyAmount", "���������� ������������ ������ � ������");

	ar.serialize(circle, "circle", "��������� ������������ �������");
	ar.serialize(circleTeam, "circleTeam", "��������� ������������ ������� ���������� � ��������� ������");

	selection_param.serialize(ar);

	ar.serialize(screenToPreload, "screenToPreload", "����� ��� ������������");
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
	ar.serialize(unitName_, "name", "&���");
	ar.serialize(race_, "race", "&����");
}

///////////////////////////////////////////////
void InterfaceTV::serialize(Archive& ar) 
{
	ar.serialize(radius_, "radius", "������ ������");
	ar.serialize(position_, "position", "��������");
	ar.serialize(orientation_, "orientation", "�������");
	ar.serialize(chain_, "chain", "������������ �������");
}

//////////////////////////////////////////
void WeaponDamage::serialize(Archive& ar) 
{
	ar.serialize(static_cast<ParameterCustom&>(*this), "MainDamage", "�������� �����������");
}

ArmorFactors::ArmorFactors() 
: front(1), back(1), left(1), right(1), top(1), used_(false)
{}

void ArmorFactors::serialize(Archive& ar)
{
	ar.serialize(front, "front", "�����");
	ar.serialize(back, "back", "���");
	ar.serialize(left, "left", "�����");
	ar.serialize(right, "right", "������");
	ar.serialize(top, "top", "������");
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
	ar.serialize(locName, "locName", "�������������� ���");
	ar.serialize(aiDelay, "aiDelay", "�������� ��");
	ar.serialize(triggerDelayFactor, "triggerDelayFactor", "����������� �������� ��������");
	ar.serialize(orderBuildingsDelay, "orderBuildingsDelay", "�������� ������������� ������");
	ar.serialize(orderUnitsDelay, "orderUnitsDelay", "�������� ������������ ������");
	ar.serialize(orderParametersDelay, "orderParametersDelay", "�������� ������������ ����������");
	ar.serialize(upgradeUnitDelay, "upgradeUnitDelay", "�������� �������� ����� � ������");
	ar.serialize(simpleScanDelay, "simpleScanDelay", "�������� ��� ������� ����������� �������");
	ar.serialize(eventScanDelay, "eventScanDelay", "�������� ��� ���������� ����������� �������");
	//ar.serialize(attackBySpecialWeaponDelay, "attackBySpecialWeaponDelay", "�������� ����� ����������� ������� �����������");
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
	ar.serialize(ModelSelector(fileName_, ModelSelector::EFFECT_OPTIONS), "fileName_", "��� �����");
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
	ar.serialize(isCycled_, "isCycled", "�����������");
	ar.serialize(stopImmediately_, "stopImmediately", "�������� ��� ���������");
	ar.serialize(bindOrientation_, "bindOrientation", "������������� �� �������");
	ar.serialize(legionColor_, "legionColor", "���������� � ���� �������");
	ar.serialize(switchOffByInterface_, "switchOffByInterface", "������ ��� ���������� ����������");
	ar.serialize(switchOffUnderWater_, "switchOffUnderWater", "��������� � ����");
	ar.serialize(switchOffUnderLava_, "switchOffUnderLava", "��������� � ����");
	ar.serialize(switchOffByDay_, "switchOffByDay", "�������� �����");
	ar.serialize(switchOffOnIce_, "switchOffOnIce", "��������� �� ����");
	ar.serialize(switchOnIce_, "switchOnIce", "�������� ������ �� ����");
	ar.serialize(ignoreFogOfWar_, "ignoreFogOfWar", "����� � ������ �����");
	ar.serialize(ignoreInvisibility_, "ignoreInvisibility", "����� �� ��������� �����");

	ar.serialize(waterPlacementMode_, "waterPlacementMode", "����� ������ �� ����");

	ar.serialize(scale_, "scale", "�������");

	ar.serialize(effectReference_, "effectReference", "&������");
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

	ar.serialize(scaleByModel_, "scaleByModel", "�������������� �� ������� �������");

	if (!isCycled_)
		ar.serialize(synchronizationWithModelAnimation_,"synchronizationWithModelAnimation","���������������� � ���������");
	ar.serialize(switchOffByAnimationChain_, "switchOffByAnimationChain", "����������� ��������� ��� ����������");
	if(needNodeName_)
		ar.serialize(node_, "node", "���� ��������");
}

// -------------------------------------------
void AttributeBase::ProducedUnits::serialize(Archive& ar)
{
	xassertStr(!(ar.isOutput() && !ar.isEdit() && !unit) && "������ ������������ ���� � ", currentLibraryKey());
	ar.serialize(unit, "unit", "&����");
	ar.serialize(number, "number", "&����������");
}

ProducedParameters::ProducedParameters() 
: time(60),
  automatic(false)
{
}

void ProducedParameters::serialize(Archive& ar)
{
	arithmetics.serialize(ar);
	ar.serialize(time, "time", "�����, �������");
	ar.serialize(cost, "cost", "���������");
	ar.serialize(accessValue, "accessValue", "����������� ��������� ��� ������������");
	ar.serialize(accessBuildingsList, "accessBuildingsList", "����������� �������� ��� ������������");
	ar.serialize(sprites_, "sprites", "������� ��� �������");
	ar.serialize(signalVariable, "signalVariable", "��� ���������� ���������� (����� �������� ��� ���������� ���������� ����������)");
	ar.serialize(automatic, "automatic", "����������� �������������");
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
	ar.serialize(abnormalStateEffects, "abnormalStateEffects", "������� �� �����������");
#ifndef _FINAL_VERSION_
	AbnormalStateEffects::const_iterator it;
	FOR_EACH(abnormalStateEffects, it)
		if(it->effectAttribute().switchOffByInterface()){
			XBuffer buf;
			buf < "����: " < AttributeBase::currentLibraryKey()
				< "\n������: " < it->effectAttribute().effectReference().c_str()
				< "\n�� �����������: \"" < it->typeRef().c_str() < "\""
				< "\n����������� ������ � �����������";
			xxassert(false, buf.c_str());
			kdError("Effects", buf.c_str());
		}
#endif
}

void HarmAttribute::serializeSources(Archive& ar)
{
	ar.serialize(deathAttribute_.sources, "sources", "���������, ���������� ����� ������");
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
	xassertStr(!(ar.isOutput() && !ar.isEdit() && !upgrade) && "������ ������� � ", currentLibraryKey());
	ar.serialize(upgrade, "upgrade", "&�������");
	ar.serialize(automatic, "automatic", "&��������������");
	ar.serialize(upgradeValue, "upgradeValue", "��������� ��������");
	ar.serialize(accessParameters, "accessParameters", "����������� ������ ���������");
	if(!ar.serialize(chainUpgradeNumber, "chainUpgradeNumber", "����� ������� ��������"))
		chainUpgradeNumber = -1;
	ar.serialize(MillisecondsWrapper(chainUpgradeTime), "chainUpgradeTime", "����� ��������");
	if(!ar.isEdit() || upgrade && upgrade->isBuilding())
		ar.serialize(built, "built", "���������� � ����������� ������");
	if(!ar.isEdit() || upgrade && upgrade->isLegionary())
		ar.serialize(level, "level", "������� �����");
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
		xxassert(bodyParts.empty() || modelPrm.size() == 1, "�� ����� ������� ����");
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
	ar.serialize(circleColor, "circleColor", "���� ������ �����������");
	ar.serialize(showRadius, "showRadius", "������, �� ������� ���������� ���� �����������");
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

    // ������������� ����� ComboBox:
	FormationPatterns::instance();
	UnitFormationTypes::instance();
	PlacementZoneTable::instance();

    // ��� ����� ���� ������������� ���
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
		if(!isUnderEditor())
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
		if(!isUnderEditor())
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

    // ������������� ����� ComboBox:
	FormationPatterns::instance().saveLibrary();
	UnitFormationTypes::instance().saveLibrary();
	PlacementZoneTable::instance().saveLibrary();

    // ��� ����� ���� �������������� ���
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
	ar.serialize(type_, "second", "��������");
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
	ar.serialize(type_, "second", "��������");
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
			xassertStr(0 && "����� ���� �� ����������", race.c_str());
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
	ar.serialize(bodyPartType, "bodyPartType", "&��� ����� ����");
	ar.serialize(functionality, "functionality", "����������");
	if(functionality & FIRE)
		ar.serialize(weapons, "weapons", "������ �� ������");
	ar.serialize(percent, "percent", "������� �� ������ ��������");
	ar.serialize(visibilitySet_, "visibilitySetName", "&����� ������");
	VisibilityGroupOfSet::setVisibilitySet(visibilitySet_);
	ar.serialize(defaultGarment, "defaultGarment", "&������ �� ���������");
	ar.serialize(possibleGarments, "possibleGarments", "��������� ������");
	ar.serialize(automaticGarments, "automaticGarments", "�������������� ������");
	ar.serialize(rigidBodyBodyPartPrm, "rigidBodyBodyPartPrm", "���������� ������");
}

void BodyPartAttribute::Garment::serialize(Archive& ar)
{
	ar.serialize(visibilityGroup, "visibilityGroup", "������ ���������");
	ar.serialize(item, "item", "�������");
}

void BodyPartAttribute::AutomaticGarment::serialize(Archive& ar)
{
	Garment::serialize(ar);
	ar.serialize(parameters, "parameters", "��������� ���������");
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
	ar.serialize(logicNode, "logicNode", "���������� ����");
	ar.serialize(graphicNode, "graphicNode", "����������� ����");
	upperLimits.set(R2G(upperLimits.x), R2G(upperLimits.y), R2G(upperLimits.z));
	lowerLimits.set(R2G(lowerLimits.x), R2G(lowerLimits.y), R2G(lowerLimits.z));
	ar.serialize(upperLimits, "upperLimits", "������� ������ �������");
	ar.serialize(lowerLimits, "lowerLimits", "������ ������ �������");
	upperLimits.set(G2R(upperLimits.x), G2R(upperLimits.y), G2R(upperLimits.z));
	lowerLimits.set(G2R(lowerLimits.x), G2R(lowerLimits.y), G2R(lowerLimits.z));
	ar.serialize(mass, "mass", "�����");
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
	ar.serialize(shadowType_, "shadowType", "��� ����");
	if(shadowType_ == SHADOW_CIRCLE)
		ar.serialize(shadowRadiusRelative_, "shadowRadiusRelative", "������ ���� (����������� �� ������� �������)");
}

void ModelShadow::serializeForEditor(Archive& ar, float factor)
{
	ar.serialize(shadowType_, "shadowType", "��� ����");
	if(shadowType_ == SHADOW_CIRCLE){
		float shadowRadiusRelative = shadowRadiusRelative_ * factor;
		ar.serialize(shadowRadiusRelative, "shadowRadiusRelative", "������ ���� (����������� �� ������� �������)");
		shadowRadiusRelative_ = shadowRadiusRelative / factor;
	}
}

