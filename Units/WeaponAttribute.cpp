#include "stdafx.h"

#include "WeaponAttribute.h"
#include "WeaponPrms.h"
#include "UnitAttribute.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "TypeLibraryImpl.h"
#include "UnitActing.h"

#include "Player.h"


BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, WeaponClass, "WeaponPrm::WeaponClass")
REGISTER_ENUM_ENCLOSED(WeaponPrm, WEAPON_BEAM, "лучевое оружие")
REGISTER_ENUM_ENCLOSED(WeaponPrm, WEAPON_PROJECTILE, "стреляющее снарядами оружие")
REGISTER_ENUM_ENCLOSED(WeaponPrm, WEAPON_AREA_EFFECT, "действующее на зону оружие")
REGISTER_ENUM_ENCLOSED(WeaponPrm, WEAPON_WAITING_SOURCE, "источники с отложенной активацией")
REGISTER_ENUM_ENCLOSED(WeaponPrm, WEAPON_GRIP, "оружие - захват")
//REGISTER_ENUM_ENCLOSED(WeaponPrm, WEAPON_TELEPORT, "оружие телепортации")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, WeaponClass)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, ShootingOnMoveMode, "WeaponPrm::ShootingOnMoveMode")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_WHILE_STANDING, "когда стоит")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_WHILE_MOVING, "на ходу")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_WHILE_RUNNING, "на бегу")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_WHILE_IN_TRANSPORT, "в транспорте")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, ShootingOnMoveMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, ShootingMode, "WeaponPrm::ShootingMode")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_MODE_DEFAULT, "обычный режим")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_MODE_INTERFACE, "по команде из интерфейса")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHOOT_MODE_ALWAYS, "всегда")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, ShootingMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, UnitMode, "WeaponPrm::UnitMode")
REGISTER_ENUM_ENCLOSED(WeaponPrm, ON_WATER_BOTTOM, "на дне")
REGISTER_ENUM_ENCLOSED(WeaponPrm, ON_WATER, "на воде")
REGISTER_ENUM_ENCLOSED(WeaponPrm, ON_GROUND, "на земле")
REGISTER_ENUM_ENCLOSED(WeaponPrm, ON_AIR, "в воздухе")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, UnitMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, TargetUnitMode, "WeaponPrm::TargetUnitMode")
REGISTER_ENUM_ENCLOSED(WeaponPrm, TARGET_ON_WATER_BOTTOM, "цели на дне")
REGISTER_ENUM_ENCLOSED(WeaponPrm, TARGET_ON_WATER, "плавающие цели")
REGISTER_ENUM_ENCLOSED(WeaponPrm, TARGET_ON_GROUND, "наземные цели")
REGISTER_ENUM_ENCLOSED(WeaponPrm, TARGET_ON_AIR, "воздушные цели")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, TargetUnitMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, RangeType, "WeaponPrm::RangeType")
REGISTER_ENUM_ENCLOSED(WeaponPrm, LONG_RANGE, "оружие дальнего боя")
REGISTER_ENUM_ENCLOSED(WeaponPrm, SHORT_RANGE, "оружие ближнего боя")
REGISTER_ENUM_ENCLOSED(WeaponPrm, ANY_RANGE, "универсальное оружие")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPrm, RangeType)

BEGIN_ENUM_DESCRIPTOR(AffectMode, "AffectMode")
REGISTER_ENUM(AFFECT_OWN_UNITS, "только на своих юнитов")
REGISTER_ENUM(AFFECT_FRIENDLY_UNITS, "на своих и союзных юнитов")
REGISTER_ENUM(AFFECT_ALLIED_UNITS, "на союзных юнитов")
REGISTER_ENUM(AFFECT_ENEMY_UNITS, "на вражеских юнитов")
REGISTER_ENUM(AFFECT_ALL_UNITS, "на всех юнитов")
REGISTER_ENUM(AFFECT_NONE_UNITS, "не воздействует на юнитов")
END_ENUM_DESCRIPTOR(AffectMode)

BEGIN_ENUM_DESCRIPTOR(WeaponDirectControlMode, "WeaponDirectMode")
REGISTER_ENUM(WEAPON_DIRECT_CONTROL_DISABLE, "не стрелять")
REGISTER_ENUM(WEAPON_DIRECT_CONTROL_NORMAL, "стрелять по нажатию левой кнопки мыши")
REGISTER_ENUM(WEAPON_DIRECT_CONTROL_ALTERNATE, "стрелять по нажатию правой кнопки мыши")
REGISTER_ENUM(WEAPON_DIRECT_CONTROL_AUTO, "автоматически выбирать цель")
END_ENUM_DESCRIPTOR(WeaponDirectControlMode)

