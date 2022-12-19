#include "StdAfx.h"
#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "terra.h"
#include "Universe.h"

#include "Squad.h"
#include "Serialization.h"
#include "ShowHead.h"
#include "PlayOgg.h"
#include "ResourceSelector.h"

#include "Actions.h"
#include "Conditions.h"
#include "IronBuilding.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"
#include "..\Water\Water.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\Ai\PlaceOperators.h"
#include "..\Ai\PFTrap.h"
#include "..\UserInterface\SelectManager.h"
#include "..\Game\SoundApp.h"
#include "RangedWrapper.h"
#include "GameOptions.h"
#include "..\Units\MicroAI.h"
#include "..\Sound\SoundSystem.h"

#include "StreamCommand.h"

// параметры поиска для установки здания
int ai_building_pause = 200; // ms
int ai_scan_step = 64;
float ai_scan_size_of_step_factor = 2;
int ai_scan_step_unable_to_find = 8; // При этом шаге прекращается сканирование, считая, что нужной позиции нет
int ai_placement_iterations_per_quant = 10;

// параметры атаки спец оружием 
float scanRadiusAim = 20.f;
int maxWeaponSpecialScan = 5;

// парметры поиска ресурсов
const int maxResourceScan = 20;

// параметры апгрейда
const int maxTimeToUpgrade = 60000;
const float stepOnConvexSide = 20.f;

string editLabelDialog();
string editAnchorLabelDialog();
////////////////////////////////////////////////////
//		Actions
/////////////////////////////////////////////////////
STARFORCE_API void initActions()
{
SECUROM_MARKER_HIGH_SECURITY_ON(2);

REGISTER_CLASS(Action, Action, "Глобальные действия\\Пустое действие")
REGISTER_CLASS(Action, ActionDelay, "Глобальные действия\\Задержка времени")
REGISTER_CLASS(Action, ActionRestartTriggers, "Глобальные действия\\Сделать неактивными все стрелки триггера")
REGISTER_CLASS(Action, ActionExitFromMission, "Глобальные действия\\Выход из миссии")
REGISTER_CLASS(Action, ActionGameQuit, "Глобальные действия\\Выход из игры")
REGISTER_CLASS(Action, ActionShowReel, "Глобальные действия\\Показать ролик")
REGISTER_CLASS(Action, ActionShowLogoReel, "Глобальные действия\\Показать программный ролик")
REGISTER_CLASS(Action, ActionStartMission, "Глобальные действия\\Запустить миссию")
REGISTER_CLASS(Action, ActionSetCurrentMission, "Глобальные действия\\Установить текущую миссию")
REGISTER_CLASS(Action, ActionReseCurrentMission, "Глобальные действия\\Очистить текущую миссию")
REGISTER_CLASS(Action, ActionSwitchTriggers, "Глобальные действия\\Включить/Выключить триггера")
REGISTER_CLASS(Action, ActionSetPlayerWin, "Глобальные действия\\Считать игрока выигравшим");
REGISTER_CLASS(Action, ActionSetPlayerDefeat, "Глобальные действия\\Считать игрока проигравшим");
REGISTER_CLASS(Action, ActionSetInt, "Глобальные действия\\Установка целочисленной переменной")
REGISTER_CLASS(Action, ActionSetSignalVariable, "Глобальные действия\\Сигнальная переменная");
REGISTER_CLASS(Action, ActionSetCutScene, "Глобальные действия\\Включить/выключить кат-сцену (убирает игровую информацию)");

REGISTER_CLASS(Action, ActionUpgradeUnit, "Контекстные действия\\Апгрейд юнита определенного типа")
REGISTER_CLASS(Action, ActionAttackLabel, "Контекстные действия\\Атаковать метку на мире")
REGISTER_CLASS(Action, ActionAttackBySpecialWeapon, "Контекстные действия\\Атаковать специальным оружием")
REGISTER_CLASS(Action, ActionActivateSpecialWeapon, "Контекстные действия\\Активировать/Деактивировать спец. оружие")
REGISTER_CLASS(Action, ActionAttack, "Контекстные действия\\Атаковать юнитами")
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
REGISTER_CLASS(Action, ActionSetUnitInvisible, "Контекстные действия\\Сделать юнита невидимым")
REGISTER_CLASS(Action, ActionSetUnitSelectAble, "Контекстные действия\\Вкл./выкл. возможность селекта юнита")
REGISTER_CLASS(Action, ActionSellBuilding, "Контекстные действия\\Продать здание")
REGISTER_CLASS(Action, ActionOrderBuildings, "Контекстные действия\\Заказать здание")
REGISTER_CLASS(Action, ActionContinueConstruction, "Контекстные действия\\Достроить здания")
REGISTER_CLASS(Action, ActionSquadMoveToAnchor, "Контекстные действия\\Отправить сквад в метку на якоре")
REGISTER_CLASS(Action, ActionPutSquadToAnchor, "Контекстные действия\\Переместить сквад в метку на якоре")
REGISTER_CLASS(Action, ActionSetCameraAtObject, "Контекстные действия\\Установить камеру на объект")
REGISTER_CLASS(Action, ActionSetObjectAnimation, "Контекстные действия\\Вкл./выкл. анимацию объекта")
REGISTER_CLASS(Action, ActionSetIgnoreFreezedByTrigger, "Контекстные действия\\Вкл./выкл. игнорирование общей заморозки для отдельного объекта")
REGISTER_CLASS(Action, ActionObjectParameterArithmetics, "Контекстные действия\\Арифметика параметров юнита")
REGISTER_CLASS(Action, ActionSetUnitLevel, "Контекстные действия\\Установить уровень юнита(не влияет на параметры)")

REGISTER_CLASS(Action, ActionOrderBuildingsOnZone, "АИ\\Заказать здание на зоне")
REGISTER_CLASS(Action, ActionOrderUnits, "АИ\\Заказать юнитов на заводе")
REGISTER_CLASS(Action, ActionOrderParameters, "АИ\\Производство параметров")
REGISTER_CLASS(Action, ActionUnitParameterArithmetics, "АИ\\Арифметика параметров")

REGISTER_CLASS(Action, ActionCreateUnit, "Миссии\\Создать объект в точке якоря")
REGISTER_CLASS(Action, ActionSwitchPlayer, "Миссии\\Переключиться на игрока")
REGISTER_CLASS(Action, ActionSquadMove, "Миссии\\Послать сквад в точку объекта по метке")
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
REGISTER_CLASS(Action, ActionSetHotKey, "Интерфейс\\Назначить горячую клавишу на кнопку")
REGISTER_CLASS(Action, ActionInterfaceHideControl, "Интерфейс\\Спрятать кнопку мягко, ПЕРЕДЕЛАТЬ на /Операции над кнопками/")
REGISTER_CLASS(Action, ActionInterfaceHideControlTrigger, "Интерфейс\\Спрятать кнопку жестко, ПЕРЕДЕЛАТЬ на /Операции над кнопками/")
REGISTER_CLASS(Action, ActionInterfaceControlOperate, "Интерфейс\\Операции над кнопками")
REGISTER_CLASS(Action, ActionInterfaceTogglAccessibility, "Интерфейс\\Переключить доступность кнопки")
REGISTER_CLASS(Action, ActionInterfaceSetControlState, "Интерфейс\\Переключить состояние кнопки")
REGISTER_CLASS(Action, ActionInterfaceUIAnimationControl, "Интерфейс\\Управление анимацией кнопки")
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

REGISTER_CLASS(Action, ActionSetCursor, "Курсоры\\Установить курсор")
REGISTER_CLASS(Action, ActionFreeCursor, "Курсоры\\Отменить установленный курсор")
REGISTER_CLASS(Action, ActionChangeUnitCursor, "Курсоры\\Сменить курсор юнита")
REGISTER_CLASS(Action, ActionChangeCommonCursor, "Курсоры\\Сменить общий курсор")

SECUROM_MARKER_HIGH_SECURITY_OFF(2);
}
//FORCE_REGISTER_CLASS(Action, ActionDeactivateSources, "Погода\\Деактивировать источники")
//FORCE_REGISTER_CLASS(Action, ActionPlaySoundTrack, "Интерфейс\\Включить музыкальный трек")


////////////////////////////////////////////////////

BEGIN_ENUM_DESCRIPTOR(AttackCondition, "AttackCondition")
REGISTER_ENUM(ATTACK_GROUND, "Атаковать поверхность")
REGISTER_ENUM(ATTACK_ENEMY_UNIT, "Атаковать юнитов врага(с учетом здоровья)")
REGISTER_ENUM(ATTACK_GROUND_NEAR_ENEMY_UNIT, "Атаковать поверхность рядом с юнитом врага")
REGISTER_ENUM(ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING, "Атаковать поверхность рядом с юнитом врага продолжительное")
REGISTER_ENUM(ATTACK_MY_UNIT, "Атаковать своих поврежденных юнитов")
END_ENUM_DESCRIPTOR(AttackCondition)

BEGIN_ENUM_DESCRIPTOR(UpgradeOption, "UpgradeOption")
REGISTER_ENUM(UPGRADE_HERE, "В любой обстановке")
REGISTER_ENUM(UPGRADE_ON_THE_DISTANCE, "На расстоянии от любого здания")
REGISTER_ENUM(UPGRADE_ON_THE_DISTANCE_TO_ENEMY, "На расстоянии от базы в сторону врага")
REGISTER_ENUM(UPGRADE_NEAR_OBJECT, "Рядом с объектом")
REGISTER_ENUM(UPGRADE_ON_THE_DISTANCE_FROM_ENEMY, "На расстоянии от базы врага")
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

BEGIN_ENUM_DESCRIPTOR(MovementMode, "MovementMode")
REGISTER_ENUM(MODE_RUN, "Бежать")
REGISTER_ENUM(MODE_WALK, "Идти")
END_ENUM_DESCRIPTOR(MovementMode)


//SNDSound ActionSoundMessage::ctrl;

ActionEnableMessage::ActionEnableMessage()
{
	mode_ = ON;
}

void ActionEnableMessage::activate()
{
	switch(mode_){
		case ON:
			{
				universe()->disabledMessages().erase(messageType_);
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

void ActionCreateUnit::activate()
{
	const Anchor* anchor = environment->findAnchor(anchorLabel_);
	if(!anchor){
		xassert_s(0 && "Якорь по метке не найден: ", anchorLabel_);
		return;
	}												
	else{
		Player* player = 0;
		if(player_ == -1)
			player = &aiPlayer();
		else
			player = universe()->findPlayer(player_);
		if(player){
			UnitSquad* squad = 0;
			for(int i = 0; i < count_; i++){
				UnitReal* unit = safe_cast<UnitReal*>(player->buildUnit(attr_));
				Vect3f pos = anchor->position();				
				if(count_ > 1)
					pos += Mat3f(2*M_PI*i/count_, Z_AXIS)*Vect3f(unit->radius()*2, 0, 0);
				unit->setPose(Se3f(anchor->pose().rot(), pos),true);
				unit->setLabel(unitLabel_.c_str());
				if(unit->attr().isLegionary()){
					UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
					if(!squad || !inTheSameSquad_){
						squad = safe_cast<UnitSquad*>(player->buildUnit(&*legionary->attr().squad));
						squad->setPose(unit->pose(), true);
					}
					squad->addUnit(legionary, false);
				}
			}
		}
	}
}

void ActionCreateUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit() && universe())
		anchorLabel_.setComboList(editAnchorLabelDialog().c_str());
	ar.serialize(anchorLabel_, "anchorLabel_", "&Метка(на якоре)");
	ar.serialize(attr_, "attr", "Тип юнита");
	ar.serialize(count_, "count", "Количество");
	ar.serialize(inTheSameSquad_, "inTheSameSquad", "В одном скваде");
	ar.serialize(unitLabel_, "unitLabel", "Метка для юнита");
	ar.serialize(player_, "player", "Номер игрока");
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

	if(unit->getUnitState() == UnitReal::MOVE_MODE || unit->isUsedByTrigger())
		return false;

	unit_ = findNearestItem(unit);

	return unit_ && unit_->position2D().distance2(unit->position2D()) > sqr(unit_->radius() +  unit->radius())*sqr(1.5f);
}

bool ActionSquadMoveToObject::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(unit->getUnitState() == UnitReal::MOVE_MODE || unit->isUsedByTrigger())
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
	contextSquad_->setShowAISign(true);
}

