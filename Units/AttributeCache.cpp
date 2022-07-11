#include "stdafx.h"
#include "AttributeCache.h"
#include "UnitAttribute.h"

AttributeCache::AttributeCache(const AttributeBase& attribute)
{
	parameters_ = parametersMax_ = attribute.parametersInitial;

	waterEffect_ = attribute.waterEffect;
	lavaEffect_ = attribute.lavaEffect;
	iceEffect_ = attribute.iceEffect;	
	earthEffect_ = attribute.earthEffect;
}

void AttributeCache::applyArithmetics(const ArithmeticsData& arithmetics)
{
	if(arithmetics.address & ArithmeticsData::UNIT_MAX)
		arithmetics.apply(parametersMax_);
	if(arithmetics.address & ArithmeticsData::UNIT)
		arithmetics.apply(parameters_, parametersMax_);
	if(arithmetics.address & ArithmeticsData::WATER_DAMAGE)
		waterEffect_.applyParameterArithmeticsOnDamage(arithmetics);
	if(arithmetics.address & ArithmeticsData::LAVA_DAMAGE)
		lavaEffect_.applyParameterArithmeticsOnDamage(arithmetics);
	if(arithmetics.address & ArithmeticsData::ICE_DAMAGE)
		iceEffect_.applyParameterArithmeticsOnDamage(arithmetics);
	if(arithmetics.address & ArithmeticsData::EARTH_DAMAGE)
		earthEffect_.applyParameterArithmeticsOnDamage(arithmetics);
}

void AttributeCache::serialize(Archive& ar)
{
	ar.serialize(parameters_, "parameters", 0);
	ar.serialize(parametersMax_, "parametersMax", 0);

	ar.serialize(waterEffect_, "waterEffect", 0);
	ar.serialize(lavaEffect_, "lavaEffect", 0);
	ar.serialize(iceEffect_, "iceEffect", 0);
	ar.serialize(earthEffect_, "earthEffect", 0);
}
