#include "StdAfx.h"
#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "vmap.h"
#include "Universe.h"

#include "Squad.h"
#include "Serialization\Serialization.h"
#include "Serialization\Dictionary.h"
#include "Conditions.h"

#include "UserInterface\UI_References.h"
#include "UserInterface\UI_Controls.h"
#include "UserInterface\UI_Logic.h"
#include "GameOptions.h"
#include "EventParameters.h"
#include "Units\UnitItemResource.h"
#include "Environment\SourceManager.h"
#include "Environment\Anchor.h"
#include "Serialization\RangedWrapper.h"
#include "Water\SkyObject.h"
#include "CheatManager.h"
#include "Ai\PlaceOperators.h"
#include "ScanningShape.h"
#include "Units\IronBuilding.h"
#include "Water\Water.h"
#include "Serialization\SerializationFactory.h"

string editSignalVariableDialog();

/////////////////////////////////////////////////////
//		Conditions
/////////////////////////////////////////////////////
BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ConditionCheckOutWater, OutWater, "OutWater")
REGISTER_ENUM_ENCLOSED(ConditionCheckOutWater, WATER_IS_WATER, "Вода")
REGISTER_ENUM_ENCLOSED(ConditionCheckOutWater, WATER_IS_ICE, "Лед")
REGISTER_ENUM_ENCLOSED(ConditionCheckOutWater, WATER_IS_LAVA, "Лава")
END_ENUM_DESCRIPTOR_ENCLOSED(ConditionCheckOutWater, OutWater)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ConditionCaptureBuilding, PlayerType, "PlayerType")
REGISTER_ENUM_ENCLOSED(ConditionCaptureBuilding, MY_PLAYER, "Агрессор")
REGISTER_ENUM_ENCLOSED(ConditionCaptureBuilding, ENEMY_PLAYER, "Жертва")
END_ENUM_DESCRIPTOR_ENCLOSED(ConditionCaptureBuilding, PlayerType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ConditionCaptureBuilding, Participators, "Participators")
REGISTER_ENUM_ENCLOSED(ConditionCaptureBuilding, FRIENDS, "Союзники")
REGISTER_ENUM_ENCLOSED(ConditionCaptureBuilding, ENEMIES, "Враги")
REGISTER_ENUM_ENCLOSED(ConditionCaptureBuilding, ALL, "Все")
END_ENUM_DESCRIPTOR_ENCLOSED(ConditionCaptureBuilding, Participators)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ConditionIsMultiplayer, Mode, "Mode")
REGISTER_ENUM_ENCLOSED(ConditionIsMultiplayer, ANY, "Любой режим мультиплеера")
REGISTER_ENUM_ENCLOSED(ConditionIsMultiplayer, LAN, "LAN")
REGISTER_ENUM_ENCLOSED(ConditionIsMultiplayer, ONLINE, "ONLINE")
REGISTER_ENUM_ENCLOSED(ConditionIsMultiplayer, DIRECT, "Direct")
END_ENUM_DESCRIPTOR_ENCLOSED(ConditionIsMultiplayer, Mode)

STARFORCE_API void initConditions()
{
SECUROM_MARKER_HIGH_SECURITY_ON(3);

REGISTER_CLASS(Condition, ConditionSwitcher, "И/ИЛИ");
REGISTER_CLASS(Condition, Condition, "Не выполняется никогда (для выключения триггеров)");

REGISTER_CLASS(Condition, ConditionIsPlayerActive, "Глобальные условия\\Игрок активный")
REGISTER_CLASS(Condition, ConditionEnvironmentTime, "Глобальные условия\\Время суток")
REGISTER_CLASS(Condition, ConditionIsPlayerAI, "Глобальные условия\\АИ ли Игрок")
REGISTER_CLASS(Condition, ConditionMissionSelected, "Глобальные условия\\Миссия выбрана")
REGISTER_CLASS(Condition, ConditionNeedUpdate, "Глобальные условия\\Есть свежее обновление")
REGISTER_CLASS(Condition, ConditionCheckGameType, "Глобальные условия\\Проверка типа игры")
REGISTER_CLASS(Condition, ConditionCheckPause, "Глобальные условия\\Проверка паузы")
REGISTER_CLASS(Condition, ConditionIsMultiplayer, "Глобальные условия\\Мультиплеер")
REGISTER_CLASS(Condition, ConditionPredefineGame, "Глобальные условия\\Режим custom/predefined и командный в текущей миссии")
REGISTER_CLASS(Condition, ConditionCheckRace, "Глобальные условия\\Проверка расы")
REGISTER_CLASS(Condition, ConditionOnlyMyClan, "Глобальные условия\\!!! Устарело !!! Остался только мой клан")
REGISTER_CLASS(Condition, ConditionNoUnitsLeft, "Глобальные условия\\У игрока не осталось дееспособных юнитов")
REGISTER_CLASS(Condition, ConditionDifficultyLevel, "Глобальные условия\\Уровень сложности")
REGISTER_CLASS(Condition, ConditionUserSave, "Глобальные условия\\Игровой сейв")
REGISTER_CLASS(Condition, ConditionCheckDirectControl, "Глобальные условия\\Проверка прямого управления")
REGISTER_CLASS(Condition, ConditionPlayerWin, "Глобальные условия\\Игрок выиграл, потому что все другие кланы проиграли") 
REGISTER_CLASS(Condition, ConditionPlayerDefeat, "Глобальные условия\\Игрок проиграл, потому что другой клан выиграл") 
REGISTER_CLASS(Condition, ConditionPlayerByNumberDefeat, "Глобальные условия\\Игрок по номеру проиграл (выствляется действием игрок проиграл)") 
REGISTER_CLASS(Condition, ConditionCheckInt, "Глобальные условия\\Проверка целочисленной переменной")
REGISTER_CLASS(Condition, ConditionCheckOutWater, "Глобальные условия\\Проверка внешней среды")
REGISTER_CLASS(Condition, ConditionTimeMatched, "Глобальные условия\\Осталось времени меньше, чем указано")
REGISTER_CLASS(Condition, ConditionScreenRatio, "Глобальные условия\\Соотнощение сторон экрана")
REGISTER_CLASS(Condition, ConditionEventString, "Глобальные условия\\Проверка чита")
REGISTER_CLASS(Condition, ConditionWeatherEnabled, "Глобальные условия\\Осадки разрешены")
REGISTER_CLASS(Condition, ConditionUnderWater, "Глобальные условия\\Область в радиусе от якоря под водой")
REGISTER_CLASS(Condition, ConditionAnchorOnScreen, "Глобальные условия\\В поле зрения камеры находится якорь")

REGISTER_CLASS(Condition, ConditionNetworkDisconnect, "Глобальные событийные условия\\Разрыв сети")
REGISTER_CLASS(Condition, ConditionEndReplay, "Глобальные событийные условия\\Реплей завершен")
REGISTER_CLASS(Condition, ConditionEventComing, "Глобальные событийные условия\\Пришло событие")

REGISTER_CLASS(Condition, ConditionContext, "Контекстные условия\\Юнит определенного типа"); 
REGISTER_CLASS(Condition, ConditionObjectOnWater, "Контекстные условия\\Объект в воде(над водой)")
REGISTER_CLASS(Condition, ConditionObjectOnIce, "Контекстные условия\\Объект на льду(не заморожен в нем)")
REGISTER_CLASS(Condition, ConditionObjectDemaged, "Контекстные условия\\Объект поврежден на X процентов")
REGISTER_CLASS(Condition, ConditionDistanceBetweenObjects, "Контекстные условия\\Типы юнитов на расстоянии меньшем от объектов")
REGISTER_CLASS(Condition, ConditionObjectIsNotUnderAttack, "Контекстные условия\\Объект никем не атакован");
REGISTER_CLASS(Condition, ConditionObjectAimed, "Контекстные условия\\Объект собираются атаковать");
REGISTER_CLASS(Condition, ConditionSquadSufficientUnits, "Контекстные условия\\В скваде юнита определенного типа определенное количество юнитов")
REGISTER_CLASS(Condition, ConditionUnitLevel, "Контекстные условия\\Уровень юнита")
REGISTER_CLASS(Condition, ConditionObjectWorking, "Контекстные условия\\Юнит занят");
REGISTER_CLASS(Condition, ConditionMyObjectNearAnchorByLabel, "Контекстные условия\\Возле якоря по метке находится мой юнит")
REGISTER_CLASS(Condition, ConditionCountObjectsInRadius, "Контекстные условия\\Количество юнитов в радиусе от списка типов юнита, меньше указанного количества")
REGISTER_CLASS(Condition, ConditionObjectsInRadiusAimed, "Контекстные условия\\Юниты в радиусе от списка типов юнита, собираются атаковать врага")
REGISTER_CLASS(Condition, ConditionCompareCountObjectsInRadius, "Контекстные условия\\Количество юнитов в радиусе от списка типов юнита, в первой группе меньше чем во второй")
REGISTER_CLASS(Condition, ConditionObjectBuildingInProgress, "Контекстные условия\\Здание строится")
REGISTER_CLASS(Condition, ConditionObjectByLabel, "Контекстные условия\\Юнит по метке")
REGISTER_CLASS(Condition, ConditionUnitInTransport, "Контекстные условия\\Юнит сидит в транспорте")
REGISTER_CLASS(Condition, ConditionCheckUnitsInTransport, "Контекстные условия\\Юниты определенного типа в транспорте в количестве")
REGISTER_CLASS(Condition, ConditionCheckPersonalParameter, "Контекстные условия\\Параметер юнита равен величине")
REGISTER_CLASS(Condition, ConditionCheckParameterProducing, "Контекстные условия\\Юнит производит параметр")
REGISTER_CLASS(Condition, ConditionUnitUnseen, "Контекстные условия\\Юнит невидимый");
REGISTER_CLASS(Condition, ConditionUnitCaptured, "Контекстные условия\\Юнит захваченный мной")
REGISTER_CLASS(Condition, ConditionCheckSurface, "Контекстные условия\\Поверхность под зданием повреждена")
REGISTER_CLASS(Condition, ConditionCheckSurfaceProducedZone, "Контекстные условия\\Зона производимая зданием повреждена")
REGISTER_CLASS(Condition, ConditionBuildingsConnected, "Контекстные условия\\К зданию подключены другие здания (является источником)")
REGISTER_CLASS(Condition, ConditionBuildingsConnectedMinimal, "Контекстные условия\\К зданию подключено минимальное количество зданий")
REGISTER_CLASS(Condition, ConditionCheckSourceNearUnit, "Контекстные условия\\В радиусе юнита находится источник")
REGISTER_CLASS(Condition, ConditionObjectHearNoise, "Контекстные условия\\Юнит слышит юнитов врага")
REGISTER_CLASS(Condition, ConditionObjectUnderShield, "Контекстные условия\\Юнит под полем врага")

REGISTER_CLASS(Condition, ConditionObjectUnderAttack, "Контекстные событийные условия\\Объект атакован")
REGISTER_CLASS(Condition, ConditionUnitSelecting, "Контекстные событийные условия\\Произошел селект юнита или здания")

REGISTER_CLASS(Condition, ConditionObjectsExists, "Объекты\\Объекты существуют")
REGISTER_CLASS(Condition, ConditionCompareObjectsCount, "Объекты\\Количество юнитов в первой группе сравнивается со второй согласно критерию")
REGISTER_CLASS(Condition, ConditionCompareSquadsCount, "Объекты\\Количество сквадов в первой группе сравнивается со второй согласно критерию")
REGISTER_CLASS(Condition, ConditionObjectUnseen, "Объекты\\Процент скрытых предметов на мире");
REGISTER_CLASS(Condition, ConditionObjectPercentOwner, "Объекты\\Процент существующих на мире объектов (общее количество фиксируется)")
REGISTER_CLASS(Condition, ConditionBuildingsConnectedToZone, "Объекты\\Здания подключены");
REGISTER_CLASS(Condition, ConditionUnitProducing, "Объекты\\Юнит производится");
REGISTER_CLASS(Condition, ConditionAllBuildingsConnected, "Объекты\\Все здания из списка типов подключены")

REGISTER_CLASS(Condition, ConditionProducedParameter, "Параметры\\Параметр произведен")
REGISTER_CLASS(Condition, ConditionProducedAllParameters, "Параметры\\Все параметры произведены")
REGISTER_CLASS(Condition, ConditionPlayerParameters, "Параметры\\Значение общего параметра")
REGISTER_CLASS(Condition, ConditionPercentOfPlayerResource, "Параметры\\Процент общего параметра от емкости")
REGISTER_CLASS(Condition, ConditionGetResourceLevel, "Параметры\\Добывается ресурса в секунду")
REGISTER_CLASS(Condition, ConditionPercentOfMaxUnits, "Параметры\\Кол-во юнитов меньше/больше процента от разрешенного количества")

REGISTER_CLASS(Condition, ConditionObjectByLabelExists, "Объект по метке\\Объект по метке существует")
REGISTER_CLASS(Condition, ConditionKillObjectByLabel, "Объект по метке\\Объект по метке уничтожен")
REGISTER_CLASS(Condition, ConditionObjectNearAnchorByLabel, "Объект по метке\\Возле якоря по метке находится объект указанного типа")
REGISTER_CLASS(Condition, ConditionCheckSurfaceNearObjectByLabel, "Объект по метке\\Высота поверхности в радиусе объекта по метке")

REGISTER_CLASS(Condition, ConditionKeyboardClick, "Интерфейс\\Клик по клавиатуре или мыши")
REGISTER_CLASS(Condition, ConditionClickOnButton, "Интерфейс\\Клик по кнопке")
REGISTER_CLASS(Condition, ConditionFocusOnButton, "Интерфейс\\Мышь над кнопкой")
REGISTER_CLASS(Condition, ConditionButtonFocus, "Интерфейс\\Кнопка получила/потеряла фокус")
REGISTER_CLASS(Condition, ConditionUI_ControlState, "Интерфейс\\включено состояние кнопки")
REGISTER_CLASS(Condition, ConditionUI_StringSelected, "Интерфейс\\в списке выбрана строка")
REGISTER_CLASS(Condition, ConditionUI_ProfilesEmpty, "Интерфейс\\Список профайлов пуст")
REGISTER_CLASS(Condition, ConditionUI_ProfileSelected, "Интерфейс\\Профайл выбран")
REGISTER_CLASS(Condition, ConditionNeedCommitSettings, "Интерфейс\\Нужно подтвердить новые настройки")
REGISTER_CLASS(Condition, ConditionSelected, "Интерфейс\\Селект юнита или здания")
REGISTER_CLASS(Condition, ConditionSquadSelected, "Интерфейс\\Селект сквада")
REGISTER_CLASS(Condition, ConditionLastNetStatus, "Интерфейс\\Последний статус работы с сетью")
REGISTER_CLASS(Condition, ConditionUI_NeedDiskOpConfirmation, "Интерфейс\\Требуется подверждение перезаписи или удаления")

REGISTER_CLASS(Condition, ConditionUnableToBuild, "Событийные условия\\АИ не смог построить здание")
REGISTER_CLASS(Condition, ConditionCommandMove, "Событийные условия\\Юниту отдана команда двигаться")
REGISTER_CLASS(Condition, ConditionCommandMoveSquad, "Событийные условия\\Скваду отдана команда двигаться")
REGISTER_CLASS(Condition, ConditionCommandAttack, "Событийные условия\\Юниту отдана команда атаковать")
REGISTER_CLASS_CONVERSION(Condition, ConditionObjectAttacking, "Событийные условия\\Объект атакует", "class ConditionBuildingAttacking")
REGISTER_CLASS(Condition, ConditionObjectByLabelAttacking, "Событийные условия\\Объект по метке атакует")
REGISTER_CLASS(Condition, ConditionCommandAttackSquad, "Событийные условия\\Скваду отдана команда атаковать")
REGISTER_CLASS(Condition, ConditionCaptureBuilding, "Событийные условия\\Юнит захвачен")
REGISTER_CLASS(Condition, ConditionStartUpgrade, "Событийные условия\\Апгрейд начат")
REGISTER_CLASS(Condition, ConditionCompleteUpgrade, "Событийные условия\\Апгрейд завершен")
REGISTER_CLASS(Condition, ConditionSoldBuilding, "Событийные условия\\Здание продано")
REGISTER_CLASS(Condition, ConditionStartProduction, "Событийные условия\\Производство юнита начато")
REGISTER_CLASS(Condition, ConditionStartProductionParameter, "Событийные условия\\Производство параметров юнитом начато")
REGISTER_CLASS(Condition, ConditionStartBuild, "Событийные условия\\Строительство здания началось")
REGISTER_CLASS(Condition, ConditionCompleteBuild, "Событийные условия\\Строительство здания завершилось")
REGISTER_CLASS(Condition, ConditionCompleteCure, "Событийные условия\\Ремонт объекта завершен")
REGISTER_CLASS(Condition, ConditionCheckResource, "Событийные условия\\Нехватка ресурса")
REGISTER_CLASS(Condition, ConditionCreateSource, "Событийные условия\\Источник создан")
REGISTER_CLASS(Condition, ConditionSourceActivated, "Событийные условия\\Источник по метке активирован")
REGISTER_CLASS(Condition, ConditionCreateObject, "Событийные условия\\Объект создан")
REGISTER_CLASS(Condition, ConditionKillObject, "Событийные условия\\Объект уничтожен")
REGISTER_CLASS(Condition, ConditionProduceParameterFinish, "Событийные условия\\Параметер произведен")
REGISTER_CLASS(Condition, ConditionRequestedAssembly, "Событийные условия\\Союзник запросил поддержку")

SECUROM_MARKER_HIGH_SECURITY_OFF(3);
}