void ActionSquadMoveToItem::activate()
{
	//Vect3f point(unit_->position().x + unit_->radius()+10, unit_->position().y + unit_->radius()+10, unit_->position().z);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, unit_->position(), 0));
	contextSquad_->setShowAISign(true);
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
	Vect2f pos = Vect2f(unit->position().x, unit->position().y);
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
	Vect2f pos = Vect2f(unit->position().x, unit->position().y);
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

	if(!SNDIsSoundEnabled() || !soundReference || soundReference->is3D() || (!aiPlayer().active() && !aiPlayer().isWorld()))
		return false;
	return true;
}

void ActionSoundMessage::activate() 
{
	switch(switchMode_)
	{
	case ON:
		environment->soundEnvironmentManager()->PlaySound(soundReference);
		break;
	case OFF:
		environment->soundEnvironmentManager()->StopSound(soundReference);
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
	LegionariesLinks::const_iterator i;
	FOR_EACH(contextSquad_->units(), i){
		(*i)->setUsedByTrigger(false);
		(*i)->setShowAISign(false);
	}
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
	if(!ar.serialize(controlMode_, "controlMode", "Режим прямого управления")){ // CONVERSION 07.11.06
		SwitchMode switchMode = ON;
		ar.serialize(switchMode, "switchMode", 0);
		controlMode_ = (switchMode == ON) ? DIRECT_CONTROL_ENABLED : DIRECT_CONTROL_DISABLED; 
	}
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

	ar.serialize(weaponref_, "weaponref_", "Спецоружия");
	removeNotAlive<WeaponPrmReferences>(weaponref_);
	ar.serialize(attackCondition, "attackCondition", "Условие атаки");
	ar.serialize(minDistance, "minDistance", "Мин. расстояние от хозяина");  
	ar.serialize(passAbility, "passAbility", "Можно атаковать непроходимую поверхность");
	ar.serialize(timeToAttack_, "timeToAttack", "Продолжительность атаки");
}

void ActionExploreArea::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mainBuilding, "mainBuilding", "Главное строение");
	ar.serialize(exploringRadius, "exploringRadius", "Радиус разведки");
}

bool ActionAttackBySpecialWeapon::checkContextUnit(UnitActing* unit) const
{
	return __super::checkContextUnit(unit) && !unit->isUpgrading() && !unit->isDocked() && unit->currentState() != CHAIN_BIRTH_IN_AIR &&
		(unit->attr().isBuilding() ? unit->isConstructed() : true) && !unit->specialFireTargetExist();
}

bool ActionAttackBySpecialWeapon::automaticCondition() const
{
	//start_timer_auto();

	if(weaponref_.empty())
		return false;

	if(!__super::automaticCondition())
		return false;

	bool canFire_ = false;
	WeaponPrmReferences::const_iterator it;
	FOR_EACH(weaponref_, it)
		if(contextUnit_->canFire((*it).get()->ID())){
			canFire_ = true;
			break;
		}
		else
			if((*it)->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE && contextUnit_->canDetonateMines())
				contextUnit_->detonateMines();

	if(!canFire_){
		return false;
	}

	targetUnit_ = 0;
	firePosition_ = Vect2f::ZERO;
	UnitActing* unit = safe_cast<UnitActing*>(&*contextUnit_);
	ID = (*it).get()->ID();

	switch(attackCondition){
		case ATTACK_GROUND:
			{
				WeaponScanOp scanOp(safe_cast<const UnitActing*>(&*contextUnit_), aiPlayer(), scanRadiusAim, (*it));
				float angle_step = M_PI/16;
				if(floatEqual(factorRadius, -1.f)){
					float minRadius = safe_cast<const UnitActing*>(&*contextUnit_)->minFireRadiusOfWeapon((*it).get()->ID());
					float dist_ = minDistance > minRadius ? minDistance : minRadius;
					factorRadius = contextUnit_->radius() + dist_;
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
					point = Vect2f(contextUnit_->position2D().x + factorRadius*cos(angle), 
								   contextUnit_->position2D().y + factorRadius*sin(angle)); 
					point = Vect2f(
						clamp(point.x, 0, vMap.H_SIZE),
						clamp(point.y, 0, vMap.V_SIZE));
					angle += angle_step;
					if(passAbility || (!pathFinder->impassabilityCheck(point.xi(), point.yi(), round(scanRadiusAim))))
						scanOp.checkPosition(point);
					scanCount++;
					if(contextUnit_->radius() + safe_cast<const UnitActing*>(&*contextUnit_)->fireRadiusOfWeapon((*it).get()->ID()) < factorRadius){
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
					if(!virtualUnit) {
						virtualUnit = aiPlayer().buildUnit(AuxAttributeReference(AUX_ATTRIBUTE_ZONE));
						virtualUnit->setRadius(scanRadiusAim);
						virtualUnit->setPose(Se3f(QuatF::ID, Vect3f(point.x , point.y, 0)), true);
					}
					virtualUnit->setPose(Se3f(QuatF::ID, Vect3f(point.x , point.y, 0)), true);
					angle = 0;
					factorRadius = -1;
					// debug
					/*if(scanOp->valid()){ 
						Vect2f v1 = Vect2f(point.x - 10, point.y - 10);
						Vect2f v2 = v1 + Vect2f(10, 10);
						show_vector(To3D(v1),To3D(Vect2f(v1.x,v2.y)),To3D(v2),To3D(Vect2f(v2.x,v1.y)),aiPlayer().unitColor());
						XBuffer buf;
						buf.SetDigits(6);
						buf < "ID : " <= (*it).get()->ID();
						show_text(To3D(point), buf, aiPlayer().unitColor());
						//unit->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(point.x, point.y, 0), weaponref.get()->ID()));	
					}*/
					return true;
				}
				break;
			}
		case ATTACK_ENEMY_UNIT:
			{
				MicroAiScaner scanOp(unit, ATTACK_MODE_DEFENCE, ID);
				universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(ID)), scanOp);
				UnitInterface* p = scanOp.processTargets();
				if(p)
				{
					firePosition_ = p->position2D();
					targetUnit_ = p;
					return true;
				}
				/*	OLD SCAN
				WeaponUnitScanOp scanOp(safe_cast<const UnitActing*>(&*contextUnit_), aiPlayer(), *it, false, true);
				scanOp.checkPosition(Vect2f(contextUnit_->position().x, contextUnit_->position().y));
				if(scanOp.valid()) {
					firePosition_ = scanOp.foundPos();
					targetUnit_ = scanOp.foundUnit();
					return true;
				}*/
				break;
			}
		case ATTACK_GROUND_NEAR_ENEMY_UNIT:
			{
				if(unit->findWeapon(ID)->weaponPrm()->weaponClass() != WeaponPrm::WEAPON_WAITING_SOURCE){
					MicroAiScaner scanOp(unit, ATTACK_MODE_DEFENCE, ID);
					universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(ID)), scanOp);
					UnitInterface* p = scanOp.processTargets();
					if(p)
					{
						firePosition_ = p->position2D();
						targetUnit_ = 0;
						return true;
					}
				}
				else{
					UnitActing* unit = safe_cast<UnitActing*>(&*contextUnit_); 
					WeaponBase* weapon = unit->findWeapon(ID);
					WeaponUnitScanOp scanOp(unit , aiPlayer(), *it, false, false);
					scanOp.checkPosition(Vect2f(contextUnit_->position().x, contextUnit_->position().y));
					if(scanOp.valid() &&  weapon->canAttack(WeaponTarget(scanOp.foundPos(), ID))) {
						firePosition_ = scanOp.foundPos();
						targetUnit_ = 0;
						return true;
					}
				}
				break;
			}
		case ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING:
			{
				firstTime = true;
				MicroAiScaner scanOp(unit, ATTACK_MODE_DEFENCE, ID);
				universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(ID)), scanOp);
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
				MicroAiScaner scanOp(unit, ATTACK_MODE_DEFENCE, ID);
				universe()->unitGrid.Scan(unit->position().x, unit->position().y, max(unit->sightRadius(), unit->fireRadiusOfWeapon(ID)), scanOp);
				UnitInterface* p = scanOp.processTargets();
				if(p)
				{
					firePosition_ = p->position2D();
					targetUnit_ = p;
					return true;
				}
				/* OLD SCAN 
				WeaponUnitScanOp scanOp(safe_cast<const UnitActing*>(&*contextUnit_), aiPlayer(), *it, true, true);
				scanOp.checkPosition(Vect2f(contextUnit_->position().x, contextUnit_->position().y));
				if(scanOp.valid()) {
					firePosition_ = scanOp.foundPos();
					targetUnit_ = scanOp.foundUnit();
					return true;
				}*/
			}

	}

	contextUnit_->selectWeapon(0);
	return false;
}

void ActionActivateSpecialWeapon::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(weaponref_, "weaponref", "Спец. оружия");
	removeNotAlive<WeaponPrmReferences>(weaponref_);
	ar.serialize(mode_, "mode", "Действие");
}

bool ActionActivateSpecialWeapon::automaticCondition() const
{
	if(weaponref_.empty())
		return false;

	if(!__super::automaticCondition())
		return false;

	if(!contextUnit_->isConstructed() || contextUnit_->isUpgrading())
		return false;

	switch(mode_){
		case ON:{
			bool canFire_ = false;

			WeaponPrmReferences::const_iterator it;
			FOR_EACH(weaponref_, it){
				int weaponID = (*it).get()->ID();
				if(contextUnit_->canFire(weaponID)){
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
				int weaponID = (*it).get()->ID();
				WeaponBase* weapon = safe_cast<UnitActing*>(&*contextUnit_)->findWeapon(weaponID);
				if(weapon && weapon->isEnabled()){
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
		int weaponID = (*it).get()->ID();
		switch(mode_){
			case ON: 
				if(contextUnit_->canFire(weaponID)){
					if(contextUnit_->getSquadPoint()){
						contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_WEAPON_ACTIVATE, weaponID));
						contextUnit_->getSquadPoint()->setShowAISign(true);
					}	
					else{
						contextUnit_->executeCommand(UnitCommand(COMMAND_ID_WEAPON_ACTIVATE, weaponID));
						contextUnit_->setShowAISign(true);
					}
				}
				break;
			case OFF:
				WeaponBase* weapon = safe_cast<UnitActing*>(&*contextUnit_)->findWeapon(weaponID);
				if(weapon && weapon->isEnabled()){
					if(contextUnit_->getSquadPoint()){
						contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_STOP, weaponID));
						contextUnit_->getSquadPoint()->setShowAISign(true);
					}
					else{
						contextUnit_->executeCommand(UnitCommand(COMMAND_ID_STOP, weaponID));
						contextUnit_->setShowAISign(false);
					}
				}
		}
	}
}

