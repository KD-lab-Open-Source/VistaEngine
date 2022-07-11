#include "StdAfx.h"

#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "vmap.h"
#include "Universe.h"

#include "Squad.h"
#include "Actions.h"

#include "Serialization\Serialization.h"
#include "Serialization\Dictionary.h"
#include "Serialization\SerializationFactory.h"
#include "kdw/PropertyEditor.h"
#include "EditableCondition.h"
#include "Conditions.h"
#include "Environment\SourceBase.h"
#include "Environment\SourceManager.h"
#include "Environment\Anchor.h"
#include "Player.h"
#include "StringWrappers.h"
#include "Serialization\BinaryArchive.h"
#include "Serialization\StringTable.h"

class EventFactoryArg1
{
public:
	EventFactoryArg1()
	{
		buffer_ = 0;
	}

	template<class Derived>
	Event* createArg() const
	{
		xassert(buffer_);
		Event* event = new Derived(*buffer_);
		buffer_ = 0;
		return event;
	}
	void setXBuffer(XBuffer& buffer){
		buffer_ = &buffer;
	}

private:
	mutable XBuffer* buffer_;
};
typedef Factory<Event::Type, Event, EventFactoryArg1> EventFactory;

typedef Event EventChangeActivePlayer;
REGISTER_CLASS_IN_FACTORY(EventFactory, Event::CHANGE_ACTIVE_PLAYER, EventChangeActivePlayer);
REGISTER_CLASS_IN_FACTORY(EventFactory, Event::CHANGE_PLAYER_AI, EventChangePlayerAI);
REGISTER_CLASS_IN_FACTORY(EventFactory, Event::KEYBOARD_CLICK, EventKeyBoardClick);
REGISTER_CLASS_IN_FACTORY(EventFactory, Event::NETWORK_DISCONNECT, EventNet);
REGISTER_CLASS_IN_FACTORY(EventFactory, Event::UI_BUTTON_CLICK_LOGIC, EventButtonClick);
REGISTER_CLASS_IN_FACTORY(EventFactory, Event::STRING, EventStringPlayer);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Event, Type, "Type")
REGISTER_ENUM_ENCLOSED(Event, END_REPLAY, "Конец реплея");
REGISTER_ENUM_ENCLOSED(Event, NETWORK_DISCONNECT, "Разрыв сети или сессии");
REGISTER_ENUM_ENCLOSED(Event, GAME_CLOSE, "Выход из игры");
REGISTER_ENUM_ENCLOSED(Event, RESET_PROFILE, "Сменился профиль");
REGISTER_ENUM_ENCLOSED(Event, CHANGE_ACTIVE_PLAYER, "Смена активного игрока");
END_ENUM_DESCRIPTOR_ENCLOSED(Event, Type)

Event* Event::create(XBuffer& buffer)
{
	Type type;
	buffer.read(type);
	buffer -= sizeof(Type);
	EventFactory::instance().setXBuffer(buffer);
	return EventFactory::instance().create(type);
}

//------------------------------------------------------
void Trigger::quant(TriggerChain& triggerChain)
{
	start_timer_auto();

	currentTriggerName_ = name();

	switch(state()){
		case SLEEPING:
		case DONE:{
			// Входящие стрелки одного цвета - И, разных - ИЛИ
			vector<int> conditions(STRATEGY_COLOR_MAX, 0);
			IncomingLinksList::iterator li;
			FOR_EACH(incomingLinks_, li)
				conditions[(*li)->colorType()] |= (*li)->active() ? 1 : 2;

			vector<int>::iterator bi;
			FOR_EACH(conditions, bi)
				if(*bi == 1){
					setState(CHECKING, triggerChain);
					XBuffer buf;
					buf < "Ready[" <= global_time()/100 < "]: " < name();
					triggerChain.addLogRecord(*this, buf);
            		break;
				}
			
			if(state() != CHECKING)
				break;
			}

		case CHECKING:{
			if(checkCondition()){
				activate(triggerChain);
				XBuffer buf;
				buf < "Start [" <= global_time()/100 < "]: " < name();
				if(action)
					buf < " - " < action->name();
				buf < " (" <= executionCounter_ < ")";
				triggerChain.addLogRecord(*this, buf);
			}
			else
				break;
			}
		case WORKING:
			if(!action || action->workedOut()){
				if(action)
					action->clear();
				int size = outcomingLinks_.size();
				setState(DONE, triggerChain, !randomizeOutcomingLinks_ ? 0 : (triggerChain.checkDeterminism() ? logicRND(size) : effectRND(size)));
				XBuffer buf;
				buf < "Finish[" <= global_time()/100 < "]: " < name();
				triggerChain.addLogRecord(*this, buf);
			}
			break;
		}

	currentTriggerName_ = "";
}

