#include "stdafx.h"
#include "Parameters.h"
#include "UnitAttribute.h"
#include "TypeLibraryImpl.h"
#include <shlwapi.h>
#include "Dictionary.h"
#include "Universe.h"

string getStringTokenByIndex(const char*, int);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ParameterType, Type, "Type")
REGISTER_ENUM_ENCLOSED(ParameterType, HEALTH, "Здоровье")
REGISTER_ENUM_ENCLOSED(ParameterType, HEALTH_RECOVERY, "Восстановление здоровья")
REGISTER_ENUM_ENCLOSED(ParameterType, ARMOR, "Броня")
REGISTER_ENUM_ENCLOSED(ParameterType, ARMOR_RECOVERY, "Восстановление брони")
REGISTER_ENUM_ENCLOSED(ParameterType, POSSESSION, "Владение")
REGISTER_ENUM_ENCLOSED(ParameterType, POSSESSION_RECOVERY, "Восстановление владения")
REGISTER_ENUM_ENCLOSED(ParameterType, POSSESSION_RECOVERY_BACK, "Восстановление владения после захвата")
REGISTER_ENUM_ENCLOSED(ParameterType, VELOCITY, "Скорость")
REGISTER_ENUM_ENCLOSED(ParameterType, RESOURCE_PICKING_TIME, "Время сбора ресурса")
REGISTER_ENUM_ENCLOSED(ParameterType, RESOURCE_PRODUCTIVITY_FACTOR, "Коэффициент производительности ресурсосборщиков (скорость сборки ресурса)")
REGISTER_ENUM_ENCLOSED(ParameterType, FIRE_RANGE, "Дальность стрельбы")
REGISTER_ENUM_ENCLOSED(ParameterType, FIRE_RANGE_MIN, "Минимальная дальность стрельбы")
REGISTER_ENUM_ENCLOSED(ParameterType, FIRE_TIME, "Время стрельбы")
REGISTER_ENUM_ENCLOSED(ParameterType, FIRE_DISPERSION, "Разброс стрельбы")
REGISTER_ENUM_ENCLOSED(ParameterType, RELOAD_TIME, "Время перезарядки")
REGISTER_ENUM_ENCLOSED(ParameterType, SIGHT_RADIUS, "Радиус видимости")
REGISTER_ENUM_ENCLOSED(ParameterType, NUMBER_OF_UNITS, "Максимальное количество юнитов")
REGISTER_ENUM_ENCLOSED(ParameterType, CONSTRUCTION_TIME_FACTOR_ON_WATER, "Коэффициент времени строительства на воде")
REGISTER_ENUM_ENCLOSED(ParameterType, VARIABLE, "Переменная (равенство при проверке)")
REGISTER_ENUM_ENCLOSED(ParameterType, OTHER, "Прочий")
END_ENUM_DESCRIPTOR_ENCLOSED(ParameterType, Type)


BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, Operation, "Operation")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, SET, "=")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, ADD, "+")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, ADD_PERCENT, "+%")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, SUB, "-")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, SUB_PERCENT, "-%")
END_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, Operation)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, CreationType, "CreationType")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, OLD, "Уже созданные")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, NEW, "Вновь создаваемые")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, ALL, "Все")
END_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, CreationType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, UnitType, "UnitType")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, TAKEN, "Юнит, получающий арифметику")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, TAKEN_TYPE, "Юниты того же типа, что и юнит, получающий арифметику")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, CHOSEN_TYPES, "Выбранные типы юнитов")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, ALL_TYPES, "Юниты всех типов")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, PLAYER, "Общие (игрока)")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, PLAYER_CAPACITY, "Емкость ресурсов игрока")
END_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, UnitType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, WeaponType, "WeaponType")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WEAPON_NONE, "Ни на какое оружие не влияет")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WEAPON_ANY, "На все оружие")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WEAPON_SHORT_RANGE, "На оружие ближнего боя")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WEAPON_LONG_RANGE, "На оружие дальнего боя")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WEAPON_TYPES, "На выбранные типы оружия")
END_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, WeaponType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, Address, "Address")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, UNIT, "Параметры юнита")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, UNIT_MAX, "Максимальные параметры юнита")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WEAPON, "Параметры оружия")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, SOURCE, "Источник оружия")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, DAMAGE, "Повреждения оружия")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, WATER_DAMAGE, "Повреждения от воды")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, LAVA_DAMAGE, "Повреждения от лавы")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, ICE_DAMAGE, "Повреждения от льда")
REGISTER_ENUM_ENCLOSED(ArithmeticsData, EARTH_DAMAGE, "Повреждения от земли")
END_ENUM_DESCRIPTOR_ENCLOSED(ArithmeticsData, Address)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ParameterShowSetting, ParameterClass, "ParameterType")
REGISTER_ENUM_ENCLOSED(ParameterShowSetting, LOGIC, "логический")
REGISTER_ENUM_ENCLOSED(ParameterShowSetting, PRODUCTION_PROGRESS, "прогресс производства")
REGISTER_ENUM_ENCLOSED(ParameterShowSetting, UPGRADE_PROGRESS, "прогресс апгрейда")
REGISTER_ENUM_ENCLOSED(ParameterShowSetting, FINISH_UPGRADE, "прогресс завершения апгрейда")
END_ENUM_DESCRIPTOR_ENCLOSED(ParameterShowSetting, ParameterClass)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ParameterShowSetting, AlignType, "AlignType")
REGISTER_ENUM_ENCLOSED(ParameterShowSetting, ALIGN_LEFT, "по левому краю")
REGISTER_ENUM_ENCLOSED(ParameterShowSetting, ALIGN_CENTER, "по центру")
END_ENUM_DESCRIPTOR_ENCLOSED(ParameterShowSetting, AlignType)

