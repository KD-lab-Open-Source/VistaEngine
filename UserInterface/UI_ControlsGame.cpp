#include "StdAfx.h"

#include "SafeCast.h"
#include "Serialization.h"

#include "UI_Render.h"
#include "UserInterface.h"
#include "UI_Logic.h"
#include "UI_Actions.h"
#include "UI_Controls.h"
#include "UI_UnitView.h"
#include "UI_CustomControls.h"

#include "GameShell.h"
#include "UnitAttribute.h"
#include "BaseUnit.h"
#include "Squad.h"
#include "..\Environment\Anchor.h"

#include "SelectManager.h"
#include "ShowHead.h"

#include "CameraManager.h"
#include "..\Environment\Environment.h"

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
	LOG_UI_STATE("begin");

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

	LOG_UI_STATE("end");
}

void UI_ControlBase::controlUpdate(ControlState& controlState)
{
}

void UI_ControlBase::actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState)
{
	switch(action_id){
		case UI_ACTION_EXPAND_TEMPLATE:{
			string str = locText_.c_str();
			UI_LogicDispatcher::instance().expandTextTemplate(str);
			if(!str.empty())
				setText(str.c_str());
			else
				controlState.hide();
			break;
									   }
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
		
		case UI_ACTION_BIND_TO_UNIT_STATE:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActionUnitState* action = safe_cast<const UI_ActionUnitState*>(action_data)){
				if(const UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
					switch(action->type()){
					case UI_ActionUnitState::UI_UNITSTATE_RUN:
						setState((int)unit->getUnitReal()->runMode());
						break;
					case UI_ActionUnitState::UI_UNITSTATE_AUTO_FIND_TRANSPORT:{
						const UnitReal* unitReal = unit->getUnitReal();
						if(unitReal->attr().isLegionary() || unitReal->attr().isBuilding()){
							controlState.show();
							setState((int)safe_cast<const UnitActing*>(unitReal)->autoFindTransport());
						} 
						else
							controlState.hide();
						break;
																			  }
					case UI_ActionUnitState::UI_UNITSTATE_SELF_ATTACK:
						setState(getStateByMark(UI_STATE_MARK_UNIT_SELF_ATTACK, (int)safe_cast<const UnitActing*>(unit->getUnitReal())->autoAttackMode()));
						break;
					case UI_ActionUnitState::UI_UNITSTATE_WEAPON_MODE:
						setState(getStateByMark(UI_STATE_MARK_UNIT_WEAPON_MODE, (int)safe_cast<const UnitActing*>(unit->getUnitReal())->weaponMode()));
						break;
					case UI_ActionUnitState::UI_UNITSTATE_WALK_ATTACK_MODE:
						setState(getStateByMark(UI_STATE_MARK_UNIT_WALK_ATTACK_MODE, (int)safe_cast<const UnitActing*>(unit->getUnitReal())->walkAttackMode()));
						break;
					case UI_ActionUnitState::UI_UNITSTATE_AUTO_TARGET_FILTER:
						setState(getStateByMark(UI_STATE_MARK_UNIT_AUTO_TARGET_FILTER, (int)safe_cast<const UnitActing*>(unit->getUnitReal())->autoTargetFilter()));
						break;

					case UI_ActionUnitState::UI_UNITSTATE_CAN_DETONATE_MINES:
						if(action->checkOnlyMainUnitInSquad() 
							? unit->getUnitReal()->canDetonateMines()
							: unit->canDetonateMines())
							controlState.show();
						else
							controlState.hide();
						break;
					case UI_ActionUnitState::UI_UNITSTATE_IS_IDLE:
						if(action->checkOnlyMainUnitInSquad() 
							? !unit->getUnitReal()->isWorking()
							: !unit->isWorking())
							controlState.show();
						else
							controlState.hide();
						break;
					case UI_ActionUnitState::UI_UNITSTATE_CAN_UPGRADE:
						controlState.apply(action->checkOnlyMainUnitInSquad()
							? unit->getUnitReal()->canUpgrade(action->data())
							: unit->canUpgrade(action->data()));
						break;
					case UI_ActionUnitState::UI_UNITSTATE_IS_UPGRADING:
						if((action->data() == -1 && unit->getUnitReal()->isUpgrading()) || (action->data() >= 0 && unit->getUnitReal()->executedUpgradeIndex() == action->data()))
							controlState.hide();
						else
							controlState.show();
						break;
					case UI_ActionUnitState::UI_UNITSTATE_CAN_BUILD:
						controlState.apply(action->checkOnlyMainUnitInSquad()
							? unit->getUnitReal()->canProduction(action->data())
							: unit->canProduction(action->data()));
						break;
					case UI_ActionUnitState::UI_UNITSTATE_IS_BUILDING:
						if(unit->selectionAttribute()->hasProdused()){
							ProducedQueue::const_iterator it;
							FOR_EACH(safe_cast<const UnitActing*>(unit->getUnitReal())->producedQueue(), it){
								if(it->type_ == PRODUCE_UNIT && it->data_ == action->data()){
									controlState.hide();
									break;
								}
							}
							controlState.show();
						}
						break;
					case UI_ActionUnitState::UI_UNITSTATE_CAN_PRODUCE_PARAMETER:
						controlState.apply(action->checkOnlyMainUnitInSquad()
							? unit->getUnitReal()->canProduceParameter(action->data())
							: unit->canProduceParameter(action->data()));
						break;
					case UI_ActionUnitState::UI_UNITSTATE_COMMON_OPERABLE:
						if(!unit->getUnitReal()->isUpgrading() && unit->getUnitReal()->isConstructed())
							controlState.enable();
						else
							controlState.disable();

						break;
					}
				}
			}
			break;

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
								bool empty = safe_cast<UnitActing*>(unit->getUnitReal())->producedQueue().empty();
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
								const LegionariesLinks& slots = safe_cast<UnitActing*>(unit->getUnitReal())->transportSlots();
								LegionariesLinks::const_iterator slot_it;
								FOR_EACH(slots, slot_it)
									if(*slot_it && (*slot_it)->isDocked())
										break;
								
								if(slot_it == slots.end() && action->queueSize() == UI_ActionBindEx::UI_QUEUE_EMPTY ||
									slot_it != slots.end() && action->queueSize() == UI_ActionBindEx::UI_QUEUE_NOT_EMPTY)
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
		
		case UI_ACTION_CLICK_MODE:
			if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
				const UI_ActionDataClickMode* dat = safe_cast<const UI_ActionDataClickMode*>(action_data);
				if(dat->modeID() == UI_CLICK_MODE_ATTACK && dat->weaponPrm())
					if(safe_cast<UnitActing*>(unit->getUnitReal())->hasWeapon(dat->weaponPrm()->ID())){
						controlState.show();
						if(unit->canFire(dat->weaponPrm()->ID()))
							controlState.enable();
						else
							controlState.disable();
					}
					else
						controlState.hide();
			}
			break;

		case UI_ACTION_UNIT_COMMAND:
			if(const UI_ActionDataUnitCommand* p = safe_cast<const UI_ActionDataUnitCommand*>(action_data)){
				UnitCommand command = p->command();
				switch(command.commandID()){
				case COMMAND_ID_DIRECT_CONTROL:
					if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
						setState((int)safe_cast<UnitActing*>(unit->getUnitReal())->isDirectControl());
					else
						setState(0);
					break;
				case COMMAND_ID_SYNDICAT_CONTROL:
					if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
						setState((int)safe_cast<UnitActing*>(unit->getUnitReal())->isSyndicateControl());
					else
						setState(0);
					break;
				case COMMAND_ID_GO_RUN:
					if(selectManager->canRun())
						controlState.enable();
					else
						controlState.disable();
					break;
				case COMMAND_ID_STOP_RUN:
					controlState.enable();
					break;
				}
			}
			break;
		
		case UI_ACTION_UNIT_HAVE_PARAMS:
			if(const UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit())
				if(unit->getUnitReal()->requestResource(safe_cast<const UI_ActionDataParams*>(action_data)->prms(), NEED_RESOURCE_SILENT_CHECK))
					controlState.hide();
				else
					controlState.show();
			else if(const Player* plr = UI_LogicDispatcher::instance().player())
				if(plr->requestResource(safe_cast<const UI_ActionDataParams*>(action_data)->prms(), NEED_RESOURCE_SILENT_CHECK))
					controlState.hide();
				else
					controlState.show();
			else
				controlState.show();
			break;
	
		case UI_ACTION_ACTIVATE_WEAPON:
			if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
				const UI_ActionDataWeapon* dat = safe_cast<const UI_ActionDataWeapon*>(action_data);
				if(dat->weaponPrm())
					if(safe_cast<UnitActing*>(unit->getUnitReal())->hasWeapon(dat->weaponPrm()->ID())){
						controlState.show();
						if(unit->canFire(dat->weaponPrm()->ID()))
							controlState.enable();
						else
							controlState.disable();
					}
					else
						controlState.hide();
			}
			else
				controlState.hide();
			break;

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

		case UI_ACTION_UNIT_IN_TRANSPORT: {
			const UI_ActionDataUnitOrBuildingRef* dp = safe_cast<const UI_ActionDataUnitOrBuildingRef*>(action_data);
			if(selectManager->isInTransport(dp->attribute(), dp->single()))
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

		case UI_BIND_PRODUCTION_QUEUE:
			if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
				const AttributeBase* producer = unit->selectionAttribute();
				if(producer->hasProdused()){
					ProducedQueue::const_iterator it;
					FOR_EACH(safe_cast<const UnitActing*>(unit->getUnitReal())->producedQueue(), it){
						if(it->type_ == PRODUCE_RESOURCE && it->data_ == safe_cast<const UI_ActionDataProduction*>(action_data)->productionNumber()){
							controlState.hide();
							break;
						}
					}
					controlState.show();
				}
			}
			break;

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
							const UnitActing* p = safe_cast<const UnitActing*>(target);
							if(p->canPutInTransport(unit_real) && 
								(p->position2D().distance2(unit_real->position2D()) < 
								sqr(p->radius() + unit_real->radius() + p->attr().transportLoadDirectControlRadius)))
									is_transport = true;
						}

						setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, is_transport ? UI_DIRECT_CONTROL_CURSOR_TRANSPORT : UI_DIRECT_CONTROL_CURSOR_ALLY));
					}
					else
						setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, UI_DIRECT_CONTROL_CURSOR_ENEMY));
				}
				else {
					setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, UI_DIRECT_CONTROL_CURSOR_NONE));
				}

				UI_Transform trans;
				float scale = dp->cursorScale(aim_distance);
				trans.setScale(Vect2f(scale, scale));
				setPermanentTransform(trans);
			}
			else
				setState(getStateByMark(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, UI_DIRECT_CONTROL_CURSOR_NONE));
			break;

		case UI_ACTION_DIRECT_CONTROL_TRANSPORT:
			controlState.hide();
			if(gameShell->underFullDirectControl()){
				if(UnitInterface* selected_unit = UI_LogicDispatcher::instance().selectedUnit()){
					UnitReal* selected_unit_real = selected_unit->getUnitReal();
					UnitInterface* ip = UI_LogicDispatcher::instance().hoverUnit();
					if(ip && ip->attr().isTransport()){
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

		case UI_ACTION_CLICK_MODE:
			if(const UI_ActionDataClickMode* p = safe_cast<const UI_ActionDataClickMode*>(action_data))
				UI_LogicDispatcher::instance().selectClickMode(p->modeID(), p->weaponPrm());
			break;

		case UI_ACTION_ANIMATION_CONTROL:
			startAnimation(safe_cast<const UI_ActionPlayControl*>(action_data)->action());
			break;

		case UI_ACTION_ACTIVATE_WEAPON:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActionDataWeapon* action = safe_cast<const UI_ActionDataWeapon*>(action_data)){
				if(action->weaponPrm())
					selectManager->makeCommand(UnitCommand(COMMAND_ID_WEAPON_ACTIVATE, action->weaponPrm()->ID()));
			}
			break;

		case UI_ACTION_UNIT_COMMAND:
			if(!UI_LogicDispatcher::instance().isGameActive())
				break;
			if(const UI_ActionDataUnitCommand* p = safe_cast<const UI_ActionDataUnitCommand*>(action_data)){
				int selectedSlot = -1;
				bool uniform = selectManager->uniform();
				UnitCommand command = p->command();

				command.setShiftModifier(UI_LogicDispatcher::instance().mouseFlags() & MK_SHIFT);
				
				// для списка юнитов нужно узнать индекс дочернего контрола
				if(const UI_ControlUnitList* own = dynamic_cast<const UI_ControlUnitList*>(owner())){
					xassert(std::find(own->controlList().begin(), own->controlList().end(), this) != own->controlList().end());
					int child_index = std::distance(own->controlList().begin(), std::find(own->controlList().begin(), own->controlList().end(), this));
					switch(own->GetType()){
					case UI_UNITLIST_SELECTED: // для списка выделенных нужно найти конкретного юнита
						selectedSlot = child_index;
						break;
					case UI_UNITLIST_PRODUCTION: // для остальных случаев конкретизировать команду
					case UI_UNITLIST_TRANSPORT:					
						command = UnitCommand(command.commandID(), child_index);
						break;
					}
					uniform = false;
				}else if(command.commandID() == COMMAND_ID_DIRECT_CONTROL){
					command = UnitCommand(command.commandID(), !gameShell->underFullDirectControl());
					uniform = false;
				}else if( command.commandID() == COMMAND_ID_SYNDICAT_CONTROL){
					command = UnitCommand(command.commandID(), !gameShell->underHalfDirectControl());
					uniform = false;
				}
				
				if(selectedSlot >= 0)
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

			if(const UI_ActionDataUnitOrBuildingAction* p = safe_cast<const UI_ActionDataUnitOrBuildingAction*>(action_data))
				selectManager->selectAll(p->attribute());
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
					if(ip && ip->attr().isTransport()){
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
					inputEventHandler(event);
					UI_LogicDispatcher::instance().inputEventProcessed(event, this);
				}

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
		xassert(event.ID() < 16);
		setInputActionFlag((ActionMode)(1 << event.ID()));
		return true;
	}
	return false;
}

//----------------------------------------

void UI_ControlBase::quant(float dt)
{
	MTG();

	LOG_UI_STATE("begin");

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

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->quant(dt);

	if(linkToAnchor_ && UI_LogicDispatcher::instance().isGameActive()){
		if(const Anchor* anchor = environment->findAnchor(anchorName_.c_str())){
			Vect2i pos(anchor->position().xi(), anchor->position().yi());
			Vect3f pos3d = To3D(pos);
			Vect3f e, w;
			cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos3d, &w, &e);
			if(w.z > 0)
				setPosition(UI_Render::instance().relativeCoords(Vect2i(e.xi(), e.yi())));
		}
	}

	transformQuant(dt);

	if(UI_LogicDispatcher::instance().gamePause())
		waitingUpdate_ = false;

	hoverToggle(false);
	actionFlags_ = 0;

	LOG_UI_STATE("end");
}

