#include "StdAfx.h"
#include "shlwapi.h"
#include "TriggerExport.h"
#include "Serialization.h"
#include "Dictionary.h"
#include "..\util\Actions.h"
#include "..\Game\Universe.h"

BEGIN_ENUM_DESCRIPTOR(ExtendedPlayerType, "ExtendedPlayerType")
REGISTER_ENUM(EXTENDED_PLAYER_MY_CLAN, "Мой клан")
REGISTER_ENUM(EXTENDED_PLAYER_ENEMY, "Враг")
REGISTER_ENUM(EXTENDED_PLAYER_ANY, "Любой (кроме мира)")
END_ENUM_DESCRIPTOR(ExtendedPlayerType)

BEGIN_ENUM_DESCRIPTOR(ColorType, "StrategyColor")
REGISTER_ENUM(STRATEGY_RED, "Красный")
REGISTER_ENUM(STRATEGY_GREEN, "Зеленый")
REGISTER_ENUM(STRATEGY_BLUE, "Синий")
REGISTER_ENUM(STRATEGY_YELLOW, "Желтый")
REGISTER_ENUM(STRATEGY_COLOR_0, "STRATEGY_COLOR_0")
REGISTER_ENUM(STRATEGY_COLOR_1, "STRATEGY_COLOR_1")
REGISTER_ENUM(STRATEGY_COLOR_2, "STRATEGY_COLOR_2")
REGISTER_ENUM(STRATEGY_COLOR_3, "STRATEGY_COLOR_3")
REGISTER_ENUM(STRATEGY_COLOR_4, "STRATEGY_COLOR_4")
REGISTER_ENUM(STRATEGY_COLOR_5, "STRATEGY_COLOR_5")
REGISTER_ENUM(STRATEGY_COLOR_MAX, "STRATEGY_COLOR_MAX")
END_ENUM_DESCRIPTOR(ColorType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TriggerLink, Type, "Type")
REGISTER_ENUM_ENCLOSED(TriggerLink, THIN, "Тонкая")
REGISTER_ENUM_ENCLOSED(TriggerLink, THICK, "Толстая")
END_ENUM_DESCRIPTOR_ENCLOSED(TriggerLink, Type)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Trigger, State, "State")
REGISTER_ENUM_ENCLOSED(Trigger, SLEEPING, "Проверяет входные связи")
REGISTER_ENUM_ENCLOSED(Trigger, CHECKING, "Проверяет условия")
REGISTER_ENUM_ENCLOSED(Trigger, WORKING, "Выполняется")
REGISTER_ENUM_ENCLOSED(Trigger, DONE, "Выполнен")
END_ENUM_DESCRIPTOR_ENCLOSED(Trigger, State)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TriggerChain, CheckType, "CheckType")
REGISTER_ENUM_ENCLOSED(TriggerChain, LOGIC_TRIGGER, "Логический триггер")
REGISTER_ENUM_ENCLOSED(TriggerChain, INTERFACE_TRIGGER, "Интерфейсный триггер")
REGISTER_ENUM_ENCLOSED(TriggerChain, IGNORE_PAUSE, "Интерфейсный триггер с игнорировнием паузы")
REGISTER_ENUM_ENCLOSED(TriggerChain, NO_CHECK, "Проверка отключена (только для главного меню!!!)")
END_ENUM_DESCRIPTOR_ENCLOSED(TriggerChain, CheckType)

bool Condition::scanWait_;
const TriggerChain* Action::triggerChain_;

//----------------------------------------------------------
void Condition::serialize(Archive& ar) 
{
	ar.serialize(inverted_, "inverted", "Инвертировано");
	ar.serialize(state_, "state", "_Текущее состояние");
	ar.serialize(internalColor_, "internalColor", 0);
}

void CPointSerialized::serialize(Archive& ar) 
{
	ar.serialize(x, "x", "x");
	ar.serialize(y, "y", "y");
}

void CRectSerialized::serialize(Archive& ar) 
{
	ar.serialize(left, "left", "left");
	ar.serialize(top, "top", "top");
	ar.serialize(right, "right", "right");
	ar.serialize(bottom, "bottom", "bottom");
}