void ActionAttackBySpecialWeapon::activate()
{
	WeaponPrmReferences::const_iterator it;
	FOR_EACH(weaponref_, it){
		int weaponID = (*it).get()->ID();
		UnitActing* unit = safe_cast<UnitActing*>(&*contextUnit_);
		WeaponBase* weapon = unit->findWeapon(weaponID);
		if(weapon && weapon->weaponPrm()->weaponClass() != WeaponPrm::WEAPON_WAITING_SOURCE){
			if(contextUnit_->canFire(weaponID) && 
				(!targetUnit_ ? true : weapon->canAttack(WeaponTarget(safe_cast<UnitInterface*>(&*targetUnit_), targetUnit_->position(), weaponID)))){
				if(contextUnit_->getSquadPoint()){
					contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID));
					contextUnit_->getSquadPoint()->setShowAISign(true);
				}
				else{
					contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID));
					contextUnit_->setShowAISign(true);
				}
				float reloadTime = weapon->parameters().findByType(ParameterType::RELOAD_TIME, 1.0f) * 1000.0f;
				resetTime_.start(reloadTime + 100);
			}
		}
	}

	dist = 10.f;
    anglex = 0;
}

void ActionAttackBySpecialWeapon::clear()
{
	if(contextUnit_){
		contextUnit_->setUsedByTrigger(false);
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

	if(!contextUnit_ || !contextUnit_->alive())
		return true;

	if(firePosition_.eq(Vect2f::ZERO) || !contextUnit_)
		return true;

	if(!resetTime_.was_started()){
		WeaponPrmReferences::const_iterator it;
		FOR_EACH(weaponref_, it){
			int weaponID = (*it).get()->ID();
			UnitActing* unit = safe_cast<UnitActing*>(&*contextUnit_);
			WeaponBase* weapon = unit->findWeapon(weaponID);
			if(weapon){
				if(weapon->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE){
					float maxDistance = unit->fireRadiusOfWeapon(weaponID);
					if(unit->canFire(weaponID)){
						while(dist < maxDistance)
						{
							while(anglex < M_PI * 2.0f)
							{
								anglex += M_PI * 10.f / dist;
								Vect2f position = Vect2f(firePosition_.x + cos(anglex)* dist, firePosition_.y + sin(anglex)* dist);
								WeaponTarget& weaponTarget = WeaponTarget(To3D(position), weaponID);
								if(unit->position2D().distance(position) < maxDistance && unit->fireCheck(weaponTarget) &&	weapon->checkFogOfWar(weaponTarget) && weapon->canAttack(weaponTarget)){
									if(contextUnit_->getSquadPoint()){
										contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID));
										contextUnit_->getSquadPoint()->setShowAISign(true);
									}
									else{
										contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, Vect3f(firePosition_.x, firePosition_.y, 0), weaponID));
										contextUnit_->setShowAISign(true);
									}
									return false;
								} 	
							}
							anglex = 0.f;
							dist += 10.f;
						}
						contextUnit_->setUsedByTrigger(false);
						contextUnit_->setUnitState(UnitReal::AUTO_MODE);
						contextUnit_->clearAttackTarget();

						if(!contextUnit_->getSquadPoint())
							contextUnit_->selectWeapon(0);
						else
							contextUnit_->getSquadPoint()->selectWeapon(0);

						return true;
					}

					if(unit->canDetonateMines()){
						unit->detonateMines();
						float reloadTime = weapon->parameters().findByType(ParameterType::RELOAD_TIME, 1.0f) * 1000.0f;
						resetTime_.start(reloadTime + 100);
					}

					if(dist >= maxDistance)
						return true;
				}
			}
		}
	}

	if(attackCondition == ATTACK_GROUND_NEAR_ENEMY_UNIT_LASTING){
		const Vect3f& pos = To3D(firePosition_); 
		if(contextUnit_->fireDistanceCheck(WeaponTarget(pos, ID)))
		{
			if(firstTime)
			{
				firstTime = false;
				durationTimer_.start(timeToAttack_ * 1000);
				aiPlayer().executeCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT, pos, 1));
				return false;
			}
			if(durationTimer_())
			{
				aiPlayer().executeCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT_MOUSE,  pos));
				return false;
			}
			else
			{
				aiPlayer().executeCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT, pos, 0));
				contextUnit_->setUsedByTrigger(false);
				contextUnit_->setUnitState(UnitReal::AUTO_MODE);
				contextUnit_->clearAttackTarget();
				if(!contextUnit_->getSquadPoint())
					contextUnit_->selectWeapon(0);
				else
					contextUnit_->getSquadPoint()->selectWeapon(0);
				return true;
			}
		}
		else{
			if(contextUnit_->getUnitState() != UnitReal::ATTACK_MODE ){
				contextUnit_->setUsedByTrigger(false);
				contextUnit_->setShowAISign(false);
				return true;
			}
			return false;
		}
	}
	else{
		bool canFire_ = false;
		WeaponPrmReferences::const_iterator it;
		FOR_EACH(weaponref_, it)
			if(contextUnit_->canFire((*it).get()->ID())){
				canFire_ = true;
				break;
			}
		
		if(resetTime_.was_started() && (!resetTime_ || !canFire_))
		{
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

ActionAttackBySpecialWeapon::ActionAttackBySpecialWeapon()
{
	attackCondition = ATTACK_GROUND;
	virtualUnit = 0;
	//angle = 0;
	factorRadius = -1.f;
	minDistance = 10.f;
	passAbility = true;
	ID = 0;
	timeToAttack_ = 2;
	dist = 10.f;
}

ActionAttackBySpecialWeapon::~ActionAttackBySpecialWeapon()
{
	//if(virtualUnit) 
	//	delete virtualUnit;
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
	if(!safe_cast<UnitLegionary*>(unit)->gatheringResource() && !unit->isUsedByTrigger() &&
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
	Vect2f pos = Vect2f(unit->position().x, unit->position().y);
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
	contextSquad_->setShowAISign(true);
}

bool ActionOrderBuildings::checkContextUnit(UnitActing* unit) const
{

	if(!__super::checkContextUnit(unit))
		return false;

	if(!attrBuilding)
		return false;

	if(startDelay){
		startDelay = false;
		// задержка времени для АИ
		difficultyTimer.start(aiPlayer().difficulty().get()->orderBuildingsDelay*1000);

		return false;
	}

	if(!difficultyTimer){
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
		if(attrBuilding.get()->needBuilders) {
			
			check = false;
			
			const RealUnits& unitList = aiPlayer().realUnits();
			RealUnits::const_iterator ui;
			FOR_EACH(unitList, ui) {
				if((*ui)->attr().isLegionary()) {
					AttributeBuildingReferences::const_iterator bi;
					const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>((*ui)->attr()); 
					FOR_EACH(attr.constructedBuildings, bi) 
						if(*bi == attrBuilding && !safe_cast<UnitLegionary*>(*ui)->constructingBuilding() && !(*ui)->isUsedByTrigger() && !(*ui)->isDocked()){
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

		PlaceScanOp scanOp(unit, attrBuilding, aiPlayer(), closeToEnemy_, radius_, 1000, extraDistance_);
		scanOp.checkPosition(unit->position2D());

		if(scanOp.found())
		{
			found_ = true;
			best_position = scanOp.bestPosition();
			return true;
		}
	}
	
	return false;
}

void ActionContinueConstruction::activate()
{
	switch(mode_){
		case MODE_RUN:
			contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_GO_RUN, (class UnitInterface*)0, 0));
			break;
		case MODE_WALK:
			contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_STOP_RUN, (class UnitInterface*)0, 0));
	}
	
	contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_OBJECT, unit_, 0));
	contextUnit_->getSquadPoint()->setShowAISign(true);
}

bool ActionContinueConstruction::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	if(!unit->attr().isLegionary())
		return false;

	unit_ = 0;
	if(!safe_cast<UnitLegionary*>(unit)->constructingBuilding() && !unit->isUsedByTrigger()){
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

void ActionOrderBuildingsOnZone::activate()
{
	finishPlacement();
	startDelay = true;
}

void ActionOrderBuildingsOnZone::placeBuildingSpecial() const
{	
	// рассматриваются все юниты мира
	const RealUnits& resourceList = universe()->worldPlayer()->realUnits();
	if(!resourceList.empty()) {
		if(indexScan < resourceList.size()) {
			RealUnits::const_iterator ri = resourceList.begin() + indexScan;
			PlacementZone buildingPlacementZone = safe_cast<const AttributeBuilding*>(attrBuilding.get())->placementZone;
			while(ri != resourceList.end()) {
				const AttributeBase& attrRes = (*ri)->attr();
				indexScan++;
				if((*ri)->attr().isResourceItem() && !(*ri)->isUnseen() && buildingPlacementZone == attrRes.producedPlacementZone && 
				   aiPlayer().fogOfWarMap()->getFogState((int) (*ri)->position2D().x, (int) (*ri)->position2D().y) != FOGST_FULL){
					const RealUnits& units = aiPlayer().realUnits();
					RealUnits::const_iterator i;
					bool soNear = false;
					FOR_EACH(units, i)
						if((*i)->position2D().distance2((*ri)->position2D()) < sqr(500.f)){
							soNear = true;
							break;
						}
					if(soNear){
						Vect2f snapPoint_;
						if(checkBuildingPosition(attrBuilding, (*ri)->position2D(), false, snapPoint_))
						{
							found_ = true;
							best_position = (*ri)->position2D();
							builder_state = FoundWhereToBuild;
							break;
						}
						float radius = attrRes.producedPlacementZoneRadius + attrBuilding.get()->radius();
						startPlace(Vect2i((*ri)->position().x - radius, (*ri)->position().y - radius),
								Vect2i((*ri)->position().x + radius, (*ri)->position().y + radius),
								ai_scan_step);
						return;
					}
				}
				ri++;
			}

			indexScan = 0;
		}
	}

	if(!found_) {
		builder_state = UnableToFindWhereToBuild;
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
	scanPolyByLineOp(points, 4, line_op);
	if(line_op.valid() && safe_cast<const AttributeBuilding*>(attr.get())->checkBuildingPosition(position, Mat2f::ID, &aiPlayer(), checkUnits, snapPosition))
		return true;

	return false;
}

void ActionOrderBuildingsOnZone::clear()
{
	__super::clear();
	if(builder_ && builder_->alive())
		builder_->setUsedByTrigger(false); // освобождаем юнита для других триггеров
}

void ActionOrderBuildingsOnZone::BuildingQuant() const
{	
	switch(builder_state){
		case BuildingIdle: {	

			indexScan = 0;

			placeBuildingSpecial();

			break;
		}	

		case FoundWhereToBuild: {

			break;
		}

		case UnableToFindWhereToBuild: {

			indexScan = 0;

			if(builder_ && builder_->alive()){ 
				builder_->setUsedByTrigger(false); // освобождаем юнита для других триггеров
				builder_->setShowAISign(false);
			}

			break;
		}

		case FindingWhereToBuild: {
			findWhereToBuildQuantSpecial();

			break;
		}

		case BuildingPause: {

			if(!building_pause)
				builder_state = BuildingIdle;

			break;
		}
	}
}	

void ActionOrderBuildingsOnZone::finishPlacement()
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
			if(builder_ && builder_->alive() && !builder_->constructingBuilding()) {
				builder_->setUsedByTrigger(false);
				builder_->squad()->executeCommand(UnitCommand(COMMAND_ID_OBJECT, curBuilding, 0));
				builder_->setShowAISign(true);
 				builder_state = BuildingPause;
				building_pause.start(ai_building_pause);
				return;
			}

			builder_state = BuildingPause;
			building_pause.start(ai_building_pause);
			if(builder_ && builder_->alive()){
				builder_->setUsedByTrigger(false);
				builder_->setShowAISign(false);
			}
			return;
		}
	}

	// если произошел срыв (позиция не валидная, или не смог построить)
	if(builder_ && builder_->alive()){ 
		builder_->setUsedByTrigger(false);
		builder_->setShowAISign(false);
	}	
	builder_state = BuildingPause;
	building_pause.start(ai_building_pause);

}

