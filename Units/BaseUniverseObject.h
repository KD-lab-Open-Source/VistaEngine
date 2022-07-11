#ifndef __BASE_UNIVERSE_OBJECT_H_INCLUDED__
#define __BASE_UNIVERSE_OBJECT_H_INCLUDED__

#include "UnitID.h"

enum UniverseObjectClass{
	UNIVERSE_OBJECT_UNIT,
	UNIVERSE_OBJECT_ENVIRONMENT,
	UNIVERSE_OBJECT_SOURCE,
	UNIVERSE_OBJECT_CAMERA_SPLINE,
	UNIVERSE_OBJECT_ANCHOR,
	UNIVERSE_OBJECT_UNKNOWN,
};

class BaseUniverseObject {
public:
	BaseUniverseObject();
	virtual ~BaseUniverseObject();

	const Vect3f& position() const { return pose_.trans(); }
	const Vect2f& position2D() const { return position(); } 
	const QuatF& orientation() const { return pose_.rot(); }
	
	const Se3f& pose() const { return pose_; }
	virtual void setPose(const Se3f& pose, bool init) { pose_ = pose; }

	virtual float angleZ() const; // slow!!!

	const UnitID& unitID() const { return unitID_; }
	bool dead() const { return unitID_.empty(); }
	virtual void enable();
	virtual void Kill();

 	float radius() const { return radius_; }
	virtual void setRadius(float radius) { radius_ = radius; }

	bool selected() const { return selected_; }
	virtual void setSelected(bool selected) { selected_ = selected; }

	virtual void serialize(Archive& ar);

	virtual UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_UNKNOWN; }

protected:
	float radius_;
	Se3f pose_;
	UnitID unitID_;
	bool selected_;
};

#endif // #ifndef __BASE_UNIVERSE_OBJECT_H_INCLUDED__
