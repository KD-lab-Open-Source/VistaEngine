#include "StdAfx.h"

#include "XTL\SafeCast.h"
#include "Serialization\Serialization.h"

#include "UI_Render.h"
#include "UserInterface.h"
#include "UI_Logic.h"
#include "UI_Actions.h"
#include "UI_Controls.h"
#include "UI_UnitView.h"
#include "UI_Minimap.h"

#include "GameShell.h"
#include "UnitAttribute.h"
#include "BaseUnit.h"
#include "Squad.h"
#include "Environment\Anchor.h"

#include "SelectManager.h"
#include "ShowHead.h"

#include "CameraManager.h"
#include "Environment\Environment.h"
#include "Environment\SourceManager.h"
#include "Terra\vMap.h"
#include "Render\Src\cCamera.h"

#include "Universe.h"
#include "WBuffer.h"

// ------------------- UI_ControlBase

void UI_ControlBase::logicInit()
{
	MTL();
	LOG_UI_STATE(0);

	waitingUpdate_ = true;
	transformMode_ = TRANSFORM_NONE;
	
	controlInit();
	
	for(int i = 0; i < actionCount(); i++)
		actionInit(actionID(i), actionData(i));

	for(StateContainer::iterator it = states_.begin(); it != states_.end(); ++it){
		for(int i = 0; i < it->actionCount(); i++)
			actionInit(it->actionID(i), it->actionData(i));
	}

	ControlList::iterator ic;
	FOR_EACH(controls_, ic)
		(*ic)->logicInit();
}

void UI_ControlBase::actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data){
	UI_LogicDispatcher::instance().controlInit(action_id, this, action_data);
}

//----------------------------------------

void UI_ControlBase::logicQuant()
{
	MTL();
	LOG_UI_STATE(L"begin");

	ControlState cs;
	bool and = false;
	bool waitingUpdate = waitingUpdate_;
	
	controlUpdate(cs);
	
	for(int i = 0; i < actionCount(); i++){
		UI_ControlActionID id;
		if((id = actionID(i)) > UI_ACTION_NONE){
			const UI_ActionData* data = actionData(i);
			if(data->needUpdate())
				actionUpdate(id, data, cs);
		}
		else if(id == UI_ACTION_INVERT_SHOW_PRIORITY)
			and = true;
	}
	
	if(const UI_ControlState* state = currentState())
		for(int i = 0; i < state->actionCount(); i++){
			UI_ControlActionID id;
			if((id = state->actionID(i)) > UI_ACTION_NONE){
				const UI_ActionData* data = state->actionData(i);
				if(data->needUpdate())
					actionUpdate(id, data, cs);
			}
			else if(id == UI_ACTION_INVERT_SHOW_PRIORITY)
				and = true;
		}

	
	if(cs.stateIdx >= 0)
		setState(cs.stateIdx);
		
	cs.setPriority(and);

	if(cs.show_control){
		if(!isVisible_)	
			show();
	}
	else if(cs.hide_control && isVisible_)
		hide(waitingUpdate_ || !isVisibleByTrigger_);

	if(isVisible() || waitingUpdate_)
	{
		if(cs.disable_control)
			disable();
		else if(cs.enable_control)
			enable();

		ControlList::iterator ic;
		FOR_EACH(controls_, ic)
			(*ic)->logicQuant();
	}
	
	if(waitingUpdate)
		waitingUpdate_ = false;

	LOG_UI_STATE(L"end");
}

void UI_ControlBase::controlUpdate(ControlState& controlState)
{
}

