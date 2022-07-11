#include "StdAfx.h"
#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "Render\src\Scene.h"
#include "Render\src\VisGeneric.h"
#include "vmap.h"
#include "Universe.h"

#include "Squad.h"
#include "Serialization\Serialization.h"
#include "Serialization\SerializationFactory.h"
#include "ShowHead.h"
#include "PlayOgg.h"
#include "Serialization\ResourceSelector.h"

#include "Actions.h"
#include "Conditions.h"
#include "IronBuilding.h"
#include "Environment\SourceManager.h"
#include "Environment\Anchor.h"
#include "Water\Water.h"
#include "UserInterface\UI_Logic.h"
#include "Ai\PlaceOperators.h"
#include "Ai\PFTrap.h"
#include "UserInterface\SelectManager.h"
#include "Game\SoundApp.h"
#include "Serialization\RangedWrapper.h"
#include "GameOptions.h"
#include "Units\MicroAI.h"
#include "Units\Inventory.h"
#include "Sound\SoundSystem.h"
#include "FileUtils\FileUtils.h"
#include "Units\CommandsQueue.h"

#include "StreamCommand.h"
#include "EnginePrm.h"

#include "ScanningShape.h"
#include "Weapon.h"
#include "WeaponPrms.h"
#include "WBuffer.h"

// параметры поиска для установки здания
int ai_building_pause = 200; // ms
int ai_scan_step = 64;
float ai_scan_size_of_step_factor = 2;
int ai_scan_step_unable_to_find = 16; // При этом шаге прекращается сканирование, считая, что нужной позиции нет
int ai_placement_iterations_per_quant = 15;

// параметры атаки спец оружием 
float scanRadiusAim = 20.f;
int maxWeaponSpecialScan = 5;

// параметры апгрейда
const float stepOnConvexSide = 20.f;

string editSignalVariableDialog();

////////////////////////////////////////////////////
//		Actions
/////////////////////////////////////////////////////
STARFORCE_API void initActions()
{
SECUROM_MARKER_HIGH_SECURITY_ON(2);

REGISTER_CLASS(Action, Action, "Глобальные действия\\Пустое действие")
REGISTER_CLASS(Action, ActionDelay, "Глобальные действия\\Задержка времени")
REGISTER_CLASS(Action, ActionRestartTriggers, "Глобальные действия\\Сделать неактивными все стрелки триггера")
REGISTER_CLASS(Action, ActionStartTrigger, "Глобальные действия\\Запустить триггер")
REGISTER_CLASS(Action, ActionExitFromMission, "Глобальные действия\\Выход из миссии")
REGISTER_CLASS(Action, ActionGameQuit, "Глобальные действия\\Выход из игры")
REGISTER_CLASS(Action, ActionOnlineLogout, "Глобальные действия\\Выход из online (logout)")
REGISTER_CLASS(Action, ActionGameUpdateOpen, "Глобальные действия\\Открыть страницу для загрузки обновления")
REGISTER_CLASS(Action, ActionShowReel, "Глобальные действия\\Показать ролик")
REGISTER_CLASS(Action, ActionShowLogoReel, "Глобальные действия\\Показать программный ролик")
REGISTER_CLASS(Action, ActionStartMission, "Глобальные действия\\Запустить миссию")
REGISTER_CLASS(Action, ActionSetCurrentMission, "Глобальные действия\\Установить текущую миссию")
REGISTER_CLASS_CONVERSION(Action, ActionResetCurrentMission, "Глобальные действия\\Очистить текущую миссию", "struct ActionReseCurrentMission")
REGISTER_CLASS(Action, ActionSetCurrentMissionAsPassed, "Глобальные действия\\Считать текущую миссию пройденной")
REGISTER_CLASS(Action, ActionSwitchTriggers, "Глобальные действия\\Включить/Выключить триггера")
REGISTER_CLASS(Action, ActionSetPlayerWin, "Глобальные действия\\Считать игрока выигравшим");
REGISTER_CLASS(Action, ActionSetPlayerDefeat, "Глобальные действия\\Считать игрока проигравшим");
REGISTER_CLASS(Action, ActionSetInt, "Глобальные действия\\Установка целочисленной переменной")
REGISTER_CLASS(Action, ActionSetSignalVariable, "Глобальные действия\\Сигнальная переменная");
REGISTER_CLASS(Action, ActionSetCutScene, "Глобальные действия\\Включить/выключить кат-сцену (убирает игровую информацию)");
REGISTER_CLASS(Action, ActionReadPlayerParameters, "Глобальные действия\\Прочитать параметры профиля");
REGISTER_CLASS(Action, ActionWritePlayerParameters, "Глобальные действия\\Записать параметры профиля");

REGISTER_CLASS(Action, ActionUpgradeUnit, "Контекстные действия\\Апгрейд юнита определенного типа")
REGISTER_CLASS(Action, ActionAttackLabel, "Контекстные действия\\Копать в метке на мире")
REGISTER_CLASS(Action, ActionAttackBySpecialWeapon, "Контекстные действия\\Атаковать специальным оружием")
REGISTER_CLASS(Action, ActionActivateSpecialWeapon, "Контекстные действия\\Активировать/Деактивировать спец. оружие")
REGISTER_CLASS(Action, ActionAttack, "Контекстные действия\\Атаковать юнитами")
REGISTER_CLASS(Action, ActionEscapeUnderShield, "Контекстные действия\\Выйти из под поля")
REGISTER_CLASS(Action, ActionAttackMyUnit, "Контекстные действия\\Атаковать юнитами своих юнитов")
REGISTER_CLASS(Action, ActionGuardUnit, "Контекстные действия\\Охранять юнита")
REGISTER_CLASS(Action, ActionSetWalkMode, "Контекстные действия\\Режим движения сквада")
REGISTER_CLASS(Action, ActionSetUnitAttackMode, "Контекстные действия\\Включить режим атаки юнита");
REGISTER_CLASS(Action, ActionReturnToBase, "Контекстные действия\\Отходить на базу");
REGISTER_CLASS(Action, ActionEscapeWater, "Контекстные действия\\Отходить на сушу")
REGISTER_CLASS(Action, ActionAIUnitCommand, "Контекстные действия\\Послать команду")
REGISTER_CLASS(Action, ActionPickResource, "Контекстные действия\\Добывать ресурс");
REGISTER_CLASS(Action, ActionSquadMoveToItem, "Контекстные действия\\Послать сквад к предмету")
REGISTER_CLASS(Action, ActionSquadMoveToObject, "Контекстные действия\\Послать сквад к объекту")
REGISTER_CLASS(Action, ActionFollowSquad, "Контекстные действия\\Следовать за сквадом на расстоянии") 
REGISTER_CLASS(Action, ActionJoinSquads, "Контекстные действия\\Объединить сквады") 
REGISTER_CLASS(Action, ActionSplitSquad, "Контекстные действия\\Разъединить сквад") 
REGISTER_CLASS(Action, ActionExploreArea, "Контекстные действия\\Разведать территорию")
REGISTER_CLASS(Action, ActionPutUnitInTransport, "Контекстные действия\\Садиться в транспорт")
REGISTER_CLASS(Action, ActionOutUnitFromTransport, "Контекстные действия\\Освободить транспорт от юнитов")
REGISTER_CLASS(Action, ActionUnitClearOrders, "Контекстные действия\\Отменить все приказы данные юниту");
REGISTER_CLASS(Action, ActionOrderUnitsInSquad, "Контекстные действия\\Заказать юнита в сквад")
REGISTER_CLASS(Action, ActionSetUnitInvisible, "Контекстные действия\\Сделать юнита невидимым")
REGISTER_CLASS(Action, ActionSetUnitSelectAble, "Контекстные действия\\Вкл./выкл. возможность селекта юнита")
REGISTER_CLASS(Action, ActionSellBuilding, "Контекстные действия\\Продать здание")
REGISTER_CLASS(Action, ActionOrderBuildings, "Контекстные действия\\Заказать здание")
REGISTER_CLASS(Action, ActionContinueConstruction, "Контекстные действия\\Достроить здания")
REGISTER_CLASS(Action, ActionSquadMoveToAnchor, "Контекстные действия\\Отправить сквад в метку на якоре")
REGISTER_CLASS(Action, ActionSquadMoveToAssemblyPoint, "Контекстные действия\\Отправить сквад в точку поддержки")
REGISTER_CLASS(Action, ActionPutSquadToAnchor, "Контекстные действия\\Переместить сквад в метку на якоре")
REGISTER_CLASS(Action, ActionSetCameraAtObject, "Контекстные действия\\Установить камеру на объект")
REGISTER_CLASS(Action, ActionSetObjectAnimation, "Контекстные действия\\Вкл./выкл. анимацию объекта")
REGISTER_CLASS(Action, ActionSetIgnoreFreezedByTrigger, "Контекстные действия\\Вкл./выкл. игнорирование общей заморозки для отдельного объекта")
REGISTER_CLASS(Action, ActionObjectParameterArithmetics, "Контекстные действия\\Арифметика параметров юнита")
REGISTER_CLASS(Action, ActionSetUnitLevel, "Контекстные действия\\Установить уровень юнита(не влияет на параметры)")
REGISTER_CLASS(Action, ActionSetUnitMinimapMark, "Контекстные действия\\Включить/выключить специальную пометку юнита")

REGISTER_CLASS(Action, ActionOrderBuildingsOnZone, "АИ\\Заказать здание на зоне")
REGISTER_CLASS(Action, ActionOrderBuildingCloseToEnemy, "АИ\\Заказать здание ближе к врагу или откл. зданию")
REGISTER_CLASS(Action, ActionOrderUnits, "АИ\\Заказать юнитов на заводе")
REGISTER_CLASS(Action, ActionOrderParameters, "АИ\\Производство параметров")
REGISTER_CLASS(Action, ActionUnitParameterArithmetics, "АИ\\Арифметика параметров")

REGISTER_CLASS(Action, ActionCommandsQueue, "Миссии\\Запустить очередь команд на выполнение")
REGISTER_CLASS(Action, ActionInterruptCommandsQueue, "Миссии\\Прервать очередь команд")
REGISTER_CLASS(Action, ActionCreateUnit, "Миссии\\Создать объект в точке якоря")
REGISTER_CLASS(Action, ActionKillUnits, "Миссии\\Удалить объекты")
REGISTER_CLASS(Action, ActionSwitchPlayer, "Миссии\\Переключиться на игрока")
REGISTER_CLASS(Action, ActionActivateObjectByLabel, "Миссии\\Активировать объект по метке")
REGISTER_CLASS(Action, ActionDeactivateObjectByLabel, "Миссии\\Деактивировать объект по метке")
REGISTER_CLASS(Action, ActionSetControlEnabled, "Миссии\\Запретить/разрешить управление игрока")
REGISTER_CLASS(Action, ActionSetFreezedByTrigger, "Миссии\\Заморозить/разморозить юнитов для кат-сцены")
REGISTER_CLASS(Action, ActionEnableMessage, "Миссии\\Запретить/разрешить сообщения определенного типа")
REGISTER_CLASS(Action, ActionEnableSounds, "Миссии\\Управление звуком")
REGISTER_CLASS(Action, ActionMessage, "Миссии\\Сообщение")
REGISTER_CLASS(Action, ActionInterrruptMessage, "Миссии\\Прервать сообщение")
REGISTER_CLASS(Action, ActionInterruptAnimation, "Миссии\\Прервать анимацию")
REGISTER_CLASS(Action, ActionTask, "Миссии\\Задача")
REGISTER_CLASS(Action, ActionSelectUnit, "Миссии\\Селектировать юнита")
REGISTER_CLASS(Action, ActionDeselect, "Миссии\\Сброс селекта")
REGISTER_CLASS(Action, ActionSoundMessage, "Миссии\\Звуковое сообщение(2D)")
REGISTER_CLASS(Action, ActionShowHead, "Миссии\\Включить анимацию головы")

REGISTER_CLASS(Action, ActionSetCamera, "Камера\\Установка Камеры")
REGISTER_CLASS(Action, ActionSetDefaultCamera, "Камера\\Установка стандартной камеры")
REGISTER_CLASS(Action, ActionOscillateCamera, "Камера\\Тряска Камеры")
REGISTER_CLASS(Action, ActionSetCameraFromObject, "Камера\\Отключать камеру от объекта")
REGISTER_CLASS(Action, ActionSetCameraAtSquad, "Камера\\Установить камеру на сквад")
REGISTER_CLASS(Action, ActionSetCameraRestriction, "Камера\\Ограничения камеры")

REGISTER_CLASS(Action, ActionSetDirectControl, "Интерфейс\\Включить/Выключить прямое/синдикатное управление")
REGISTER_CLASS(Action, ActionSetGamePause, "Интерфейс\\Включить/Выключить паузу игры")
REGISTER_CLASS(Action, ActionSave, "Интерфейс\\Сохранить игру")
REGISTER_CLASS(Action, ActionSaveAuto, "Интерфейс\\Сохранить игру (автосейв)")
REGISTER_CLASS(Action, ActionLoadGameAuto, "Интерфейс\\Загрузить игру (автосейв)")
REGISTER_CLASS(Action, ActionSetInterface, "Интерфейс\\Включить/выключить интерфейс")
REGISTER_CLASS(Action, ActionSelectInterfaceScreen, "Интерфейс\\Включить экран интерфейса")
REGISTER_CLASS(Action, ActionUI_ScreenSwitchOff, "Интерфейс\\Выключить экран интерфейса")
REGISTER_CLASS(Action, ActionInterfaceHideControl, "Интерфейс\\Спрятать кнопку мягко, ПЕРЕДЕЛАТЬ на /Операции над кнопками/")
REGISTER_CLASS(Action, ActionInterfaceHideControlTrigger, "Интерфейс\\Спрятать кнопку жестко, ПЕРЕДЕЛАТЬ на /Операции над кнопками/")
REGISTER_CLASS(Action, ActionInterfaceControlOperate, "Интерфейс\\Операции над кнопками")
REGISTER_CLASS(Action, ActionInterfaceTogglAccessibility, "Интерфейс\\Переключить доступность кнопки")
REGISTER_CLASS(Action, ActionInterfaceSetControlState, "Интерфейс\\Переключить состояние кнопки")
REGISTER_CLASS(Action, ActionCreateNetClient, "Интерфейс\\Создать сетевой клиент")
REGISTER_CLASS(Action, ActionUI_GameStart, "Интерфейс\\Старт игры")
REGISTER_CLASS(Action, ActionUI_LanGameStart, "Интерфейс\\Старт сетевой игры")
REGISTER_CLASS(Action, ActionUI_LanGameJoin, "Интерфейс\\Присоединение к сетевой игре")
REGISTER_CLASS(Action, ActionUI_LanGameCreate, "Интерфейс\\Создание сетевой игры")
REGISTER_CLASS(Action, ActionUI_UnitCommand, "Интерфейс\\Контекстная команда заселекченным юнитам")
REGISTER_CLASS(Action, ActionResetNetCenter, "Интерфейс\\Завершить сетевую игру")
REGISTER_CLASS(Action, ActionKillNetCenter, "Интерфейс\\Завершить работу с сетью")
REGISTER_CLASS(Action, ActionToggleBuildingInstaller, "Интерфейс\\Включить режим установки здания")
REGISTER_CLASS(Action, ActionUI_ConfirmDiskOp, "Интерфейс\\Подтверждение перезаписи или удаления")
REGISTER_CLASS(Action, ActionUI_InventoryQuickAccessMode, "Интерфейс\\Режим работы инвентаря быстрого доступа")

REGISTER_CLASS(Action, ActionSetCursor, "Курсоры\\Установить курсор")
REGISTER_CLASS(Action, ActionFreeCursor, "Курсоры\\Отменить установленный курсор")
REGISTER_CLASS(Action, ActionChangeUnitCursor, "Курсоры\\Сменить курсор юнита")

SECUROM_MARKER_HIGH_SECURITY_OFF(2);
}

////////////////////////////////////////////////////

BEGIN_ENUM_DESCRIPTOR(AttackCondition, "AttackCondition")
REGISTER_ENUM(ATTACK_GROUND, "Атаковать поверхность")
REGISTER_ENUM(ATTACK_ENEMY_UNIT, "Атаковать юнитов врага(с учетом здоровья)")
REGISTER_ENUM(ATTACK_GROUND_NEAR_ENEMY_UNIT, "Атаковать поверхность рядом с юнитом врага")
REGISTER_ENUM(ATTACK_GROUND_NEAR_MY_UNIT, "Атаковать поверхность рядом с моим юнитом")
REGISTER_ENUM(ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING, "Атаковать поверхность рядом с юнитом врага продолжительное")
REGISTER_ENUM(ATTACK_MY_UNIT, "Атаковать своих поврежденных юнитов")
END_ENUM_DESCRIPTOR(AttackCondition)

BEGIN_ENUM_DESCRIPTOR(UpgradeOption, "UpgradeOption")
REGISTER_ENUM(UPGRADE_HERE, "В любой обстановке")
REGISTER_ENUM(UPGRADE_ON_THE_DISTANCE, "На расстоянии от любого здания")
REGISTER_ENUM(UPGRADE_ON_THE_DISTANCE_TO_ENEMY, "На расстоянии от базы в сторону врага")
REGISTER_ENUM(UPGRADE_NEAR_OBJECT, "Юнит находится рядом с объектом")
REGISTER_ENUM(UPGRADE_ON_THE_DISTANCE_FROM_ENEMY, "На расстоянии от базы врага")
REGISTER_ENUM(UPGRADE_ON_THE_ENEMY_DIRECTION, "На расстоянии от границы базы строго к центру базы врага")
REGISTER_ENUM(UPGRADE_FROM_UNIT_ON_THE_ENEMY_DIRECTION, "В направлении к центру базы врага с текущего положения")
REGISTER_ENUM(UPGRADE_ON_THE_DISCONNECTED_DIRECTION, "В направлении отключенного здания с текущего положения")
END_ENUM_DESCRIPTOR(UpgradeOption)

BEGIN_ENUM_DESCRIPTOR(AIPlayerType, "AIPlayerType")
REGISTER_ENUM(AI_PLAYER_TYPE_ME, "Я")
REGISTER_ENUM(AI_PLAYER_TYPE_ENEMY, "Враг")
REGISTER_ENUM(AI_PLAYER_TYPE_WORLD, "Мир")
REGISTER_ENUM(AI_PLAYER_TYPE_ANY, "Любой")
END_ENUM_DESCRIPTOR(AIPlayerType)

BEGIN_ENUM_DESCRIPTOR(SwitchMode, "SwitchMode")
REGISTER_ENUM(ON, "Включить")
REGISTER_ENUM(OFF, "Выключить")
END_ENUM_DESCRIPTOR(SwitchMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionSetSignalVariable, Acting, "Acting")
REGISTER_ENUM_ENCLOSED(ActionSetSignalVariable, ACTION_ADD, "Добавить")
REGISTER_ENUM_ENCLOSED(ActionSetSignalVariable, ACTION_REMOVE, "Удалить")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionSetSignalVariable, Acting)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionAttack, AimObject, "AimObject")
REGISTER_ENUM_ENCLOSED(ActionAttack, AIM_UNIT, "Юниты")
REGISTER_ENUM_ENCLOSED(ActionAttack, AIM_BUILDING, "Здания")
REGISTER_ENUM_ENCLOSED(ActionAttack, AIM_ANY, "Все")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionAttack, AimObject)

BEGIN_ENUM_DESCRIPTOR(SwitchModeTriple, "SwitchModeTriple")
REGISTER_ENUM(MODE_ON, "Включить")
REGISTER_ENUM(MODE_OFF, "Выключить")
REGISTER_ENUM(MODE_RESTORE, "Восстановить")
END_ENUM_DESCRIPTOR(SwitchModeTriple)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionEnableSounds, SoundType, "SoundType")
REGISTER_ENUM_ENCLOSED(ActionEnableSounds, TYPE_SOUND, "Звуки")
REGISTER_ENUM_ENCLOSED(ActionEnableSounds, TYPE_VOICE, "Голосовые сообщения")
REGISTER_ENUM_ENCLOSED(ActionEnableSounds, TYPE_MUSIC, "Музыка")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionEnableSounds, SoundType)


BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionSwitchTriggers, Mode, "Mode")
REGISTER_ENUM_ENCLOSED(ActionSwitchTriggers, SWITCH_ON_CURRENT_PLAYER_AI, "Включить АИ триггера для текущего игрока")
REGISTER_ENUM_ENCLOSED(ActionSwitchTriggers, SWITCH_OFF_CURRENT_PLAYER_AI, "Выключить АИ триггера для текущего игрока")
REGISTER_ENUM_ENCLOSED(ActionSwitchTriggers, SWITCH_OFF_ALL_PLAYERS_AI, "Выключить АИ триггера для всех игроков")
REGISTER_ENUM_ENCLOSED(ActionSwitchTriggers, SWITCH_OFF_CURRENT_PLAYER_TRIGGERS, "Выключить для текущего игрока все триггера")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionSwitchTriggers, Mode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionMessage, Type, "ActionMessage::Type")
REGISTER_ENUM_ENCLOSED(ActionMessage, MESSAGE_ADD, "Добавить в список")
REGISTER_ENUM_ENCLOSED(ActionMessage, MESSAGE_REMOVE, "Удалить из списка")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionMessage, Type)

