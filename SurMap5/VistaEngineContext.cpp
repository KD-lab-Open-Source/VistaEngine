#include "StdAfx.h"
#include "Actions.h"
#include "Conditions.h"
#include "AbnormalStateAttribute.h"
//#include "WeaponAttribute.h"
#include "IronBullet.h"
#include "IronLegion.h"
#include "GameOptions.h"

#include "TreeEditors\TreeReferenceEditor.h"
#include "TreeEditors\TreeTypeLibrarySelector.h"

#define REGISTER_REFERENCE_SELECTOR(Type) \
typedef TreeReferenceSelector<Type> Type##_Selector; \
REGISTER_TREE_EDITOR(typeid(Type).name(), Type##_Selector, &Type::StringTableType::instance);

REGISTER_REFERENCE_SELECTOR(SourceReference)
REGISTER_REFERENCE_SELECTOR(WeaponPrmReference)
REGISTER_REFERENCE_SELECTOR(AttributeReference)

REGISTER_REFERENCE_SELECTOR(AttributeUnitReference)
REGISTER_REFERENCE_SELECTOR(AttributeUnitOrBuildingReference)
REGISTER_REFERENCE_SELECTOR(AttributeItemReference)
REGISTER_REFERENCE_SELECTOR(AttributeItemResourceReference)
REGISTER_REFERENCE_SELECTOR(AttributeItemInventoryReference)

REGISTER_REFERENCE_SELECTOR(SoundReference)
REGISTER_REFERENCE_SELECTOR(TerToolReference)
REGISTER_REFERENCE_SELECTOR(EffectReference)
REGISTER_REFERENCE_SELECTOR(AttributeProjectileReference)
REGISTER_REFERENCE_SELECTOR(ParameterValueReference)
REGISTER_REFERENCE_SELECTOR(PlacementZone)
REGISTER_REFERENCE_SELECTOR(UnitFormationTypeReference)
REGISTER_REFERENCE_SELECTOR(ParameterFormulaReference)
REGISTER_REFERENCE_SELECTOR(ParameterTypeReference)
REGISTER_REFERENCE_SELECTOR(ParameterGroupReference)
REGISTER_REFERENCE_SELECTOR(AbnormalStateTypeReference)
REGISTER_REFERENCE_SELECTOR(BodyPartType)
REGISTER_REFERENCE_SELECTOR(UI_MessageTypeReference)

#undef REGISTER_REFERENCE_POLYMORPHIC_SELECTOR

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

void ActionReseCurrentMission::activate()
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

void ActionChangeCommonCursor::activate()
{
}

void ActionCreateNetClient::activate()
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

void UnitActing::setActiveDirectControl(DirectControlMode activeDirectControl)
{

}

void UpdateSilhouettes()
{
	gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));
}