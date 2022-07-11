#ifndef __RIGID_BODY_BASE_H__
#define __RIGID_BODY_BASE_H__

//#include "Factory.h"
#include "Geom.h"

////////////////////////////////////////
enum RigidBodyType
{
	RIGID_BODY_BASE,
	RIGID_BODY_ENVIRONMENT,
	RIGID_BODY_UNIT,
	RIGID_BODY_MISSILE,
	RIGID_BODY_BOX,
	RIGID_BODY_TRAIL,
	RIGID_BODY_RAG_DOLL,
	RIGID_BODY_UNIT_RAG_DOLL,
	RIGID_BODY_NODE
};

///////////////////////////////////////////////////////////////
// RigidBodyBase
///////////////////////////////////////////////////////////////

class RigidBodyBase {
public:
	
	enum CollisionType {
		GROUND_COLLIDING = 1,
		WATER_COLLIDING = 2,
		WORLD_BORDER_COLLIDING = 4
	};

	RigidBodyBase();
	~RigidBodyBase();

	const RigidBodyPrm& prm() const { return *prm_; }

	const Se3f& pose() const { return pose_; }
	virtual void setPose(const Se3f& pose) { pose_ = pose; }
	virtual void initPose(const Se3f& pose) { pose_ = pose; }

	const Vect3f& position() const { return pose_.trans(); }
	const Vect2f& position2D() const { return *(Vect2f*)&pose_.trans(); }

	const QuatF& orientation() const { return pose_.rot(); }
	const Mat3f rotation() const { return Mat3f(pose_.rot()); }
	float angleZ() const { return atan2f(rotation()[1][0], rotation()[0][0]); }

	Vect3f boxMin() const { return centreOfGravityLocal() - extent(); }
	Vect3f boxMax() const { return centreOfGravityLocal() + extent(); }
	Vect3f centreOfGravity() const;
	const Vect3f& centreOfGravityLocal() const { return centreOfGravityLocal_; }
	void setCentreOfGravityLocal(const Vect3f& centreOfGravity) { centreOfGravityLocal_ = centreOfGravity; }
	GeomBox& box() { return box_; }
	const Vect3f& extent() const { return box_.getExtent(); }
	GeomSphere& sphere() { return sphere_; }
	float boundRadius() const {return sphere_.getRadius(); }
	float maxRadius() const {return max(radius(), 2.0f * boundRadius()); }
	const Geom* geom() const 
	{ 
		switch(geomType_)
		{
		case Geom::GEOM_SPHERE:
			return &sphere_; 
		case Geom::GEOM_BOX:
			return &box_;
		}
		return geom_;
	}
	virtual bool bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo);
	virtual int computeBoxSectionPenetration(Vect3f& contactPoint, const Vect3f& sectionBegin, const Vect3f& sectionEnd) const;
	
	float radius() const {return radius_; }
	virtual void setRadius(float radius) { radius_ = radius; } 

	virtual void build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass);
	virtual bool evolve(float dt);

	bool asleep() const { return !awakeCounter_; }
	void awake() { awakeCounter_ = prm().awakeCounter; }
	void sleep() { awakeCounter_ = 0; }

	bool unmovable() const { return unmovable_; }
	void makeStatic() { if(!unmovable_) debugMessage("makeStatic"); unmovable_ = 1; }
	void makeDynamic() { if(unmovable_) debugMessage("makeDynamic"); unmovable_ = 0; }
	
	int colliding() const { return colliding_; }

	virtual int bodyPart() const { return -1; }

	virtual const Vect3f& velocity() { return Vect3f::ZERO; }
	virtual void setVelocity(const Vect3f& velocity) {}

	//=======================================================
	bool isBase() const { return rigidBodyType == RIGID_BODY_BASE; }
	bool isEnvironment() const { return rigidBodyType == RIGID_BODY_ENVIRONMENT; }
	bool isUnit() const { return rigidBodyType == RIGID_BODY_UNIT || rigidBodyType == RIGID_BODY_TRAIL || rigidBodyType == RIGID_BODY_UNIT_RAG_DOLL; }
	bool isMissile() const { return rigidBodyType == RIGID_BODY_MISSILE; }
	bool isBox() const { return rigidBodyType == RIGID_BODY_BOX; }
	bool isTrail() const { return rigidBodyType == RIGID_BODY_TRAIL; }
	bool isUnitRagDoll() const { return rigidBodyType == RIGID_BODY_UNIT_RAG_DOLL; }
	bool isNode() const { return rigidBodyType == RIGID_BODY_NODE; }

	//=======================================================
	//	Проверка поверхностей.
	//=======================================================
	bool checkWater(int xc, int yc, int r) const;
	bool checkLowWater(const Vect3f& pos, float r) const;
	virtual bool checkImpassability(const Vect3f& pos, float radius) const { return true; }
	virtual bool checkImpassabilityStatic(const Vect3f& pos) const { return true; }

	//=======================================================
	//	2D функции обработки пересечений. Используются в PT.
	//=======================================================
	bool bodyIntersect(const RigidBodyBase* body, MatX2f bodyPose);
	bool pointInBody(const Vect2f& point);

	// Debug info
	virtual void show();
	void setColor(sColor4c color) { debugColor_ = color; }

	static RigidBodyBase* buildRigidBody(const RigidBodyType& rigidBodyType, const RigidBodyPrm* prm, const Vect3f& boxMin, const Vect3f& boxMax, float mass);

	bool onWater;
	bool onWaterPrev;
	bool onLowWater;
	bool onIce;
	bool onIcePrev;

protected:

	float analyseArea(bool analyseWater, Vect3f& normalNonNormalized);

#ifndef _FINAL_VERSION_
	string debugMessageList_;
	void debugMessage(const char* text);
#else
	void debugMessage(const char* text) {}
#endif

	const RigidBodyPrm* prm_;

	RigidBodyType rigidBodyType;

	Se3f pose_;
	
	Vect3f centreOfGravityLocal_;
	GeomBox box_;
	GeomSphere sphere_;
	GeomMesh* geom_;
	Geom::GeomType geomType_;

	float radius_;

	int awakeCounter_;
	Vect3f averagePosition_;
	Vect3f averageOrientation_;

	bool unmovable_;
	int colliding_;

	// Debug info
	sColor4c debugColor_;

};

///////////////////////////////////////////////////////////////
// RigidBodyEnvironment
///////////////////////////////////////////////////////////////

class RigidBodyEnvironment : public RigidBodyBase {
public:

	RigidBodyEnvironment();

	void build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass);
	void initPose(const Se3f& pose);
	bool evolve(float dt);
	void setRadius(float radius);
	/// Принудительное включение точечного анализа поверхности.
	void setPointAreaAnalize() { pointAreaAnalize_ = true; }
	void setHoldOrientation(bool verticalOrientation) { holdOrientation_ = true; verticalOrientation_ = verticalOrientation;}

private:

	void placeToGround();

	float angle_;

	bool pointAreaAnalize_; // true - принудительный точечный анализ поверхности.

	bool holdOrientation_;

	bool verticalOrientation_;
};

///////////////////////////////////////////////////////////////
// Rigid Body Factory
///////////////////////////////////////////////////////////////

typedef Factory<RigidBodyType, RigidBodyBase> RigidBodyFactory;

#endif // __RIGID_BODY_BASE_H__