void UI_ControlBase::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
	switch(action_id){
		case UI_ACTION_LOCALIZE_CONTROL:
			if(safe_cast<const UI_ActionDataLocString*>(action_data)->expand()){
				wstring str(safe_cast<const UI_ActionDataLocString*>(action_data)->text());
				ExpandInfo info(ExpandInfo::MESSAGE,
					safe_cast<const UI_ActionDataLocString*>(action_data)->forHovered()
					? UI_LogicDispatcher::instance().hoverUnit()
					: UI_LogicDispatcher::instance().selectedUnit());
				UI_LogicDispatcher::instance().expandTextTemplate(str, info);
				if(!str.empty()){
					setText(str.c_str());
					controlState.show();
				}
				else {
					setText(0);
					controlState.hide();
				}
			}
			break;
		case UI_ACTION_BIND_TO_UNIT:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActionDataUnitOrBuildingUpdate* action = safe_cast<const UI_ActionDataUnitOrBuildingUpdate*>(action_data)){
				UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit();
				if(	unit &&
					(!action->uniformSelection() || selectManager->uniform()) &&
					(!action->single() || selectManager->selectionSize() == 1)){
					
					if(unit->selectionAttribute() == action->attribute())
						controlState.show();
					else
						controlState.hide();
				}
				else
					controlState.hide();
			}
			break;
		
		case UI_ACTION_BIND_TO_HOVERED_UNIT:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActiobDataUnitHovered* action = safe_cast<const UI_ActiobDataUnitHovered*>(action_data)){
				UnitInterface* unit = UI_LogicDispatcher::instance().hoverUnit();
				if(	unit &&
					(!action->own() || unit->player() == UI_LogicDispatcher::instance().player()))
				{
						if(unit->selectionAttribute() == action->attribute())
							controlState.show();
						else
							controlState.hide();
				}
				else
					controlState.hide();
			}
			break;
		
		case UI_ACTION_BIND_TO_UNIT_STATE: {
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			const UI_ActionUnitState* action = safe_cast<const UI_ActionUnitState*>(action_data);
			const UnitInterface* unit = (action->playerUnit() ?
				UI_LogicDispatcher::instance().player()->playerUnit()
				: UI_LogicDispatcher::instance().selectedUnit());
			if(unit){
				const UnitReal* real = unit->getUnitReal();
				const UnitActing* acting = real && real->attr().isActing() ? safe_cast<const UnitActing*>(real) : 0; 
				switch(action->type()){
				case UI_ActionUnitState::UI_UNITSTATE_RUN:
					if(real && real->attr().isLegionary())
						controlState.setState((int)safe_cast<const UnitLegionary*>(real)->runMode());
					break;
				case UI_ActionUnitState::UI_UNITSTATE_AUTO_FIND_TRANSPORT:{
					if(acting){
						controlState.show();
						controlState.setState((int)acting->autoFindTransport());
					} 
					else
						controlState.hide();
					break;
																			}
				case UI_ActionUnitState::UI_UNITSTATE_SELF_ATTACK:
					controlState.setState(getStateByMark(UI_STATE_MARK_UNIT_SELF_ATTACK, (int)acting->autoAttackMode()));
					break;
				case UI_ActionUnitState::UI_UNITSTATE_WEAPON_MODE:
					controlState.setState(getStateByMark(UI_STATE_MARK_UNIT_WEAPON_MODE, (int)acting->weaponMode()));
					break;
				case UI_ActionUnitState::UI_UNITSTATE_WALK_ATTACK_MODE:
					controlState.setState(getStateByMark(UI_STATE_MARK_UNIT_WALK_ATTACK_MODE, (int)acting->walkAttackMode()));
					break;
				case UI_ActionUnitState::UI_UNITSTATE_AUTO_TARGET_FILTER:
					controlState.setState(getStateByMark(UI_STATE_MARK_UNIT_AUTO_TARGET_FILTER, (int)acting->autoTargetFilter()));
					break;

				case UI_ActionUnitState::UI_UNITSTATE_CAN_DETONATE_MINES:
					if(action->checkOnlyMainUnitInSquad() ? real->canDetonateMines() : unit->canDetonateMines())
						controlState.show();
					else
						controlState.hide();
					break;
				case UI_ActionUnitState::UI_UNITSTATE_IS_IDLE:
					if(action->checkOnlyMainUnitInSquad() ? !real->isWorking() : !unit->isWorking())
						controlState.show();
					else
						controlState.hide();
					break;
				case UI_ActionUnitState::UI_UNITSTATE_CAN_UPGRADE:
					controlState.apply(action->checkOnlyMainUnitInSquad() ? real->canUpgrade(action->data()) : unit->canUpgrade(action->data()));
					break;
				case UI_ActionUnitState::UI_UNITSTATE_IS_UPGRADING:
					if((action->data() == -1 && real->isUpgrading()) || (action->data() >= 0 && real->executedUpgradeIndex() == action->data()))
						controlState.hide();
					else
						controlState.show();
					break;
				case UI_ActionUnitState::UI_UNITSTATE_CAN_BUILD:
					controlState.apply(action->checkOnlyMainUnitInSquad() ? real->canProduction(action->data()) : unit->canProduction(action->data()));
					break;
				case UI_ActionUnitState::UI_UNITSTATE_IS_BUILDING:
					if(unit->selectionAttribute()->hasProdused()){
						ProducedQueue::const_iterator it;
						FOR_EACH(acting->producedQueue(), it){
							if(it->type_ == PRODUCE_UNIT && it->data_ == action->data()){
								controlState.hide();
								break;
							}
						}
						controlState.show();
					}
					break;
				case UI_ActionUnitState::UI_UNITSTATE_CAN_PRODUCE_PARAMETER:
					controlState.apply(action->checkOnlyMainUnitInSquad() ? real->canProduceParameter(action->data()) : unit->canProduceParameter(action->data()));
					break;
				case UI_ActionUnitState::UI_UNITSTATE_COMMON_OPERABLE:
					if(!real->isUpgrading() && real->isConstructed())
						controlState.enable();
					else
						controlState.disable();
					break;
				case UI_ActionUnitState::UI_UNITSTATE_SQUAD_CAN_QUERY_UNITS:
					if(const UnitSquad* squad = unit->getSquadPoint()){
						if(UI_LogicDispatcher::instance().player()->canBuildStructure(action->attribute(), &squad->attr()) == CAN_START 
								&& squad->canQueryUnits(action->attribute()) 
								&& UI_LogicDispatcher::instance().player()->findFreeFactory(action->attribute()))
							controlState.enable();
						else
							controlState.disable();
					}
					break;
				}
			}
			break;
										   }
		case UI_ACTION_BINDEX:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActionBindEx* action = safe_cast<const UI_ActionBindEx*>(action_data)){
				UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit();
				if(	unit &&
					(!action->uniformSelection() || selectManager->uniform()) &&
						(action->selectionSize() == UI_ActionBindEx::UI_SEL_ANY ||
						(action->selectionSize() == UI_ActionBindEx::UI_SEL_SINGLE && selectManager->selectionSize() == 1) ||
						(action->selectionSize() == UI_ActionBindEx::UI_SEL_MORE_ONE && selectManager->selectionSize() > 1)) )
				{
					
					switch(action->type()){
					case UI_ActionBindEx::UI_BIND_PRODUCTION:
						if(unit->selectionAttribute()->hasProdused())
							if(action->queueSize() == UI_ActionBindEx::UI_QUEUE_ANY)
								controlState.show();
							else{
								bool empty = (unit = unit->getUnitReal())->attr().isActing() ? safe_cast<const UnitActing*>(unit)->producedQueue().empty() : true;
								if(empty && action->queueSize() == UI_ActionBindEx::UI_QUEUE_EMPTY ||
									!empty && action->queueSize() == UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
									controlState.show();
								else
									controlState.hide();
							}
						else
							if(action->queueSize() != UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
								controlState.show();
							else
								controlState.hide();
						
						break;
					case UI_ActionBindEx::UI_BIND_PRODUCTION_SQUAD:
						if(unit->getSquadPoint()){
							if(action->queueSize() == UI_ActionBindEx::UI_QUEUE_ANY)
								controlState.show();
							else{
								bool empty = unit->getSquadPoint()->requestedUnits().empty();
								if(empty && action->queueSize() == UI_ActionBindEx::UI_QUEUE_EMPTY ||
									!empty && action->queueSize() == UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
									controlState.show();
								else
									controlState.hide();
							}
						}else
							if(action->queueSize() != UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
								controlState.show();
							else
								controlState.hide();
						
						break;
						
					case UI_ActionBindEx::UI_TRANSPORT:
						if(unit->selectionAttribute()->isTransport())
							if(action->queueSize() == UI_ActionBindEx::UI_QUEUE_ANY)
								controlState.show();
							else{
								bool empty = !(unit = unit->getUnitReal())->attr().isActing();
								if(!empty){
									const LegionariesLinks& slots = safe_cast<UnitActing*>(unit->getUnitReal())->transportSlots();
									LegionariesLinks::const_iterator slot_it;
									FOR_EACH(slots, slot_it)
										if(*slot_it && (*slot_it)->isDocked())
											break;
									empty = (slot_it == slots.end());
								}
								
								if(empty && action->queueSize() == UI_ActionBindEx::UI_QUEUE_EMPTY ||
									!empty && action->queueSize() == UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
									controlState.show();
								else
									controlState.hide();
							}
						else
							if(action->queueSize() != UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
								controlState.show();
							else
								controlState.hide();
						
						break;
					case UI_ActionBindEx::UI_BIND_ANY:
						controlState.show();
						break;
					}
				}
				else
					controlState.hide();

			}
			break;
		
		case UI_ACTION_UNIT_COMMAND:
			if(const UI_ActionDataUnitCommand* p = safe_cast<const UI_ActionDataUnitCommand*>(action_data)){
				if(p->playerUnit())
					break;
				UnitCommand command = p->command();
				switch(command.commandID()){
				case COMMAND_ID_DIRECT_CONTROL:
					if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
						if((unit = unit->getUnitReal())->attr().isActing())
							controlState.setState((int)safe_cast<const UnitActing*>(unit)->isDirectControl());
						else
							controlState.setState(0);
					else
						controlState.setState(0);
					break;
				case COMMAND_ID_SYNDICAT_CONTROL:
					if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
						if((unit = unit->getUnitReal())->attr().isActing())
							controlState.setState((int)safe_cast<const UnitActing*>(unit)->isSyndicateControl());
						else
							controlState.setState(0);
					else
						controlState.setState(0);
					break;
				case COMMAND_ID_CHANGE_MOVEMENT_MODE:
					if(command.commandData() == MODE_RUN && !selectManager->canRun())
						controlState.disable();
					else
						controlState.enable();
					break;
				}
			}
			break;
		
		case UI_ACTION_WEAPON_STATE: {
			const UI_ActionDataWeaponRef* action = safe_cast<const UI_ActionDataWeaponRef*>(action_data);
			const UnitInterface* unit = (action->playerUnit() ?
				UI_LogicDispatcher::instance().player()->playerUnit()
				: UI_LogicDispatcher::instance().selectedUnit());
			if(unit && unit->attr().isActing())
				if(const WeaponPrm* prm =  action->weaponPrm()){
					const WeaponBase* weapon = safe_cast<const UnitActing*>(unit->getUnitReal())->findWeapon(prm->ID());
					if(weapon && weapon->isEnabled()){
						controlState.show();
						if(unit->canFire(prm->ID()))
							controlState.enable();
						else
							controlState.disable();
						if(action->showAutoFire())
							controlState.setState(weapon->isAutoFireOn() ? 1 : 0);
					}
					else
						controlState.hide();
				}
				else
					controlState.hide();
			break;
									 }
		case UI_ACTION_UNIT_ON_MAP:
			if(const Player* player = UI_LogicDispatcher::instance().player()){
				if(player->realUnits(safe_cast<const UI_ActionDataUnitOrBuildingRef*>(action_data)->attribute()).empty())
					controlState.hide();
				else
					controlState.show();
			}
			else
				controlState.hide();
			break;
		
		case UI_ACTION_SQUAD_ON_MAP:
			if(const Player* player = UI_LogicDispatcher::instance().player()){
				int size = player->squadList(safe_cast<const UI_ActionDataSquadRef*>(action_data)->attribute()).size();
				if(safe_cast<const UI_ActionDataSquadRef*>(action_data)->showCount())
					if(size > 1)
						setText((WBuffer() <= size).c_str());
					else
						setText(0);
				if(!size)
					controlState.hide();
				else
					controlState.show();
			}
			else
				controlState.hide();
			break;

		case UI_ACTION_UNIT_IN_TRANSPORT: {
			const UI_ActionDataUnitOrBuildingRef* dp = safe_cast<const UI_ActionDataUnitOrBuildingRef*>(action_data);
			if(selectManager->isInTransport(dp->attribute(), false))
				controlState.show();
			else
				controlState.hide();
										  }
			break;

		case UI_ACTION_MERGE_SQUADS:
			controlState.apply(selectManager->canMergeSquads());
			break;

		case UI_ACTION_SPLIT_SQUAD:
			if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
				if(!unit->attr().isBuilding()){
					if(selectManager->canSplitSquad())
						controlState.enable();
					else
						controlState.disable();

					controlState.show();
				}
				else
					controlState.hide();
			}
			else
				controlState.hide();
			break;

		case UI_BIND_PRODUCTION_QUEUE: {
			const UI_ActionDataProduction* action = safe_cast<const UI_ActionDataProduction*>(action_data);
			const UnitInterface* unit = (action->playerUnit() ?
				UI_LogicDispatcher::instance().player()->playerUnit()
				: UI_LogicDispatcher::instance().selectedUnit());
			if(unit && (unit = unit->getUnitReal())->attr().isActing()){
				const AttributeBase* producer = unit->selectionAttribute();
				if(producer->hasProdused()){
					ProducedQueue::const_iterator it;
					FOR_EACH(safe_cast<const UnitActing*>(unit)->producedQueue(), it){
						if(it->type_ == PRODUCE_RESOURCE && it->data_ == safe_cast<const UI_ActionDataProduction*>(action_data)->productionNumber()){
							controlState.hide();
							break;
						}
					}
					controlState.show();
				}
			}
			break;
									   }
		case UI_ACTION_DIRECT_CONTROL_CURSOR:
			if(!gameShell->underFullDirectControl()){
				controlState.hide();
				break;
			}
			if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
				if(cameraManager->directControlFreeCamera()){
					controlState.hide();
					break;
				}
				controlState.show();
				const UI_ActionDataDirectControlCursor* dp = safe_cast<const UI_ActionDataDirectControlCursor*>(action_data);
				float aim_distance = UI_LogicDispatcher::instance().aimDistance();

				if(const UnitInterface* target = UI_LogicDispatcher::instance().hoverUnit()){
					aim_distance = unit->position().distance(target->position());
					if(!unit->isEnemy(target)){
						bool is_transport = false;
						if(target->attr().isTransport()){
							UnitReal* unit_real = unit->getUnitReal();
							if(unit_real->attr().isActing()){
								const UnitActing* p = safe_cast<const UnitActing*>(target);
								if(p->canPutInTransport(unit_real) && 
									(p->position2D().distance2(unit_real->position2D()) < 
									sqr(p->radius() + unit_real->radius() + p->attr().transportLoadDirectControlRadius)))
									is_transport = true;
							}
						}

						controlState.setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, is_transport ? UI_DIRECT_CONTROL_CURSOR_TRANSPORT : UI_DIRECT_CONTROL_CURSOR_ALLY));
					}
					else
						controlState.setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, UI_DIRECT_CONTROL_CURSOR_ENEMY));
				}
				else {
					controlState.setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, UI_DIRECT_CONTROL_CURSOR_NONE));
				}

				UI_Transform trans;
				float scale = dp->cursorScale(aim_distance);
				trans.setScale(Vect2f(scale, scale));
				setPermanentTransform(trans);
			}
			else
				controlState.setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, UI_DIRECT_CONTROL_CURSOR_NONE));
			break;

		case UI_ACTION_DIRECT_CONTROL_TRANSPORT:
			controlState.hide();
			if(gameShell->underFullDirectControl()){
				if(UnitInterface* selected_unit = UI_LogicDispatcher::instance().selectedUnit()){
					UnitReal* selected_unit_real = selected_unit->getUnitReal();
					UnitInterface* ip = UI_LogicDispatcher::instance().hoverUnit();
					if(ip && ip->attr().isTransport() && ip->attr().isActing()){
						UnitActing* p = safe_cast<UnitActing*>(ip);
						if(p->canPutInTransport(selected_unit_real) && 
							(p->position2D().distance2(selected_unit_real->position2D()) < 
							sqr(p->radius() + selected_unit_real->radius() + p->attr().transportLoadDirectControlRadius))){
								controlState.show();
						}
					} else if(selected_unit_real->attr().isTransport()){
						controlState.show();
					}
				}
			}
			break;

		default:
			UI_LogicDispatcher::instance().controlUpdate(action_id, this, action_data, controlState);
	}
}

//----------------------------------------

void UI_ControlBase::action()
{
	MTG();

	actionActive_ = false;
	
	ActionModeModifer modifer = UI_LogicDispatcher::instance().modifiers();

	for(int i = 0; i < actionCount(); i++){
		const UI_ActionData* data = actionData(i);
		if(data->needExecute(actionFlags_, modifer))
			actionExecute(actionID(i), data);
	}

	if(const UI_ControlState* state = currentState())
		for(int i = 0; i < state->actionCount(); i++){
			const UI_ActionData* data = state->actionData(i);
			if(data->needExecute(actionFlags_, modifer))
				actionExecute(state->actionID(i), data);
		}
}