/////////////////////////////////////////////////////
BEGIN_ENUM_DESCRIPTOR(ScopeType, "ScopeType")
REGISTER_ENUM(SCOPE_GLOBAL, "Глобальная переменная (сохраняется между миссиями, но не записывается)")
REGISTER_ENUM(SCOPE_PROFILE, "Переменная профиля текущего игрока (сохраняется между миссиями, записывается)")
REGISTER_ENUM(SCOPE_PLAYER, "Переменная текущего игрока (существует в течении миссии, записывается в игровой сейв)")
REGISTER_ENUM(SCOPE_UNIVERSE, "Переменная текущего мира (существует в течении миссии, записывается в игровой сейв)")
REGISTER_ENUM(SCOPE_MISSION_DESCRIPTION, "Переменная описания миссии (0 - 31)")
END_ENUM_DESCRIPTOR(ScopeType)

BEGIN_ENUM_DESCRIPTOR(CompareOperator, "CompareOperator")
REGISTER_ENUM(COMPARE_LESS, "Меньше")
REGISTER_ENUM(COMPARE_LESS_EQ, "Меньше либо равно")
REGISTER_ENUM(COMPARE_EQ, "Равно")
REGISTER_ENUM(COMPARE_NOT_EQ, "Не равно")
REGISTER_ENUM(COMPARE_GREATER, "Больше")
REGISTER_ENUM(COMPARE_GREATER_EQ, "Больше либо равно")
END_ENUM_DESCRIPTOR(CompareOperator)

BEGIN_ENUM_DESCRIPTOR(UnitsConstruction, "UnitsConstruction")
REGISTER_ENUM(CONSTRUCTED, "Построенные")
REGISTER_ENUM(CONSTRUCTING, "Строящиеся")
END_ENUM_DESCRIPTOR(UnitsConstruction)

BEGIN_ENUM_DESCRIPTOR(UnitsTransformation, "UnitsTransformation")
REGISTER_ENUM(UNIT_STATE_CONSTRUCTED, "Построенные")
REGISTER_ENUM(UNIT_STATE_CONSTRUCTING, "Строящиеся")
REGISTER_ENUM(UNIT_STATE_UPGRADING, "Апгрейдищиеся")
END_ENUM_DESCRIPTOR(UnitsTransformation)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ConditionObjectUnderAttack, WeaponType, "WeaponType")
REGISTER_ENUM_ENCLOSED(ConditionObjectUnderAttack, SHORT_RANGE, "Ближний бой")
REGISTER_ENUM_ENCLOSED(ConditionObjectUnderAttack, LONG_RANGE, "Дальний бой")
REGISTER_ENUM_ENCLOSED(ConditionObjectUnderAttack, FROM_INTERFACE, "Из интерфейса")
REGISTER_ENUM_ENCLOSED(ConditionObjectUnderAttack, ANY_TYPE, "Любой тип")
END_ENUM_DESCRIPTOR_ENCLOSED(ConditionObjectUnderAttack, WeaponType)

BEGIN_ENUM_DESCRIPTOR(Activity, "Activity")
REGISTER_ENUM(ACTIVITY_PRODUCING, "Производит что-то")
REGISTER_ENUM(ACTIVITY_CONSTRUCTING, "Строится сам")
REGISTER_ENUM(ACTIVITY_BUILDING, "Cтроит что-то")
REGISTER_ENUM(ACTIVITY_MOVING, "Двигается куда-то")
REGISTER_ENUM(ACTIVITY_UPGRADING, "Апгрейдится")
REGISTER_ENUM(ACTIVITY_PICKING_RESOURCE, "Собирает ресурс")
REGISTER_ENUM(ACTIVITY_TELEPORTATING, "Телепортируется")
REGISTER_ENUM(ACTIVITY_ATTACKING, "Атакует")
REGISTER_ENUM(ACTIVITY_MOVING_TO_TRANSPORT, "Садится в транспорт")
REGISTER_ENUM(ACTIVITY_WAITING_FOR_PASSENGER, "Ожидает пассажира")
END_ENUM_DESCRIPTOR(Activity)

/////////////////////////////////////////////////////

ConditionContext::ConditionContext()
{
	ignoreContext_ = false;
	squadFilter_ = false;
	ignoreContextCounter_ = 0;
}

void ConditionCheckSurfaceNearObjectByLabel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "Метка якоря");
	ar.serialize(radius_, "radius", "Радиус");
	ar.serialize(compareOperator_, "compareOperator", "Оператор сравнения");
	ar.serialize(height_, "height", "Высота");
}

ConditionCheckSurfaceNearObjectByLabel::ConditionCheckSurfaceNearObjectByLabel()
{
	radius_ = 50;
	compareOperator_ = COMPARE_GREATER_EQ;
	height_ = 0;
}

bool ConditionCheckSurfaceNearObjectByLabel::check() const
{
	if(!anchor_){
		xassertStr(0 && "Якорь по метке не найден: ", anchor_.c_str());
		return false;
	}												
	else{
		Vect2f position = anchor_->position2D();
		ScanGroundHeightLineOp line_op(compareOperator_, height_);

		ScanningShape shape;
		shape.setCircle(radius_ >> kmGrid);
		ScanningShape::const_iterator i;
		int y = shape.rect().y + (position.yi() >> kmGrid);
		int xCenter = position.xi() >> kmGrid;
		FOR_EACH(shape.intervals(), i){
			line_op(xCenter + (*i).xl, xCenter + (*i).xr, y);
			y++;
			if(!line_op.valid())
				return false;
		}

		return line_op.valid();
	}
}

bool ConditionWeatherEnabled::check() const
{
	return GameOptions::instance().getBool(OPTION_WEATHER);
}

bool ConditionObjectBuildingInProgress::check(UnitActing* unit) const
{
	if(!unit->attr().isBuilding())
		return false;

	UnitBuilding* building = safe_cast<UnitBuilding*>(unit);

	return !building->isConstructed() && building->constructor();
}

ConditionCheckUnitsInTransport::ConditionCheckUnitsInTransport()
{
	count_ = 1;
	compareOperator_ = COMPARE_GREATER_EQ;
}

void ConditionCheckUnitsInTransport::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(units_, "units", "Типы юнитов(в транспорте)");
	removeZeros(units_);
	ar.serialize(compareOperator_, "compareOperator_", "Критерий сравнения");
    ar.serialize(count_, "count_", "Общее количество");
}

bool ConditionCheckUnitsInTransport::check(UnitActing* unit) const
{
	if(!unit->attr().isTransport())
		return false;

	int counter = 0;
	LegionariesLinks::const_iterator li;
	FOR_EACH(unit->transportSlots(), li){
		if(!units_.empty() && units_.front()){
			AttributeUnitReferences::const_iterator it;
			FOR_EACH(units_, it)
				if((*li) && &(*li)->attr() == *it && (*li)->inTransport())
					++counter;
		}
		else{
			if((*li) && (*li)->inTransport())
				++counter;
		}
	}

	return compare(counter, count_, compareOperator_); 
}

bool ConditionObjectUnseen::check() const
{
	const RealUnits& items = universe()->worldPlayer()->realUnits(attrItem);
	RealUnits::const_iterator i;
	int counter = 0;
	FOR_EACH(items, i)
		if((*i)->attr().isResourceItem() && &(*i)->attr() == attrItem && safe_cast<const UnitItemResource*>(*i)->isUnseen())
			++counter;

	return compare(counter, round(percent*0.01*universe()->worldPlayer()->countUnits(attrItem)), compareOperator);
}

bool ConditionUnitUnseen::check(UnitActing* unit) const
{
	return unit->isUnseen();
}

ConditionCheckSurface::ConditionCheckSurface()
{
	deviationCosMin_ = 0.96f;
}

bool ConditionCheckSurface::check(UnitActing* unit) const
{
	if(!unit->attr().isBuilding())
		return false;

	if(abs(1 - unit->rigidBody()->cosTheta()) > (1 - deviationCosMin_))
		return true;

	return false;
}

bool ConditionBuildingsConnected::check(UnitActing* unit) const
{
	if(!unit->attr().isBuilding())
		return false;

	return safe_cast<UnitBuilding*>(unit)->connectionTargets();
}

ConditionBuildingsConnectedMinimal::ConditionBuildingsConnectedMinimal()
{
	rounds_ = 0;
	connectionsMin_ = 100;
}

void ConditionBuildingsConnectedMinimal::clearContext()
{
	rounds_++;
}

bool ConditionBuildingsConnectedMinimal::check(UnitActing* unit) const
{
	if(!unit->attr().isBuilding())
		return false;

	int connections = safe_cast<UnitBuilding*>(unit)->connectionTargets();
	if(!connections)
		return true;

	if(rounds_ > 1 && connections == connectionsMin_){
		rounds_ = 0;
		connectionsMin_ = 100;
		return true;
	}

	connectionsMin_ = min(connectionsMin_, connections);
	return false;
}

void ConditionCheckSurface::serialize(Archive& ar)
{
	__super::serialize(ar);
	int angle = round(R2G(acosf(deviationCosMin_)));
	ar.serialize(angle, "angleMax", "Максимальное отклонение, градусы");
	deviationCosMin_ = cosf(G2R(angle));
}

ConditionCheckSurfaceProducedZone::ConditionCheckSurfaceProducedZone()
{
	percent_ = 50;
}

void ConditionCheckSurfaceProducedZone::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(percent_, "percent", "Процент разрушения");
}

