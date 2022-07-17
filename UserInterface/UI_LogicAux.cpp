#include "StdAfx.h"

#include "UI_Logic.h"

UI_LogicDispatcher::UI_LogicDispatcher()
{
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

void UI_LogicDispatcher::addMark(const UI_MarkObjectInfo&, float)
{
}

void UI_LogicDispatcher::selectClickMode(UI_ClickModeID mode_id, const WeaponPrm* selected_weapon)
{
}

bool UI_LogicDispatcher::inputEventProcessed(const UI_InputEvent& event, UI_ControlBase* control)
{
	return false;
}

void UI_LogicDispatcher::focusControlProcess(const UI_ControlBase*)
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


void UI_LogicDispatcher::saveGame(const string& gameName)
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

void UI_LogicDispatcher::startLoadProgressSection(UI_LoadProgressSectionID id)
{
}

void UI_LogicDispatcher::loadProgressUpdate()
{
}

const char* UI_LogicDispatcher::getParam(const char* name, UnitInterface* unit, const AttributeBase* attr)
{
	return "";
}

void UI_LogicDispatcher::setDiskOp(UI_DiskOpID id, const char* path)
{
}

void UI_LogicDispatcher::expandTextTemplate(string& text, UnitInterface* unit, const AttributeBase* attr)
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