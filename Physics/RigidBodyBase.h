#ifndef __RIGID_BODY_BASE_H__
#define __RIGID_BODY_BASE_H__

#include "RigidBodyPrm.h"
#include "Geom.h"
#include "Serialization\Factory.h"

///////////////////////////////////////////////////////////////

enum RigidBodyType
{
	RIGID_BODY_BASE,
	RIGID_BODY_ENVIRONMENT,
	RIGID_BODY_UNIT,
	RIGID_BODY_MISSILE,
	RIGID_BODY_BOX,
	RIGID_BODY_UNIT_RAG_DOLL
};

///////////////////////////////////////////////////////////////
//
//    class RigidBodyCollision
//
///////////////////////////////////////////////////////////////

class RigidBodyCollision 
{
public:
	RigidBodyCollision();
	virtual ~RigidBodyCollision();
	const Se3f& pose() const { return pose_; }
	const Vect3f& position() const { return pose_.trans(); }
	const QuatF& orientation() const { return pose_.rot(); }
	const Vect3f& centreOfGravity() const { return centreOfGravity_; }
	const Vect3f& centreOfGravityLocal() const { return centreOfGravityLocal_; }
	float boundRadius() const { return sphere_.getRadius(); }
	const Geom* collisionObject() const 
	{ 
		if(sphereCollision_)
			return &sphere_; 
		return geom_;
	}
	bool sphereCollision() { return sphereCollision_; }
	virtual void setPose(const Se3f& pose) 
	{ 
		pose_ = pose; 
		computeCentreOfGravity();
	}
	void setCentreOfGravityPose(const Se3f& pose)
	{
		centreOfGravity_ = pose.trans();
		pose_.rot() = pose.rot();
		Vect3f centerLocal = centreOfGravityLocal();
		orientation().xform(centerLocal);
		pose_.trans().sub(centreOfGravity(), centerLocal);
	}
	void setCentreOfGravityLocal(const Vect3f& centreOfGravityLocal) { centreOfGravityLocal_ = centreOfGravityLocal; }
	virtual void addContact(const Vect3f& point, const Vect3f& normal, float penetration) {}
	virtual float waterLevel() const { return 0.0f; }
	virtual float lavaLevel() const { return 0.0f; }
	void setOrientation(const QuatF& orientation) 
	{ 
		pose_.rot() = orientation;
		computeCentreOfGravity();
	}
	void addPathTrackingVelocity(const Vect3f& velocity) 
	{ 
		pose_.trans() += velocity; 
		centreOfGravity_ += velocity;
	}
	
protected:
	bool checkLowWater(const Vect3f& pos, float r) const;
	
	void setPosition(const Vect3f& position) 
	{ 
		pose_.trans() = position; 
		computeCentreOfGravity();
	}
	void setPoseZ(float poseZ) 
	{ 
		pose_.trans().z = poseZ;
		computeCentreOfGravity();
	}
	void setBoundRadius(float boundRadius) { sphere_.setRadius(boundRadius); }
	void setSphereCollision(bool sphereCollision) { sphereCollision_ = sphereCollision; }
	const Geom* geom() const { return geom_; }
	void setGeom(Geom* geom) 
	{
		delete geom_;
		geom_ = geom;
	}

private:
	void computeCentreOfGravity() { pose_.xformPoint(centreOfGravityLocal_, centreOfGravity_); }

	Se3f pose_;
	Vect3f centreOfGravity_;
	Vect3f centreOfGravityLocal_;
    GeomSphere sphere_;
	bool sphereCollision_;
	Geom* geom_;
};

///////////////////////////////////////////////////////////////
//
//    class RigidBodyBase
//
///////////////////////////////////////////////////////////////

class RigidBodyBase : public RigidBodyCollision
{
public:
	enum CollisionType {
		GROUND_COLLIDING = 1,
		WATER_COLLIDING = 2,
		WORLD_BORDER_COLLIDING = 4
	};

	///////////////////////////////////////////////////////////
	//
	//    class WaterScan
	//
	///////////////////////////////////////////////////////////

	class WaterScan 
	{
	public:
		WaterScan(int x_, int y_, int tileShift_);
		int goNextX();
		void goNextY();
	
	private:
		void nextTile(int tile00, int tile01);
	
		int tile11;
		int tile10;
		int tileDeltaX;
		int tileDeltaY;
		int startX;
		int tileDeltaXStart;
		int x;
		int y;
		int tileHLeft;
		int tileScale;
		int tileShift;
		int tileDeltaHX;
	};

	///////////////////////////////////////////////////////////////

