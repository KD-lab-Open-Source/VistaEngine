#ifndef __GROUND_COLLISION_H_INCLUDED__
#define __GROUND_COLLISION_H_INCLUDED__

#include "CD//CDSphere.h"

class RigidBodyCollision;
class UnitBase;

///////////////////////////////////////////////////////////////
//
//    struct ContactInfo
//
///////////////////////////////////////////////////////////////

struct ContactInfo
{
	Vect3f cp1, cp2;
	int bodyPart1, bodyPart2;
	UnitBase *unit1, *unit2;

	ContactInfo();

	const Vect3f& collisionPoint(const UnitBase* unit) const { return unit == unit1 ? cp1 : cp2; }
	int bodyPart(const UnitBase* unit) const { return unit == unit1 ? bodyPart1 : bodyPart2; }
};

///////////////////////////////////////////////////////////////
//
//    class Geom
//
///////////////////////////////////////////////////////////////

class Geom {
public:
	enum GeomType
	{
		GEOM_BASE,
		GEOM_SPHERE,
		GEOM_BOX,
		GEOM_MESH
	};

	Geom() : geomType_(GEOM_BASE) {}
	GeomType geomType() const { return geomType_; }
	virtual bool groundCollision(RigidBodyCollision* body) const = 0;
	virtual bool waterCollision(RigidBodyCollision* body) const = 0;
	virtual bool bodyCollision(RigidBodyCollision* body1, RigidBodyCollision* body2, ContactInfo& contactInfo) const  = 0;
	virtual void getTOI(float mass, Vect3f& TOI) const = 0;
	virtual float getVolume() const = 0;
	virtual bool waterContact(RigidBodyCollision* body) const  = 0;
	
protected:
	GeomType geomType_;
};

///////////////////////////////////////////////////////////////
//
//    class GeomSphere
//
///////////////////////////////////////////////////////////////

class GeomSphere : public Geom
{
public:
	GeomSphere(float radius);
	void setRadius(float radius) { sphere_.setRadius(radius); }
	float getRadius() const { return sphere_.getRadius(); }
	void getTOI(float mass, Vect3f& toi)  const;
	float getVolume() const;
	bool groundCollision(RigidBodyCollision* body) const;
	bool waterCollision(RigidBodyCollision* body) const;
	bool bodyCollision(RigidBodyCollision* body1, RigidBodyCollision* body2, ContactInfo& contactInfo) const;
	bool waterContact(RigidBodyCollision* body)  const;
	const CD::Sphere& sphere() const { return sphere_; }
	
private:
	float getWaterHeight(int x, int y, RigidBodyCollision* body) const;
	bool planeCollision(int x, int y, RigidBodyCollision* body)  const;
	bool planeCollisionWater(int x, int y, RigidBodyCollision* body) const;
	bool triangleCollision(const Vect3f& p1, const Vect3f& p2, const Vect3f& p3, RigidBodyCollision* body)  const;
	bool pointInTriangle(const Vect2f& p1, const Vect2f& p2, const Vect2f& p3, const Vect2f& p) const;
	Vect3f closestPointOnLine(const Vect3f& a, const Vect3f& b, const Vect3f& p) const;
	Vect3f closestPointToTriange(const Vect3f& a, const Vect3f& b, const Vect3f& c, const Vect3f& p) const;

	CD::Sphere sphere_;
};

///////////////////////////////////////////////////////////////
//
//    class GeomBox
//
///////////////////////////////////////////////////////////////

class GeomBox : public Geom
{
public:
	GeomBox(const Vect3f& extent);
	void setExtent(const Vect3f& extent) { box_.setExtent(extent); }
	const Vect3f& getExtent() const { return box_.getExtent(); }
	void computeVertex(Vect3f vertex[8], const Vect3f& position, const QuatF& orientation) const;
	bool groundCollision(RigidBodyCollision* body) const;
	bool waterCollision(RigidBodyCollision* body) const;
	bool bodyCollision(RigidBodyCollision* body1, RigidBodyCollision* body2, ContactInfo& contactInfo) const;
	void getTOI(float mass, Vect3f& TOI) const;
	float getVolume() const;
	bool waterContact(RigidBodyCollision* body) const;
	float computeGroundPenetration(const Vect3f& centreOfGravity, const QuatF& orientation) const;
	const CD::Box& box() const { return box_; }

private:
	void computeNormals(Vect3f vertex[3], const QuatF& orientation) const;
	void computeVertex(Vect3f vertex[8], const Vect3f& position, const Vect3f normals[3]) const;
	bool collisionVertex(RigidBodyCollision* body, Vect3f vertex[8]) const;
	bool collisionVertexWater(RigidBodyCollision* body, Vect3f vertex[8]) const;
	bool collisionPlane(RigidBodyCollision* body, Vect3f& p1, Vect3f& p2, Vect3f& p3, Vect3f& p4, Vect3f& normal) const;
	bool collisionPlaneWater(RigidBodyCollision* body, Vect3f& p1, Vect3f& p2, Vect3f& p3, Vect3f& p4, Vect3f& normal) const;
	bool pointInPlane(Vect3f& p1, Vect3f& p2, Vect3f& p3, Vect3f& p4, Vect3f& normal, Vect3f& point) const;

	CD::Box box_;
};

///////////////////////////////////////////////////////////////
//
//    class GeomMesh
//
///////////////////////////////////////////////////////////////

class GeomMesh: public GeomBox
{
public:
	GeomMesh(vector<Vect3f>& vertex);
	bool groundCollision(RigidBodyCollision* body) const;
	bool waterCollision(RigidBodyCollision* body) const;
	Vect3f computeCentreOfGravity();
	void getTOI(const Vect3f& centreOfGravity, float mass, Vect3f& TOI) const;
	bool waterContact(RigidBodyCollision* body) const;
	void simplifyMesh(int vertexNum);
	void showDebugInfo(const Se3f& pose) const;

private:
	vector<Vect3f> points;
};

#endif