//----------------------------------------------------------

void Condition::writeInfo(XBuffer& buffer, string offset, bool debug) const
{
	buffer < offset.c_str();
	if(inverted())
		buffer < TRANSLATE("НЕ") < " ";

	const char* name = TRANSLATE(triggerInterface().conditionName(*this));
	const char* str = name;
	for(; *str; ++str)
		if(*str == '\\'){
			++str;
			break;
		}
	if(*str)
		name = str;

	buffer < name;
	if(debug)
		buffer < " = " < (state() ? "1" : "0");
	buffer < "\r\n";
}

//----------------------------------------------------------
TriggerLink::TriggerLink() 
{
	colorType_ = STRATEGY_RED;
	autoRestarted_ = THIN;
	active_ = false;
	child = parent = 0;
}

void TriggerLink::activate(TriggerChain& triggerChain)
{
	active_ = true;
	triggerChain.activateTrigger(child);
}

void TriggerLink::deactivate(TriggerChain& triggerChain)
{
	active_ = false;
	if(!child->active())
		triggerChain.deactivateTrigger(child);
}

void TriggerLink::serialize(Archive& ar) 
{
	ar.serialize(triggerName_, "triggerName", 0);
	ar.serialize(colorType_, "color", 0);
	ar.serialize(autoRestarted_, "type", 0);
	ar.serialize(active_, "active_", 0);
	
	ar.serialize(parentOffset_, "parentOffset", 0);
	ar.serialize(childOffset_, "childOffset", 0);
}

//----------------------------------------------------------
Trigger::Trigger() 
{
	condition = 0;
	action = 0;
	state_ = SLEEPING;
	executionCounter_ = 0;
	internalColor_ = 0;
	selected_ = false;
	breakWhenActivate_ = false;
	randomizeOutcomingLinks_ = false;
	isContext_ = false;
	indexScan_ = 0;
	onlyIfAi_ = false;

	extendedScan_ = false;
	extendedPlayerType_ = EXTENDED_PLAYER_ANY;
	unitsPerQuant_ = 1;
	indexScanPlayer_ = 0;
}

void Trigger::serialize(Archive& ar) 
{
	ar.serialize(name_, "name", "&Имя");
	ar.serialize(unitsPerQuant_, "unitsPerQuant", "Обрабатывать юнитов за квант");
	ar.serialize(extendedScan_, "extendedScan", "Расширенное сканирование контекста");
	ar.serialize(extendedPlayerType_, "extendedPlayerType", "Диапазон игроков");
	xassert("BUG: Проблема с копированием триггеров!" && (!condition || condition->numRef() == 1));
	ar.serialize(condition, "condition", "Условие");
	xassert("BUG: Проблема с копированием триггеров!" && (!action || action->numRef() == 1));
	ar.serialize(action, "action", "Действие");
	ar.serialize(outcomingLinks_, "outcomingLinks", "_Исходящие связи");
	ar.serialize(state_, "state_", "_Текущее состояние");
	ar.serialize(executionCounter_, "executionCounter_", "_Запускался");
	ar.serialize(internalColor_, "internalColor_", 0);
	ar.serialize(randomizeOutcomingLinks_, "randomizeOutcomingLinks", "Рандомизировать исходящие стрелки");
	ar.serialize(breakWhenActivate_, "breakWhenActivate", "Остановиться при активации (дебаг)");
		
	ar.serialize(cellIndex_, "cellIndex", 0);
	ar.serialize(boundingRect_, "boundingRect", 0);
	if (ar.isInput() && action){
		isContext_ = action->isContext();
		if(condition)
			isContext_ |= condition->isContext();
		ActionForAI* actionAi = dynamic_cast<ActionForAI*>(&*action);
		if(actionAi)
			onlyIfAi_ = actionAi->onlyIfAi;
	}
}

void Trigger::setName(const char* name)
{
	IncomingLinksList::iterator i;
	FOR_EACH(incomingLinks_, i)
		(*i)->setTriggerName(name);
	name_ = name;
}

bool Trigger::active() const 
{ 
	if(state() == CHECKING || state() == WORKING)
		return true;
	
	IncomingLinksList::const_iterator li;
	FOR_EACH(incomingLinks_, li)
		if((*li)->active())
			return true;

	return false;
}

