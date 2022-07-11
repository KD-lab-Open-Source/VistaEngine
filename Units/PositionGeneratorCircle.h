#ifndef __POSITION_GENERATOR_CIRCLE_H_INCLUDED__
#define __POSITION_GENERATOR_CIRCLE_H_INCLUDED__

#include "ObjectSpreader.h"

//class UnitBase;

#include "BaseUnit.h"
#include "..\Game\Universe.h"


template <class TUnitList>
class PositionGeneratorCircle: public ObjectSpreader
{

	Vect2f centerPoint;
	float radius;

	const UnitBase* currentUnit;
	bool unitConditions;
	TUnitList* ignoreGroup;

	bool checkZones;
	bool checkUnits;
	Vect2f gridPoint;

	bool checkConditions(const Vect2i pose, bool checkZones, bool checkUnits);

	bool unitInIgnore(UnitBase * unit) const;

public:

	void init(float _radius, const Vect2f& _centerPoint, TUnitList * _ignoreGroup);

	Vect2i get(const UnitBase * unit, bool _checkZones = true, bool _checkUnits = true);

	void operator () (UnitBase* p);

};

template <class TUnitList>
bool PositionGeneratorCircle<TUnitList>::checkConditions(const Vect2i pose, bool checkZones, bool checkUnits)
{
	bool zones = true;
	unitConditions = true;

	if(checkZones && currentUnit->rigidBody())
		zones = currentUnit->rigidBody()->checkImpassabilityStatic(Vect3f(pose.x,pose.y,0));

	if(checkUnits) {
		gridPoint = pose;
		universe()->unitGrid.Scan(pose,radius,*this);
	}

	return zones && unitConditions;
}

template <class TUnitList>
bool PositionGeneratorCircle<TUnitList>::unitInIgnore(UnitBase * unit) const
{
	if(!ignoreGroup)
		return false;

	TUnitList::const_iterator ui;
	FOR_EACH((*ignoreGroup), ui)
		if(unit == (*ui))return true;
	return false;
}

template <class TUnitList>
void PositionGeneratorCircle<TUnitList>::init(float _radius, const Vect2f& _centerPoint, TUnitList * _ignoreGroup)
{
	radius = _radius;
	centerPoint = _centerPoint;
	ignoreGroup = _ignoreGroup;
}

template <class TUnitList>
Vect2i PositionGeneratorCircle<TUnitList>::get(const UnitBase * unit, bool _checkZones = true, bool _checkUnits = true)
{
	checkZones = _checkZones;
	checkUnits = _checkUnits;
	currentUnit = unit;
	Vect2f point;
	int a =0;
	do {
		point = centerPoint + addCircle(2.2*unit->radius() + radius).position;
	} while(!checkConditions(point, checkZones, checkUnits) && ((a++) < 10));
	return point;
}

template <class TUnitList>
void PositionGeneratorCircle<TUnitList>::operator () (UnitBase* p)
{
	if(!unitInIgnore(p) && gridPoint.distance2(Vect2f(p->pose().trans().x, p->pose().trans().y)) < sqr(currentUnit->radius()+p->radius()) && p->checkInPathTracking(currentUnit)) 
		unitConditions = false;
}

#endif