BEGIN_ENUM_DESCRIPTOR(SquadMoveMode, "SquadMoveMode")
REGISTER_ENUM(DO_NOT_WAIT, "Не ждать")
REGISTER_ENUM(WAIT_FOR_ONE, "Ждать пока дойдет хотя бы один юнит")
REGISTER_ENUM(WAIT_FOR_ALL, "Ждать пока дойдут все юниты")
END_ENUM_DESCRIPTOR(SquadMoveMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionOrderBuildingCloseToEnemy, BuildState, "ActionOrderBuildingCloseToEnemy::BuildState")
REGISTER_ENUM_ENCLOSED(ActionOrderBuildingCloseToEnemy, BuildToEnemyCenter, "В направлении центра базы врага")
REGISTER_ENUM_ENCLOSED(ActionOrderBuildingCloseToEnemy, BuildToDisconnected, "В направлении отключенного здания")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionOrderBuildingCloseToEnemy, BuildState)

//SNDSound ActionSoundMessage::ctrl;

ActionOrderBuildingCloseToEnemy::ActionOrderBuildingCloseToEnemy()
{
	maxOrientBuildings_ = 5;
	indexOrientBuilding_ = 0;
	bestPosition_ = Vect2f::ZERO;
	buildState_ = BuildToEnemyCenter;
	playerType_ = AI_PLAYER_TYPE_ME;
}
Vect2f ActionOrderBuildingCloseToEnemy::nearestDisconnected() const
{
	RealUnits unitList;
	switch(playerType_)
	{
	case AI_PLAYER_TYPE_ME:
		{
			const RealUnits& units = attrBuildingToDisconnected_ ?
				aiPlayer().realUnits(attrBuildingToDisconnected_) : aiPlayer().realUnits();
			unitList.insert(unitList.end(), units.begin(), units.end());
		}
		break;
	case AI_PLAYER_TYPE_ENEMY:
		{
			PlayerVect::iterator pi;
			FOR_EACH(universe()->Players, pi)
				if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
				{
					const RealUnits& units = attrBuildingToDisconnected_ ?
						(*pi)->realUnits(attrBuildingToDisconnected_) : (*pi)->realUnits(); 
					unitList.insert(unitList.end(), units.begin(), units.end());
				}
		}
		break;
	case AI_PLAYER_TYPE_WORLD:
		{
			const RealUnits& units = attrBuildingToDisconnected_ ?
				universe()->worldPlayer()->realUnits(attrBuildingToDisconnected_) : universe()->worldPlayer()->realUnits();
			unitList.insert(unitList.end(), units.begin(), units.end());
		}
	    break;
	case AI_PLAYER_TYPE_ANY:
		{
			PlayerVect::iterator pi;
			FOR_EACH(universe()->Players, pi)
			{
				const RealUnits& units = attrBuildingToDisconnected_ ?
					(*pi)->realUnits(attrBuildingToDisconnected_) : (*pi)->realUnits();
				unitList.insert(unitList.end(), units.begin(), units.end());
			}
		}
		break;
	}

	if(unitList.empty())
		return Vect2f::ZERO;

	Vect2f aimPos = Vect2f::ZERO;
	float maxDistance = FLT_INF;
	RealUnits::const_iterator i;
	FOR_EACH(unitList, i)
		if(!(*i)->isConnected())
		{
			float dist = aiPlayer().centerPosition().distance2((*i)->position2D());
			if(dist < maxDistance)
			{
				aimPos = (*i)->position2D();
				maxDistance = dist;
			}
		}

	return aimPos;
}

Vect2f ActionOrderBuildingCloseToEnemy::nearestEnemyCenter() const
{
	PlayerVect::iterator pi;
	float maxDistance = FLT_INF;
	Vect2f center = Vect2f::ZERO;
	FOR_EACH(universe()->Players, pi)
		if(aiPlayer().isEnemy(*pi) && !(*pi)->isWorld()){
			Vect2f enemyCenter = (*pi)->centerPosition();
			if(enemyCenter.eq(Vect2f::ZERO, FLT_EPS))
				continue;
			float dist = enemyCenter.distance2(aiPlayer().centerPosition());
			if(dist < maxDistance)
			{
				center = enemyCenter;
				maxDistance = dist;
			}
		}
	return center;
}

void ActionOrderBuildingCloseToEnemy::createOrientBuildingsList(Vect2f& aimPos) const
{
	orientBuildings_.clear();
	const RealUnits& units = aiPlayer().realUnits(attrBuildingToOrient_);
	RealUnits::const_iterator ui;
	float maxDistance = FLT_INF;
	FOR_EACH(units, ui){
		float dist = (*ui)->position2D().distance2(aimPos);
		if(dist <= maxDistance)
		{
			maxDistance = dist;
			orientBuildings_.insert(orientBuildings_.begin(), *ui);
			while(orientBuildings_.size() > maxOrientBuildings_)
				orientBuildings_.erase(orientBuildings_.end() - 1);
		}
	}	
}

void ActionOrderBuildingCloseToEnemy::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrBuildingToOrient_, "attrBuildingToOrient", "Тип здания рядом с которым заказывать");
	ar.serialize(attrBuildingToOrder_, "attrBuildingToOrder", "Тип заказываемого здания");
	ar.serialize(buildState_, "buildState", "Критерий близости");
	ar.serialize(attrBuildingToDisconnected_, "attrBuildingToDisconnected", "Тип отключенного здания");
	ar.serialize(playerType_ , "playerType_", "Владелец отключенного здания");
}

bool ActionOrderBuildingCloseToEnemy::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(!attrBuildingToOrient_)
		return false;

	if(!indexOrientBuilding_){
		Vect2f aimPos = Vect2f::ZERO;
		switch(buildState_)
		{
		case BuildToEnemyCenter:
			aimPos = nearestEnemyCenter();
			break;
		case BuildToDisconnected:
			aimPos = nearestDisconnected();
			break;
		}	
		if(aimPos.eq(Vect2f::ZERO))
			return false;

		createOrientBuildingsList(aimPos);
		if(orientBuildings_.empty())
			return false;
	}

	// проверка условий возможности строительства по ресурсам
	if(!aiPlayer().checkUnitNumber(attrBuildingToOrder_))
		return false;
	if(!aiPlayer().accessible(attrBuildingToOrder_))
		return false;
	if(!aiPlayer().requestResource(attrBuildingToOrder_->installValue, NEED_RESOURCE_TO_INSTALL_BUILDING))
		return false;

	UnitReal* unit = 0;
	while(indexOrientBuilding_ < orientBuildings_.size()){
        unit = orientBuildings_[indexOrientBuilding_];
		if(unit)
			break;
		else
			indexOrientBuilding_++;
		if(indexOrientBuilding_ == orientBuildings_.size()){
			indexOrientBuilding_ = 0;
			return false;
		}
	}
	if(unit){
		int radius = min(attrBuildingToOrient_.get()->basementExtent.x, attrBuildingToOrient_->basementExtent.y);
		while(radius < 100.f)
		{
			PlaceScanOp scanOp(unit, attrBuildingToOrder_, aiPlayer(), false, radius);
			scanOp.checkPosition(unit->position2D());
			if(scanOp.found())
			{
				bestPosition_ = scanOp.bestPosition();
				return true;
			}
			radius += 10.f;
		}

		indexOrientBuilding_++;
		if(indexOrientBuilding_ == orientBuildings_.size()){
			indexOrientBuilding_ = 0;
		}
	}
	aiPlayer().checkEvent(EventUnitAttributePlayer(Event::UNABLE_BUILD, attrBuildingToOrder_, &aiPlayer()));

	return false;
}

void ActionOrderBuildingCloseToEnemy::activate()
{
	Vect2f snapPoint;
	if(!bestPosition_.eq(Vect2f::ZERO) && safe_cast<const AttributeBuilding*>(&*attrBuildingToOrder_)->checkBuildingPosition(bestPosition_, Mat2f::ID, &aiPlayer(), true, snapPoint))
		aiPlayer().buildStructure(attrBuildingToOrder_, Vect3f(bestPosition_.x, bestPosition_.y, 0.f), false);
	
	bestPosition_ = Vect2f::ZERO;
	indexOrientBuilding_ = 0;
}

ActionEnableMessage::ActionEnableMessage()
{
	mode_ = ON;
}

void ActionEnableMessage::activate()
{
	switch(mode_){
		case ON:
			{
				universe()->disabledMessages().remove(messageType_);
			}
			break;
		case OFF:
			universe()->disabledMessages().add(messageType_);
	}	

}

void ActionEnableMessage::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(messageType_, "messageType", "Тип сообщения");
	ar.serialize(mode_, "mode", "Действие");
}

void ActionGameUpdateOpen::activate()
{
	UI_LogicDispatcher::instance().openUpdateUrl();
}

void fCommandSwitchPlayer(XBuffer& stream)
{
	int playerID = 0;
	stream.read(playerID);
	universe()->setActivePlayer(playerID);
	universe()->activePlayer()->setAI(false);
	universe()->activePlayer()->setRealPlayerType(REAL_PLAYER_TYPE_PLAYER);
}

ActionSwitchPlayer::ActionSwitchPlayer()
{
	playerID = 0;
}

void ActionSwitchPlayer::activate()
{
	streamLogicCommand.set(fCommandSwitchPlayer);
	streamLogicCommand << playerID;
}

void ActionSwitchPlayer::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(playerID, "playerID", "Номер игрока");
}

ActionSetCameraRestriction::ActionSetCameraRestriction()
{
	switchMode_ = MODE_ON;
}

void ActionSetCameraRestriction::serialize(Archive& ar)
{
	__super::serialize(ar);
	SwitchMode switchMode;
	if(ar.isInput() && ar.serialize(switchMode, "switchMode", 0)){ // CONVERSION 05.10.06
		if(switchMode == ON)
			switchMode_ = MODE_ON;
		else
			switchMode_ = MODE_OFF;
	}
	else
		ar.serialize(switchMode_, "switchMode_", "Действие");
}

void ActionSetCameraRestriction::activate()
{
	switch(switchMode_){
		case MODE_ON:
			cameraManager->setRestriction(true);
			break;
		case MODE_OFF:
			cameraManager->setRestriction(false);
			break;
		case MODE_RESTORE:
			cameraManager->setRestriction(GameOptions::instance().getBool(OPTION_CAMERA_RESTRICTION));
	}
}

void ActionSetDefaultCamera::activate()
{
	cameraManager->splineToDefaultCoordinate(duration_, cameraManager->coordinate().position());
}

void ActionSetDefaultCamera::serialize( Archive& ar )
{
	__super::serialize(ar);
	ar.serialize(duration_, "duration", "Время перехода");
}

class NearestFreeTransportScanOp
{
public:
	NearestFreeTransportScanOp(UnitReal* unit1, const AttributeBase* attribute2, float distance)
	: unit1_(unit1), attribute2_(attribute2), distance2_(sqr(distance)), found_(false) 
	{
		unit_ = 0;
		distBest = FLT_INF;
	}

	bool operator()(UnitBase* unit2)
	{
		float dist = 0.f;
		if(unit2->alive() && unit2 != unit1_ && &unit2->attr() == attribute2_ 
		  && (dist = unit1_->position2D().distance(unit2->position2D()) < distance2_) 
		  && dist < distBest 
		  && unit2->attr().isActing() && safe_cast<UnitActing*>(unit2)->canPutInTransport(unit1_)){
			distBest = dist;
			found_ = true; 
			unit_ = safe_cast<UnitReal*>(unit2);
		}
		return true;
	}

	bool found() const { return found_ && unit_; }
	UnitReal* unit() const { return unit_; }
private:
	UnitReal* unit1_;
	const AttributeBase* attribute2_;
	float distance2_;
	float distBest;
	bool found_;
	UnitReal* unit_;
};

ActionCreateUnit::ActionCreateUnit() 
{
	player_ = -1;
	count_ = 1;
	inTheSameSquad_ = false;
}

void ActionCreateUnit::activate()
{
	if(!anchor_){
		xassertStr(0 && "Якорь по метке не найден: ", anchor_.c_str());
		return;
	}												
	else{
		Player* player = 0;
		if(player_ == -1)
			player = &aiPlayer();
		else
			player = universe()->findPlayer(player_);
		if(player){
			if(!attr_){
				xassert(0 && "Создать объект в точке якоря, не указан тип юнита!!!");
				return;
			}
			UnitSquad* squad = 0;
			for(int i = 0; i < count_; i++){
				UnitReal* unit = safe_cast<UnitReal*>(player->buildUnit(attr_));
				Vect3f pos = anchor_->position();				
				if(count_ > 1)
					pos += Mat3f(2.0f*M_PI*i/count_, Z_AXIS)*Vect3f(unit->radius()*2.0f, 0.0f, 0.0f);
				unit->setPose(Se3f(anchor_->pose().rot(), pos),true);
				if(unit->attr().isLegionary()){
					UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
					if(!squad || !inTheSameSquad_ || !squad->checkFormationConditions(legionary->attr().formationType)){
						squad = safe_cast<UnitSquad*>(player->buildUnit(&*legionary->attr().squad));
						squad->setPose(unit->pose(), true);
					}
					squad->addUnit(legionary);
				}
			}
		}
	}
}

void ActionCreateUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|anchorLabel_", "&Метка(на якоре)");
	ar.serialize(attr_, "attr", "Тип юнита");
	ar.serialize(count_, "count", "Количество");
	ar.serialize(inTheSameSquad_, "inTheSameSquad", "В одном скваде");
	ar.serialize(player_, "player", "Номер игрока");
}

void ActionKillUnits::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(units_, "units", "Юниты");
}

void ActionKillUnits::activate()
{
	AttributeUnitOrBuildingReferences::iterator i;
	FOR_EACH(units_, i){
		const RealUnits& units = aiPlayer().realUnits(*i);
		RealUnits::const_iterator ui;
		FOR_EACH(units, ui)
			(*ui)->Kill();
	}
}

void ActionSquadMoveToItem::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrItem, "attrItem", "Тип предмета");
}

void ActionSquadMoveToObject::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrObject_, "attrObject", "Тип объекта");
	ar.serialize(aiPlayerType_, "aiPlayerType", "Владелец объекта");
}

bool ActionSquadMoveToItem::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	unit_ = findNearestItem(unit);

	return unit_ && unit_->position2D().distance2(unit->position2D()) > sqr(unit_->radius() +  unit->radius())*sqr(1.5f);
}

bool ActionSquadMoveToObject::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	unit_ = findNearestObject(unit);

	return unit_ && unit_->position2D().distance2(unit->position2D()) > sqr(unit_->radius() +  unit->radius())*sqr(1.5f);
}

ActionSquadMoveToObject::ActionSquadMoveToObject()
{
	unit_ = 0;
	aiPlayerType_ = AI_PLAYER_TYPE_ME;
}

ActionSquadMoveToItem::ActionSquadMoveToItem()
{
	unit_ = 0;
}

void ActionSquadMoveToObject::activate()
{
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, unit_->position(), 0));
	contextSquad_->setUsedByTrigger(priority_, this);
}

void ActionSquadMoveToItem::activate()
{
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, unit_->position(), 0));
	contextSquad_->setUsedByTrigger(priority_, this);
}

UnitActing* ActionSquadMoveToObject::nearestUnit(const Vect2f& pos, const RealUnits& unitList) const
{
	UnitActing* bestUnit = 0;
	float bestDist = FLT_INF;
	float dist = 0;

	if(!unitList.empty()) {
		RealUnits::const_iterator ui = unitList.begin();
		while(ui != unitList.end()) {
			if((*ui)->attr().isActing() && !(*ui)->isUnseen() && bestDist > (dist = pos.distance2((*ui)->position2D())) && dist > 0){
					bestDist = dist;
					bestUnit = safe_cast<UnitActing*>(*ui);
			}
			ui++;
		}
	}

	return bestUnit;
}

UnitActing* ActionSquadMoveToObject::findNearestObject(UnitActing* unit) const
{
	Vect2f pos = unit->position2D();
	UnitActing* bestUnit = 0;
	float bestDist = FLT_INF;
	float dist = 0;

	switch(aiPlayerType_){
		case AI_PLAYER_TYPE_ME:{
			const RealUnits& unitList = attrObject_ ? aiPlayer().realUnits(attrObject_) : aiPlayer().realUnits();
			bestUnit = nearestUnit(pos, unitList);
			break;
		}
		case AI_PLAYER_TYPE_ENEMY:{
			UnitActing* tmpUnit = 0;
			PlayerVect::iterator pi;
			FOR_EACH(universe()->Players, pi){
				if(!(*pi)->isWorld()&& (*pi)->isEnemy(&aiPlayer())){
					const RealUnits& unitList = attrObject_ ? (*pi)->realUnits(attrObject_) : (*pi)->realUnits();
					tmpUnit = nearestUnit(pos, unitList);
					if(tmpUnit && bestDist > (dist = pos.distance2(tmpUnit->position2D()))){
						bestDist = dist;
						bestUnit = tmpUnit;
					}
				}
			}
			break;
		}
		case AI_PLAYER_TYPE_WORLD:{ 
			const RealUnits& unitList = attrObject_ ? universe()->worldPlayer()->realUnits(attrObject_) : universe()->worldPlayer()->realUnits();
			bestUnit = nearestUnit(pos, unitList);
			break;
		}
		case AI_PLAYER_TYPE_ANY:{
			UnitActing* tmpUnit = 0;
			PlayerVect::iterator pi;
			FOR_EACH(universe()->Players, pi){
				const RealUnits& unitList = attrObject_ ? (*pi)->realUnits(attrObject_) : (*pi)->realUnits();
				tmpUnit = nearestUnit(pos, unitList);
				if(tmpUnit && bestDist > (dist = pos.distance2(tmpUnit->position2D()))){
					bestDist = dist;
					bestUnit = tmpUnit;
				}
			}
		}
	}

	return bestUnit;
}

UnitReal* ActionSquadMoveToItem::findNearestItem(UnitReal* unit) const
{
	Vect2f pos = unit->position2D();
	UnitReal* bestUnit = 0;
	float bestDist = FLT_INF;
	float dist = 0;

	const RealUnits& itemList = universe()->worldPlayer()->realUnits(attrItem);
	if(!itemList.empty()) {
		RealUnits::const_iterator ri = itemList.begin();
		while(ri != itemList.end()) {
			if(!(*ri)->isUnseen() && bestDist > (dist = pos.distance2((*ri)->position2D()))){
					bestDist = dist;
					bestUnit = *ri;
			}
			ri++;
		}
	}
	return bestUnit;
}

bool ActionShowHead::automaticCondition() const
{
	return __super::automaticCondition() && aiPlayer().active(); 
}

void ActionSetCameraFromObject::activate()
{
    cameraManager->SetCameraFollow();
	cameraManager->setCoordinate(cameraManager->coordinate());
}

bool ActionSoundMessage::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(!soundReference || soundReference->is3D() || (!aiPlayer().active() && !aiPlayer().isWorld()))
		return false;
	return true;
}

void ActionSoundMessage::activate() 
{
	if(!SNDIsSoundEnabled())
		return;

	switch(switchMode_)
	{
	case ON:
		universe()->soundEnvironmentManager()->playSound(soundReference);
		break;
	case OFF:
		universe()->soundEnvironmentManager()->stopSound(soundReference);
	}
}

void ActionSoundMessage::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchMode_, "switchMode", "Действие");
	ar.serialize(soundReference, "soundReference", "Звук");
}

void ActionUnitClearOrders::activate()
{
    contextSquad_->clearOrders();
	contextSquad_->setUsedByTrigger(0, this);
}

void ActionSetUnitSelectAble::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchMode_, "switchMode_", "Режим");
}

void ActionSetUnitInvisible::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchMode_, "switchMode_", "Режим");
}

bool ActionSetUnitSelectAble::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit) || !unit->attr().isLegionary())
		return false;

	UnitLegionary* unitLegionary = safe_cast<UnitLegionary*>(unit);
	switch(switchMode_){
		case ON:
			if(unitLegionary->selectAble())
				return false;
			break;
		case OFF:
			if(!unitLegionary->selectAble())
				return false;
	}

	return true;
}

bool ActionSetUnitInvisible::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit) || !unit->attr().canChangeVisibility)
		return false;

	switch(switchMode_){
		case ON:
			if(unit->isUnseen())
				return false;
			break;
		case OFF:
			if(!unit->isUnseen())
				return false;
	}

	return true;
}

void ActionSetUnitSelectAble::activate()
{
	UnitLegionary* unit = safe_cast<UnitLegionary*>(&*contextUnit_);
	switch(switchMode_){
		case ON:
			unit->setSelectAble(true);
			break;
		case OFF:
			unit->setSelectAble(false);
	}
}

void ActionSetUnitInvisible::activate()
{
	switch(switchMode_){
		case ON:
			contextUnit_->setVisibility(false, 10000000);
			break;
		case OFF:
			contextUnit_->setVisibility(true, 10000000);
	}
}