void ActionOrderBuildings::finishPlacement()
{
	Vect2f snapPoint_;

	if(found_ && safe_cast<const AttributeBuilding*>(attrBuilding.get())->checkBuildingPosition(best_position, Mat2f::ID, &aiPlayer(), true, snapPoint_)){
		UnitBuilding* curBuilding = aiPlayer().buildStructure(attrBuilding, Vect3f(best_position.x, best_position.y, 0.f), false);
		if(curBuilding){
			if(builder_ && builder_->alive() && !builder_->constructingBuilding()) {
				builder_->squad()->executeCommand(UnitCommand(COMMAND_ID_OBJECT, curBuilding, 0));
				builder_->setShowAISign(true);
			}
		}
	}
}

ActionOrderBuildingsOnZone::ActionOrderBuildingsOnZone()
{
	builder_state = BuildingIdle;
	builder_ = 0;
	indexScan = 0;
	found_ = false;
	foundPosition_ = Vect2f::ZERO;
	startDelay = true;
}


ActionOrderBuildings::ActionOrderBuildings()
{
	builder_ = 0;
	radius_ = 0.f;
	extraDistance_ = 0.f;
	closeToEnemy_ = false;

	found_ = false;
	startDelay = true;
}

bool ActionOrderBuildingsOnZone::automaticCondition() const 
{	
	if(!__super::automaticCondition()) 
		return false;

	if(!attrBuilding)
		return false;

	if(startDelay){
		startDelay = false;
		// задержка времени для АИ
		difficultyTimer.start(aiPlayer().difficulty().get()->orderBuildingsDelay*1000);

		return false;
	}
	if(!difficultyTimer){
		// проверка условий возможности строительства по ресурсам
		if(!aiPlayer().checkUnitNumber(attrBuilding))
			return false;
		if(!aiPlayer().accessible(attrBuilding))
			return false;
		if(!aiPlayer().requestResource(attrBuilding->installValue, NEED_RESOURCE_TO_INSTALL_BUILDING))
			return false;

		bool check = true;

		builder_ = 0;

		// если требуется строитель
		if(attrBuilding.get()->needBuilders) {
			
			check = false;
			
			const RealUnits& unitList = aiPlayer().realUnits();
			RealUnits::const_iterator ui;
			FOR_EACH(unitList, ui) {
				if((*ui)->attr().isLegionary()) {
					AttributeBuildingReferences::const_iterator bi;
					const AttributeLegionary& attr = safe_cast_ref<const AttributeLegionary&>((*ui)->attr()); 
					FOR_EACH(attr.constructedBuildings, bi) 
						if(*bi == attrBuilding && !safe_cast<UnitLegionary*>(*ui)->constructingBuilding() && !(*ui)->isUsedByTrigger() && !(*ui)->isDocked()){
							builder_ = safe_cast<UnitLegionary*> (*ui);
							builder_->setUsedByTrigger(true);
							builder_state = BuildingIdle;
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

		// если строитель умер :(
		if(builder_ && !builder_->alive()){
			builder_ = 0;
			indexScan = 0;
			builder_state = BuildingIdle;
			return false;
		}


		ActionOrderBuildingsOnZone();
		BuildingQuant();

		if(builder_state == UnableToFindWhereToBuild)
		{
			builder_ = 0;
			indexScan = 0;
			builder_state = BuildingIdle;
			return false;
		}

		if(builder_ && builder_state != FoundWhereToBuild){
			builder_->setUsedByTrigger(false);
			builder_->setShowAISign(false);
		}
		return builder_state == FoundWhereToBuild;
	}

	return false;
}

void ActionOrderBuildingsOnZone::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(attrBuilding, "attrBuilding", "Тип здания");
}

void ActionOrderBuildings::activate()
{
	finishPlacement();
	startDelay = true;
}

void ActionOrderBuildings::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(attrBuilding, "attrBuilding", "Тип здания");
	ar.serialize(radius_, "radius_", "Радиус");
	ar.serialize(extraDistance_, "extraDistance_", "Расстояние между зданиями данного типа");
	ar.serialize(closeToEnemy_, "closeToEnemy_", "В направлении врага");
}

ActionUpgradeUnit::ActionUpgradeUnit()
{
	upgradeOption = UPGRADE_HERE;
	scanOp = 0;
	upgradeNumber = 0; 
	extraRadius = 100;
	unit_ = 0; 
	interrupt_ = false; 
}

void ActionUpgradeUnit::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(upgradeNumber, "upgradeNumber", "Номер апгрейда");
	ar.serialize(upgradeOption, "upgradeOption", "Произвести апгрейд");
	ar.serialize(extraRadius, "extraRadius", "Расстояние до объектов");
	ar.serialize(objects_, "objects", "Рядом объекты");
	removeNotAlive<AttributeUnitOrBuildingReferences>(objects_);
	ar.serialize(interrupt_, "interrupt", "Прерывать производство");
}

bool ActionOrderUnits::automaticCondition() const
{
	if(!__super::automaticCondition()) 
		return false;

	UnitActing* factory = aiPlayer().findFreeFactory(attrUnit);
	if(factory && factory->isConstructed() && !factory->isUpgrading()){
		if(checkProduceParameters(attrUnit)){
			difficultyTimer.start(aiPlayer().difficulty().get()->orderUnitsDelay*1000);
			return true;
		}
		return false;
	}
	return false;
}

bool ActionOrderParameters::automaticCondition() const
{
	start_timer_auto();
	if(!__super::automaticCondition()) 
		return false;
	UnitActing* factory = findFreeFactory(attrFactory);
	if(factory && factory->isConstructed() && !factory->isUpgrading()){ 
		if(!factory->canProduceParameter(numberOfParameter))
			return false;
		const ProducedParameters& prm = 
			factory->attr().producedParameters[numberOfParameter];
		if(factory->accessibleParameter(prm)){
			difficultyTimer.start(aiPlayer().difficulty().get()->orderParametersDelay*1000);
			return true;
		}
		return false;
	}
	return false;
}

ConvexHull::ConvexHull(const Polygon& points)
{
	downRightPoint_ = Vect2f::ZERO;
	polygon_ = points;
	Polygon::iterator it;
	Polygon::iterator pointToErase;
	FOR_EACH(polygon_, it)
		if((*it).y > downRightPoint_.y || // нижняя
		   (floatEqual((*it).y, downRightPoint_.y) && (*it).x > downRightPoint_.x) ){ // нижняя-правая
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
	return angleBeetwenVectors(Vect2f(1.f, 0.f), vect1 - downRightPoint_) < angleBeetwenVectors(Vect2f(1.f, 0.f), vect2 - downRightPoint_) ||
		(floatEqual(angleBeetwenVectors(Vect2f(1.f, 0.f), vect1 - downRightPoint_), angleBeetwenVectors(Vect2f(1.f, 0.f), vect2 - downRightPoint_)) && vect1.distance2(downRightPoint_) < vect2.distance2(downRightPoint_));
}

Vect2f ActionUpgradeUnit::normal(const Vect2f& p0, const Vect2f& p1)
{
	return Vect2f(p1.y - p0.y, p0.x - p1.x);
}

Vect2f ActionUpgradeUnit::getPointInSection(const Vect2f& p0, const Vect2f& p1, float alfa)
{
	return Vect2f(p0.x + (p1.x - p0.x) * alfa, p0.y + (p1.y - p0.y) * alfa);
}

Vect2f ActionUpgradeUnit::getPointOnDistance(const Vect2f& guide, const Vect2f& point, float distance)
{
	Vect2f guide_ = guide;
	guide_.Normalize(1.f);
	return Vect2f(point.x - distance * guide_.x, point.y - distance * guide_.y);
}

int ConvexHull::isLeft(const Vect2f& p0, const Vect2f& p1, const Vect2f& p2) 
{
	return SIGN((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
}

void ActionUpgradeUnit::clear()
{
	__super::clear();
	if(unit_)
		unit_->setUsedByTrigger(false);
	deleteScanOp();
}

void ActionUpgradeUnit::positionFound(UnitReal* unit)
{	
	if(unit_->getUnitState() == UnitReal::MOVE_MODE && !unit->wayPoints().empty()) {
		scanOp->checkPosition(Vect2f(unit->wayPoints().back().x, unit->wayPoints().back().y));
		Vect2f point;
		bool flagPoint = false;
		while (!scanOp->valid()) {
			if(fabs(angle - 2*M_PI) < FLT_COMPARE_TOLERANCE) {
				angle = 0;
				factorRadius += 1;
			}
			flagPoint = true;
			point = Vect2f(unit->position2D().x + factorRadius*(unit->radius() + extraRadius)*cos(angle), 
						   unit->position2D().y + factorRadius*(unit->radius() + extraRadius)*sin(angle));
			point = Vect2f(
				clamp(point.x, 0, vMap.H_SIZE),
				clamp(point.y, 0, vMap.V_SIZE));
			if(!pathFinder->checkWater(round(point.x), round(point.y), 0))
				scanOp->checkPosition(point);
			angle += M_PI/12;
			if(factorRadius > 20) 
				break;
		}
		if(flagPoint){ 
			unit->setUsedByTrigger(true);
			unit->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(point.x, point.y, unit->position().z), 0));
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
	return !unit->isUsedByTrigger() && !unit->isUpgrading() && 
		   unit->getUnitState() != UnitReal::MOVE_MODE && 
		   !unit->fireTargetExist() ;
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
		while(aiPlayer().fogOfWarMap()->getFogState((int) point.x, (int) point.y) != FOGST_FULL){
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
			contextSquad_->setShowAISign(true);
		}
	}
}

bool  ActionUpgradeUnit::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	// юнит не рядом со списком зданий => не подходит
	if(upgradeOption == UPGRADE_NEAR_OBJECT)
	{
		CountObjectsScanOp op = CountObjectsScanOp(&aiPlayer(), objects_, extraRadius);
		op.checkInRadius(unit->position2D());
		if(!op.foundNumber())
			return false;
	}

	// считаем что если юнит используется триггером и это производство, 
	// значит процесс производства помечен использован триггером
	return !unit->isDocked() && !unit->isUpgrading() && 
		(!unit->isUsedByTrigger() || (interrupt_ && safe_cast<UnitActing*>(unit)->isProducing()) || (!safe_cast<UnitActing*>(unit)->isProducing() && upgradeOption == UPGRADE_HERE)) && 
		   unit->isConstructed();
}

bool ActionUpgradeUnit::automaticCondition() const
{
	if(!__super::automaticCondition()){
		return false;
	}
	if(contextUnit_) {
		unit_ = contextUnit_;
		if(unit_->attr().upgrades.exists(upgradeNumber)){
			const AttributeBase* attribute = unit_->attr().upgrades[upgradeNumber].upgrade;

			if(!aiPlayer().accessible(attribute))
				return false;
			if(unit_->attr().formationType != attribute->formationType && !aiPlayer().checkUnitNumber(attribute, object_))
				return false;

			//const ParameterSet& upgradeValue = attrUnit->upgrades[upgradeNumber].upgradeValue;
			if(unit_->canUpgrade(upgradeNumber) != CAN_START)
				return false;
		}
		else
			return false;

		if(unit_->attr().isBuilding() && upgradeOption != UPGRADE_NEAR_OBJECT) 
			upgradeOption = UPGRADE_HERE;
		if(unit_) {
				if(unit_->attr().upgrades.exists(upgradeNumber) && unit_->attr().upgrades[upgradeNumber].upgrade.get()->isBuilding())
					difficultyTimer.start(aiPlayer().difficulty().get()->upgradeUnitDelay*1000);
				unit_->setUsedByTrigger(true);
				if(!difficultyTimer)
					timeToUpgrade.start(maxTimeToUpgrade);
				else
					timeToUpgrade.start(maxTimeToUpgrade + aiPlayer().difficulty().get()->upgradeUnitDelay);	
				unit_->stop();
				firstTime = true;
				return true;
		}
	}
	return false;
}

void ActionUpgradeUnit::deleteScanOp()
{
	if(scanOp) {
		delete scanOp;
		scanOp = 0;
	}
}

bool ActionUpgradeUnit::workedOut()
{
	//start_timer_auto();
	// вычищаем дохликов
	if(!unit_ || (unit_ && !unit_->alive())){
		unit_ = 0;
		deleteScanOp();
		return true;
	}
	if(!timeToUpgrade){ 
		//прошло максимальное время для апгрейда, триггер считается отработанным, юнит освобождается
		if(unit_){
			unit_->setUsedByTrigger(false);
			unit_->setShowAISign(false);
		}
		deleteScanOp();
		return true;
	}

	if(!difficultyTimer){	
		if(unit_) {
			if(!scanOp)
				if(upgradeOption == UPGRADE_ON_THE_DISTANCE_FROM_ENEMY)
					scanOp = new RadiusScanOp(unit_, 0, aiPlayer(), 2 * unit_->radius());
				else
					scanOp = new RadiusScanOp(unit_, 0, aiPlayer(), extraRadius);
			if(unit_->alive() && !unit_->isUpgrading()) {
				switch(upgradeOption) {
					case UPGRADE_NEAR_OBJECT:
					case UPGRADE_HERE: {
						unit_->setUsedByTrigger(false);
						unit_->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
						unit_->setShowAISign(true);
						unit_ =0;
						deleteScanOp();
						return true;
					}
					case UPGRADE_ON_THE_DISTANCE: {
						scanOp->checkPosition(unit_->position2D());
						if(scanOp->valid() && (unit_->rigidBody()->flyingMode() ? !unit_->rigidBody()->onDeepWater : !unit_->rigidBody()->onWater)) {
								unit_->setUsedByTrigger(false);
                   				unit_->executeCommand(UnitCommand(COMMAND_ID_POINT, unit_->position(), 0));
								unit_->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
								unit_->setShowAISign(true);
								deleteScanOp();
								unit_ = 0;
								return true;
						}
						else {
 							if(unit_->getUnitState() != UnitReal::MOVE_MODE || firstTime) {
								firstTime = false;
								Vect2f point = Vect2f(unit_->position2D().x + unit_->radius() + 3*extraRadius, unit_->position2D().y + unit_->radius() + 3*extraRadius); 
								point = Vect2f(
									clamp(point.x, 0, vMap.H_SIZE),
									clamp(point.y, 0, vMap.V_SIZE));
									unit_->setUsedByTrigger(true);
									unit_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(point.x, point.y, unit_->position().z), 0));
							}
							angle = 0; factorRadius = 3;
							positionFound(unit_);
						}
						break;
					}
					case UPGRADE_ON_THE_DISTANCE_FROM_ENEMY: {
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
											if(curDist < checkDist && scanOp->valid() /*&& !pathFinder->checkWater((int) aim.x, (int) aim.y, 0)*/){
												checkDist = curDist;
												aim_ = aim;
											}
											//show_line(To3D(aim), To3D(enemyCenter), (*pi)->unitColor());
										}
										//else
										//	show_line(To3D(aim), To3D(enemyCenter), aiPlayer().unitColor());
										searchAngle += M_PI / 12.f;
									}
								}
							//show_line(To3D(aim_), To3D(myCenter), aiPlayer().unitColor());
							if(!aim_.eq(Vect2f::ZERO, FLT_EPS)){
								unit_->setUsedByTrigger(true);
								unit_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim_.x, aim_.y, unit_->position().z), 0));
								return false;
							}
						}
						else
							if(unit_->getUnitState() != UnitReal::MOVE_MODE){
									scanOp->checkPosition(aim_);
									if(scanOp->valid() && aim_.eq(unit_->position2D(), unit_->attr().radius()) /*&& !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0)*/){
										unit_->setUsedByTrigger(false);
										unit_->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
										unit_->setShowAISign(true);
										deleteScanOp();
										unit_ = 0;
										return true;
									}
									else
										firstTime = true;
							}
						break;
					}

					case UPGRADE_ON_THE_DISTANCE_TO_ENEMY: {
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
									points.push_back(unit_->position2D());
									appendDistance = unit_->attr().radius();
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
								//ConvexHull::Polygon::const_iterator i;
								//FOR_EACH(polygon_, i)
								//	points3D.push_back(To3D(*i));
								//show_convex(points3D.size(), &points3D.front(), aiPlayer().unitColor());
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
											if (fabs(angleBeetwenVectors(normal_, *ec - aim)) > M_PI * 0.5f){
												goodGuide =true;
												break;
											}
										if(goodGuide){
											aim_ = getPointOnDistance(normal_, aim, unit_->attr().radius() * 2 + extraRadius + appendDistance);
											scanOp->checkPosition(aim_);
											aim_ = Vect2f(
												clamp(aim_.x, 0, vMap.H_SIZE),
												clamp(aim_.y, 0, vMap.V_SIZE));
											//show_line(To3D(aim), To3D(aim_), aiPlayer().unitColor());
											if(scanOp->valid() && !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0)){
												unit_->setUsedByTrigger(true);
												unit_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(aim_.x, aim_.y, unit_->position().z), 0));
												return false;
											}
										}
									}
								}
								unit_->setUsedByTrigger(false);
								unit_->setShowAISign(false);
								deleteScanOp();
								return true; // если не нашли
						}
						else
							if(unit_->getUnitState() != UnitReal::MOVE_MODE){
									scanOp->checkPosition(aim_);
									if(scanOp->valid() && aim_.eq(unit_->position2D(), unit_->attr().radius()) && !pathFinder->checkWater((int) aim_.x, (int) aim_.y, 0)){
										unit_->setUsedByTrigger(false);
										unit_->executeCommand(UnitCommand(COMMAND_ID_UPGRADE, upgradeNumber));
										unit_->setShowAISign(true);
										deleteScanOp();
										unit_ = 0;
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
				unit_ = 0;
			return false;
		}
		if(!unit_ || !unit_->isUsedByTrigger()){
			deleteScanOp();
			return true;
		}
	}
	return false;
}

