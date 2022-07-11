#ifndef __POSITION_GENERATOR_FORMATION_H_INCLUDED__
#define __POSITION_GENERATOR_FORMATION_H_INCLUDED__

#include "StringTableReference.h"

class FormationPattern;
typedef StringTableReference<FormationPattern, false> FormationPatternReference;

class UnitFormationType;
typedef StringTableReference<UnitFormationType, true> UnitFormationTypeReference;

class PositionGeneratorFormation {
	
	bool* freePositions;
	FormationPatternReference formation;
	int unitCount;
	bool invertMatrix;
	MatX2f pose;

public:
	PositionGeneratorFormation();
	~PositionGeneratorFormation();

	void setFormationPattern(FormationPatternReference _formation);
	void initPositions();
	Vect2f getUnitPositionLocal(const UnitFormationTypeReference& type);

	void setPosition(MatX2f& _pose) { pose = _pose; }
	
	void setPosition(const Vect2f& prevPose, const Vect2f& nextPose, bool rotateFront);
	Vect2f getUnitPositionGloabal(const UnitFormationTypeReference& type);
};

#endif
