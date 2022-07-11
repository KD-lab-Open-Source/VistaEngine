#include "StdAfx.h"
#include "shlwapi.h"
#include "TriggerExport.h"
#include "Serialization\Serialization.h"
#include "Serialization\Dictionary.h"
#include "Serialization\SerializationFactory.h"
#include "Serialization\EnumDescriptor.h"
#include "Serialization\Decorators.h"
#include "Serialization\MultiArchive.h"
#include "XMath\ComboListColor.h"

// Для StrStrI
#pragma message("Automatically linking with shlwapi.lib") 
#pragma comment(lib, "shlwapi.lib") 

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

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ConditionSwitcher, Type, "Type")
REGISTER_ENUM_ENCLOSED(ConditionSwitcher, AND, "И")
REGISTER_ENUM_ENCLOSED(ConditionSwitcher, OR, "ИЛИ")
END_ENUM_DESCRIPTOR_ENCLOSED(ConditionSwitcher, Type)

const TriggerChain* Action::triggerChain_;

const char* Trigger::currentTriggerName_ = "";

//----------------------------------------------------------

void Condition::serialize(Archive& ar) 
{
	if(ar.isEdit())
		ar.serialize(NotDecorator(inverted_), "inverted", "^");
	else
		ar.serialize(inverted_, "inverted", "Инвертировано");
	ar.serialize(state_, "state", 0);
	ar.serialize(internalColor_, "internalColor", 0);
}

bool Condition::checkDebug(UnitActing* unit) 
{ 
	state_ = check(unit) ? !inverted() : inverted();
	return state_;
}


//----------------------------------------------------------

ConditionSwitcher::ConditionSwitcher() 
{
	type = AND; 
}

void ConditionSwitcher::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(type, "type", "&<");
	ar.serialize(conditions, "conditions", "^Условия");
	if(ar.isInput()){
		Conditions::iterator ci;
		FOR_EACH(conditions, ci)
			if(!*ci){
				ci = conditions.erase(ci);
				--ci;
			}
	}
}

bool ConditionSwitcher::isContext(ContextFilter& filter) const 
{
	bool result = false;
	Conditions::const_iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci && (*ci)->isContext(filter))
			result = true;

	return result;
}

bool ConditionSwitcher::allowable(bool forLogic) const
{
	Conditions::const_iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci && !(*ci)->allowable(forLogic))
			return false;

	return true;
}

bool ConditionSwitcher::isEvent() const 
{
	Conditions::const_iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci && (*ci)->isEvent())
			return true;

	return false;
}

bool ConditionSwitcher::check() const
{ 
	if(conditions.empty())
		return true;

	if(conditions.size() == 1)
		return conditions.front()->checkDebug();

	if(type == AND){
		Conditions::const_iterator ci;
		FOR_EACH(conditions, ci)
			if(!(*ci)->checkDebug())
				return false;
		return true;
	}
	else{
		Conditions::const_iterator ci;
		FOR_EACH(conditions, ci)
			if((*ci)->checkDebug())
				return true;
		return false;
	}
}

bool ConditionSwitcher::checkDebug(UnitActing* unitToCheck)
{
	if(conditions.empty())
		return state_ = true;

	if(type == AND){
		state_ = true;
		Conditions::const_iterator ci;
		FOR_EACH(conditions, ci)
			if(!(*ci)->checkDebug(unitToCheck))
				state_ = false;
		return state_;
	}
	else{
		state_ = false;
		Conditions::const_iterator ci;
		FOR_EACH(conditions, ci)
			if((*ci)->checkDebug(unitToCheck))
				state_ = true;
		return state_;
	}
}

void ConditionSwitcher::remove(Condition* condition)
{
	xassert(condition);

	Conditions::iterator it = find(conditions.begin(), conditions.end(), condition);
	if(it != conditions.end())
		conditions.erase(it);
	else
		xassert(0);
}

void ConditionSwitcher::replace(Condition* from, Condition* to)
{
	xassert(from && to);

	Conditions::iterator it = find(conditions.begin(), conditions.end(), from);
	if(it != conditions.end()){
		ShareHandle<Condition> oldValue = *it;
		*it = to;
	}
	else
		xassert(0);
}