BEGIN_ENUM_DESCRIPTOR(ShowEvent, "ShowEvent")
REGISTER_ENUM(SHOW_AT_NOT_HOVER_OR_SELECT, "когда нет наведения и не выбран")
REGISTER_ENUM(SHOW_AT_SELECT, "при выборе")
REGISTER_ENUM(SHOW_AT_HOVER, "при наведении")
REGISTER_ENUM(SHOW_AT_HOVER_OR_SELECT, "при наведении или выборе")
REGISTER_ENUM(SHOW_AT_PARAMETER_INCREASE, "при увеличении параметра")
REGISTER_ENUM(SHOW_AT_PARAMETER_DECREASE, "при уменьшении параметра")
REGISTER_ENUM(SHOW_AT_BUILD, "при строительстве")
REGISTER_ENUM(SHOW_AT_BUILD_OR_UPGRADE, "при строительстве или апгрейде")
REGISTER_ENUM(SHOW_AT_DIRECT_CONTROL, "в прямом управлении")
REGISTER_ENUM(SHOW_ALWAYS, "всегда")
END_ENUM_DESCRIPTOR(ShowEvent)

WRAP_LIBRARY(ParameterTypeTable, "ParameterType", "Типы параметров", "Scripts\\Content\\ParameterType", 0, true);

WRAP_LIBRARY(ParameterValueTable, "ParameterValue", "Параметры", "Scripts\\Content\\ParameterValue", 0, true);

WRAP_LIBRARY(ParameterFormulaTable, "ParameterFormula", "Формулы параметров", "Scripts\\Content\\ParameterFormula", 0, true);

WRAP_LIBRARY(ParameterGroupTable, "ParameterGroup", "Группа параметров", "Scripts\\Content\\ParameterGroup", 0, false);

/////////////////////////////////
ParameterType::ParameterType(const char* name) : 
StringTableBase(name) 
{
	type_ = OTHER;
	counter_ = 0;
	setCounter();
}

void ParameterType::serialize(Archive& ar) 
{
	if(ar.isOutput() && !ar.isEdit()){
		setCounter();
	}

	StringTableBase::serialize(ar);
	ar.serialize(type_, "type", "&Тип");
	ar.serialize(counter_, "counter", "&Номер");
	ar.serialize(comment_, "comment", "Комментарий");
	//ar.serialize(fullName_, "fullName", 0);
	ar.serialize(tipsName_, "tipsName", "Название параметра (лок, ключ является меткой)");
}

void ParameterType::setCounter()
{
	if(!ParameterTypeTable::instance().empty()){
again:
		for(int i = 0; i < ParameterTypeTable::instance().size(); i++){
			if(&ParameterTypeTable::instance()[i] != this && ParameterTypeTable::instance()[i].type() == type()
		      && ParameterTypeTable::instance()[i].counter() == counter_){
				counter_++;
				goto again;
			}
		}
	}
}

void ParameterFormula::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar);
	if(ar.isEdit()){
		ParameterFormulaString formula;
		(FormulaString&)formula = formula_;
		formula.group_ = ParameterFormulaString::parameterGroup_;
		formula.value_ = ParameterFormulaString::parameterValue_;
		ar.serialize(formula, "formula", "Формула");
		formula_ = (FormulaString&)formula;
	}else{
		ar.serialize(formula_, "formula", 0);
	}
}

void ParameterValue::editorGroupMoveBefore(int index, int beforeIndex)
{
	ParameterTypeTable::instance().editorElementMoveBefore(index, beforeIndex);
}

void ParameterValue::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar);
	ar.serialize(value_, "value", "Значение (X)");
	ar.serialize(type_, "type", "&Тип");
	if(ar.isEdit()){
		ParameterFormulaString::parameterValue_ = value_;
		ParameterFormulaString::parameterGroup_ = group_;
	}
	ar.serialize(formula_, "formula", "Формула");
	ar.serialize(group_, "group", 0);
	if (ar.isInput ()) {
		state_ = NOT_CALCULATED;
	} 
	if (ar.isEdit()) {
		float calculated_value = value();
		ar.serialize(calculated_value, "calculated_value", "Вычисленное значение");
	}
	ar.serialize(can_modify_, "can_modify", "Изменяемый в Excel");
}

float ParameterValue::rawValue() const
{
	return value_;
}


const char* tokenEnd(const char* begin){
	if(*begin == '*')
		return ++begin;
	do{
		++begin;
	}while(*begin != '*' && *begin != '\0');

	return begin;
}

bool matchMask(const char* mask, const char* text)
{
	if(StrCmpI(mask, text) == 0)
		return true;

	const char* mask_it = mask;
	const char* text_it = text;

	while(*text_it != '\0'){
		if(*mask_it == '*'){
			++mask_it;
			if(*mask_it == '*')
				continue;
			if(*mask_it == '\0')
				return true;
			const char* nextToken = tokenEnd(mask_it);
			
			std::string token(mask_it, nextToken);
			
			if(text_it = const_cast<char*>(StrStrI(text_it, token.c_str())))
				text_it += nextToken - mask_it;
			else
				return false;

			mask_it = nextToken;
		}
		else{
			const char* nextToken = tokenEnd(mask_it);
			std::size_t len = nextToken - mask_it;
			if(strlen(text_it) < len || StrCmpNI(text_it, mask_it, len) != 0)
				return false;
			mask_it = nextToken;
			text_it += len;
		}

	}
	return true;
}