////////////////////////////////////////////////////
void ActionSetDirectControl::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attr, "attr", "Тип юнита");
	ar.serialize(controlMode_, "controlMode", "Режим прямого управления");
}

bool ActionSetDirectControl::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false; 

	unit = 0;
	const RealUnits& unitList = attr ? aiPlayer().realUnits(attr) : aiPlayer().realUnits();
	if(!unitList.empty() && aiPlayer().active()){
		RealUnits::const_iterator i;
		FOR_EACH(unitList, i)
			if(controlMode_ == DIRECT_CONTROL_DISABLED ? safe_cast<UnitActing*>(*i)->activeDirectControl() : true){
				unit = (*i);
				return true;
			}
	}

	return true;
}

ActionSetDirectControl::ActionSetDirectControl()
{
	unit = 0;
	controlMode_ = DIRECT_CONTROL_ENABLED;
}

void ActionSwitchTriggers::activate()
{
	switch(mode) {
		case SWITCH_ON_CURRENT_PLAYER_AI:
			aiPlayer().setAI(true);
			break;
		
		case SWITCH_OFF_CURRENT_PLAYER_AI:
			aiPlayer().setAI(false);
			break;
		
		case SWITCH_OFF_ALL_PLAYERS_AI: 
			{
				PlayerVect::const_iterator pi;
				FOR_EACH(universe()->Players, pi) 
					if(!(*pi)->isWorld()) 
						(*pi)->setAI(false);
			}
			break;
		
		case SWITCH_OFF_CURRENT_PLAYER_TRIGGERS:
			aiPlayer().setTriggersDisabled();
			break;
	}
}

ActionSwitchTriggers::ActionSwitchTriggers()
{
	mode = SWITCH_ON_CURRENT_PLAYER_AI;
}

void ActionSwitchTriggers::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mode, "mode", "Режим");
}

void ActionAttackBySpecialWeapon::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(weapons_, "|weapons|weaponref_", "Апгрейды спецоружия");
	removeZeros(weapons_);
	ar.serialize(attackCondition, "attackCondition", "Условие атаки");
	ar.serialize(minDistance, "minDistance", "Мин. расстояние от хозяина");  
	ar.serialize(passAbility, "passAbility", "Можно атаковать непроходимую поверхность");
	ar.serialize(timeToAttack_, "timeToAttack", "Продолжительность атаки");
	ar.serialize(unitsToAttack, "unitsToAttack", "Типы своих юнитов для атаки поверхности рядом");
}

void ActionExploreArea::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mainBuilding, "mainBuilding", "Главное строение");
	ar.serialize(exploringRadius, "exploringRadius", "Радиус разведки");
}

bool ActionAttackBySpecialWeapon::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(unit->isUpgrading() || unit->isDocked() || unit->currentState() == CHAIN_BIRTH_IN_AIR ||
	  (unit->attr().isBuilding() && unit->isConstructed()) || unit->specialFireTargetExist())
		return false;
	
	bool canFire = false;
	WeaponPrmReferences::const_iterator it;
	FOR_EACH(weapons_, it)
		if(unit->canFire((*it)->ID())){
			canFire = true;
			weaponID_ = (*it)->ID();
			break;
		}
		else if((*it)->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE && unit->canDetonateMines())
			unit->detonateMines();

	if(!canFire)
		return false;

	targetUnit_ = 0;
	firePosition_ = Vect2f::ZERO;

	switch(attackCondition){
		case ATTACK_GROUND:
			{
				WeaponScanOp scanOp(unit, aiPlayer(), scanRadiusAim, (*it));
				float angle_step = M_PI/16;
				if(isEq(factorRadius, -1.f)){
					float minRadius = unit->minFireRadiusOfWeapon((*it)->ID());
					float dist_ = minDistance > minRadius ? minDistance : minRadius;
					factorRadius = unit->radius() + dist_;
					startAngle = logicRNDfabsRnd(M_PI * 2.0f);
					angle = startAngle;
				}
				int scanCount = 0;
				Vect2f point;
				while (!scanOp.valid()) {
					if(angle > 2.0f*M_PI + startAngle) {
						//angle -= 2*M_PI;
						startAngle = logicRNDfabsRnd(M_PI * 2.0f);
						angle = startAngle;
						factorRadius += 5;
					}
					point = Vect2f(unit->position2D().x + factorRadius*cos(angle), 
						unit->position2D().y + factorRadius*sin(angle)); 
					point = Vect2f(
						clamp(point.x, 0, vMap.H_SIZE),
						clamp(point.y, 0, vMap.V_SIZE));
					angle += angle_step;
					if(passAbility || (!pathFinder->impassabilityCheckForUnit(point.xi(), point.yi(), round(scanRadiusAim), unit->impassability())))
						scanOp.checkPosition(point);
					scanCount++;
					if(unit->radius() + unit->fireRadiusOfWeapon((*it)->ID()) < factorRadius){
						factorRadius = -1.f;
						break;
					}
					if(scanCount > maxWeaponSpecialScan) 
						break;
				}
				if(scanOp.valid()) {
					firePosition_ = Vect2f(
						clamp(point.x, 0, vMap.H_SIZE),
						clamp(point.y, 0, vMap.V_SIZE));
					if(!virtualUnit_){
						virtualUnit_ = aiPlayer().buildUnit(AuxAttributeReference(AUX_ATTRIBUTE_ZONE));
						virtualUnit_->setRadius(scanRadiusAim);
						virtualUnit_->setPose(Se3f(QuatF::ID, Vect3f(point.x , point.y, 0)), true);
					}
					virtualUnit_->setPose(Se3f(QuatF::ID, Vect3f(point.x , point.y, 0)), true);
					angle = 0;
					factorRadius = -1;
					return true;
				}
				break;
			}
		case ATTACK_ENEMY_UNIT:
			{
				MicroAI_Scaner scanOp(unit, ATTACK_MODE_DEFENCE, weaponID_);
				universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(weaponID_)), scanOp);
				UnitInterface* p = scanOp.processTargets();
				if(p){
					firePosition_ = p->position2D();
					targetUnit_ = p;
					return true;
				}
				break;
			}
		case ATTACK_GROUND_NEAR_ENEMY_UNIT:
			{
				if(unit->findWeapon(weaponID_)->weaponPrm()->weaponClass() != WeaponPrm::WEAPON_WAITING_SOURCE){
					MicroAI_Scaner scanOp(unit, ATTACK_MODE_DEFENCE, weaponID_);
					universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(weaponID_)), scanOp);
					UnitInterface* p = scanOp.processTargets();
					if(p)
					{
						firePosition_ = p->position2D();
						targetUnit_ = 0;
						return true;
					}
				}
				else{
					WeaponBase* weapon = unit->findWeapon(weaponID_);
					WeaponUnitScanOp scanOp(unit , aiPlayer(), *it, false, false);
					scanOp.checkPosition(unit->position2D());
					if(scanOp.valid() &&  weapon->canAttack(WeaponTarget(scanOp.foundPos(), weaponID_))) {
						firePosition_ = scanOp.foundPos();
						targetUnit_ = 0;
						return true;
					}
				}
				break;
			}
		case ATTACK_GROUND_NEAR_MY_UNIT:
			{
				AttributeUnitOrBuildingReferences::const_iterator ui;
				FOR_EACH(unitsToAttack, ui)
				{
					const RealUnits& units = aiPlayer().realUnits(*ui);
					if(!units.empty()){
						firePosition_ = (*units.begin())->position2D();
						targetUnit_ = 0;
						return true;
					}
				}
				break;
			}
		case ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING:
			{
				firstTime = true;
				MicroAI_Scaner scanOp(unit, ATTACK_MODE_DEFENCE, weaponID_);
				universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(weaponID_)), scanOp);
				UnitInterface* p = scanOp.processTargets();
				if(p)
				{
					firePosition_ = p->position2D();
					targetUnit_ = 0;
					return true;
				}
				break;
			}
		case ATTACK_MY_UNIT:
			{
				MicroAI_Scaner scanOp(unit, ATTACK_MODE_DEFENCE, weaponID_);
				universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(weaponID_)), scanOp);
				UnitInterface* p = scanOp.processTargets();
				if(p){
					firePosition_ = p->position2D();
					targetUnit_ = p;
					return true;
				}
			}
	}

	unit->selectWeapon(0);
	return false;
}

void ActionActivateSpecialWeapon::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(weaponref_, "weaponref", "Спец. оружия");
	removeZeros(weaponref_);
	ar.serialize(mode_, "mode", "Действие");
}

bool ActionActivateSpecialWeapon::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(weaponref_.empty())
		return false;

	if(!unit->isConstructed() || unit->isUpgrading())
		return false;

	switch(mode_){
		case ON:{
			bool canFire_ = false;

			WeaponPrmReferences::const_iterator it;
			FOR_EACH(weaponref_, it){
				int weaponID = (*it)->ID();
				if(unit->canFire(weaponID)){
					canFire_ = true;
					break;
				}
			}
			if(!canFire_)
				return false;

			return true;
		}
		break;
		case OFF:{
			bool cannotFire_ = false;

			WeaponPrmReferences::const_iterator it;
			FOR_EACH(weaponref_, it){
				int weaponID = (*it)->ID();
				WeaponBase* weapon = unit->findWeapon(weaponID);
				if(weapon && weapon->isActive()){
					cannotFire_ = true;
					break;
				}
			}
			if(!cannotFire_)
				return false;

			return true;
		}
	}

	return false;
}

ActionActivateSpecialWeapon::ActionActivateSpecialWeapon()
{
	mode_ = ON;
}

void ActionActivateSpecialWeapon::activate()
{
	WeaponPrmReferences::const_iterator it;
	FOR_EACH(weaponref_, it){
		int weaponID = (*it)->ID();
		switch(mode_){
			case ON: 
				if(contextUnit_->canFire(weaponID)){
					contextUnit_->setUsedByTrigger(priority_, this);
					if(contextUnit_->getSquadPoint())
						contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_WEAPON_ACTIVATE, weaponID));
					else
						contextUnit_->executeCommand(UnitCommand(COMMAND_ID_WEAPON_ACTIVATE, weaponID));
				}
				break;
			case OFF:
				WeaponBase* weapon = contextUnit_->findWeapon(weaponID);
				if(weapon && weapon->isActive()){
					contextUnit_->setUsedByTrigger(priority_, this);
					if(contextUnit_->getSquadPoint())
						contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_STOP, weaponID));
					else
						contextUnit_->executeCommand(UnitCommand(COMMAND_ID_STOP, weaponID));
				}
		}
	}
}

ActionAttackBySpecialWeapon::ActionAttackBySpecialWeapon()
{
	attackCondition = ATTACK_GROUND;
	virtualUnit_ = 0;
	factorRadius = -1.f;
	minDistance = 10.f;
	passAbility = true;
	weaponID_ = 0;
	timeToAttack_ = 2;
	dist = 10.f;
}

void ActionAttackBySpecialWeapon::activate()
{
	contextUnit_->setUsedByTrigger(priority_, this);
	WeaponBase* weapon = contextUnit_->findWeapon(weaponID_);
	if(weapon && weapon->weaponPrm()->weaponClass() != WeaponPrm::WEAPON_WAITING_SOURCE){
		if(contextUnit_->canFire(weaponID_) && 
			(!targetUnit_ ? true : weapon->canAttack(WeaponTarget(targetUnit_, targetUnit_->position(), weaponID_)))){
			if(contextUnit_->getSquadPoint())
				contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID_));
			else
				contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID_));
			float reloadTime = weapon->parameters().findByType(ParameterType::RELOAD_TIME, 1.0f) * 1000.0f;
			resetTime_.start(reloadTime + 100);
		}
	}

	dist = 10.f;
    anglex = 0;
}

void ActionAttackBySpecialWeapon::clear()
{
	if(contextUnit_){
		contextUnit_->setUsedByTrigger(0, this);
		contextUnit_->setUnitState(UnitReal::AUTO_MODE);
		contextUnit_->clearAttackTarget();

		if(!contextUnit_->getSquadPoint())
			contextUnit_->selectWeapon(0);
		else
			contextUnit_->getSquadPoint()->selectWeapon(0);
	}
}

bool ActionAttackBySpecialWeapon::workedOut()
{
	if(!contextUnit_ || firePosition_.eq(Vect2f::ZERO))
		return true;

	if(!resetTime_.started()){
		WeaponBase* weapon = contextUnit_->findWeapon(weaponID_);
		if(weapon){
			if(weapon->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE){
				float maxDistance = contextUnit_->fireRadiusOfWeapon(weaponID_);
				if(contextUnit_->canFire(weaponID_)){
					while(dist < maxDistance){
						while(anglex < M_PI * 2.0f){
							anglex += M_PI * 10.f / dist;
							Vect2f position = Vect2f(firePosition_.x + cos(anglex)* dist, firePosition_.y + sin(anglex)* dist);
							WeaponTarget& weaponTarget = WeaponTarget(To3D(position), weaponID_);
							contextUnit_->setUsedByTrigger(priority_, this);
							if(contextUnit_->position2D().distance(position) < maxDistance && contextUnit_->fireCheck(weaponTarget) &&	weapon->checkFogOfWar(weaponTarget) && weapon->canAttack(weaponTarget)){
								if(contextUnit_->getSquadPoint())
									contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID_));
								else
									contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID_));
								return false;
							} 	
						}
						anglex = 0.f;
						dist += 10.f;
					}
					return true;
				}

				if(contextUnit_->canDetonateMines()){
					contextUnit_->detonateMines();
					float reloadTime = weapon->parameters().findByType(ParameterType::RELOAD_TIME, 1.0f) * 1000.0f;
					resetTime_.start(reloadTime + 100);
				}

				if(dist >= maxDistance)
					return true;
			}
		}
	}

	if(attackCondition == ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING){
		const Vect3f& pos = To3D(firePosition_); 
		if(contextUnit_->fireDistanceCheck(WeaponTarget(pos, weaponID_))){
			if(firstTime){
				firstTime = false;
				durationTimer_.start(timeToAttack_ * 1000);
				aiPlayer().startUsedByTriggerAttack(timeToAttack_ * 1000);
				aiPlayer().directShootCommand(pos, 1);
				return false;
			}
			if(durationTimer_.busy()){
				aiPlayer().directShootCommandMouse(pos);
				return false;
			}
			else{
				aiPlayer().directShootCommand(pos, 0);
				return true;
			}
		}
		else{
			if(contextUnit_->unitState() != UnitReal::ATTACK_MODE ){
				return true;
			}
			return false;
		}
	}
	else{
		if(resetTime_.started() && (!resetTime_.busy() || !contextUnit_->canFire(weaponID_))){
			contextUnit_->setUnitState(UnitReal::AUTO_MODE);
			contextUnit_->clearAttackTarget();
			if(!contextUnit_->getSquadPoint())
				contextUnit_->selectWeapon(0);
			else
				contextUnit_->getSquadPoint()->selectWeapon(0);

			resetTime_.stop();
			return true;
		}
	}
	return false;
}

void ActionPickResource::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(parameterType_, "parameterType", "Тип ресурса");
}

bool ActionPickResource::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>(unit->attr());
	if(!safe_cast<UnitLegionary*>(unit)->gatheringResource() &&
	  attr.resourcer && attr.canPickResource(parameterType_)) 
			return true;

	return false;
}

ActionPickResource::ActionPickResource()
{
	unit_ = 0;
}

UnitReal* ActionPickResource::findNearestResource(UnitReal* unit) const
{
	Vect2f pos = unit->position2D();
	UnitReal* bestUnit = 0;
	float bestDist = FLT_INF;
	float dist = 0;

	const RealUnits& resourceList = universe()->worldPlayer()->realUnits();
	if(!resourceList.empty()) {
		RealUnits::const_iterator ri = resourceList.begin();
		while(ri != resourceList.end()) {
			if((*ri)->attr().isResourceItem() && !(*ri)->isUnseen() && (*ri)->alive() && !(*ri)->hiddenLogic()) {
				const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>(unit->attr());
				int index = attr.resourcerCapacity((*ri)->parameters());
				if(index != -1 && !attr.canPick(*ri)) 
					if(bestDist > (dist = pos.distance2((*ri)->position2D()))){
						bestDist = dist;
						bestUnit = *ri;
					}
			}
			ri++;
		}
	}

	return bestUnit;
}

bool ActionPickResource::automaticCondition() const 
{
	
	if(!__super::automaticCondition())
		return false;
	unit_ = findNearestResource(contextUnit_);

	return unit_;
}

void ActionPickResource::activate()
{
	contextSquad_->clearOrders();
	safe_cast<UnitLegionary*>(&*contextUnit_)->setResourceItem(unit_);
	contextSquad_->setUsedByTrigger(priority_, this);
}

bool ActionOrderBuildings::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!attrBuilding)
		return false;

	// condition
	if(!unit->attr().isBuilding() || !safe_cast_ref<const AttributeBuilding&>(unit->attr()).includeBase)
		return false;

	// проверка условий возможности строительства по ресурсам
	if(!aiPlayer().checkUnitNumber(attrBuilding))
		return false;
	if(!aiPlayer().accessible(attrBuilding))
		return false;
	if(!aiPlayer().requestResource(attrBuilding->installValue, NEED_RESOURCE_TO_INSTALL_BUILDING))
		return false;

	found_ = false;
	builder_ = 0;

	bool check = true;

	// если требуется строитель
	if(attrBuilding->needBuilders) {
		
		check = false;
		
		const RealUnits& unitList = aiPlayer().realUnits();
		RealUnits::const_iterator ui;
		FOR_EACH(unitList, ui) {
			if((*ui)->attr().isLegionary()) {
				AttributeBuildingReferences::const_iterator bi;
				const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>((*ui)->attr()); 
				FOR_EACH(attr.constructedBuildings, bi) 
					if(*bi == attrBuilding && !safe_cast<UnitLegionary*>(*ui)->constructingBuilding() && !(*ui)->usedByTrigger(priority_) && !(*ui)->isDocked()){
						builder_ = safe_cast<UnitLegionary*> (*ui);
						check = true;
						break;
					}
			
			}
			if(check)
				break;
		}
	}

	if(!check)
		return false;

	PlaceScanOp scanOp(unit, attrBuilding, aiPlayer(), closeToEnemy_, radius_);
	scanOp.checkPosition(unit->position2D());
	wasScan_ = true;

	if(scanOp.found())
	{
		found_ = true;
		best_position = scanOp.bestPosition();
		return true;
	}

	return false;
}

void ActionContinueConstruction::activate()
{
	contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_CHANGE_MOVEMENT_MODE, mode_));
	contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_OBJECT, unit_, 0));
	contextUnit_->getSquadPoint()->setUsedByTrigger(priority_, this);
}

bool ActionContinueConstruction::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!unit->attr().isLegionary())
		return false;

	unit_ = 0;
	if(!safe_cast<UnitLegionary*>(unit)->constructingBuilding()){
		const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>(unit->attr()); 
		if(attr.constructedBuildings.empty())
			return false;
		float dist = 0.f;
		float minDist = FLT_INF;
		const RealUnits& realUnits = aiPlayer().realUnits();
		RealUnits::const_iterator ui;
		FOR_EACH(realUnits, ui)
		{
			if((*ui)->attr().isBuilding()){
				const UnitBuilding* building = safe_cast<UnitBuilding*>(*ui);
				if(!building->isConstructed() && !building->constructionInProgress() && !building->constructor())
				{
					if((dist = unit->position2D().distance2((*ui)->position2D())) < minDist){
						AttributeBuildingReferences::const_iterator bi;
						FOR_EACH(attr.constructedBuildings, bi) 
							if(*bi == &(*ui)->attr()){
								unit_ = *ui;
								minDist = dist;
								break;
							}
					}
				}
			}
		}
	}

	return unit_;
}

void ActionOrderBuildingsOnZone::placeBuildingSpecial() const
{	
	// рассматриваются все юниты
	RealUnits resourceList;
	PlayerVect::iterator pi;
	const RealUnits& myUnits = aiPlayer().realUnits();
	resourceList.clear();
	resourceList.insert(resourceList.end(), myUnits.begin(), myUnits.end());

	if(!onlyMyZone){
		FOR_EACH(universe()->Players, pi){
			if(*pi != &aiPlayer()){
				const RealUnits& units = (*pi)->realUnits();
				resourceList.insert(resourceList.end(), units.begin(), units.end());
			}
		}
	}	
	if(!resourceList.empty()) {
		if(indexScan < resourceList.size()) {
			RealUnits::const_iterator ri = resourceList.end() - 1 - indexScan;
			PlacementZone buildingPlacementZone = safe_cast<const AttributeBuilding*>(&*attrBuilding)->placementZone;
			while(ri >= resourceList.begin()) {
				const AttributeBase& attrRes = (*ri)->attr();
				indexScan++;
				if(/*(*ri)->attr().isResourceItem() &&*/ !(*ri)->isUnseen() && buildingPlacementZone == attrRes.producedPlacementZone && 
					aiPlayer().fogOfWarMap() ? ((*ri)->player() == &aiPlayer() ? true : aiPlayer().fogOfWarMap()->checkFogStateInCircle(Vect2i((*ri)->position2D().xi(), (*ri)->position2D().yi()), (*ri)->attr().producedPlacementZoneRadius)) : true){
					const RealUnits& units = aiPlayer().realUnits();
					RealUnits::const_iterator i;
					bool soNear = false;
					FOR_EACH(units, i)
						if((*i)->position2D().distance2((*ri)->position2D()) < sqr(500.f)){
							soNear = true;
							break;
						}
					if(soNear){
						/*Vect2f snapPoint_;
						if(checkBuildingPosition(attrBuilding, (*ri)->position2D(), false, snapPoint_))
						{
							found_ = true;
							best_position = (*ri)->position2D();
							builder_state = FoundWhereToBuild;
							break;
						}*/
						float radius = attrRes.producedPlacementZoneRadius + attrBuilding->radius();
						startPlace(Vect2i((*ri)->position().x - radius, (*ri)->position().y - radius),
								Vect2i((*ri)->position().x + radius, (*ri)->position().y + radius),
								ai_scan_step);
						return;
					}
				}
				ri--;
			}

			indexScan = 0;
		}
	}

	if(!found_){
		indexScan = 0;
		builder_state = BuildingPause;
		aiPlayer().checkEvent(EventUnitAttributePlayer(Event::UNABLE_BUILD, attrBuilding, &aiPlayer()));
	}
}

