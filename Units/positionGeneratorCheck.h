#ifndef __POSITION_GENERATOR_CHECK_H_INCLUDED__
#define __POSITION_GENERATOR_CHECK_H_INCLUDED__

#include "PositionGenerator.h"
#include "PositionGeneratorSquad.h"
#include "BaseUnit.h"
#include "..\Game\Universe.h"
//#include "SelectManager.h"

// Генерирует позиции учитывая зоны непрходимости и пересечения с другими юнитами.
class PositionGeneratorCheck:public PositionGeneratorSquad {

	Vect2f centerPosition;
	UnitBase* currentUnit;
	bool unitConditions;
	UnitList* ignoreGroup;
	float radius;

	bool checkConditions(const Vect2i pose, bool checkZones, bool checkUnits) {
		bool zones = true;
		unitConditions = true;

		if(checkZones && currentUnit->rigidBody())
			zones = currentUnit->rigidBody()->checkImpassabilityStatic(Vect3f(pose.x,pose.y,0));

		if(checkUnits)
			universe()->unitGrid.Scan(pose,currentUnit->radius(),*this);

		return zones && unitConditions;
	}

	bool unitInIgnore(UnitBase * unit) {
		UnitList::iterator ui;
		FOR_EACH((*ignoreGroup), ui)
			if(unit == (*ui))
				return true;
        return false;
	}

public:
	
	PositionGeneratorCheck(const Vect2f& _centerPosition, float _radius, UnitList * _ignoreGroup)
	{
		init(_radius);
		ignoreGroup = _ignoreGroup;
		centerPosition = _centerPosition;
		radius = _radius;
	}

	Vect2f get(UnitBase * unit, bool checkZones = true, bool checkUnits = true) {
		currentUnit = unit;
		int a =0;
		Vect2f point;
		do {
			point = PositionGeneratorSquad::get() + centerPosition;
		} while (!checkConditions(point,checkZones,checkUnits) && ((a++) < 10));

		return (a==31)?centerPosition:point;
	}

	void operator () (UnitBase* p) {
		if(p->checkInPathTracking(currentUnit) && !unitInIgnore(p)) 
			unitConditions = false;
	}

};


#endif