ActionUpgradeUnit::~ActionUpgradeUnit()
{
}

ActionForAI::ActionForAI() 
{
	onlyIfAi = true; 
}

bool ActionForAI::automaticCondition() const
{
	return __super::automaticCondition() && (!onlyIfAi || aiPlayer().isAI());
}

void ActionForAI::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(onlyIfAi, "onlyIfAi", "Запускать только для АИ");
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

	if(contextUnit_->getUnitState() == UnitReal::MOVE_MODE && 
		!contextUnit_->wayPoints().empty() && 
		!pathFinder->checkWater(round(contextUnit_->wayPoints().back().x), round(contextUnit_->wayPoints().back().y), 0))
		return false;

	return true;
}

void ActionEscapeWater::activate()
{
	Vect2i pos = Vect2i::ZERO;
	if(pathFinder->findNearestPoint(aiPlayer().centerPosition() , nearBase_, PathFinder::GROUND_FLAG, pos)){
		contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, Vect3f(pos, 0), 0));
		contextSquad_->setShowAISign(true);
	}
}

void ActionSetCamera::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(cameraSplineName, "cameraSplineName", "Имя сплайна камеры");
	ar.serialize(cycles, "cycles", "Количество циклов");
	ar.serialize(smoothTransition, "smoothTransition", "Плавный переход");
}	

void ActionSquadMove::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(attrSquad, "attrSquad", "Тип сквада");
	if(ar.isOutput() && ar.isEdit() && universe())
		label.setComboList(editLabelDialog().c_str());
	ar.serialize(label, "label", "Метка объекта");
	ar.serialize(mode, "mode", "Критерий завершения");
}	

void ActionSquadMoveToAnchor::serialize(Archive& ar) 
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit() && universe())
		label.setComboList(editAnchorLabelDialog().c_str());
	ar.serialize(label, "label", "Метка якоря");
	ar.serialize(mode, "mode", "Критерий завершения");
}	

void ActionPutSquadToAnchor::serialize(Archive& ar) 
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit() && universe())
		label.setComboList(editAnchorLabelDialog().c_str());
	ar.serialize(label, "label", "Метка якоря");
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
	ar.serialize(delay_, "delay", "Задержка, секунды");
	ar.serialize(pause_, "pause", "Пауза между сообщениями, секунды");

	ar.serialize(type_, "type", "Действие");
	ar.serialize(fadeTime_, "fadeTime", "Фейд, секунды(при удалении)");

	ar.serialize(delayTimer_, "delayTimer_", 0);
	ar.serialize(pauseTimer_, "pauseTimer_", 0);
	ar.serialize(durationTimer_, "durationTimer_", 0);
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
}

void ActionSelectUnit::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(unitID, "unitID", "Идентификатор юнита");
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
	return triggerChain()->ignorePause() ? !nonStopTimer_ : !timer_;
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

void ActionSetHotKey::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(controlReference_, "controlReference", "Кнопка");
	ar.serialize(hotKey_, "hotKey", "Горячая клавиша");
}

//------------------------------------------------------

void ActionSetHotKey::activate() 
{
	if(UI_ControlBase* cp = controlReference_.control())
		cp->setHotKey(hotKey_);
}


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
	AtomActions::const_iterator it;
	FOR_EACH(actions_, it)
		if(!it->workedOut())
			return false;
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

void ActionInterfaceUIAnimationControl::activate()
{
	if(UI_ControlBase* cp = controlReference_.control())
		cp->startAnimation(action_);
}

void ActionInterfaceUIAnimationControl::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(controlReference_, "controlReference", "Кнопка");
	ar.serialize(action_, "action", "Команда");
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
		universe()->setCountDownTime(timer() ? timer() : -1);
		aiPlayer().checkEvent(EventTime(timer()));
	}
	return !useNonStopTimer ? !timer : !nonStopTimer; 
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

ActionOrderUnits::ActionOrderUnits()
{
	countUnits = 1;
	inQuery = countUnits;
	maxProduce = 0;
	unitNumber = 0;
}

ActionOrderParameters::ActionOrderParameters()
{
	countParameters = 1;
	inQuery = countParameters;
	maxProduce = 0;
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
				if(!factory->isProducing() && !factory->isUpgrading() && factory->isConstructed())
					return factory;
			}
		}
	}
	return 0;
}

void ActionOrderParameters::clear()
{
	__super::clear();
	if(freeFactories.empty())
		return;
	FreeFactories::iterator fi;
	FOR_EACH(freeFactories, fi)
		if((*fi) && (*fi)->alive())
			(*fi)->setUsedByTrigger(false);
}

void ActionOrderParameters::activate()
{
	inQuery = countParameters;
	freeFactories.clear();
	if(!difficultyTimer){
		UnitActing* factory = findFreeFactory(attrFactory);
		if(factory && factory->isConstructed() && !factory->isUpgrading()) {
			// Ищем свободные фабрики
			while (factory && factory->isConstructed() && inQuery && !factory->isUpgrading()) {
				const ProducedParameters& prm = 
					factory->attr().producedParameters[numberOfParameter];
				if(factory->accessibleParameter(prm)){
					freeFactories.push_back(factory);
					factory->setUsedByTrigger(true);
					factory->executeCommand(UnitCommand(COMMAND_ID_PRODUCE_PARAMETER, numberOfParameter));
					inQuery--;
				}
				else break;
				if(!inQuery)
					return; 
				factory = findFreeFactory(attrFactory);
			}
			if(inQuery) {
				FreeFactories::iterator fi;
				fi = freeFactories.begin();
				// странно, но насколько понял одна очередь на параметры и юниты => и одна max длина
				maxProduce = (*fi)->attr().producedUnitQueueSize;
				// Раскидываем юнитов по фабрикам
				for (int j = 1; j <= maxProduce-1; j++) {
					for(fi = freeFactories.begin(); fi != freeFactories.end(); ++fi) 
						if(inQuery) {
							const ProducedParameters& prm = 
								(*fi)->attr().producedParameters[numberOfParameter];
							if((*fi)->accessibleParameter(prm)){
								(*fi)->setUsedByTrigger(true);
								(*fi)->executeCommand(UnitCommand(COMMAND_ID_PRODUCE_PARAMETER, numberOfParameter));
								inQuery--;
							}
							else 
								break;
						}
						else
							return;
				}
			}
		}
	}
}