bool ConditionCheckSurfaceProducedZone::check(UnitActing* unit) const
{
	if(!unit->attr().isBuilding())
		return false;

	ScanGroundDamagedLineOp line_op;
	ScanningShape shape;
	shape.setCircle(round(unit->attr().producedPlacementZoneRadius) >> kmGrid);
	ScanningShape::const_iterator i;
	int y = shape.rect().y + (unit->position2D().yi() >> kmGrid);
	int xCenter = unit->position2D().xi() >> kmGrid;
	FOR_EACH(shape.intervals(), i){
		line_op(xCenter + (*i).xl, xCenter + (*i).xr, y);
		y++;
	}

	if(line_op.sigma2() > 0.1f*percent_/100.f)
		return true;
	
	return false;
}

bool ConditionUnderWater::check() const
{
	if(!anchor_){
		xassertStr(0 && "Метка не найдена", anchor_.c_str());
		return false;
	}
	if(onlyDeep_)
		return environment->water()->isUnderWater(anchor_->position(), radius_);
	else
		return environment->water()->isWater(anchor_->position(), radius_);
}

ConditionUnderWater::ConditionUnderWater()
{
	radius_ = 20;
	onlyDeep_ = false;
}

void ConditionUnderWater::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "Метка якоря");
	ar.serialize(radius_, "radius", "Радиус");
	ar.serialize(onlyDeep_, "onlyDeep", "Учитывать только глубокую воду");
}

void ConditionObjectUnseen::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrItem, "attrItem", "Тип предмета");
	ar.serialize(compareOperator, "compareOperator", "Условие сравнения");
	ar.serialize(percent, "percent", "Процент");
}

ConditionCheckResource::ConditionCheckResource() 
{ 
	requestResourceType = RequestResourceType(-1);
}

void ConditionCheckResource::checkEvent(const Event& event)
{
	if(event.type() == Event::LACK_OF_RESOURCE){
		const EventParameters& eventParameter = safe_cast_ref<const EventParameters&>(event);
		if(eventParameter.requestResourceType() & requestResourceType && eventParameter.parameters().findByIndex(parameterType.key()) > FLT_EPS) 
			setCheck();
	}
}

bool ConditionUnitInTransport::check(UnitActing* unit) const
{
	return unit->attr().isLegionary() && safe_cast<UnitLegionary*>(unit)->inTransport();
}

void ConditionCheckResource::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(parameterType, "parameterType", "Тип ресурса");
	ar.serialize(requestResourceType, "requestResourceType", "Причина нехватки");
}

bool ConditionPercentOfMaxUnits::check() const
{
	return compare(aiPlayer().unitNumber(type_), round(percent*0.01*aiPlayer().unitNumberMax(type_)), compareOperator);
}

void ConditionPercentOfMaxUnits::serialize(Archive &ar)
{
	__super::serialize(ar);
	ar.serialize(type_, "|type|typeRef", "Тип максимального количества юнитов");
	ar.serialize(compareOperator, "compareOperator", "&Условие сравнения");
	ar.serialize(percent, "percent", "&Процент от максимального кол-ва");
}

ConditionObjectDemaged::ConditionObjectDemaged()
{
	percent_ = 10;
}

bool ConditionObjectDemaged::check(UnitActing* unit) const
{
	return unit->parameters().contain(parameterType_) &&
		unit->parameters().findByIndex(parameterType_.key()) < unit->parametersMax().findByIndex(parameterType_.key()) * (1.f - percent_ * 0.01f); 
}

void ConditionObjectDemaged::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(parameterType_, "parameterType", "Тип параметра");
	ar.serialize(percent_, "percent", "Процент");
}

ConditionPlayerWin::ConditionPlayerWin()
{
	checkAuxPlayers = false;
}

bool ConditionPlayerWin::check() const
{
	bool theresEnemy = false;
	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi)
		if(!(*pi)->isWorld() && (checkAuxPlayers || (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER))
			if((*pi)->clan() == aiPlayer().clan()){
				if((*pi)->isWin())
					return true;
				//if((*pi)->isDefeat())
                //    return false;
			}
			else{
				theresEnemy = true;
				if(!(*pi)->isDefeat() || (*pi)->isWin())
                    return false;
			}
	return theresEnemy;			
}

void ConditionPlayerWin::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(checkAuxPlayers, "checkAuxPlayers", "Учитывать служебных игроков");
}

bool ConditionPlayerDefeat::check() const
{
	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi)
		if(!(*pi)->isWorld() && (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER
		  && (*pi)->clan() != aiPlayer().clan() && (*pi)->isWin())
				return true;
	return false;
}

ConditionObjectPercentOwner::ConditionObjectPercentOwner() 
{
	percent = 0;
	compareOperator = COMPARE_GREATER;
	countFriends = false;
	totalMax = 0;
}

void ConditionObjectPercentOwner::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrSet, "attrSet", "Множество типов");
	removeZeros(attrSet);
	ar.serialize(compareOperator, "compareOperator", "Критерий сравнения");
	ar.serialize(percent, "percent", "Процент объектов");
	ar.serialize(countFriends, "countFriends", "Считать союзников");
}

bool ConditionObjectPercentOwner::check() const
{
	if(attrSet.empty()) 
		return false;

	int part = 0;
	int total = 0;
	PlayerVect::iterator pi;
	AttributeUnitOrBuildingReferences::const_iterator i;
	FOR_EACH(universe()->Players, pi){
		int sum = 0;
		FOR_EACH(attrSet,i)
			sum += (*pi)->countUnits(*i); 
		total += sum;
		if(*pi == &aiPlayer() || countFriends && (*pi)->clan() == aiPlayer().clan())
			part += sum;
	}

	totalMax = max(total, totalMax);
	return compare(total ? round(part * 100.f / totalMax) : 0, percent, compareOperator); 
}

void ConditionCreateObject::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы объектов");
	removeZeros(objects_);
	ar.serialize(playerType, "playerType", "&Игрок");
}

void ConditionUnableToBuild::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы зданий");
	removeZeros(objects_);
}

void ConditionKillObject::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы объектов");
	removeZeros(objects_);
	ar.serialize(playerType, "playerType", "&Игрок");
}
ConditionBuildingsConnectedToZone::ConditionBuildingsConnectedToZone()
{
	counter = 1; 
	playerType = AI_PLAYER_TYPE_ME;
	compareOperator = COMPARE_GREATER_EQ;
}

bool ConditionObjectUnderShield::check(UnitActing* unit) const
{
	ShieldScanOp shieldOp(Vect3f(unit->position2D(), 0.f), &aiPlayer());
	sourceManager->sourceGrid.Scan(unit->position2D(), 1, shieldOp);
	return shieldOp.underShield();
}

void ConditionAllBuildingsConnected::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects, "objects", "&Объекты");
	removeZeros(objects);
}

bool ConditionAllBuildingsConnected::check() const
{
	if(objects.empty())
	{
		const RealUnits& units = aiPlayer().realUnits();
		RealUnits::const_iterator ui;
		FOR_EACH(units, ui)
			if((*ui)->alive() && (*ui)->attr().isBuilding() && !(*ui)->isConnected())
				return false;

		return true;
	}

	AttributeBuildingReferences::const_iterator i;
	FOR_EACH(objects, i){
		const RealUnits& list = aiPlayer().realUnits(*i);
		RealUnits::const_iterator ui;
		FOR_EACH(list, ui)
			if((*ui)->alive() && !(*ui)->isConnected())
				return false;
	}

	return true;

}

void ConditionBuildingsConnectedToZone::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects, "objects", "&Объекты");
	removeZeros(objects);
	ar.serialize(counter, "counter", "&Количество общее");
	ar.serialize(compareOperator, "compareOperator", "Критерий сравнения");
	ar.serialize(playerType, "playerType", "&Владелец объектов");
}

int ConditionBuildingsConnectedToZone::countBuildingsConnected(const Player* player, const AttributeBase* building) const
{
	int count = 0;
	const RealUnits& list = player->realUnits(building);
	RealUnits::const_iterator ui;
	FOR_EACH(list, ui)
		if((*ui)->alive() && (*ui)->isConnected())
			++count;
	return count;
}

bool ConditionBuildingsConnectedToZone::check() const 
{ 
	if(objects.empty()) 
		return false;

	AttributeBuildingReferences::const_iterator i;
	switch(playerType){
			case AI_PLAYER_TYPE_ME:{
				int total = 0;
				FOR_EACH(objects,i)
					total += countBuildingsConnected(&aiPlayer(), *i);
				return compare(total, counter, compareOperator);
								   }
			case AI_PLAYER_TYPE_ENEMY:{
				PlayerVect::iterator pi;
				int total = 0;
				FOR_EACH(objects,i)
					FOR_EACH(universe()->Players, pi)
					if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
						total += countBuildingsConnected(*pi, *i); 
				return compare(total, counter, compareOperator);
									  }

			case AI_PLAYER_TYPE_WORLD:{
				int total = 0;
				FOR_EACH(objects,i)
					total += countBuildingsConnected(universe()->worldPlayer(), *i);
				return compare(total, counter, compareOperator);
									  }

			case AI_PLAYER_TYPE_ANY:{
				PlayerVect::iterator pi;
				int total = 0;
				FOR_EACH(objects,i)
					FOR_EACH(universe()->Players, pi)
					total += countBuildingsConnected(*pi, *i); 
				return compare(total, counter, compareOperator);
			}
	}

	return false;
}

ConditionObjectsExists::ConditionObjectsExists() 
{
	counter = 1; 
	playerType = AI_PLAYER_TYPE_ME;
	constructedAndConstructing = CONSTRUCTED | CONSTRUCTING; 
	compareOperator = COMPARE_GREATER_EQ;
}

void ConditionObjectsExists::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(objects, "objects", "&Объекты");
	removeZeros(objects);
	ar.serialize(counter, "counter", "&Количество общее");
	ar.serialize(compareOperator, "compareOperator", "Критерий сравнения");
	ar.serialize(playerType, "playerType", "&Владелец объектов");
	ar.serialize(constructedAndConstructing, "constructedAndConstructing", "Построенные или недостроенные");
}

void ConditionCaptureBuilding::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(playerType_, "playerType", "Текущий игрок");
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
	ar.serialize(participators_, "participators", "Участники захвата"); 
}

void ConditionProduceParameterFinish::serialize(Archive& ar) 
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit())
		signalVariable.setComboList(editSignalVariableDialog().c_str());
	ar.serialize(signalVariable, "signalVariable", "&Имя сигнальной переменной параметра");
}

void ConditionProducedParameter::serialize(Archive& ar) 
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit())
		signalVariable_.setComboList(editSignalVariableDialog().c_str());
	ar.serialize(signalVariable_, "signalVariable", "&Имя сигнальной переменной");
	ar.serialize(playerScanner_, "playerScanner", "Игрок");
}

void ConditionProduceParameterFinish::checkEvent(const Event& event)
{
	if(event.type() == Event::PRODUCED_PARAMETER){
		const EventParameter& eventParameter = safe_cast_ref<const EventParameter&>(event);
		if(eventParameter.signalVariable() == signalVariable.value())
			setCheck();
	}
}

bool ConditionProducedParameter::check() const 
{ 
	do {
		if(playerScanner_.player(aiPlayer()).checkUniqueParameter(signalVariable_, 1))
			return true;
	} while(playerScanner_.next(aiPlayer()));
	
	return false;
}

void ConditionObjectByLabelExists::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "&Метка объекта");
}

void ConditionKillObjectByLabel::serialize(Archive& ar) 
{ 
	__super::serialize(ar);
	ar.serialize(label_, "label", "&Метка объекта");
	ar.serialize(playerType, "playerType", "&Игрок");
}

void ConditionTimeMatched::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(time, "time", "&Время, секунды");
}

void ConditionClickOnButton::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(control_, "control", "&Кнопка");
	ar.serialize(events_, "events", "Тип клика");
	ar.serialize(modifiers_, "modifiers", "Модификатор");
	ar.serialize(onlyEnabled_, "onlyEnabled", "Только по разрешенной");
	ar.serialize(ignorePause_, "ignorePause", "Работать во время паузы");
	if(ar.isInput())
		controlPtr_ = 0;
}

bool ConditionFocusOnButton::checkStatus() const
{
	if(!controlPtr_) controlPtr_ = control_.control();

	dassert(controlPtr_ == control_.control());

	return UI_LogicDispatcher::instance().hoverControl() == controlPtr_;
}

void ConditionFocusOnButton::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(control_, "control", "&Кнопка");
	if(ar.isInput())
		controlPtr_ = 0;
}

void ConditionButtonFocus::checkEvent(const Event& event) 
{
	if(!controlPtr_) controlPtr_ = control_.control();

	dassert(controlPtr_ == control_.control());

	if(event.type() == Event::UI_BUTTON_FOCUS_ON || event.type() == Event::UI_BUTTON_FOCUS_OFF){
		const EventButton& eventControl = safe_cast_ref<const EventButton&>(event);
		if(eventControl.control() == controlPtr_ && eventControl.checkPlayerID(aiPlayer().playerID()) &&
			(event.type() == Event::UI_BUTTON_FOCUS_ON && onFocus_ || event.type() == Event::UI_BUTTON_FOCUS_OFF && !onFocus_))
				setCheck();
	}
}

void ConditionButtonFocus::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(control_, "control", "&Кнопка");
	ar.serialize(onFocus_, "onFocus", "&При наведении");
	if(ar.isInput())
		controlPtr_ = 0;
}

void ConditionLastNetStatus::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(status_, "status", "Статус");
}

MissionDescriptionForTrigger::MissionDescriptionForTrigger()
: useLoadedMission_(false)
{}

void MissionDescriptionForTrigger::serialize(Archive& ar)
{
	ar.serialize(useLoadedMission_, "useLoadedMission", "Использовать загруженную миссию");
}

ConditionPredefineGame::ConditionPredefineGame() 
: scenarioGameType_(SCENARIO_GAME_TYPE_PREDEFINE)
, teamGameType_(TEAM_GAME_TYPE_INDIVIDUAL)
{
}