void ActionOrderBuildingsOnZone::startPlace(const Vect2i& scanMin, const Vect2i& scanMax, int scanStep) const
{	
	builder_state = FindingWhereToBuild;
	
	scanStep_ = scanStep;

	scanMin_ = scanMin;
	scanMax_ = scanMax;

	placement_coords = scanMin_ - Vect2i(scanStep_, 0);
}

void ActionOrderBuildingsOnZone::findWhereToBuildQuantSpecial() const
{
	for(int i = 0; i < ai_placement_iterations_per_quant; i++) {
		if((placement_coords.x += scanStep_) >= scanMax_.x || found_) {
			placement_coords.x = scanMin_.x;
			if((placement_coords.y += scanStep_) >= scanMax_.y || found_) {
				placement_coords.y = scanMin_.y;
                if(found_) {
					/* шаг сканирования меньше минимального значения и считаем что дальше продолжать поиск
						нет смысла ввиду его дороговизны и считаем что место для строительства здания найдено */
					builder_state = FoundWhereToBuild;
					best_position = foundPosition_;
					return;
				}
				else{
					if(scanStep_ > ai_scan_step_unable_to_find){
						/* если шаг больше чем шаг остановки сканирования при условии что не найдено подходящей позиции
						   пытаемся уменьшить шаг сканирования */
						startPlace(scanMin_, scanMax_, scanStep_/2);
					}
					else{
						/* если шаг меньше чем шаг остановки сканирования при условии что не найдено подходящей позиции
						   поиск закончен и место не найдено, пытаемся взять координаты следующего ресурса */
						placeBuildingSpecial();
					}
				}
				return;
			}
		}

		Vect2f snapPoint_ = Vect2f::ZERO;

		if(checkBuildingPosition(attrBuilding, placement_coords, false, snapPoint_))
		{
			found_ = true;
			foundPosition_ = placement_coords;
		}
	}
}

bool ActionOrderBuildingsOnZone::checkBuildingPosition(const AttributeBuildingReference attr, const Vect2f& position, bool checkUnits, Vect2f& snapPosition) const
{
	Vect2i points[4];
	attr->calcBasementPoints(0, position, points);
	int x0 = INT_INF, y0 = INT_INF; 
	int x1 = -INT_INF, y1 = -INT_INF;
	for(int i = 0; i < 4; i++){
		const Vect2i& v = points[i];
		if(x0 > v.x)
			x0 = v.x;
		if(y0 > v.y)
			y0 = v.y;
		if(x1 < v.x)
			x1 = v.x;
		if(y1 < v.y)
			y1 = v.y;
	}

	int sx = x1 - x0 + 2;
	int sy = y1 - y0 + 2;

	ScanGroundLineOp line_op(x0, y0, sx, sy);
	ScanningShape shape;
	shape.setPolygon(&points[0], 4);
	ScanningShape::const_iterator j;
	int y = shape.rect().y;
	FOR_EACH(shape.intervals(), j){
		line_op((*j).xl, (*j).xr, y);
		y++;
		if(!line_op.valid())
			break;
	}
	if(showDebugPlayer.placeOp)
		show_vector(To3D(points[0]),To3D(points[1]),To3D(points[2]),To3D(points[3]), aiPlayer().unitColor());
	if(line_op.valid() && safe_cast<const AttributeBuilding*>(&*attr)->checkBuildingPosition(position, Mat2f::ID, &aiPlayer(), checkUnits, snapPosition))
		return true;

	return false;
}

void ActionOrderBuildingsOnZone::clear()
{
	__super::clear();
	if(builder_)
		builder_->setUsedByTrigger(0, this);
}

void ActionOrderBuildingsOnZone::activate()
{
	Vect2f snapPoint_ = Vect2f::ZERO;
	if(found_ && checkBuildingPosition(attrBuilding, best_position, false, snapPoint_)){
		found_ = false;
        UnitBuilding* curBuilding = 0;
		if(!snapPoint_.eq(Vect2f::ZERO))
			curBuilding = aiPlayer().buildStructure(attrBuilding, Vect3f(snapPoint_.x, snapPoint_.y, 0.f), false);
		if(!curBuilding)
			curBuilding = aiPlayer().buildStructure(attrBuilding, Vect3f(best_position.x, best_position.y, 0.f), false);
		if(curBuilding){
			if(builder_ && !builder_->constructingBuilding()) {
				builder_->squad()->executeCommand(UnitCommand(COMMAND_ID_OBJECT, curBuilding, 0));
 			}
			else if(builder_)
				builder_->setUsedByTrigger(0, this);
			builder_state = BuildingPause;
			building_pause.start(ai_building_pause);
			return;
		}
	}

	// если произошел срыв (позиция не валидная, или не смог построить)
	if(builder_)
		builder_->setUsedByTrigger(0, this);
	builder_state = BuildingPause;
	building_pause.start(ai_building_pause);
}

ActionOrderBuildingsOnZone::ActionOrderBuildingsOnZone()
{
	onlyMyZone = true;
	builder_state = BuildingIdle;
	indexScan = 0;
	found_ = false;
	foundPosition_ = Vect2f::ZERO;
}

ActionOrderBuildings::ActionOrderBuildings()
{
	builder_ = 0;
	radius_ = 20.f;
	closeToEnemy_ = false;

	found_ = false;
	wasFound_ = false;
	wasScan_ = false;
}

bool ActionOrderBuildingsOnZone::automaticCondition() const 
{	
	if(!__super::automaticCondition()) 
		return false;

	if(!attrBuilding)
		return false;

	// проверка условий возможности строительства по ресурсам
	if(!aiPlayer().checkUnitNumber(attrBuilding))
		return false;
	if(!aiPlayer().accessible(attrBuilding))
		return false;
	if(!aiPlayer().requestResource(attrBuilding->installValue, NEED_RESOURCE_TO_INSTALL_BUILDING))
		return false;

	switch(builder_state){
	case BuildingIdle: 
		placeBuildingSpecial();
		break;

	case FindingWhereToBuild: 
		findWhereToBuildQuantSpecial();
		break;

	case BuildingPause: 
		if(!building_pause.busy())
			builder_state = BuildingIdle;
		break;
	}

	if(builder_state == FoundWhereToBuild){
		if(!attrBuilding->needBuilders)
			return true;

		const RealUnits& unitList = aiPlayer().realUnits();
		RealUnits::const_iterator ui;
		FOR_EACH(unitList, ui) 
			if((*ui)->attr().isLegionary()) {
				AttributeBuildingReferences::const_iterator bi;
				const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>((*ui)->attr()); 
				FOR_EACH(attr.constructedBuildings, bi) 
					if(*bi == attrBuilding && !safe_cast<UnitLegionary*>(*ui)->constructingBuilding() && !(*ui)->usedByTrigger(priority_) && !(*ui)->isDocked()){
						builder_ = safe_cast<UnitLegionary*> (*ui);
						builder_->setUsedByTrigger(priority_, this);
						builder_state = BuildingIdle;
						return true;
					}
			}
	}
	return false;
}

void ActionOrderBuildingsOnZone::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(attrBuilding, "attrBuilding", "Тип здания");
	ar.serialize(onlyMyZone,"onlyMyZone", "Зону генерит мой юнит");
}

void ActionOrderBuildings::activate()
{
	Vect2f snapPoint_;

	if(found_ && safe_cast<const AttributeBuilding*>(&*attrBuilding)->checkBuildingPosition(best_position, Mat2f::ID, &aiPlayer(), true, snapPoint_)){
		UnitBuilding* curBuilding = aiPlayer().buildStructure(attrBuilding, Vect3f(best_position.x, best_position.y, 0.f), false);
		if(curBuilding){
			if(builder_ && builder_->alive() && !builder_->constructingBuilding()) {
				builder_->squad()->executeCommand(UnitCommand(COMMAND_ID_OBJECT, curBuilding, 0));
				builder_->setUsedByTrigger(priority_, this);
			}
		}
	}

	wasFound_ = true;
}

void ActionOrderBuildings::clearContext()
{
	if(!wasFound_ && wasScan_)
		aiPlayer().checkEvent(EventUnitAttributePlayer(Event::UNABLE_BUILD, attrBuilding, &aiPlayer()));
	wasFound_ = false;
	wasScan_ = false;
}

void ActionOrderBuildings::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(attrBuilding, "attrBuilding", "Тип здания");
	ar.serialize(radius_, "radius_", "Радиус");
	ar.serialize(closeToEnemy_, "closeToEnemy_", "В направлении врага");
}

ActionUpgradeUnit::ActionUpgradeUnit()
{
	priority_ = 2;
	upgradeOption = UPGRADE_HERE;
	scanOp = 0;
	upgradeNumber = 0; 
	extraRadius = 100;
	contextUnit_ = 0; 
	interrupt_ = false; 
	notInWater_ = false;
	playerType_ = AI_PLAYER_TYPE_ME;
}

void ActionUpgradeUnit::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(upgradeNumber, "upgradeNumber", "Номер апгрейда");
	ar.serialize(upgradeOption, "upgradeOption", "Произвести апгрейд");
	ar.serialize(extraRadius, "extraRadius", "Расстояние до объектов");
	ar.serialize(objects_, "objects", "Рядом объекты");
	removeZeros(objects_);
	ar.serialize(interrupt_, "interrupt", "Прерывать производство");
	ar.serialize(notInWater_, "notInWater", "Не в воде");
	ar.serialize(attrBuildingToDisconnected_, "attrBuildingToDisconnected", "Тип отключенного здания");
	ar.serialize(playerType_, "playerType", "Владелец отключенного здания");
}

ConvexHull::ConvexHull(const Polygon& points)
{
	downRightPoint_ = Vect2f::ZERO;
	polygon_ = points;
	Polygon::iterator it;
	Polygon::iterator pointToErase;
	FOR_EACH(polygon_, it)
		if((*it).y > downRightPoint_.y || // нижняя
		   (isEq((*it).y, downRightPoint_.y) && (*it).x > downRightPoint_.x) ){ // нижняя-правая
				downRightPoint_ = (*it);
				pointToErase = it;
		} 
	polygon_.erase(pointToErase);	

	// обычная сортировка обменами (кол-во элементов мало)
	Polygon::iterator i;
	Polygon::iterator j;
	for(i = polygon_.begin(); i < polygon_.end() - 1; ++i)
		for(j = i+1; j < polygon_.end(); ++j)
			if(!compareCriteria(*i, *j))
			{
				Vect2f tmp;
				tmp = *i;
				*i = *j;
				*j = tmp;
			}

	polygon_.insert(polygon_.begin(), downRightPoint_);

	int v = 0; 
	int n = polygon_.size()-1;
	while (v < n){
		if(isLeft(polygon_[v+1], polygon_[v], polygon_[(v+2) % (n+1)]) > 0)
			v++;
	    else {
			polygon_.erase(polygon_.begin() + (v+1));
			n = polygon_.size()-1;
			v = (v-1) > 0 ? v-1 : 0;
	    }
	}
	
}

bool ConvexHull::compareCriteria(Polygon::value_type vect1, Polygon::value_type vect2)
{
	float angle1 = Vect2f(1.f, 0.f).angle(vect1 - downRightPoint_);
	float angle2 = Vect2f(1.f, 0.f).angle(vect2 - downRightPoint_);
	return angle1 < angle2 || (isEq(angle1, angle2)
		&& vect1.distance2(downRightPoint_) < vect2.distance2(downRightPoint_));
}

Vect2f ActionUpgradeUnit::normal(const Vect2f& p0, const Vect2f& p1)
{
	return Vect2f(p1.y - p0.y, p0.x - p1.x);
}

Vect2f ActionUpgradeUnit::intersect(const Vect2f& l1p0, const Vect2f& l1p1, const Vect2f& l2p0, const Vect2f& l2p1)
{
	if(fabs(l1p1.x - l1p0.x) < FLT_EPS || fabs(l2p1.x - l2p0.x) < FLT_EPS)
		return Vect2f::ZERO;
	float k1 = (l1p1.y - l1p0.y) / (l1p1.x - l1p0.x);
	float k2 = (l2p1.y - l2p0.y) / (l2p1.x - l2p0.x);
	float b1 = l1p0.y - l1p0.x * k1;
	float b2 = l2p0.y - l2p0.x * k2;
	if(fabs(k1 - k2) < FLT_EPS)
		return Vect2f::ZERO;
	float x = (b2-b1) / (k1 - k2);
	return Vect2f(x, k1 * x + b1);
}

bool ActionUpgradeUnit::pointIn(const Vect2f& l1p0, const Vect2f& l1p1, const Vect2f& ptCheck) const 
{
	if(l1p0.x < l1p1.x){
		if(ptCheck.x < l1p0.x || ptCheck.x > l1p1.x)
			return false;
	}
	else 
		if(ptCheck.x > l1p0.x || ptCheck.x < l1p1.x)
			return false;

	if(l1p0.y < l1p1.y){
		if(ptCheck.y < l1p0.y || ptCheck.y > l1p1.y)
			return false;
	}
	else 
		if(ptCheck.y > l1p0.y || ptCheck.y < l1p1.y)
			return false;

	return true;
}

Vect2f ActionUpgradeUnit::getPointInSection(const Vect2f& p0, const Vect2f& p1, float alfa)
{
	return Vect2f(p0.x + (p1.x - p0.x) * alfa, p0.y + (p1.y - p0.y) * alfa);
}

Vect2f ActionUpgradeUnit::getPointOnDistance(const Vect2f& guide, const Vect2f& point, float distance)
{
	Vect2f guide_ = guide;
	guide_.normalize(1.f);
	return Vect2f(point.x - distance * guide_.x, point.y - distance * guide_.y);
}

int ConvexHull::isLeft(const Vect2f& p0, const Vect2f& p1, const Vect2f& p2) 
{
	return SIGN((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
}

void ActionUpgradeUnit::clear()
{
	__super::clear();
	deleteScanOp();
}

Vect2f ActionUpgradeUnit::nearestEnemyCenter()
{
	PlayerVect::iterator pi;
	float maxDistance = FLT_INF;
	Vect2f center = Vect2f::ZERO;
	FOR_EACH(universe()->Players, pi)
		if(aiPlayer().isEnemy(*pi) && !(*pi)->isWorld()){
			Vect2f enemyCenter = (*pi)->centerPosition();
			if(enemyCenter.eq(Vect2f::ZERO, FLT_EPS))
				continue;
			float dist = enemyCenter.distance2(contextUnit_->position2D());
			if(dist < maxDistance)
			{
				center = enemyCenter;
				maxDistance = dist;
			}
		}
		return center;
}

Vect2f ActionUpgradeUnit::nearestDisconnected()
{
	RealUnits unitList;
	switch(playerType_)
	{
	case AI_PLAYER_TYPE_ME:
		{
			const RealUnits& units = attrBuildingToDisconnected_ ?
				aiPlayer().realUnits(attrBuildingToDisconnected_) : aiPlayer().realUnits();
			unitList.insert(unitList.end(), units.begin(), units.end());
		}
		break;
	case AI_PLAYER_TYPE_ENEMY:
		{
			PlayerVect::iterator pi;
			FOR_EACH(universe()->Players, pi)
				if(!(*pi)->isWorld() && (*pi)->isEnemy(&aiPlayer()))
				{
					const RealUnits& units = attrBuildingToDisconnected_ ?
						(*pi)->realUnits(attrBuildingToDisconnected_) : (*pi)->realUnits(); 
					unitList.insert(unitList.end(), units.begin(), units.end());
				}
		}
		break;
	case AI_PLAYER_TYPE_WORLD:
		{
			const RealUnits& units = attrBuildingToDisconnected_ ?
				universe()->worldPlayer()->realUnits(attrBuildingToDisconnected_) : universe()->worldPlayer()->realUnits();
			unitList.insert(unitList.end(), units.begin(), units.end());
		}
		break;
	case AI_PLAYER_TYPE_ANY:
		{
			PlayerVect::iterator pi;
			FOR_EACH(universe()->Players, pi)
			{
				const RealUnits& units = attrBuildingToDisconnected_ ?
					(*pi)->realUnits(attrBuildingToDisconnected_) : (*pi)->realUnits();
				unitList.insert(unitList.end(), units.begin(), units.end());
			}
		}
		break;
	}

	if(unitList.empty())
		return Vect2f::ZERO;

	Vect2f aimPos = Vect2f::ZERO;
	float maxDistance = FLT_INF;
	RealUnits::const_iterator i;
	FOR_EACH(unitList, i)
		if(!(*i)->isConnected())
		{
			float dist = contextUnit_->position2D().distance2((*i)->position2D());
			if(dist < maxDistance)
			{
				aimPos = (*i)->position2D();
				maxDistance = dist;
			}
		}

		return aimPos;
}

void ActionUpgradeUnit::positionFound()
{	
	UnitSquad* squad = contextUnit_->getSquadPoint();
	if(contextUnit_->unitState() == UnitReal::MOVE_MODE && !squad->wayPointsEmpty()) {
		scanOp->checkPosition(squad->wayPoint());
		Vect2f point;
		bool flagPoint = false;
		while (!scanOp->valid()) {
			if(fabs(angle - 2*M_PI) < FLT_COMPARE_TOLERANCE) {
				angle = 0;
				factorRadius += 1;
			}
			flagPoint = true;
			point = Vect2f(contextUnit_->position2D().x + factorRadius*(contextUnit_->radius() + extraRadius)*cos(angle), 
						   contextUnit_->position2D().y + factorRadius*(contextUnit_->radius() + extraRadius)*sin(angle));
			clampWorldPosition(point, contextUnit_->rigidBody()->maxRadius() + 2.0f);
			scanOp->checkPosition(point);
			angle += M_PI/12;
			if(factorRadius > 20) 
				break;
		}
		if(flagPoint && factorRadius < 20 && contextUnit_->rigidBody()->checkImpassability(point)){ 
			squad->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(point, contextUnit_->position().z), 0));
		}
	}
}

ActionExploreArea::ActionExploreArea()
{
	exploringRadius = 500;
	curRadius = 30;
	curAngle = 0;
}

bool ActionExploreArea::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;
	return !unit->isUpgrading() && unit->unitState() != UnitReal::MOVE_MODE && !unit->fireTargetExist();
}

bool ActionExploreArea::automaticCondition() const
{
	return __super::automaticCondition() && curRadius < exploringRadius;
}

void ActionExploreArea::activate()
{
	const RealUnits& realUnits = mainBuilding ? aiPlayer().realUnits(mainBuilding) : aiPlayer().realUnits();
	if(!realUnits.empty()){
		float curAngle_step = M_PI/18;
		RealUnits::const_iterator it;
		Vect2f center = Vect2f::ZERO;
		FOR_EACH(realUnits, it)
			center += (*it)->position2D();
		center /= realUnits.size();
		Vect2f point = Vect2f(center.x + curRadius*cos(curAngle), 
							  center.y + curRadius*sin(curAngle)); 
		point = Vect2f(
			clamp(point.x, 0, vMap.H_SIZE),
			clamp(point.y, 0, vMap.V_SIZE));
		while(aiPlayer().fogOfWarMap() ? aiPlayer().fogOfWarMap()->getFogState(point.xi(), point.yi()) != FOGST_FULL : true){
			curAngle += curAngle_step;
			point = Vect2f(center.x + curRadius*cos(curAngle), 
						   center.y + curRadius*sin(curAngle)); 
			point = Vect2f(
				clamp(point.x, 0, vMap.H_SIZE),
				clamp(point.y, 0, vMap.V_SIZE));
			if(curAngle > 2*M_PI) {
				curAngle -= 2*M_PI;
				curRadius += 30;
			}
			if(curRadius > exploringRadius)
				break;
		}
		curAngle += curAngle_step;
		if(curAngle > 2*M_PI) {
			curAngle -= 2*M_PI;
			curRadius += 30;
		}
		if(curRadius < exploringRadius){
			contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(point.x, point.y, 0), 0));
			contextSquad_->setUsedByTrigger(priority_, this);
		}
	}
}