bool ActionOrderParameters::workedOut()
{	
	//start_timer_auto();
	if(!difficultyTimer){
		// Ищем свободные фабрики
		UnitActing* factory = findFreeFactory(attrFactory);
		while (factory && factory->isConstructed() && inQuery && !factory->isUpgrading()) {
			const ProducedParameters& prm = 
				factory->attr().producedParameters[numberOfParameter];
			maxProduce = factory->attr().producedUnitQueueSize;
			if(factory->accessibleParameter(prm)){
				freeFactories.push_back(factory);
				factory->setUsedByTrigger(true);
				factory->executeCommand(UnitCommand(COMMAND_ID_PRODUCE_PARAMETER, numberOfParameter));
				inQuery--;
			}
			else 
				break;
			if(!inQuery)
				break; 
			factory = findFreeFactory(attrFactory);
		}
		if(!freeFactories.empty()) {
			if(inQuery) {
				FreeFactories::iterator fi;
				while (inQuery) {
					bool allBusy = true;
					fi = freeFactories.begin();
					while(fi != freeFactories.end()) {
						// удаляем убитые фабрики
						if(!(*fi)->alive()) {
							inQuery +=(*fi)->producedQueue().size();
							fi = freeFactories.erase(fi);
							if(fi == freeFactories.end()) 
								break;
						}
						else{
							if(inQuery){
								if((*fi)->producedQueue().size()< maxProduce) {
									const ProducedParameters& prm = 
										(*fi)->attr().producedParameters[numberOfParameter];
									if((*fi)->accessibleParameter(prm)){
										(*fi)->setUsedByTrigger(true);
										(*fi)->executeCommand(UnitCommand(COMMAND_ID_PRODUCE_PARAMETER, numberOfParameter));
										inQuery--;
									}
									else
										break;
									allBusy = false;
								}
							}
							else
								break;
							++fi;
						}
					}
					if(allBusy) break;
				}
				return false;
			}
			else {
				FreeFactories::iterator fi;
				fi = freeFactories.begin();
				while(fi != freeFactories.end()) {
					// удаляем убитые фабрики
					if(!(*fi)->alive()) {
						inQuery +=(*fi)->producedQueue().size();
						fi = freeFactories.erase(fi);
						if(fi == freeFactories.end()) 
							break;
					}
					else
						++fi;
				}
				for(fi = freeFactories.begin(); fi != freeFactories.end(); ++fi)
					if((*fi)->isProducing()) 
						return false;
			}
		}	
		if(!inQuery){ 
			FreeFactories::iterator fi;
			for(fi = freeFactories.begin(); fi != freeFactories.end(); ++fi){
				(*fi)->setUsedByTrigger(false);
				(*fi)->setShowAISign(false);
			}
			return true;
		}	
		return false;
	}
	return false;
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
	safe_cast<UnitActing*>(&*contextUnit_)->putOutTransport();
	//contextUnit_->setShowAISign(true);
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
					voiceManager.setEnabled(false);
			else
				if(switchMode_ == MODE_ON)
					voiceManager.setEnabled(true);
				else
					voiceManager.setEnabled(GameOptions::instance().getBool(OPTION_VOICE_ENABLE));
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

bool ActionOrderUnits::checkProduceParameters(const AttributeBase* producedUnit) const
{
	if(!aiPlayer().checkUnitNumber(producedUnit))
		return false;
	if(!aiPlayer().accessible(producedUnit))
		return false;
	if(!aiPlayer().requestResource(producedUnit->installValue, NEED_RESOURCE_TO_PRODUCE_UNIT))
		return false;
	return true;
}
void ActionOrderUnits::clear()
{
	__super::clear();
	if(freeFactories.empty())
		return;
	FreeFactories::iterator fi;
	FOR_EACH(freeFactories, fi)
		if((*fi) && (*fi)->alive())
			(*fi)->setUsedByTrigger(false);
}

void ActionOrderUnits::activate()
{
	inQuery = countUnits;
	freeFactories.clear();
	if(!difficultyTimer){
		UnitActing* factory = aiPlayer().findFreeFactory(attrUnit);
		if(factory && factory->isConstructed() && !factory->isUpgrading()) {
			AttributeBase::ProducedUnitsVector::const_iterator i;
			FOR_EACH(factory->attr().producedUnits, i)
				if(attrUnit == i->second.unit) {
					// Ищем свободные фабрики
					while (factory && factory->isConstructed() && !factory->isUpgrading() && inQuery) {
						unitNumber = i->first;
						if(checkProduceParameters(attrUnit)){ 
							freeFactories.push_back(factory);
							factory->setUsedByTrigger(true);
							factory->executeCommand(UnitCommand(COMMAND_ID_PRODUCE, i->first));
							inQuery--;
						}
						else 
							break;
						if(!inQuery)
							return; 
						factory = aiPlayer().findFreeFactory(attrUnit);
					}
					if(inQuery) {
						FreeFactories::iterator fi;
						fi = freeFactories.begin();
						maxProduce = (*fi)->attr().producedUnitQueueSize;
						// Раскидываем юнитов по фабрикам
						for (int j = 1; j <= maxProduce-1; j++) {
							for(fi = freeFactories.begin(); fi != freeFactories.end(); ++fi) 
								if(inQuery) {
									if(checkProduceParameters(attrUnit)){ 
										(*fi)->setUsedByTrigger(true);
										(*fi)->executeCommand(UnitCommand(COMMAND_ID_PRODUCE, i->first));
										inQuery--;
									}
									else
										break;
								}
								else
									return;
						}
					}
					break;
				}
		}
		//xassert(!inQuery && "Слишком много юнитов надо произвести, фабрик не хватает");
	}
}

bool ActionOrderUnits::workedOut()
{	
	//start_timer_auto();
	if(!difficultyTimer){
		// Ищем свободные фабрики
		UnitActing* factory = aiPlayer().findFreeFactory(attrUnit);
		while (factory && factory->isConstructed() && inQuery && !factory->isUpgrading()) {
			// инициализация unitNumber и maxProduce если не было фабрик при вызове activate()
			AttributeBase::ProducedUnitsVector::const_iterator i;
			FOR_EACH(factory->attr().producedUnits, i)
				if(attrUnit == i->second.unit)
					unitNumber = i->first;
			maxProduce = factory->attr().producedUnitQueueSize;
			if(checkProduceParameters(attrUnit)){ 
				freeFactories.push_back(factory);
				factory->setUsedByTrigger(true);
				factory->executeCommand(UnitCommand(COMMAND_ID_PRODUCE, unitNumber));
				inQuery--;
			}
			else 
				break;
			if(!inQuery)
				break; 
			factory = aiPlayer().findFreeFactory(attrUnit);
		}
		if(!freeFactories.empty()) {
			if(inQuery) {
				FreeFactories::iterator fi;
				while (inQuery) {
					bool allBusy = true;
					fi = freeFactories.begin();
					while(fi != freeFactories.end()) {
						// удаляем убитые фабрики
						if(!(*fi)->alive()) {
							inQuery +=(*fi)->producedQueue().size();
							//(*fi)->setUsedByTrigger(false);
							fi = freeFactories.erase(fi);
							if(fi == freeFactories.end()) 
								break;
						}
						else{
							if(inQuery){
								if((*fi)->producedQueue().size()< maxProduce) {
									if(checkProduceParameters(attrUnit)){ 
										(*fi)->setUsedByTrigger(true);
										(*fi)->executeCommand(UnitCommand(COMMAND_ID_PRODUCE, unitNumber));
										inQuery--;
									}
									else
										break;
									allBusy = false;
								}
							}
							else
								break;
							++fi;
						}
					}
					if(allBusy) break;
				}
				return false;
			}
			else {
				FreeFactories::iterator fi;
				fi = freeFactories.begin();
				while(fi != freeFactories.end()) {
					// удаляем убитые фабрики
					if(!(*fi)->alive()) {
						inQuery +=(*fi)->producedQueue().size();
						//(*fi)->setUsedByTrigger(false);
						fi = freeFactories.erase(fi);
						if(fi == freeFactories.end()) 
							break;
					}
					else
						++fi;
				}
				for(fi = freeFactories.begin(); fi != freeFactories.end(); ++fi)
					if((*fi)->isProducing()) 
						return false;
			}
		}
		if(!inQuery){ 
			FreeFactories::iterator fi;
			for(fi = freeFactories.begin(); fi != freeFactories.end(); ++fi){
				(*fi)->setUsedByTrigger(false);
				(*fi)->setShowAISign(false);
			}
			return true;
		}	
		return false;
	}
	return false;
}

void ActionOrderParameters::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrFactory, "attrFactory", "Тип фабрики");
	ar.serialize(numberOfParameter, "numberOfParameter", "Номер параметра");
	ar.serialize(countParameters, "countParameters", "Количество");
}


void ActionOrderUnits::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attrUnit, "attrUnit", "Тип юнита");
	ar.serialize(countUnits, "countUnits", "Количество");
}

ActionFollowSquad::ActionFollowSquad()
{
	radius_ = 0;
}

void ActionFollowSquad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(squadToFollow_, "squadToFollow", "Сквад, за которым следовать");
	ar.serialize(radius_, "radius", "Расстояние");
}

bool ActionFollowSquad::automaticCondition() const
{
	if(!__super::automaticCondition()) 
		return false;
	squad_ = findSquad();

	return squad_;
}

UnitSquad* ActionFollowSquad::findSquad() const
{
	const SquadList& squadList = aiPlayer().squadList();
	SquadList::const_iterator si;
	float distMin = FLT_INF;
	UnitSquad* squad = 0;
	FOR_EACH(squadList, si)
		if(*si != contextSquad_ && &(*si)->attr() == squadToFollow_){
			float dist = (*si)->position2D().distance2(contextSquad_->position2D());
			if(distMin > dist){
				distMin = dist;
				squad = *si;
			}
		}
	return squad;
}

void ActionFollowSquad::activate()
{
	contextSquad_->setSquadToFollow(squad_);
	contextSquad_->setShowAISign(true);
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
	removeNotAlive<AttributeUnitReferences>(attrUnits_);
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
		safe_cast<UnitActing*>(&*contextUnit_)->setAttackMode(attackMode_);
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
	removeNotAlive<AttributeBuildingReferences>(headquaters_);
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

	switch(mode_){
		case MODE_RUN:
			if(unit->runMode())
				return false;
			break;
		case MODE_WALK:
			if(!unit->runMode())
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
	switch(mode_){
		case MODE_RUN:
			contextSquad_->executeCommand(UnitCommand(COMMAND_ID_GO_RUN, (class UnitInterface*)0, 0));
			break;
		case MODE_WALK:
			contextSquad_->executeCommand(UnitCommand(COMMAND_ID_STOP_RUN, (class UnitInterface*)0, 0));
	}
}

bool ActionReturnToBase::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	// считаю что действие имеет наивысший приоритет
	//if(unit->isUsedByTrigger())
	//	return false;

	headquater = findNearestHeadquater(unit->position2D());
	if(headquater){
		float dist = headquater->position2D().distance(unit->position2D());
		if(headquater && dist > radius_ + headquater->radius() && unit->getUnitState() != UnitReal::MOVE_MODE) 
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
	const LegionariesLinks& units = contextSquad_->units();
	LegionariesLinks::const_iterator it;
    FOR_EACH(units, it)
	switch(mode_){
		case MODE_RUN:
			(*it)->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_GO_RUN, (class UnitInterface*)0, 0));

			break;
		case MODE_WALK:
			(*it)->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_STOP_RUN, (class UnitInterface*)0, 0));
	}
	contextSquad_->setAttackMode(attackMode_);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_POINT, headquater->position(), 0));
	contextSquad_->startUsedByTrigger(returnTime_*1000, REASON_MOVE);
}