void ConditionSwitcher::add(Condition* condition)
{
	conditions.push_back(condition);
}

UnitActing* ConditionSwitcher::eventContextUnit() const
{
	Conditions::const_iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci){
			UnitActing* unit = (*ci)->eventContextUnit();
			if(unit)
				return unit;
		}

		return 0;
}

void ConditionSwitcher::checkEvent(const Event& event) 
{ 
	Conditions::iterator ci;
	FOR_EACH(conditions, ci)
		if((*ci) && (*ci)->isEvent())
			(*ci)->checkEvent(event);
}

void ConditionSwitcher::clear() 
{
	Conditions::iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci)
			(*ci)->clear();
}

void ConditionSwitcher::clearContext() 
{
	Conditions::iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci)
			(*ci)->clearContext();
}

void ConditionSwitcher::setPlayer(Player* aiPlayer) 
{ 
	Condition::setPlayer(aiPlayer);
	Conditions::iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci)
			(*ci)->setPlayer(aiPlayer);
}

void ConditionSwitcher::writeInfo(XBuffer& buffer, string offset, bool debug) const
{
	if(conditions.empty())
		return;

	buffer < offset.c_str();
	if(inverted())
		buffer < TRANSLATE("НЕ") < " ";

	if(conditions.size() == 1){
		conditions.front()->writeInfo(buffer, offset, debug);
		return;
	}
	buffer < (type == AND ? TRANSLATE("И") : TRANSLATE("ИЛИ"));
	if(debug)
		buffer < " = " < (state() ? "1" : "0");
	buffer < "\r\n";
	offset += "    ";
	Conditions::const_iterator ci;
	FOR_EACH(conditions, ci)
		if(*ci){
			(*ci)->writeInfo(buffer, offset, debug);
			if(type == AND ? !(*ci)->state() : (*ci)->state())
				debug = false;
		}
}

//----------------------------------------------------------