const AttributeBase* UI_ControlBase::actionUnit(const AttributeBase* selected) const
{
	const AttributeBase* unit = 0;

	for(StateContainer::const_iterator it = states_.begin(); it != states_.end(); ++it)
		for(int i = 0; i < it->actionCount(); i++){
			switch(it->actionID(i)){
			//case UI_ACTION_BIND_TO_UNIT:
			case UI_ACTION_UNIT_SELECT:
				unit = safe_cast<const UI_ActionDataUnitOrBuildingAction*>(it->actionData(i))->attribute();
				break;
			case UI_ACTION_BUILDING_INSTALLATION:
				unit = safe_cast<const UI_ActionDataBuildingAction*>(it->actionData(i))->attribute();
				break;
			case UI_ACTION_BUILDING_CAN_INSTALL:
				unit = safe_cast<const UI_ActionDataBuildingUpdate*>(it->actionData(i))->attribute();
				break;
			case UI_ACTION_BIND_TO_UNIT_STATE:
				unit = safe_cast<const UI_ActionUnitState*>(it->actionData(i))->getAttribute(selected);
				break;
			}
			if(unit)
				return unit;
		}
	
	for(int i = 0; i < actionCount(); i++){
		switch(actionID(i)){
		//case UI_ACTION_BIND_TO_UNIT:
		case UI_ACTION_UNIT_SELECT:
			unit = safe_cast<const UI_ActionDataUnitOrBuildingAction*>(actionData(i))->attribute();
			break;
		case UI_ACTION_BUILDING_INSTALLATION:
			unit = safe_cast<const UI_ActionDataBuildingAction*>(actionData(i))->attribute();
			break;
		case UI_ACTION_BUILDING_CAN_INSTALL:
			unit = safe_cast<const UI_ActionDataBuildingUpdate*>(actionData(i))->attribute();
			break;
		case UI_ACTION_BIND_TO_UNIT_STATE:
			unit = safe_cast<const UI_ActionUnitState*>(actionData(i))->getAttribute(selected);
			break;
		}
		if(unit)
			return unit;
	}

	return 0;
	
}