void UI_ControlBase::handleAction(const ControlMessage& msg)
{
	UI_ControlActionID ID = msg.id_;
	if(ID == UI_ACTION_CONTROL_COMMAND){
		const UI_ActionDataControlCommand* data = safe_cast<const UI_ActionDataControlCommand*>(msg.data_);
		for(int i = 0; i < actionCount(); i++)
			if(actionID(i) == data->target())
				switch(data->command()){
				case UI_ActionDataControlCommand::NONE:
					break;
				case UI_ActionDataControlCommand::RE_INIT:
					UI_Dispatcher::instance().lockUI();
					actionInit(actionID(i), actionData(i));
					UI_Dispatcher::instance().unlockUI();
					break;
				case UI_ActionDataControlCommand::EXECUTE:
					MTG();
					actionExecute(actionID(i), actionData(i));
					break;
				default:
					actionExecute(UI_ACTION_CONTROL_COMMAND, data);
				}
	
		if(const UI_ControlState* state = currentState())
			for(int i = 0; i < state->actionCount(); i++)
				if(state->actionID(i) == data->target())
					switch(data->command()){
						case UI_ActionDataControlCommand::NONE:
							break;
						case UI_ActionDataControlCommand::RE_INIT:
							UI_Dispatcher::instance().lockUI();
							actionInit(state->actionID(i), state->actionData(i));
							UI_Dispatcher::instance().unlockUI();
							break;
						case UI_ActionDataControlCommand::EXECUTE:
							MTG();
							actionExecute(state->actionID(i), state->actionData(i));
						default:
							actionExecute(UI_ACTION_CONTROL_COMMAND, data);
					}
	}
	else{
		for(int i = 0; i < actionCount(); i++)
			if(ID == actionID(i))
				if(ID == UI_ACTION_SET_KEYS)
					actionExecute(ID, &UI_ActionKeys(safe_cast<const UI_ActionKeys*>(msg.data_)->ActionType(), safe_cast<const UI_ActionKeys*>(actionData(i))->Option()));
				else
					actionExecute(ID, msg.data_);
	
		if(const UI_ControlState* state = currentState())
			for(int i = 0; i < state->actionCount(); i++)
				if(ID == state->actionID(i))
					if(ID == UI_ACTION_SET_KEYS)
						actionExecute(ID, &UI_ActionKeys(safe_cast<const UI_ActionKeys*>(msg.data_)->ActionType(), safe_cast<const UI_ActionKeys*>(state->actionData(i))->Option()));
					else
						actionExecute(ID, msg.data_);
	}

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->handleAction(msg);
}

void UI_ControlBase::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data)
{
	switch(action_id)
	{
		case UI_ACTION_CONTROL_COMMAND:
			if(const UI_ActionDataControlCommand* data = safe_cast<const UI_ActionDataControlCommand*>(action_data)){
				switch(data->command()){
					case UI_ActionDataControlCommand::CLEAR:
						setText(0);
						break;
					default:
						UI_LogicDispatcher::instance().controlAction(action_id, this, action_data);
				}
			}
			break;

		case UI_ACTION_ACTIVATE_WEAPON: {
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			const UI_ActionDataWeaponActivate* action = safe_cast<const UI_ActionDataWeaponActivate*>(action_data);
			if(action->weaponPrm()){
				UnitCommand command(COMMAND_ID_WEAPON_ACTIVATE, action->weaponPrm()->ID());
				command.setShiftModifier(action->fireOnce());
				if(action->playerUnit()){
					if(UnitActing* unit = UI_LogicDispatcher::instance().player()->playerUnit())
						unit->sendCommand(command);
				}
				else
					selectManager->makeCommand(command);
			}
			break;
										}
		case UI_ACTION_DEACTIVATE_WEAPON: {
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			const UI_ActionDataWeapon* action = safe_cast<const UI_ActionDataWeapon*>(action_data);
			if(action->weaponPrm()){
				UnitCommand command(COMMAND_ID_WEAPON_DEACTIVATE, action->weaponPrm()->ID());
				if(action->playerUnit()){
					if(UnitActing* unit = UI_LogicDispatcher::instance().player()->playerUnit())
						unit->sendCommand(command);
				}
				else
					selectManager->makeCommand(command);
			}
			break;
										}
		case UI_ACTION_UNIT_COMMAND:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActionDataUnitCommand* p = safe_cast<const UI_ActionDataUnitCommand*>(action_data)){
				int selectedSlot = -1;
				UnitInterface* commandUnit = 0;
				bool uniform = selectManager->uniform();
				UnitCommand command = p->command();

				command.setShiftModifier(UI_LogicDispatcher::instance().isMouseFlagSet(MK_SHIFT));
				
				if(p->playerUnit())
					commandUnit = UI_LogicDispatcher::instance().player()->playerUnit();
				else if(const UI_ControlUnitList* own = dynamic_cast<const UI_ControlUnitList*>(owner())){ // для списка юнитов нужно узнать индекс дочернего контрола
					xassert(std::find(own->controlList().begin(), own->controlList().end(), this) != own->controlList().end());
					int child_index = std::distance(own->controlList().begin(), std::find(own->controlList().begin(), own->controlList().end(), this));
					switch(own->GetType()){
					case UI_UNITLIST_SELECTED: // для списка выделенных нужно найти конкретного юнита
						selectedSlot = child_index;
						break;
					case UI_UNITLIST_PRODUCTION: // конкретизировать команду
					case UI_UNITLIST_TRANSPORT:					
						command = UnitCommand(command.commandID(), child_index);
						break;
					case UI_UNITLIST_SQUAD:
					case UI_UNITLIST_PRODUCTION_SQUAD:
					case UI_UNITLIST_PRODUCTION_COMPACT:
						break;
					case UI_UNITLIST_SQUADS_IN_WORLD: {
						const SquadList& squadList = UI_LogicDispatcher::instance().player()->squadList(own->squadAtrribute());
						if(child_index < squadList.size())
							commandUnit = squadList[child_index];
						break;
													  }
					default:
						xassert(0 && "новый тип списка селекта");
					}
					uniform = false;
				}
				else if(command.commandID() == COMMAND_ID_DIRECT_CONTROL){
					command = UnitCommand(command.commandID(), !gameShell->underFullDirectControl());
					uniform = false;
				}
				else if( command.commandID() == COMMAND_ID_SYNDICAT_CONTROL){
					command = UnitCommand(command.commandID(), !gameShell->underHalfDirectControl());
					uniform = false;
				}
				
				if(commandUnit)
					commandUnit->sendCommand(command);
				else if(selectedSlot >= 0)
					selectManager->sendCommandSlot(command, selectedSlot);
				else
					selectManager->makeCommand(command, !uniform && !p->sendForAll());
			}
			break;
		
		case UI_ACTION_BUILDING_INSTALLATION:
			if(const UI_ActionDataBuildingAction* p = safe_cast<const UI_ActionDataBuildingAction*>(action_data))
				UI_LogicDispatcher::instance().toggleBuildingInstaller(p->attribute());
			break;
		
		case UI_ACTION_UNIT_SELECT:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;

			if(const UI_ActionDataSelectUnit* p = safe_cast<const UI_ActionDataSelectUnit*>(action_data))
				if(const AttributeBase* attr = p->attribute() ? p->attribute() : p->squad())
					selectManager->selectAll(attr, p->onlyPowered());
			break;

		case UI_ACTION_SET_SELECTED:
			if(const UI_ControlUnitList* own = dynamic_cast<const UI_ControlUnitList*>(owner())){
				if(own->GetType() != UI_UNITLIST_SELECTED)
					break;

				xassert(std::find(own->controlList().begin(), own->controlList().end(), this) != own->controlList().end());
				selectManager->setSelectedObject(std::distance(own->controlList().begin(), std::find(own->controlList().begin(), own->controlList().end(), this)));
				
			}
			break;

		case UI_ACTION_SELECTION_OPERATE:
			if(const UI_ActionDataSelectionOperate* data = safe_cast<const UI_ActionDataSelectionOperate*>(action_data)){
				switch(data->command()){
				case UI_ActionDataSelectionOperate::LEAVE_TYPE_ONLY:
					selectManager->leaveOnlyType(data->selectonID());
					break;
				case UI_ActionDataSelectionOperate::LEAVE_SLOT_ONLY:
					selectManager->leaveOnlySlot(data->selectonID());
					break;
				case UI_ActionDataSelectionOperate::SAVE_SELECT:
					selectManager->saveSelection(data->selectonID());
					break;
				case UI_ActionDataSelectionOperate::RETORE_SELECT:
					selectManager->restoreSelection(data->selectonID());
					break;
				case UI_ActionDataSelectionOperate::ADD_SELECT:
					selectManager->addSelection(data->selectonID());
					break;
				case UI_ActionDataSelectionOperate::SUB_SELECT:
					selectManager->subSelection(data->selectonID());
					break;
				}
			}
			break;

		case UI_ACTION_UNIT_DESELECT:
			if(const UI_ControlUnitList* own = dynamic_cast<const UI_ControlUnitList*>(owner())){
				if(own->GetType() != UI_UNITLIST_SELECTED)
					break;

				xassert(std::find(own->controlList().begin(), own->controlList().end(), this) != own->controlList().end());
				selectManager->deselectObject(std::distance(own->controlList().begin(), std::find(own->controlList().begin(), own->controlList().end(), this)));

			}
			break;

		case UI_ACTION_CHANGE_STATE:
			if(const UI_ActionDataStateChange* p = safe_cast<const UI_ActionDataStateChange*>(action_data)){
				if(p->reverseDirection()){
					if(--currentStateIndex_ < 0)
						currentStateIndex_ = states_.size() - 1;
				}
				else {
					if(++currentStateIndex_ >= states_.size())
						currentStateIndex_ = 0;
				}
			}
			break;

		case UI_ACTION_MERGE_SQUADS:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;

			selectManager->squadsMerge();
			break;
	
		case UI_ACTION_SPLIT_SQUAD:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;

			selectManager->squadSplit();
			break;
		case UI_ACTION_EXTERNAL_CONTROL:
				for_each(	safe_cast<const UI_ActionExternalControl*>(action_data)->actions().begin(),
							safe_cast<const UI_ActionExternalControl*>(action_data)->actions().end(),
							mem_fun_ref(&AtomAction::apply));
				break;

		case UI_ACTION_DIRECT_CONTROL_TRANSPORT:
			if(gameShell->underFullDirectControl()){
				if(UnitInterface* selected_unit = UI_LogicDispatcher::instance().selectedUnit()){
					UnitReal* selected_unit_real = selected_unit->getUnitReal();
					UnitInterface* ip = UI_LogicDispatcher::instance().hoverUnit();
					if(ip && ip->attr().isTransport() && ip->attr().isActing()){
						UnitActing* p = safe_cast<UnitActing*>(ip);
						if((selected_unit_real->player() == p->player() || p->player()->isWorld()) && 
							p->canPutInTransport(selected_unit_real) && 
							(p->position2D().distance2(selected_unit_real->position2D()) < 
							sqr(p->radius() + selected_unit_real->radius() + p->attr().transportLoadDirectControlRadius)))
							selected_unit_real->sendCommand(UnitCommand(COMMAND_ID_DIRECT_PUT_IN_TRANSPORT, p));
					}else if(selected_unit_real->attr().isTransport())
						selected_unit_real->sendCommand(UnitCommand(COMMAND_ID_DIRECT_PUT_OUT_TRANSPORT));
				}
			}
			break;

		default:
			UI_LogicDispatcher::instance().controlAction(action_id, this, action_data);
	}
}


