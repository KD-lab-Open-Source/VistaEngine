#include "stdafx.h"
#include "PositionGeneratorFormation.h"
#include "AttributeSquad.h"

PositionGeneratorFormation::PositionGeneratorFormation()
:freePositions(0),unitCount(0),invertMatrix(true) 
{ 
	pose = MatX2f::ID; 
}

PositionGeneratorFormation::~PositionGeneratorFormation() 
{ 
	if(freePositions) delete[] freePositions; 
}


void PositionGeneratorFormation::setFormationPattern(FormationPatternReference _formation)
{
	formation = _formation;
	unitCount = _formation->cells().size();
	if(freePositions) delete[] freePositions;
	freePositions = new bool[unitCount];
	initPositions();
}

void PositionGeneratorFormation::initPositions() 
{
	xassert(freePositions && "Формация не заданна");
	for(int i=0; i< unitCount; i++)
		freePositions[i] = true;
}

Vect2f PositionGeneratorFormation::getUnitPositionLocal(const UnitFormationTypeReference& type)
{
	xassert(freePositions && "Формация не заданна");
	for(int i=0; i<formation->cells().size(); i++)
		if(formation->cells()[i].type == type && freePositions[i]) {
			freePositions[i] = false;
			Vect2f temp;
			if(!invertMatrix)
				temp = formation->cells()[i];
			else
				temp = Vect2f(formation->cells()[i].x,-formation->cells()[i].y);
			return Vect2f(-temp.y, temp.x);
		}

	// Возможно кол-во разрешенных в скваде юнитов больше чем заданно в формации
	xassert(!"Лишний юнит в скваде или позиции не инициализированны...");
	return Vect2f::ZERO;
}

void PositionGeneratorFormation::setPosition(const Vect2f& prevPose, const Vect2f& nextPose, bool rotateFront)
{
	if(rotateFront) {
		Vect2f dir = nextPose - prevPose;
		dir /= dir.norm() + FLT_EPS;
		Mat2f rot;
		Vect2f local = pose.rot.invXform(dir);
		invertMatrix = (!(local.x < 0) && invertMatrix) || ((local.x < 0) && !invertMatrix); // логический XOR
		rot = Mat2f(dir.x, dir.y, dir.y, -dir.x);
		pose.set(rot, nextPose);
	} else {
		pose.set(pose.rot, nextPose);
	}
}

Vect2f PositionGeneratorFormation::getUnitPositionGloabal(const UnitFormationTypeReference& type)
{
	Vect2f localPose = getUnitPositionLocal(type);
	localPose *= pose;
	return localPose;
}

