#include "StdAfx.h"
#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "vmap.h"
#include "Universe.h"
#include "Squad.h"
#include "Serialization\Serialization.h"
#include "kdw/kdWidgetsLib.h"
#include "ShowHead.h"
#include "Serialization\ResourceSelector.h"
#include "GameShell.h"
#include "Actions.h"
#include "Conditions.h"
#include "UI_Logic.h"
#include "Network\P2P_interface.h"
#include "SelectManager.h"
#include "GameOptions.h"
#include "Environment\Environment.h"
#include "UserInterface\GameShell.h"
#include "UserInterface\UI_NetCenter.h"
#include "Network\UniverseX.h"
#include "FileUtils\FileUtils.h"
#include "Units\CommandsQueue.h"
#include "Render\src\VisGeneric.h"
#include "StreamCommand.h"
#include "EditorVisual.h"

EditorVisual::Interface& editorVisual()
{
	xassert(0 && "EditorVisual можно использовать только в редакторе!");
	return *reinterpret_cast<EditorVisual::Interface*>(0);
}

void ActionSetDirectControl::activate()
{
	if(aiPlayer().active() && unit){
		if(controlMode_ != DIRECT_CONTROL_DISABLED){
			if(UnitInterface* selectedUnit = UI_LogicDispatcher::instance().selectedUnit())
				safe_cast<UnitActing*>(selectedUnit->getUnitReal())->setDirectControl(DIRECT_CONTROL_DISABLED);
			UnitActing* unitAct = safe_cast<UnitActing*>(unit);
			unitAct->setDirectControl(controlMode_);
			universe()->select(unit, false);
			unitAct->setActiveDirectControl(controlMode_);
		}else
			safe_cast<UnitActing*>(unit)->setDirectControl(controlMode_);
	}
}

void ActionShowReel::activate()
{
	if(gameShell->isVideoEnabled()){
		gameShell->reelAbortEnabled = skip;
		gameShell->showReelModal(localizedVideo ? extractFileName(binkFileName.c_str()).c_str() : binkFileName.c_str(), 
			localizedVoice ? extractFileName(soundFileName.c_str()).c_str() : soundFileName.c_str(),
			localizedVideo, localizedVoice, stopBGMusic, alpha);
		timer.start(0);
	}
}

bool ActionShowReel::workedOut()
{
	if(gameShell->isVideoEnabled())
		return !timer.busy();
	else
		return true;
}

void ActionShowLogoReel::activate()
{
	if(gameShell->isVideoEnabled()){
		LogoAttributes logoAttributes;
		logoAttributes.fishName = fishName;
		logoAttributes.groundName = groundName;
		logoAttributes.logoName = logoName;
		logoAttributes.fishAlpha = fishAlpha;
		logoAttributes.logoAlpha = logoAlpha;
		logoAttributes.fadeTime = fadeTime;
		gameShell->showLogoModal(logoAttributes, blobsSetting, localized, workTime,soundAttributes);
		timer.start(workTime);
	}
}

void ActionLoadGameAuto::activate()
{
	gameShell->startMissionSuspended(MissionDescription(name.c_str(), gameType_));
}

void ActionStartMission::activate()
{
	MissionDescription mission(missionName.c_str(), gameType_);
	mission.setGamePaused(paused_ || mission.isGamePaused());
	gameShell->startMissionSuspended(mission, preloadScreen_.referenceString());
}

void ActionSetCurrentMission::activate()
{
	UI_LogicDispatcher::instance().setCurrentMission(MissionDescription(missionName.c_str(), gameType_));
}

void ActionResetCurrentMission::activate()
{
	UI_LogicDispatcher::instance().resetCurrentMission();
}

void ActionExitFromMission::activate()
{
	gameShell->terminateMission();
}

void ActionSetGamePause::activate()
{
	if(!universe()->isMultiPlayer()) {
		if(switchType == ON)
			gameShell->pauseGame(GameShell::PAUSE_BY_MENU);
		else
			gameShell->resumeGame(GameShell::PAUSE_BY_MENU);
	}
}