//----------------------------------------

bool UI_ControlBase::handleInput(const UI_InputEvent& event)
{
	bool ret = false;

	if(!isVisible())
		return false;

	if(event.isMouseEvent()){
		for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it){
			if((*it)->handleInput(event)){
				ret = true;
				break;
			}
		}
		if(!ret){
			if(hitTest(event.cursorPos())){
				if(isEnabled()){
					if(ret = inputEventHandler(event))
						UI_LogicDispatcher::instance().inputEventProcessed(event, this);
				}
				else
					ret = true;
			}
		}
	}
	else {
		if(isEnabled()){
			ret = inputEventHandler(event);
			UI_LogicDispatcher::instance().inputEventProcessed(event, this);
		}

		for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it){
			if((*it)->handleInput(event))
				ret = true;
		}
	}

	return ret;
}

bool UI_ControlBase::inputEventHandler(const UI_InputEvent& event)
{
	if(event.isMouseEvent()){
		xassert(event.ID() < sizeof(ActionMode) * 8);
		setInputActionFlag((ActionMode)(1 << event.ID()));
		return true;
	}
	return false;
}

//----------------------------------------

void UI_ControlBase::quant(float dt)
{
	MTG();

	LOG_UI_STATE(L"begin");

	if(isActive_){
		if(needDeactivation_)
			isActive_ = false;
	}
	else {
		if(isEnabled() && checkInputActionFlag(UI_USER_ACTION_MASK)){
			if(activate())
				actionRequest();
		}
	}

	if(textChanged_){
		applyNewText(newText_.c_str());
		textChanged_ = false;
	}

	if(!isDeactivating_ && !isBackgroundAnimationActive(UI_BackgroundAnimation::PLAY_STARTUP)){
		if(mouseHover_ && isEnabled()){
			if(mouseHoverPrev_){
				if(!isBackgroundAnimationActive(UI_BackgroundAnimation::PLAY_HOVER_STARTUP))
					startBackgroundAnimation(UI_BackgroundAnimation::PLAY_HOVER_PERMANENT);
			}
			else {
				startBackgroundAnimation(UI_BackgroundAnimation::PLAY_HOVER_STARTUP);
			}
		}
		else {
			if(mouseHoverPrev_){
				startBackgroundAnimation(UI_BackgroundAnimation::PLAY_HOVER_STARTUP, false, true);
			}
		}
	}

	mouseHoverPrev_ = mouseHover_;

	if(!backgroundAnimations_.empty()){
		if(isVisible() && !isDeactivating_ && animationPlayMode_ == UI_BackgroundAnimation::PLAY_STARTUP && !isBackgroundAnimationActive(UI_BackgroundAnimation::PLAY_STARTUP))
			startBackgroundAnimation(UI_BackgroundAnimation::PLAY_PERMANENT, true);
	}

	animationQuant(dt);

	if(isEnabled()){
		if(isActivated()){
			setShowMode(UI_SHOW_ACTIVATED);
		}
		else {
			if(mouseHover_)
				setShowMode(UI_SHOW_HIGHLITED);
			else
				setShowMode(UI_SHOW_NORMAL);
		}
		if(actionActive_)
			action();
	}
	else
		setShowMode(UI_SHOW_DISABLED);

	if(hasCoordLink_)
		adjastToLink();

	transformQuant(dt);

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->quant(dt);

	if(UI_LogicDispatcher::instance().gamePause())
		waitingUpdate_ = false;

	hoverToggle(false);
	actionFlags_ = 0;

	LOG_UI_STATE(L"end");
}

void UI_ControlBase::adjastToLink()
{
	if(!UI_LogicDispatcher::instance().isGameActive())
		return;

	for(int idx = 0; idx < actionCount(); ++idx){
		switch(actionID(idx)){
		case UI_ACTION_LINK_TO_MOUSE:{
			const UI_ActionDataLinkToMouse* linkAction = safe_cast<const UI_ActionDataLinkToMouse*>(actionData(idx));
			Vect2f pos(UI_LogicDispatcher::instance().mousePosition());
			pos += UI_Render::instance().relativeSize(linkAction->shift());
			Vect2f size(position_.width(), position_.height());
			pos.x = clamp(pos.x + size.x, size.x, 1.f);
			pos.y = clamp(pos.y + size.y, size.y, 1.f);
			pos -= size;
			setPosition(pos);
			return;
									 }
		case UI_ACTION_LINK_TO_ANCHOR:{
			const UI_ActionDataLinkToAnchor* linkAction = safe_cast<const UI_ActionDataLinkToAnchor*>(actionData(idx));
			const BaseUniverseObject* anchor = 0;
			if(linkAction->selected())
				anchor = UI_LogicDispatcher::instance().selectedUnitIfOne();
			else
				anchor = sourceManager->findAnchor(linkAction->link());
			if(anchor){
				Vect3f e, w;
				cameraManager->GetCamera()->ConvertorWorldToViewPort(&To3D(anchor->position()), &w, &e);
				if(w.z > 0){
					show();
					setPosition(UI_Render::instance().relativeCoords(e));
				}
				else
					hide(true);
			}
			return;
									  }
		case UI_ACTION_LINK_TO_PARENT:{
			const UI_ActionDataLinkToParent* linkAction = safe_cast<const UI_ActionDataLinkToParent*>(actionData(idx));
			if(const UI_ControlBase* parent = dynamic_cast<const UI_ControlBase*>(owner()))
				setPosition(parent->position().left_top() - linkAction->shift());
			return;
									  }
		}
	}
}

const AttributeBase* UI_ControlBase::actionUnit(const AttributeBase* selected) const
{
	const AttributeBase* unit = 0;

	if(const UI_ControlState* st = currentState())
		for(int i = 0; i < st->actionCount(); i++){
			switch(st->actionID(i)){
			case UI_ACTION_UNIT_SELECT:
				unit = safe_cast<const UI_ActionDataSelectUnit*>(st->actionData(i))->attribute();
				break;
			case UI_ACTION_BUILDING_INSTALLATION:
				unit = safe_cast<const UI_ActionDataBuildingAction*>(st->actionData(i))->attribute();
				break;
			case UI_ACTION_BUILDING_CAN_INSTALL:
				unit = safe_cast<const UI_ActionDataUnitOrBuildingUpdate*>(st->actionData(i))->attribute();
				break;
			case UI_ACTION_BIND_TO_UNIT_STATE:
				unit = safe_cast<const UI_ActionUnitState*>(st->actionData(i))->getAttribute(selected);
				break;
			case UI_ACTION_UNIT_COMMAND:
				unit = selected;
				break;
			}
			if(unit)
				return unit;
		}
	
	for(int i = 0; i < actionCount(); i++){
		switch(actionID(i)){
		case UI_ACTION_UNIT_SELECT:
			unit = safe_cast<const UI_ActionDataSelectUnit*>(actionData(i))->attribute();
			break;
		case UI_ACTION_BUILDING_INSTALLATION:
			unit = safe_cast<const UI_ActionDataBuildingAction*>(actionData(i))->attribute();
			break;
		case UI_ACTION_BUILDING_CAN_INSTALL:
			unit = safe_cast<const UI_ActionDataUnitOrBuildingUpdate*>(actionData(i))->attribute();
			break;
		case UI_ACTION_BIND_TO_UNIT_STATE:
			unit = safe_cast<const UI_ActionUnitState*>(actionData(i))->getAttribute(selected);
			break;
		case UI_ACTION_UNIT_COMMAND:
			unit = selected;
			break;
		}
		if(unit)
			return unit;
	}

	return 0;
	
}

bool UI_ControlBase::actionParameters(const AttributeBase* unit, ParameterSet& params) const
{
	if(const UI_ControlState* st = currentState())
		for(int i = 0; i < st->actionCount(); i++){
			switch(st->actionID(i)){
			case UI_ACTION_UNIT_COMMAND:{
				const UnitCommand& command = safe_cast<const UI_ActionDataUnitCommand*>(st->actionData(i))->command();
				switch(command.commandID()){
				case COMMAND_ID_UPGRADE:
					if(unit && unit->upgrades.exists(command.commandData())){
						params = unit->upgrades[command.commandData()].upgradeValue;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE:
					if(unit && unit->producedUnits.exists(command.commandData())){
						const AttributeBase::ProducedUnits& builds = unit->producedUnits[command.commandData()];
						params = builds.unit->installValue;
						params *= builds.number;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE_PARAMETER:
					if(unit && unit->producedParameters.exists(command.commandData())){
						params = unit->producedParameters[command.commandData()].cost;
						return true;
					}
					break;
				}
				break;
										}
			case UI_ACTION_WEAPON_STATE:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataWeaponRef*>(st->actionData(i))->weaponPrm()){
					wprm->getFireCostReal(params);
					return true;
				}
				break;
			case UI_ACTION_ACTIVATE_WEAPON:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataWeapon*>(st->actionData(i))->weaponPrm()){
					wprm->getFireCostReal(params);
					return true;
				}
				break;
			case UI_ACTION_CLICK_MODE:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataClickMode*>(st->actionData(i))->weaponPrm()){
					wprm->getFireCostReal(params);
					return true;
				}
				break;
			case UI_ACTION_BUILDING_INSTALLATION:
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataBuildingAction*>(st->actionData(i))->attribute()){
					params = attr->installValue;
					return true;
				}
				break;
			case UI_ACTION_BUILDING_CAN_INSTALL:
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataUnitOrBuildingUpdate*>(st->actionData(i))->attribute()){
					params = attr->installValue;
					return true;
				}
				break;
			}
		}

		for(int i = 0; i < actionCount(); i++){
			switch(actionID(i)){
			case UI_ACTION_UNIT_COMMAND:{
				const UnitCommand& command = safe_cast<const UI_ActionDataUnitCommand*>(actionData(i))->command();
				switch(command.commandID()){
				case COMMAND_ID_UPGRADE:
					if(unit && unit->upgrades.exists(command.commandData())){
						params = unit->upgrades[command.commandData()].upgradeValue;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE:
					if(unit && unit->producedUnits.exists(command.commandData())){
						const AttributeBase::ProducedUnits& builds = unit->producedUnits[command.commandData()];
						params = builds.unit->installValue;
						params *= builds.number;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE_PARAMETER:
					if(unit && unit->producedParameters.exists(command.commandData())){
						params = unit->producedParameters[command.commandData()].cost;
						return true;
					}
					break;
				}
				break;
										}
			case UI_ACTION_WEAPON_STATE:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataWeaponRef*>(actionData(i))->weaponPrm()){
					wprm->getFireCostReal(params);
					return true;
				}
				break;
			case UI_ACTION_ACTIVATE_WEAPON:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataWeapon*>(actionData(i))->weaponPrm()){
					wprm->getFireCostReal(params);
					return true;
				}
				break;
			case UI_ACTION_CLICK_MODE:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataClickMode*>(actionData(i))->weaponPrm()){
					wprm->getFireCostReal(params);
					return true;
				}
				break;
			case UI_ACTION_BUILDING_INSTALLATION:
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataBuildingAction*>(actionData(i))->attribute()){
					params = attr->installValue;
					return true;
				}
				break;
			case UI_ACTION_BUILDING_CAN_INSTALL:
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataUnitOrBuildingUpdate*>(actionData(i))->attribute()){
					params = attr->installValue;
					return true;
				}
				break;
			}
		}

		return false;
}