float ParameterValue::value () const 
{
	switch(state_) {
	case NOT_CALCULATED:
		state_ = CALCULATING;
		FormulaString::Dummy dummy;
		if (formula_->formula().evaluate(calculated_value_, value_, LookupParameter(group_), dummy, dummy, dummy) == FormulaString::EVAL_SUCCESS) {
			state_ = CALCULATED;
		}
	case CALCULATED:
		return calculated_value_;
	case CALCULATING:
	default:
		xassertStr(0 && "Циклическое обращение к значениям параметров:", c_str());
		return 0;
	};
}

/////////////////////////////////////
const float ParameterSet::eps = 0.1f;

ParameterSet::ParameterSet()
{}

ParameterSet& ParameterSet::operator*=(float k)
{
	Values::iterator i;
	FOR_EACH(values_, i)
		i->value *= k;
	return *this;
}

void ParameterSet::set(float value)
{
	Values::iterator i;
	FOR_EACH(values_, i)
		i->value = value;
}

ParameterSet& ParameterSet::operator+=(const ParameterSet& rhs)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = rhs.values_.begin();
	while(i != values_.end() && j != rhs.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index > j->index)
			++j;
		else{
			i->value += j->value;
			++i;
			++j;
		}
	}
	return *this;
}

float ParameterSet::dot(const ParameterSet& rhs) const
{
	float value = 0;
	Values::const_iterator i = values_.begin();
	Values::const_iterator j = rhs.values_.begin();
	while(i != values_.end() && j != rhs.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index > j->index)
			++j;
		else{
			value += i->value*j->value;
			++i;
			++j;
		}
	}
	return value;
}

void ParameterSet::subPositiveOnly(const ParameterSet& rhs)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = rhs.values_.begin();
	while(i != values_.end() && j != rhs.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index > j->index)
			++j;
		else{
			if(i->value > 0 && (i->value -= j->value) < 0)
				i->value = 0;
			++i;
			++j;
		}
	}
}

void ParameterSet::subClamped(const ParameterSet& rhs)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = rhs.values_.begin();
	while(i != values_.end() && j != rhs.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index > j->index)
			++j;
		else{
			if((i->value -= j->value) < 0)
				i->value = 0;
			++i;
			++j;
		}
	}
}

void ParameterSet::sub(const ParameterSet& rhs)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = rhs.values_.begin();
	while(i != values_.end() && j != rhs.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index > j->index)
			++j;
		else{
			i->value -= j->value;
			++i;
			++j;
		}
	}
}

void ParameterSet::scaleAdd(const ParameterSet& parameters, float k)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = parameters.values_.begin();
	while(i != values_.end() && j != parameters.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index > j->index)
			++j;
		else{
			i->value += j->value*k;
			++i;
			++j;
		}
	}
}

void ParameterSet::serialize(Archive& ar) 
{
	Values::iterator i;
	FOR_EACH(values_, i){
		const char* name = ParameterTypeTable::instance()[i->index].c_str();
		ar.serialize(i->value, transliterate(name).c_str(), name);
	}
}

void ParameterSet::write(XBuffer& buffer) const
{
	buffer < values_.size();
	Values::const_iterator i;
	FOR_EACH(values_, i)
		buffer < i->index < i->value;
}

void ParameterSet::read(XBuffer& buffer)
{
	values_.clear();
	int size;
	buffer > size;
	values_.reserve(size);
	while(size--){
		Value value;
		buffer > value.index > value.value;
		values_.push_back(value);
	}
}

void ParameterSet::showDebug(const Vect3f& position, const sColor4c& color) const
{
	XBuffer buffer(1000, 1);
	Values::const_iterator i;
	FOR_EACH(values_, i){
		const ParameterType& parameterType = ParameterTypeTable::instance()[i->index];
		const char* name = parameterType.c_str();
		const char* type = getEnumNameAlt(parameterType.type());
		buffer < name < " (" <= i->index < ", " < type < ") = " <= i->value < "\n";
	}
	show_text(position, buffer, color);
}

const char* ParameterSet::debugStr() const
{
	static XBuffer buffer(1000, 1);
	buffer.init();
	Values::const_iterator i;
	FOR_EACH(values_, i){
		const char* name = ParameterTypeTable::instance()[i->index].c_str();
		buffer < name < " = " <= i->value < "; ";
	}
	return buffer;
}


float ParameterSet::sum() const
{
	float result = 0;
	Values::const_iterator i;
	FOR_EACH(values_, i)
		result += i->value;
	return result;
}

float ParameterSet::health() const
{
	float result = FLT_INF;
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::HEALTH)
			result = min(result, i->value);
	return result < FLT_INF ? result : 1;
}

float ParameterSet::armor() const
{
	float result = FLT_INF;
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::ARMOR)
			result = min(result, i->value);
	return result < FLT_INF ? result : 1;
}

float ParameterSet::possession() const
{
	float result = FLT_INF;
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::POSSESSION)
			result = min(result, i->value);
	return result < FLT_INF ? result : 1;
}

float ParameterSet::findByIndex(int index, float defaultValue) const
{
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(i->index == index)
			return i->value;
	return defaultValue;
}

float ParameterSet::findByLabel(const char* label) const
{
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(!strcmp(ParameterTypeTable::instance()[i->index].label(), label))
			return i->value;
	return 0;
}

float ParameterSet::findByName(const char* name, float defaultValue) const
{
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(!strcmp(ParameterTypeTable::instance()[i->index].c_str(), name))
			return i->value;
	return defaultValue;
}

float ParameterSet::findByType(ParameterType::Type type, float defaultValue) const
{
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() == type)
			return i->value;
	return defaultValue;
}

bool ParameterSet::contain(const ParameterTypeReference& type) const
{
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(i->index == type.key())
			return true;
	return false;
}