BEGIN_ENUM_DESCRIPTOR(WeaponSourcesCreationMode, "WeaponSourcesCreationMode")
REGISTER_ENUM(WEAPON_SOURCES_CREATE_ON_TARGET_HIT, "при попадании в цель")
REGISTER_ENUM(WEAPON_SOURCES_CREATE_ON_GROUND_HIT, "при попадании в землю")
REGISTER_ENUM(WEAPON_SOURCES_CREATE_ALWAYS, "всегда")
END_ENUM_DESCRIPTOR(WeaponSourcesCreationMode)

BEGIN_ENUM_DESCRIPTOR(WeaponGroupShootingMode, "WeaponGroupShootingMode")
REGISTER_ENUM(WEAPON_GROUP_INDEPENDENT, "независимо")
REGISTER_ENUM(WEAPON_GROUP_MODE_PRIORITY, "в соответствии с приоритетом")
END_ENUM_DESCRIPTOR(WeaponGroupShootingMode)

WRAP_LIBRARY(WeaponGroupTypeTable, "WeaponGroup", "Типы групп оружия", "Scripts\\Content\\WeaponGroup", 1, false);

// ------------------------------

WeaponAimAnglePrm::WeaponAimAnglePrm()
{
	precision_ = 1.0f;
	turnSpeed_ = 10.0f;
	turnSpeedDirectControl_ = 2000.0f;

	valueMin_ = 0;
	valueMax_ = 360;
	valueDefault_ = 0;

	rotateByLogic_ = false;
}

void WeaponAimAnglePrm::serialize(Archive& ar)
{
	ar.serialize(nodeGraphics_, "nodeGraphics", "&имя узла для управления поворотом");
	ar.serialize(nodeLogic_, "nodeLogic", "&имя логического узла для управления поворотом");

	ar.serialize(rotateByLogic_, "rotateByLogic", "поворачивать графический узел по осям логического");

	ar.serialize(precision_, "precision", "точность наведения");

	ar.serialize(turnSpeed_, "turnSpeed", "скорость поворота, градус/сек");
	ar.serialize(turnSpeedDirectControl_, "turnSpeedDirectControl", "скорость поворота при прямом управлении, градус/сек");

	ar.serialize(valueMin_, "valueMin", "минимальное значение");
	ar.serialize(valueMax_, "valueMax", "максимальное значение");
	ar.serialize(valueDefault_, "valueDefault", "значение по умолчанию");

	if(valueDefault_ < valueMin_) valueDefault_ = valueMin_;
	if(valueDefault_ > valueMax_) valueDefault_ = valueMax_;
}

WeaponAimControllerPrm::WeaponAimControllerPrm()
{
	isEnabled_ = false;
	hasAnimation_ = false;
	animationChainCounter_ = 0;
	animationChainAimCounter_ = 0;
	angleThetaPrm_.setValueLimits(-90, 90);
}

void WeaponAimControllerPrm::serialize(Archive& ar)
{
	ar.serialize(isEnabled_, "isEnabled", "&включено");

	if(!ar.serialize(barrels_, "barrels", "стволы")){
		Barrel barrel;
		barrel.serialize(ar);
		barrels_.push_back(barrel);
	}

	ar.serialize(hasAnimation_, "hasAnimation", "есть анимация");
	ar.serialize(animationChainCounter_, "animationChainCounter", "номер цепочки анимации стрелять");
	ar.serialize(animationChainAimCounter_, "animationChainAimCounter", "номер цепочки анимации целиться");

	ar.serialize(anglePsiPrm_, "anglePsiPrm", "горизонтальный угол");
	ar.serialize(angleThetaPrm_, "angleThetaPrm", "вертикальный угол");
}

void WeaponAimControllerPrm::Barrel::serialize(Archive& ar)
{
	ar.serialize(nodeLogic_, "nodeLogic", "&место вылета снаряда");
	ar.serialize(nodeGraphics_, "nodeGraphics", "&место спецэффекта выстрела");
}

// ------------------------------

WeaponSlotAttribute::WeaponSlotAttribute()
{
	equipmentCellType_ = 0;
	priority_ = 0;
}

void WeaponSlotAttribute::serialize(Archive& ar)
{
	ar.serialize(aimControllerPrm_, "aimControllerPrm", "управление наведением");
	ar.serialize(weaponPrmReference_, "weaponPrmReference", "&название оружия");

	ar.serialize(equipmentCellType_, "equipmentCellType", "тип ячейки снаряжения");
	ar.serialize(groupType_, "groupType", "группа оружия");
	ar.serialize(priority_, "priority", "приоритет");
}