bool ActionUpgradeUnit::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	// юнит не рядом со списком зданий => не подходит
	if(upgradeOption == UPGRADE_NEAR_OBJECT){
		CountObjectsScanOp op = CountObjectsScanOp(&aiPlayer(), objects_, extraRadius);
		op.checkInRadius(unit->position2D());
		if(!op.foundNumber())
			return false;
	}

	if(unit->isDocked() || unit->isUpgrading() || !unit->isConstructed())
	   return false;

	if(unit->attr().upgrades.exists(upgradeNumber)){
		const AttributeBase* attribute = unit->attr().upgrades[upgradeNumber].upgrade;
		if(!aiPlayer().accessible(attribute))
			return false;
		if(unit->attr().formationType != attribute->formationType && !aiPlayer().checkUnitNumber(attribute, object_))
			return false;
		if(unit->getSquadPoint()){
			if(unit->getSquadPoint()->canUpgrade(upgradeNumber) != CAN_START)
				return false;
		}
		else
			if(unit->canUpgrade(upgradeNumber) != CAN_START)
				return false;

		//if(unit->attr().upgrades.exists(upgradeNumber) && unit->attr().upgrades[upgradeNumber].upgrade->isBuilding())
		//	return true;

		return true;
	}
	else
		return false;

}

void ActionUpgradeUnit::activate()
{
	if(contextUnit_->attr().isBuilding() && upgradeOption != UPGRADE_NEAR_OBJECT) 
		upgradeOption = UPGRADE_HERE;

	contextUnit_->stop();
	contextUnit_->setUsedByTrigger(priority_, this);
	firstTime = true;

	timeToUpgrade.start(60000);
}

void ActionUpgradeUnit::deleteScanOp()
{
	if(scanOp){
		delete scanOp;
		scanOp = 0;
	}
}

bool ActionUpgradeUnit::workedOut()
{
	if(!contextUnit_ || !contextUnit_->checkUsage(this))
		return true;

	if(!timeToUpgrade.busy()){ 
		contextUnit_->setUsedByTrigger(0, this);
		return true;
	}

	UnitSquad* squad = contextUnit_->getSquadPoint();

	if(!scanOp)
		switch(upgradeOption){
			case UPGRADE_ON_THE_DISTANCE_FROM_ENEMY :
				scanOp = new RadiusScanOp(contextUnit_, 0, aiPlayer(), 2 * contextUnit_->radius());
				break;
			case UPGRADE_ON_THE_DISTANCE_TO_ENEMY:
				scanOp = new RadiusScanOp(contextUnit_, 0, aiPlayer(), extraRadius > contextUnit_->radius() ? extraRadius - contextUnit_->radius() : 1);
				break;
			case UPGRADE_ON_THE_DISCONNECTED_DIRECTION:
			case UPGRADE_ON_THE_ENEMY_DIRECTION:
				scanOp = new RadiusScanOp(contextUnit_, 0, aiPlayer(), contextUnit_->radius());
				break;
			default:
				scanOp = new RadiusScanOp(contextUnit_, 0, aiPlayer(), extraRadius);
		}	

	if(!contextUnit_->isUpgrading()){
		switch(upgradeOption) {
			case UPGRADE_NEAR_OBJECT:
			case UPGRADE_HERE: 
				if(squad)
					squad->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
				else
					contextUnit_->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
				return true;
			
			case UPGRADE_FROM_UNIT_ON_THE_ENEMY_DIRECTION:
			case UPGRADE_ON_THE_DISCONNECTED_DIRECTION:	
				if(firstTime){
					firstTime = false;
					Vect2f aim_ = (upgradeOption == UPGRADE_ON_THE_DISCONNECTED_DIRECTION) ? 
						nearestDisconnected() : nearestEnemyCenter();
					if(aim_.eq(Vect2f::ZERO))
						return true;
					Vect2f aimVect = (aim_ - contextUnit_->position2D());
					Vect2f point = aimVect.normalize(extraRadius);
					if(point.norm2() > aimVect.norm2())
						point = aimVect;
					point += contextUnit_->position2D();
					point.x += logicRND(contextUnit_->radius());
					point.y += logicRND(contextUnit_->radius());
					safe_cast<UnitLegionary*>(&*contextUnit_)->squad()->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(point.x, point.y, contextUnit_->position().z), 0));
				}
				else if(contextUnit_->unitState() != UnitReal::MOVE_MODE){
					squad->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
					return true;
				}
				break;
			
			case UPGRADE_ON_THE_DISTANCE: 
				xassert(squad);
				scanOp->checkPosition(contextUnit_->position2D());
				if(scanOp->valid()){
					if(notInWater_ ? (contextUnit_->rigidBody()->flyingMode() ? !contextUnit_->rigidBody()->onDeepWater() : !contextUnit_->rigidBody()->onWater()) : true) {
                   		squad->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
						return true;
					}
				}
				else{
 					if(contextUnit_->unitState() != UnitReal::MOVE_MODE || firstTime) {
						firstTime = false;
						float angle = logicRNDfabsRnd(M_PI * 2.0f);
						Vect2f point = Vect2f(contextUnit_->position2D().x + cos(angle)*(contextUnit_->radius() + 3*extraRadius), contextUnit_->position2D().y + sin(angle)*(contextUnit_->radius() + 3*extraRadius)); 
						clampWorldPosition(point, contextUnit_->rigidBody()->maxRadius() + 2.0f);
						safe_cast<UnitLegionary*>(&*contextUnit_)->squad()->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(point.x, point.y, contextUnit_->position().z), 0));
					}
					angle = 0; 
					factorRadius = 3;
					positionFound();
				}
				break;
			
			case UPGRADE_ON_THE_DISTANCE_FROM_ENEMY: {
				xassert(squad);
				if(firstTime) {
					firstTime = false;
					Vect2f myCenter = aiPlayer().centerPosition();
					float checkDist = FLT_INF;
					aim_ = Vect2f::ZERO;
					PlayerVect::iterator pi;
					FOR_EACH(universe()->Players, pi)
						if(aiPlayer().isEnemy(*pi) && !(*pi)->isWorld()){
							Vect2f enemyCenter = (*pi)->centerPosition();
							if(enemyCenter.eq(Vect2f::ZERO, FLT_EPS))
								continue;
							float maxDist = (*pi)->maxDistance() + extraRadius;
							float distToEnemy2 = enemyCenter.distance2(myCenter);
							float searchAngle = 0.f;
							while(searchAngle < 2.f * M_PI){
								Vect2f aim = Vect2f(enemyCenter.x + maxDist * cos(searchAngle),
													enemyCenter.y + maxDist * sin(searchAngle));
								aim = Vect2f(
									clamp(aim.x, 0, vMap.H_SIZE),
									clamp(aim.y, 0, vMap.V_SIZE));
								float curDist = aim.distance2(myCenter);
								if (curDist < distToEnemy2){
									scanOp->checkPosition(aim);
									if(curDist < checkDist && scanOp->valid() &&
										(notInWater_ ? !pathFinder->checkWater((int) aim.x, (int) aim.y, 0) : true)){
										checkDist = curDist;
										aim_ = aim;
									}
								}
								searchAngle += M_PI / 12.f;
							}
						}
					if(!aim_.eq(Vect2f::ZERO, FLT_EPS)){
						squad->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim_.x, aim_.y, contextUnit_->position().z), 0));
						return false;
					}
				}
				else
					if(contextUnit_->unitState() != UnitReal::MOVE_MODE){
							scanOp->checkPosition(aim_);
							if(scanOp->valid() && aim_.eq(contextUnit_->position2D(), contextUnit_->attr().radius()) &&
								(notInWater_ ? !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0) : true)){
								squad->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
								return true;
							}
							else
								firstTime = true;
					}
				break;
			}

			case UPGRADE_ON_THE_ENEMY_DIRECTION:{
				xassert(squad);
				if(firstTime) {
					firstTime = false;
					float appendDistance = 0;
					ConvexHull::Polygon points;
					const RealUnits& realUnits = aiPlayer().realUnits();
					RealUnits::const_iterator it;
					FOR_EACH(realUnits, it)
						if((*it)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*it)->attr()).includeBase){ 
							points.push_back((*it)->position2D());
							if(appendDistance < (*it)->attr().radius())
								appendDistance = (*it)->attr().radius();
						}
					int nPoints = points.size();
					if(!nPoints){
						points.push_back(contextUnit_->position2D());
						appendDistance = contextUnit_->attr().radius();
						nPoints = 1;
					}
					if(nPoints < 3) 
						for(int i = 0; i < nPoints; ++i){
							points.push_back(Vect2f(points[i].x + 2 * stepOnConvexSide, points[i].y));
							points.push_back(Vect2f(points[i].x, points[i].y + 2 * stepOnConvexSide));
						}
					ConvexHull convex(points);
					ConvexHull::Polygon polygon_ = convex.getPolygon();
					int n = polygon_.size() - 1; 
					vector<Vect2f> enemyCenters;
					PlayerVect::iterator pi;
					FOR_EACH(universe()->Players, pi)
						if(aiPlayer().isEnemy(*pi)){
							Vect2f enemyCenter = (*pi)->centerPosition();
							if(!enemyCenter.eq(Vect2f::ZERO, FLT_EPS)){
									vector<Vect2f>::iterator ec = enemyCenters.end();
									FOR_EACH(enemyCenters, ec)
										if(enemyCenter.distance2(aiPlayer().centerPosition())< (*ec).distance2(aiPlayer().centerPosition()))
											break;
									if(ec == enemyCenters.end())
										enemyCenters.push_back(enemyCenter);
									else
										enemyCenters.insert(ec, enemyCenter);
							}
						}
					for(int i = 0; i <= n; ++i){
						vector<Vect2f>::iterator ec;
						FOR_EACH(enemyCenters, ec){
							Vect2f point = intersect(polygon_[i],polygon_[(i+1) % (n+1)], *ec, aiPlayer().centerPosition());
							if(pointIn(polygon_[i],polygon_[(i+1) % (n+1)], point) && pointIn(*ec, aiPlayer().centerPosition(), point)){
								Vect2f guide = aiPlayer().centerPosition() - (*ec);
								aim_ = getPointOnDistance(guide, point, contextUnit_->attr().radius() * 2 + extraRadius + appendDistance);
								scanOp->checkPosition(aim_);
								aim_ = Vect2f(
									clamp(aim_.x, 0, vMap.H_SIZE),
									clamp(aim_.y, 0, vMap.V_SIZE));
								if(scanOp->valid() && !pathFinder->checkImpassability(aim_.xi(), aim_.yi(), squad->impassability(), squad->passability()) &&
									(notInWater_ ? !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0) : true)){
									squad->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim_.x, aim_.y, contextUnit_->position().z), 0));
									return false;
								}
								else 
								{
									int steps = 6;
									float dist = 20.f;
									float maxDist = 100.f;
									while(dist <= maxDist)
									{
										for(int i = 0; i < steps; i++)
										{
											Vect2f direction = Vect2f(cos(i * 2.f * M_PI / steps), sin(i * 2.f * M_PI / steps));
											if (direction.dot(guide) > 0.f)
												continue;
											Vect2f aim = getPointOnDistance(direction, aim_, dist);
											scanOp->checkPosition(aim);
											aim = Vect2f(
												clamp(aim.x, 0, vMap.H_SIZE),
												clamp(aim.y, 0, vMap.V_SIZE));
											if(scanOp->valid() && !pathFinder->checkImpassability(aim.xi(), aim.yi(), squad->impassability(), squad->passability()) && 
												(notInWater_ ? !pathFinder->checkWater((int) aim.x, (int) aim.y, 0) : true)){
												aim_ = aim;
												squad->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim.x, aim.y, contextUnit_->position().z), 0));
												return false;
											}
										
		
										}
										dist += 20.f;
									}
								}
							}
						}
					}
					squad->setUsedByTrigger(0, this);
					return true; // если не нашли
				}
				else
					if(contextUnit_->unitState() != UnitReal::MOVE_MODE){
						scanOp->checkPosition(aim_);
						if(scanOp->valid() && aim_.eq(contextUnit_->position2D(), 2*contextUnit_->attr().radius()) && 
							(notInWater_ ? !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0) : true)){
							squad->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
							return true;
						}
						else
							firstTime = true;
					}
					break;
			}

			case UPGRADE_ON_THE_DISTANCE_TO_ENEMY: {
				xassert(squad);
				if(firstTime) {
						firstTime = false;
						float appendDistance = 0;
						ConvexHull::Polygon points;
						const RealUnits& realUnits = aiPlayer().realUnits();
						RealUnits::const_iterator it;
						FOR_EACH(realUnits, it)
							if((*it)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*it)->attr()).includeBase){ 
								points.push_back((*it)->position2D());
								if(appendDistance < (*it)->attr().radius())
									appendDistance = (*it)->attr().radius();
							}
						int nPoints = points.size();
						if(!nPoints){
							points.push_back(contextUnit_->position2D());
							appendDistance = contextUnit_->attr().radius();
							nPoints = 1;
						}
						if(nPoints < 3) 
							for(int i = 0; i < nPoints; ++i){
								points.push_back(Vect2f(points[i].x + 2 * stepOnConvexSide, points[i].y));
								points.push_back(Vect2f(points[i].x, points[i].y + 2 * stepOnConvexSide));
							}
						ConvexHull convex(points);
						vector<Vect3f> points3D;
						ConvexHull::Polygon polygon_ = convex.getPolygon();
						int n = polygon_.size() - 1; 
						vector<Vect2f> enemyCenters;
						PlayerVect::iterator pi;
						FOR_EACH(universe()->Players, pi)
							if(aiPlayer().isEnemy(*pi)){
								Vect2f enemyCenter = (*pi)->centerPosition();
								if(!enemyCenter.eq(Vect2f::ZERO, FLT_EPS))
									enemyCenters.push_back(enemyCenter);
							}
						for(int i = 0; i <= n; ++i){
							int max_points = round(polygon_[i].distance(polygon_[(i+1) % (n+1)]) / stepOnConvexSide) + 1; 
							for(int j = 0; j <= max_points; j++){
								float alfa = j * 1.f / max_points;
								Vect2f aim = getPointInSection(polygon_[i],polygon_[(i+1) % (n+1)], alfa);
								Vect2f normal_ = normal(polygon_[i],polygon_[(i+1) % (n+1)]);
								vector<Vect2f>::iterator ec;
								bool goodGuide = false;
								FOR_EACH(enemyCenters, ec)
									if (normal_.dot(*ec - aim) < 0.f){
										goodGuide =true;
										break;
									}
								if(goodGuide){
									aim_ = getPointOnDistance(normal_, aim, contextUnit_->attr().radius() * 2 + extraRadius + appendDistance);
									scanOp->checkPosition(aim_);
									aim_ = Vect2f(
										clamp(aim_.x, 0, vMap.H_SIZE),
										clamp(aim_.y, 0, vMap.V_SIZE));
									if(scanOp->valid() && (notInWater_ ? !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0) : true)){
										squad->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim_.x, aim_.y, contextUnit_->position().z), 0));
										return false;
									}
								}
							}
						}
						squad->setUsedByTrigger(0, this);
						return true; // если не нашли
				}
				else
					if(contextUnit_->unitState() != UnitReal::MOVE_MODE){
							scanOp->checkPosition(aim_);
							if(scanOp->valid() && aim_.eq(contextUnit_->position2D(), 2*contextUnit_->attr().radius()) && 
								(notInWater_ ? !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0) : true)){
								squad->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
								return true;
							}
							else
								firstTime = true;
					}
				break;
			}	
		}	
	}
	else 
		contextUnit_ = 0;
	
	return false;
	
}

ActionForAI::ActionForAI() 
{
	onlyIfAi_ = true; 
	priority_ = 3;
}

void ActionForAI::clear()
{
	__super::clear();
	difficultyTimer_.stop();
}

bool ActionForAI::automaticCondition() const
{
	if(__super::automaticCondition() && (!onlyIfAi_ || aiPlayer().isAI())){
		if(!difficultyTimer_.started())
			difficultyTimer_.start(difficultyDelay());
		if(difficultyTimer_.finished())
			return true;
		else 
			return false;
	}
	return false;
}

void ActionForAI::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(onlyIfAi_, "onlyIfAi", "Запускать только для АИ");
	ar.serialize(RangedWrapperi(priority_, 1, 5), "priority", "Приоритет");
}

void ActionEscapeWater::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(nearBase_, "nearBase" , "Радиус поиска возле базы(не более 500)");
}

bool ActionEscapeWater::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(contextUnit_->unitState() == UnitReal::MOVE_MODE && 
		!contextSquad_->wayPointsEmpty() && 
		!pathFinder->checkWater(round(contextSquad_->wayPoint().x), round(contextSquad_->wayPoint().y), 0))
		return false;

	return true;
}

void ActionEscapeWater::activate()
{
	Vect2i pos = Vect2i::ZERO;
	if(pathFinder->findNearestPoint(aiPlayer().centerPosition() , nearBase_, PathFinder::GROUND_FLAG, pos)){
		contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(pos, 0), 0));
		contextSquad_->setUsedByTrigger(priority_, this);
	}
}

void ActionSetCamera::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(cameraSplineName, "cameraSplineName", "Имя сплайна камеры");
	ar.serialize(cycles, "cycles", "Количество циклов");
	ar.serialize(smoothTransition, "smoothTransition", "Плавный переход");
	ar.serialize(timer_, "timer", 0);
}	

void ActionSquadMoveToAnchor::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "Метка якоря");
	ar.serialize(mode, "mode", "Критерий завершения");
}	

void ActionPutSquadToAnchor::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(anchor_, "|anchor|label", "Метка якоря");
}	

void ActionSetControlEnabled::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(controlEnabled, "controlEnabled", "Разрешить");
}	

void ActionSetFreezedByTrigger::activate()
{
	UnitActing::setFreezedByTrigger(freeze);
}

void ActionSetFreezedByTrigger::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(freeze, "freeze", "Заморозить");
}	

void ActionMessage::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(messageSetup_, "messageSetup", "&Сообщение");
	ar.serialize(type_, "type", "Действие");
	ar.serialize(pause_, "pause", "Пауза после сообщения, секунды");
	ar.serialize(fadeTime_, "fadeTime", "Фейд, секунды");

	ar.serialize(workTimer_, "workTimer", 0);
	ar.serialize(fadeTimer_, "fadeTimer", 0);
	ar.serialize(finishTimer_, "finishTimer", 0);

	pause_ = max(pause_, fadeTime_);
}		

void ActionTask::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(state_, "state", "Состояние");
	if(state_ == UI_TASK_ASSIGNED)
		ar.serialize(isSecondary_, "isSecondary", "Второстепенная задача");

	ar.serialize(messageSetup_, "messageSetup", "Сообщение");

	ar.serialize(durationTimer_, "durationTimer_", 0);
}	

void ActionSetCameraAtObject::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(transitionTime, "transitionTime", "Время перехода, секунды");
	ar.serialize(setFollow, "setFollow", "Установить слежение");
	ar.serialize(turnTime, "turnTime", "Время поворота");
	ar.serialize(cameraSplineName, "cameraSplineName", "Имя сплайна камеры");
	ar.serialize(timer_, "timer", 0);
	ar.serialize(turnStarted_, "turnStarted", 0);
}

bool ActionSelectUnit::automaticCondition() const
{
	return !aiPlayer().realUnits(unitID).empty();
}

void ActionSelectUnit::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(unitID, "unitID", "Идентификатор юнита");
	ar.serialize(onlyConnected_, "onlyConnected", "Только подключенные");
}	

void ActionSetInterface::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(enableInterface, "enableInterface", "Включить интерфейс");
}	

//------------------------------------------------------

void ActionCreateNetClient::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	ar.serialize(type_, "type", "Тип сетевой подсистемы");
}

//------------------------------------------------------

void ActionSelectInterfaceScreen::activate()
{
	int time = round(1000*UI_Dispatcher::instance().getSelectScreenTime(screenReference_.screen()));
	if(triggerChain()->ignorePause())
		nonStopTimer_.start(time);
	else
		timer_.start(time);
	if(!universe() || aiPlayer().active())
		UI_Dispatcher::instance().selectScreen(screenReference_.screen());
}

bool ActionSelectInterfaceScreen::workedOut()
{
	return triggerChain()->ignorePause() ? !nonStopTimer_.busy() : !timer_.busy();
}

void ActionSelectInterfaceScreen::serialize(Archive& ar) 
{
	__super::serialize(ar);

	ar.serialize(screenReference_, "screenReference", "Экран");
}	

void ActionInterfaceHideControl::activate()
{
	if(UI_ControlBase* cp = controlReference_.control()){
		if(hideControl_)
			cp->hide();
		else
			cp->show();
	}
}

void ActionInterfaceHideControl::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(controlReference_, "controlReference", "Кнопка");
	ar.serialize(hideControl_, "hideControl", "Спрятать кнопку");
}

//------------------------------------------------------

void ActionInterfaceHideControlTrigger::activate()
{
	if(UI_ControlBase* cp = controlReference_.control()){
		if(hideControl_)
			cp->hideByTrigger();
		else
			cp->showByTrigger();
	}
}

