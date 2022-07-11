#ifndef __CD_SPHERE_H_INCLUDED__
#define __CD_SPHERE_H_INCLUDED__

#include "CDConvex.h"

class RigidBodyBox;

namespace CD
{
///////////////////////////////////////////////////////////////
//
//    class Sphere
//
///////////////////////////////////////////////////////////////

	class Sphere
	{
	public:
		Sphere(float radius) : radius_(radius) {}
		void setRadius(float radius) { radius_ = radius; }
		float getRadius() const { return radius_; }
		void getTOI(float mass, Vect3f& toi);
		void collision(RigidBodyBox* body);
		bool waterContact(RigidBodyBox* body);
		bool computeBoxPenetration(Vect3f& pcontact, Vect3f& qcontact, const Vect3f& spherePosition,
			const Box& box, const Vect3f& boxPosition, const QuatF& boxOrientation) const;

	private:
		float radius_;
	};
}

#endif // __CD_SPHERE_H_INCLUDED__
