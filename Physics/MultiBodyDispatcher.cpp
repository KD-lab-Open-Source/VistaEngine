#include "stdafx.h"
#include "RigidBody.h"
#include "CD/CD-Sphere.h"
#include "CD/CD-Dual.h"

//--------------------------------------------------------
int closestFeaturesHTsizeMax = 500; // Максимальный размер хеш-таблицы перед автоочищением
float collision_detection_epsilon = 2;

//--------------------------------------------------------
MultiBodyDispatcher::MultiBodyDispatcher()
{
	RigidBody::IDs = 0;
}

bool MultiBodyDispatcher::test(RigidBody& b1, RigidBody& b2, MatXf& Xr1r2, ContactInfo& contactInfo)
{
	start_timer_auto(test, STATISTICS_GROUP_PHYSICS);
	Vect3f cp1, cp2;
	float dist = Xr1r2.trans().norm();
	if(b1.boundRadius() + b2.boundRadius() < dist)
		return false;
	float penetration = b1.inscribed_radius + b2.inscribed_radius - dist;
	if(penetration > 0){
		if(dist > FLT_EPS){	
			cp1 = Xr1r2.trans();
			cp1.Normalize();
			Xr1r2.xformVect(cp1, cp2);
			cp1 *= b1.inscribed_radius;
			cp2 *= -b2.inscribed_radius;
		}
		else{
			cp1.set(0.01f, 0, 0);
			cp2.set(-0.01f, 0, 0);
		}
	}
	else if(b1.prm().polyhedral_bound || b2.prm().polyhedral_bound){ 
			if(vclipe_collision_detection){
			start_timer_auto(vclip, STATISTICS_GROUP_PHYSICS);
			penetration = collision_detection_epsilon - PolyTree::vclip(b1.polytree, b2.polytree, Xr1r2, closest_features_ht, BodyPair(b1.ID, b2.ID), cp1, cp2);
			if(!b1.prm().polyhedral_bound){
				penetration += b1.boundRadius();
				Vect3f delta;
				Xr1r2.invXformPoint(cp2, delta);
				delta -= cp1;
				delta.Normalize(b1.boundRadius());
				cp1 += delta;
			}
			if(!b2.prm().polyhedral_bound){
				penetration += b2.boundRadius();
				Vect3f delta;
				Xr1r2.xformPoint(cp1, delta);
				delta -= cp2;
				delta.Normalize(b2.boundRadius());
				cp2 += delta;
			}
		}
		else
		{
			Xr1r2.trans().sub(b2.centreOfGravity);
			Vect3f temp(b1.centreOfGravity);
			Xr1r2.trans().add(Xr1r2.xformVect(temp));
			if(!b1.prm().polyhedral_bound){
				start_timer_auto(CDSphere, STATISTICS_GROUP_PHYSICS);
				CD::Sphere sphere(b1.boundRadius());
				if(!sphere.computeBoxPenetration(cp1, cp2, b2.box, Xr1r2))
					return false;
			}
			else{
				if(!b2.prm().polyhedral_bound){
					start_timer_auto(CDSphere, STATISTICS_GROUP_PHYSICS);
					CD::Sphere sphere(b2.boundRadius());
					MatXf relativePose;
					relativePose.Invert(Xr1r2);
					if(!sphere.computeBoxPenetration(cp2, cp1, b1.box, relativePose))
						return false;
				}
				else{
					start_timer_auto(CDBox, STATISTICS_GROUP_PHYSICS);
					CD::CDDuality penetrate(CD::Transform(Xr1r2, b1.box), b2.box);
					if(!penetrate.computePenetrationDistance(cp1, cp2))
						return false;
					Xr1r2.invXformPoint(cp1);
				}
			}
			cp1.sub(b1.centreOfGravity);
			cp2.sub(b2.centreOfGravity);
		}
	}

	contactInfo.cp1l = cp1;
	contactInfo.cp2l = cp2;
	contactInfo.body1 = &b1;
	contactInfo.body2 = &b2;

	//Vect3f p1, p2;
	//b1.matrix().xformPoint(cp1, p1);
	//b2.matrix().xformPoint(cp2, p2);
	//show_line(p1, p2, penetration > 0 ? MAGENTA : GREEN);

    if(!vclipe_collision_detection || penetration > 0)
		return true;
	
	//show_line(b1.position(), b2.position(), GREEN);
	return false;
}

void MultiBodyDispatcher::prepare()
{
	if(closest_features_ht.size() > closestFeaturesHTsizeMax)
		closest_features_ht.clear();
}
  