void ConditionPredefineGame::serialize(Archive& ar)
{
	__super::serialize(ar);
	missionDescription.serialize(ar);
	ar.serialize(scenarioGameType_, "predefineGameType", "Тип настроек игры");
	ar.serialize(teamGameType_, "teamGameType", "Тип командной игры");
}

bool ConditionPredefineGame::check() const
{
	MissionDescription* mission = missionDescription();

	if(mission){
		int checks = 2;
		
		if(scenarioGameType_ == SCENARIO_GAME_TYPE_ANY || mission->useMapSettings() == (scenarioGameType_ == SCENARIO_GAME_TYPE_PREDEFINE))
			--checks;
		
		if(teamGameType_ == TEAM_GAME_TYPE_ANY)
			--checks;
		else if((teamGameType_ == TEAM_GAME_TYPE_TEEM) == (mission->gameType() == GAME_TYPE_MULTIPLAYER_COOPERATIVE))
			--checks;
		
		return !checks;
	}
	
	return false;
}

bool ConditionUI_ControlState::checkStatus() const
{
	if(!controlPtr_) controlPtr_ = control_.control();

	dassert(controlPtr_ == control_.control());

	return (controlPtr_ && controlPtr_->currentStateIndex() == state_);
}

void ConditionUI_ControlState::serialize(Archive& ar) 
{
	__super::serialize(ar);
	
	ar.serialize(control_, "control", "&Кнопка");
	ar.serialize(state_, "state", "&Состояние");
	if(ar.isInput())
		controlPtr_ = 0;
}

bool ConditionUI_StringSelected::check() const
{
	if(!controlPtr_) controlPtr_ = control_.control();

	dassert(controlPtr_ == control_.control());

	if(const UI_ControlStringList* lp = dynamic_cast<const UI_ControlStringList*>(controlPtr_)){
		if(anyStringSelected_){
			if(lp->selectedString())
				return true;
		}
		else {
			if(lp->selectedStringIndex() == selectedStringIndex_)
				return true;
		}
	}

	return false;
}

void ConditionUI_StringSelected::serialize(Archive& ar) 
{
	__super::serialize(ar);
	
	ar.serialize(control_, "control", "&Кнопка");
	ar.serialize(anyStringSelected_, "anyStringSelected", "Любая строка выбрана");
	if(!anyStringSelected_)
		ar.serialize(selectedStringIndex_, "selectedStringIndex", "Индекс выбранной строки");
	if(ar.isInput())
		controlPtr_ = 0;
}

bool ConditionUI_ProfilesEmpty::check() const
{
	return UI_LogicDispatcher::instance().profileSystem().profilesVector().empty();
}

bool ConditionUI_ProfileSelected::check() const
{
	return UI_LogicDispatcher::instance().profileSystem().currentProfileIndex() >= 0;
}

bool ConditionUI_NeedDiskOpConfirmation::check() const
{
	return (UI_LogicDispatcher::instance().diskOpID() != UI_DISK_OP_NONE && 
		!UI_LogicDispatcher::instance().diskOpConfirmed());
}

bool ConditionNeedCommitSettings::check() const
{
	return GameOptions::instance().needCommit();
}

void ConditionDifficultyLevel::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(difficulty, "difficulty", "&Уровень");
}

//---------------------------------------
bool Condition::compare(int op1, int op2, CompareOperator op)
{
	switch(op){
		case COMPARE_LESS:
			return op1 < op2;
		case COMPARE_LESS_EQ:
			return op1 <= op2;
		case COMPARE_EQ:
			return op1 == op2;
		case COMPARE_NOT_EQ:
			return op1 != op2;
		case COMPARE_GREATER:
			return op1 > op2;
		case COMPARE_GREATER_EQ:
			return op1 >= op2;
		default: 
			xassert(0);
			return false;
	}
}

bool Condition::compare(float op1, float op2, CompareOperator op, float eps)
{
	switch(op){
		case COMPARE_LESS:
			return op1 < op2;
		case COMPARE_LESS_EQ:
			return op1 < op2 + eps;
		case COMPARE_EQ:
			return fabsf(op1 - op2) < eps;
		case COMPARE_NOT_EQ:
			return fabsf(op1 - op2) > eps;
		case COMPARE_GREATER:
			return op1 > op2;
		case COMPARE_GREATER_EQ:
			return op1 > op2 - eps;
		default: 
			xassert(0);
			return false;
	}
}

bool Condition::checkPlayer(const class Player* player1, const Player* player2, AIPlayerType playerType)
{
	switch(playerType){
	case AI_PLAYER_TYPE_ME:
		return player1 == player2;
	case AI_PLAYER_TYPE_ENEMY:
		return player1->isEnemy(player2) && !player2->isWorld();
	case AI_PLAYER_TYPE_WORLD:
		return player2->isWorld();
	case AI_PLAYER_TYPE_ANY:
		return true;
	default:
		xassert(0);
		return false;
	}
}

//---------------------------------------
bool ConditionIsPlayerAI::check() const 
{ 
	return aiPlayer().isAI(); 
}

bool ConditionIsPlayerActive::check() const 
{ 
	return aiPlayer().active(); 
}

bool ConditionCheckRace::check() const 
{ 
	return aiPlayer().race() == race; 
}

void ConditionCheckRace::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(race, "race", "&Раса");
}

bool ConditionObjectsExists::check() const 
{ 
	if(objects.empty()) 
		return false;
	AttributeReferences::const_iterator i;
	if((constructedAndConstructing & CONSTRUCTED) && (constructedAndConstructing & CONSTRUCTING)){
		switch(playerType){
		case AI_PLAYER_TYPE_ME:{
			int total = 0;
			FOR_EACH(objects,i)
				total += aiPlayer().countUnits(*i);
			return compare(total, counter, compareOperator);
		}
		case AI_PLAYER_TYPE_ENEMY:{
			PlayerVect::iterator pi;
			int total = 0;
			FOR_EACH(objects,i)
				FOR_EACH(universe()->Players, pi)
					if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
						total += (*pi)->countUnits(*i); 
			return compare(total, counter, compareOperator);
			}
		
		case AI_PLAYER_TYPE_WORLD:{
			int total = 0;
			FOR_EACH(objects,i)
				total += universe()->worldPlayer()->countUnits(*i);
			return compare(total, counter, compareOperator);
		}
		
		case AI_PLAYER_TYPE_ANY:{
			PlayerVect::iterator pi;
			int total = 0;
			FOR_EACH(objects,i)
				FOR_EACH(universe()->Players, pi)
					total += (*pi)->countUnits(*i); 
			return compare(total, counter, compareOperator);
		}
		}
	}
	else{
		if (constructedAndConstructing & CONSTRUCTED) {
			switch(playerType){
			case AI_PLAYER_TYPE_ME:{
				int total = 0;
				FOR_EACH(objects,i)
					total += aiPlayer().countUnitsConstructed(*i);
				return compare(total, counter, compareOperator);
			}
			case AI_PLAYER_TYPE_ENEMY:{
				PlayerVect::iterator pi;
				int total = 0;
				FOR_EACH(objects,i)
					FOR_EACH(universe()->Players, pi)
						if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
							total += (*pi)->countUnitsConstructed(*i); 
				return compare(total, counter, compareOperator);
				}
			
			case AI_PLAYER_TYPE_WORLD:{
				int total = 0;
				FOR_EACH(objects,i)
					total += universe()->worldPlayer()->countUnitsConstructed(*i);
				return compare(total, counter, compareOperator);
			}
			
			case AI_PLAYER_TYPE_ANY:{
				PlayerVect::iterator pi;
				int total = 0;
				FOR_EACH(objects,i)
					FOR_EACH(universe()->Players, pi)
						total += (*pi)->countUnitsConstructed(*i); 
				return compare(total, counter, compareOperator);
			}
			}
		}
		else 
			if (constructedAndConstructing & CONSTRUCTING) {	
				switch(playerType){
				case AI_PLAYER_TYPE_ME:{
					int total = 0;
					FOR_EACH(objects,i)
						total += aiPlayer().countUnits(*i) - aiPlayer().countUnitsConstructed(*i);
					return compare(total, counter, compareOperator);
				}
				case AI_PLAYER_TYPE_ENEMY:{
					PlayerVect::iterator pi;
					int total = 0;
					FOR_EACH(objects,i)
						FOR_EACH(universe()->Players, pi)
							if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
								total += (*pi)->countUnits(*i) - (*pi)->countUnitsConstructed(*i); 
					return compare(total, counter, compareOperator);
					}
				
				case AI_PLAYER_TYPE_WORLD:{
					int total = 0;
					FOR_EACH(objects,i)
						total += universe()->worldPlayer()->countUnits(*i) - universe()->worldPlayer()->countUnitsConstructed(*i);
					return compare(total, counter, compareOperator);
				}
				
				case AI_PLAYER_TYPE_ANY:{
					PlayerVect::iterator pi;
					int total = 0;
					FOR_EACH(objects,i)
						FOR_EACH(universe()->Players, pi)
							total += (*pi)->countUnits(*i) - (*pi)->countUnitsConstructed(*i); 
					return compare(total, counter, compareOperator);
				}
				}
			}
	}

	return false;
}

ConditionCheckSourceNearUnit::ConditionCheckSourceNearUnit()
{
	radius_ = 100;
}

void ConditionCheckSourceNearUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(sources_, "sources", "Типы источников");
	ar.serialize(radius_, "radius", "Радиус");
}

bool ConditionCheckSourceNearUnit::check(UnitActing* unit) const
{
	found_ = false;
	sourceManager->sourceGrid.Scan(unit->position2D(), radius_, *this);
	return found_;
}

void ConditionCheckSourceNearUnit::operator()(SourceBase* source) const
{
	if(found_)
		return;

	SourceReferences::const_iterator si;
	FOR_EACH(sources_, si)
		if(!strcmp(source->libraryKey(), (*si)->libraryKey())){
			found_ = true;
			return;
		}
}

void ConditionSourceActivated::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(source_, "source", "Источник");
}

void ConditionSourceActivated::checkEvent(const Event& event)
{
	if(event.type() == Event::ACTIVATE_SOURCE){
		const EventSourceBase& eventSource = safe_cast_ref<const EventSourceBase&>(event);
		if(source_ == eventSource.source())
			setCheck();
	}
}

void ConditionCreateSource::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(sourceref, "sourceref", "Тип источника");
}

void ConditionCreateSource::checkEvent(const Event& event)
{
	if(event.type() == Event::CREATE_SOURCE){
		const EventSource& eventSource = safe_cast_ref<const EventSource&>(event);
		if(sourceref == eventSource.attrSource()->attr())
			setCheck();
	}
}

void ConditionUnitSelecting::checkEvent(const Event& event)
{
	if(event.type() == Event::SELECT_UNIT){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		if(!eventUnit.unit()->attr().isActing())
			return;
		const UnitActing* unit_ = safe_cast<const UnitActing*>(eventUnit.unit());

		if(!checkUnit(const_cast<UnitActing*>(safe_cast<const UnitActing*>(unit_))))
			return;

		if(unit_->isUpgrading() && !(unitState_ & UNIT_STATE_UPGRADING))
				return;

		if(unit_->attr().isBuilding()){
			const UnitBuilding* unit = safe_cast<const UnitBuilding*>(unit_);	
			if(unit->isConstructed() && !(unitState_ & UNIT_STATE_CONSTRUCTED))
				return;
			if(!unit->isConstructed() && !(unitState_ & UNIT_STATE_CONSTRUCTING))
				return;
		}

		setEventContextUnit(const_cast<UnitActing*>(safe_cast<const UnitActing*>(unit_)));
	}
}

void ConditionUnitSelecting::serialize(Archive& ar)
{
	__super::serialize(ar);

	ConstructionState constructedAndConstructing; 
	if(ar.isInput() && ar.serialize(constructedAndConstructing, "constructedAndConstructing", 0)) // CONVERSION 24.10.06
	{
		if((constructedAndConstructing & CONSTRUCTED))
			unitState_ |= UNIT_STATE_CONSTRUCTED;
		else
			unitState_ &= ~UNIT_STATE_CONSTRUCTED;

		if((constructedAndConstructing & CONSTRUCTING))
			unitState_ |= UNIT_STATE_CONSTRUCTING;
		else
			unitState_ &= ~UNIT_STATE_CONSTRUCTING;

		unitState_ |= UNIT_STATE_UPGRADING;
	}
	else
		ar.serialize(unitState_, "unitState", "Состояние юнита");
}

void ConditionCreateObject::checkEvent(const Event& event) 
{
	if(event.type() == Event::CREATE_OBJECT || event.type() == Event::PRODUCE_UNIT_IN_SQUAD){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const AttributeBase& attr = eventUnit.unit()->attr();
		AttributeReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(*oi == &attr && checkPlayer(&aiPlayer(), eventUnit.player(), playerType)){
				setCheck();
				break;
			}
	}
}

ConditionEnvironmentTime::ConditionEnvironmentTime()
{
	compareOperator_ = COMPARE_GREATER_EQ;
	time_ = 0.f;
}

bool ConditionEnvironmentTime::check() const
{
	float curTime = environment->environmentTime()->GetTime();

	return compare(curTime, time_, compareOperator_, 0.083f); // 0.083f - соответствует 5-ти минутам
}

void ConditionEnvironmentTime::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(compareOperator_, "compareOperator", "Оператор сравнения");
	ar.serialize(time_, "time", "Время");
}

