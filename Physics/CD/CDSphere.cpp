#include "StdAfx.h"
#include "CDSphere.h"

using namespace CD;

bool Sphere::computeBoxPenetration(Vect3f& pcontact, Vect3f& qcontact, const Vect3f& spherePosition,
						   const Box& box, const Vect3f& boxPosition, const QuatF& boxOrientation) const
{
	start_timer_auto();
	Vect3f relativePosition(spherePosition);
	relativePosition.sub(boxPosition);
	boxOrientation.invXform(relativePosition);
	Vect3f dif(fabs(relativePosition.x), fabs(relativePosition.y), fabs(relativePosition.z));
	dif.sub(box.getExtent());
	bool region0(dif.x > 0), region1(dif.y > 0), region2(dif.z > 0);
	if(!region0 && !region1 && !region2){ 
		//111 center inside box
		if(dif.x > dif.y)
			if(dif.x > dif.z)
				region0 = true;
			else
				region2 = true;
		else if(dif.y > dif.z)
			region1 = true;
		else
			region2 = true;
	}
	float dist, sign0, sign1;

	if(region0){
		if(region1){
			if(region2){
				//nearest vertex
				Vertex vertex;
				box.computeSupportVertex(vertex, relativePosition);
				pcontact.sub(vertex, relativePosition);
				dist = pcontact.norm();
				if(dist > radius_)
					return false;
				xassert(dist > FLT_EPS);
				pcontact.scale(radius_ / dist);
				pcontact.add(relativePosition);
				qcontact = vertex;
			}
			else{
				//nearest edge XY
				dist = sqrtf(sqr(dif.x) + sqr(dif.y));
				if(dist > radius_)
					return false;
				pcontact = relativePosition;
				sign0 = SIGN(pcontact.x);
				sign1 = SIGN(pcontact.y);
				dist = radius_ / dist;
				pcontact.x -= dif.x * sign0 * dist;
				pcontact.y -= dif.y * sign1 * dist;
				qcontact.set(box.getExtent().x * sign0, box.getExtent().y * sign1, pcontact.z);
			}
		}
		else{
			if(region2){
				//nearest edge XZ
				dist = sqrtf(sqr(dif.x) + sqr(dif.z));
				if(dist > radius_)
					return false;
				pcontact = relativePosition;
				sign0 = SIGN(pcontact.x);
				sign1 = SIGN(pcontact.z);
				dist = radius_ / dist;
				pcontact.x -= dif.x * sign0 * dist;
				pcontact.z -= dif.z * sign1 * dist;
				qcontact.set(box.getExtent().x * sign0, pcontact.y, box.getExtent().z * sign1);
			}
			else{
				//nearest face X
				if(dif.x > radius_)
					return false;
				pcontact = relativePosition;
				sign0 = SIGN(pcontact.x);
				pcontact.x -= radius_*sign0;
				qcontact.set(box.getExtent().x*sign0, pcontact.y, pcontact.z);
			}
		}
	}
	else{
		if(region1){
			if(region2){
				//nearest edge YZ
				dist = sqrtf(sqr(dif.y) + sqr(dif.z));
				if(dist > radius_)
					return false;
				pcontact = relativePosition;
				sign0 = SIGN(pcontact.y);
				sign1 = SIGN(pcontact.z);
				dist = radius_ / dist;
				pcontact.y -= dif.y * sign0 * dist;
				pcontact.z -= dif.z * sign1 * dist;
				qcontact.set(pcontact.x, box.getExtent().y * sign0, box.getExtent().z * sign1);
			}
			else{
				//nearest face Y
				if(dif.y > radius_)
					return false;
				pcontact = relativePosition;
				sign0 = SIGN(pcontact.y);
				pcontact.y -= radius_*sign0;
				qcontact.set(pcontact.x, box.getExtent().y*sign0, pcontact.z);
			}
		}
		else{
			if(region2){
				//nearest face Z
				if(dif.z > radius_)
					return false;
				pcontact = relativePosition;
				sign0 = SIGN(pcontact.z);
				pcontact.z -= radius_*sign0;
				qcontact.set(pcontact.x, pcontact.y, box.getExtent().z*sign0);
			}
		}
	}
	boxOrientation.xform(pcontact);
	pcontact.add(boxPosition);
	boxOrientation.xform(qcontact);
	qcontact.add(boxPosition);
	return true;
}