ActionSquadMove::ActionSquadMove() 
: label("")
{
	mode = DO_NOT_WAIT;
}

ActionPutSquadToAnchor::ActionPutSquadToAnchor() 
: label("")
{
}

ActionSquadMoveToAnchor::ActionSquadMoveToAnchor() 
: label("")
{
	mode = DO_NOT_WAIT;
}

bool ActionSquadMove::automaticCondition() const
{
	if(!__super::automaticCondition()) 
		return false;
	const SquadList& squadList_ = aiPlayer().squadList();
	if(!squadList_.empty()){
		UnitLink<UnitReal> unit_ = universe()->findUnitByLabel(label);
		if(unit_) {
			SquadList::const_iterator si;
			FOR_EACH(squadList_, si)
				if(&(*si)->attr() == attrSquad) 
					return true;
		}
	}
	return false;
}

void ActionSquadMoveToAnchor::clear()
{
	__super::clear();
	LegionariesLinks::iterator i;
	FOR_EACH(unitList, i)
		if((*i))
			(*i)->setUsedByTrigger(false);
}

bool ActionSquadMoveToAnchor::workedOut()
{
	if(unitList.empty())
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
					if((*i)->position2D().distance2(anchor->position2D()) > sqr((*i)->radius()) + sqr(anchor->radius()))
						return false;
				}
				else
					if((*i)->position2D().distance2(anchor->position2D()) < sqr((*i)->radius()) + sqr(anchor->radius())){
						flag = true;
						break;
					}
				++i;
			}
		}
	}

	if(flag){
		FOR_EACH(unitList, i)
			if((*i)){
				(*i)->setUsedByTrigger(false);
				(*i)->setShowAISign(false);
			}
		return true;
	}

	return false;
}

void ActionSquadMove::clear()
{
	__super::clear();
	LegionariesLinks::iterator i;
	FOR_EACH(unitList, i)
		if((*i))
			(*i)->setUsedByTrigger(false);
}

bool ActionSquadMove::workedOut()
{
	//start_timer_auto();
	if(!unit->alive()){
		unit = 0;
		LegionariesLinks::iterator i;
		FOR_EACH(unitList, i)
			if((*i)){
				(*i)->setUsedByTrigger(false);
				(*i)->setShowAISign(false);
			}
		return true;
	}

	if(unitList.empty())
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
					if((*i)->position2D().distance2(unit->position2D()) > sqr((*i)->radius()) + sqr(unit->radius()) + sqr(10.f))
						return false;
				}
				else
					if((*i)->position2D().distance2(unit->position2D()) < sqr((*i)->radius()) + sqr(unit->radius()) + sqr(10.f)){
						flag = true;
						break;
					}
				++i;
			}
		}
	}

	if(flag){
		FOR_EACH(unitList, i)
			if((*i)){
				(*i)->setUsedByTrigger(false);
				(*i)->setShowAISign(false);
			}
		return true;
	}

	return false;
}

void ActionSquadMoveToAnchor::activate()
{
	unitList.clear();
	const LegionariesLinks& units = contextSquad_->units();
	LegionariesLinks::const_iterator ui; 
	FOR_EACH(units, ui){ 			
		(*ui)->executeCommand(UnitCommand(COMMAND_ID_POINT, anchor->position(), 0));
		unitList.push_back(*ui);
		(*ui)->setUsedByTrigger(true);
	}
}

bool ActionPutSquadToAnchor::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	anchor = environment->findAnchor(label);
	if(!anchor)
	{
		xassert(0 && "Метка не найдена" && label);
		return false;
	}

	if(!unit->attr().isLegionary())
		return false;

	//UnitSquad* squad = safe_cast<UnitLegionary*>(unit)->squad();
	//if(squad->position().distance2(anchor->position()) < sqr(anchor->radius()))
	//	return false;

	return true;
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
			squadIn->addUnit(unit, false, false);
		}
		contextUnit_->setPose(anchor->pose(), true); 
		return;
	} 

	FOR_EACH(units, i)
	{
		Vect3f shift = (*i)->position() - squadCenter;
		shift = anchor->pose().xformVect(shift);
		(*i)->setPose(Se3f(anchor->pose().rot(), anchor->position() + shift), true); 
	}
}

bool ActionSquadMoveToAnchor::checkContextUnit(UnitActing* unit) const
{
	if(!__super::checkContextUnit(unit))
		return false;

	anchor = environment->findAnchor(label);
	if(!anchor)
	{
		xassert(0 && "Метка не найдена" && label);
		return false;
	}

	if(unit->position().distance2(anchor->position()) < sqr(unit->radius() + anchor->radius()))
		return false;

	return !unit->isUsedByTrigger() && !(unit->getUnitState() == UnitReal::MOVE_MODE && 
		   !unit->wayPoints().empty() && unit->wayPoints().back().eq(anchor->position(), anchor->radius() + unit->radius()));
}

void ActionSquadMove::activate()
{
	const SquadList& squadList = aiPlayer().squadList();
	if(!squadList.empty()){
		unit = universe()->findUnitByLabel(label);
		if(unit) {
			SquadList::const_iterator si;
			unitList.clear();
			FOR_EACH(squadList, si)
				if(&(*si)->attr() == attrSquad) { 
					const LegionariesLinks& units = (*si)->units();
					LegionariesLinks::const_iterator ui;
					FOR_EACH(units, ui) 			
						if(!(*ui)->constructingBuilding() && 
							!(*ui)->gatheringResource() &&
							!(*ui)->isUsedByTrigger() && !(*ui)->isUpgrading()){ 	
								(*ui)->executeCommand(UnitCommand(COMMAND_ID_POINT, unit->position(), 0));
								unitList.push_back(*ui);
								(*ui)->setUsedByTrigger(true);
							}
				}
		}
	}
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
		cameraManager->SetCameraFollow();
		cameraManager->erasePath();
		if(spline_ = cameraManager->findSpline(cameraSplineName.c_str())){
			cameraManager->loadPath(*spline_, smoothTransition);
			cameraManager->startReplayPath(spline_->stepDuration(), cycles);
		}
	}
}
 
bool ActionSetCamera::workedOut()
{
	if(aiPlayer().active() && cameraSplineName.empty())
		return true;
	if(!cameraManager->isPlayingBack()){
		cameraManager->erasePath();
		return true;
	}
	if(cameraManager->isPlayingBack() && spline_ && cameraManager->loadedSpline() != spline_)
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
	UI_LogicDispatcher::instance().saveGame(name_);
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
	UnitReal* unit = universe()->findUnitByLabel(label);
	if(!unit){
		xassert_s(0 && "Объект по метке не найден: ", label);
	}
	else{
		unit->hide(UnitReal::HIDE_BY_TRIGGER, !active_);
	}
}

void ActionActivateObjectByLabel::serialize(Archive& ar) 
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit() && universe())
		label.setComboList(editLabelDialog().c_str());
	ar.serialize(label, "label", "Имя объекта");
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

ActionMessage::~ActionMessage()
{
	if(type_ == MESSAGE_ADD && started_ && messageSetup_.isInterruptOnDestroy() && 
		messageSetup_.text() && messageSetup_.text()->hasVoice() && 
		voiceManager.validatePlayingFile(messageSetup_.text()->voice()))
		UI_Dispatcher::instance().removeMessage(messageSetup_, true);
}

ActionMessage::ActionMessage() 
{
	started_ = false;
	delay_ = 0.f;
	pause_ = 30.f;
	type_ = MESSAGE_ADD;
	mustInterrupt_ = false;
	fadeTime_ = 0.f;
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
	FOR_EACH(universe()->activeMessages().container(), it){
		ActionMessage* action = safe_cast<ActionMessage*>(*it);
		if(action->messageSetup().type() == messageType_)
			action->interrupt();
	}
}