void Trigger::initState(TriggerChain& triggerChain)
{
	state_ = SLEEPING;
	if(action)
		action->clear();
    OutcomingLinksList::iterator li;
	FOR_EACH(outcomingLinks_, li)
		li->deactivate(triggerChain);
	executionCounter_ = 0;
}

void Trigger::setState(State state, TriggerChain& triggerChain, int activateOffset)
{
	state_ = state;
	switch(state){
	case WORKING: {
		IncomingLinksList::iterator li;
		FOR_EACH(incomingLinks_, li){
			if((*li)->active()){ // Деактивировать связи из родительского триггера других цветов
				Trigger* trigger = (*li)->parent;
				OutcomingLinksList::iterator lj;
				FOR_EACH(trigger->outcomingLinks_, lj){
					if(lj->getType() != (*li)->getType()){
						if(!lj->autoRestarted()){
							if(lj->child->state() == CHECKING) // Выключить другие триггера
								lj->child->state_ = lj->child->executionCounter_ ? DONE : SLEEPING;
							lj->deactivate(triggerChain);
						}
					}
				}
				if(!(*li)->autoRestarted()) // Деактивировать, если тонкая
					(*li)->deactivate(triggerChain);
			}
		}
		executionCounter_++;
		break; }

	case DONE: {
		int size = outcomingLinks_.size();
		for(int i = 0; i < size; i++)
			outcomingLinks_[(i + activateOffset) % size].activate(triggerChain);
		if(!active())
			triggerChain.deactivateTrigger(this);
		break; }
	}
}

const char* Trigger::displayText() const
{
	static XBuffer buffer(5000);
	buffer.init();
	buffer < name();
	if(action)
		buffer < " - " < action->name();
	return buffer;
}

const char* Trigger::debugDisplayText(bool debug) const
{
	static XBuffer buffer(5000);
	buffer.init();
	buffer < name();
	if(action)
		buffer < " - " < TRANSLATE(action->name());
	if(debug)
		buffer < " (" <= executionCounter_ < ")";
	if(condition){
		buffer < "\r\n";
		condition->writeInfo(buffer, "", debug);
		buffer -= 2;
		buffer < "";
	}
	return buffer;
}

bool Trigger::removeLinkByChild(Trigger * child)
{
	typedef OutcomingLinksList::iterator Iterator;
	Iterator i = outcomingLinks().begin(), e = outcomingLinks().end();
	for(; i != e; ++i)
	{
		TriggerLink& link = *i;
		if (link.child == child)
		{
/*!
	раскоментировать, если 
	из remove_link удален вызов метода initialize()

//			typedef IncomingLinksList::iterator Iterator;
//			Iterator res = find(child->incomingLinks_.begin(), 
//								child->incomingLinks_.end(), &link);
//			assert (res != child->incomingLinks_.end());
//			child->incomingLinks_.erase(res);

*/

			outcomingLinks().erase(i);
			return true;
		}
	}
	return false;
}

bool Trigger::isChild(const Trigger& child) const
{
	OutcomingLinksList::const_iterator i;
	FOR_EACH(outcomingLinks(), i) 
		if(i->child == &child)
			return false;
	return true;
}

//------------------------------------------------------------
void TriggerEvent::serialize(Archive& ar) 
{
	ar.serialize(event, "event", 0);
	ar.serialize(triggerName, "triggerName", 0);
	ar.serialize(state, "state", 0);
}	

//------------------------------------------------------------
int TriggerChain::currentVersion_ = 0;

TriggerChain::TriggerChain() 
{
	version_ = currentVersion_;
	checkType_ = LOGIC_TRIGGER;
	initialize();
}

void TriggerChain::serializeProperties(Archive& ar) 
{
	if(ar.isInput() && !ar.isEdit()){
		bool ignorePause, checkDeterminism;
		ar.serialize(ignorePause, "ignorePause", "Игнорировать паузу");
		ar.serialize(checkDeterminism, "checkDeterminism", "Проверять на повторяемость");
		if(checkDeterminism)
			checkType_ = LOGIC_TRIGGER;
		else if(ignorePause)
			checkType_ = IGNORE_PAUSE;
		else
			checkType_ = INTERFACE_TRIGGER;
	}
	ar.serialize(checkType_, "checkType", "Тип проверки триггера");
}

