#ifndef __UNIT_TRAIL_H__
#define __UNIT_TRAIL_H__

#include "IronLegion.h"
#include "..\physics\RigidBodyTrail.h"

////////////////////////////////////////////////////////////

class AttributeTrail : public AttributeLegionary {
public:

	AttributeTrail();
	void serialize(Archive& ar);

};

////////////////////////////////////////////////////////////

class TrailPoint {
public:
	
	enum DockingSide {
		SIDE_FRONT = 1,
		SIDE_BACK = 2,
		SIDE_LEFT = 4,
		SIDE_RIGHT = 8
	};

	TrailPoint(): dockingSide(SIDE_FRONT), pose(Se3f::ID), radius(0) {}

	string name;
	DockingSide dockingSide;

	// Расчитывется динамически
	Se3f pose;
	float radius;
	
	void serialize(Archive& ar);

};

typedef SwapVector<TrailPoint> TrailPointList;
class Anchor;

////////////////////////////////////////////////////////////

class UnitTrail : public UnitLegionary {
public:
	
	UnitTrail(const UnitTemplate& data);

	const AttributeTrail& attr() const { return safe_cast_ref<const AttributeTrail&>(__super::attr()); }

    void Quant();

	void serialize(Archive& ar);
	TrailPoint* checkTrailPoint(Vect2f& point);

	bool isDocked() { return rigidBody()->isDocked(); }

	RigidBodyTrail* rigidBody() { return safe_cast<RigidBodyTrail*>(rigidBody_); }

private:

	TrailPointList trailPointList_;

};

#endif //__UNIT_TRAIL_H__