void ParameterSet::setArmor(const ParameterSet& currentSet, const ParameterSet& maxSet)
{
	values_.clear();
	Values::const_iterator i;
	FOR_EACH(currentSet.values_, i){
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::ARMOR && i->value > FLT_EPS){
			for(int j = 0; j < ParameterTypeTable::instance().size(); j++)
				if(ParameterTypeTable::instance()[j].type() == ParameterType::HEALTH && ParameterTypeTable::instance()[j].counter() == ParameterTypeTable::instance()[i->index].counter()){
					values_.push_back(Value(maxSet.values_[i - currentSet.values_.begin()].value, j));
					break;
				}
		}
	}
}

void ParameterSet::setRecovery(const ParameterSet& parameters)
{
	// HEALTH_RECOVERY -> HEALTH
	// ARMOR_RECOVERY -> ARMOR

	values_.clear();
	Values::const_iterator i;
	FOR_EACH(parameters.values_, i){
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::HEALTH_RECOVERY){
			for(int j = 0; j < ParameterTypeTable::instance().size(); j++)
				if(ParameterTypeTable::instance()[j].type() == ParameterType::HEALTH && ParameterTypeTable::instance()[j].counter() == ParameterTypeTable::instance()[i->index].counter()){
					values_.push_back(Value(i->value, j));
					break;
				}
		}
		else if(ParameterTypeTable::instance()[i->index].type() == ParameterType::ARMOR_RECOVERY){
			for(int j = 0; j < ParameterTypeTable::instance().size(); j++)
				if(ParameterTypeTable::instance()[j].type() == ParameterType::ARMOR && ParameterTypeTable::instance()[j].counter() == ParameterTypeTable::instance()[i->index].counter()){
					values_.push_back(Value(i->value, j));
					break;	  
				}
		}
	}
}

void ParameterSet::setPossessionRecovery(const ParameterSet& parameters, const ParameterSet& parametersMax)
{
	// POSSESSION_RECOVERY -> POSSESSION

	values_.clear();
	Values::const_iterator i;
	FOR_EACH(parameters.values_, i){
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::POSSESSION_RECOVERY){
			for(int j = 0; j < ParameterTypeTable::instance().size(); j++)
				if(ParameterTypeTable::instance()[j].type() == ParameterType::POSSESSION && ParameterTypeTable::instance()[j].counter() == ParameterTypeTable::instance()[i->index].counter()){
					float valueMax = parametersMax.findByIndex(j);
					float valueCurr = parameters.findByIndex(j);
					values_.push_back(Value(valueCurr + i->value < valueMax ? i->value : valueMax - valueCurr, j));
					break;
				}
		}
	}
}

void ParameterSet::setPossessionRecoveryBack(const ParameterSet& parameters)
{
	// POSSESSION_RECOVERY_BACK -> POSSESSION

	values_.clear();
	Values::const_iterator i;
	FOR_EACH(parameters.values_, i){
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::POSSESSION_RECOVERY_BACK){
			for(int j = 0; j < ParameterTypeTable::instance().size(); j++)
				if(ParameterTypeTable::instance()[j].type() == ParameterType::POSSESSION && ParameterTypeTable::instance()[j].counter() == ParameterTypeTable::instance()[i->index].counter()){
					values_.push_back(Value(i->value, j));
					break;
				}
		}
	}
}

bool ParameterSet::restorePossessionRecoveryBack(const ParameterSet& parametersMax)
{
	float result = FLT_INF;
	int counter = -1;
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::POSSESSION)
			if(result > i->value){
				result = i->value;
				counter = ParameterTypeTable::instance()[i->index].counter();
			}

	if(counter == -1)
		return false;

	bool log = false;
	FOR_EACH(parametersMax.values_, i){
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::POSSESSION_RECOVERY_BACK){
			if(ParameterTypeTable::instance()[i->index].counter() == counter && i->value > FLT_EPS){
			  values_[i - parametersMax.values_.begin()].value = i->value;
			  log = true;
			}
			else
				values_[i - parametersMax.values_.begin()].value = 0;
		  }
	}
	return log;
}

void ParameterSet::updateRange(ParameterSet& prm_min, ParameterSet& prm_max) const
{
	xassert(prm_min.size() == prm_max.size());

	Values::const_iterator i;
	FOR_EACH(values_, i){
		if(ParameterTypeTable::instance()[i->index].type() == ParameterType::ARMOR || ParameterTypeTable::instance()[i->index].type() == ParameterType::HEALTH){
			Values::iterator j;
			FOR_EACH(prm_min.values_, j){
				if(j->index == i->index)
					break;
			}
			if(j != prm_min.values_.end()){
				j->value = min(j->value, i->value);

				int idx = j - prm_min.values_.begin();
				j = prm_max.values_.begin() + idx;

				j->value = max(j->value, i->value);
			}
			else {
				prm_min.values_.push_back(*i);
				prm_max.values_.push_back(*i);
			}
		}
	}
}

void ParameterSet::set(float value, ParameterType::Type type) 
{
	Values::iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() == type)
			i->value = value;
}

void ParameterSet::mask(ParameterType::Type type) 
{
	Values::iterator i;
	FOR_EACH(values_, i)
		if(ParameterTypeTable::instance()[i->index].type() != type)
			i->value = 0;
}

void ParameterSet::mask(const ParameterSet& set) 
{
	Values::iterator i;
	FOR_EACH(values_, i){
		ParameterTypeReference type;
		type.setKey(i->index);
		if(!set.contain(type))
			i->value = 0;
	}
}