void ConditionUnableToBuild::checkEvent(const Event& event)
{
	if(event.type() == Event::UNABLE_BUILD){
		const EventUnitAttributePlayer& eventUnit = safe_cast_ref<const EventUnitAttributePlayer&>(event);
		const AttributeBase& attr = eventUnit.unitAttr();
		AttributeBuildingReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}
}

void ConditionKillObject::checkEvent(const Event& event) 
{
	if(event.type() == Event::KILL_OBJECT){
		const EventUnitMyUnitEnemy& eventUnit = safe_cast_ref<const EventUnitMyUnitEnemy&>(event);
		const UnitBase* unit_ = eventUnit.unitMy();
		const AttributeBase& attr = unit_->attr();
		if(attr.excludeFromAutoAttack)
			return;
		AttributeReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi && checkPlayer(&aiPlayer(), unit_->player(), playerType)){
				setCheck();
				break;
			}
	}
}

void ConditionStartBuild::checkEvent(const Event& event)
{
	if(event.type() == Event::STARTED_BUILDING){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const AttributeBase& attr = eventUnit.unit()->attr();
		AttributeBuildingReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}
}

void ConditionCommandMove::checkEvent(const Event& event)
{
	if(event.type() == Event::COMMAND_MOVE){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const AttributeBase& attr = eventUnit.unit()->attr();;
		AttributeUnitReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}
}

bool ConditionUnitProducing::check() const
{
	do{
		const RealUnits& units = playerScanner_.player(aiPlayer()).realUnits();
		RealUnits::const_iterator i;
		FOR_EACH(units, i){
			const AttributeBase* prod = safe_cast<const UnitActing*>(*i)->producedUnit();
			if(prod){
				AttributeUnitReferences::const_iterator oi;
				FOR_EACH(objects_, oi)	
					if(prod == *oi)
						return true;
			}
		}
	} while(playerScanner_.next(aiPlayer()));

	return false;
}

void ConditionUnitProducing::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_,"objects", "Типы юнитов");
	removeZeros(objects_);
	ar.serialize(playerScanner_, "playerScanner", "Игрок");
}

void ConditionCommandMoveSquad::checkEvent(const Event& event)
{
	if(event.type() == Event::COMMAND_MOVE){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const UnitBase* unit_ = eventUnit.unit();
		if(unit_->attr().isLegionary() && &safe_cast<const UnitLegionary*>(unit_)->squad()->attr() == attr)
			setCheck();
	}
}
void ConditionRequestedAssembly::checkEvent(const Event& event)
{
	if(event.type() == Event::REQUEST_ASSEMBLY)
		setCheck();
}

void ConditionObjectAttacking::checkEvent(const Event& event)
{
	if(event.type() == Event::UNIT_ATTACKING){
		const EventUnitPlayerInt& eventUnit = safe_cast_ref<const EventUnitPlayerInt&>(event);
		const AttributeBase& attr = eventUnit.unit()->attr();
		const UnitActing* unit = safe_cast<const UnitActing*>(eventUnit.unit());
		int id = eventUnit.id();
		AttributeUnitOrBuildingReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi)
				if(!weapon_ || weapon_->ID() == id){
					setCheck();
					break;
				}
	}
}

void ConditionCommandAttack::checkEvent(const Event& event)
{
	if(event.type() == Event::COMMAND_ATTACK){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const WeaponPrm* weaponPrm = safe_cast<const UnitActing*>(eventUnit.unit())->selectedWeapon();
		if(!weaponPrm)
			return;
        int weaponID = weaponPrm->ID();
		AttributeUnitOrBuildingReferences::const_iterator oi;
		const AttributeBase& attr = eventUnit.unit()->attr();
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				if(weaponref_.empty()){
					setCheck();
					break;
				}
				else{
					bool mustBreak = false;
					WeaponPrmReferences::const_iterator i;
					FOR_EACH(weaponref_, i)
						if((*i).get()->ID() == weaponID){
							setCheck();
							mustBreak = true;
							break;
						}
					if(mustBreak)
						break;
				}
				
			}
	}
}

void ConditionCommandAttackSquad::checkEvent(const Event& event)
{
	if(event.type() == Event::COMMAND_ATTACK){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		if(eventUnit.unit()->attr().isLegionary() &&
		   &safe_cast<const UnitLegionary*>(eventUnit.unit())->squad()->attr() == attr)
			setCheck();
	}
}

void ConditionCommandAttackSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attr , "attr", "Тип сквада");
}

void ConditionSoldBuilding::checkEvent(const Event& event)
{
	if(event.type() == Event::SOLD_BUILDING){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const UnitBase* unit_ = eventUnit.unit();
		const AttributeBase& attr = unit_->attr();
		AttributeBuildingReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}

}

void ConditionSoldBuilding::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы зданий");
	removeZeros(objects_);
}

void ConditionCompleteBuild::checkEvent(const Event& event)
{
	if(event.type() == Event::COMPLETE_BUILDING){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const AttributeBase& attr = eventUnit.unit()->attr();
		AttributeBuildingReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}
}

void ConditionCompleteCure::checkEvent(const Event& event)
{
	if(event.type() == Event::COMPLETE_CURE){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		const AttributeBase& attr = eventUnit.unit()->attr();
		AttributeUnitOrBuildingReferences::const_iterator oi;
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}
}

void ConditionCompleteCure::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
}

void ConditionCompleteBuild::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы зданий");
	removeZeros(objects_);
}

void ConditionCompleteUpgrade::checkEvent(const Event& event)
{
	if(event.type() == Event::COMPLETE_UPGRADE){
		const EventUnitUnitAttributePlayer& eventUnit = safe_cast_ref<const EventUnitUnitAttributePlayer&>(event);
		const AttributeBase& attr = eventUnit.unitAttr();
		const UnitBase* unit = eventUnit.unit();

		AttributeUnitOrBuildingReferences::const_iterator oi;
		bool before = false;
		FOR_EACH(objectsBefore_, oi)
			if(&attr == *oi){
				before = true;
				break;
			}

		if(before || objectsBefore_.empty()){
			const AttributeBase& attrAfter = unit->attr();
			FOR_EACH(objects_, oi)
				if(&attrAfter == *oi){
					setCheck();
					break;
				}
		}
	}
}

void ConditionCompleteUpgrade::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objectsBefore_, "objectsBefore_", "&Типы юнитов (до апгрейда)");
	removeZeros(objectsBefore_);
	ar.serialize(objects_, "objects_", "&Типы юнитов (после апгрейда)");
	removeZeros(objects_);
}

void ConditionStartUpgrade::checkEvent(const Event& event)
{
	if(event.type() == Event::STARTED_UPGRADE){
		const EventUnitUnitAttributePlayer& eventUnit = safe_cast_ref<const EventUnitUnitAttributePlayer&>(event);
		const AttributeBase& attr = eventUnit.unitAttr();
		const UnitBase* unit = eventUnit.unit();

		AttributeUnitOrBuildingReferences::const_iterator oi;
		bool before = false;
		const AttributeBase& attrBefore = unit->attr();
		FOR_EACH(objectsBefore_, oi)
			if(&attrBefore == *oi){
				before = true;
				break;
			}

		if(before || objectsBefore_.empty())
			FOR_EACH(objects_, oi)
				if(&attr == *oi){
					setCheck();
					break;
				}
	}
}

void ConditionStartUpgrade::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objectsBefore_, "objectsBefore_", "&Типы юнитов (до апгрейда)");
	removeZeros(objectsBefore_);
	ar.serialize(objects_, "objects_", "&Типы юнитов (после апгрейда)");
	removeZeros(objects_);
}

void ConditionStartProduction::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
}

void ConditionStartProduction::checkEvent(const Event& event)
{
	if(event.type() == Event::STARTED_PRODUCTION){
		const EventUnitAttributePlayer& eventUnit = safe_cast_ref<const EventUnitAttributePlayer&>(event);
		AttributeUnitReferences::const_iterator oi;
		const AttributeBase& attr = eventUnit.unitAttr();
		FOR_EACH(objects_, oi)
			if(&attr == *oi){
				setCheck();
				break;
			}
	}
}

void ConditionStartProductionParameter::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(producer_, "producer", "&Производитель");
}

void ConditionStartProductionParameter::checkEvent(const Event& event)
{
	if(event.type() == Event::STARTED_PRODUCTION){
		const EventUnitAttributePlayer& eventUnit = safe_cast_ref<const EventUnitAttributePlayer&>(event);
		if(&eventUnit.unitAttr() == producer_)
			setCheck();
	}
}

CountObjectsScanOp::CountObjectsScanOp(Player* player, const AttributeUnitOrBuildingReferences& units, float radius, bool aimed) 
					: units_(units), radius_(radius), aimed_(aimed), player_(player)
{
	count_ = 0;
	position_ = Vect2f::ZERO;
}

void CountObjectsScanOp::checkInRadius(const Vect2f& position)
{
	count_ = 0;
	position_ = position;
	universe()->unitGrid.Scan(position.x, position.y, round(radius_) , *this);
	show(position, Color4c::RED);
}

void CountObjectsScanOp::show(Vect2f pos, const Color4c& color)
{
	if(showDebugPlayer.showSearchRegion){
		show_vector(To3D(pos), radius_, color);
		XBuffer buf;
		buf <= count_;
		show_text(To3D(pos), buf, color);
	}
}

void CountObjectsScanOp::operator()(UnitBase* unit)
{
	if(units_.empty() || !units_.front())
	{
		if(unit->position2D().distance(position_) < radius_){
			if(aimed_ ? unit->attr().isActing() && safe_cast<UnitActing*>(unit)->targetUnit() && player_->isEnemy(unit) : true)
				++count_;
			return;
		}
	}

	AttributeUnitOrBuildingReferences::const_iterator i;
	FOR_EACH(units_, i)
		if(&unit->attr() == *i && unit->position2D().distance(position_) < radius_){
			if(aimed_ ? unit->attr().isActing() && safe_cast<UnitActing*>(unit)->targetUnit() && player_->isEnemy(unit) : true)
				++count_;
			return;
		}
}

ConditionCountObjectsInRadius::ConditionCountObjectsInRadius()
{
	radius_ = 150.f;	
	count_ = 1;
}

ConditionObjectsInRadiusAimed::ConditionObjectsInRadiusAimed()
{
	radius_ = 150.f;	
}

ConditionCompareCountObjectsInRadius::ConditionCompareCountObjectsInRadius()
{
	radius_ = 150.f;	
}

bool ConditionObjectsInRadiusAimed::check(UnitActing* unit) const
{
	CountObjectsScanOp op = CountObjectsScanOp(&aiPlayer(), attributes_, radius_, true);

	if(squad_ && unit->getSquadPoint())
		op.checkInRadius(unit->getSquadPoint()->position2D());
	else
		op.checkInRadius(unit->position2D());

	return op.foundNumber();
}

bool ConditionCountObjectsInRadius::check(UnitActing* unit) const
{
	CountObjectsScanOp op = CountObjectsScanOp(&aiPlayer(), attributes_, radius_);
	op.checkInRadius(unit->position2D());
	return op.foundNumber() < count_;
}

bool ConditionCompareCountObjectsInRadius::check(UnitActing* unit) const
{
	CountObjectsScanOp op = CountObjectsScanOp(&aiPlayer(), attributesLess_, radius_);
	op.checkInRadius(unit->position2D());
	int count = op.foundNumber();

	CountObjectsScanOp opGreater = CountObjectsScanOp(&aiPlayer(), attributesGreater_, radius_);
	opGreater.checkInRadius(unit->position2D());

	return count < opGreater.foundNumber();
}

void ConditionCompareCountObjectsInRadius::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attributesLess_, "attributesLess", "Первая группа(типы объектов чье количество меньше)");
	removeZeros(attributesLess_);
	ar.serialize(attributesGreater_, "attributesGreater", "Вторая группа(типы объектов чье количество больше)");
	removeZeros(attributesGreater_);
	ar.serialize(radius_, "radius", "Радиус поиска");
}

void ConditionObjectsInRadiusAimed::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attributes_, "attributes", "Типы объектов, которые собираются атаковать");
	removeZeros(attributes_);
	ar.serialize(radius_, "radius", "Радиус поиска");
}

void ConditionCountObjectsInRadius::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attributes_, "attributes", "Типы объектов, для которых считается количество");
	removeZeros(attributes_);
	ar.serialize(radius_, "radius", "Радиус поиска");
	ar.serialize(count_, "count", "Количество");
}

void ConditionCommandMove::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
}

void ConditionCommandMoveSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attr, "attr", "Тип сквада");
}

void ConditionObjectAttacking::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
	ar.serialize(weapon_, "weapon", "Тип оружия");
}

void ConditionObjectByLabelAttacking::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(label_, "label", "Метка юнита");
}

void ConditionObjectByLabelAttacking::checkEvent(const Event& event)
{
	if(event.type() == Event::UNIT_ATTACKING){
		const EventUnitPlayerInt& eventUnit = safe_cast_ref<const EventUnitPlayerInt&>(event);
		if(_stricmp(safe_cast<const UnitReal*>(eventUnit.unit())->label(), label_.c_str()))
			return;
		__super::checkEvent(event);
	}
}
void ConditionCommandAttack::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
	ar.serialize(weaponref_, "weaponref", "&Типы оружия");
}

void ConditionStartBuild::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы зданий");
	removeZeros(objects_);
}

ConditionCaptureBuilding::ConditionCaptureBuilding()
{
	playerType_ = MY_PLAYER;
	participators_ = ENEMIES;
}

bool ConditionUnitCaptured::check(UnitActing* unit) const
{
	Player* playerPrev = unit->playerPrev();
	return (playerPrev != 0 && playerPrev != unit->player());
}