void TriggerChain::serialize(Archive& ar) 
{
	ar.serialize(name, "name", 0);
	ar.serialize(version_, "version", 0);
	serializeProperties(ar);
	ar.serialize(triggers, "triggers", "Триггера");

	ar.serialize(boundingRect_, "boundingRect", 0);
	ar.serialize(viewRect_, "viewRect", 0);
	ar.serialize(triggerEvents_, "triggerEvents", 0);
	
	if(ar.isInput())
		initialize();
}

void TriggerChain::buildLinks()
{
	TriggerList::iterator ti;
	FOR_EACH(triggers, ti)
		ti->incomingLinks_.clear();

	FOR_EACH(triggers, ti){
		OutcomingLinksList::iterator li;
		FOR_EACH(ti->outcomingLinks_, li){
			Trigger* trigger = find(li->triggerName());
			if(trigger && trigger != &*ti){
				trigger->incomingLinks_.push_back(&*li);
				li->parent = &*ti;
				li->child = trigger;
			}
			else{
				li = ti->outcomingLinks_.erase(li);
				--li;
			}
		}
	}

	activeTriggers_.clear();
	activeTriggersEvent_.clear();
	FOR_EACH(triggers, ti)
		if(ti->active()){
			Condition* condition = (*ti).condition;
			if(condition && condition->isEvent())
				activeTriggersEvent_.push_back(&(*ti));
			else
				activeTriggers_.push_back(&(*ti));
		}
}

void TriggerChain::initialize()
{
	if(triggers.empty() || strcmp(triggers.front().name(), "START"))
		triggers.insert(triggers.begin(), Trigger());
	triggers.front().setName("START");

	buildLinks();

	if(checkType_ != NO_CHECK){
		TriggerList::iterator ti;
		FOR_EACH(triggers, ti){
			if(ti->condition && !ti->condition->allowable(checkType_ == LOGIC_TRIGGER)){
				XBuffer message;
				message < name.c_str() < " - " < ti->name();
				xassertStr("Недопустимое условие в триггере" && 0, message);
			}
		}
	}
}

void TriggerChain::activateTrigger(Trigger* trigger)
{
	Condition* condition = trigger->condition;
	if(condition && condition->isEvent()){
		if(std::find(activeTriggersEvent_.begin(), activeTriggersEvent_.end(), trigger) == activeTriggersEvent_.end())
			activeTriggersEvent_.push_back(trigger);
	}
	else
		if(std::find(activeTriggers_.begin(), activeTriggers_.end(), trigger) == activeTriggers_.end())
			activeTriggers_.push_back(trigger);
}

void TriggerChain::deactivateTrigger(Trigger* trigger)
{
	Condition* condition = trigger->condition; 
	if(condition && condition->isEvent())
		activeTriggersEvent_.erase(remove(activeTriggersEvent_.begin(), activeTriggersEvent_.end(), trigger), activeTriggersEvent_.end());
	else
		activeTriggers_.erase(remove(activeTriggers_.begin(), activeTriggers_.end(), trigger), activeTriggers_.end());
}

void TriggerChain::restart(bool resetToStart)
{
	TriggerList::iterator ti;
	FOR_EACH(triggers, ti)
		ti->initState(*this);

	activeTriggers_.clear(); 
	activeTriggersEvent_.clear();

	if(resetToStart){
		Trigger* trigger = find("START");
		trigger->state_ = Trigger::CHECKING;
		activeTriggers_.push_back(trigger);
	}
}

TriggerChain& TriggerChain::operator=(const TriggerChain& rhs)
{
	triggers = rhs.triggers;
	boundingRect_ = rhs.boundingRect_;
	viewRect_ = rhs.viewRect_;
	buildLinks();
	return *this;
}