void ActionInterruptAnimation::activate()
{
	ActiveMessages::iterator it;
	FOR_EACH(universe()->activeAnimation().container(), it){
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

void ActionMessage::activate()
{
	delayTimer_.start(round(delay_*1000.f));
	started_ = false;
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

bool ActionMessage::workedOut()
{ 
	start_timer_auto();

	if(mustInterrupt_){
		universe()->activeMessages().erase(this);
		UI_Dispatcher::instance().removeMessage(messageSetup_, true);
		mustInterrupt_ = false;
		return true;
	}

	if(!delayTimer_()){
		if(!started_){
			started_ = true;

			if(!messageSetup_.isEmpty()){
				float time = 0;
				switch(type_){
				case MESSAGE_ADD:{
					UI_Dispatcher::instance().playVoice(messageSetup_);
					UI_Dispatcher::instance().sendMessage(messageSetup_);
					universe()->activeMessages().add(this);
					time = messageSetup_.displayTime();
					}
					break;
				case MESSAGE_REMOVE:
					if(fadeTime_ && messageSetup_.hasVoice()){
						voiceManager.FadeVolume(fadeTime_, 0);
						time = fadeTime_;
					}
					else{
						UI_Dispatcher::instance().removeMessage(messageSetup_, false);
						time = 0.f;
					}
					break;
				}
				durationTimer_.start(round(time * 1000.f));
				pauseTimer_.start(round((time + pause_) * 1000.f));
			}
		}

		if(!durationTimer_() && !pauseTimer_()){
			if(type_ == MESSAGE_REMOVE && fadeTime_)
                UI_Dispatcher::instance().removeMessage(messageSetup_, false);
		    universe()->activeMessages().erase(this);
			return true;
		}
	}
	return false;
} 

bool ActionMessage::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	if(type_ == MESSAGE_ADD && universe()->disabledMessages().find(messageSetup_.type()) != universe()->disabledMessages().container().end())
		return false;

	if(messageSetup_.text() && messageSetup_.text()->hasVoice() && !messageSetup_.isCanInterruptVoice() && voiceManager.isPlaying())
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
	return !durationTimer_();
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
	const SquadList& squadList = aiPlayer().squadList();  
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
	return !timer;
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

	cameraManager->SetCameraFollow();
	cameraManager->erasePath();

	spline_ = 0;
	UnitReal* unit = contextUnit_;
	if(unit && (spline_ = cameraManager->findSpline(cameraSplineName.c_str()))){
		cameraManager->loadPath(*spline_, false);
		cameraManager->startReplayPath(spline_->stepDuration(), 1, unit);
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
		CameraCoordinate coord(position, cameraManager->coordinate().psi(), cameraManager->coordinate().theta(), cameraManager->coordinate().distance());
		if(transitionTime){
			CameraSpline spline;
			spline.push_back(coord);
			cameraManager->loadPath(spline, true);
			cameraManager->startReplayPath(transitionTime*1000, 1);
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

	if(messageSetup_ != UI_MessageSetup() && universe()->disabledMessages().find(messageSetup_.type()) != universe()->disabledMessages().container().end())
		return false;

	if(!messageSetup_.isCanInterruptVoice() && voiceManager.isPlaying())
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
		universe()->activeAnimation().erase(this);
		UI_Dispatcher::instance().removeMessage(messageSetup_, true);
		if(contextUnit_)
			contextUnit_->finishState(StateTrigger::instance());
		mustInterrupt_ = false;
		return true;
	}

	if(!timer){
		universe()->activeAnimation().erase(this);
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
	if(cameraManager->isPlayingBack() && spline_ && cameraManager->loadedSpline() != spline_)
		return true;

	if(!cameraManager->isPlayingBack()){
		cameraManager->erasePath();
		if(!turnStarted_ && turnTime){
			xassert(!setFollow);
			UnitReal* unit = contextUnit_;
			if(unit){
				CameraSpline spline;
				spline.push_back(
					CameraCoordinate(unit->position2D(), cycle(cameraManager->coordinate().psi(), 2*M_PI) + 2*M_PI, cameraManager->coordinate().theta(), cameraManager->coordinate().distance()));
				cameraManager->loadPath(spline, true);
				cameraManager->startReplayPath(turnTime*1000, 1);
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
	ar.serialize(ResourceSelector(bkgName, ResourceSelector::Options("*.tga", "Resource\\", "Will select location of an file textures")), "bgFileName", "Имя логотипа");
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

	XStream ff(0);
	if(ff.open(name.c_str())){
		ff.close();
		return true;
	}

	return false;
}

bool ActionAttackLabel::checkContextUnit(UnitActing* unit) const
{
	return __super::checkContextUnit(unit) && !unit->isUsedByTrigger();
}

ActionAttackLabel::ActionAttackLabel() : label_("")
{
	timeToAttack_ = 30;
}

void ActionAttackLabel::clear()
{
	__super::clear();
	if(contextUnit_)
		contextUnit_->setUsedByTrigger(false);
}

void ActionAttackLabel::activate()
{
	const Anchor* anchor = environment->findAnchor(label_);
	firstTime = true;
	if(anchor)
	{
		position_ = anchor->position(); 
		int weaponID = weapon_.get()->ID();
		if(contextUnit_->canFire(weaponID)){ 
			if(contextUnit_->getSquadPoint())
				contextUnit_->getSquadPoint()->executeCommand(UnitCommand(COMMAND_ID_ATTACK, position_, weaponID));
			else
				contextUnit_->executeCommand(UnitCommand(COMMAND_ID_ATTACK, position_, weaponID));
			contextUnit_->setUsedByTrigger(true);
		}
	}
	else
		position_ = Vect3f::ZERO;
}


bool ActionAttackLabel::workedOut()
{
	if(position_ == Vect3f::ZERO || !contextUnit_)
		return true;

	if(contextUnit_->fireDistanceCheck(WeaponTarget(position_, weapon_.get()->ID())))
	{
		if(firstTime)
		{
			firstTime = false;
			durationTimer_.start(timeToAttack_ * 1000);
			aiPlayer().executeCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT, position_, 1));
			return false;
		}
		if(durationTimer_())
		{
			aiPlayer().executeCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT_MOUSE,  position_));
			return false;
		}
		else
		{
			aiPlayer().executeCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT, position_, 0));
			contextUnit_->setUsedByTrigger(false);
			return true;
		}
	}
	else{
		if(contextUnit_->getUnitState() != UnitReal::ATTACK_MODE ){
			contextUnit_->setUsedByTrigger(false);
			contextUnit_->setShowAISign(false);
			return true;
		}
		return false;
	}
			
}

void ActionAttackLabel::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(weapon_, "weapon", "Оружие");
	if(ar.isOutput() && ar.isEdit() && universe())
		label_.setComboList(editAnchorLabelDialog().c_str());
	ar.serialize(label_, "label", "Метка(на якоре)");
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
}

void ActionStartMission::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(ResourceSelector(missionName, ResourceSelector::MISSION_OPTIONS), "missionName", "Имя миссии");
	ar.serialize(gameType_, "gameType", "Режим загрузки");
	ar.serialize(paused_, "paused", "На паузе");
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

void ActionChangeCommonCursor::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(cursorType, "cursorType", "Тип курсора");
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
		contextUnit_->setShowAISign(true);
}

ActionGuardUnit::ActionGuardUnit()
{
	attackTime_ = 180;
	maxDistance_ = 300.f;
}

ActionAttack::ActionAttack()
{
	attackTime_ = 180;
	attackEnemy_ = true;
	aimObject_ = AIM_ANY;
}

ActionAttackMyUnit::ActionAttackMyUnit()
{
	attackTime_ = 180;
	maxDistance_ = 300.f; 
	attackEnemy_ = false;
}

void ActionSellBuilding::activate()
{
	contextUnit_->executeCommand(UnitCommand(COMMAND_ID_UNINSTALL,0));
}

bool ActionAttack::checkContextUnit(UnitActing* unit) const
{
	start_timer_auto();

	if(!__super::checkContextUnit(unit))
		return false;

	if(unit->isUpgrading() || !safe_cast<UnitActing*>(unit)->isDefaultWeaponExist())
		return false;

	UnitSquad* squad = unit->getSquadPoint();

	const LegionariesLinks& units = squad->units();
	LegionariesLinks::const_iterator i;
	FOR_EACH(units, i)
		if(((*i)->targetUnit() && unit->isEnemy((*i)->targetUnit())) || (*i)->isUsedByTrigger())
			return false;

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
			if(aiPlayer().isEnemy(*pi) || (*pi)->isWorld()){
				const RealUnits& realUnits = (*pi)->realUnits(*ai);
				RealUnits::const_iterator ui;
				FOR_EACH(realUnits, ui)
					if((*ui)->alive() && (*ui)->attr().isActing() && !(*ui)->attr().excludeFromAutoAttack && (aimObject_ == AIM_ANY || (aimObject_ == AIM_UNIT  && (*ui)->attr().isLegionary()) || (aimObject_ == AIM_BUILDING  && (*ui)->attr().isBuilding())) && 
						(!((*ui)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*ui)->attr()).teleport) && squad->canAttackTarget(WeaponTarget(*ui)) && 
						(((*ui)->attr().isLegionary() && !(*ui)->isUnseen()) || (*ui)->attr().isBuilding()))){
							float distance = unit->position2D().distance((*ui)->position2D());
							if(bestDistance + Check_Radius > distance){
								if(distance < bestDistance){ 
									if(bestDistance - distance > Check_Radius)
										target_ = *ui;
									bestDistance = distance;
								}
								if(!target_)
									target_ = *ui;
								priority = microAI_TargetPriority(unit, (*ui));
								if(bestPriority < priority){
									bestPriority = priority;
									target_ = *ui;
								}
							}
					}
			}
	}
	if(target_)
		return target_;

	// конкретный объект для атаки или все юниты
	FOR_EACH(universe()->Players, pi)
		if(aiPlayer().isEnemy(*pi) || (*pi)->isWorld()){
			const RealUnits& realUnits = attrTarget_ ? (*pi)->realUnits(attrTarget_) : (*pi)->realUnits();
			RealUnits::const_iterator ui;
			FOR_EACH(realUnits, ui)
				if((*ui)->alive() && (*ui)->attr().isActing() && !(*ui)->attr().excludeFromAutoAttack && (aimObject_ == AIM_ANY || (aimObject_ == AIM_UNIT  && (*ui)->attr().isLegionary()) || (aimObject_ == AIM_BUILDING  && (*ui)->attr().isBuilding())) &&
					(!((*ui)->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>((*ui)->attr()).teleport) && squad->canAttackTarget(WeaponTarget(*ui)) && 
					(((*ui)->attr().isLegionary() && !(*ui)->isUnseen()) || (*ui)->attr().isBuilding()))){
						float distance = unit->position2D().distance((*ui)->position2D());
						if(bestDistance + Check_Radius > distance){
							if(distance < bestDistance){ 
								if(bestDistance - distance > Check_Radius)
									target_ = *ui;
								bestDistance = distance;
							}
							if(!target_)
								target_ = *ui;
							priority = microAI_TargetPriority(unit, (*ui));
							if(bestPriority < priority){
								bestPriority = priority;
								target_ = *ui;
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

	UnitSquad* squad = unit->getSquadPoint();
	if(unit->targetUnit() || unit->isUsedByTrigger() || unit->isUpgrading() || !safe_cast<UnitActing*>(unit)->isDefaultWeaponExist() || squad->locked())
		return false;

	target_ = 0;
	float distMin = floatEqual(maxDistance_, 0.f) ? FLT_INF : sqr(maxDistance_);
	
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

	if (unit->isUsedByTrigger() || unit->isUpgrading() || !safe_cast<UnitActing*>(unit)->isDefaultWeaponExist())
		return false;

	float maxDist = floatEqual(maxDistance_, 0.f) ? FLT_INF : sqr(maxDistance_);

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

	return target_ && safe_cast<UnitActing*>(unit)->targetUnit() != target_;
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
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, target_, 0));
	contextSquad_->startUsedByTrigger(attackTime_*1000, REASON_ATTACK);
}

void ActionAttackMyUnit::activate()
{
	contextSquad_->setAttackMode(attackMode_);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, target_, 0));
	contextSquad_->startUsedByTrigger(attackTime_*1000, REASON_ATTACK);
}

void ActionGuardUnit::activate()
{
	contextSquad_->setAttackMode(attackMode_);
	contextSquad_->executeCommand(UnitCommand(COMMAND_ID_OBJECT, target_, 0));
	contextSquad_->startUsedByTrigger(attackTime_*1000, REASON_ATTACK);

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

void ActionGuardUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(maxDistance_, "maxDistance", "Максимальное растояние до охраняемого юнита");
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(attrTargets_, "attrTargets_", "&Типы юнитов для охраны");
	removeNotAlive<AttributeUnitOrBuildingReferences>(attrTargets_);
	ar.serialize(attackTime_, "attackTime", "Время атаки, секунды");
}

void ActionAttack::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(attrPriorityTarget_, "attrPriorityTarget", "Приоритетные цели");
	removeNotAlive<AttributeUnitOrBuildingReferences>(attrPriorityTarget_);
	ar.serialize(attrTarget_, "attrTarget", "Тип цели");
	ar.serialize(attackTime_, "attackTime", "Время атаки, секунды");
	ar.serialize(aimObject_, "aimObject", "Цель");

	ar.serialize(attackEnemy_, "attackEnemy_", 0);// FOR FUTURE CONVERSION 24.05.06
}

void ActionAttackMyUnit::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(attackMode_, "attackMode", "Режим атаки");
	ar.serialize(attrTargets_, "attrTargets_", "&Типы цели");
	removeNotAlive<AttributeUnitOrBuildingReferences>(attrTargets_);
	ar.serialize(attackTime_, "attackTime", "Время атаки, секунды");
	ar.serialize(maxDistance_, "maxDistance", "Максимальное расстояние до цели");

	ar.serialize(attackEnemy_, "attackEnemy_", 0);// FOR FUTURE CONVERSION 24.05.06
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

void ActionContext::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(object_, "object", "Тип юнита");
}

bool ActionContextSquad::checkContextUnit(UnitActing* unit) const 
{ 
	if(!ActionContext::checkContextUnit(unit))
		return false;
	if(!unit->attr().isLegionary())
		return false;
	return !squad_ || &safe_cast<UnitLegionary*>(unit)->squad()->attr() == squad_;
}

bool ActionFollowSquad::checkContextUnit(UnitActing* unit) const
{
	return __super::checkContextUnit(unit) && !safe_cast<UnitLegionary*>(unit)->squad()->getSquadToFollow();
}

bool ActionContext::checkContextUnit(UnitActing* unit) const
{ 
	return !object_ || object_ == &unit->attr(); 
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
	safe_cast<UnitActing*>(&*contextUnit_)->setIgnoreFreezedByTrigger(switchMode_);
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

ActionSetCutScene::ActionSetCutScene()
{
	switchMode = ON;
}

void ActionSetCutScene::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchMode, "switchMode", "Режим");
}


