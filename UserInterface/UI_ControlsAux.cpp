#include "StdAfx.h"

#include "UserInterface.h"
#include "UI_Controls.h"
#include "UI_Inventory.h"

// ------------------- UI_ControlBase

void UI_ControlBase::logicInit()
{
	controlInit();

	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->logicInit();
}

void UI_ControlBase::actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data){
}

void UI_ControlBase::logicQuant()
{
}

void UI_ControlBase::controlUpdate(ControlState& controlState)
{
}

void UI_ControlBase::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
}

void UI_ControlBase::quant(float dt)
{
	animationQuant(dt);
	transformQuant(dt);

	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->quant(dt);
}

void UI_ControlBase::action(){
}

void UI_ControlBase::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data){
}

void UI_ControlBase::handleAction(const ControlMessage& msg){
}

bool UI_ControlBase::handleInput(const UI_InputEvent& event)
{
	return false;
}

bool UI_ControlBase::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}

const AttributeBase* UI_ControlBase::actionUnit(const AttributeBase* selected) const
{
	return 0;
}
// ------------------- UI_ControlButton

void UI_ControlButton::textChanged()
{
}

void UI_ControlButton::actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data){
}

void UI_ControlButton::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState){
}

void UI_ControlButton::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data){
}

// ------------------- UI_ControlTextList

void UI_ControlTextList::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
}

bool UI_ControlTextList::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}

// ------------------- UI_ControlSlider

bool UI_ControlSlider::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}
// ------------------- UI_ControlHotKeyInput

void UI_ControlHotKeyInput::quant(float dt)
{
	__super::quant(dt);
}

bool UI_ControlHotKeyInput::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}

void UI_ControlHotKeyInput::actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data)
{
}

void UI_ControlHotKeyInput::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data)
{
}

// ------------------- UI_ControlEdit


void UI_ControlEdit::quant(float dt)
{
	__super::quant(dt);
}

bool UI_ControlEdit::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}

// ------------------- UI_ControlStringList
bool UI_ControlStringList::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}

// ------------------- UI_ControlProgressBar
void UI_ControlProgressBar::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState&){
}

// ------------------- UI_ControlCustom


bool UI_ControlCustom::redraw() const
{
	return UI_ControlBase::redraw();
}

void UI_ControlCustom::controlInit()
{
}

bool UI_ControlCustom::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}

// ------------------- UI_ControlUnitList

void UI_ControlUnitList::controlUpdate(ControlState& cs)
{
}

// ------------------- UI_ControlInventory

bool UI_ControlInventory::inputEventHandler(const UI_InputEvent& event)
{
	return false;
}
