#include "stdafx.h"
#include "SourceTeleport.h"

#include "Serialization.h"

#include "..\Units\PositionGeneratorCircle.h"
#include "Squad.h"
#include "..\Units\UnitInterface.h"
#include "..\Units\IronLegion.h"

SourceTeleport::SourceTeleport() :
SourceDamage()
{
	notTeleportOwner_ = true;
}

void SourceTeleport::quant()
{
	if(!active()){
		__super::quant();
		return;
	}

	units_.clear();

	__super::quant();

	PositionGeneratorCircle<const LegionaryUnitList> positionGenerator;
	
	Vect2f teleportPosition = target_.unit() ? target_.unit()->position2D() : Vect2f(target_.position());
	positionGenerator.init(0, teleportPosition, &units_);

	LegionaryUnitList::const_iterator ui = units_.begin();

	while(ui != units_.end()){
		Vect2f point = positionGenerator.get(*ui);
		(*ui)->setPose(Se3f((*ui)->orientation(), To3D(point)), true);
		++ui;
	}

	kill();
}

void SourceTeleport::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(notTeleportOwner_, "notTeleportOwner", "Себя не телепортировать (для оружия)");
	ar.serialize(squadTypes_, "squadTypes", "Переносимые сквады");
}

bool SourceTeleport::setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target)
{
	target_ = *target;
	return true;
}

void SourceTeleport::apply(UnitBase* unit)
{
	__super::apply(unit);
	if(unit->alive() && unit->attr().isLegionary()){
		if(notTeleportOwner_)
			if(unit == owner())
				return;

		UnitLegionary* trooper = safe_cast<UnitLegionary*>(unit);
		
		if(std::find(squadTypes_.begin(), squadTypes_.end(), trooper->attr().squad) == squadTypes_.end())
			return;
		
		trooper->getSquadPoint()->clearOrders();
		trooper->getSquadPoint()->fireStop();

		units_.push_back(trooper);
	}
}