bool UI_ControlBase::actionParameters(const AttributeBase* unit, ParameterSet& params) const
{
	for(StateContainer::const_iterator it = states_.begin(); it != states_.end(); ++it)
		for(int i = 0; i < it->actionCount(); i++){
			switch(it->actionID(i)){
			case UI_ACTION_UNIT_COMMAND:{
				const UnitCommand& command = safe_cast<const UI_ActionDataUnitCommand*>(it->actionData(i))->command();
				switch(command.commandID()){
				case COMMAND_ID_UPGRADE:
					if(unit->upgrades.exists(command.commandData())){
						params = unit->upgrades[command.commandData()].upgradeValue;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE:
					if(unit->producedUnits.exists(command.commandData())){
						const AttributeBase::ProducedUnits& builds = unit->producedUnits[command.commandData()];
						params = builds.unit->installValue;
						params *= builds.number;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE_PARAMETER:
					if(unit->producedParameters.exists(command.commandData())){
						params = unit->producedParameters[command.commandData()].cost;
						return true;
					}
					break;
				}
				break;
										}
			case UI_ACTION_ACTIVATE_WEAPON:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataWeapon*>(it->actionData(i))->weaponPrm()){
					params = wprm->fireCost();
					return true;
				}
				break;
			case UI_ACTION_CLICK_MODE:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataClickMode*>(it->actionData(i))->weaponPrm()){
					params = wprm->fireCost();
					return true;
				}
				break;
			case UI_ACTION_BUILDING_INSTALLATION:
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataBuildingAction*>(it->actionData(i))->attribute()){
					params = attr->installValue;
					return true;
				}
				break;
			case UI_ACTION_BUILDING_CAN_INSTALL:
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataBuildingUpdate*>(it->actionData(i))->attribute()){
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
					if(unit->upgrades.exists(command.commandData())){
						params = unit->upgrades[command.commandData()].upgradeValue;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE:
					if(unit->producedUnits.exists(command.commandData())){
						const AttributeBase::ProducedUnits& builds = unit->producedUnits[command.commandData()];
						params = builds.unit->installValue;
						params *= builds.number;
						return true;
					}
					break;
				case COMMAND_ID_PRODUCE_PARAMETER:
					if(unit->producedParameters.exists(command.commandData())){
						params = unit->producedParameters[command.commandData()].cost;
						return true;
					}
					break;
				}
				break;
										}
			case UI_ACTION_ACTIVATE_WEAPON:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataWeapon*>(actionData(i))->weaponPrm()){
					params = wprm->fireCost();
					return true;
				}
				break;
			case UI_ACTION_CLICK_MODE:
				if(const WeaponPrm* wprm = safe_cast<const UI_ActionDataClickMode*>(actionData(i))->weaponPrm()){
					params = wprm->fireCost();
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
				if(const AttributeBase* attr = safe_cast<const UI_ActionDataBuildingUpdate*>(actionData(i))->attribute()){
					params = attr->installValue;
					return true;
				}
				break;
			}
		}

		return false;
}