const WeaponPrm* UI_ControlBase::actionWeapon() const
{
	const WeaponPrm* weapon = 0;

	if(const UI_ControlState* st = currentState())
		for(int i = 0; i < st->actionCount(); i++){
			switch(st->actionID(i)){
			case UI_ACTION_WEAPON_STATE:
				weapon = safe_cast<const UI_ActionDataWeaponRef*>(st->actionData(i))->weaponPrm();
				break;
			case UI_ACTION_ACTIVATE_WEAPON:
				weapon = safe_cast<const UI_ActionDataWeapon*>(st->actionData(i))->weaponPrm();
				break;
			case UI_ACTION_CLICK_MODE:
				weapon = safe_cast<const UI_ActionDataClickMode*>(st->actionData(i))->weaponPrm();
				break;
			}
			if(weapon)
				return weapon;
		}

		for(int i = 0; i < actionCount(); i++){
			switch(actionID(i)){
			case UI_ACTION_WEAPON_STATE:
				weapon = safe_cast<const UI_ActionDataWeaponRef*>(actionData(i))->weaponPrm();
				break;
			case UI_ACTION_ACTIVATE_WEAPON:
				weapon = safe_cast<const UI_ActionDataWeapon*>(actionData(i))->weaponPrm();
				break;
			case UI_ACTION_CLICK_MODE:
				weapon = safe_cast<const UI_ActionDataClickMode*>(actionData(i))->weaponPrm();
				break;
			}
			if(weapon)
				return weapon;
		}

		return 0;
}

const ProducedParameters* UI_ControlBase::actionBuildParameter(const AttributeBase* unit) const
{
	if(const UI_ControlState* st = currentState())
		for(int i = 0; i < st->actionCount(); i++)
			switch(st->actionID(i)){
			case UI_ACTION_UNIT_COMMAND:{
				const UnitCommand& command = safe_cast<const UI_ActionDataUnitCommand*>(st->actionData(i))->command();
				switch(command.commandID()){
				case COMMAND_ID_PRODUCE_PARAMETER:
					if(unit->producedParameters.exists(command.commandData()))
						return &unit->producedParameters[command.commandData()];
					break;
				}
				break;
										}
			}

	for(int i = 0; i < actionCount(); i++){
		switch(actionID(i)){
		case UI_ACTION_UNIT_COMMAND:{
			const UnitCommand& command = safe_cast<const UI_ActionDataUnitCommand*>(actionData(i))->command();
			switch(command.commandID()){
			case COMMAND_ID_PRODUCE_PARAMETER:
				if(unit->producedParameters.exists(command.commandData()))
					return &unit->producedParameters[command.commandData()];
				break;
			}
			break;
								}
		}
	}
	return 0;
}

InterfaceGameControlID UI_ControlBase::controlID() const
{
	if(const UI_ActionData* data = findAction(UI_ACTION_SET_KEYS))
		return safe_cast<const UI_ActionKeys*>(data)->Option();

	return CTRL_MAX;
}

// ------------------- UI_ControlButton


void UI_ControlButton::textChanged()
{
	adjustSize();
}

void UI_ControlButton::actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data){
	switch(action_id){
	case UI_ACTION_AUTOCHANGE_STATE:
		setState(effectRND(states().size()));
		data_ = effectRND.fabsRnd(safe_cast<const UI_ActionAutochangeState*>(action_data)->time().minimum(), safe_cast<const UI_ActionAutochangeState*>(action_data)->time().maximum());
		break;
	case UI_ACTION_FIND_UNIT:
		safe_cast<const UI_ActionDataFindUnit*>(action_data)->init();
		break;
	default:
		UI_ControlBase::actionInit(action_id, action_data);
	}
}


void UI_ControlButton::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
	switch(action_id){
	case UI_ACTION_AUTOCHANGE_STATE:
		if((data_ -= logicPeriodSeconds) <= 0.f){
			int newState;
			while((newState = effectRND(states().size())) == currentStateIndex());
			controlState.setState(newState);
			data_ = effectRND.fabsRnd(safe_cast<const UI_ActionAutochangeState*>(action_data)->time().minimum(), safe_cast<const UI_ActionAutochangeState*>(action_data)->time().maximum());
		}
		break;

	case UI_ACTION_UNIT_FACE:
		if(const UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
			if(const UI_ShowModeSprite* sprite = unit->selectionAttribute()->getUnitFace(safe_cast<const UI_ActionDataUnitFace*>(action_data)->faceNum())){
				setSprites(*sprite);
				controlState.show();
			}
			else {
				setSprites(UI_ShowModeSprite::EMPTY);
				controlState.hide();
			}
		else {
			setSprites(UI_ShowModeSprite::EMPTY);
			controlState.hide();
		}
		break;

	default:
		UI_ControlBase::actionUpdate(action_id, action_data, controlState);
	}

}

void UI_ControlButton::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data){
	switch(action_id){
	case UI_ACTION_FIND_UNIT:{
		const UI_ActionDataFindUnit* data = safe_cast<const UI_ActionDataFindUnit*>(action_data);
		if(UnitObjective* unit = data->getUnit()){
			if(data->targeting())
				unit->sendCommand(UnitCommand(COMMAND_ID_CAMERA_FOCUS));
			if(data->select())
				selectManager->selectUnit(unit, UI_LogicDispatcher::instance().isMouseFlagSet(MK_SHIFT));
			}
		break;
							 }
	case UI_ACTION_SET_KEYS:{
		const UI_ActionKeys* action = safe_cast<const UI_ActionKeys*>(action_data);

		static bool inRecurse = false;
		if(inRecurse)
			break;
		inRecurse = true;

		switch(action->ActionType())
		{
		case UI_OPTION_APPLY:	{
			UI_ActionKeys msg(UI_OPTION_APPLY);
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_SET_KEYS, &msg));
			ControlManager::instance().saveLibrary();
								}
			break;
		
		case UI_OPTION_DEFAULT:	{
			UI_ActionKeys msg(UI_OPTION_DEFAULT);
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_SET_KEYS, &msg));
								}
			break;

		case UI_OPTION_CANCEL:	{
			UI_ActionKeys msg(UI_OPTION_CANCEL);
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_SET_KEYS, &msg));
								}
			break;
		}
		
		inRecurse = false;

							}
		break;
	default:
		UI_ControlBase::actionExecute(action_id, action_data);
	}
}

// ------------------- UI_ControlTextList

void UI_ControlTextList::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
	switch(action_id){	
	case UI_ACTION_MESSAGE_LIST:{
		const UI_ActionDataMessageList* action = safe_cast<const UI_ActionDataMessageList*>(action_data);
		if(UI_LogicDispatcher::instance().messagesEnabled() || action->ignoreMessageDisabling())
		{
			wstring str;
			if(UI_Dispatcher::instance().getMessageQueue(
					str,
					action->types(),
					action->reverse(),
					action->firstOnly()
				))
			{
				controlState.show();
				setText(str.c_str());
			}
			else {
				setText(0);
				controlState.hide();			
			}
		}
		else
			controlState.hide();
		break;
								}
	case UI_ACTION_TASK_LIST: {
		wstring str;
		if(UI_Dispatcher::instance().getTaskList(str, safe_cast<const UI_ActionDataTaskList*>(action_data)->reverse()))
			setText(str.c_str());
		else
			setText(0);
		break;
							 }
	default:
		UI_ControlBase::actionUpdate(action_id, action_data, controlState);
	}
}

bool UI_ControlTextList::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	switch(event.ID()){
		case UI_INPUT_MOUSE_WHEEL_UP:
			if(firstVisibleLine_ <= 0)
				break;
			firstVisibleLine_--;
			break;

		case UI_INPUT_MOUSE_WHEEL_DOWN:
			if(scrollSize() <= 0 || firstVisibleLine_ + 1 >= stringCount_)
				break;
			firstVisibleLine_++;
			break;
		
		default:
			return ret;

	}

	if(slider())
		slider()->setValue(index2sliderPhase(firstVisibleLine_));

	return true;
}

// ------------------- UI_ControlSlider

bool UI_ControlSlider::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	switch(event.ID()){
	case UI_INPUT_MOUSE_LBUTTON_DOWN:
		dragMode_ = true;
		ret = true;
		break;
	}

	return ret;
}

// ------------------- UI_ControlHotKeyInput

void UI_ControlHotKeyInput::quant(float dt)
{
	__super::quant(dt);

	WBuffer keyName;
	if(isVisible())
		setText(key_.toString(keyName));
}

bool UI_ControlHotKeyInput::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	switch(event.ID()) {
	case UI_INPUT_MOUSE_LBUTTON_DBLCLICK:
	case UI_INPUT_MOUSE_RBUTTON_DBLCLICK:
		if(!waitingInput_)
			saveKey_ = key_;
		done(sKey(addModifiersState(event.keyCode())), true);
		return true;
	}

	if(!waitingInput_)
		return ret;

	switch(event.ID()) {
	case UI_INPUT_MOUSE_MOVE:
	case UI_INPUT_MOUSE_LBUTTON_UP:
	case UI_INPUT_MOUSE_RBUTTON_UP:
	case UI_INPUT_MOUSE_MBUTTON_UP:
	case UI_INPUT_CHAR:
		return ret;
	case UI_INPUT_KEY_UP:
		if(key_ == event.keyCode())
			done(sKey(addModifiersState(event.keyCode())), true);
		else
			done(sKey(addModifiersState(0)), false);
		break;
	default:
		done(sKey(addModifiersState(event.keyCode())), false);
	}

	return true;
}

void UI_ControlHotKeyInput::actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data){
	switch(action_id){
	case UI_ACTION_SET_KEYS:{
		const UI_ActionKeys* action = safe_cast<const UI_ActionKeys*>(action_data);

		if(action->ActionType() == UI_OPTION_UPDATE){
			key_ = ControlManager::instance().key(action->Option());
			saveKey_ = key_;
		}
						  }
		break;
	default:
		UI_ControlBase::actionInit(action_id, action_data);
	}
}

