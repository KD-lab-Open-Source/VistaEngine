#include "StdAfx.h"

#include "Triggers.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "terra.h"
#include "Universe.h"

#include "Squad.h"
#include "Actions.h"

#include "Serialization.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "EditArchive.h"
#include "Dictionary.h"
#include "AttribEditorInterface.h"
#include "..\Util\EditableCondition.h"
#include "..\Util\Conditions.h"
#include "..\Environment\SourceBase.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"
#include "Player.h"
#include "StringWrappers.h"

#include "AttribEditorInterface.h"
AttribEditorInterface& attribEditorInterface();

class EventCreatorBase
{
public:
	static void setBuffer(XBuffer& buffer) { buffer_ = &buffer; }

protected: 
	static XBuffer* buffer_;
};

template<class Derived>
class ObjectCreator<Event, Derived> : public EventCreatorBase
{
public:
	static Event* create() { 
		xassert(buffer_);
		Event* event = new Derived(*buffer_);
		buffer_ = 0;
		return event;
	}
};

typedef Factory<Event::Type, Event> EventFactory;

XBuffer* EventCreatorBase::buffer_ = 0;
// typedef Factory<Event::Type, Event, Arguments1<XBuffer&> > EventFactory;

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
	// return EventFactory::instance().create(type, buffer);
	EventCreatorBase::setBuffer(buffer);
	return EventFactory::instance().create(type);
}

