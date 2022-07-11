#include "stdafx.h"

#include "TypeLibraryImpl.h"
#include "UnitAttribute.h"

WRAP_LIBRARY(ExplodeTable, "ExplodeTable", "Типы взрывов", "Scripts\\Engine\\ExplodeTable", 0, false);

WRAP_LIBRARY(AbnormalStateTypeTable, "AbnormalStateType", "Типы воздействий", "Scripts\\Content\\AbnormalStateType", 1, true);

void ExplodeProperty::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar); 
	ar.serialize(enableExplode, "enableExplode", "enableExplode"); 
	ar.serialize(animatedDeath, "animatedDeath", "animatedDeath"); 
	ar.serialize(enableRagDoll, "enableRagDoll", "enableRagDoll");
	ar.serialize(alphaDisappear, "alphaDisappear", "alphaDisappear"); 
	ar.serialize(explodeFactors, "explodeAxisFactors", "explodeAxisFactors" ); 
	ar.serialize(selfPower, "selfPower", "selfPower"); 
	ar.serialize(liveTime, "liveTime", "liveTime"); 
	ar.serialize(z_gravity, "z_gravity", "z_gravity"); 
	ar.serialize(friction, "friction", "friction");
	ar.serialize(restitution, "restitution", "restitution"); 
}

// -----------------------------------------
int AbnormalStateType::maskCounter_ = 0;

AbnormalStateType::AbnormalStateType(const char* name) 
: StringTableBase(name)
{
	priority = 1;
}

void AbnormalStateType::serialize(Archive& ar)
{
	if(ar.isInput() && !ar.isEdit()){
		mask_ = 1 << maskCounter_++;
		xassert(maskCounter_ <= 31 && "Общее количество воздействий должно быть меньше 32");
	}

	StringTableBase::serialize(ar);
	ar.serialize(priority, "priority", "&Приоритет");
}

// -----------------------------------------

DeathAttribute::DeathAttribute()
{
	enableExplodeFantom = false;
	explodeFactor = 1.0f;
}

void DeathAttribute::serialize(Archive& ar)
{
	ar.serialize(explodeReference, "explodeReference", "Тип смерти");
	ar.serialize(explodeFactor, "explodeFactor", "Коэффициент мощности взрыва");
	ar.serialize(effectAttr, "effectAttr", "Эффект для осколков");
	ar.serialize(effectAttrFly, "effectAttrFly", "Эффект на трупе");
	ar.serialize(enableExplodeFantom, "enableExplodeFantom", "Оставлять осколки на мире");

	ar.serialize(sources, "sources", "Источники, остающиеся после гибели");
}

// -----------------------------------------
UnitColor UnitColor::ZERO;

AbnormalStateEffect::AbnormalStateEffect()
{
	deathAttribute_ = 0;
}

AbnormalStateEffect::~AbnormalStateEffect()
{
	delete deathAttribute_;
}

void AbnormalStateEffect::serialize(Archive& ar)
{
	ar.serialize(type_, "type", "&тип");

	/// CONVERSION 2006-06-05
	if(!ar.serialize(effectColor_, "effectColor", "цвет юнита")){
		sColor4c colors_[3];
		if(!ar.serializeArray(colors_, "effectColors", 0)){
			sColor4f colors[3];
			ar.serializeArray(colors, "colors", 0);
			for(int i = 0; i < 3; i++)
				colors_[i] = sColor4c(colors[i]);
		}
		effectColor_.color = colors_[1];
		effectColor_.isBrightColor = false;
	}
	/// ^^^^^^
	ar.serialize(effectAttribute_, "effectAttribute", "эффект");
	ar.serialize(soundReference_, "soundReference", "звук");

	bool hasDeathAttribute = deathAttribute_;
	ar.serialize(hasDeathAttribute, "hasDeathAttribute", "свои настройки гибели");
	if(hasDeathAttribute && !deathAttribute_)
		deathAttribute_ = new DeathAttribute;
	else if(!hasDeathAttribute && deathAttribute_){
		delete deathAttribute_;
		deathAttribute_ = 0;
	}
	if(deathAttribute_)
		ar.serialize(*deathAttribute_, "deathAttribute", "Гибель");
}

// -----------------------------------------

AbnormalStateAttribute::AbnormalStateAttribute()
{
	duration_ = 1.0f;
	durationRnd_ = 0.0f;
	useArithmetics_ = false;
	freeze_ = false;
}

void AbnormalStateAttribute::serialize(Archive& ar)
{
	ar.serialize(type_, "type", "&тип");

	ar.serialize(duration_, "duration", "длительность действия");
	ar.serialize(durationRnd_, "durationRnd", "разброс длительности действия");

	damage_ *= 1.f/logicPeriodSeconds;
	ar.serialize(damage_, "damage", "повреждения в секунду");
	damage_ *= logicPeriodSeconds;

	ar.serialize(useArithmetics_, "useArithmetics", "Использовать арифметику");
	if(useArithmetics_)
		ar.serialize(arithmetics_, "arithmetics", "Арифметика");
	
	ar.serialize(freeze_, "freeze", "Замораживать");
}

void AbnormalStateAttribute::applyParameterArithmetics(const ArithmeticsData& arithmetics)
{
	if(arithmetics.address & ArithmeticsData::DAMAGE)
		arithmetics.apply(damage_);
}

void AbnormalStateAttribute::applyParameterArithmeticsOnDamage(const ArithmeticsData& parameterArithmetics)
{
	parameterArithmetics.apply(damage_);
}