void ParameterSet::set(const ParameterSet& rhs)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = rhs.values_.begin();
	while(i != values_.end() && j != rhs.values_.end()){
		if(i->index == j->index){
			i->value = j->value;
			++i;
			++j;
		}
		else if(i->index < j->index)
			++i;
		else
			++j;
	}
}

float ParameterSet::minFraction(const ParameterSet& parametersMax) const
{
	xassert(values_.size() == parametersMax.values_.size());

	float fraction = 1.f;
	for(int i = 0; i < values_.size(); i++){
		ParameterType::Type type = ParameterTypeTable::instance()[values_[i].index].type();
		if(type == ParameterType::HEALTH || type == ParameterType::ARMOR || type == ParameterType::POSSESSION){
			float f = values_[i].value/(parametersMax.values_[i].value + FLT_EPS);
			if(fraction > f)
				fraction = f;
		}
	}

	return fraction;
}

void ParameterSet::scaleByProgress(float factor)
{
	for(int i = 0; i < values_.size(); i++){
		ParameterType::Type type = ParameterTypeTable::instance()[values_[i].index].type();
		if(type == ParameterType::HEALTH || type == ParameterType::ARMOR || type == ParameterType::POSSESSION)
			values_[i].value *= factor;
	}
}

void ParameterSet::scaleType(ParameterType::Type type, float factor)
{
	for(int i = 0; i < values_.size(); i++){
		if(ParameterTypeTable::instance()[values_[i].index].type() == type)
			values_[i].value *= factor;
	}
}

bool ParameterSet::contain(const ParameterSet& subSet) const
{
	//if(values_.size() < subSet.values_.size())
	//	return false;
	bool valid = false;
	Values::const_iterator i = values_.begin();
	Values::const_iterator j = subSet.values_.begin();
	while(i != values_.end() && j != subSet.values_.end()){
		if(i->index < j->index){
			++i;
		}
		else if(i->index > j->index)
			return false;
		else{
			++i;
			++j;
			valid = true;
		}
	}

	return valid;
}	

bool ParameterSet::containAny(const ParameterSet& subSet) const
{
	for(Values::const_iterator i = values_.begin(); i != values_.end(); ++i){
		for(Values::const_iterator j = subSet.values_.begin(); j != subSet.values_.end(); ++j){
			if(i->index == j->index)
				return true;
		}
	}

	return false;
}

bool ParameterSet::above(const ParameterSet& subSetlevel) const
{
	Values::const_iterator i = values_.begin();
	Values::const_iterator j = subSetlevel.values_.begin();
	
	if(j == subSetlevel.values_.end())
		return true;
	if(i == values_.end())
		return false;
	
	while(i != values_.end() && j != subSetlevel.values_.end()){
		if(i->index < j->index){
			++i;
		}
		else if(i->index == j->index){
			if(ParameterTypeTable::instance()[i->index].type() != ParameterType::VARIABLE ? i->value + eps < j->value : fabsf(i->value - j->value) > eps)
				return false;
			++i;
			++j;
		}
		else
			return false;
	}

	return j == subSetlevel.values_.end();
}

float ParameterSet::progress(const ParameterSet& resource1, const ParameterSet& resource2) const
{
	float progress = 1.f;
	Values::const_iterator need = values_.begin();
	Values::const_iterator is1 = resource1.values_.begin();
	Values::const_iterator is1end = resource1.values_.end();
	Values::const_iterator is2 = resource2.values_.begin();
	Values::const_iterator is2end = resource2.values_.end();

	while(need != values_.end())
	{
		while(is1 != is1end && is1->index < need->index)
			++is1;
		while(is2 != is2end && is2->index < need->index)
			++is2;
		
		if(is1 == is1end || is1->index > need->index){ // в первом наборе параметра нет, смотрим только во втором
			if(is2 == is2end || is2->index > need->index)
				return 0.f; // не хватает некоторых типов параметров
			else if(ParameterTypeTable::instance()[need->index].type() != ParameterType::VARIABLE){
				if(need->value > is2->value)
					progress = min(progress, ::clamp(is2->value / need->value, 0.f, 1.f));
			}
			else if(fabsf(need->value - is2->value) > eps)
				return 0.f; // если хоть один параметр-переменная не равен, то общий прогресс 0
		}
		else if(is2 == is2end || is2->index > need->index){ // во втором наборе параметра нет, смотрим только в первом
			if(ParameterTypeTable::instance()[need->index].type() != ParameterType::VARIABLE){
				if(need->value > is1->value)
					progress = min(progress, ::clamp(is1->value / need->value, 0.f, 1.f));
			}
			else if(fabsf(need->value - is1->value) > eps)
				return 0.f;
		}
		else { // параметр есть в обоих наборах
			if(ParameterTypeTable::instance()[need->index].type() != ParameterType::VARIABLE){
				float val = max(is1->value, is2->value);
				if(need->value > val)
					progress = min(progress, ::clamp(val / need->value, 0.f, 1.f));
			}
			else if(fabsf(need->value - is1->value) > eps && fabsf(need->value - is2->value) > eps)
				return 0.f;
		}
		
		++need;
	}
	return progress;

}