void ActionInterfaceHideControlTrigger::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(controlReference_, "controlReference", "Кнопка");
	ar.serialize(hideControl_, "hideControl", "Спрятать кнопку");
}

//------------------------------------------------------

void ActionInterfaceControlOperate::activate()
{
	for_each(actions_.begin(), actions_.end(), mem_fun_ref(&AtomAction::apply));
}

bool ActionInterfaceControlOperate::workedOut()
{
	//AtomActions::const_iterator it;
	//FOR_EACH(actions_, it)
	//	if(!it->workedOut())
	//		return false;
	return true;
}

void ActionInterfaceControlOperate::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(actions_, "actions", "Действия с кнопками");
}

//------------------------------------------------------

void ActionInterfaceSetControlState::activate()
{
	if(UI_ControlBase* cp = controlReference_.control())
		cp->setState(state_);
}

void ActionInterfaceSetControlState::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(controlReference_, "controlReference", "Кнопка");
	ar.serialize(state_, "state", "номер состояния");
}

//------------------------------------------------------

void ActionInterfaceTogglAccessibility::activate()
{
	if(UI_ControlBase* cp = controlReference_.control()){
		if(enableControl_)
			cp->enable();
		else
			cp->disable();
	}
}

void ActionInterfaceTogglAccessibility::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(controlReference_, "controlReference", "Кнопка");
	ar.serialize(enableControl_, "enableControl", "разрешить кнопку");
}

//------------------------------------------------------

void ActionUI_ScreenSwitchOff::activate()
{
	UI_Dispatcher::instance().selectScreen(0);
}

bool ActionUI_ScreenSwitchOff::workedOut()
{
	return UI_Dispatcher::instance().isScreenActive(0);
}

//------------------------------------------------------

void ActionUI_ConfirmDiskOp::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(confirmDiskOp_, "confirmDiskOp", "разрешить перезапись");
}

void ActionUI_ConfirmDiskOp::activate()
{
	UI_LogicDispatcher::instance().confirmDiskOp(confirmDiskOp_);
}

//------------------------------------------------------

void ActionUI_InventoryQuickAccessMode::activate()
{
	InventorySet::setQuickAccessMode(quickAccessMode_);
}

void ActionUI_InventoryQuickAccessMode::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(quickAccessMode_, "quickAccessMode", "Режим работы инвентаря быстрого доступа");
}

//------------------------------------------------------

void ActionToggleBuildingInstaller::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(attributeReference_, "attributeReference", "здание");
}

//------------------------------------------------------
ActionDelay::ActionDelay() 
{
	duration = 10.f;
	showTimer = true;
	scaleByDifficulty = false;
	useNonStopTimer = false;
	randomTime = false;
}

void ActionDelay::activate() 
{ 
	if(!useNonStopTimer){
		int time = scaleByDifficulty ? round(aiPlayer().difficulty()->triggerDelayFactor*duration*1000) : duration*1000;
		if(randomTime)
			time = logicRND(time);
		timer.start(time); 
	}
	else{
		int time = duration*1000;
		if(randomTime)
			time = effectRND(time);
		nonStopTimer.start(time);
	}
}

bool ActionDelay::workedOut()
{ 
	if(showTimer){
		universe()->setCountDownTime(timer.busy() ? timer.timeRest() : -1);
		aiPlayer().checkEvent(EventTime(timer.timeRest()));
	}
	return !useNonStopTimer ? !timer.busy() : !nonStopTimer.busy(); 
}

void ActionDelay::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(duration, "duration", "Время, секунды");
	ar.serialize(showTimer, "showTimer", "Показывать таймер");
	ar.serialize(scaleByDifficulty, "scaleByDifficulty", "Влияние уровня сложности");
	ar.serialize(useNonStopTimer, "useNonStopTimer", "Использовать неостанавливаемый во время паузу таймер (только для неповторяемых триггреров!)");
	ar.serialize(randomTime, "randomTime", "Случайное время в диапазоне 0..указанное");
	if(!useNonStopTimer)
		ar.serialize(timer, "timer", 0);
}	

//-------------------------------------

ActionOrderParameters::ActionOrderParameters()
{
	queried_ = false;
	numberOfParameter = 0;
}

UnitActing* ActionOrderParameters::findFreeFactory(const AttributeBase* factoryAttribute) const
{
	if(factoryAttribute){
		const RealUnits& factories = aiPlayer().realUnits(factoryAttribute);
		RealUnits::const_iterator i;
		if(!factories.empty() && factoryAttribute->producedParameters.exists(numberOfParameter)){
			FOR_EACH(factories, i){
				UnitActing* factory = safe_cast<UnitActing*>(*i);
				if(factory->isProducing() || factory->isUpgrading() || !factory->isConstructed() || !factory->isConnected() || factory->usedByTrigger(priority_))
					return 0;
				if(factory->canProduceParameter(numberOfParameter) != CAN_START)
					return 0;
				const ProducedParameters& prm = factory->attr().producedParameters[numberOfParameter];
				if(!factory->accessibleParameter(prm))
					return 0;
				return factory;
			}
		}
	}
	return 0;
}

bool ActionOrderParameters::automaticCondition() const
{
	if(!__super::automaticCondition()) 
		return false;
	UnitActing* factory = findFreeFactory(attrFactory);
	if(!factory)
		return false;
	return true;
}

void ActionOrderParameters::activate()
{
	queried_ = false;
}

bool ActionOrderParameters::workedOut()
{	
	if(factory_ && !factory_->isProducing()){
		factory_->setUsedByTrigger(0, this);
		factory_ = 0;
	}

	if(queried_ && !factory_)
		return true;

	if(!queried_){
		factory_ = findFreeFactory(attrFactory);
		if(factory_){
			queried_ = true;
			factory_->setUsedByTrigger(priority_, this);
			factory_->executeCommand(UnitCommand(COMMAND_ID_PRODUCE_PARAMETER, numberOfParameter));
		}
	}
	return false;
}

void ActionOrderParameters::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrFactory, "attrFactory", "Тип фабрики");
	ar.serialize(numberOfParameter, "numberOfParameter", "Номер параметра");
}


bool ActionPutUnitInTransport::checkContextUnit(UnitActing* unit) const
{
	return __super::checkContextUnit(unit) 
		&& unit->attr().isLegionary() && !safe_cast<UnitLegionary*>(unit)->inTransport()
		&& !safe_cast<UnitLegionary*>(unit)->isMovingToTransport();
}

bool ActionPutUnitInTransport::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(!attrTransport)
		return false;

	NearestFreeTransportScanOp op(contextUnit_, attrTransport, distance);
	Vect2i pos = contextUnit_->position2D();
	universe()->unitGrid.ConditionScan(pos.x, pos.y, round(distance), op);
	if(op.found()){
		unitTransport = op.unit();
		return true;
	}

	return false;
}

void ActionPutUnitInTransport::activate()
{
	safe_cast<UnitActing*>(&*unitTransport)->putInTransport(safe_cast<UnitLegionary*>(&*contextUnit_)->squad());
}

void ActionOutUnitFromTransport::activate()
{
	contextUnit_->putOutTransport();
	contextUnit_->setUsedByTrigger(priority_, this);
}

void ActionOutUnitFromTransport::serialize(Archive& ar)
{
	__super::serialize(ar);

}

void ActionPutUnitInTransport::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(attrTransport, "attrTransport", "Тип транспорта/здания");
	ar.serialize(distance, "distance", "Максимальная дистанция до транспорта");
}

ActionEnableSounds::ActionEnableSounds()
{
	switchMode_ = MODE_OFF;
	soundType_ = TYPE_SOUND;
}

void ActionEnableSounds::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(soundType_, "soundType", "Тип звука");
	ar.serialize(switchMode_, "switchMode", "Действие");
}

void ActionEnableSounds::activate()
{
	switch(soundType_) 
	{
		case TYPE_SOUND:{
			if(switchMode_ == MODE_OFF)
				sndSystem.EnableSound(false);
			else
				if(switchMode_ == MODE_ON)
					sndSystem.EnableSound(true);
				else
					sndSystem.EnableSound(GameOptions::instance().getBool(OPTION_SOUND_ENABLE));
			break;
		}
		case TYPE_VOICE:{
			if(switchMode_ == MODE_OFF)
					voiceManager().setEnabled(false);
			else
				if(switchMode_ == MODE_ON)
					voiceManager().setEnabled(true);
				else
					voiceManager().setEnabled(GameOptions::instance().getBool(OPTION_VOICE_ENABLE));
			break;
		}
		case TYPE_MUSIC:{
			if(switchMode_ == MODE_OFF)
				musicManager.Enable(false, true);
			else
				if(switchMode_ == MODE_ON)
					musicManager.Enable(true, true);
				else
					musicManager.Enable(GameOptions::instance().getBool(OPTION_MUSIC_ENABLE), true);
		}
	}
}

ActionOrderUnits::ActionOrderUnits()
{
	unitsNumber_ = 1;
	counter_ = 0;
}

bool ActionOrderUnits::automaticCondition() const
{
	if(!__super::automaticCondition()) 
		return false;

	UnitActing* factory = aiPlayer().findFreeFactory(unitAttribute_, priority_);
	if(!factory)
		return false;
	int number = factory->attr().productionNumber(unitAttribute_);
	if(factory->canProduction(number) != CAN_START)
		return false;
	return true;
}

void ActionOrderUnits::activate()
{
	counter_ = unitsNumber_;
}

bool ActionOrderUnits::workedOut()
{	
	removeZeros(factories_);
	
	for(Factories::iterator fi = factories_.begin(); fi != factories_.end();){
		if(!(*fi)->isProducing()){
			(*fi)->setUsedByTrigger(0);
			fi = factories_.erase(fi);
		}
		else
			++fi;
	}
	
	if(counter_){
		UnitActing* factory = aiPlayer().findFreeFactory(unitAttribute_, priority_);
		if(factory){
			int number = factory->attr().productionNumber(unitAttribute_);
			if(factory->canProduction(number) == CAN_START){
				factories_.push_back(factory);
				factory->setUsedByTrigger(priority_, this);
				factory->executeCommand(UnitCommand(COMMAND_ID_PRODUCE, number));
				counter_--;
			}
		}
	}
	
	if(!counter_ && factories_.empty())
		return true;
	else
		return false;
}

void ActionOrderUnits::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(unitAttribute_, "|unitAttribute|attrUnit", "Тип юнита");
	ar.serialize(unitsNumber_, "|unitsNumber|countUnits", "Количество");
}

ActionFollowSquad::ActionFollowSquad()
{
	radius_ = 0;
}

void ActionFollowSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(squadsToFollow_, "squadsToFollow", "Сквады, за которым следовать");
	ar.serialize(radius_, "radius", "Расстояние");
}

bool ActionFollowSquad::automaticCondition() const
{
	if(!__super::automaticCondition()) 
		return false;
	
	squad_ = 0;
	const SquadList& squads = aiPlayer().squads();
	SquadList::const_iterator si;
	float distMin = FLT_INF;
	FOR_EACH(squads, si){
		if(*si != contextSquad_ && findReference(squadsToFollow_, &(*si)->attr())){
			float dist = (*si)->position2D().distance2(contextSquad_->position2D());
			if(distMin > dist){
				distMin = dist;
				squad_ = *si;
			}
		}
	}
	return squad_;
}

void ActionFollowSquad::activate()
{
	contextSquad_->setSquadToFollow(squad_);
	contextSquad_->setUsedByTrigger(priority_, this);
}

bool ActionSplitSquad::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;
	return contextSquad_ && contextSquad_->units().size() > 1;
}

void ActionSplitSquad::activate()
{
	contextSquad_->split(false);
}

ActionJoinSquads::ActionJoinSquads()
{
	radius_ = 100;
}

void ActionJoinSquads::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(radius_, "radius", "Расстояние сканирования");
	ar.serialize(attrUnits_, "attrUnits", "Типы присоединяемых юнитов");
	removeZeros(attrUnits_);
}

bool ActionJoinSquads::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(attrUnits_.empty() || object_){
		UnitSquad* squad = safe_cast<UnitLegionary*>(unit)->squad();
		return !squad->locked() && squad->addUnitsFromArea(attrUnits_, radius_, true);
	}

	AttributeUnitReferences::const_iterator i;
	FOR_EACH(attrUnits_, i)
		if(&unit->attr() == (*i)){
			UnitSquad* squad = safe_cast<UnitLegionary*>(unit)->squad();
			return !squad->locked() && squad->addUnitsFromArea(attrUnits_, radius_, true);
		}

	return false;
}	

bool ActionJoinSquads::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;
	return contextSquad_ && contextSquad_->addUnitsFromArea(attrUnits_, radius_);
}

void ActionJoinSquads::activate()
{
}

void ActionSetUnitAttackMode::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
}

bool ActionSetUnitAttackMode::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(squad_ && !unit->attr().isLegionary())
		xassert(0 && "У здания нет сквада");

	if(unit->attackMode().autoAttackMode() == attackMode_.autoAttackMode() &&
	   unit->attackMode().autoTargetFilter() == attackMode_.autoTargetFilter() &&
	   unit->attackMode().walkAttackMode() == attackMode_.walkAttackMode() &&
	   unit->attackMode().weaponMode() == attackMode_.weaponMode())
	   return false;

	return true;
}

void ActionSetUnitAttackMode::activate()
{
	UnitSquad* squad = contextUnit_->getSquadPoint();
	if(squad)
		squad->setAttackMode(attackMode_);
	else
		contextUnit_->setAttackMode(attackMode_);
}

ActionSetGamePause::ActionSetGamePause()
{
	switchType = ON;
}

void ActionSetGamePause::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchType, "switchType", "Действие");
}

ActionReturnToBase::ActionReturnToBase()
{
	radius_ = 100;
	mode_ = MODE_WALK;
	returnTime_ = 180;
}

UnitReal* ActionReturnToBase::findNearestHeadquater(const Vect2f& positionSquad) const
{
	if(headquaters_.empty() || !headquaters_.front())
		return 0;

	UnitReal* bestUnit = 0;
	float dist, bestDist = FLT_INF;
	AttributeBuildingReferences::const_iterator i;
	FOR_EACH(headquaters_, i){
		const RealUnits& realUnits = aiPlayer().realUnits(*i);
		if(!realUnits.empty()) {
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits, ui)
			if(bestDist > (dist = positionSquad.distance2((*ui)->position2D()))){
				bestDist = dist;
				bestUnit = *ui;
			}
		}
	}

	return bestUnit;
}

void ActionReturnToBase::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(headquaters_, "headquaters", "&Типы штаба");
	removeZeros(headquaters_);
	ar.serialize(radius_, "radius", "Радиус");
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(mode_, "mode", "Движение");
	ar.serialize(returnTime_, "returnTime", "Время возвращения");
}
ActionSetWalkMode::ActionSetWalkMode()
{
	mode_ = MODE_RUN;
}

bool ActionSetWalkMode::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!unit->attr().isLegionary())
		return false;

	const UnitLegionary* legionary = safe_cast<const UnitLegionary*>(unit); 
	switch(mode_){
		case MODE_RUN:
			if(legionary->runMode())
				return false;
			break;
		case MODE_WALK:
			if(!legionary->runMode())
				return false;
	}
	
	return true;
}

void ActionSetWalkMode::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mode_, "mode", "Режим движения");
}

void ActionSetWalkMode::activate()
{
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_CHANGE_MOVEMENT_MODE, mode_));
}

bool ActionReturnToBase::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	headquater = findNearestHeadquater(unit->position2D());
	if(headquater){
		float dist = headquater->position2D().distance(unit->position2D());
		if(headquater && dist > radius_ + headquater->radius() && unit->unitState() != UnitReal::MOVE_MODE) 
			return true;
	}
	return false;
}

ActionContinueConstruction::ActionContinueConstruction()
{
	mode_ = MODE_WALK;
}

void ActionContinueConstruction::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mode_, "mode", "Режим движения");
}

void ActionReturnToBase::activate()
{
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_CHANGE_MOVEMENT_MODE, mode_));
	contextSquad_->setAttackMode(attackMode_);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, headquater->position(), 0));
	contextSquad_->setUsedByTrigger(priority_, this, returnTime_*1000);
}

ActionSquadMoveToAnchor::ActionSquadMoveToAnchor() 
{
	mode = DO_NOT_WAIT;
}

bool ActionSquadMoveToAnchor::workedOut()
{
	if(unitList.empty() || !contextSquad_)
		return true;

	LegionariesLinks::iterator i;
	i = unitList.begin();
	while(i != unitList.end())
		if(!(*i)){
			i = unitList.erase(i);
			if(i == unitList.end())
				return false;
		}
		else
			++i;
	
	bool flag = true;
	if(mode != DO_NOT_WAIT){
		flag = false;
		i = unitList.begin();
		while(i != unitList.end()){
			if(!(*i)){
				i = unitList.erase(i);
				if(i == unitList.end())
					return false;
			}
			else{
				if(mode == WAIT_FOR_ALL){
					flag = true;
					if((*i)->position2D().distance2(anchor_->position2D()) > sqr((*i)->radius()) + sqr(anchor_->radius()))
						return false;
				}
				else
					if((*i)->position2D().distance2(anchor_->position2D()) < sqr((*i)->radius()) + sqr(anchor_->radius())){
						flag = true;
						break;
					}
				++i;
			}
		}
	}

	if(flag){
		contextSquad_->setUsedByTrigger(0);
		return true;
	}

	return false;
}

void ActionSquadMoveToAnchor::activate()
{
	unitList.clear();
	const LegionariesLinks& units = contextSquad_->units();
	LegionariesLinks::const_iterator ui; 
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, anchor_->position(), 0));
	FOR_EACH(units, ui)
		unitList.push_back(*ui);
	contextSquad_->setUsedByTrigger(priority_, this);
}

bool ActionPutSquadToAnchor::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!anchor_){
		xassert(0 && "Метка не найдена" && anchor_.c_str());
		return false;
	}

	if(!unit->attr().isLegionary())
		return false;

	//UnitSquad* squad = safe_cast<UnitLegionary*>(unit)->squad();
	//if(squad->position().distance2(anchor->position()) < sqr(anchor->radius()))
	//	return false;

	return true;
}
bool ActionOrderUnitsInSquad::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!unit->getSquadPoint())
		return false;

	return aiPlayer().canBuildStructure(attr_, &unit->getSquadPoint()->attr()) == CAN_START && 
		unit->getSquadPoint()->canQueryUnits(attr_) && aiPlayer().findFreeFactory(attr_);
}

void ActionOrderUnitsInSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attr_, "attr", "Тип заказываемого юнита");
}

void ActionOrderUnitsInSquad::activate() 
{
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_PRODUCTION_INC, attr_, Vect3f::ZERO, 0));
}

void ActionPutSquadToAnchor::activate()
{
	contextSquad_->clearOrders();
    LegionariesLinks::const_iterator i;
	const LegionariesLinks& units = contextSquad_->units();
	Vect3f squadCenter = contextSquad_->position();

	if(units.size() == 1 || !squad_){
		if(units.size() != 1){
			UnitLegionary* unit = safe_cast<UnitLegionary*>(&*contextUnit_);
			contextSquad_->removeUnit(unit);
			UnitSquad* squadIn = safe_cast<UnitSquad*>(aiPlayer().buildUnit(unit->attr().squad));
		    squadIn->setPose(unit->pose(), true);
			squadIn->addUnit(unit);
		}
		contextUnit_->setPose(anchor_->pose(), true); 
		return;
	} 

	FOR_EACH(units, i){
		Vect3f shift = (*i)->position() - squadCenter;
		shift = anchor_->pose().xformVect(shift);
		(*i)->setPose(Se3f(anchor_->pose().rot(), anchor_->position() + shift), true); 
	}
}


bool ActionSquadMoveToAssemblyPoint::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	anchor_ = 0;
	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi)
		if(&aiPlayer() != *pi && (*pi)->clan() == aiPlayer().clan())
			if(anchor_ = const_cast<Anchor*>((*pi)->requestedAssemblyPosition()))
				break;
	if(!anchor_)
		return false;

	if(unit->position().distance2(anchor_->position()) < sqr(unit->radius() + anchor_->radius()))
		return false;

	UnitSquad* squad = unit->getSquadPoint();
	return !(unit->unitState() == UnitReal::MOVE_MODE && 
		!squad->wayPointsEmpty() && squad->wayPoint().eq(anchor_->position(), anchor_->radius() + unit->radius()));
}

void ActionSquadMoveToAssemblyPoint::activate()
{
		contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, anchor_->position(), 0));
		contextSquad_->setUsedByTrigger(priority_, this, 60000);
}

bool ActionSquadMoveToAnchor::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!anchor_){
		xassert(0 && "Метка не найдена" && anchor_.c_str());
		return false;
	}

	UnitSquad* squad = unit->getSquadPoint();
	return !(unit->unitState() == UnitReal::MOVE_MODE && 
		   !squad->wayPointsEmpty() && squad->wayPoint().eq(anchor_->position(), anchor_->radius() + unit->radius()));
}

//-------------------------------------
ActionSetCamera::ActionSetCamera() 
{
	smoothTransition = false;
	cycles = 1;
	spline_ = 0;
}