void Trigger::setPlayer(Player* aiPlayer)
{
	if(condition)
		condition->setPlayer(aiPlayer);
	if(action)
		action->setPlayer(aiPlayer);
}

bool Trigger::checkCondition()
{
	start_timer_auto();

	if(onlyIfAi_ && !action->aiPlayer().isAI())
		return false;

	if(isContext_){
		start_timer_auto();
		for(int i = 0; i < unitsPerQuant_; ++i){
			UnitActing* unit = condition ? condition->eventContextUnit() : 0;
			if(unit && !unit->alive())
				unit = 0;

			while(!unit){
				Player* player = !extendedScan_ ? &action->aiPlayer() : universe()->findPlayer(playerIndex_);
				const RealUnits& unitList = !contextFilter_.attributes.empty() ? player->realUnits(contextFilter_.attributes[listIndex_]) : player->realUnits();
				if(unitList.empty() || unitIndex_ >= unitList.size()){
					unitIndex_ = 0;
					if(++listIndex_ < contextFilter_.attributes.size())
						return false;
					else
						listIndex_ = 0;
					if(!extendedScan_ || ++playerIndex_ >= universe()->Players.size()){
						playerIndex_ = 0;
						if(condition)
							condition->clearContext();
						action->clearContext();
					}
					return false;
				}

				UnitReal* unitReal = unitList[unitIndex_];
				if(!unitReal->alive() || !unitReal->attr().isActing())
					unitIndex_++;
				else
					unit = safe_cast<UnitActing*>(unitReal);
			}

			if(condition && !condition->checkDebug(unit) || !action->checkContextUnit(unit)){
				unitIndex_++;
			}
			else{
				action->setContextUnit(unit);
				if(action->automaticCondition())
					return true; // хороший контекст
			}
		}
	} 
	else{
		start_timer_auto();
		return (!condition || condition->checkDebug()) && (!action || action->automaticCondition()); // если не контекстн. то сразу результат
	}

	return false; // не нашли нужный контекст
}

void Trigger::checkEvent(const Event& event)
{
	if((state() == CHECKING || state() == WORKING) && condition)
		condition->checkEvent(event);
}

void Trigger::activate(TriggerChain& triggerChain)
{
	++unitIndex_;

	if(triggerChain.checkDeterminism()){
		log_var(triggerChain.name);
		log_var(name());
	}

#ifndef _FINAL_VERSION_
	if(breakWhenActivate_){
		xassertStr("Активировался триггер: " && 0, name());
		checkCondition();
	}
#endif

	if(condition)
		condition->clear();

	if(action && action->auxType() == Action::RESET_TO_CURRENT){
		triggerChain.restart(false);
		triggerChain.addLogRecord(*this, "RESET_TO_CURRENT");
	}

	setState(WORKING, triggerChain);

	if(action){
#ifndef _FINAL_VERSION_
		if(showDebugUnitInterface.debugString){
			if(ActionContextSquad* squadAct = dynamic_cast<ActionContextSquad*>(&*action))
				squadAct->contextSquad_->setDebugString(debugDisplayTextAction());
			else if(ActionContext* unitAct = dynamic_cast<ActionContext*>(&*action))
				unitAct->contextUnit_->setDebugString(debugDisplayTextAction());
		}
#endif
		action->activate();

		if(action->auxType() == Action::RESET_TO_START){
			triggerChain.restart(true);
			triggerChain.addLogRecord(*this, "RESET_TO_START");
		}
	}
}

const char* Trigger::debugDisplayTextAction() const
{
	static XBuffer buffer(1024, 1);
	buffer.init();
	buffer < name();
	if(action)
		buffer < " - " < action->name();
	buffer < " (" <= executionCounter_ < ", " <= global_time()/1000 < ")";
	return buffer;
}