bool ParameterSet::below(const ParameterSet& resource1, const ParameterSet& resource2) const
{
	Values::const_iterator i = values_.begin();
	Values::const_iterator j1 = resource1.values_.begin();
	Values::const_iterator j2 = resource2.values_.begin();
	
	while(i != values_.end()){
		bool repeat = false;

		if(j1 != resource1.values_.end())
			if(j1->index < i->index){
				++j1;
				repeat = true;
			}
			else if(j1->index == i->index){
				if(ParameterTypeTable::instance()[i->index].type() != ParameterType::VARIABLE ? i->value > j1->value + eps : fabsf(i->value - j1->value) > eps)
					return false;
				if(++i == values_.end())
					break;
				++j1;
				repeat = true;
			}

		if(j2 != resource2.values_.end())
			if(j2->index < i->index){
				++j2;
				repeat = true;
			}
			else if(j2->index == i->index){
				if(ParameterTypeTable::instance()[i->index].type() != ParameterType::VARIABLE ? i->value > j2->value + eps : fabsf(i->value - j2->value) > eps)
					return false;
				++i;
				++j2;
				repeat = true;
			}

		if(!repeat)
			return false;
	}

	return true;
}


void ParameterSet::clamp(const ParameterSet& upperBound)
{
	Values::iterator i = values_.begin();
	Values::const_iterator j = upperBound.values_.begin();
	while(i != values_.end() && j != upperBound.values_.end()){
		if(i->index < j->index){
			++i;
		}
		else if(i->index > j->index)
			++j;
		else{
			i->value = ::clamp(i->value, 0, j->value);
			++i;
			++j;
		}
	}
}

bool ParameterSet::zero() const 
{
	Values::const_iterator i;
	FOR_EACH(values_, i)
		if(i->value > 0)
			return false;
	return true;
}

bool ParameterSet::zero(const ParameterSet& filter) const
{
	Values::const_iterator i = values_.begin();
	Values::const_iterator j = filter.values_.begin();
	while(i != values_.end() && j != filter.values_.end()){
		if(i->index < j->index){
			++i;
		}
		else if(i->index > j->index)
			++j;
		else{
			if(j->value > 0 && i->value > 0)
				return false;
			++i;
			++j;
		}
	}

	return true;
}

void ParameterSet::printSufficient(XBuffer& buffer, const ParameterSet& resource, const char* enableColor, const char* disableColor) const
{
	if(resource.values_.empty() || values_.empty())
		return;

	Values::const_iterator i = values_.begin();
	Values::const_iterator j = resource.values_.begin();
	while(i != values_.end() && j != resource.values_.end()){
		if(i->index < j->index)
			++i;
		else if(i->index == j->index){
			buffer < (i->value < j->value ? disableColor : enableColor) < ParameterTypeTable::instance()[i->index].tipsName() < " " <= round(j->value) < " ";
			++i;
			++j;
		}
		else
			++j;
	}
	buffer < '\0';
}

void ParameterSet::toString(XBuffer& buffer) const
{
	if(values_.empty())
		return;

	Values::const_iterator it;
	FOR_EACH(values_, it)
		buffer < ParameterTypeTable::instance()[it->index].tipsName() < " " <= round(it->value) < " ";
	buffer < '\0';
}

//////////////////////////////////////////////////////
struct ParameterValueReferenceLess
{
	bool operator()(const ParameterValueReference& p1, const ParameterValueReference& p2) const {
		return p1->type() < p2->type();
	}
};

struct ParameterValueReferenceEq
{
	bool operator()(const ParameterValueReference& p1, const ParameterValueReference& p2) const {
		return p1->type() == p2->type();
	}
};

bool ParameterCustom::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isOutput() && !ar.isEdit())
		std::sort(vector_.begin(), vector_.end(), ParameterValueReferenceLess());
	
	bool nodeExists = ar.serialize(vector_, name, nameAlt);
	
	if(ar.isInput()){
		values_.clear();
		Vector::iterator i;
		FOR_EACH(vector_, i){
			Values::iterator vi;
			FOR_EACH(values_, vi)
				if(vi->index == (*i)->type().key())
					break;
			if(vi == values_.end())
				values_.push_back(Value((*i)->value(), (*i)->type().key()));
			else
				xassertStr(0 && "Параметры одинакового типа:", (string(nameAlt ? nameAlt : "") + " - " + (*i)->c_str()).c_str());
		}
	}
	return nodeExists;
}

void ParameterCustom::add(const ParameterCustom& parameters)
{
	vector_.insert(vector_.end(), parameters.vector_.begin(), parameters.vector_.end());
}

//////////////////////////////////////////////////////
ArithmeticsData::ArithmeticsData()
{
	creationType = ALL;
	unitType = PLAYER;
	weaponType = WEAPON_NONE;
	address = UNIT;
	influenceInStatistic_ = false;

	operation = SET;
	value = 0;

	invertOnApply_ = false;
	inverted_ = false;
	restoreIterator_ = restoreVector_.end();
}

void ArithmeticsData::apply(ParameterSet& parameters) const
{
	ParameterSet::Values::iterator j;
	FOR_EACH(parameters.values_, j){
		if(j->index == parameterType.key()){
			if(!inverted_){
				float oldValue = j->value;
 				switch(operation){
				case SET:
					j->value = value;
					break;
				case ADD:
					j->value += value;
					break;
				case ADD_PERCENT:
					j->value += j->value*value/100.f;
					break;
				case SUB:
					j->value -= value;
					break;
				case SUB_PERCENT:
					j->value -= j->value*value/100.f;
					break;
				}
				if(invertOnApply_)
					restoreVector_.push_back(j->value - oldValue);
			}
			else{
				xassert(restoreIterator_ != restoreVector_.end());
				j->value -= *restoreIterator_++;
				if(j->value < 0)
					j->value = 0;
			}
			break;
		}
	}
}