void ConditionCaptureBuilding::checkEvent(const Event& event) 
{
	if(event.type() == Event::CAPTURE_UNIT){
		const EventUnitMyUnitEnemy& eventUnit = safe_cast_ref<const EventUnitMyUnitEnemy&>(event);
		switch(participators_){
			case FRIENDS:
				if(eventUnit.unitMy()->isEnemy(eventUnit.unitEnemy()))
					return;
				break;
			case ENEMIES:
				if(!eventUnit.unitMy()->isEnemy(eventUnit.unitEnemy()))
					return;
				break;
			case ALL:
				break;
		}
		switch(playerType_){
			case MY_PLAYER: {
				if(&aiPlayer() == eventUnit.unitEnemy()->player()){
					if(objects_.empty()) {
						setCheck();
						return;
					}
					AttributeUnitOrBuildingReferences::const_iterator oi;
					const AttributeBase& attr = eventUnit.unitMy()->attr();
					FOR_EACH(objects_, oi)
						if(*oi == &attr){
							setCheck();
							break;
						}
				}
				break;
			}
			case ENEMY_PLAYER: {
				if(&aiPlayer() == eventUnit.unitMy()->player()){
					if(objects_.empty()) {
						setCheck();
						return;
					}
					AttributeUnitOrBuildingReferences::const_iterator oi;
					const AttributeBase& attr = eventUnit.unitMy()->attr();
					FOR_EACH(objects_, oi)
						if(*oi == &attr){
							setCheck();
							break;
						}
				}
				break;
			}
		}
	}
}

bool ConditionObjectByLabelExists::check() const 
{ 
	return anchor_;
}

void ConditionKillObjectByLabel::checkEvent(const Event& event) 
{
	if(event.type() == Event::DESTROY_OBJECT){
		const EventUnitPlayer& eventUnit = safe_cast_ref<const EventUnitPlayer&>(event);
		if(eventUnit.unit()->attr().isObjective() && !strcmp(label_.c_str(), safe_cast<const UnitReal*>(eventUnit.unit())->label())
			&& checkPlayer(&aiPlayer(), eventUnit.player(), playerType))
			setCheck();
	}
}

void ConditionTimeMatched::checkEvent(const Event& event) 
{
	if(event.type() == Event::TIME){
		const EventTime& eventTime = safe_cast_ref<const EventTime&>(event);
		if(eventTime.time() < time*1000)
			setCheck();
	}
}

void ConditionNetworkDisconnect::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(hardDisconnect_, "hardDisconnect", "Полная потеря связи, требуется перелогин");
}

void ConditionNetworkDisconnect::checkEvent(const Event& event)
{
	if(event.type() == Event::NETWORK_DISCONNECT)
		if(safe_cast_ref<const EventNet&>(event).isHard() == hardDisconnect_)
			setCheck();
}

void ConditionKeyboardClick::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(keys_, "keys", "Набор клавиш");
}

void ConditionKeyboardClick::checkEvent(const Event& event) 
{
	if(event.type() == Event::KEYBOARD_CLICK){
		if(keys_.empty()){
			setCheck();
			return;
		}
		int fullKey = safe_cast_ref<const EventKeyBoardClick&>(event).fullKey();
		Keys::const_iterator ki;
		FOR_EACH(keys_, ki)
			if((*ki).fullkey == fullKey){
				setCheck();
				break;
			}
	}
}

ConditionClickOnButton::ConditionClickOnButton()
: controlPtr_(0)
, events_(UI_MOUSE_LBUTTON_DOWN)
{
	wasClicked_ = false;
	onlyEnabled_ = true;
	ignorePause_ = false;
	timeOut_ = 1;
}

void ConditionClickOnButton::checkEvent(const Event& event) 
{
	if(!controlPtr_) controlPtr_ = control_.control();

	dassert(controlPtr_ == control_.control());

	if(event.type() == (ignorePause_ ? Event::UI_BUTTON_CLICK : Event::UI_BUTTON_CLICK_LOGIC)){
		const EventButtonClick& eventButtonClick = safe_cast_ref<const EventButtonClick&>(event);
		if(eventButtonClick.checkPlayerID(aiPlayer().playerID()) && eventButtonClick.control() == controlPtr_
			&& (!onlyEnabled_ || eventButtonClick.enabled())
			&& (eventButtonClick.inputEvents() & events_)
			&& eventButtonClick.modifiers() == modifiers_)
				wasClicked_ = true;
	}
}

void ConditionMyObjectNearAnchorByLabel::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "&Метка(на якоре)");
	ar.serialize(distance, "distance", "&Максимальное расстояние");
	ar.serialize(onlyVisible_, "onlyVisible", "Только видимые юниты");
}

void ConditionObjectNearAnchorByLabel::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "&Метка(на якоре)");
	ar.serialize(objects_, "objects", "&Указанные объекты");
	removeZeros(objects_);
	ar.serialize(playerType, "playerType", "Владелец объекта");
	ar.serialize(distance, "distance", "&Максимальное расстояние");
	ar.serialize(constructedAndConstructing, "constructedAndConstructing", "Построенные или недостроенные");
	ar.serialize(onlyVisible_, "onlyVisible", "Учитывать только видимые объекты");
	ar.serialize(count_, "count", "Минимальное количество");
}

bool ConditionObjectByLabel::check(UnitActing* unit) const
{
	return !_stricmp(unit->label(), label_.c_str());
}

bool ConditionCheckPersonalParameter::check(UnitActing* unit) const
{
	return compare(unit->parameters().findByIndex(parameterType_.key(), 0), value_, COMPARE_EQ);
}

void ConditionCheckPersonalParameter::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(parameterType_, "parameterType", "Параметр");
	ar.serialize(value_, "value", "Величина");
}

void ConditionObjectByLabel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(label_, "label", "Метка юнита");
}

ConditionObjectNearAnchorByLabel::ConditionObjectNearAnchorByLabel() 
{
	playerType = AI_PLAYER_TYPE_ME; 
	distance = 100; 
	constructedAndConstructing = CONSTRUCTED | CONSTRUCTING; 
	onlyVisible_ = false;
	count_ = 1;
}

ConditionMyObjectNearAnchorByLabel::ConditionMyObjectNearAnchorByLabel() 
{
	distance = 100; 
	onlyVisible_ = false;
}

bool ConditionMyObjectNearAnchorByLabel::check(UnitActing* unit) const
{
	if(!anchor_){
		xassertStr(0 && "Якорь по метке не найден: ", anchor_.c_str());
		return false;
	}												
	else{
		Vect2f position = anchor_->position2D();

		if(onlyVisible_ && unit->isUnseen())
			return false;
		if(unit->position2D().distance2(position) < sqr(distance))
			return true;
		return false;
	}
}

void ConditionAnchorOnScreen::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "&Метка(на якоре)");
	ar.serialize(distance_, "distance", "Макс. расстояние до камеры");
}

bool ConditionAnchorOnScreen::check() const
{
	if(!anchor_){
		xassertStr(0 && "Якорь по метке не найден: ", anchor_.c_str());
		return false;
	}												
	else{
		Vect3f pos = anchor_->position();
		Vect3f e;
		Vect3f pv;
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos,&pv,&e);

		Vect2f screenSize = cameraManager->GetCamera()->GetRenderSize();
		if(e.x > 0 && e.x < screenSize.x && e.y > 0 && e.y < screenSize.y)
			if(cameraManager->eyePosition().distance(pos) < distance_)
				return true;
	}

	return false;
}

class NoiseScaner
{
public:
	NoiseScaner(UnitActing* owner, AttributeUnitOrBuildingReferences attrTargets);

	void operator()(UnitBase* unit);

	UnitBase* target() const { return target_; }

private:

	UnitActing* owner_;
	AttributeUnitOrBuildingReferences attrTargets_;

	UnitBase* target_;
};

NoiseScaner::NoiseScaner(UnitActing* owner, AttributeUnitOrBuildingReferences attrTargets) : 
	owner_(owner), target_(0), attrTargets_(attrTargets)
{}

void NoiseScaner::operator()(UnitBase* unit)
{
	if(target_)
		return;

	if(!unit->alive() || !owner_->isEnemy(unit))
		return;

	bool found = false;
	AttributeUnitOrBuildingReferences::const_iterator i;
	FOR_EACH(attrTargets_, i)
		if((*i) == &unit->attr())
		{
			found = true;
			break;
		}
	if(!found)
		return;

	float dist2 = owner_->position2D().distance2(unit->position2D());
	float noise_r2 = sqr(unit->noiseRadius());
	if(dist2 < noise_r2)
		target_ = unit;
}

void ConditionObjectHearNoise::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrTargets_, "attrTargets", "Типы юнитов, которые слышны");
}

bool ConditionObjectHearNoise::check(UnitActing* unit) const
{
	if(attrTargets_.empty())
		return unit->noiseTarget();
	else
	{
		NoiseScaner noise_scaner(unit, attrTargets_);
		universe()->unitGrid.Scan(unit->position().xi(), unit->position().yi(), unit->sightRadius(), noise_scaner);
		return noise_scaner.target();
	}
}

int ConditionObjectNearAnchorByLabel::countfindUnit(Player* player, const AttributeBase* attr) const
{
	int counter = 0;
	xassert(anchor_);
	Vect2f nearPosition = anchor_->position2D();
	float dist;
	RealUnits::const_iterator ui;
	if(attr){
		FOR_EACH(player->realUnits(),ui){
			dist = nearPosition.distance2((*ui)->position2D());
			if(&(*ui)->attr() == attr && dist < sqr(distance)){
				if(onlyVisible_ && (*ui)->isUnseen())
					continue;
				if((constructedAndConstructing & CONSTRUCTED) && (constructedAndConstructing & CONSTRUCTING) ||
					(constructedAndConstructing & CONSTRUCTED) && (*ui)->isConstructed() ||
					(constructedAndConstructing & CONSTRUCTING) && !(*ui)->isConstructed()){
						++counter;
					}
			}
		}
	}
	else{
		FOR_EACH(player->realUnits(), ui){
			dist = nearPosition.distance2((*ui)->position2D());
			if(!(*ui)->attr().internal && dist < sqr(distance)){
				if(onlyVisible_ && (*ui)->isUnseen())
					continue;
				if((constructedAndConstructing & CONSTRUCTED) && (constructedAndConstructing & CONSTRUCTING) ||
					(constructedAndConstructing & CONSTRUCTED) && (*ui)->isConstructed() ||
					(constructedAndConstructing & CONSTRUCTING) && !(*ui)->isConstructed()){
						++counter;
					}
			}
		}
	}

	return counter;
}

bool ConditionObjectNearAnchorByLabel::check() const
{
	if(!anchor_){
		xassertStr(0 && "Якорь по метке не найден: ", anchor_.c_str());
		return false;
	}												
	else{
		int counter = 0;
		AttributeReferences::const_iterator i;
		if(objects_.empty() || !objects_.front()){
			switch(playerType){
				case AI_PLAYER_TYPE_ME:
					counter = countfindUnit(&aiPlayer(), 0);
					break;
				case AI_PLAYER_TYPE_ENEMY:
					{
						PlayerVect::iterator pi;
						FOR_EACH(universe()->Players, pi)
							if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
								counter += countfindUnit(*pi,0);
						break;
					}
				case AI_PLAYER_TYPE_WORLD:
					counter = countfindUnit(universe()->worldPlayer(), 0);
					break;
				case AI_PLAYER_TYPE_ANY:
					{
						PlayerVect::iterator pi;
						FOR_EACH(universe()->Players, pi)
							counter += countfindUnit(*pi, 0);
						break; 
					}

			}
			
			if(counter >= count_)
				return true;
		}
		else
			FOR_EACH(objects_, i){
				switch(playerType){
					case AI_PLAYER_TYPE_ME:
						counter = countfindUnit(&aiPlayer(), *i);
						break;
					case AI_PLAYER_TYPE_ENEMY:
						{
							PlayerVect::iterator pi;
							FOR_EACH(universe()->Players, pi)
								if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
									counter += countfindUnit(*pi, *i);
							break;
						}
					case AI_PLAYER_TYPE_WORLD:
						counter = countfindUnit(universe()->worldPlayer(), *i);
						break;
					case AI_PLAYER_TYPE_ANY:
						{
							PlayerVect::iterator pi;
							FOR_EACH(universe()->Players, pi)
								counter += countfindUnit(*pi, *i);
							break; 
						}

				}

				if(counter >= count_)
					return true;
			}

		return false;
	}
}

ConditionObjectUnderAttack::ConditionObjectUnderAttack()
{
	damagePercent_ = 0;
	weaponType_ = ANY_TYPE;
	checkUnit_ = false;
}

ConditionObjectWorking::ConditionObjectWorking()
 { 
	 activity_ = -1; 
 }

void ConditionObjectWorking::serialize(Archive& ar)
{
	start_timer_auto();
	__super::serialize(ar);
	ar.serialize(activity_, "activity", "Производимые действия");
}

ConditionCompareObjectsCount::ConditionCompareObjectsCount()
{
	connectedOnly_ = false;
	compare_ = COMPARE_GREATER_EQ;
}

void ConditionCompareObjectsCount::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(group1_, "group1", "Группа типов 1");
	removeZeros(group1_);
	ar.serialize(group2_, "group2", "Группа типов 2");
	removeZeros(group2_);
	ar.serialize(playerScanner_, "|playerScanner|playerType", "Владелец объектов");
	ar.serialize(connectedOnly_, "connectedOnly", "Только подключенные");
	ar.serialize(compare_, "compare", "Критерий сравнения");
}