const ProducedParameters* UI_ControlBase::actionBuildParameter(const AttributeBase* unit) const
{
	for(StateContainer::const_iterator it = states_.begin(); it != states_.end(); ++it){
		for(int i = 0; i < it->actionCount(); i++)
			switch(it->actionID(i)){
			case UI_ACTION_UNIT_COMMAND:{
				const UnitCommand& command = safe_cast<const UI_ActionDataUnitCommand*>(it->actionData(i))->command();
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
			setState(newState);
			data_ = effectRND.fabsRnd(safe_cast<const UI_ActionAutochangeState*>(action_data)->time().minimum(), safe_cast<const UI_ActionAutochangeState*>(action_data)->time().maximum());
		}
		break;

	default:
		UI_ControlBase::actionUpdate(action_id, action_data, controlState);
	}

}

void UI_ControlButton::actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data){
	switch(action_id){
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
	case UI_ACTION_MESSAGE_LIST: 
		if(UI_LogicDispatcher::instance().messagesEnabled()
			|| safe_cast<const UI_ActionDataMessageList*>(action_data)->ignoreMessageDisabling())
		{
			string str;
			if(UI_Dispatcher::instance().getMessageQueue(
					str,
					safe_cast<const UI_ActionDataMessageList*>(action_data)->types(),
					safe_cast<const UI_ActionDataMessageList*>(action_data)->reverse()
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

	case UI_ACTION_TASK_LIST: {
		string str;
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

	if(isVisible())
		setText(key_.toString().c_str());
}

bool UI_ControlHotKeyInput::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);
	
	switch(event.ID()) {
	case UI_INPUT_MOUSE_LBUTTON_DBLCLICK:
	case UI_INPUT_MOUSE_RBUTTON_DBLCLICK:
		done(sKey(event.keyCode(), true), true);
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
			done(sKey(event.keyCode(), true), true);
		else
			done(sKey(0, true), false);
		break;
	default:
		done(sKey(event.keyCode(), true), false);
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
				if(UnitInterface* unit = UI_LogicDispatcher::instance().selectedUnit()){
					const UnitActing* produser = safe_cast<const UnitActing*>(unit->getUnitReal());
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
		case UI_ACTION_RELOAD_PROGRESS:
			if(UI_LogicDispatcher::instance().isGameActive()){
				const UI_ActionDataWeaponReload* action = safe_cast<const UI_ActionDataWeaponReload*>(action_data);
				if(UnitInterface* sp = UI_LogicDispatcher::instance().selectedUnit()){
					const WeaponPrm* weapon = action->weaponPrm();
					if(!weapon || safe_cast<UnitActing*>(sp->getUnitReal())->hasWeapon(weapon->ID()))
					{
						setProgress(sp->weaponChargeLevel(weapon ? weapon->ID() : 0));

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
		case UI_ACTION_DIRECT_CONTROL_WEAPON_LOAD:
			if(!gameShell->underFullDirectControl()){
				controlState.hide();
				break;
			}
			if(UnitInterface* p = UI_LogicDispatcher::instance().selectedUnit()){
				if(cameraManager->directControlFreeCamera()){
					controlState.hide();
					break;
				}
				controlState.show();
				UnitActing* unit = safe_cast<UnitActing*>(p->getUnitReal());
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
		showHead().ToggleDraw(true);
		if(showHead().IsPlay()){
			minimap().startDelay();
			return true;
		}
		else if(!minimap().delay())
			showHead().ToggleDraw(false);
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
		minimap().redraw(alpha());
		return true;
	}

	return false;
}

void UI_ControlCustom::controlInit()
{
	if(checkType(UI_CUSTOM_CONTROL_MINIMAP)){
		if(useSelectedMap_){
			minimap().setViewParameters(mapAlpha_, false, false, false, false);
			if(const MissionDescription* mission = UI_LogicDispatcher::instance().currentMission())
				minimap().init(mission->worldSize(), mission->worldData("map.tga").c_str());
			else
				minimap().releaseMapTexture();
		}
		else{
			minimap().setViewParameters(mapAlpha_, drawViewZone_, drawFogOfWar_, true, true, viewZoneColor_, rotateByCamera_, mapAlign_, miniMapBorderColor_);
			minimap().init(Vect2f(vMap.H_SIZE, vMap.V_SIZE), vMap.getTargetName("map.tga"));
		}
		minimap().setViewStartLocationsParameters(viewStartLocations_, font_);
		if(getAngleFromWorld_ && environment)
			minimapAngle_ = environment->minimapAngle();
		minimap().setRotate(minimapAngle_ / 180.f * M_PI);
	}
	
	UI_ControlBase::controlInit();
}

bool UI_ControlCustom::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = false;

	if(clickAction_ && (event.isMouseClickEvent() || (event.ID() == UI_INPUT_MOUSE_MOVE && UI_LogicDispatcher::instance().isMouseFlagSet(MK_LBUTTON | MK_RBUTTON)))){
		ret = true;
		if(checkType(UI_CUSTOM_CONTROL_MINIMAP)){
			if((!checkType(UI_CUSTOM_CONTROL_HEAD) || !showHead().IsPlay()) &&
				(!checkType(UI_CUSTOM_CONTROL_SELECTION) || UI_UnitView::instance().isEmpty())){
					if(selectManager->isSelectionEmpty())
						minimap().pressEvent(UI_LogicDispatcher::instance().mousePosition());
					else{
						UI_InputEvent event_ = event;
						Vect2f mousePos = UI_LogicDispatcher::instance().mousePosition();  
						event_.setCursorPos(minimap().minimap2world(UI_LogicDispatcher::instance().mousePosition()));
						if((event_.ID() == UI_INPUT_MOUSE_LBUTTON_DOWN && UI_LogicDispatcher::instance().clickMode() == UI_CLICK_MODE_NONE) ||
							(event.ID() == UI_INPUT_MOUSE_MOVE && UI_LogicDispatcher::instance().isMouseFlagSet(MK_LBUTTON)))
							minimap().pressEvent(mousePos);
						else{
							Vect3f hoverPosition = UI_LogicDispatcher::instance().hoverPosition();
							Vect3f hoverPositionTerrain = UI_LogicDispatcher::instance().hoverPositionTerrain();
							UI_LogicDispatcher::instance().setHoverPosition(To3D(minimap().minimap2world(mousePos)));
							UI_LogicDispatcher::instance().setHoverPositionTerrain(To3D(minimap().minimap2world(mousePos)));
							if(event.isMouseClickEvent())
								minimap().checkEvent(EventMinimapClick(Event::UI_MINIMAP_ACTION_CLICK, mousePos));
							UI_Dispatcher::instance().handleInput(event_);
							UI_LogicDispatcher::instance().setHoverPosition(hoverPosition);
							UI_LogicDispatcher::instance().setHoverPositionTerrain(hoverPositionTerrain);
						}
					}
			}
		}else if(checkType(UI_CUSTOM_CONTROL_SELECTION) && !UI_UnitView::instance().isEmpty()){
			if(!checkType(UI_CUSTOM_CONTROL_HEAD) || !showHead().IsPlay())
				selectManager->makeCommand(UnitCommand(COMMAND_ID_CAMERA_FOCUS), true);
		}
	}

	return __super::inputEventHandler(event) || ret;
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

	UI_ControlContainer::ControlList ctrls = controlList();
	UI_ControlContainer::ControlList::const_iterator ctrl_it;

	bool need_clear_unit_list = false;

	switch(GetType()){
	case UI_UNITLIST_SELECTED:{
		SelectManager::Slots selection;
		selectManager->getSelectList(selection);
		
		int activeSlot = selectManager->selectedSlot();
		int idx = 0;
		
		SelectManager::Slots::iterator slot_it = selection.begin();
		FOR_EACH(ctrls, ctrl_it)
			if(slot_it != selection.end()){
				setSprites(*ctrl_it, slot_it->attr());
				
				int size = slot_it->units.size() > 1
					? slot_it->units.size()
					: (slot_it->units.front()->getSquadPoint() ? slot_it->units.front()->getSquadPoint()->unitsNumber() : 1);
				
				if(size > 1){
					XBuffer buf;
					buf < (slot_it->uniform() ? "" : "M") <= size;
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
				(*ctrl_it)->setText(0);
				(*ctrl_it)->setPermanentTransform(UI_Transform::ID);
			}

		break;
							  }
	case UI_UNITLIST_PRODUCTION:
		if(!selectManager) break;
		if(UnitInterface* unit = selectManager->getUnitIfOne()){
			const AttributeBase* producer = unit->selectionAttribute();
			if(producer->hasProdused()){
				
				const ProducedQueue& products = safe_cast<UnitActing*>(unit->getUnitReal())->producedQueue();
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
							setSprites(ctrl, sprites);
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
	
	case UI_UNITLIST_TRANSPORT:
		if(!selectManager) break;
		if(UnitInterface* unit = selectManager->getUnitIfOne()){
			if(unit->selectionAttribute()->isTransport()){
				
				const LegionariesLinks& slots = safe_cast<UnitActing*>(unit->getUnitReal())->transportSlots();
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
					
					const AttributeBase* attribute = pr_it->unit;
					setSprites(ctrl, attribute);
					
					++pr_it;
				
				} else
					setSprites(ctrl);

			}
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

// ------------------- UI_ControlInventory

bool UI_ControlInventory::inputEventHandler(const UI_InputEvent& event)
{
	bool ret = __super::inputEventHandler(event);

	if(event.ID() != UI_INPUT_MOUSE_LBUTTON_DOWN && event.ID() != UI_INPUT_MOUSE_RBUTTON_DOWN)
		return ret;

	Vect2i pos;
	if(!getCellCoords(event.cursorPos(), pos))
		return ret;

	if(UnitInterface* p = selectManager->getUnitIfOne()){
		if(const InventorySet* ip = p->inventory()){
			int index = ip->getPosition(this, pos);

			if(!(ip->takenItem())){
				if(!ip->isCellEmpty(index)){
					switch(event.ID()){
					case UI_INPUT_MOUSE_LBUTTON_DOWN:
						p->sendCommand(UnitCommand(COMMAND_ID_ITEM_TAKE, index));
						break;
					case UI_INPUT_MOUSE_RBUTTON_DOWN:
						p->sendCommand(UnitCommand(COMMAND_ID_ITEM_DROP, index));
						break;
					}
				}
			}
			else {
				switch(event.ID()){
				case UI_INPUT_MOUSE_LBUTTON_DOWN:
					p->sendCommand(UnitCommand(COMMAND_ID_ITEM_RETURN, index));
					break;
				}
			}

			ret = true;
		}
	}

	return ret;
}
