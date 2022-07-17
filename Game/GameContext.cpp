#include "StdAfx.h"
#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "terra.h"
#include "Universe.h"
#include "Squad.h"
#include "Serialization.h"
#include "ShowHead.h"
#include "ResourceSelector.h"
#include "GameShell.h"
#include "Actions.h"
#include "Conditions.h"
#include "..\HT\ht.h"
#include "UI_Logic.h"
#include "..\Network\P2P_interface.h"
#include "SelectManager.h"
#include "GameOptions.h"
#include "..\Environment\Environment.h"
#include "..\UserInterface\GameShell.h"
#include "..\UserInterface\UI_NetCenter.h"
#include "..\Network\UniverseX.h"
#include "..\Util\FileUtils.h"

#include "TreeInterface.h"

#include "StreamCommand.h"
#include "EditorVisual.h"

EditorVisual::Interface& editorVisual()
{
	xassert(0 && "EditorVisual можно использовать только в редакторе!");
	return *reinterpret_cast<EditorVisual::Interface*>(0);
}

extern GameShell* gameShell;
void UpdateGameshellWindowSize();
void CalcRealWindowPos(int xPos,int yPos,int xScr,int yScr,bool fullscreen,Vect2i& pos,Vect2i& size);
bool SetMultisampleByOptions(int xScr,int yScr,int renderMode);
void ErrorInitialize3D();

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
		return !timer;
	else
		return true;
}

void ActionShowLogoReel::activate()
{
	if(gameShell->isVideoEnabled()){
		LogoAttributes logoAttributes;
		logoAttributes.bkgName = bkgName;
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
	gameShell->startMissionSuspended(mission);
}

void ActionSetCurrentMission::activate()
{
	UI_LogicDispatcher::instance().setCurrentMission(MissionDescription(missionName.c_str(), gameType_));
}

void ActionReseCurrentMission::activate()
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


void ActionChangeCommonCursor::activate()
{
	UI_GlobalAttributes::instance().setCursor(cursorType, cursor);
}

void ActionCreateNetClient::activate()
{
	UI_LogicDispatcher::instance().getNetCenter().create(type_);
}

void ActionUI_GameStart::activate()
{
	MissionDescription* mission = UI_LogicDispatcher::instance().currentMission();
	if(mission)
		mission->setGamePaused(paused_ || mission->isGamePaused());
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
	stream.read(unitAttr);
	selectManager->selectAll(*unitAttr);
}

void ActionSelectUnit::activate()
{
	streamLogicCommand.set(fCommandSelectUnitByAttr) << (void*)&unitID;
}

void ActionDeselect::activate()
{
	selectManager->deselectAll();
}

void ActionGameQuit::activate()
{
	gameShell->terminate();
}

bool ConditionIsMultiplayer::check() const
{
	if(!gameShell->getNetClient())
		return false;

	switch(mode){
	case ANY:
		return true;
	case LAN:
		return gameShell->isNetClientConfigured(PNCWM_LAN_DW);
	case ONLINE:
		return gameShell->isNetClientConfigured(PNCWM_ONLINE_DW);
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
			if(gameShell->getNetClient())
				gameShell->getNetClient()->changeMissionDescription(MissionDescription::CMDV_ChangeTriggers, mission->triggerFlags());
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

void RepositionWindow(Vect2i size)
{
	Vect2i real_pos;
	Vect2i real_size;

	CalcRealWindowPos(
		0, 0,
		size.x, size.y,
		terFullScreen,
		real_pos,
		real_size
		);

	DWORD overlap = WS_OVERLAPPED| WS_CAPTION| WS_SYSMENU | WS_MINIMIZEBOX;
	if(GameOptions::instance().getBool(OPTION_DEBUG_WINDOW))
		overlap |= WS_THICKFRAME | WS_MAXIMIZEBOX;

	SetWindowLong(gb_RenderDevice->GetWindowHandle(),GWL_STYLE,(terFullScreen ? 0 : overlap)|WS_POPUP|WS_VISIBLE);
	SetWindowPos(
		gb_RenderDevice->GetWindowHandle(),
		HWND_NOTOPMOST,
		real_pos.x,
		real_pos.y,
		real_size.x,
		real_size.y,
		SWP_SHOWWINDOW | SWP_FRAMECHANGED
		);
}

void updateResolution(Vect2i size, bool change_size)
{
	RepositionWindow(size);
	gb_RenderDevice->SetMultisample(GameOptions::instance().getInt(OPTION_ANTIALIAS));

	int mode = RENDERDEVICE_MODE_RETURNERROR;
	if(!terFullScreen)
		mode |= RENDERDEVICE_MODE_WINDOW;

	if(!gb_RenderDevice->ChangeSize(size.x, size.y, mode))
	{
		gb_RenderDevice->SetMultisample(0);
		if(!gb_RenderDevice->ChangeSize(800,600,mode))
			ErrorInitialize3D();
	}

	if(change_size)
		gb_VisGeneric->ReloadAllFont();

	RepositionWindow(Vect2i(gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY()));

	if(cameraManager)
		cameraManager->SetFrustum();

	UpdateGameshellWindowSize();
}

bool isUnderEditor()
{
	return false;
}

AttribEditorInterface& attribEditorInterface(){
    return attribEditorInterfaceExternal();
} 

void UnitBase::showPathTrackingMap()
{
	UnitInterface* unit = selectManager->selectedUnit();
	if(unit && checkInPathTracking(unit->getUnitReal()))
		show_vector(position(), radius(), WHITE);
}

void UnitActing::setActiveDirectControl(DirectControlMode activeDirectControl)
{
	if(activeDirectControl_ != activeDirectControl)
		gameShell->setDirectControl(activeDirectControl_ = activeDirectControl, this, 1000);
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