void ActionSetCamera::activate() 
{ 
	if(aiPlayer().active()){
		cameraManager->setSkipCutScene(false);
		cameraManager->erasePath();
		if(spline_ = cameraManager->findSpline(cameraSplineName.c_str())){
			cameraManager->loadPath(*spline_, smoothTransition);
			int time = cameraManager->startReplayPath(spline_->stepDuration(), cycles);
			timer_.start(time);
		}
	}
}
 
bool ActionSetCamera::workedOut()
{
	if(aiPlayer().active() && cameraSplineName.empty())
		return true;
	if(!timer_.busy()){
		cameraManager->erasePath();
		return true;
	}
	if(spline_ && cameraManager->loadedSpline() != spline_)
		return true;

	return false;
}

void ActionOscillateCamera::activate() 
{
	cameraManager->startOscillation(duration*1000, factor);
}

void ActionOscillateCamera::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(duration, "duration", "Длительность, секунды");
	ar.serialize(factor, "factor", "Амплитуда");
}

ActionSaveAuto::ActionSaveAuto() : name_("")
{
}

void ActionSaveAuto::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(name_, "name", "Имя для сейва");
}

void ActionSaveAuto::activate()
{
	UI_LogicDispatcher::instance().saveGame(name_, true);
}

bool ActionSaveAuto::automaticCondition() const
{
	return !name_.empty();
}

void ActionSave::serialize(Archive& ar)
{
	__super::serialize(ar);
}

void ActionSave::activate()
{
	UI_LogicDispatcher::instance().saveGame();
}

void ActionActivateObjectByLabel::activate()
{
	if(!label_){
		xassertStr(0 && "Объект по метке не найден: ", label_.c_str());
	}
	else
		label_->hide(UnitReal::HIDE_BY_TRIGGER, !active_);
}

void ActionActivateObjectByLabel::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(label_, "label", "Имя объекта");
}	

ShowHeadData::ShowHeadData(bool camera)
: modelChain_(""), 
camera_(camera)
{}

bool ShowHeadData::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	// Format: "head.3dx, chain"

	if(ar.isEdit() && ar.isOutput()){
		cScene* scene = gb_VisGeneric->CreateScene();
		string comboList;
		ShowHeadNames::const_iterator hi;
		FOR_EACH(GlobalAttributes::instance().showHeadNames, hi){
			cObject3dx* model = scene->CreateObject3dx(*hi);
			if(!model)
				continue;
			int number = model->GetChainNumber();
			for(int i = 0; i < number; i++){
				const char* chainName = model->GetChain(i)->name.c_str();
				if(!camera_ && strstr(chainName, "camera_") || camera_ && !strstr(chainName, "camera_"))
					continue;
				if(!comboList.empty())
					comboList += "|";
				comboList += *hi;
				comboList += ", ";
				comboList += chainName;
			}
			model->Release();
		}
		modelChain_.setComboList(comboList.c_str());
		RELEASE(scene);
	}

	bool nodeExists = ar.serialize(modelChain_, name, nameAlt);
	
	if(ar.isInput()){
		model_ = modelChain_;
		int pos = model_.find(",");
		if(pos == string::npos)
			return nodeExists;
		chain_ = string(&model_[pos + 2], model_.size() - pos - 2);
		model_.erase(pos, model_.size());
	}

	return nodeExists;
}

ActionShowHead::ActionShowHead() 
: mainChain_(false),
silenceChain_(false),
cameraChain_(true)
{
	duration_ = 0;
	cycled_ = false;
	skinColor_.set(255,255,1,1);
	enable_ = true;
}

void ActionShowHead::activate()
{
	if(enable_)
	{
		showHead().LoadHead(mainChain_.model(),mainChain_.chain(),skinColor_);	
		showHead().Play(cycled_);
	}else
	{
		showHead().resetHead();
	}
}

void ActionShowHead::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(enable_,"Enable","Включить");
	ar.serialize(mainChain_, "mainChain", "Анимация");
	ar.serialize(cycled_,"Cycled","Зациклить");
	ar.serialize(skinColor_,"SkinColor","Skin Color");
}

void ActionInterrruptMessage::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(messageType_, "messageType", "Тип сообщения");
}

void ActionInterruptAnimation::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(messageType_, "messageType", "Тип сообщения");
}

void ActionInterrruptMessage::activate()
{
	ActiveMessages::iterator it;
	FOR_EACH(universe()->activeMessages(), it){
		ActionMessage* action = safe_cast<ActionMessage*>(*it);
		if(action->messageSetup().type() == messageType_)
			action->interrupt();
	}
}

void ActionInterruptAnimation::activate()
{
	ActiveMessages::iterator it;
	FOR_EACH(universe()->activeAnimation(), it){
		ActionSetObjectAnimation* action = safe_cast<ActionSetObjectAnimation*>(*it);
		if(action->messageSetup().type() == messageType_)
			action->interrupt();
	}
}


bool ActionInterrruptMessage::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	return aiPlayer().active();
}

bool ActionInterruptAnimation::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	return aiPlayer().active();
}

void ActionSetSignalVariable::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(acting_, "acting", "Действие");
	if(ar.isOutput() && ar.isEdit())
		signalVariable.setComboList(editSignalVariableDialog().c_str());
	ar.serialize(signalVariable, "signalVariable", "&Имя сигнальной переменной");
}

void ActionSetSignalVariable::activate()
{
	switch(acting_){
		case ACTION_ADD:
			aiPlayer().addUniqueParameter(signalVariable, 1);
			break;
		case ACTION_REMOVE:
			aiPlayer().addUniqueParameter(signalVariable, 0);
	}
}

ActionMessage::ActionMessage() 
{
	pause_ = 0;
	type_ = MESSAGE_ADD;
	fadeTime_ = 0;
}

ActionMessage::~ActionMessage()
{
	if(type_ == MESSAGE_ADD && workTimer_.started() && messageSetup_.isInterruptOnDestroy() && 
		messageSetup_.text() && messageSetup_.text()->hasVoice() && 
		voiceManager().validatePlayingFile(messageSetup_.text()->voice()))
		UI_Dispatcher::instance().removeMessage(messageSetup_, true);
}

void ActionMessage::activate()
{
	if(!messageSetup_.isEmpty()){
		switch(type_){
		case MESSAGE_ADD:{
			UI_Dispatcher::instance().playVoice(messageSetup_);
			UI_Dispatcher::instance().sendMessage(messageSetup_);
			universe()->activeMessages().add(this);
			int time = round(messageSetup_.displayTime()*1000.f);
			workTimer_.start(time ? time : INT_INF);
			}
			break;
		case MESSAGE_REMOVE:
			workTimer_.start();
			break;
		}
	}
}

void ActionMessage::interrupt() 
{ 
	workTimer_.start();
} 

bool ActionMessage::workedOut()
{ 
	if(workTimer_.busy())
		return false;
	
	if(!finishTimer_.started()){
		finishTimer_.start(round(pause_*1000.f));
		fadeTimer_.start(round(fadeTime_*1000.f));
		if(fadeTime_ && messageSetup_.hasVoice())
			voiceManager().FadeVolume(fadeTime_, 0);
		return false;
	}
	else if(finishTimer_.busy()){
		if(fadeTimer_.busy())
			UI_Dispatcher::instance().sendMessage(messageSetup_, 1.f - fadeTimer_.factor());
		return false;
	}
	else{
        UI_Dispatcher::instance().removeMessage(messageSetup_, false);
		universe()->activeMessages().remove(this);
		workTimer_.stop();
		fadeTimer_.stop();
		finishTimer_.stop();
		return true;
	}
} 

bool ActionMessage::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(type_ == MESSAGE_ADD && universe()->disabledMessages().find(messageSetup_.type()) != universe()->disabledMessages().end())
		return false;

	if(messageSetup_.text() && messageSetup_.text()->hasVoice() && !messageSetup_.isCanInterruptVoice() && voiceManager().isPlaying())
		return false;

	return aiPlayer().active();
}

ActionTask::ActionTask()
{
	state_ = UI_TASK_ASSIGNED;
	isSecondary_ = false;
}

void ActionTask::activate()
{ 
	if(!aiPlayer().active())
		return;

	UI_Dispatcher::instance().setTask(state_, messageSetup_, isSecondary_);

	int time = round(messageSetup_.displayTime() * 1000.f);
	durationTimer_.start(time);
} 

bool ActionTask::workedOut()
{ 
	return !durationTimer_.busy();
} 

bool ActionTask::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	return aiPlayer().active();
}

ActionSetCameraAtSquad::ActionSetCameraAtSquad()
{
	transitionTime = 1000;
}

UnitSquad* ActionSetCameraAtSquad::findSquad(const AttributeBase* attr, const Vect2f& nearPosition, float distanceMin) const
{
	UnitSquad* bestSquad = 0;
	float dist, bestDist = FLT_INF;
	SquadList::const_iterator si;
	const SquadList& squadList = aiPlayer().squads();  
	if(!squadList.empty() && attr) {
		FOR_EACH(squadList,si)
			if(&(*si)->attr() == attr && bestDist > (dist = nearPosition.distance2((*si)->position2D())) && dist > sqr(distanceMin)){
				bestDist = dist;
				bestSquad = *si;
			}
	}
	return bestSquad;
}

bool ActionSetCameraAtSquad::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	UnitSquad* squad = findSquad(attrSquad, 
		Vect2f(cameraManager->coordinate().position().x, cameraManager->coordinate().position().y), 0);
	if(squad)
		return true;
	return false;
}

void ActionSetCameraAtSquad::activate()
{
	UnitSquad* squad = findSquad(attrSquad, 
		Vect2f(cameraManager->coordinate().position().x, cameraManager->coordinate().position().y), 0);
	if(squad){
		cameraManager->SetCameraFollow(squad, transitionTime);
		timer.start(transitionTime);
	}
}

bool ActionSetCameraAtSquad::workedOut()
{
	return !timer.busy();
}


void ActionSetCameraAtSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrSquad, "attrSquad", "Тип сквада");
	//ar.serialize(transitionTime, "transitionTime", "Время перехода, секунды");
}

void ActionSetCameraAtObject::activate()
{
	turnStarted_ = false;

	cameraManager->setSkipCutScene(false);

	cameraManager->erasePath();

	spline_ = 0;
	UnitReal* unit = contextUnit_;
	if(unit && (spline_ = cameraManager->findSpline(cameraSplineName.c_str()))){
		cameraManager->loadPath(*spline_, false);
		int time = cameraManager->startReplayPath(spline_->stepDuration(), 1, unit);
		timer_.start(time);
		return;
	}

	if(setFollow && unit){
		cameraManager->SetCameraFollow(unit, transitionTime*1000);
	}
	else{
		Vect2f position;
		if(unit)
			position = unit->position2D();
		else
			return;
		CameraCoordinate coord(position, cameraManager->coordinate().psi(), 
										 cameraManager->coordinate().theta(), 
										 cameraManager->coordinate().fi(), 
										 cameraManager->coordinate().distance());
		if(transitionTime){
			CameraSpline spline;
			spline.push_back(coord);
			cameraManager->loadPath(spline, true);
			int time = cameraManager->startReplayPath(transitionTime*1000, 1);
			timer_.start(time);
		}
		else
			cameraManager->setCoordinate(coord);
	}
}

ActionSetObjectAnimation::ActionSetObjectAnimation() 
{
	switchMode = ON;
	counter = 0;
	mustInterrupt_ = false;
}

ActionSetObjectAnimation::~ActionSetObjectAnimation() 
{
	if(messageSetup_.isInterruptOnDestroy())
		UI_Dispatcher::instance().removeMessage(messageSetup_, true);
}

bool ActionSetObjectAnimation::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(messageSetup_ != UI_MessageSetup() && universe()->disabledMessages().find(messageSetup_.type()) != universe()->disabledMessages().end())
		return false;

	if(!messageSetup_.isCanInterruptVoice() && voiceManager().isPlaying())
		return false;

	return true;
}

void ActionSetObjectAnimation::activate()
{
	UnitReal* unit = contextUnit_;
	if(unit && ( unit->attr().isObjective() || unit->attr().isInventoryItem())){
		switch(switchMode) {
		case ON: {
			unit->startState(StateTrigger::instance());
			unit->setChain(CHAIN_TRIGGER, counter);
			UI_Dispatcher::instance().playVoice(messageSetup_);
			UI_Dispatcher::instance().sendMessage(messageSetup_);
			timer.start(messageSetup_.voiceDuration()*1000);
			universe()->activeAnimation().add(this);
		}
		break;

		case OFF: 
			unit->finishState(StateTrigger::instance());
		}
	}
}

bool ActionSetObjectAnimation::workedOut()
{
	if(mustInterrupt_){
		universe()->activeAnimation().remove(this);
		UI_Dispatcher::instance().removeMessage(messageSetup_, true);
		if(contextUnit_)
			contextUnit_->finishState(StateTrigger::instance());
		mustInterrupt_ = false;
		return true;
	}

	if(!timer.busy()){
		universe()->activeAnimation().remove(this);
		return true;
	}

	return false;
}

void ActionSetObjectAnimation::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(counter, "counter", "Номер цепочки триггера");
	ar.serialize(switchMode, "switchMode", "Действие");
	ar.serialize(messageSetup_, "messageSetup", "&Сообщение");
	ar.serialize(timer, "timer", 0);
}

bool ActionSetCameraAtObject::workedOut()
{
	if(timer_.busy() && spline_ && cameraManager->loadedSpline() != spline_)
		return true;

	if(!timer_.busy()){
		cameraManager->erasePath();
		if(!turnStarted_ && turnTime){
			xassert(!setFollow);
			UnitReal* unit = contextUnit_;
			if(unit){
				CameraSpline spline;
				spline.push_back(
					CameraCoordinate(unit->position2D(), cycle(cameraManager->coordinate().psi(), 2*M_PI) + 2*M_PI, cameraManager->coordinate().theta(), 
														 cameraManager->coordinate().fi(), cameraManager->coordinate().distance()));
				cameraManager->loadPath(spline, true);
				int time = cameraManager->startReplayPath(turnTime*1000, 1);
				timer_.start(time);
				turnStarted_ = true;
				return false;
			}
		}
		return true;
	}

	return false;
}

void ActionSetInterface::activate()
{
	UI_Dispatcher::instance().setEnabled(enableInterface);
}

ActionShowLogoReel::ActionShowLogoReel()
{
	localized = false;
	workTime = 5000;
	fishAlpha = 0.3f;
	logoAlpha = 0.7f;
	fadeTime = 1.f;
}

ActionShowReel::ActionShowReel()
{
	localizedVideo = false;
	localizedVoice = true;
	stopBGMusic = true;
	alpha = 255;
	skip = true;
}

void ActionShowReel::serialize(Archive& ar)
{
	__super::serialize(ar);
	static ResourceSelector::Options optionsVideo("*.bik", "Resource\\", "Will select location of video file");
	ar.serialize(ResourceSelector(binkFileName, optionsVideo), "binkFileName", "&Имя видеофайла");
	static ResourceSelector::Options optionsVoice("*.ogg", "Resource\\", "Will select location of voice file");
	ar.serialize(ResourceSelector(soundFileName, optionsVoice), "soundFileName", "&Имя звукового файла");
	ar.serialize(localizedVideo, "localizedVideo", "Локализация видео");
	ar.serialize(localizedVoice, "localizedVoice", "Локализация звука");
	ar.serialize(stopBGMusic, "stopBGMusic", "Выключать фоновую музыку");
	ar.serialize(alpha, "alpha", "Альфа");
	ar.serialize(skip, "skip", "Возможность пропуска ролика");
}

void ActionShowLogoReel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(ModelSelector(groundName, ModelSelector::DEFAULT_OPTIONS), "groundName", "Модель дна");
	ar.serialize(ModelSelector(fishName, ModelSelector::DEFAULT_OPTIONS), "modelName", "Модель рыбки");
	ar.serialize(ModelSelector(logoName, ModelSelector::DEFAULT_OPTIONS), "logoName", "Модель логотипа");
	float tmp = fishAlpha*100;
	ar.serialize(tmp,"fishAlpha","Прозрачность рыбки при наведении (%)");
	fishAlpha = tmp*0.01f;
	tmp = logoAlpha*100;
	ar.serialize(tmp,"logoAlpha","Прозрачность логотипа при наведении (%)");
	logoAlpha = tmp*0.01f;
	ar.serialize(fadeTime,"fadeTime","Время изменения прозрачности");
	ar.serialize(soundAttributes,"soundAttributes","Звуки");
	ar.serialize(blobsSetting, "blobsSetting", "Параметры шариков");
	ar.serialize(localized, "localized", "Локализация");
	ar.serialize(workTime, "workTime", "Время показа (мс)");
}

bool ActionLoadGameAuto::checkMissionExistence(const string& missionName, GameType gameType) const
{
	name = UI_LogicDispatcher::instance().profileSystem().getSavePath(gameType);
	name += missionName;
	name = setExtention(name.c_str(), MissionDescription::getExtention(gameType));
	return isFileExists(name.c_str());
}

ActionAttackLabel::ActionAttackLabel() 
{
	mode_ = IDLE;
	timeToAttack_ = 10;
	step_ = 0;
	helixAngle_ = M_PI/10.f;
	helixRadius_ = 50;
}

bool ActionAttackLabel::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(helixRadius_ > max(vMap.H_SIZE, vMap.V_SIZE))
		return false;
	
	return aiPlayer().centerPosition().distance2(Vect2f::ZERO) > 1;
}

bool ActionAttackLabel::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	return true;
}

void ActionAttackLabel::activate()
{
	if(mode_ == THROW)
		mode_ = DIG;
	center_ = aiPlayer().centerPosition();
	aiPlayer().startUsedByTriggerAttack(timeToAttack_*1000);
	durationTimer_.start(timeToAttack_*1000);
	contextUnit_->setUsedByTrigger(priority_, this);
	contextUnit_->setPreSelectedWeaponID(weaponDig_->ID());
}

bool ActionAttackLabel::workedOut()
{
	if(!contextUnit_)
		return true;

	if(!contextUnit_->checkUsage(this) || durationTimer_.finished()){
		aiPlayer().directShootCommand(To3D(position_), 0);
		contextUnit_->setUsedByTrigger(0, this);
		contextUnit_->setPreSelectedWeaponID(0);
		aiPlayer().startUsedByTriggerAttack(0);
		return true;
	}

	switch(mode_){
	case IDLE:
		mode_ = FIND;
		break;

	case FIND: 
		if(helixRadius_ > max(vMap.H_SIZE, vMap.V_SIZE)){
			contextUnit_->setUsedByTrigger(0, this);
			return true;
		}
		position_ = clampWorldPosition(center_ + Mat2f(helixAngle_)*Vect2f(helixRadius_, 0), EnginePrm::instance().actionAttackLabel.sourceRadius);
		helixAngle_ += EnginePrm::instance().actionAttackLabel.helixDeltaAngle/helixAngle_;
		helixRadius_ += EnginePrm::instance().actionAttackLabel.helixDeltaRadius;
		aiPlayer().directShootCommand(To3D(position_), 0);
		if(checkSource(position_)){
			mode_ = DIG;
			contextUnit_->setPreSelectedWeaponID(weaponDig_->ID());
		}
		break;

	case DIG:
		if(safe_cast<UnitPlayer*>(&*contextUnit_)->currentCapacity() < 0.99f){
			step_ = ++step_ % 10;
			Vect3f curPos = To3D(position_ + Mat2f(step_*M_PI/10.f)*Vect2f(EnginePrm::instance().actionAttackLabel.digRadius, 0));
			if(!step_){
				aiPlayer().directShootCommand(curPos, 1);
				contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, curPos, weaponDig_->ID()));
			}
			aiPlayer().directShootCommandMouse(curPos);

			if(checkCrystal(position_)){
				aiPlayer().directShootCommand(To3D(position_), 0);
				mode_ = PICK;
			}
		}
		else{
			contextUnit_->setPreSelectedWeaponID(weaponThrow_->ID());
			mode_ = THROW;
		}
		break;

	case THROW:
		if(safe_cast<UnitPlayer*>(&*contextUnit_)->currentCapacity() > 0.01f){
			step_ = ++step_ % 20;
			Vect3f curPos = To3D(position_ + Mat2f(step_*M_PI/10.f)*Vect2f(EnginePrm::instance().actionAttackLabel.throwRadius, 0));
			if(!step_){
				aiPlayer().directShootCommand(curPos, 1);
				contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, curPos, weaponThrow_->ID()));
			}
			aiPlayer().directShootCommandMouse(curPos);
		}
		else{
			contextUnit_->setPreSelectedWeaponID(weaponDig_->ID());
			mode_ = DIG;
		}
		break;

	case PICK:
		if(checkCrystal(position_)){
			step_ = ++step_ % 10;
			Vect3f curPos = To3D(position_ + Mat2f(step_*M_PI/10.f)*Vect2f(EnginePrm::instance().actionAttackLabel.pickRadius, 0));
			aiPlayer().directShootCommand(curPos, step_ ? 1 : 0);
			contextUnit_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, safe_cast<UnitInterface*>(crystalFound_), weaponDig_->ID()));
		}
		else{
			mode_ = IDLE;
			aiPlayer().directShootCommand(To3D(position_), 0);
		}
		break;
	}

	return false;		
}

