#ifndef __ATTRIBUTE_SQUAD_H__
#define __ATTRIBUTE_SQUAD_H__

#include "UnitAttribute.h"
#include "XTL\UniqueVector.h"

////////////////////////////////////////////////

class FormationPattern : public StringTableBase
{
	friend class FormationEditorViewPort;
public:
	struct Cell : Vect2f
	{
		UnitFormationTypeReference type;

		Cell() : Vect2f(Vect2f::ZERO) {}
		void serialize(Archive& ar);
	};
	typedef vector<Cell> Cells;

	explicit FormationPattern(const char* name = "Свободная формация") : StringTableBase(name) {}
	const Cells& cells() const { return cells_; }
	void serialize(Archive& ar);

private:
	Cells cells_;
};

typedef StringTable<FormationPattern> FormationPatterns;
typedef StringTableReference<FormationPattern, false> FormationPatternReference;

typedef vector<UnitFormationTypeReference> UnitFormationTypeReferences;

////////////////////////////////////////////////

enum VelocityCorrection 
{
	VELOCITY_DEFAULT = 0,
	VELOCITY_MIN = 1,
	VELOCITY_AVERAGE = 2,
	VELOCITY_MAX = 3
};

////////////////////////////////////////////////

class AttributeSquad : public AttributeBase, public StringTableBase
{
public:
	struct Formation
	{
		FormationPatternReference formationPattern;
		bool rotateFront;
		bool uniformFormation;
		bool attackByRadius;
		VelocityCorrection velocityCorrection;
		float attackRadius;

		Formation();
		void serialize(Archive& ar);
	};
	typedef vector<Formation> Formations;
	Formations formations;

	int showAllWayPointDist;
	bool showAllWayPoints;
	
	bool enableJoin;
	bool automaticJoin;
	float automaticJoinRadius;
	CircleManagerParam automaticJoinRadiusEffect;
	float joinRadius;
	CircleManagerParam joinRadiusEffect;

	bool showWayPoint;
	bool showTriggerWayPoint;
	UI_MarkObjectAttribute targetPoint;

	Vect2f homePositionOffsetFactor;
	float formationRadiusBase;
	float followDistanceFactor; // followDistanceFactor*(radius1 + radius2)

	bool disableMainUnitAutoAttack;
	bool forceUnitsAutoAttack;
	bool followMainUnitInAutoMode;

	EffectAttributeAttachable mainUnitEffect;
	EffectAttributeAttachable waitingUnitEffect;
	bool showSightSector;

	UnitFormationTypeReferences allowedUnits;
	typedef UniqueVector<AttributeReference> AllowedUnitsAttributes;
	AllowedUnitsAttributes allowedUnitsAttributes;

	AttributeSquad(const char* name = "");
	const char* libraryKey() const { return c_str(); }
	void serialize(Archive& ar);
};

typedef StringTable<AttributeSquad> AttributeSquadTable;
typedef StringTableReference<AttributeSquad, false> AttributeSquadReference;

#endif //__ATTRIBUTE_SQUAD_H__