// ------------------------------

void WeaponPrm::Upgrade::serialize(Archive& ar)
{
	ar.serialize(reference_, "reference", "&оружие");
}

WeaponPrm::WeaponPrm() : fireEffect_(false)
{
	ID_ = -1;

	clearTargets_ = false;
	exclusiveTarget_ = false;
	clearAttackClickMode_ = true;

	disableOwnerMove_ = false;

	fireCostAtOnce_ = false;

	queueFire_ = false;
	queueFireDelay_ = 200;

	weaponClass_ = WEAPON_BEAM;
	attackClass_ = ATTACK_CLASS_IGNORE;

	affectMode_ = AFFECT_ENEMY_UNITS;

	rangeType_ = LONG_RANGE;
	shootingMode_ = SHOOT_MODE_DEFAULT;
	directControlMode_ = WEAPON_DIRECT_CONTROL_NORMAL;

	shootingOnMoveMode_ = SHOOT_WHILE_STANDING | SHOOT_WHILE_MOVING | SHOOT_WHILE_RUNNING;

	isInterrupt_ = true;
	fireDuringClick_ = false;
	canShootThroughShield_ = false;
	canShootUnderFogOfWar_ = true;
	clearTargetOnLoading_ = false;

	targetUnitMode_.resize(UNIT_MODE_COUNT, TARGET_MODE_ALL);

	aimLockTime_ = 0.2f;
	
	visibleTimeOnShoot_ = 0.f;

	fireRadiusCircle_.color.set(255, 255, 255, 0);
	fireRadiusCircle_.width = 4;
	fireRadiusCircle_.dotted = CircleColor::CIRCLE_CHAIN_LINE;

	fireMinRadiusCircle_ = fireRadiusCircle_;
	fireMinRadiusCircle_.width = 2;

	hideCursor_ = false;
	alwaysPutInQueue_ = false;
}

WeaponPrm::~WeaponPrm()
{
}

void WeaponPrm::serialize(Archive& ar)
{
	ar.serialize(ID_, "ID", 0);
	ar.serialize(tipsName_, "tipsName", "Имя оружия для интерфейса/метка для типсов");

	ar.serialize(aimLockTime_, "aimLockTime", "время возвращения в исходное состояние");

	ar.serialize(queueFire_, "queueFire", "стреляет очередями");
	if(queueFire_){
		float time = float(queueFireDelay_) / 1000.f;
		ar.serialize(time, "queueFireDelay", "задержка между выстрелами в очереди");
		queueFireDelay_ = round(time * 1000.f);
	}

	ar.serialize(clearTargets_, "clearTargets", "сбрасывать цель после выстрела");
	ar.serialize(exclusiveTarget_, "exclusiveTarget", "не может делить цель с другим оружием");
	ar.serialize(clearAttackClickMode_, "clearAttackClickMode", "сбрасывать курсор после указания цели");

	ar.serialize(targetMark_, "targetMark", "визуализация точки прицеливания");
	ar.serialize(hideCursor_, "hideCursor", "прятать курсор во время прицеливания");

	if(weaponClass_ == WeaponPrm::WEAPON_BEAM || weaponClass_ == WeaponPrm::WEAPON_GRIP){
		ar.serialize(damage_, "damage", "повреждения");
	}

	ar.serialize(attackClass_, "attackClass", "класс атакуемых юнитов");
	ar.serialize(affectMode_, "affectMode", "на кого действует");

	ar.serialize(rangeType_, "rangeType", "тип оружия");
	ar.serialize(shootingMode_, "shootingMode", "режим стрельбы");
	ar.serialize(directControlMode_, "directControlMode", "режим стрельбы в прямом управлении");
	ar.serialize(directControlEffect_, "directControlEffect", "спецэффект для прямого управления");
	ar.serialize(directControlDisabledEffect_, "directControlDisabledEffect", "спецэффект для прямого управления при нехватке ресурсов для стрельбы");

	ar.serialize(effect_, "effect", "спецэффект для обычного режима");
	ar.serialize(disabledEffect_, "disabledEffect", "спецэффект для обычного режима при нехватке ресурсов для стрельбы");

	ar.serialize(canShootThroughShield_, "canShootThroughShield", "стреляет сквозь защитные поля");
	ar.serialize(canShootUnderFogOfWar_, "canShootUnderFogOfWar", "стреляет под туман войны");

	ar.serialize(shootingOnMoveMode_, "shootingOnMoveMode", "может стрелять");

	ar.serialize(isInterrupt_, "isInterrupt", "прерываемая стрельба");
	ar.serialize(clearTargetOnLoading_, "clearTargetOnLoading", "сбрасывать цель во время перезарядки");

	ar.serialize(disableOwnerMove_, "disableOwnerMove", "запретить подход и поворот юнита к цели");

	ar.openBlock("surfaceMode", "атакуемые цели");
	for(int i = 0; i < UNIT_MODE_COUNT; i++){
		UnitMode mode = UnitMode(i);
		if(!ar.serialize(targetUnitMode_[i], getEnumName(mode), getEnumNameAlt(mode)))
			targetUnitMode_[i] = TARGET_MODE_ALL;
	}
	ar.closeBlock();

	ar.serialize(fireDuringClick_, "fireDuringClick", "Стрелять пока нажата кнопка");

	ar.serialize(fireCost_, "fireCost", "стоимость стрельбы");
	ar.serialize(fireCostAtOnce_, "fireCostAtOnce", "снимать всю стоимость сразу");

	ar.serialize(parameters_, "parameters", "Личные параметры (дальность, время выстрела, время перезарядки, точность)");
	ar.serialize(visibleTimeOnShoot_, "visibleTimeOnShoot", "Время видимости при выстреле");
	ar.serialize(fireRadiusCircle_, "fireRadiusCircle", "Кружок радиуса атаки");
	ar.serialize(fireMinRadiusCircle_, "fireMinRadiusCircle", "Кружок минимального радиуса атаки");

	if(weaponClass_ == WeaponPrm::WEAPON_BEAM || weaponClass_ == WeaponPrm::WEAPON_GRIP)
		ar.serialize(abnormalState_, "abnormalState", "воздействие на цель");

	ar.serialize(fireEffect_, "fireEffect", "спецэффект выстрела");
	ar.serialize(fireSound_, "fireSound", "звук выстрела");

	ar.serialize(upgrades_, "upgrades", "апгрейды");

	ar.openBlock("cost", "Стоимость апгрейда");
	ar.serialize(accessValue_, "accessValue", "необходимые параметры");
	
	ar.serialize(accessBuildingsList_, "accessBuildingsList", "необходимые строения");

	ar.closeBlock();
}