void UI_ControlHotKeyInput::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data){
	switch(action_id){
	case UI_ACTION_SET_KEYS:{
		const UI_ActionKeys* action = safe_cast<const UI_ActionKeys*>(action_data);

		switch(action->ActionType()){
			case UI_OPTION_APPLY:
				if(ControlManager::instance().checkHotKey(key_.fullkey))
					ControlManager::instance().setKey(action->Option(), key_);
				else
					key_ = saveKey_;
				break;
			
			case UI_OPTION_DEFAULT:
				key_ = ControlManager::instance().defaultKey(action->Option());
				saveKey_ = key_;
				break;

			case UI_OPTION_CANCEL:
				actionInit(UI_ACTION_SET_KEYS, &UI_ActionKeys(UI_OPTION_UPDATE, action->Option()));
				break;
		}
						  }
		break;
	default:
		UI_ControlBase::actionExecute(action_id, action_data);
	}
}

// ------------------- UI_ControlEdit

void UI_ControlEdit::quant(float dt)
{
	UI_ControlBase::quant(dt);

	if(isEditing_){
		caretTimer_ -= dt;

		if(caretTimer_ <= 0.f){
			caretVisible_ = !caretVisible_;
			caretTimer_ = 0.5f; // Период мигания курсора
		}
	}
}


bool UI_ControlEdit::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	switch(event.ID()){
	case UI_INPUT_KEY_DOWN:
		ret = editInput(event.keyCode());
		break;
	case UI_INPUT_CHAR:
		ret = editCharInput(event.charInput());
		break;
	}

	return ret;
}


// ------------------- UI_ControlStringList

bool UI_ControlStringList::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	switch(event.ID()){
		case UI_INPUT_MOUSE_LBUTTON_DOWN:
		case UI_INPUT_MOUSE_RBUTTON_DOWN:
			if(UI_ControlSlider* p = slider()){
				if(p->dragMode())
					break;
				firstVisibleString_ = sliderPhase2Index(p->value());
			}

			ret = true;
			toggleString(clamp(firstVisibleString_ + floor((UI_LogicDispatcher::instance().mousePosition().y - textPosition().top()) / stringHeight_), -1, (int)strings_.size() - 1));
			break;
		
		case UI_INPUT_MOUSE_MOVE:
			if(UI_ControlSlider* p = slider()){
				if(p->dragMode())
					break;
				firstVisibleString_ = sliderPhase2Index(p->value());
			}

			ret = true;
			break;
		
		case UI_INPUT_MOUSE_WHEEL_UP:
			if(firstVisibleString_ <= 0)
				break;
			firstVisibleString_--;
			
			ret = true;
			break;
		
		case UI_INPUT_MOUSE_WHEEL_DOWN:
			if(firstVisibleString_ > (int)strings_.size() - round(textPosition().height() / stringHeight_))
				break;
			firstVisibleString_++;

			ret = true;
			break;

	}

	if(slider())
		slider()->setValue(index2sliderPhase(firstVisibleString_));

	return ret;
}

// ------------------- UI_ControlProgressBar

void UI_ControlProgressBar::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
	switch(action_id){	
		case UI_ACTION_PRODUCTION_PROGRESS:
		case UI_ACTION_PRODUCTION_PARAMETER_PROGRESS:
			if(UI_LogicDispatcher::instance().isGameActive()){
				const UI_ActionDataProduction* action = safe_cast<const UI_ActionDataProduction*>(action_data);
				UnitInterface* unit = (action->playerUnit() ?
					UI_LogicDispatcher::instance().player()->playerUnit()
					: UI_LogicDispatcher::instance().selectedUnit());
				if(unit && (unit = unit->getUnitReal())->attr().isActing()){
					const UnitActing* produser = safe_cast<const UnitActing*>(unit);
					const ProducedQueue& products = produser->producedQueue();
					if(!products.empty()){
						const ProduceItem& factory = *products.begin();
						if(action->productionNumber() == -1 || factory.data_ == action->productionNumber())
							setProgress(action_id == UI_ACTION_PRODUCTION_PROGRESS ? produser->productionProgress() : produser->productionParameterProgress());
						else
							setProgress(0);
					}
					else
						setProgress(0);
				}
				else
					setProgress(0);
			}
			break;
		case UI_ACTION_PRODUCTION_CURRENT_PROGRESS:
			if(UI_LogicDispatcher::instance().isGameActive()){
				if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
					if((unit = unit->getUnitReal())->attr().isActing())
						setProgress(safe_cast<const UnitActing*>(unit)->currentProductionProgress());
					else
						setProgress(0);
				else
					setProgress(0);
			}
			break;
		case UI_ACTION_RELOAD_PROGRESS:
			if(UI_LogicDispatcher::instance().isGameActive()){
				const UI_ActionDataWeaponReload* action = safe_cast<const UI_ActionDataWeaponReload*>(action_data);
				const UnitInterface* unit = (action->playerUnit() ?
					UI_LogicDispatcher::instance().player()->playerUnit()
					: UI_LogicDispatcher::instance().selectedUnit());
				if(unit && (unit = unit->getUnitReal())->attr().isActing()){
					const WeaponPrm* weapon = action->weaponPrm();
					if(!weapon || safe_cast<const UnitActing*>(unit)->hasWeapon(weapon->ID()))
					{
						setProgress(unit->weaponChargeLevel(weapon ? weapon->ID() : 0));

						if(action->ShowNotCharged())
							if(progress_ < 1.f - FLT_EPS)
								controlState.show();
							else
								controlState.hide();
					}
				}
				else
					setProgress(0);
			}
			break;
		case UI_ACTION_DIRECT_CONTROL_WEAPON_LOAD:{
			if(!gameShell->underFullDirectControl()){
				controlState.hide();
				break;
			}
			UnitInterface* p = UI_LogicDispatcher::instance().selectedUnit();
			if(p && (p = p->getUnitReal())->attr().isActing()){
				if(cameraManager->directControlFreeCamera()){
					controlState.hide();
					break;
				}
				controlState.show();
				UnitActing* unit = safe_cast<UnitActing*>(p);
				const UI_ActionDataDirectControlWeaponLoad* dp = safe_cast<const UI_ActionDataDirectControlWeaponLoad*>(action_data);
				float weapon_load = 0.f;
				int weapon_count = 0;

				WeaponDirectControlMode mode = dp->weaponType() == UI_ActionDataDirectControlWeaponLoad::WEAPON_PRIMARY ? WEAPON_DIRECT_CONTROL_NORMAL : WEAPON_DIRECT_CONTROL_ALTERNATE;

				UnitActing::WeaponSlots::const_iterator it;
				switch(dp->weaponType()){
				case UI_ActionDataDirectControlWeaponLoad::WEAPON_PRIMARY:
					FOR_EACH(unit->weaponSlots(), it){
						if(it->weapon()->weaponPrm()->directControlMode() == WEAPON_DIRECT_CONTROL_NORMAL){
							weapon_load += it->weapon()->chargeLevel();
							weapon_count++;
						}
					}
					break;
				case UI_ActionDataDirectControlWeaponLoad::WEAPON_SECONDARY:
					if(const WeaponBase* weapon = unit->findWeapon(unit->directControlWeaponID())){
						weapon_load += weapon->chargeLevel();
						weapon_count++;
					}
					break;
				}

				if(weapon_count)
					weapon_load /= float(weapon_count);

				setProgress(weapon_load);
			}
			break;
												  }
		default:
			UI_ControlBase::actionUpdate(action_id, action_data, controlState);
	}

	if(show_only_not_empty_)
		if(progress_ > FLT_EPS)
			controlState.show();
		else
			controlState.hide();

}


// ------------------- UI_ControlCustom

bool UI_ControlCustom::redraw() const
{
	if(!UI_ControlBase::redraw())
		return false;

	if(checkType(UI_CUSTOM_CONTROL_HEAD)){
		Rectf rc = UI_Render::instance().relative2deviceCoords(transfPosition()) + Vect2f(0.5f, 0.5f);
		showHead().SetWndPos(sRectangle4f(rc.left(), rc.top(), rc.right(), rc.bottom()));
		if(showHead().IsPlay()){
			showHead().ToggleDraw(true);
			headDelayTimer_.start(300);
			return true;
		}
		else if(headDelayTimer_.busy()){
			showHead().ToggleDraw(true);
			return true;
		}
	}
	
	if(checkType(UI_CUSTOM_CONTROL_SELECTION) && !UI_UnitView::instance().isEmpty()){
		UI_UnitView::instance().setPosition(transfPosition());
		UI_UnitView::instance().toggleDraw(true);
		return true;
	}
	else if(checkType(UI_CUSTOM_CONTROL_MINIMAP)){
		minimap().setPosition(transfPosition());
		if(useSelectedMap_)
			if(const MissionDescription* mission = UI_LogicDispatcher::instance().currentMission())
				minimap().init(mission->worldSize(), mission->worldData("map.tga").c_str());
			else
				minimap().releaseMapTexture();

		minimap().redraw(mapAlpha_ * alpha());
		return true;
	}

	return false;
}

void UI_ControlCustom::controlInit()
{
	if(checkType(UI_CUSTOM_CONTROL_MINIMAP)){
		if(useSelectedMap_){
			if(const MissionDescription* mission = UI_LogicDispatcher::instance().currentMission())
				minimap().init(mission->worldSize(), mission->worldData("map.tga").c_str());
			else
				minimap().releaseMapTexture();
		}
		else {
			minimap().init(Vect2f(vMap.H_SIZE, vMap.V_SIZE), vMap.getTargetName("map.tga"));
			minimap().waterColor(environment->minimapWaterColor());
		}
		minimap()
			.water(!useSelectedMap_)
			.drawEvents(!useSelectedMap_)
			.viewZone(drawViewZone_)
			.zoneColor(viewZoneColor_)
			.borderColor(miniMapBorderColor_)
			.canFog(drawFogOfWar_)
			.showZones(drawInstallZones_)
			.wind(drawWindDirection_)
			.rotate(rotateByCamera_)
			.toggleRotateByCamera(rotateByCameraInitial_)
			.rotScale(rotationScale_)
			.align(mapAlign_)
			.scale(scaleMinimap_)
			.drag(dragMinimap_)
			.toSelect(minimapToSelect_)
			.userScale(minimapScale_)
			.startLocations(viewStartLocations_)
			.font(font_)
			.mask(maskTexture_ ? maskTexture_->texture() : 0)
			.setRotate((getAngleFromWorld_ && environment ? universe()->minimapAngle() : minimapAngle_) / 180.f * M_PI);
	}
	
	UI_ControlBase::controlInit();
}

void UI_ControlCustom::sendToWorld(UI_InputEvent event, const Vect2f& mousePos) const
{
	event.setFlag(UI_InputEvent::BY_MINIMAP);

	UI_LogicDispatcher::instance().saveCursorPositionInfo();
	UI_LogicDispatcher::instance().setHoverPosition(To3D(minimap().minimap2world(mousePos)), true);

	minimap().checkEvent(EventMinimapClick(Event::UI_MINIMAP_ACTION_CLICK, mousePos));
	UI_Dispatcher::instance().handleInput(event);

	UI_LogicDispatcher::instance().restoreCursorPositionInfo();
}

