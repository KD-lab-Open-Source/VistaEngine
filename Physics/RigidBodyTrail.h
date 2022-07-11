#ifndef __RIGID_BODY_TRAIL_H__
#define __RIGID_BODY_TRAIL_H__

#include "RigidBodyUnit.h"

class TrailPoint;

////////////////////////////////////////////////////////////

class RigidBodyTrail : public RigidBodyUnit {
public:

	RigidBodyTrail();

	bool evolve(float dt);

	Se3f getDockingPose();
	Vect3f getEnterPoint();
	bool getEnterLine(Vect2f& p1, Vect2f& p2); // √лобальные коорд.

	bool isDocked() { return docked_; }

private:

	bool checkDockingPoint();
	bool interpolateDockingPose(const Se3f& poseIn); // true - когда встал в указанную позицию.

	TrailPoint* dockingPoint_;
	
	bool docked_;
	bool docking_;

};



#endif // __RIGID_BODY_TRAIL_H__

