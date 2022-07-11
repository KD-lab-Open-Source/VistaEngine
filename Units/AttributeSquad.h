#ifndef __ATTRIBUTE_SQUAD_H__
#define __ATTRIBUTE_SQUAD_H__

#include "UnitAttribute.h"

////////////////////////////////////////////////

class FormationPattern : public StringTableBase
{
	friend class CFormationEditor;
public:
	struct Cell : Vect2f
	{
		UnitFormationTypeReference type;

		Cell() : Vect2f(0, 0) {}
		void serialize(Archive& ar);
	};
	typedef vector<Cell> Cells;

	explicit FormationPattern(const char* name = "Свободная формация") : StringTableBase(name) {}
	const Cells& cells() const { return cells_; }
	void serialize(Archive& ar);

    float describedRadius() const
	{
		float radius = 0;
		for(int i=0;i<cells_.size();i++)
			radius = max(radius, cells_[i].norm()+cells_[i].type->radius());
		return radius;
	}

private:
	Cells cells_;
};

typedef StringTable<FormationPattern> FormationPatterns;
typedef StringTableReference<FormationPattern, false> FormationPatternReference;


////////////////////////////////////////////////

class AttributeSquad : public AttributeBase, public StringTableBase
{
public:
	struct Formation
	{
		FormationPatternReference formationPattern;
		bool rotateFront;
		bool hardFormation;
		bool attackByRadius;
		bool correctSpeed;
		float attackRadius;

		Formation();
		void serialize(Archive& ar);
	};
	typedef vector<Formation> Formations;
	Formations formations;

	UnitNumbers numbers;

	bool enableJoin;
	bool automaticJoin;
	float automaticJoinRadius;
	CircleEffect automaticJoinRadiusEffect;
	float joinRadius;
	CircleEffect joinRadiusEffect;

	Vect2f homePositionOffsetFactor;
	float formationRadiusBase;
	float followDistanceFactor; // followDistanceFactor*(radius1 + radius2)

	AttributeSquad(const char* name = "");
	void serialize(Archive& ar);
};

typedef StringTable<AttributeSquad> AttributeSquadTable;
typedef StringTableReference<AttributeSquad, false> AttributeSquadReference;

#endif //__ATTRIBUTE_SQUAD_H__