bool UI_ControlCustom::inputEventHandler(const UI_InputEvent& ev)
{
	if(ev.isFlag(UI_InputEvent::BY_MINIMAP))
		return true;

	bool ret = __super::inputEventHandler(ev);

	if(!clickAction_)
		return ret;

	if(checkType(UI_CUSTOM_CONTROL_HEAD) && showHead().IsPlay())
		return ret;

	const ControlManager& man = ControlManager::instance();
	int code = ev.keyCode();

	if(man.key(CTRL_CAMERA_MOUSE_LOOK).check(code) || man.key(CTRL_CAMERA_MAP_SHIFT).check(code))
		return ret;

	bool isClickActionKey = man.key(CTRL_CLICK_ACTION).check(code);

	if(checkType(UI_CUSTOM_CONTROL_SELECTION) && !UI_UnitView::instance().isEmpty()){
		if(isClickActionKey || ev.isMouseClickEvent()){
			selectManager->makeCommand(UnitCommand(COMMAND_ID_CAMERA_FOCUS), true);
			return true;
		}
	}
	else if(checkType(UI_CUSTOM_CONTROL_MINIMAP)){
		const WeaponPrm* wprm = UI_LogicDispatcher::instance().selectedWeapon();
		if(wprm && wprm->weaponClass() == WeaponPrm::WEAPON_PAD)
			return false;
		Vect2f mousePos = UI_LogicDispatcher::instance().mousePosition();
		if(ev.isMouseEvent() || hitTest(mousePos)){
			if(ev.ID() == UI_INPUT_MOUSE_MOVE && ev.isFlag(MK_RBUTTON | MK_MBUTTON))
				minimap().drag(gameShell->mousePositionDelta());
			else if(minimap().hitTest(mousePos)){
			
				bool isMouseDrag = ev.ID() == UI_INPUT_MOUSE_MOVE && ev.isFlag(MK_LBUTTON);
				bool isMouseClick = ev.ID() == UI_INPUT_MOUSE_LBUTTON_DOWN;
				bool isContextCommand = man.key(CTRL_ATTACK).check(code) || man.key(CTRL_MOVE).check(code);
				
				if(ev.ID() == UI_INPUT_MOUSE_WHEEL_UP)
					minimap().zoomIn(mousePos);
				else if(ev.ID() == UI_INPUT_MOUSE_WHEEL_DOWN)
					minimap().zoomOut(mousePos);

				if(selectManager->isSelectionEmpty()){
					if(UI_LogicDispatcher::instance().clickMode() == UI_CLICK_MODE_ASSEMBLY){
						if(isMouseClick)
							sendToWorld(ev, mousePos);
					}
					else if(isMouseClick || isMouseDrag)
						minimap().pressEvent(mousePos);
				}
				else if(isMouseDrag)
					minimap().pressEvent(mousePos);
				else if(UI_LogicDispatcher::instance().clickMode() != UI_CLICK_MODE_NONE ? isClickActionKey : isContextCommand)
					sendToWorld(ev, mousePos);
				else if(isMouseClick)
					minimap().pressEvent(mousePos);
				else
					return ret;
				return true;
			}
		}
	}

	return ret;
}

// ------------------- UI_ControlUnitList

void UI_ControlUnitList::controlUpdate(ControlState& cs){
	start_timer_auto();
	// работаем так:
	// есть список дочерних контролов (простых кнопок) по числу слотов доступных для отображения юнитов
	// заполняем их текстурами асоциированными с юнитами, которые сейчас в selectManager, остальные чистим
	// отождествление с юнитом такое: индекс контрола в списке дочерних соответствует индексу юнита в selectManager
	if(!UI_LogicDispatcher::instance().isGameActive())
		return;

	const UI_ControlContainer::ControlList& ctrls = controlList();
	UI_ControlContainer::ControlList::const_iterator ctrl_it;

	bool need_clear_unit_list = false;

	switch(GetType()){
	case UI_UNITLIST_SELECTED:{
		if(!selectManager) break;
		SelectManager::Slots selection;
		selectManager->getSelectList(selection);
		
		int activeSlot = selectManager->selectedSlot();
		int idx = 0;
		
		SelectManager::Slots::iterator slot_it = selection.begin();
		FOR_EACH(ctrls, ctrl_it)
			if(slot_it != selection.end()){
				if(slot_it->units.size() > 1)
					setSprites(*ctrl_it, slot_it->attr());
				else
					setSprites(*ctrl_it, slot_it->units.front()->getUnitReal());
				
				int size = slot_it->units.size() > 1
					? slot_it->units.size()
					: (slot_it->units.front()->getSquadPoint() ? slot_it->units.front()->getSquadPoint()->unitsNumber() : 1);
				
				if(size > 1){
					WBuffer buf;
					buf < (slot_it->uniform() ? L"" : L"M") <= size;
					(*ctrl_it)->setText(buf.c_str());
				}
				else
					(*ctrl_it)->setText(0);

				if(idx == activeSlot)
					(*ctrl_it)->setPermanentTransform(activeTransform_);
				else
					(*ctrl_it)->setPermanentTransform(UI_Transform::ID);

				++slot_it;
				++idx;
			}
			else {
				setSprites(*ctrl_it);
				(*ctrl_it)->setPermanentTransform(UI_Transform::ID);
			}

		break;
							  }
	case UI_UNITLIST_SQUAD: {
		if(!selectManager) break;
		if(UnitInterface* unit = selectManager->getUnitIfOne()){
			if(const UnitSquad* squad = unit->getSquadPoint()){
				const LegionariesLinks& selection = squad->units();
				const UnitBase* activeSlot = squad->getUnitReal();

				LegionariesLinks::const_iterator slot_it = selection.begin();
				FOR_EACH(ctrls, ctrl_it)
					if(slot_it != selection.end()){
						setSprites(*ctrl_it, *slot_it);

						if(*slot_it == activeSlot)
							(*ctrl_it)->setPermanentTransform(activeTransform_);
						else
							(*ctrl_it)->setPermanentTransform(UI_Transform::ID);

						++slot_it;
					}
					else {
						setSprites(*ctrl_it);
						(*ctrl_it)->setPermanentTransform(UI_Transform::ID);
					}
			} else need_clear_unit_list = true;
		} else need_clear_unit_list = true;
		break;
							}
	case UI_UNITLIST_PRODUCTION:{
		if(!selectManager) break;
		UnitInterface* unit = selectManager->getUnitIfOne();
		if(unit && (unit = unit->getUnitReal())->attr().isActing()){
			const AttributeBase* producer = unit->selectionAttribute();
			if(producer->hasProdused()){
				
				const ProducedQueue& products = safe_cast<const UnitActing*>(unit)->producedQueue();
				ProducedQueue::const_iterator pr_it = products.begin();
				
				FOR_EACH(ctrls, ctrl_it){
					UI_ControlBase* ctrl = *ctrl_it;
	
					if(pr_it != products.end()){
						if(pr_it->type_ == PRODUCE_UNIT){
							const AttributeBase* attribute = producer->producedUnits[pr_it->data_].unit;
							setSprites(ctrl, attribute);
						}
						else if(pr_it->type_ == PRODUCE_RESOURCE){
							const UI_ShowModeSpriteReference& sprites = producer->producedParameters[pr_it->data_].sprites_;
							ctrl->setSprites(*sprites);
							ctrl->show();
						}
						else
							setSprites(ctrl);
						++pr_it;
					} else
						setSprites(ctrl);

				}
			} else need_clear_unit_list = true;
		} else need_clear_unit_list = true;
		break;
								}
	case UI_UNITLIST_PRODUCTION_COMPACT:{
		if(!selectManager) break;
		UnitInterface* unit = selectManager->getUnitIfOne();
		if(unit && (unit = unit->getUnitReal())->attr().isActing()){
			const AttributeBase* producer = unit->selectionAttribute();
			if(producer->hasProdused()){

				const ProducedQueue& products = safe_cast<const UnitActing*>(unit->getUnitReal())->producedQueue();
				ProducedQueue::const_iterator pr_it;

				int size = products.size();
				//int size = 0;
				//FOR_EACH(products, pr_it)
				//	if(pr_it->type_ == PRODUCE_UNIT)
				//		size += producer->producedUnits[pr_it->data_].number;
				//	else if(pr_it->type_ == PRODUCE_RESOURCE)
				//		++size;
					
				WBuffer size_str;
				if(size > 1)
					size_str <= size;

				pr_it = products.begin();
				FOR_EACH(ctrls, ctrl_it){
					UI_ControlBase* ctrl = *ctrl_it;

					if(pr_it != products.end() && pr_it == products.begin()){
						if(pr_it->type_ == PRODUCE_UNIT){
							ctrl->setText(size_str);
							const AttributeBase* attribute = producer->producedUnits[pr_it->data_].unit;
							setSprites(ctrl, attribute);
						}
						else if(pr_it->type_ == PRODUCE_RESOURCE){
							ctrl->setText(size_str);
							const UI_ShowModeSpriteReference& sprites = producer->producedParameters[pr_it->data_].sprites_;
							ctrl->setSprites(*sprites);
							ctrl->show();
						}
						else {
							setSprites(ctrl);
						}
						++pr_it;
					}
					else {
						setSprites(ctrl);
					}

				}
			} else need_clear_unit_list = true;
		} else need_clear_unit_list = true;
		break;
										}
	case UI_UNITLIST_TRANSPORT:{
		if(!selectManager) break;
		UnitInterface* unit = selectManager->getUnitIfOne();
		if(unit && (unit = unit->getUnitReal())->attr().isActing()){
			if(unit->selectionAttribute()->isTransport()){
				
				const LegionariesLinks& slots = safe_cast<UnitActing*>(unit)->transportSlots();
				LegionariesLinks::const_iterator slot_it = slots.begin();
				
				FOR_EACH(ctrls, ctrl_it){
					UI_ControlBase* ctrl = *ctrl_it;
					
					if(slot_it != slots.end()){
						UnitInterface* un = *slot_it;
						
						if(un && un->isDocked())
							setSprites(ctrl, un->selectionAttribute());
						else
							setSprites(ctrl);
						
						++slot_it;
					
					} else
						setSprites(ctrl);

				}
			} else need_clear_unit_list = true;
		}  else need_clear_unit_list = true;
		break;
							   }
	case UI_UNITLIST_PRODUCTION_SQUAD:
		if(!selectManager) break;
		if(UnitInterface* unit = selectManager->getUnitIfOne()){
			if(!unit->attr().isSquad()){
				need_clear_unit_list = true;
				break;
			}
			const UnitSquad::RequestedUnits& products = unit->getSquadPoint()->requestedUnits();
				
			UnitSquad::RequestedUnits::const_iterator pr_it = products.begin();
			
			FOR_EACH(ctrls, ctrl_it){
				UI_ControlBase* ctrl = *ctrl_it;
				
				if(pr_it != products.end()){
					
					const AttributeBase* attribute = pr_it->unit();
					setSprites(ctrl, attribute);
					
					++pr_it;
				
				} else
					setSprites(ctrl);

			}
		} else need_clear_unit_list = true;
		break;
	
	case UI_UNITLIST_SQUADS_IN_WORLD:
		if(const Player* player = UI_LogicDispatcher::instance().player()){
			const SquadList& squadList = player->squadList(&*squadRef_);
			if(!squadList.empty()){
				SquadList::const_iterator squad_it = squadList.begin();
				FOR_EACH(ctrls, ctrl_it){
					if(squad_it != squadList.end()){
						setSprites(*ctrl_it, *squad_it);

						int size = (*squad_it)->unitsNumber();
						if(size > 1)
							(*ctrl_it)->setText((WBuffer() <= size).c_str());
						else
							(*ctrl_it)->setText(0);

						++squad_it;
					}
					else
						setSprites(*ctrl_it);
				}
			}  else need_clear_unit_list = true;
		} else need_clear_unit_list = true;
		break;
	
	default:
		need_clear_unit_list = true;
		break;
	}

	if(need_clear_unit_list)
		FOR_EACH(ctrls, ctrl_it)
			setSprites(*ctrl_it);

	UI_ControlBase::controlUpdate(cs);
}