//------------------------------------------------------
void TriggerChain::setAIPlayerAndTriggerChain(Player* aiPlayer) 
{
	TriggerList::iterator ti;
	FOR_EACH(triggers, ti)
		ti->setPlayer(aiPlayer);
}

void TriggerChain::quant(bool pause)
{
	if(pause && !ignorePause())
		return;

	Action::setTriggerChain(this);
	VoiceAttribute::setDeterministic(checkDeterminism());

	ActiveTriggers temporaryTriggers = activeTriggers_;
	for(int i = 0; i < temporaryTriggers.size(); i++)
		temporaryTriggers[i]->quant(*this);

	temporaryTriggers = activeTriggersEvent_;
	for(int i = 0; i < temporaryTriggers.size(); i++)
		temporaryTriggers[i]->quant(*this);

	Action::setTriggerChain(0);
	VoiceAttribute::setDeterministic(false);
}

void TriggerChain::checkEvent(const Event& event)
{
	ActiveTriggers::iterator ti;
	FOR_EACH(activeTriggersEvent_, ti)
		(*ti)->checkEvent(event);
}

string editSignalVariableDialog()
{
	string comboList = "";
	AttributeLibrary::Map::const_iterator mi;
	FOR_EACH(AttributeLibrary::instance().map(), mi){
		const AttributeBase* attribute = mi->get();
		if(attribute){
			AttributeBase::ProducedParametersList::const_iterator i;
			FOR_EACH(attribute->producedParameters, i)
				if(strlen(i->second.signalVariable.c_str())){
					comboList += i->second.signalVariable;
					comboList += "|";
				}
		}
	}
	return comboList;
}

/////////////////////////////////////////////////

EventButton::EventButton(XBuffer& buffer) : Event(buffer)
{
	int controlID;
	buffer.read(controlID);
	control_ = UI_ControlReference::getControlByID(controlID);
	buffer.read(playerID_);
}

void EventButton::writeNet(XBuffer& buffer) const
{
	Event::writeNet(buffer); 
	
	buffer.write(UI_ControlReference::getIdByControl(control_));
	buffer.write(playerID_);
}

EventButtonClick::EventButtonClick(XBuffer& buffer) : EventButton(buffer)
{
	buffer.read(inputEvents_);
	buffer.read(modifiers_);
	buffer.read(enabled_);
}

void EventButtonClick::writeNet(XBuffer& buffer) const
{
	EventButton::writeNet(buffer); 
	
	buffer.write(inputEvents_);
	buffer.write(modifiers_);
	buffer.write(enabled_);
}

// --------------------------------------------------------------------------

EventNet::EventNet(XBuffer& buffer) : Event(buffer)
{
	buffer.read(data_);
}

void EventNet::writeNet(XBuffer& buffer) const
{
	Event::writeNet(buffer); 

	buffer.write(data_);
}

// --------------------------------------------------------------------------

EventChangePlayerAI::EventChangePlayerAI(XBuffer& buffer) : Event(buffer)
{
	buffer.read(playerID_);
}

void EventChangePlayerAI::writeNet(XBuffer& buffer) const
{
	Event::writeNet(buffer); 

	buffer.write(playerID_);
}

// --------------------------------------------------------------------------

EventStringPlayer::EventStringPlayer(XBuffer& buffer) : Event(buffer)
{
	buffer > StringInWrapper(name_);
	buffer > playerID_;
}

void EventStringPlayer::writeNet(XBuffer& buffer) const
{
	Event::writeNet(buffer); 

	buffer < StringOutWrapper(name_);
	buffer < playerID_;
}

void ContextFilter::addUnit(const AttributeBase* attr)
{
	if(attr)
		attributes.add(attr);
}

void ContextFilter::addSquad(const AttributeSquad* attr)
{
	if(attr){
		xassert(!attr->allowedUnitsAttributes.empty() && "В триггере указан сквад, к которому не может принадлежать ни один юнит (или нужно перезаписать редактор войск");
		AttributeSquad::AllowedUnitsAttributes::const_iterator i;
		FOR_EACH(attr->allowedUnitsAttributes, i)
			attributes.add(*i);
	}
}