void ActionSetControlEnabled::activate()
{
	if(universeX()->isPlayingReel())
		return;
	aiPlayer().setControlEnabled(controlEnabled);
	gameShell->setControlEnabled(controlEnabled);
}

void ActionSetCursor::activate()
{
	UI_LogicDispatcher::instance().setCursor(cursor);
	UI_LogicDispatcher::instance().toggleCursorTriggered(true);
}

void ActionFreeCursor::activate()
{
	UI_LogicDispatcher::instance().toggleCursorTriggered(false);
}

void ActionChangeUnitCursor::activate()
{
	if (attribute.get())
		const_cast<AttributeBase*>(attribute.get())->setSelectionCursorProxy(cursor);
}

void ActionCreateNetClient::activate()
{
	UI_LogicDispatcher::instance().getNetCenter().create(type_);
}

void ActionUI_GameStart::activate()
{
	MissionDescription* mission = UI_LogicDispatcher::instance().currentMission();
	if(mission){
		mission->setGamePaused(paused_ || mission->isGamePaused());
		if(isBattle_)
			mission->setBattle(true);
	}
	UI_LogicDispatcher::instance().missionStart();
}

void ActionOnlineLogout::activate()
{
	if(aiPlayer().active())
		UI_LogicDispatcher::instance().getNetCenter().logout();
}

void ActionUI_LanGameStart::activate()
{
	if(aiPlayer().active())
		UI_LogicDispatcher::instance().getNetCenter().startGame();
}

void ActionUI_LanGameJoin::activate()
{
	if(aiPlayer().active())
		UI_LogicDispatcher::instance().getNetCenter().joinGame();
}

void ActionUI_LanGameCreate::activate()
{
	if(aiPlayer().active())
		UI_LogicDispatcher::instance().getNetCenter().createGame();
}

void ActionResetNetCenter::activate()
{
	if(aiPlayer().active())
		UI_LogicDispatcher::instance().getNetCenter().reset();
}

void ActionKillNetCenter::activate()
{
	if(aiPlayer().active())
		UI_LogicDispatcher::instance().getNetCenter().release();
}

bool ConditionLastNetStatus::check() const
{
	return UI_LogicDispatcher::instance().getNetCenter().status() == status_;
}

void ActionToggleBuildingInstaller::activate()
{
	if(attributeReference_)
		UI_LogicDispatcher::instance().toggleBuildingInstaller(attributeReference_);
}

MissionDescription* MissionDescriptionForTrigger::operator ()() const 
{
	if(useLoadedMission_)
		return (gameShell->CurrentMission.isLoaded() ? &gameShell->CurrentMission : 0);
	else
		return UI_LogicDispatcher::instance().currentMission();
}

void ActionUI_UnitCommand::activate()
{
	selectManager->makeCommand(unitCommand);
}

void fCommandSelectUnitByAttr(XBuffer& stream)
{
	AttributeReference* unitAttr;
	bool onlyPowered;
	stream.read(unitAttr);
	stream.read(onlyPowered);
	selectManager->selectAll(*unitAttr, onlyPowered);
}

void ActionSelectUnit::activate()
{
	streamLogicCommand.set(fCommandSelectUnitByAttr) << (void*)&unitID << onlyConnected_;
}

void ActionDeselect::activate()
{
	bool controlEnabled = aiPlayer().controlEnabled();
	if(!controlEnabled)
		aiPlayer().setControlEnabled(true);

	selectManager->deselectAll();
	if(stopPlayerUnit_ && aiPlayer().playerUnit()){
		aiPlayer().playerUnit()->executeCommand(UnitCommand(COMMAND_ID_STOP));
		if(aiPlayer().active())
			UI_LogicDispatcher::instance().clearClickMode();
	}

	if(!controlEnabled)
		aiPlayer().setControlEnabled(false);
}

