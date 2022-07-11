#ifndef __UNIT_LINK_H__
#define __UNIT_LINK_H__

#include "UnitID.h"

template<class Unit>
class UnitLink 
{
public:
	UnitLink(Unit* unit = 0) : unitID_(const_cast<BaseUniverseObject*>(reinterpret_cast<const BaseUniverseObject*>(unit))) {}
	void operator=(Unit* unit) { unitID_ = const_cast<BaseUniverseObject*>(reinterpret_cast<const BaseUniverseObject*>(unit)); }
	operator Unit* () const { return get(); }
	Unit* operator->() const { return get(); }
	Unit& operator*() const { return *get(); }
	
	bool empty() const { return unitID_.empty(); }
	void serialize(Archive& ar) { unitID_.serialize(ar); }

	int index() const { return unitID_.index(); } // for debug only!

private:
	UnitID unitID_;

#ifdef _FINAL_VERSION_
	Unit* get() const { return (Unit*)(unitID_.get()); }
#else
	Unit* get() const;
#endif
};

// Необходимо явно инстанцировать функцию get
#ifdef _FINAL_VERSION_
#define UNIT_LINK_GET(Unit)	
#else
#define UNIT_LINK_GET(Unit)	Unit* UnitLink<Unit>::get() const { return safe_cast<Unit*>(unitID_.get()); }
#endif


#endif //__UNIT_LINK_H__