//------------------------------------------------------
void Trigger::quant(TriggerChain& triggerChain)
{
	start_timer_auto();

	switch(state()){
		case SLEEPING:
		case DONE:{
			// Входящие стрелки одного цвета - И, разных - ИЛИ
			vector<int> conditions(STRATEGY_COLOR_MAX, 0);
			IncomingLinksList::iterator li;
			FOR_EACH(incomingLinks_, li)
				conditions[(*li)->getType()] |= (*li)->active() ? 1 : 2;

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
				return;
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
				int size = outcomingLinks_.size();
				setState(DONE, triggerChain, !randomizeOutcomingLinks_ ? 0 : (triggerChain.checkDeterminism() ? logicRND(size) : effectRND(size)));
				XBuffer buf;
				buf < "Finish[" <= global_time()/100 < "]: " < name();
				triggerChain.addLogRecord(*this, buf);
			}
			break;
		}
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
		UnitActing* unitStarted = 0;
		for(int i = 0; i < unitsPerQuant_; ++i){
			UnitActing* unit = 0;
			unit = condition ? condition->eventContextUnit() : 0;
			if(unit && !unit->alive())
				unit = 0;
			while(!unit){
				Player* player = getExtendedPlayer();
				if(!player)
					return false;

				const RealUnits& unitList = player->realUnits();

				if(unitList.empty() || indexScan_ >= unitList.size()){
					++indexScanPlayer_;
					indexScan_ = 0;
					return false;
				}

				UnitReal* unitReal = unitList[indexScan_];
				if(!unitReal->alive() || !unitReal->attr().isActing())
					indexScan_++;
				else
					unit = safe_cast<UnitActing*>(unitReal);
			}

			if(!unitStarted)
				unitStarted = unit;
			else if(unitStarted == unit)
				return false; // cycle!!!

			if(condition && !condition->checkDebug(unit) || !action->checkContextUnit(unit)){
				if(!Condition::scanWait_)
					indexScan_++;
				Condition::scanWait_ = false;
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

Player* Trigger::getExtendedPlayer()
{
	xassert(action && "Контекстное условие нельзя применять без действия");
	Player* aiPlayer = &action->aiPlayer();
	if(!extendedScan_)
		return aiPlayer;

	for(;;){
		Player* player = universe()->findPlayer(indexScanPlayer_);
		if(player->isWorld()){ // Мир - последний в списке
			indexScanPlayer_ = 0;
			return 0;
		}

		switch(extendedPlayerType_){
		case EXTENDED_PLAYER_MY_CLAN:
			if(player->clan() == aiPlayer->clan())
				return player;
			break;
		case EXTENDED_PLAYER_ENEMY:
			if(player->clan() != aiPlayer->clan())
				return player;
			break;
		case EXTENDED_PLAYER_ANY:
			return player;
		}
		++indexScanPlayer_;
	}
}

void Trigger::checkEvent(const Event& event)
{
	if((state() == CHECKING || state() == WORKING) && condition)
		condition->checkEvent(event);
}

void Trigger::activate(TriggerChain& triggerChain)
{
	++indexScan_;

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
void TriggerChain::load(const char* fileName) 
{
	MultiIArchive ia;
	if(ia.open(fileName, strstr(fileName, "GlobalTrigger") ? "..\\ContentBin" : "..\\..\\ContentBin\\Triggers", "scr"))
		ia.serialize(*this, "TriggerChain", 0);
	name = fileName;
}

void TriggerChain::setAIPlayerAndTriggerChain(Player* aiPlayer) 
{
	TriggerList::iterator ti;
	FOR_EACH(triggers, ti)
		ti->setPlayer(aiPlayer);
}

void TriggerChain::save() 
{
	restart(true);
	triggerEvents_.clear();

	MultiOArchive(name.c_str(), strstr(name.c_str(), "GlobalTrigger") ? "..\\ContentBin" : "..\\..\\ContentBin\\Triggers", "scr").serialize(*this, "TriggerChain", 0);
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

AttribEditorInterface& attribEditorInterface();

string editLabelDialog()
{
	string comboList = "";
	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi){
		RealUnits::const_iterator ui;
		FOR_EACH((*pi)->realUnits(), ui)
			if(strlen((*ui)->label())){
				comboList += (*ui)->label();
				comboList += "|";
			}
	}

	return comboList;
}

string editAnchorLabelDialog()
{
	string comboList = "";
	const Environment::Anchors& anchors = environment->anchors();
	Environment::Anchors::const_iterator i;
	FOR_EACH(anchors, i)
		if(strlen((*i)->c_str())){
			comboList += (*i)->c_str();
			comboList += "|";
		}

	return comboList;
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

class TriggerInterfaceImpl : public TriggerInterface
{
public:
	const char* actionComboList() {
		static std::string result = ClassCreatorFactory<Action>::instance().comboListAlt();
		return result.c_str();
	}

	const char* conditionComboList() {  
		static std::string result = ClassCreatorFactory<Condition>::instance().comboListAlt();
		return result.c_str();
	}

	ActionPtr createAction(int typeIndex) {
		return ClassCreatorFactory<Action>::instance().findByIndex(typeIndex).create();
	}

	ConditionPtr createCondition(int typeIndex) {
		return ClassCreatorFactory<Condition>::instance().findByIndex(typeIndex).create();
	}

	ConditionPtr createConditionSwitcher() {
		return new ConditionSwitcher;
	}

	const char* actionName(const Action& action) {
		return ClassCreatorFactory<Action>::instance().find(&action).nameAlt();
	}

	const char* conditionName(const Condition& condition) {
		return ClassCreatorFactory<Condition>::instance().find(&condition).nameAlt();
	}

	bool editCondition(Trigger& trigger, HWND hwnd) {
		bool result = false;
		
		EditArchive ar(hwnd, TreeControlSetup(0, 0, 500, 400, "Scripts\\TreeControlSetups\\editConditionSetup", true));
		if(attribEditorInterface().isTreeEditorRegistered(typeid(EditableCondition).name())){
			EditableCondition condition;
			EditOArchive oar;
			oar.serialize(trigger.condition, "condition", "Condition");
			EditIArchive iar(oar);
			iar.serialize(condition.condition, "condition", "Condition");

			result = ar.edit(condition);
			if(result)
				trigger.condition = condition.condition;
		}
		else{
			static_cast<EditOArchive&>(ar).serialize(trigger.condition, "condition", "Condition");
			result = ar.edit();
			if(result){
				trigger.condition = 0;
				static_cast<EditIArchive&>(ar).serialize(trigger.condition, "condition", "Condition");
			}		
		}
		return result;
	}

	bool editAction(Trigger& trigger, HWND hwnd) {
		EditArchive ar(hwnd, TreeControlSetup(0,0,500,400,"Scripts\\TreeControlSetups\\editActionSetup",true));
		static_cast<EditOArchive&>(ar).serialize(trigger.action, "action", "action");
		bool result = ar.edit();
		if(result){
			trigger.condition = 0;
			static_cast<EditIArchive&>(ar).serialize(trigger.action, "action", "action");
		}		
		return result;
	}

	bool editTrigger(Trigger& trigger, HWND hwnd) {
		EditArchive editArchive(hwnd, TreeControlSetup(0,0,500,400,"Scripts\\TreeControlSetups\\editTriggerSetup",true));
		bool result = editArchive.edit(trigger);
		return result;
	}
	
	void copyTrigger(Trigger& destination, const Trigger& source){
		destination = source;
		destination.action = 0;
		destination.condition = 0;
		EditOArchive oa;
		oa.serialize(source, "trigger", "trigger");
		EditIArchive ia(oa);
		ia.serialize(destination, "trigger", "trigger");
	}

	AttribEditorInterface& attribEditorInterface(){
		return ::attribEditorInterface();
	}

	void* malloc(size_t n) {
		return ::malloc(n);
	}

	void free(void* p) {
		::free(p);
	}
};

TriggerInterface& triggerInterface() {
	return Singleton<TriggerInterfaceImpl>::instance();
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
