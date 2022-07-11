#ifndef __SOURCE_TELEPOR_H__
#define __SOURCE_TELEPOR_H__

#include "SourceBase.h"
#include "SourceEffect.h"
#include "Units\WeaponTarget.h"
#include "Units\AttributeSquad.h"
class Archive;

class UnitLegionary;
typedef  vector<UnitLegionary*> LegionaryUnitList;

typedef  vector<AttributeSquadReference> AttributeSquadReferences;

class SourceTeleport : public SourceDamage
{
public:
	SourceTeleport();
	SourceType type() const { return SOURCE_TELEPORT; }
	SourceBase* clone() const { return new SourceTeleport(*this); }

	void quant();
	void serialize(Archive& ar);

	void apply(UnitBase* target);
	bool setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target = 0);

private:
	
	LegionaryUnitList units_;
	WeaponTarget target_;
	
	bool notTeleportOwner_;	
	
	AttributeSquadReferences squadTypes_;
};


#endif //__SOURCE_TELEPOR_H__