	RigidBodyBase();
	virtual void build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass);
	static RigidBodyBase* buildRigidBody(const RigidBodyType& rigidBodyType, const RigidBodyPrm* prm, const Vect3f& center, const Vect3f& extent, float mass);
	const RigidBodyPrm& prm() const { return *prm_; }
	virtual void initPose(const Se3f& pose) { RigidBodyCollision::setPose(pose); }
	const Vect2f& position2D() const { return position(); }
	float computeAngleZ() const 
	{ 
		return atan2f(orientation().x() * orientation().y() + orientation().z() * orientation().s(), 
			sqr(orientation().s()) + sqr(orientation().x()) - 0.5f); 
	}
	float cosTheta() const { return 2.f * (sqr(orientation().s()) + sqr(orientation().z_) - 0.5f); }
	virtual bool bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo);
	virtual int computeBoxSectionPenetration(Vect3f& contactPoint, const Vect3f& sectionBegin, const Vect3f& sectionEnd) const;
	const Vect3f& extent() const { return box()->getExtent(); }
	float radius() const {return radius_; }
	float maxRadius() const {return max(radius(), 2.0f * boundRadius()); }
	virtual void setRadius(float radiusNew) { radius_ = radiusNew; } 
	virtual bool evolve(float dt);
	bool asleep() const { return !awakeCounter_; }
	void awake() 
	{ 
		awakeCounter_ = prm().awakeCounter; 
		attach();
	}
	void sleep() 
	{ 
		awakeCounter_ = 0; 
		detach();
	}
	virtual void attach() {}
	virtual void detach() {}
	int colliding() const { return colliding_; }
	virtual int bodyPart() const { return -1; }
	virtual const Vect3f& velocity() { return Vect3f::ZERO; }
	virtual void setVelocity(const Vect3f& velocity) {}
	virtual void setForwardVelocity(float v) {}
	///////////////////////////////////////////////////////////
	bool isBase() const { return rigidBodyType_ == RIGID_BODY_BASE; }
	bool isEnvironment() const { return rigidBodyType_ == RIGID_BODY_ENVIRONMENT; }
	bool isUnit() const { return rigidBodyType_ == RIGID_BODY_UNIT || rigidBodyType_ == RIGID_BODY_UNIT_RAG_DOLL; }
	bool isMissile() const { return rigidBodyType_ == RIGID_BODY_MISSILE; }
	bool isBox() const { return rigidBodyType_ == RIGID_BODY_BOX; }
	bool isUnitRagDoll() const { return rigidBodyType_ == RIGID_BODY_UNIT_RAG_DOLL; }
	///////////////////////////////////////////////////////////
	//	ѕроверка поверхностей.
	///////////////////////////////////////////////////////////
	virtual bool checkImpassability(const Vect2f& pos) const { return true; }
	///////////////////////////////////////////////////////////
	//	2D функции обработки пересечений. »спользуютс€ в PT.
	///////////////////////////////////////////////////////////
	void setBoundCheck(bool boundCheck) { ptBoundCheck_ = boundCheck; }
	bool ptBoundCheck() const { return ptBoundCheck_; }
	bool bodyIntersect(const RigidBodyBase* body, const Vect2f& bodyPosition, const Mat2f& bodyOrientation);
	// States
	bool groundColliding() const { return (colliding_ & GROUND_COLLIDING) != 0; } // сталкиваетс€, стоит на земле
	bool waterColliding() const { return (colliding_ & WATER_COLLIDING) != 0; } // сталкиваетс€, стоит на воде
	// Debug info
	virtual void show();

	bool onWater() const { return onWater_; }
	bool onLowWater() const { return onLowWater_; }
	bool checkWater(int xc, int yc, int r) const;

protected:
	void setRigidBodyType(RigidBodyType rigidBodyType) { rigidBodyType_ = rigidBodyType; }
	const GeomBox* box() const { return safe_cast<const GeomBox*>(geom()); }
	
	float analyzeWaterFast(const Vect2i& center, int r, Vect3f& normalNonNormalized);
	float analyzeArea(bool analyseWater, Vect3f& normalNonNormalized);

	int colliding_;
	int awakeCounter_;

	bool onWater_;
	bool onLowWater_;
	
private:
	RigidBodyType rigidBodyType_;
	const RigidBodyPrm* prm_;
	float radius_;
	Vect3f averagePosition_;
	Vect3f averageOrientation_;
	bool ptBoundCheck_;

	// Debug info
public:
	void setColor(Color4c color) { debugColor_ = color; }
#ifndef _FINAL_VERSION_
	string debugMessageList_;
	void debugMessage(const char* text);
#else
	void debugMessage(const char* text) {}
#endif

private:
	Color4c debugColor_;
};

///////////////////////////////////////////////////////////////
//
//    class RigidBodyEnvironment
//
///////////////////////////////////////////////////////////////

class RigidBodyEnvironment : public RigidBodyBase {
public:
	RigidBodyEnvironment();
	void build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass);
	void initPose(const Se3f& pose);
	bool evolve(float dt);
	void setRadius(float radiusNew);
	void setPointAreaAnalize() { pointAreaAnalize_ = true; } // ѕринудительное включение точечного анализа поверхности.
	void setHoldOrientation(bool verticalOrientation) { holdOrientation_ = true; verticalOrientation_ = verticalOrientation;}
	float angleZ() { return angle_; }
	void setAngleZ(float angle) { angle_ = angle; }

private:
	void placeToGround();

	float angle_;
	bool pointAreaAnalize_; // true - принудительный точечный анализ поверхности.
	bool holdOrientation_;
	bool verticalOrientation_;
};

///////////////////////////////////////////////////////////////

typedef Factory<RigidBodyType, RigidBodyBase> RigidBodyFactory;

#endif // __RIGID_BODY_BASE_H__