bool ActionAttackLabel::checkCrystal(const Vect2f& position) 
{
	crystalFound_ = 0;
	return !universe()->unitGrid.ConditionScan(position.x, position.y, EnginePrm::instance().actionAttackLabel.sourceRadius, *this);
}

bool ActionAttackLabel::operator()(UnitBase* unit)
{
	if(&unit->attr() == crystal_){
		crystalFound_ = unit;
		return false;
	}
	return true;
}

bool ActionAttackLabel::checkSource(const Vect2f& position) 
{
	return !sourceManager->sourceGrid.ConditionScan(position.x, position.y, EnginePrm::instance().actionAttackLabel.sourceRadius, *this);
}

bool ActionAttackLabel::operator()(SourceBase* source)
{
	if(strstr(source->label(), "CRYSTAL") && source->position2D().distance2(position_) < sqr(EnginePrm::instance().actionAttackLabel.sourceRadius)){
		position_ = source->position2D();
		return false;
	}
	return true;
}

void ActionAttackLabel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(crystal_, "crystal", "Тип кристалла (Метка на мире должна содержать 'CRYSTAL'!)");
	ar.serialize(weaponDig_, "|weaponDig|weapon", "Оружие для копания");
	ar.serialize(weaponThrow_, "|weaponThrow|weapon1", "Оружие для высыпания");
	ar.serialize(timeToAttack_, "timeToAttack", "Время атаки");
}

bool ActionLoadGameAuto::automaticCondition() const
{
	return __super::automaticCondition() && checkMissionExistence(missionName, gameType_);
}

bool ActionStartMission::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;
	XStream ff(0);
	if(ff.open(missionName.c_str())){
		ff.close();
		return true;
	}

	return false;
}

void ActionLoadGameAuto::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(missionName, "missionName", "Имя сейва для загрузки");
	ar.serialize(gameType_, "gameType", "Режим загрузки");
}

void ActionUI_GameStart::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(paused_, "paused", "На паузе");
	ar.serialize(isBattle_, "isBattle", "Запускать как сражение");
}

void ActionStartMission::serialize(Archive& ar)
{
	__super::serialize(ar);
	static ResourceSelector::Options missionOptions("*.spg; *.rpl", "Resource\\Worlds", "Will select location of mission file", false);
	ar.serialize(ResourceSelector(missionName, missionOptions), "missionName", "Имя миссии");
	ar.serialize(gameType_, "gameType", "Режим загрузки");
	ar.serialize(paused_, "paused", "На паузе");
	ar.serialize(preloadScreen_, "preloadScreen", "Экран для предзагрузки");
}

void ActionSetCursor::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(cursor, "cursor", "Курсор");
}

void ActionChangeUnitCursor::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attribute, "attribute", "Юнит");
	ar.serialize(cursor, "cursor", "Курсор");
}

void ActionUnitParameterArithmetics::activate()
{
	aiPlayer().applyParameterArithmetics(unit, arithmetics);
}

void ActionObjectParameterArithmetics::activate()
{
	contextUnit_->applyParameterArithmetics(arithmetics);
}

void ActionObjectParameterArithmetics::serialize(Archive& ar)
{
	__super::serialize(ar);
	arithmetics.serialize(ar);
}

void ActionUnitParameterArithmetics::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(unit, "unit", "Тип юнита");
	arithmetics.serialize(ar);
}

ActionUI_UnitCommand::ActionUI_UnitCommand()
: unitCommand(*new UnitCommand(COMMAND_ID_UPGRADE, 0))
{
}

ActionUI_UnitCommand::~ActionUI_UnitCommand()
{
	delete &unitCommand;
}

void ActionUI_UnitCommand::serialize(Archive& ar)
{
	__super::serialize(ar);
	unitCommand.serialize(ar);
}

ActionAIUnitCommand::ActionAIUnitCommand()
{}

void ActionAIUnitCommand::activate()
{
	contextUnit_->executeCommand(unitCommand);
	if(&*contextUnit_)
		contextUnit_->setUsedByTrigger(priority_, this);
}

ActionGuardUnit::ActionGuardUnit()
{
	attackTime_ = 180;
	maxDistance_ = 300.f;
}

ActionAttack::ActionAttack()
{
	attackTime_ = 180;
	aimObject_ = AIM_ANY;
	ignoreWorldPlayer_ = true;
}

ActionAttackMyUnit::ActionAttackMyUnit()
{
	attackTime_ = 180;
	maxDistance_ = 300.f; 
}

ActionSellBuilding::ActionSellBuilding()
{
	onlyConnected_ = true;
}

void ActionSellBuilding::activate()
{
	contextUnit_->executeCommand(UnitCommand(COMMAND_ID_UNINSTALL,0));
}

bool ActionSellBuilding::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	return onlyConnected_ ? unit->isConnected() : true;
}

void ActionSellBuilding::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(onlyConnected_, "onlyConnected", "Только подключенные");
}

bool ActionEscapeUnderShield::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	return true;
}

void ActionEscapeUnderShield::activate()
{
	int r = 20;
	float angle = 0.f;
	Vect2f pos = contextUnit_->position2D();
	while(r < 500)
	{
		while(angle < 2*M_PI)
		{
			Vect2f aim = Vect2f(pos.x + r*cos(angle), pos.y + r*sin(angle));
			ShieldScanOp shieldOp(To3D(aim), &aiPlayer());
			sourceManager->sourceGrid.Scan(aim, contextUnit_->radius(), shieldOp);
			if(!shieldOp.underShield() && !pathFinder->checkImpassability(aim.xi(), aim.yi(), contextSquad_->impassability(), contextSquad_->passability()))
			{
				contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim.x, aim.y, contextUnit_->position().z), 0));
				contextSquad_->setUsedByTrigger(priority_, this, 6000);
				return;
			}
			angle += M_PI / 6.0f;
		}
		angle = 0.f;
		r += 20;
	}
}

bool ActionAttack::checkContextUnit(UnitActing* unit) const
{
	start_timer_auto();

	if(!__super::checkContextUnit(unit))
		return false;

	if(unit->isUpgrading() || !unit->isDefaultWeaponExist())
		return false;

	UnitSquad* squad = unit->getSquadPoint();
    if(squad->locked())
		return false;

	Vect2f center = squad->position2D();
 	target_ = 0;
	float priority, bestPriority = 0;
	float bestDistance = FLT_INF;
	PlayerVect::iterator pi;

	const float Check_Radius = 100.f;

	// список приоритетных объектов для атаки
	AttributeUnitOrBuildingReferences::const_iterator ai;
	FOR_EACH(attrPriorityTarget_, ai){
		FOR_EACH(universe()->Players, pi)
			if(!(*pi)->isWorld() || !ignoreWorldPlayer_)
				if(aiPlayer().isEnemy(*pi)){
					const RealUnits& realUnits = (*pi)->realUnits(*ai);
					RealUnits::const_iterator ui;
					FOR_EACH(realUnits, ui)
						if((*ui)->alive() && (*ui)->attr().isActing() && !(*ui)->attr().excludeFromAutoAttack && (aimObject_ == AIM_ANY || (aimObject_ == AIM_UNIT  && (*ui)->attr().isLegionary()) || (aimObject_ == AIM_BUILDING  && (*ui)->attr().isBuilding())) && 
							(!((*ui)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*ui)->attr()).teleport) && squad->canAttackTarget(WeaponTarget(*ui)) && 
							(((*ui)->attr().isLegionary() && !(*ui)->isUnseen()) || (*ui)->attr().isBuilding()))){
								ShieldScanOp shieldOp((*ui)->position(), &aiPlayer());
								sourceManager->sourceGrid.Scan((*ui)->position2D(), (*ui)->radius(), shieldOp);
								if(shieldOp.underShield() && !unit->selectedWeapon()->canShootThroughShield())
									continue;
								float distance = unit->getSquadPoint()->position2D().distance((*ui)->position2D());
								if(bestDistance + Check_Radius > distance){
									if(distance < bestDistance){ 
										if(bestDistance - distance > Check_Radius){
											target_ = *ui;
											bestDistance = distance;
										}
									}
									if(!target_)
										target_ = *ui;
									priority = microAI_TargetPriority(unit, (*ui));
									if(bestPriority < priority || (bestPriority == priority && distance < bestDistance)){
										bestPriority = priority;
										target_ = *ui;
										bestDistance = distance;
									}
								}
						}
			}
	}
	if(!target_){
		// конкретный объект для атаки или все юниты
		FOR_EACH(universe()->Players, pi)
			if(!(*pi)->isWorld() || !ignoreWorldPlayer_)
				if(aiPlayer().isEnemy(*pi)){
					const RealUnits& realUnits = attrTarget_ ? (*pi)->realUnits(attrTarget_) : (*pi)->realUnits();
					RealUnits::const_iterator ui;
					FOR_EACH(realUnits, ui)
						if((*ui)->alive() && (*ui)->attr().isActing() && !(*ui)->attr().excludeFromAutoAttack && (aimObject_ == AIM_ANY || (aimObject_ == AIM_UNIT  && (*ui)->attr().isLegionary()) || (aimObject_ == AIM_BUILDING  && (*ui)->attr().isBuilding())) &&
							(!((*ui)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*ui)->attr()).teleport) && squad->canAttackTarget(WeaponTarget(*ui)) && 
							(((*ui)->attr().isLegionary() && !(*ui)->isUnseen()) || (*ui)->attr().isBuilding()))){
								ShieldScanOp shieldOp((*ui)->position(), &aiPlayer());
								sourceManager->sourceGrid.scanPoint((*ui)->position2D().x, (*ui)->position2D().y, shieldOp);
								if(shieldOp.underShield() && !unit->selectedWeapon()->canShootThroughShield())
									continue;
								float distance = unit->getSquadPoint()->position2D().distance((*ui)->position2D());
								if(bestDistance + Check_Radius > distance){
									if(distance < bestDistance){ 
										if(bestDistance - distance > Check_Radius){
											target_ = *ui;
											bestDistance = distance;
										}
									}
									if(!target_)
										target_ = *ui;
									priority = microAI_TargetPriority(unit, (*ui));
									if(bestPriority < priority || (bestPriority == priority && distance < bestDistance)){
										bestPriority = priority;
										target_ = *ui;
										bestDistance = distance;
									}
								}
						}
				}
	}

	return target_;
}

bool ActionAttackMyUnit::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(unit->targetUnit() || unit->isUpgrading() || !unit->isDefaultWeaponExist() || unit->getSquadPoint()->locked())
		return false;

	target_ = 0;
	float distMin = isEq(maxDistance_, 0.f) ? FLT_INF : sqr(maxDistance_);
	
	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi)
		if((*pi)->clan() == aiPlayer().clan()){
			if(attrTargets_.empty() || !attrTargets_.front()){
				const RealUnits& realUnits = aiPlayer().realUnits();
				UnitReal* foundUnit = 0;
				if(foundUnit = findTarget(unit, realUnits, distMin))
					target_ = foundUnit;
			}
			else 
			{
				AttributeUnitOrBuildingReferences::const_iterator i; 
				FOR_EACH(attrTargets_, i){
					const RealUnits& realUnits = aiPlayer().realUnits(*i);
					UnitReal* foundUnit = 0;
					if(foundUnit = findTarget(unit, realUnits, distMin))
						target_ = foundUnit;
				}
			}
		}

	return target_;
}

UnitReal* ActionAttackMyUnit::findTarget(UnitReal* unit, const RealUnits& units, float minDist) const
{
	UnitReal* target = 0;
	float dist = FLT_INF;
	Vect2f center = unit->getSquadPoint()->position2D();

	RealUnits::const_iterator ui;
	FOR_EACH(units, ui)
		if((*ui)->alive() && (*ui)->attr().isActing() && unit != *ui && unit->getSquadPoint()->canAttackTarget(WeaponTarget(*ui)) && 
			(((*ui)->attr().isLegionary() && !(*ui)->isUnseen()) || (*ui)->attr().isBuilding()) &&  
			minDist > (dist = center.distance2((*ui)->position2D()))){
			minDist = dist;
			target = *ui;
		}

	return target;
}

bool ActionGuardUnit::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(unit->isUpgrading() || !unit->isDefaultWeaponExist())
		return false;

	float maxDist = isEq(maxDistance_, 0.f) ? FLT_INF : sqr(maxDistance_);

	target_ = 0;
	if(attrTargets_.empty() || !attrTargets_.front()){
		const RealUnits& realUnits = aiPlayer().realUnits();
		UnitReal* foundUnit = 0;
		if(foundUnit = findTarget(unit, realUnits, maxDist))
			target_ = foundUnit;
	}
	else 
	{
		AttributeUnitOrBuildingReferences::const_iterator i; 
		FOR_EACH(attrTargets_, i){
			const RealUnits& realUnits = aiPlayer().realUnits(*i);
			UnitReal* foundUnit = 0;
			if(foundUnit = findTarget(unit, realUnits, maxDist)){
				target_ = foundUnit;
				break;
			}	
		}
	}

	return target_ && unit->targetUnit() != target_;
}

UnitReal* ActionGuardUnit::findTarget(UnitReal* unit, const RealUnits& realUnits, float maxDist) const
{
	UnitReal* target = 0;

	RealUnits::const_iterator ui;
	FOR_EACH(realUnits, ui){
		if(!safe_cast<UnitActing*>(*ui)->underAttack())
			continue;
		
		if((*ui)->position2D().distance2(unit->position2D()) < maxDist){		 
			UnitTargetScanOp unitTargetOp(*ui, &aiPlayer());
			universe()->unitGrid.Scan((*ui)->position().x, (*ui)->position().y, (*ui)->sightRadius(), unitTargetOp);
			if(target = safe_cast<UnitReal*>(unitTargetOp.unit()))		 
				break;
		}
	}

	return target;
}

void ActionAttack::activate()
{
	contextSquad_->setAttackMode(attackMode_);
	if(target_ != contextUnit_->targetUnit())
		contextSquad_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, target_, 0));
	contextSquad_->setUsedByTrigger(priority_, this, attackTime_*1000);
}

void ActionAttackMyUnit::activate()
{
	contextSquad_->setAttackMode(attackMode_);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, target_, 0));
	contextSquad_->setUsedByTrigger(priority_, this, attackTime_*1000);
}

void ActionGuardUnit::activate()
{
	contextSquad_->setAttackMode(attackMode_);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, target_, 0));
	contextSquad_->setUsedByTrigger(priority_, this, attackTime_*1000);
}

bool ActionSetUnitLevel::checkContextUnit(UnitActing* unit) const
{
	return __super::checkContextUnit(unit) && unit->attr().isLegionary();
}

void ActionSetUnitLevel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(level_, "level", "Уровень");
}

void ActionSetUnitLevel::activate()
{
	safe_cast<UnitLegionary*>(&*contextUnit_)->setLevel(level_, false);
}

bool ActionSetUnitMinimapMark::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	return enable_ == ON ? !unit->isSpecialMinimapMark() : unit->isSpecialMinimapMark();
}

void ActionSetUnitMinimapMark::activate()
{
	contextUnit_->setSpecialMinimapMark(enable_ == ON ? true : false);
}

void ActionSetUnitMinimapMark::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(enable_, "enable", "Действие с пометкой юнита на миникарте");
}

void ActionGuardUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(maxDistance_, "maxDistance", "Максимальное растояние до охраняемого юнита");
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(attrTargets_, "attrTargets_", "&Типы юнитов для охраны");
	removeZeros(attrTargets_);
	ar.serialize(attackTime_, "attackTime", "Время атаки, секунды");
}

void ActionAttack::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(attrPriorityTarget_, "attrPriorityTarget", "Приоритетные цели");
	removeZeros(attrPriorityTarget_);
	ar.serialize(attrTarget_, "attrTarget", "Тип цели");
	ar.serialize(attackTime_, "attackTime", "Время атаки, секунды");
	ar.serialize(aimObject_, "aimObject", "Цель");
	ar.serialize(ignoreWorldPlayer_, "ignoreWorldPlayer", "Игнорировать игрока Мир");
}

void ActionAttackMyUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(attrTargets_, "attrTargets_", "&Типы цели");
	removeZeros(attrTargets_);
	ar.serialize(attackTime_, "attackTime", "Время атаки, секунды");
	ar.serialize(maxDistance_, "maxDistance", "Максимальное расстояние до цели");
}

ActionSetInt::ActionSetInt()
{
	scope_ = SCOPE_UNIVERSE;
	value_ = 0;
	valueBool_ = 0;
}

void ActionSetInt::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(scope_, "scope", "Область действия переменной");
	if(scope_ != SCOPE_MISSION_DESCRIPTION){
		ar.serialize(name_, "name", "Имя переменной");
		ar.serialize(value_, "value", "Значение");
	}
	else{
		missionDescription.serialize(ar);
		ar.serialize(RangedWrapperi(value_, 0, 31), "value", "&Номер переменной");
		ar.serialize(valueBool_, "valueBool", "Значение");
	}
}

bool ActionContext::isContext(ContextFilter& filter) const 
{ 
	filter.addUnit(object_);
	return true; 
}

void ActionContext::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(object_, "object", "Тип юнита");
}

bool ActionContextSquad::isContext(ContextFilter& filter) const 
{ 
	__super::isContext(filter);
	filter.addSquad(squad_);
	return true; 
}

bool ActionContextSquad::checkContextUnit(UnitActing* unit) const 
{ 
	if(!ActionContext::checkContextUnit(unit))
		return false;
	if(!unit->attr().isLegionary())
		return false;
	UnitSquad* squad = safe_cast<UnitLegionary*>(unit)->squad();
	return (!squad_ || &squad->attr() == squad_) && !squad->usedByTrigger(priority_);
}

bool ActionFollowSquad::checkContextUnit(UnitActing* unit) const
{
	return __super::checkContextUnit(unit) && !unit->getSquadPoint()->followSquad();
}

bool ActionContext::checkContextUnit(UnitActing* unit) const
{ 
	return (!object_ || object_ == &unit->attr()) && !unit->usedByTrigger(priority_); 
}

void ActionContextSquad::setContextUnit(UnitActing* unit)
{
	__super::setContextUnit(unit);
	xassert(unit->attr().isLegionary());
	contextSquad_ = safe_cast<UnitLegionary*>(unit)->squad();
}

void ActionContextSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(squad_, "squad", "Тип сквада");
}

ActionSetIgnoreFreezedByTrigger::ActionSetIgnoreFreezedByTrigger() 
{
	switchMode_ = ON;
}

void ActionSetIgnoreFreezedByTrigger::activate() 
{
	contextUnit_->setIgnoreFreezedByTrigger(switchMode_);
}

void ActionSetIgnoreFreezedByTrigger::serialize( Archive& ar ) 
{
	__super::serialize(ar);
	ar.serialize(switchMode_, "switchMode", "Включить");
}

ActionRestartTriggers::ActionRestartTriggers()
{
	resetToStart_ = true;
}

void ActionRestartTriggers::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(resetToStart_, "resetToStart", "Перезапустить с начала (нет - продолжить от текущего триггера)");
}

void ActionStartTrigger::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(triggerChainName_, "triggerChainName", "Имя триггера");
}

void ActionStartTrigger::activate()
{
	aiPlayer().startTigger(triggerChainName_);
}

ActionSetCutScene::ActionSetCutScene()
{
	switchMode = ON;
}

void ActionSetCutScene::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchMode, "switchMode", "Режим");
}

void ActionReadPlayerParameters::activate()
{
	if(aiPlayer().active()){
		ParameterSet resource = aiPlayer().resource();
		resource.mask(GlobalAttributes::instance().profileParameters);
		aiPlayer().subResource(resource);
		aiPlayer().addResource(universe()->currentProfileParameters(), false);
	}
}

void ActionCommandsQueue::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(commandsQueue_, "commandsQueue", "Очередь команд");
}

void ActionCommandsQueue::activate() 
{
	LegionariesLinks actors;
	CommandsQueue::Actors::const_iterator i;
	FOR_EACH(commandsQueue_->actors, i)
		actors.push_back(safe_cast<UnitLegionary*>(aiPlayer().findUnit(*i)));
	processor_ = new CommandsQueueProcessor(*commandsQueue_, actors);
}

bool ActionCommandsQueue::workedOut()
{
	if(processor_ && processor_->quant())
		return false;
	processor_ = 0;
	return true;
}

void ActionInterruptCommandsQueue::activate() 
{
	CommandsQueueProcessor::interrupt();
}

void ActionWritePlayerParameters::activate()
{
	if(aiPlayer().active()){
		ParameterSet resource = GlobalAttributes::instance().profileParameters;
		resource.set(0);
		resource += aiPlayer().resource();
		resource.mask(GlobalAttributes::instance().profileParameters);
		universe()->currentProfileParameters() = resource;
	}
}

ActionDeselect::ActionDeselect()
{
	stopPlayerUnit_ = true;
}

void ActionDeselect::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(stopPlayerUnit_, "stopPlayerUnit", "Выключать оружие игрока");
}