bool ConditionCompareObjectsCount::check() const
{
	int counter1 = 0;
	int counter2 = 0;
	if(!connectedOnly_){
		do{
			Player& player = playerScanner_.player(aiPlayer());
			AttributeUnitOrBuildingReferences::const_iterator i;
			FOR_EACH(group1_, i)
				counter1 += player.realUnits(*i).size();
			FOR_EACH(group2_, i)
				counter2 += player.realUnits(*i).size();
		} while(playerScanner_.next(aiPlayer()));
	}
	else{
		do{
			Player& player = playerScanner_.player(aiPlayer());
			AttributeUnitOrBuildingReferences::const_iterator i;
			RealUnits::const_iterator j;
			FOR_EACH(group1_, i){
				xassert((*i)->isBuilding());
				const RealUnits& list = player.realUnits(*i);
				FOR_EACH(list, j)
					if(safe_cast<UnitBuilding*>(*j)->isConstructed())
						++counter1;
			}
			FOR_EACH(group2_, i){
				xassert((*i)->isBuilding());
				const RealUnits& list = player.realUnits(*i);
				FOR_EACH(list, j)
					if(safe_cast<UnitBuilding*>(*j)->isConstructed())
						++counter2;
			}
		} while(playerScanner_.next(aiPlayer()));
	}

	return compare(counter1, counter2, compare_);
}

bool ConditionObjectWorking::check(UnitActing* unit) const
{
	if(!unit->getSquadPoint() || !squad_){ 
		if((activity_ & ACTIVITY_CONSTRUCTING && !unit->isConstructed()) 
			|| (activity_ & ACTIVITY_MOVING && unit->isMoving() && unit->unitState() == UnitReal::MOVE_MODE)
			|| (activity_ & ACTIVITY_UPGRADING && unit->isUpgrading())
			|| (activity_ & ACTIVITY_PRODUCING && unit->attr().isActing() && safe_cast<UnitActing*>(unit)->isProducing())
			|| (activity_ & ACTIVITY_BUILDING && unit->attr().isLegionary() && safe_cast<UnitLegionary*>(unit)->constructingBuilding())
			|| (activity_ & ACTIVITY_ATTACKING && unit->attr().isActing() && safe_cast<UnitActing*>(unit)->fireTargetExist() /*&& unit->getUnitState() == UnitReal::ATTACK_MODE*/)
			|| (activity_ & ACTIVITY_PICKING_RESOURCE && unit->attr().isLegionary() && safe_cast<UnitLegionary*>(unit)->gatheringResource())
			|| (activity_ & ACTIVITY_TELEPORTATING && unit->attr().isLegionary() && safe_cast<UnitLegionary*>(unit)->isTeleporting())
			|| (activity_ & ACTIVITY_MOVING_TO_TRANSPORT && unit->attr().isLegionary() && safe_cast<UnitLegionary*>(unit)->isMovingToTransport())
			|| (activity_ & ACTIVITY_WAITING_FOR_PASSENGER && unit->attr().isActing() && safe_cast<UnitActing*>(unit)->waitingForPassenger()))
			return true;
	}
	else{ 
		const LegionariesLinks& units = unit->getSquadPoint()->units();
		LegionariesLinks::const_iterator i;
		FOR_EACH(units, i){
			if((activity_ & ACTIVITY_CONSTRUCTING && !(*i)->isConstructed()) 
				|| (activity_ & ACTIVITY_MOVING && (*i)->isMoving() && (*i)->unitState() == UnitReal::MOVE_MODE)
				|| (activity_ & ACTIVITY_UPGRADING && (*i)->isUpgrading())
				|| (activity_ & ACTIVITY_PRODUCING && (*i)->isProducing())
				|| (activity_ & ACTIVITY_BUILDING && (*i)->constructingBuilding())
				|| (activity_ & ACTIVITY_ATTACKING && (*i)->fireTargetExist() /*&& (*i)->getUnitState() == UnitReal::ATTACK_MODE*/)
				|| (activity_ & ACTIVITY_PICKING_RESOURCE && (*i)->gatheringResource())
				|| (activity_ & ACTIVITY_TELEPORTATING && (*i)->isTeleporting())
				|| (activity_ & ACTIVITY_MOVING_TO_TRANSPORT && (*i)->isMovingToTransport())
				|| (activity_ & ACTIVITY_WAITING_FOR_PASSENGER && (*i)->waitingForPassenger()))
				return true;

		}
	}
	return false;
}

void ConditionCheckOutWater::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(outWater_, "outWater", "Тип внешней среды");
}

bool ConditionCheckOutWater::check() const
{
	switch(outWater_){
		case WATER_IS_ICE:
			return water && water->waterIsIce();
		case WATER_IS_LAVA:
			return water && water->isLava();
		case WATER_IS_WATER:
			return water && !water->waterIsIce() && !water->isLava();
	}

	return false;
}

void ConditionObjectUnderAttack::checkEvent(const Event& event)
{
	if(event.type() == Event::ATTACK_OBJECT){
		const EventUnitMyUnitEnemy& eventUnit = safe_cast_ref<const EventUnitMyUnitEnemy&>(event);
		const UnitBase* unitMy = eventUnit.unitMy();
		
		if(unitMy->player() != &aiPlayer())
			return;

		if(!checkUnit(const_cast<UnitActing*>(safe_cast<const UnitActing*>(unitMy))))
			return;

		if((1 - unitMy->health())*100 < damagePercent_)
			return;
		
		const UnitActing* unitEnemy = safe_cast<const UnitActing*>(eventUnit.unitEnemy());

		if(checkUnit_ && (!unitEnemy || !unitEnemy->isEnemy(unitMy)))
			return;

		if(!weapons_.empty() || weaponType_ || !objectsEnemy_.empty()){
  			if(!unitEnemy)
				return;
			const WeaponPrm* weapon = unitEnemy->selectedWeapon();
			
			bool checkWeapon = false;

			if(weapons_.empty() || !weapons_.front())
				checkWeapon = true;

			WeaponPrmReferences::const_iterator wi;
			FOR_EACH(weapons_, wi)
				if(*wi && weapon == *wi){
					checkWeapon = true;
					break;
				}

			if(!checkWeapon)
				return;

			if(weaponType_ && !weapon)
				return;
			switch(weaponType_){
				case SHORT_RANGE:
					if(!weapon->isShortRange())
						return;
					break;
				case LONG_RANGE:
					if(!weapon->isLongRange())
						return;
					break;
				case FROM_INTERFACE:
					if(weapon->shootingMode() == WeaponPrm::SHOOT_MODE_INTERFACE)
						return;
			}

			AttributeUnitOrBuildingReferences::const_iterator i;
			bool equalAttr = objectsEnemy_.empty() ? true : false;
			FOR_EACH(objectsEnemy_, i)
				if(*i && &unitEnemy->attr() == *i)
					equalAttr = true;
			if(!equalAttr)
				return;
		}

		setEventContextUnit(const_cast<UnitActing*>(safe_cast<const UnitActing*>(unitMy)));
	}
}

void ConditionObjectUnderAttack::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(damagePercent_, "damagePercent_", "&Процент урона");
	if(!ar.serialize(objectsEnemy_, "objectsEnemy_", "&Типы атакующих объектов")) // CONVERSION 11.06.2007
	{
		AttributeUnitOrBuildingReference objectEnemy;
		ar.serialize(objectEnemy, "objectEnemy_", "&Тип атакующего объекта");
		if(objectEnemy)
			objectsEnemy_.push_back(objectEnemy);
	}	
	removeZeros(objectsEnemy_);
	ar.serialize(weapons_, "weapons", "&Виды оружия");
	removeZeros(weapons_);
	ar.serialize(weaponType_, "weaponType_", "&Тип оружия");
	ar.serialize(checkUnit_, "checkUnit", "Учитывать только атаки вражеских юнитов");
}

bool ConditionContext::checkDebug(UnitActing* unit) 
{ 
	bool found = false;
	if(objects_.empty())
		found = true;
	else{
		AttributeUnitOrBuildingReferences::const_iterator oi; 
		FOR_EACH(objects_, oi)
			if(!(*oi) || &unit->attr() == *oi){
				found = true;
				break;
			}
	}

	if(squadFilter_ && squad_ && unit->attr().isLegionary() && &safe_cast<UnitLegionary*>(unit)->squad()->attr() != squad_)
		found = false;

	if(!ignoreContext_)
		state_ = found ? (check(unit) ? !inverted() : inverted()) : false; // инвертируется предикат
	else{ 
		if(found && check(unit)) // инвертируется все условие
			ignoreContextCounter_ = 2;
		state_ = ignoreContextCounter_ ? !inverted() : inverted();
	}

	return state_;
}

void ConditionContext::clearContext()
{
	if(ignoreContextCounter_)
		ignoreContextCounter_--;
}

void ConditionContext::clear()
{
	__super::clear();
	ignoreContextCounter_ = 0;
}

bool ConditionContext::isContext(ContextFilter& filter) const 
{ 
	AttributeUnitOrBuildingReferences::const_iterator i;
	FOR_EACH(objects_, i)
		filter.addUnit(*i);
	filter.addSquad(squad_);
	return true; 
}

void ConditionContext::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
	if(squadFilter_)
		ar.serialize(squad_, "squad", "Тип сквада");
	ar.serialize(ignoreContext_, "ignoreContext", "&Игнорировать контекст условия");
}

bool ConditionProducedAllParameters::check() const
{
	if(!object_)
		return false;

	AttributeBase::ProducedParametersList::const_iterator i;
	FOR_EACH(object_.get()->producedParameters, i)
		if(!aiPlayer().checkUniqueParameter(i->second.signalVariable.c_str(), 1))
			return false;

	return true;
}

void ConditionProducedAllParameters::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(object_, "object", "Тип юнита");
}

bool ConditionObjectOnWater::check(UnitActing* unit)  const
{
	return unit->rigidBody() && (unit->rigidBody()->flyingMode() ? unit->rigidBody()->onDeepWater() : unit->rigidBody()->onWater());
}

bool ConditionObjectOnIce::check(UnitActing* unit)  const
{
	return unit->rigidBody() && unit->rigidBody()->onIce() && !unit->isFrozen();
}

void ConditionObjectAimed::clear()
{
	__super::clear();

	timer_.stop();
}

bool ConditionObjectAimed::check(UnitActing* unit) const
{
	return unit->aimed();
}

bool ConditionObjectIsNotUnderAttack::check(UnitActing* unit) const
{
	return !safe_cast<UnitActing*>(unit)->underAttack();
}

ConditionSquadSufficientUnits::ConditionSquadSufficientUnits() 
{
	counter_ = 1;
	compareOperator_ = COMPARE_LESS;
}

bool ConditionSquadSufficientUnits::check(UnitActing* unit) const
{
	if(objects_.empty() || !objects_.front())
		return  unit->attr().isLegionary() && compare(safe_cast<UnitLegionary*>(unit)->squad()->unitsNumber(), counter_, compareOperator_);
	else
		return  unit->attr().isLegionary() && compare(safe_cast<UnitLegionary*>(unit)->squad()->unitsNumber(&unit->attr()), counter_, compareOperator_);
}

void ConditionSquadSufficientUnits::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(compareOperator_, "compareOperator", "Условие сравнения");
	ar.serialize(counter_, "counter", "Количество");
}

ConditionOnlyMyClan::ConditionOnlyMyClan()
{
	checkAuxPlayers = true;
}

bool ConditionOnlyMyClan::check() const
{
	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi)
		if(!(*pi)->isWorld() && (checkAuxPlayers || (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER) && (*pi)->clan() != aiPlayer().clan()){ 
            RealUnits::const_iterator ui;
			FOR_EACH((*pi)->realUnits(), ui)
				if((*ui)->attr().accountInCondition)
					return false;
		}
	return true;
}

void ConditionOnlyMyClan::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(checkAuxPlayers, "checkAuxPlayers", "Учитывать служебных игроков");
}

void ConditionNoUnitsLeft::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(playerNum_, "playerNum", "Номер игрока");
}

bool ConditionNoUnitsLeft::check() const
{
	Player* player = 0;

	if(playerNum_ == -1)
		player = &aiPlayer();
	else
		player = universe()->findPlayer(playerNum_);

	if(player){
		RealUnits::const_iterator ui;
		FOR_EACH(player->realUnits(), ui)
			if((*ui)->attr().accountInCondition)
				return false;
		return true;
	}

	return false;
}

bool ConditionDifficultyLevel::check() const
{
	return aiPlayer().difficulty() == difficulty;
}

ConditionUserSave::ConditionUserSave()
{
	userSave_ = universe() && universe()->userSave();
}

ConditionUnitLevel::ConditionUnitLevel()
{
 	level_ = 0;
	compareOperator_ = COMPARE_EQ; 
}

bool ConditionUnitLevel::check(UnitActing* unit) const
{
	if(!unit->attr().isLegionary())
		return false;

	return compare(safe_cast<UnitLegionary*>(unit)->level(), level_, compareOperator_);
}

void ConditionUnitLevel::serialize(Archive& ar)
{
	__super::serialize(ar);

    ar.serialize(compareOperator_, "compareOperator", "&Критерий сравнения");
	ar.serialize(level_, "level", "&Уровень");
}

ConditionSelected::ConditionSelected()
{
	singleOnly = false;
	uniform = false;
}

void ConditionSelected::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(objects_, "objects_", "&Типы юнитов");
	removeZeros(objects_);
	ar.serialize(singleOnly, "singleOnly", "&Только один объект в селекте");
	ar.serialize(uniform, "uniform", "&Только один тип объекта в селекте");
}

ConditionSquadSelected::ConditionSquadSelected()
{
	singleOnly = false;
}

void ConditionSquadSelected::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(attribute, "attribute", "&Тип сквада");
	ar.serialize(singleOnly, "singleOnly", "&Только один объект в селекте");
}

