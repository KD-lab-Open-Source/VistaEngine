#include "StdAfx.h"

#include "UI_Logic.h"

UI_LogicDispatcher::UI_LogicDispatcher()
{ 
	mousePosition_ = Vect2f::ZERO;
	mouseFlags_ = 0;

	inputFlags_ = 0;

	activeCursor_ = 0;
    cursorVisible_ = true;
	cursorEffect_ = 0;
	cursorTriggered_ = false;

	hoverUnit_ = 0;
    startTrakingUnit_ = 0;
    selectedUnit_ = 0;
    selectedUnitIfOne_ = 0;

	hoverControl_ = 0;
	hoverControlInfo_ = 0;

	lastHotKeyInput_ = 0;

	enableInput_ = true;
	gamePause_ = false;

	weaponUpKeyPressed_ = false;
	weaponDownKeyPressed_ = false;

	selectedWeapon_ = 0;
	currentWeaponCursor_ = 0;

	showAllUnitParametersMode_ = false;
	showItemHintsMode_ = false;

	clickMode_ = UI_CLICK_MODE_NONE;
	cachedClickMode_ = UI_CLICK_MODE_NONE;
	clearClickMode_ = false;

	cachedWeaponID_ = 0;
	mousePressPos_ = Vect2f(0,0);
	trackMode_ = false;

	hoverPassable_ = false;
	traficabilityCommonFlag_ = 0;

	aimPosition_ = Vect3f::ZERO;
	aimDistance_ = 0.f;

	selectionTexture_ = 0;
	selectionCornerTexture_ = 0;
	selectionCenterTexture_ = NULL;

	showTips_ = true;
	showMessages_ = true;

	takenItemSet_ = false;

	currentClickAttackState_ = false;
	attackCoordTime_ = 0;

	currentTeemGametype_ = TEAM_GAME_TYPE_INDIVIDUAL;

	diskOpID_ = UI_DISK_OP_NONE;
	diskOpConfirmed_ = false;

	lastCurrentHiVer_ = 0;
	lastCurrentLoVer_ = 0;
}

UI_LogicDispatcher::~UI_LogicDispatcher()
{
}

bool UI_LogicDispatcher::init()
{
	return true;
}

void UI_LogicDispatcher::controlInit(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data){
}

void UI_LogicDispatcher::controlUpdate(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data, ControlState&){
}

void UI_LogicDispatcher::controlAction(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data){
}

void UI_LogicDispatcher::redraw()
{
}

void UI_LogicDispatcher::showDebugInfo() const
{
}

void UI_LogicDispatcher::drawDebugInfo() const
{
}

void UI_LogicDispatcher::drawDebug2D() const
{
}

void UI_LogicDispatcher::sendBuildMessages(const AttributeBase* buildingAttr) const
{
}

void UI_LogicDispatcher::destroyLinkQuant()
{
}

void UI_LogicDispatcher::logicPreQuant()
{
}

void UI_LogicDispatcher::logicQuant(float dt)
{
}

void UI_LogicDispatcher::directShootQuant()
{
}

void UI_LogicDispatcher::reset()
{
}

void UI_LogicDispatcher::graphQuant(float dt)
{
}

void UI_LogicDispatcher::updateLinkToCursor(const class cObject3dx*, int, const class EffectAttribute&)
{
}

void UI_LogicDispatcher::handleMessage(const ControlMessage& msg)
{
}

bool UI_LogicDispatcher::handleInput(const UI_InputEvent& event)
{
	return false;
}

void UI_LogicDispatcher::updateInput(const UI_InputEvent& event)
{
}

void UI_LogicDispatcher::updateAimPosition()
{
}

void UI_LogicDispatcher::setCursor(const UI_Cursor* cursor_ref)
{
}

void UI_LogicDispatcher::showCursor() 
{
}

void UI_LogicDispatcher::hideCursor() 
{
}

void UI_LogicDispatcher::updateCursor()
{
}

void UI_LogicDispatcher::setDefaultCursor()
{
}

void UI_LogicDispatcher::SetSelectPeram(const struct SelectionBorderColor& param)
{
}

void UI_LogicDispatcher::createSignSprites()
{
}

void UI_LogicDispatcher::releaseSignSprites()
{
}

bool UI_LogicDispatcher::releaseResources()
{
	return false;
}

bool UI_LogicDispatcher::addLight(int light_index, const Vect3f& position) const
{
	return false;
}

void UI_LogicDispatcher::addMark(const UI_MarkObjectInfo&)
{
}

void UI_LogicDispatcher::selectClickMode(UI_ClickModeID mode_id, const WeaponPrm* selected_weapon)
{
}

void UI_LogicDispatcher::inputEventProcessed(const UI_InputEvent& event, UI_ControlBase* control)
{
}

void UI_LogicDispatcher::focusControlProcess(const UI_ControlBase*)
{
}

void UI_LogicDispatcher::updateHoverInfo()
{
	if(hoverControl_)
		hoverControl_->startShowInfo();
}

void UI_LogicDispatcher::setTakenItem(const UI_InventoryItem* item)
{
}

void UI_LogicDispatcher::toggleTracking(bool state)
{
}

void UI_LogicDispatcher::toggleBuildingInstaller(const AttributeBase* attr)
{
}

void UI_LogicDispatcher::disableDirectControl()
{
}
void UI_LogicDispatcher::createCursorEffect()
{
}

void UI_LogicDispatcher::moveCursorEffect()
{
}


void UI_LogicDispatcher::saveGame(const string& gameName, bool autoSave)
{
}

int UI_LogicDispatcher::selectedWeaponID() const
{
	return 0;
}

MissionDescription* UI_LogicDispatcher::currentMission() const
{
	return 0;
}

bool UI_LogicDispatcher::isGameActive() const
{
	return false;
}

bool UI_LogicDispatcher::drawSelection(const Vect2f& vv0, const Vect2f& vv1) const
{
	return false;
}

Player* UI_LogicDispatcher::player() const
{
	return 0;
}

void UI_LogicDispatcher::addMovementMark()
{
}

void UI_LogicDispatcher::showLoadProgress(float val)
{
}

const wchar_t* UI_LogicDispatcher::getParam(const wchar_t* name, const ExpandInfo&, WBuffer& retBuf)
{
	return L"";
}

void UI_LogicDispatcher::setDiskOp(UI_DiskOpID id, const char* path)
{
}

void UI_LogicDispatcher::expandTextTemplate(wstring& text, const ExpandInfo&)
{
}

void fCommandResetIdleUnits(XBuffer&)
{
}

class UnitActing;
void fCommandAddIdleUnits(XBuffer& stream)
{
	UnitActing* unit;
	stream.read(unit);
}

class UI_Cursor;
void UI_LogicDispatcher::selectCursor(const UI_Cursor*)
{
}

void UI_LogicDispatcher::handleMessageReInitGameOptions()
{
}

bool UI_LogicDispatcher::parseGameVersion(const char* ptr)
{
	return true;
}

bool UI_LogicDispatcher::checkNeedUpdate() const
{
	return false;
}

void UI_LogicDispatcher::openUpdateUrl() const
{
}

void UI_LogicDispatcher::addSquadSelectSign(const Recti&, class UnitInterface*)
{
}

void UI_LogicDispatcher::addUnitOffscreenSprite(const UnitObjective* unit)
{
}

void UI_LogicDispatcher::drawUnitSideSprites()
{
}