void ArithmeticsData::apply(ParameterSet& parameters, const ParameterSet& parametersMax) const
{
	ParameterSet::Values::const_iterator jm = parametersMax.values_.begin();
	for(ParameterSet::Values::iterator j = parameters.values_.begin(); j != parameters.values_.end(); ++j, ++jm){
		if(j->index == parameterType.key()){
			if(!inverted_){
				float oldValue = j->value;
 				switch(operation){
				case SET:
					j->value = value;
					break;
				case ADD:
					j->value += value;
					break;
				case ADD_PERCENT:
					j->value += j->value*value/100.f;
					break;
				case SUB:
					j->value -= value;
					break;
				case SUB_PERCENT:
					j->value -= j->value*value/100.f;
					break;
				}
				if(j->value > jm->value)
					j->value = jm->value;
				if(invertOnApply_)
					restoreVector_.push_back(j->value - oldValue);
			}
			else{
				xassert(restoreIterator_ != restoreVector_.end());
				j->value -= *restoreIterator_++;
				if(j->value < 0)
					j->value = 0;
			}
			break;
		}
	}
}

void ArithmeticsData::operator*=(float k)
{
	value *= k;
}

void ArithmeticsData::reserve(ParameterSet& parameters) const
{
	ParameterSet::Values::iterator j;
	FOR_EACH(parameters.values_, j)
		if(j->index == parameterType.key())
			return;
	parameters.values_.insert(j, ParameterSet::Value(0, parameterType.key()));
}

bool ArithmeticsData::checkWeapon(const WeaponPrm* weaponPrm) const
{
	if(!weaponPrm)
		return false;

	switch(weaponType){
	case WEAPON_ANY:
		return true;
	case WEAPON_SHORT_RANGE:
		return weaponPrm->isShortRange();
	case WEAPON_LONG_RANGE:
		return !weaponPrm->isShortRange();
	case WEAPON_TYPES: {
		WeaponPrmReferences::const_iterator i;
		FOR_EACH(weapons, i)
			if(weaponPrm == *i)
				return true;
		return false;
		}
	default:
		return true;
	}
}

void ArithmeticsData::serialize(Archive& ar)
{
	ar.serialize(unitType, "unitType", "Юниты, на которые влияет");
	if(unitType & ~(PLAYER | PLAYER_CAPACITY))
		ar.serialize(creationType, "creationType", "Юниты созданные или новые");

	ar.serialize(weaponType, "weaponType", "Влияние на оружие");

	if(unitType & CHOSEN_TYPES)
		ar.serialize(attributes, "attributes", "Типы юнитов");
	if(weaponType == WEAPON_TYPES)
		ar.serialize(weapons, "weapons", "Типы оружия");
	
	ar.serialize(address, "address", "На что влиять");
	
	if((address & (WEAPON | SOURCE | DAMAGE)) && weaponType == WEAPON_NONE)
		weaponType = WEAPON_ANY;

	if(unitType & ALL_TYPES)
		unitType &= ~(TAKEN | TAKEN_TYPE | CHOSEN_TYPES);
	if(unitType & CHOSEN_TYPES)
		unitType &= ~(TAKEN | TAKEN_TYPE);
	if(unitType & TAKEN_TYPE)
		unitType &= ~TAKEN;

	ar.serialize(parameterType, "parameterType", "&Тип параметра");
	ar.serialize(operation, "operation", "&Операция");
	ar.serialize(value, "value", "&Значение");

	if(universe() && universe()->userSave()){
		ar.serialize(invertOnApply_, "invertOnApply", 0);
		ar.serialize(inverted_, "inverted", 0);
		ar.serialize(restoreVector_, "restoreVector", 0);
		if(ar.isInput()){
			if(inverted_)
				restoreIterator_ = restoreVector_.begin();
			else
				restoreIterator_ = restoreVector_.end();
		}
	}

    ar.serialize(influenceInStatistic_, "influenceInStatistic", "Отражать в статистике");
}


//////////////////////////////////////////////////////

void ParameterArithmetics::serialize(Archive& ar)
{
	ar.serialize(data, "data", "Список арифметики");
}

void ParameterArithmetics::operator*=(float k) 
{ 
	Data::iterator i;
	FOR_EACH(data, i)
		*i *= k; 
}

void ParameterArithmetics::setInvertOnApply() 
{ 
	Data::iterator i;
	FOR_EACH(data, i)
		i->setInvertOnApply(); 
}

void ParameterArithmetics::setInverted() 
{ 
	Data::iterator i;
	FOR_EACH(data, i)
		i->setInverted(); 
}

void ParameterArithmetics::reserve(ParameterSet& parameters) const
{ 
	Data::const_iterator i;
	FOR_EACH(data, i)
		i->reserve(parameters); 
}

void ParameterArithmetics::apply(ParameterSet& parameters) const
{ 
	Data::const_iterator i;
	FOR_EACH(data, i)
		i->apply(parameters); 
}

void ParameterArithmetics::operator*=(const ParameterArithmeticsMultiplicator& multi)
{
	ParameterArithmeticsMultiplicator::Values::const_iterator i;
	FOR_EACH(multi.values_, i){
		Data::iterator j;
		FOR_EACH(data, j)
			if(j->parameterType == i->type)
				j->value *= i->factor;
	}
}

ParameterArithmeticsMultiplicator::Value::Value() 
: factor(1.f) 
{}

void ParameterArithmeticsMultiplicator::Value::serialize(Archive& ar)
{
	ar.serialize(type, "type", "Тип");
	ar.serialize(factor, "factor", "Коэффициент");
}

bool ParameterArithmeticsMultiplicator::serialize(Archive& ar, const char* name, const char* nameAlt) 
{
	return ar.serialize(values_, name, nameAlt);
}

///////////////////////////////////////////////

ParameterConsumer::ParameterConsumer()
{
	clear();
}

void ParameterConsumer::clear()
{
	totalSum_ = 0;
	timer_ = 0;
	timerMax_ = 1;
	showControllers_.clear();
}