void WeaponPrm::updateIdentifiers()
{
	int id = 1;
	for(WeaponPrmLibrary::Strings::const_iterator it = WeaponPrmLibrary::instance().strings().begin(); it != WeaponPrmLibrary::instance().strings().end(); ++it) {
		if(it->get()) {
			it->get()->setID(id++);
		}
	}
}

const WeaponPrm* WeaponPrm::accessibleUpgrade(UnitActing* ownerUnit) const
{
	const WeaponPrm* upgrade = 0;

	WeaponPrm::Upgrades::const_iterator it;
	FOR_EACH(upgrades(), it){
		if(const WeaponPrm* p = (*it)()){
			if(ownerUnit->weaponAccessible(p)){
				upgrade = p;
			}
		}
	}


	if(upgrade){
		const WeaponPrm* prm = upgrade->accessibleUpgrade(ownerUnit);

		if(prm)
			upgrade = prm;
	}

	return upgrade;
}

bool WeaponPrm::checkTargetMode(UnitMode owner_mode, UnitMode target_mode) const 
{
	start_timer_auto();

	return (targetUnitMode_[owner_mode] & (1 << target_mode)) != 0;
}

WeaponPrm* WeaponPrm::getWeaponPrmByID(int id)
{
	WeaponPrmLibrary::Strings::const_iterator it;
	FOR_EACH(WeaponPrmLibrary::instance().strings(), it)
		if(it->get() && it->get()->ID() == id)
			return it->get();
	return 0;
}

WeaponPrm* WeaponPrm::defaultPrm()
{
	static WeaponPrm prm;
	prm.setShootingMode(SHOOT_MODE_INTERFACE);
	return &prm;
}

// ------------------------------

AccessBuilding::AccessBuilding()
{
	needConstructed = true;
}

void AccessBuilding::serialize(Archive& ar)
{
	ar.serialize(building, "building", "&Здание");
	ar.serialize(needConstructed, "needConstructed", "&Должно быть построено");
}

bool AccessBuildings::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(static_cast<vector<AccessBuilding>&>(*this), name, "&ИЛИ");
}

// ------------------------------

void WeaponGroupType::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(shootingMode_, "shootingMode", "режим стрельбы в группе");
}

