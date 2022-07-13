#include "StdAfx.h"
#include "Game\Actions.h"
#include "Game\Conditions.h"
#include "Game\GameOptions.h"
#include "Serialization\SerializationFactory.h"
#include "CommandsQueue.h"
#include "Render\Src\VisGeneric.h"

void ActionSetDirectControl::activate()
{
}

void ActionShowReel::activate()
{
}

bool ActionShowReel::workedOut()
{
	return true;
}

void ActionShowLogoReel::activate()
{
}

void ActionStartMission::activate()
{
}

void ActionLoadGameAuto::activate()
{
}

void ActionSetControlEnabled::activate()
{
}

void ActionSetCurrentMission::activate()
{
}

void ActionResetCurrentMission::activate()
{
}

void ActionSetGamePause::activate()
{
}

void ActionExitFromMission::activate()
{
}

void ActionSetCursor::activate()
{
}

void ActionFreeCursor::activate()
{
}

void ActionChangeUnitCursor::activate()
{
}

void ActionCreateNetClient::activate()
{
}

void ActionOnlineLogout::activate()
{
}

void ActionUI_GameStart::activate()
{
}

void ActionUI_LanGameStart::activate()
{
}

void ActionUI_LanGameJoin::activate()
{
}

void ActionUI_LanGameCreate::activate()
{
}

void ActionResetNetCenter::activate()
{
}

void ActionKillNetCenter::activate()
{
}

bool ConditionLastNetStatus::check() const
{
	return false;
}

void ActionToggleBuildingInstaller::activate()
{
}

void ActionUI_UnitCommand::activate()
{
}

void ActionDeselect::activate()
{
}

void ActionGameQuit::activate()
{
}

void ActionSelectUnit::activate()
{
}

void ActionSetInt::activate()
{
}

void ActionSetCutScene::activate()
{
}

void ActionSetCurrentMissionAsPassed::activate()
{
}

bool ConditionSelected::check() const
{
	return false;
}

bool ConditionSquadSelected::check() const
{
	return false;
}

bool ConditionCheckPause::check() const
{
	return false;
}

bool ConditionIsMultiplayer::check() const
{
	return false;
}

bool ConditionPlayerByNumberDefeat::check() const
{
	return false;
}

void updateResolution(Vect2i size, bool change_size)
{
}

bool isUnderEditor()
{
	return true;
}

bool applicationHasFocus()
{
	return true;
}

void UnitBase::showPathTrackingMap()
{

}

MissionDescription* MissionDescriptionForTrigger::operator ()() const
{
	return 0;
}

void UnitActing::setActiveDirectControl(DirectControlMode activeDirectControl, int transitionTime)
{

}

void UpdateSilhouettes()
{
	gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));
}

CommandColorManager::CommandColorManager()
{
	for(int i = 0; i < COMMAND_MAX; i++){
		float hue = (360.f * i) / COMMAND_MAX;
		Color4f color;
		color.setHSV(hue, 1.f, 1.f);
		colors_.push_back(Color3c(color));
	}
}