void ParameterConsumer::start(const UnitInterface* showOwner, const ParameterSet& total, float time)
{
	totalSum_ = total.sum();
	current_ = total;
	delta_ = total;
	delta_ *= (logicPeriodSeconds/max(time, logicPeriodSeconds));
	timer_ = 0;
	timerMax_ = max(time/logicPeriodSeconds, 1.f);
	showControllers_.create(showOwner, current_, ShowChangeParametersController::VALUES);
}

bool ParameterConsumer::addQuant(const ParameterSet& resource, float factor)
{
	ParameterSet delta = resource;
	delta.clamp(current_);
	delta *= factor;
	current_.subClamped(delta);
	timer_ += factor;
	showControllers_.update(current_);
	return totalSum_ > FLT_EPS ? current_.zero() : timer_ >= timerMax_;
}

float ParameterConsumer::progress() const
{
	return totalSum_ > FLT_EPS ? 1 - current_.sum()/totalSum_ : timer_/timerMax_;
}

void ParameterConsumer::setProgress(float progress) 
{
	if(totalSum_ > FLT_EPS)
		current_ *= (1 - progress)*totalSum_/(current_.sum() + 0.00001f);
	else
		timer_ = progress*timerMax_;
}

void ParameterConsumer::serialize(Archive& ar)
{
	ar.serialize(totalSum_, "totalSum", 0);
	ar.serialize(current_, "current", 0);
	ar.serialize(delta_, "delta", 0);
	ar.serialize(timer_, "timer", 0);
	ar.serialize(timerMax_, "timerMax", 0);
}

///////////////////////////////////////////////

ParameterShowSetting::ParameterShowSetting() :
	type_(LOGIC),
	dataType(0),
	paramOffset(0.0f, 0.0f),
	alignType(ALIGN_CENTER),
	showEvent(SHOW_ALWAYS),
	notShowEmpty(false),
	minValColor(1.0f,0.0f,0.0f,1.0f),
	maxValColor(0.0f,1.0f,0.0f,1.0f),
	borderColor(0.0f, 0.0f, 0.0f, 1.0f),
	backgroundColor(0.0f, 0.0f, 0.0f, 0.25f),
	barLength(30.0f),
	delayShowTime(0)
{
}

ParameterCustom* ParameterShowSetting::possibleParameters_ = 0;

void ParameterShowSetting::serialize(Archive& ar)
{
	ar.serialize(type_, "type_", "&Тип параметра");
	
	switch(type_){
	case LOGIC:
		if(ar.isEdit()){
			string combo;
			ComboListString comboList;
			if(possibleParameters_ && !possibleParameters_->vector_.empty()){
				ParameterCustom::Vector::iterator it;
				FOR_EACH(possibleParameters_->vector_, it){
					combo += combo.empty() ? "": "|";
					combo += it->c_str();
					if((*it)->type() == paramTypeRef){
						if(!comboList.value().empty()){
							XBuffer msg;
							msg < TRANSLATE("Следующие параметры имееют одинаковые типы:\n");
							msg < "\t" < comboList.value().c_str();
							msg < "\nи\t" < it->c_str();
							msg < TRANSLATE("\nТип: ") < paramTypeRef.c_str(); 
							xassertStr(0, msg);
						}
						comboList = it->c_str();
					}
				}
				comboList.setComboList(combo.c_str());
				if(comboList.value().empty())
					comboList = getStringTokenByIndex(combo.c_str(), 0);
				//xassert(indexInComboListString(comboList.comboList(), comboList) >= 0);
				ar.serialize(comboList, "comboList", "&Параметр");
				//xassert(indexInComboListString(comboList.comboList(), comboList) >= 0);
				paramTypeRef = possibleParameters_->vector_[clamp(indexInComboListString(comboList.comboList(), comboList), 0, possibleParameters_->vector_.size())]->type();
			}
		}
		else
			ar.serialize(paramTypeRef, "paramTypeRef", "Параметр");
		break;
	case UPGRADE_PROGRESS:
		ar.serialize(dataType, "dataType", "Номер апгрейда");
		break;
	case FINISH_UPGRADE:
		ar.serialize(dataType, "dataType", "Номер апгрейда юнита из которого апгрейдились");
		break;
	}
	
	ar.serialize(paramOffset, "paramOffset", "Смещение");
	ar.serialize(alignType, "alignType", "Выравнивание");
	ar.serialize(showEvent, "showEvent", "Когда выводить");

	ar.serialize(notShowEmpty, "notShowEmpty", "не показывать пустой");
	
	if(showEvent == SHOW_AT_PARAMETER_INCREASE || showEvent == SHOW_AT_PARAMETER_DECREASE)
		if(type_ == LOGIC)
			ar.serialize(delayShowTime, "delayShowTime", "Время показа параметра (мс) (работает при увеличении или уменьшении)");
		else
			showEvent = SHOW_AT_SELECT;

	ar.serialize(minValColor, "minValColor", "Цвет при минимуме");
	ar.serialize(maxValColor, "maxValColor", "Цвет при максимуме");
	ar.serialize(borderColor, "borderColor", "Цвет рамки");
	ar.serialize(backgroundColor, "backgroundColor", "Цвет подложки");
	ar.serialize(barLength, "barLength", "Длина полоски");

	ar.serialize(showChangeSettings, "showChangeSettings", "Показ изменения параметров взлетающим текстом");
}


float ParameterFormulaString::parameterValue_ = 0.0f;
ParameterGroupReference ParameterFormulaString::parameterGroup_;

void ParameterFormulaString::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(group_, "group", 0);
	ar.serialize(value_, "value", 0);
}