void ActionGameQuit::activate()
{
	gameShell->terminate();
}

bool ConditionIsMultiplayer::check() const
{
	if(!PNetCenter::isNCCreated())
		return false;

	switch(mode){
	case ANY:
		return true;
	case LAN:
		return PNetCenter::isNCConfigured(PNCWM_LAN_DW);
	case ONLINE:
		return PNetCenter::isNCConfigured(PNCWM_ONLINE_DW);
	case DIRECT:
		return PNetCenter::isNCConfigured(PNCWM_ONLINE_P2P);
	default:
		xassert(0);
		return false;
	}
}

void ActionSetInt::activate()
{
	switch(scope_){
	case SCOPE_GLOBAL:
		Singleton<IntVariables>::instance()[name_] = value_;
		break;
	case SCOPE_PROFILE:
		universe()->currentProfileIntVariables()[name_] = value_;
		break;
	case SCOPE_PLAYER:
		aiPlayer().intVariables()[name_] = value_;
		break;
	case SCOPE_UNIVERSE:
		universe()->intVariables()[name_] = value_;
		break;
	case SCOPE_MISSION_DESCRIPTION: {
		MissionDescription* mission = missionDescription();
		if(mission){
			mission->setTriggerFlags(valueBool_ ?
				mission->triggerFlags() | (1 << value_) : mission->triggerFlags() & ~(1 << value_));
			if(PNetCenter::isNCCreated())
				PNetCenter::instance()->changeMissionDescription(MissionDescription::CMDV_ChangeTriggers, mission->triggerFlags());
		}
		}
		break;
	}
}

void ActionSetCutScene::activate()
{
	gameShell->setCutScene(switchMode);
	universe()->setInterfaceEnabled(!switchMode);
	if(gameShell->CurrentMission.silhouettesEnabled)
		gb_VisGeneric->EnableSilhouettes( switchMode == ON ? false : GameOptions::instance().getBool(OPTION_SILHOUETTE));
}

void ActionSetCurrentMissionAsPassed::activate()
{
	UI_LogicDispatcher::instance().currentProfile().passedMissions.add(gameShell->CurrentMission.missionGUID());
}


bool ConditionSelected::check() const
{
	AttributeUnitOrBuildingReferences::const_iterator it;
	FOR_EACH(objects_, it)
		if(selectManager->isSelected(*it, singleOnly, uniform))
			return true;
	return false;
}

bool ConditionSquadSelected::check() const
{
	return selectManager->isSquadSelected(attribute, singleOnly);
}

bool ConditionCheckPause::check() const
{
	return gameShell->isPaused((pausedByUser_ ? GameShell::PAUSE_BY_USER : 0) |	(pausedByMenu_ ? GameShell::PAUSE_BY_MENU : 0));
}

bool ConditionPlayerByNumberDefeat::check() const
{
	int index = gameShell->CurrentMission.findPlayerIndex(playerIndex);
	return index >= 0 && universe()->findPlayer(index)->isDefeat();
}

bool isUnderEditor()
{
	return false;
}

void UnitBase::showPathTrackingMap()
{
	UnitInterface* unit = selectManager->selectedUnit();
	if(unit && checkInPathTracking(unit->getUnitReal()))
		show_vector(position(), radius(), Color4c::WHITE);
}

void UnitActing::setActiveDirectControl(DirectControlMode activeDirectControl, int transitionTime)
{
	if(activeDirectControl_ != activeDirectControl)
		gameShell->setDirectControl(activeDirectControl_ = activeDirectControl, this, transitionTime);
}

void UpdateSilhouettes()
{
	if(universe()){
		if(gameShell->CurrentMission.silhouettesEnabled)
			gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));
	}
	else
		gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));
}

CommandColorManager::CommandColorManager()
{
	for(int i = 0; i < COMMAND_MAX; i++){
		colors_.push_back(Color3c(0, 0, 0));
	}
}