ConditionDistanceBetweenObjects::ConditionDistanceBetweenObjects()
{
	distance = 100;
	aiPlayerType_ = AI_PLAYER_TYPE_ANY;
	onlyLegionaries_ = false;
	onlyVisible_ = false;
	unit1_ = 0;
}

ConditionGetResourceLevel::ConditionGetResourceLevel()
{
	counter_ = 1.f;
	compareOperator_ = COMPARE_GREATER_EQ;
}

void ConditionGetResourceLevel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(parameterType_, "parameterType", "Тип ресурса");
	ar.serialize(compareOperator_, "compareOperator", "Критерий сравнения");
	ar.serialize(counter_, "counter", "Количество");
}

bool ConditionGetResourceLevel::check() const
{
	start_timer_auto();
	ParameterSet resourceVelocity_ = aiPlayer().resourceDelta();
	return resourceVelocity_.contain(parameterType_) && 
		compare(resourceVelocity_.findByIndex(parameterType_.key()) * 10.f, counter_, compareOperator_);
}

bool ConditionDistanceBetweenObjects::check(UnitActing* unit) const
{
	start_timer_auto();
	if(foundTimer_.busy())
		return true;
	
	unit1_ = unit;
	Vect2i pos = unit->position2D(); 
	universe()->unitGrid.ConditionScan(pos.x, pos.y, round(distance), const_cast<ConditionDistanceBetweenObjects&>(*this));
	return foundTimer_.busy();
}

bool ConditionDistanceBetweenObjects::operator()(UnitBase* unit2)
{
	if(unit2 != unit1_){
		if(onlyLegionaries_ && !unit2->attr().isLegionary())
			return true;
		if(onlyVisible_ && unit2->isUnseen())
			return true;
		if(!checkPlayer(&aiPlayer(), unit2->player(), aiPlayerType_))
			return true;

		if(!objects.empty()){
			AttributeReferences::const_iterator it;
			FOR_EACH(objects, it)
				if(&unit2->attr() == (*it) && unit1_->position2D().distance2(unit2->position2D()) < sqr(distance)){
					foundTimer_.start(2000);
					return false;
				}
		}
		else if(unit2->attr().isActing() && unit1_->position2D().distance2(unit2->position2D()) < sqr(distance)){
			foundTimer_.start(2000);
			return false;
		}
	}
	return true;
}

void ConditionDistanceBetweenObjects::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(objects, "objects", "&Объекты");
	removeZeros(objects);
	ar.serialize(distance, "distance", "&Расстояние");
	ar.serialize(aiPlayerType_, "aiPlayerType", "Владелец объектов");
	ar.serialize(onlyLegionaries_, "onlyLegionaries", "Учитывать только юнитов");
	ar.serialize(onlyVisible_, "onlyVisible", "Учитывать только видимых юнитов");
}

ConditionPlayerParameters::Data::Data()
{
	operation = COMPARE_GREATER_EQ;
	value = 0;
}

void ConditionPlayerParameters::Data::serialize(Archive& ar)
{
	ar.serialize(type, "type", "&Тип");
	ar.serialize(operation, "operation", "&Операция");
	ar.serialize(value, "value", "&Значение");
}

void ConditionPlayerParameters::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(vector_, "vector", "Список");
}

bool ConditionPlayerParameters::check() const
{
	Vector::const_iterator i;
	FOR_EACH(vector_, i)
		if(!compare(aiPlayer().resource().findByIndex(i->type.key()), i->value, i->operation, 0.01f))
			return false;
	return true;
}

ConditionPercentOfPlayerResource::ConditionPercentOfPlayerResource()
{
	percent_ = 50;
	operation_ = COMPARE_LESS;
}

bool ConditionPercentOfPlayerResource::check() const
{
	float resource = aiPlayer().resource().findByIndex(type_.key());
	float capacity = aiPlayer().resourceCapacity().findByIndex(type_.key());
	return compare(resource, percent_*0.01*capacity, operation_);
}

void ConditionPercentOfPlayerResource::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(type_, "type", "Тип параметра");
	ar.serialize(operation_, "operation", "&Условие сравнения");
	ar.serialize(percent_, "percent", "&Процент от максимального кол-ва");
}

ConditionCheckInt::ConditionCheckInt()
{
	scope_ = SCOPE_UNIVERSE;
	op_ = COMPARE_EQ;
	value_ = 0;
}

bool ConditionCheckInt::check() const
{
	if(scope_ != SCOPE_MISSION_DESCRIPTION){
		int value = 0;
		typedef StaticMap<string, int> IntVariables;
		const IntVariables* table;
		switch(scope_){
		case SCOPE_GLOBAL:
			table = &Singleton<IntVariables>::instance();
			break;
		case SCOPE_PROFILE:
			table = &universe()->currentProfileIntVariables();
			break;
		case SCOPE_PLAYER:
			table = &aiPlayer().intVariables();
			break;
		case SCOPE_UNIVERSE:
			table = &universe()->intVariables();
			break;
		}

		IntVariables::const_iterator i = table->find(name_);
		if(i != table->end())
			value = i->second;
		return compare(value, value_, op_);
	}
	else
		return missionDescription() && missionDescription()->triggerFlags() & (1 << value_);
}

void ConditionCheckInt::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(scope_, "scope", "&Область действия переменной");
	if(scope_ != SCOPE_MISSION_DESCRIPTION){
		ar.serialize(name_, "name", "&Имя переменной");
		ar.serialize(op_, "operator", "&Операция сравнения");
		ar.serialize(value_, "value", "&Значение");
	}
	else{
		missionDescription.serialize(ar);
		ar.serialize(RangedWrapperi(value_, 0, 31), "value", "&Номер переменной");
	}
}

ConditionEvent::ConditionEvent()
{
	timeOut_ = 10;
}

void ConditionEvent::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(timeOut_, "timeOut", "Время удержания события, секунды");
	ar.serialize(timer_, "timer", 0);
}

ConditionContextEvent::ConditionContextEvent()
{
	eventContextUnit_ = 0;
	timeOut_ = 60;
}

void ConditionContextEvent::serialize(Archive& ar)
{
	//inverted_ = false; // насильный запрет инвертирования
	__super::serialize(ar);
	ar.serialize(timeOut_, "timeOut", "Время удержания события, секунды");
	ar.serialize(timer_, "timer", 0);
	//ar.serialize(eventContextUnit_, "eventContextUnit", 0);
}

bool ConditionContextEvent::checkUnit(UnitActing* unit) const
{
	if(objects_.empty())
		return true;

	AttributeUnitOrBuildingReferences::const_iterator oi; 
	FOR_EACH(objects_, oi)
		if(!(*oi) || &unit->attr() == *oi)
			return true;
	
	return false;
}

void ConditionContextEvent::setEventContextUnit(UnitActing* unit)
{
	if(!ignoreContext_) 
		eventContextUnit_ = unit; 
	timer_.start(timeOut_* 1000);
}

bool ConditionContextEvent::check(UnitActing* unit) const
{
	return timer_.busy() && (ignoreContext_ || eventContextUnit_ == unit);
}

void ConditionContextEvent::clear()
{
	__super::clear(); 
	eventContextUnit_ = 0; 
	timer_.stop();
}

ConditionIsMultiplayer::ConditionIsMultiplayer()
{
	mode = ANY;
}

void ConditionIsMultiplayer::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mode, "mode", "Режим");
}



ConditionScreenRatio::ConditionScreenRatio()
{
	ratio_ = 4.f/3.f;
	op_ = COMPARE_EQ;
}

bool ConditionScreenRatio::check() const
{
	return compare((float)gb_RenderDevice->GetSizeX()/(float)gb_RenderDevice->GetSizeY(), ratio_, op_, 0.01f); 
}

void ConditionScreenRatio::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(op_, "op", "Операция сравнения");
	ar.serialize(ratio_, "ratio", "Отношение ширины к высоте");
}

ConditionCheckDirectControl::ConditionCheckDirectControl()
{
	syndicateControl_ = false;
}

bool ConditionCheckDirectControl::check() const
{
	RealUnits::const_iterator ui;
	const RealUnits& realUnits = aiPlayer().realUnits();
	if(!syndicateControl_){
		FOR_EACH(realUnits, ui)
			if((*ui)->attr().isActing() && safe_cast<UnitActing*>(*ui)->isActiveDirectControl())
				return true;
	}
	else
		FOR_EACH(realUnits, ui)
			if((*ui)->attr().isActing() && safe_cast<UnitActing*>(*ui)->isActiveSyndicateControl())
				return true;
	return false;
}

void ConditionCheckDirectControl::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(syndicateControl_, "syndicateControl", "Синдикат-контрол");
}

bool ConditionMissionSelected::check() const
{
	return UI_LogicDispatcher::instance().currentMission() != 0;
}

bool ConditionNeedUpdate::check() const
{
	return UI_LogicDispatcher::instance().checkNeedUpdate();
}

ConditionCheckGameType::ConditionCheckGameType()
{
	gameType_ = GAME_TYPE_SCENARIO;
}

bool ConditionCheckGameType::check() const
{
	MissionDescription* mission = missionDescription_();
	if(!mission)
		return false;
	if(gameType_ & GAME_TYPE_REEL)
		return mission->gameType() & GAME_TYPE_REEL;
	if(gameType_ & GameType_Multiplayer)
		return mission->gameType() & GameType_Multiplayer;
    return (mission->gameType() & GameType_MASK) == gameType_;
}

void ConditionCheckGameType::serialize(Archive& ar)
{
	__super::serialize(ar);
	missionDescription_.serialize(ar);
	ar.serialize(gameType_, "gameType", "Тип игры");
}

ConditionEventComing::ConditionEventComing()
{
	eventType_ = Event::END_REPLAY;
}

void ConditionEventComing::checkEvent(const Event& event)
{
	if(event.type() == eventType_)
        setCheck();
}

void ConditionEventComing::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(eventType_, "eventType", "Тип события");
}

ConditionPlayerByNumberDefeat::ConditionPlayerByNumberDefeat()
{
	playerIndex = 0;
}

void ConditionPlayerByNumberDefeat::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(playerIndex, "playerIndex", "Номер игрока");
}

void ConditionEventString::checkEvent(const Event& event)
{
	if(event.type() == Event::STRING){
		const EventStringPlayer& eventString = safe_cast_ref<const EventStringPlayer&>(event);
		if(eventString.playerID() == aiPlayer().playerID() && !strcmp(eventString.name(), name_.c_str())) 
			setCheck();
	}
}

void ConditionEventString::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(name_, "name", "Строка");
	if(ar.isInput()){
		strlwr((char*)name_.c_str());
		cheatManager.addString(name_.c_str());
	}
}

ConditionCheckPause::ConditionCheckPause()
{
	pausedByUser_ = false;
	pausedByMenu_ = false;
}

void ConditionCheckPause::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(pausedByUser_, "pausedByUser", "Пауза игрока");
	ar.serialize(pausedByMenu_, "pausedByMenu", "Пауза меню");
}

void ConditionCompareSquadsCount::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(squads1_, "squads1", "Сквады1");
	removeZeros(squads1_);
	ar.serialize(squads2_, "squads2", "Сквады2");
	removeZeros(squads2_);
	ar.serialize(playerScanner_, "playerScanner", "Владелец объектов");
	ar.serialize(compare_, "compare", "Критерий сравнения");
}

ConditionCompareSquadsCount::ConditionCompareSquadsCount()
{
	compare_ = COMPARE_GREATER_EQ;
}

bool ConditionCompareSquadsCount::check() const
{
	int counter1 = 0;
	int counter2 = 0;
	do{
		Player& player = playerScanner_.player(aiPlayer());
		AttributeSquadReferences::const_iterator i;
		FOR_EACH(squads1_, i)
			counter1 += player.squadList(*i).size();
		FOR_EACH(squads2_, i)
			counter2 += player.squadList(*i).size();
	} while(playerScanner_.next(aiPlayer()));

	return compare(counter1, counter2, compare_);
}

AIPlayerScanner::AIPlayerScanner(AIPlayerType playerType)
{
	playerType_ = playerType;
	player_ = 0;
}

bool AIPlayerScanner::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(playerType_, name, nameAlt);
}

Player& AIPlayerScanner::player(Player& aiPlayer)
{
	if(!player_)
		next(aiPlayer);
	return *player_;
}

bool AIPlayerScanner::next(Player& aiPlayer)
{
	switch(playerType_)
	{
	case AI_PLAYER_TYPE_ME:
		player_ = &aiPlayer;
		return false;
	case AI_PLAYER_TYPE_WORLD:
		player_ = universe()->worldPlayer();
		return false;
	}

	bool cycle = false;
	int index = player_ ? player_->playerID() : -1;
	while(true){
		if(++index == universe()->Players.size()){
			index = 0;
			cycle = true;
		}
		player_ = universe()->Players[index];
		if(playerType_ == AI_PLAYER_TYPE_ANY || aiPlayer.isEnemy(player_))
			return !cycle;
	}
}

ConditionStatusTimeBase::ConditionStatusTimeBase() : Condition()
{
	time_ = 0;
}

bool ConditionStatusTimeBase::check() const
{
	if(checkStatus())
		timer_.start(time_*1000);
	else
		timer_.stop();

	return timer_.busy();
}

void ConditionStatusTimeBase::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(time_, "time", "Время проверки статуса (сек.)");
}

void ConditionCheckParameterProducing::serialize(Archive& ar)
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit())
		signalVariable_.setComboList(editSignalVariableDialog().c_str());
	ar.serialize(signalVariable_, "signalVariable", "&Имя сигнальной переменной");
}

bool ConditionCheckParameterProducing::check(UnitActing* unit) const
{
	if(unit->producedParameter() && unit->producedParameter()->signalVariable == signalVariable_.value())
		return true;
	return false;
}