void UI_ControlUnitList::setSprites(UI_ControlBase* control, const AttributeBase* attr)
{
	xassert(control);
	if(!attr){
		control->hide(true);
		control->setText(0);
		control->setSprites(UI_ShowModeSprite::EMPTY);
	}
	else{
		if(const UI_ShowModeSprite* sprites = getSprite(attr))
			control->setSprites(*sprites);
		else if(const UI_ShowModeSprite* sprites = attr->getSelectSprite())
			control->setSprites(*sprites);
		else
			control->setSprites(defSprite_);
		control->show();
	}
}

void UI_ControlUnitList::setSprites(UI_ControlBase* control, const UnitInterface* unit)
{
	xassert(unit);
	if(const UI_ShowModeSprite* sprites = getSprite(&unit->getUnitReal()->attr()))
		control->setSprites(*sprites);
	else if(const UI_ShowModeSprite* sprites = unit->getSelectSprite())
		control->setSprites(*sprites);
	else 
		control->setSprites(defSprite_);
	control->show();
}

// ------------------- UI_ControlCustomList

void UI_ControlCustomList::controlUpdate(ControlState& cs)
{
	if(controlList().size() < 2)
		return;

	Nodes preNodes = nodes_;

	Nodes::iterator it;
	FOR_EACH(nodes_, it)
		it->cached = true;

	long clicked = InterlockedExchange(&activated_, -1);
	if(clicked >= (int)nodes_.size())
		clicked = -1;
	
	if(clicked >= 0 && !showOnHover_){
		if(nodes_[clicked].open)
			nodes_[clicked].open = false;
		else {
			FOR_EACH(nodes_, it)
				it->open = false;
			nodes_[clicked].open = true;
		}
	}

	UI_Tasks newTasks;
	const UI_Tasks& tasks = UI_Dispatcher::instance().tasks();
	UI_Tasks::const_iterator tit;
	FOR_EACH(tasks, tit){
		if(tit->state() != UI_TASK_ASSIGNED)
			continue;
		it = find(nodes_.begin(), nodes_.end(), *tit);
		if(it != nodes_.end())
			it->cached = false;
		else
			newTasks.push_back(*tit);
	}

	UI_Messages newMessages;
	const UI_Messages& messages = UI_Dispatcher::instance().messages();
	UI_Messages::const_iterator mit;
	FOR_EACH(messages, mit){
		if(find(messageTypes_.begin(), messageTypes_.end(), mit->type()) == messageTypes_.end())
			continue;
		if(!mit->messageSetup().hasText())
			continue;
		it = find(nodes_.begin(), nodes_.end(), *mit);
		if(it != nodes_.end())
			it->cached = false;
		else
			newMessages.push_back(*mit);
	}

	if(deleteOld_){
		if(autoDelete_){
			it = nodes_.begin();
			while(it != nodes_.end())
				if(it->cached)
					it = nodes_.erase(it);
				else
					++it;
		}
		else if(clicked >= 0){
			if(nodes_[clicked].cached)
				nodes_.erase(nodes_.begin() + clicked);
		}
	}

	Nodes::const_iterator old = preNodes.begin();
	int startUpdate = 0;
	for(; startUpdate < nodes_.size(); ++startUpdate){
		if(old == preNodes.end() || nodes_[startUpdate] != *old)
			break;
		++old;
	}
	
	bool reposition = false;
	if(clicked <= startUpdate)
		reposition = true;

	nodes_.insert(nodes_.end(), newTasks.begin(), newTasks.end());
	nodes_.insert(nodes_.end(), newMessages.begin(), newMessages.end());

	const UI_ControlBase* hovered = UI_LogicDispatcher::instance().hoverControl();

	it = nodes_.begin();

	int nodeIdx = -1;
	bool hideHint = true;
	UI_ControlContainer::ControlList::const_iterator ctrl_it = controlList().begin() + 1;
	for(;ctrl_it != controlList().end(); ++ctrl_it){
		if(it != nodes_.end()){
			++nodeIdx;
			if(nodeIdx >= startUpdate){
				const UI_ShowModeSprite* sprites = 0;
				if(it->isTask)
					if(it->task.isSecondary())
						if(it->cached)
							sprites = &completedSecTask_;
						else
							sprites = &activeSecTask_;
					else
						if(it->cached)
							sprites = &completedTask_;
						else
							sprites = &activeTask_;
				else
					if(it->cached)
						sprites = &oldMessage_;
					else
						sprites = &activeMessage_;
				(*ctrl_it)->setSprites(*sprites);
			}
			(*ctrl_it)->show();
			if(showOnHover_ && ctrl_it->get() == hovered || !showOnHover_ && it->open){
				if(nodeIdx != lastNode_)
					reposition = true;
				lastNode_ = nodeIdx;
				
				wstring text;
				if(it->isTask ? it->task.getText(text, true) : it->message.getText(text)){
					while(!text.empty() && *(text.end() - 1) == L'\n')
						text.pop_back();
					UI_LogicDispatcher::instance().expandTextTemplate(text, ExpandInfo(ExpandInfo::MESSAGE));
				}
				if(text.empty())
					text = L"NO TEXT";
				
				Vect2f text_size = UI_Render::instance().textSize(text.c_str(), hint()->font(), hint()->originalPosition().width() * hint()->relativeTextPosition().width());
				text_size.x /= hint()->relativeTextPosition().width(); 
				text_size.y /= hint()->relativeTextPosition().height();
				if(!text_size.eq(hint()->position().size()))
					reposition = true;
				
				hint()->setText(text.c_str());

				if(reposition){
					Rectf pos(text_size);
					pos.center((*ctrl_it)->position().center());
					switch(hintSide_){
					case RIGHT:
						pos.left(clamp(position().right(), 0.f, 1.f));
						break;
					case LEFT:
						pos.right(clamp(position().left(), 0.f, 1.f));
						break;
					case TOP:
						pos.bottom(clamp(position().top(), 0.f, 1.f));
						break;
					case BOTTOM:
						pos.top(clamp(position().bottom(), 0.f, 1.f));
						break;
					}
					hintPosition_ = pos;
				}

				hideHint = false;
			}
			++it;
		}
		else 
			(*ctrl_it)->hide();
	}
	if(hideHint){
		lastNode_ = -1;
		hint()->hide();
		hint()->setText(0);
	}
}

void UI_ControlCustomList::quant(float dt)
{
	if(controlList().size() < 2){
		__super::quant(dt);
		return;
	}

	long idx = -1;
	UI_ControlContainer::ControlList::const_iterator ctrl_it = controlList().begin() + 1;
	while(ctrl_it != controlList().end()){
		if((*ctrl_it)->isActivated()){
			idx = distance(controlList().begin(), ctrl_it) - 1;
			break;
		}
		++ctrl_it;
	}
	if(idx >= 0)
		InterlockedExchange(&activated_, idx);

	Rectf newPos = hintPosition_;
	hintPosition_.width(-1.f);
	if(newPos.width() > 0.f){
		hint()->setPosition(newPos);
		hint()->show();
	}

	__super::quant(dt);
}


// ------------------- UI_ControlInventory

bool UI_ControlInventory::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	if(event.ID() != UI_INPUT_MOUSE_LBUTTON_DOWN && event.ID() != UI_INPUT_MOUSE_RBUTTON_DOWN && event.ID() != UI_INPUT_MOUSE_MOVE){
		if(event.ID() == UI_INPUT_HOTKEY){
			if(UnitInterface* p = selectManager->getUnitIfOne()){
				if(const InventorySet* ip = p->inventory()){
					int index = ip->getPosition(this, Vect2i(0,0));
					if(!(ip->takenItem()) && !ip->isCellEmpty(index)){
						UI_LogicDispatcher::instance().setTakenItemOffset(Vect2f(0,0));
						p->sendCommand(UnitCommand(COMMAND_ID_ITEM_TAKE, index));
					}
				}
			}
		}
		return ret;
	}

	if(UnitInterface* p = selectManager->getUnitIfOne()){
		if(const InventorySet* ip = p->inventory()){
			Vect2i pos(0,0);
			Vect2f cursor_pos = (!(ip->takenItem())) ? event.cursorPos() : event.cursorPos() - UI_LogicDispatcher::instance().takenItemOffset();

			if(!getCellCoords(cursor_pos, pos))
				return ret;

			if(!(ip->takenItem())){
				if(event.ID() == UI_INPUT_MOUSE_MOVE)
					highlight_ = Recti(0,0,0,0);

				Vect2i item_pos;

				int index = ip->getPosition(this, pos);
				if(!ip->isCellEmpty(index)){
					switch(event.ID()){
					case UI_INPUT_MOUSE_LBUTTON_DOWN:
						item_pos = itemPosition(pos);
						if(item_pos != Vect2i(-1,-1)){
							Vect2f screen_pos = screenCoords(item_pos);
							UI_LogicDispatcher::instance().setTakenItemOffset(event.cursorPos() - screen_pos);
						}
						p->sendCommand(UnitCommand(COMMAND_ID_ITEM_TAKE, index));
						break;
					case UI_INPUT_MOUSE_RBUTTON_DOWN:
						p->sendCommand(UnitCommand(COMMAND_ID_ITEM_DROP, index));
						break;
					}
				}
			}
			else {
				Vect2i size = ip->takenItem().size();
				pos = adjustPosition(pos, size);

				switch(event.ID()){
				case UI_INPUT_MOUSE_MOVE:
					highlight_ = Recti(0,0,0,0);
					
					if(canPutItem(pos, size))
						highlight_ = Recti(pos, size);
					break;
				case UI_INPUT_MOUSE_LBUTTON_DOWN:
					p->sendCommand(UnitCommand(COMMAND_ID_ITEM_RETURN, ip->getPosition(this, pos)));
					break;
				}
			}

			ret = true;
		}
	}

	return ret;
}
