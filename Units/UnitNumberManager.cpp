#include "stdafx.h"
#include "UnitNumberManager.h"

UnitNumberManager::Number::Number() 
: reserved(0)
{}

UnitNumberManager::Number::Number(const UnitNumber& data) 
: type(data.type), numberParameters(data.numberParameters), reserved(0) 
{
}

void UnitNumberManager::Number::serialize(Archive& ar)
{
 	ar.serialize(type, "type", "&Тип");
 	ar.serialize(numberParameters, "numberParameters", "&Количество");
}

int UnitNumberManager::Number::number() const
{
	return round(numberParameters.findByType(ParameterType::NUMBER_OF_UNITS, 1000)) - reserved;
}

UnitNumberManager::UnitNumberManager(const UnitNumbers& unitNumbers)
{
	UnitNumbers::const_iterator i;
	FOR_EACH(unitNumbers, i)
		numbers_.push_back(*i);
}

void UnitNumberManager::apply(const ArithmeticsData& arithmetics)
{
	Numbers::iterator i;
	FOR_EACH(numbers_, i)
		arithmetics.apply(i->numberParameters);
}

void UnitNumberManager::add(const ParameterSet& parameters)
{
	Numbers::iterator i;
	FOR_EACH(numbers_, i)
		i->numberParameters += parameters;
}

void UnitNumberManager::sub(const ParameterSet& parameters)
{
	Numbers::iterator i;
	FOR_EACH(numbers_, i)
		i->numberParameters.subClamped(parameters);
}

int UnitNumberManager::number(const UnitFormationTypeReference& type) const
{
	Numbers::const_iterator i;
	FOR_EACH(numbers_, i)
		if(i->type == type)
			return i->number();
	return -1;
}

void UnitNumberManager::reserve(const UnitFormationTypeReference& type, int counter)
{
	Numbers::iterator i;
	FOR_EACH(numbers_, i)
		if(i->type == type)
			i->reserved += counter;
}