bool TriggerChain::operator==(const TriggerChain& rhs) const
{	
	if(triggers.size() != rhs.triggers.size())
		return false;

	for(int i = 0; i < triggers.size(); i++)
		if(strcmp(triggers[i].name(), rhs.triggers[i].name()))
			return false;

	return true;
}

Trigger* TriggerChain::find(const char* name)
{
	TriggerList::iterator ti;
	FOR_EACH(triggers, ti)
		if(!strcmp(ti->name(), name))
			return &*ti;
	return 0;
}

Trigger* TriggerChain::findBySubString(const char* subString, int occurence)
{
	TriggerList::iterator ti;

	int index = 0;
	FOR_EACH(triggers, ti){
		if(StrStrI(ti->name(), subString)){
			if(index == occurence)
				return &*ti;
			++index;
		}
	}
	return 0;
}

TriggerLink* TriggerChain::addLink(int parentIndex, int childIndex, int type) 
{
	Trigger& parent = const_cast<Trigger&>(triggers[parentIndex]);
	Trigger& child = const_cast<Trigger&>(triggers[childIndex]);

	parent.outcomingLinks_.push_back(TriggerLink());
	TriggerLink& newLink = parent.outcomingLinks_.back();
	newLink.setTriggerName(child.name());
	newLink.autoRestarted_ = static_cast<TriggerLink::Type>(type);
	newLink.parent = &parent;
	newLink.child = &child;

	buildLinks();
	return &newLink;
}

bool TriggerChain::removeLink(int parentIndex, int childIndex) 
{
	Trigger& parent = const_cast<Trigger&>(triggers[parentIndex]);
	Trigger& child = const_cast<Trigger&>(triggers[childIndex]);
	if (parent.removeLinkByChild(&child))
	{
		//! перед удалением исправить removeLinkByChild(&child))
		buildLinks();
		return true;
	}
	return false;
}

Trigger* TriggerChain::addTrigger(Trigger const& trigger)
{
	// triggers.push_back(trigger);
	// buildLinks();
	// return &triggers.back();
	triggers.push_back(Trigger());
	triggerInterface().copyTrigger(triggers.back(), trigger);
	buildLinks();
	return &triggers.back();
}

Trigger* TriggerChain::insertTrigger(int pos, Trigger const& trigger){
	TriggerList::iterator itr = triggers.insert(triggers.begin() + pos, trigger);
	if (itr == triggers.end())
		return NULL;
	buildLinks();
	return &*itr;
}

void TriggerChain::removeTrigger(int triggerIndex)
{
	TriggerList::iterator forErase = triggers.begin();
	std::advance(forErase, triggerIndex);
	triggers.erase(forErase);
	buildLinks();
}

int TriggerChain::triggerIndex(const Trigger& trigger) const
{
	if(&trigger < &triggers.front() || &triggers.back() < &trigger)
		return -1;
	return (&trigger - &triggers.front());
}


void TriggerChain::addLogRecord(const Trigger& trigger, const char* event)
{
#ifndef _FINAL_VERSION_
	triggerEvents_.push_back(TriggerEvent());
	TriggerEvent& record = triggerEvents_.back();
	record.event = event;
	record.triggerName = trigger.name();
	record.state = trigger.state_;
#endif
}

void TriggerChain::setLogRecord(int index)
{
	if(index < 0 || index >= triggerEvents_.size())
		return;
	
	restart(true);

	for(int i = 0; i <= index; i++){
		TriggerEvent& event = triggerEvents_[i];
		if(event.event == "RESET_TO_START"){
			restart(true);
			continue;
		}
		else if(event.event == "RESET_TO_CURRENT")
			restart(true);

		Trigger* trigger = find(event.triggerName.c_str());
		if(trigger)
			trigger->setState(event.state, *this);
	}
}

bool TriggerChain::isLogValid() const
{
	//return !triggerEvents_.empty();
	return true;
}

void TriggerChain::setLogData(const TriggerEventList& data)
{
	triggerEvents_ = data;
}

//----------------------------------------------
Action::Action() 
{
	aiPlayer_ = 0;
}

const char* Action::name() const
{
	const char* name = triggerInterface().actionName(*this);
	for(const char* str = name; *str; ++str)
		if(*str == '\\')
			return ++str;
	return name;
}