void Condition::writeInfo(XBuffer& buffer, string offset, bool debug) const
{
	buffer < offset.c_str();
	if(inverted())
		buffer < TRANSLATE("НЕ") < " ";

	const char* name = TRANSLATE(FactorySelector<Condition>::Factory::instance().find(this).nameAlt());
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
Color4c TriggerLink::colors[STRATEGY_COLOR_MAX] = 
{
	Color4c::GREEN, 
	Color4c::BLUE,
	Color4c::RED,
	Color4c::YELLOW,
	Color4c::CYAN,
	Color4c::MAGENTA,
	Color4c(255, 128, 0),
	Color4c(198, 198, 0),
	Color4c(100, 0, 0),
	Color4c(0, 100, 0)
};

Color4c TriggerLink::debugColors[2] = 
{
	Color4c(128, 128, 128),
	Color4c::RED
};


TriggerLink::TriggerLink() 
{
	colorType_ = STRATEGY_RED;
	autoRestarted_ = THIN;
	active_ = false;
	child = parent = 0;
	parentOffset_ = childOffset_ = Vect2f::ZERO;
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
	if(ar.isEdit()){
		ColorContainer comboList;
		for(int i = 0; i < STRATEGY_COLOR_MAX; i++)
			comboList.push_back(Color4f(colors[i]));
		ComboListColor color(comboList, Color4f(colors[colorType_]));
		ar.serialize(color, "color", "Цвет");
		colorType_ = (ColorType)color.index();
	}
	else
		ar.serialize(colorType_, "color", "Цвет");
	ar.serialize(autoRestarted_, "type", "Тип");
	ar.serialize(active_, "active_", 0);
	
	ar.serialize(parentOffset_, "parentOffset", 0);
	ar.serialize(childOffset_, "childOffset", 0);

	if(ar.isInput()){
		childOffset_.x = clamp(childOffset_.x, -Trigger::SIZE_X/2, Trigger::SIZE_X/2);
		childOffset_.y = clamp(childOffset_.y, -Trigger::SIZE_Y/2, Trigger::SIZE_Y/2);

		parentOffset_.x = clamp(parentOffset_.x, -Trigger::SIZE_X/2, Trigger::SIZE_X/2);
		parentOffset_.y = clamp(parentOffset_.y, -Trigger::SIZE_Y/2, Trigger::SIZE_Y/2);
	}
}

Vect2f TriggerLink::parentPoint() const 
{ 
	return parent->leftTop() + Trigger::gridSize()*.5f + Vect2f(parentOffset_); 
}

Vect2f TriggerLink::childPoint() const 
{ 
	return child->leftTop() + Trigger::gridSize()*.5f + Vect2f(childOffset_); 
}

static bool cut(Vect2f& p0, Vect2f& p1, Vect2f& u0, Vect2f& u1)
{
	Vect2f d12 = p0 - u0;
	Vect2f d1 = p1 - p0;
	Vect2f d2 = u1 - u0;
	float d = d1.x*d2.y - d1.y*d2.x;
	if(fabsf(d) < FLT_EPS)
		return false;

	float dInv = 1.f/d;
	float t1 = (d12.y*d2.x - d2.y*d12.x)*dInv;
	float t2 = (-d12.x*d1.y + d1.x*d12.y)*dInv;

	if(t1 > 0 && t1 < 1.f && t2 > 0 && t2 < 1.f){
		p1 = p0 + d1*t1;
		return true;
	}
	return false;
}

void TriggerLink::setParentPoint(const Vect2f& point)
{
	(Vect2i&)parentOffset_ = point - parent->leftTop() - Trigger::gridSize()*.5f; 
	fixParentOffset();
}

void TriggerLink::fixParentOffset()
{
	parentOffset_.x = clamp(parentOffset_.x, -Trigger::SIZE_X/2, Trigger::SIZE_X/2);
	parentOffset_.y = clamp(parentOffset_.y, -Trigger::SIZE_Y/2, Trigger::SIZE_Y/2);

	Vect2f p0 = childPoint();
	Vect2f p1 = parentPoint();
	Vect2f leftTop = parent->leftTop();
	bool log = cut(p0, p1, leftTop, leftTop + Vect2f(Trigger::SIZE_X, 0))
		| cut(p0, p1, leftTop, leftTop + Vect2f(0, Trigger::SIZE_Y))
		| cut(p0, p1, leftTop + Trigger::gridSize(), leftTop + Vect2f(Trigger::SIZE_X, 0))
		| cut(p0, p1, leftTop + Trigger::gridSize(), leftTop + Vect2f(0, Trigger::SIZE_Y));
	if(log)
		(Vect2i&)parentOffset_ = p1 - leftTop - Trigger::gridSize()*.5f; 
}

void TriggerLink::setChildPoint(const Vect2f& point)
{
	(Vect2i&)childOffset_ = point - child->leftTop() - Trigger::gridSize()*.5f; 
	fixChildOffset();
}

void TriggerLink::fixChildOffset()
{
	childOffset_.x = clamp(childOffset_.x, -Trigger::SIZE_X/2, Trigger::SIZE_X/2);
	childOffset_.y = clamp(childOffset_.y, -Trigger::SIZE_Y/2, Trigger::SIZE_Y/2);

	Vect2f p0 = parentPoint();
	Vect2f p1 = childPoint();
	Vect2f leftTop = child->leftTop();
	bool log = cut(p0, p1, leftTop, leftTop + Vect2f(Trigger::SIZE_X, 0))
		| cut(p0, p1, leftTop, leftTop + Vect2f(0, Trigger::SIZE_Y))
		| cut(p0, p1, leftTop + Trigger::gridSize(), leftTop + Vect2f(Trigger::SIZE_X, 0))
		| cut(p0, p1, leftTop + Trigger::gridSize(), leftTop + Vect2f(0, Trigger::SIZE_Y));
	if(log)
		(Vect2i&)childOffset_ = p1 - leftTop - Trigger::gridSize()*.5f; 
}

//----------------------------------------------------------
Color4c Trigger::debugColors[DONE + 1] = {
	Color4c(192, 192, 192), // SLEEPING
	Color4c::YELLOW, // CHECKING
	Color4c::RED, // WORKING
	Color4c::GREEN //	DONE 
};

Trigger::Trigger() 
{
	condition = 0;
	action = 0;
	state_ = SLEEPING;
	executionCounter_ = 0;
	color_ = Color4c(128, 255, 128);
	selected_ = false;
	breakWhenActivate_ = false;
	randomizeOutcomingLinks_ = false;
	isContext_ = false;
	unitIndex_ = 0;
	listIndex_ = 0;
	onlyIfAi_ = false;

	extendedScan_ = false;
	unitsPerQuant_ = 1;
	playerIndex_ = 0;
	
	cellIndex_.x = cellIndex_.y = 0;
}

void Trigger::serialize(Archive& ar) 
{
	ar.serialize(name_, "name", "&Имя");
	
	if(!ar.serialize(color_, "color", "Цвет")){ // CONVERSION
		int internalColor_;
		ar.serialize(internalColor_, "internalColor_", 0);
		color_.setGDI(internalColor_); 
	}

	if(!ar.isEdit() || isContext_){
		ar.serialize(unitsPerQuant_, "unitsPerQuant", "Обрабатывать юнитов за квант для контекстных условий");
		ar.serialize(extendedScan_, "extendedScan", "Сканирование контекста по всем игрокам");
	}
	xassert("BUG: Проблема с копированием триггеров!" && (!condition || condition->numRef() == 1));
	ar.serialize(condition, "condition", "<Условие");
	xassert("BUG: Проблема с копированием триггеров!" && (!action || action->numRef() == 1));
	ar.serialize(action, "action", "<Действие");
	ar.serialize(outcomingLinks_, "outcomingLinks", 0);
	ar.serialize(state_, "state_", 0);
	ar.serialize(executionCounter_, "executionCounter_", 0);
	ar.serialize(randomizeOutcomingLinks_, "randomizeOutcomingLinks", "Рандомизировать исходящие стрелки");
	ar.serialize(breakWhenActivate_, "breakWhenActivate", "Остановиться при активации (дебаг)");
		
	ar.serialize(cellIndex_, "cellIndex", 0);
	if(ar.isInput()){
		cellIndex_.x = clamp(cellIndex_.x, -100, 100);
		cellIndex_.y = clamp(cellIndex_.y, -100, 100);
		contextFilter_ = ContextFilter();
		if(action){
			isContext_ = action->isContext(contextFilter_);
			onlyIfAi_ = action->auxType() == Action::ONLY_IF_AI;
		}
		if(condition)
			isContext_ |= condition->isContext(contextFilter_);
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
					if(lj->colorType() != (*li)->colorType()){
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
	if(action){
		buffer < " - " < TRANSLATE(action->name());
		if(debug)
			buffer < " (" <= executionCounter_ < ")";
	}
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

void TriggerChain::save() 
{
	restart(true);
	triggerEvents_.clear();

	MultiOArchive(name.c_str(), "..\\..\\ContentBin\\Triggers", "scr", ARCHIVE_TEXT).serialize(*this, "TriggerChain", 0);
}

void TriggerChain::load(const char* fileName) 
{
	MultiIArchive ia(ARCHIVE_TEXT);
	if(ia.open(fileName, "..\\..\\ContentBin\\Triggers", "scr"))
		ia.serialize(*this, "TriggerChain", 0);
	name = fileName;
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
	ar.serialize(triggers, "triggers", 0);

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

string TriggerChain::uniqueName(const char* _name)
{
	int counter = 0;
	string name = _name;
	while(find(name.c_str())){
		while(!name.empty() && isdigit((unsigned char)name[name.size() - 1]))
			name.erase(name.size() - 1);
		name += XBuffer() <= counter++;
	}
	return name;
}

void TriggerChain::removeTrigger(int triggerIndex)
{
	triggers.erase(triggers.begin() + triggerIndex);
	buildLinks();
}

void TriggerChain::renameTrigger(const char* oldName, const char* newName)
{
	TriggerList::iterator ti;
	FOR_EACH(triggers, ti){
		OutcomingLinksList::iterator li;
		FOR_EACH(ti->outcomingLinks(), li)
			if(!strcmp(li->triggerName(), oldName))
				li->setTriggerName(newName);
		if(!strcmp(ti->name(), oldName))
			ti->setName(newName);
	}
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
	const char* name = FactorySelector<Action>::Factory::instance().find(this).nameAlt();

	for(const char* str = name; *str; ++str)
		if(*str == '\\')
			return ++str;
	return name;
}



