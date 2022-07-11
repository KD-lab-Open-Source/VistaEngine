#ifndef __POSITION_GENERATOR_WAVE_H_INCLUDED__
#define __POSITION_GENERATOR_WAVE_H_INCLUDED__

template <class TUnitList>
class PositionGeneratorWave
{

	Vect2i center;
	vector<Vect2i> cells;
	list<Vect2i> front;

	Vect2f centerPoint;
	float radius;

	UnitBase* currentUnit;
	bool unitConditions;
	TUnitList* ignoreGroup;

	bool checkZones;
	bool checkUnits;
	Vect2f gridPoint;

	bool checkConditions(const Vect2i pose, bool checkZones, bool checkUnits) {
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

	bool unitInIgnore(UnitBase * unit) {
		TUnitList::iterator ui;
		FOR_EACH((*ignoreGroup), ui)
			if(unit == (*ui))return true;
        return false;
	}

	bool checkCell(const Vect2i& cell)
	{
		for(int i = 0; i<cells.size(); i++)
			if(cell == cells[i])
				return false;

		return checkConditions(Vect2i(cell*2*radius + centerPoint),checkZones,checkUnits);
	}

	void addFrontCell(const Vect2i& cell)
	{
		if(checkCell(cell)) {
			cells.push_back(cell);
			front.push_back(cell);
		}
	}

	Vect2i getNext() 
	{
		if(front.empty())
			return Vect2i(0,0);

		Vect2i cell = front.front();
		front.pop_front();
        addFrontCell(cell+Vect2i(0,1));
        addFrontCell(cell+Vect2i(0,-1));
        addFrontCell(cell+Vect2i(1,0));
        addFrontCell(cell+Vect2i(-1,0));
		return cell;
	}

public:

	void init(float _radius, const Vect2f& _centerPoint, TUnitList * _ignoreGroup)
	{
		cells.clear();
		front.clear();
        
		front.push_back(Vect2i(0,0));
        cells.push_back(Vect2i(0,0));

		radius = _radius;
		centerPoint = _centerPoint;
		ignoreGroup = _ignoreGroup;
	}

	Vect2i get(UnitBase * unit, bool _checkZones = true, bool _checkUnits = true)
	{
		checkZones = _checkZones;
		checkUnits = _checkUnits;
		currentUnit = unit;
		return Vect2f(getNext()*2*radius) + centerPoint;
	}

	void operator () (UnitBase* p) {
		if(gridPoint.distance2(Vect2f(p->pose().trans().x, p->pose().trans().y)) < sqr(currentUnit->radius()+p->radius()) && p->checkInPathTracking(currentUnit) && !unitInIgnore(p)) 
			unitConditions = false;
	}

};

#endif
