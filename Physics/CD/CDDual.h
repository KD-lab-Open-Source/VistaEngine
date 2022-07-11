#ifndef __CD_DUAL_H_INCLUDED__
#define __CD_DUAL_H_INCLUDED__

#include "CDConvex.h"

namespace CD
{
///////////////////////////////////////////////////////////////
//
//    class CDDuality
//
///////////////////////////////////////////////////////////////

	class CDDuality
	{
	public:
		CDDuality(const Convex& p, const Convex& q) :
			p_(p), q_(q) {}

		bool computeBoxBoxPenetration(Vect3f& pcontact, Vect3f& qcontact)
		{
			start_timer_auto();
			c.sub(p_.getCenter(), q_.getCenter());
			
			p_.computeSupportFace(maxFV.face(), -c);
			q_.computeSupportVertex(maxFV.vertex(), -maxFV.face().getNormal());
			maxFV.distance() = maxFV.vertex().dot(maxFV.face().getNormal()) - maxFV.face().getDistance();
			if(maxFV.distance() > FLT_EPS)
				return false;

			q_.computeSupportFace(maxVF.face(), c);
			p_.computeSupportVertex(maxVF.vertex(), -maxVF.face().getNormal());
			maxVF.distance() = maxVF.vertex().dot(maxVF.face().getNormal()) - maxVF.face().getDistance();
			if(maxVF.distance() > FLT_EPS)
				return false;

			p_.computeIncidentFaceEdges(pEdges, maxFV.face(), maxVF.vertex());
			q_.computeIncidentFaceEdges(qEdges, maxVF.face(), maxFV.vertex());
			
			maxEE.distance() = maxFV.distance();
			
			adjacentEE.clear();
			pushAllPairs();
			while(maxEE.distance() < FLT_EPS && adjacentEE.size() > 0){
				vector<DualityVertexEE>::iterator tempAdjacent(adjacentEE.end()-1);
				if(checkFaceEE(*tempAdjacent)){
					Vect3f norm;
					norm.sub(tempAdjacent->edgeP().vertex1(), tempAdjacent->edgeP().vertex0());
					norm.postcross(tempAdjacent->edgeQ().vertex1() - tempAdjacent->edgeQ().vertex0());
					norm.Normalize(SIGN(c.dot(norm)));
                    tempAdjacent->distance() = tempAdjacent->edgeP().vertex1().dot(norm) - tempAdjacent->edgeQ().vertex1().dot(norm);
					if(maxEE < *tempAdjacent)
						maxEE = *tempAdjacent;
				}
				adjacentEE.pop_back();
			}

			if(maxEE.distance() > FLT_EPS)
				return false;

			if(maxVF > maxFV){
				if(maxEE > maxVF)
					penetrationEE(pcontact, qcontact);
				else
					penetrationVF(pcontact, qcontact);
			}
			else{
				if(maxEE > maxFV)
					penetrationEE(pcontact, qcontact);
				else
					penetrationFV(pcontact, qcontact);
			}
			return true;
		}

		bool computePenetrationDistance(Vect3f& pcontact, Vect3f& qcontact)
		{
			start_timer_auto();
			c = p_.getCenter() - q_.getCenter();
			cnorm = c.norm();
			if(c.norm() < FLT_EPS)
			{
				c = p_.getCenter() - q_.getInternalPoint();
				cnorm = c.norm();
			}
			if(penetration())
			{
				if(maxVF > maxFV)
				{
					if(maxEE > maxVF)
						penetrationEE(pcontact, qcontact);
					else
						penetrationVF(pcontact, qcontact);
				}
				else
				{
					if(maxEE > maxFV)
					{
						penetrationEE(pcontact, qcontact);
					}
					else
					{
						penetrationFV(pcontact, qcontact);
					}
				}
				return true;
			}
			return false;
		}

	private:
		xm_inline bool searchFV()
		{
			start_timer_auto();
			precomputeMaxFV();
			signedDistanceFV(maxFV);
			while(maxFV.distance() < FLT_EPS)
			{
				p_.computeAdjacentFaces(adjacent, maxFV.face());
				vector<DualityVertexFV>::iterator itAdjacent;
				FOR_EACH(adjacent, itAdjacent)
					signedDistanceFV(*itAdjacent);
				DualityVertexFV& adjacentMax(*max_element(adjacent.begin(),adjacent.end()));
				if(maxFV < adjacentMax)
				{
					maxFV = adjacentMax;

				}
				else
				{
					return true;
				}
			}
			return false;
		}

		xm_inline void precomputeMaxFV()
		{
			p_.computeSupportFace(maxFV.face(), -c);
		}

		xm_inline void signedDistanceFV(DualityVertexFV& dualityVertex)
		{
			q_.computeSupportVertex(dualityVertex.vertex(), -dualityVertex.face().getNormal());
			computeFaceFV(dualityVertex);
			dualityVertex.distance() = computeDistance(dualityVertex.minkowskiFace());
		}
		
		xm_inline void computeFaceFV(DualityVertexFV& dualityVertex)
		{
			float distance = dualityVertex.vertex().dot(dualityVertex.face().getNormal());
			distance -= dualityVertex.face().getDistance();
			dualityVertex.minkowskiFace().set(dualityVertex.face().getNormal(), -distance);
		}
		

		xm_inline bool searchVF()
		{
			start_timer_auto();
			precomputeMaxVF();
			signedDistanceVF(maxVF);
			while(maxVF.distance() < FLT_EPS)
			{
				q_.computeAdjacentFaces(adjacent, maxVF.face());
				vector<DualityVertexFV>::iterator itAdjacent;
				FOR_EACH(adjacent, itAdjacent)
					signedDistanceVF(*itAdjacent);
				DualityVertexFV& adjacentMax(*max_element(adjacent.begin(),adjacent.end()));
				if(maxVF < adjacentMax)
				{
					maxVF = adjacentMax;
				}
				else
				{
					return true;
				}
			}
			return false;
		}

		xm_inline void precomputeMaxVF()
		{
			q_.computeIncidentFace(maxVF.face(), maxFV.vertex());
		}

		xm_inline void signedDistanceVF(DualityVertexFV& dualityVertex)
		{
			p_.computeSupportVertex(dualityVertex.vertex(), -dualityVertex.face().getNormal());
			computeFaceVF(dualityVertex);
			dualityVertex.distance() = computeDistance(dualityVertex.minkowskiFace());
		}

		xm_inline void computeFaceVF(DualityVertexFV& dualityVertex)
		{
			float distance=dualityVertex.vertex().dot(dualityVertex.face().getNormal());
			distance-=dualityVertex.face().getDistance();
			dualityVertex.minkowskiFace().set(-dualityVertex.face().getNormal(),-distance);
		}

		xm_inline void searchEE()
		{
			start_timer_auto();
			if(maxFV > maxVF)
			{
				p_.computeFaceEdges(pEdges, maxFV.face());
				q_.computeIncidentEdges(qEdges, maxFV.vertex());
				maxEE.distance() = maxFV.distance();
			}
			else
			{
				p_.computeIncidentEdges(pEdges, maxVF.vertex());
				q_.computeFaceEdges(qEdges, maxVF.face());
				maxEE.distance() = maxVF.distance();
			}
			adjacentEE.clear();
			pushAllPairs();
			while(maxEE.distance() < FLT_EPS && adjacentEE.size() > 0)
			{
				DualityVertexEE tempAdjacent(*(adjacentEE.end()-1));
				adjacentEE.pop_back();
							
				if(computeFaceEE(tempAdjacent))
				{
					tempAdjacent.distance() = computeDistance(tempAdjacent.minkowskiFace());
					if(maxEE < tempAdjacent)
					{
						maxEE = tempAdjacent;
						p_.computeIncidentEdges(pEdges, tempAdjacent.edgeP());
						q_.computeIncidentEdges(qEdges, tempAdjacent.edgeQ());
						pushAllPairs();
					}
				}
			}
		}

		xm_inline void pushAllPairs()
		{
			//int size(adjacentEE.size());
			adjacentEE.resize(/*size + */pEdges.size() * qEdges.size());
			vector<Edge>::iterator ipedges, iqedges;
			vector<DualityVertexEE>::iterator iadjacent = adjacentEE.begin()/* + size*/;
			FOR_EACH(pEdges, ipedges)
			{
				FOR_EACH(qEdges, iqedges)
				{
					iadjacent->edgeP() = *ipedges;
					iadjacent->edgeQ() = *iqedges;
					++iadjacent;
				}
			}
		}

		xm_inline bool checkFaceEE(DualityVertexEE& dualityVertex)
		{
			Vect3f temp_a, temp_b, temp_c, temp_d;
			p_.computeEdgeFacesNormals(temp_a, temp_b, dualityVertex.edgeP());
			q_.computeEdgeFacesNormals(temp_c, temp_d, dualityVertex.edgeQ());
			temp_c.negate();
			temp_d.negate();
			float cba(signedVolume(temp_c, temp_b, temp_a)), dba(signedVolume(temp_d, temp_b, temp_a)), adc(signedVolume(temp_a, temp_d, temp_c)), bdc(signedVolume(temp_b, temp_d, temp_c));
			if((cba * dba < -FLT_EPS) && (adc * bdc < -FLT_EPS) && (cba * bdc > FLT_EPS))
				return true;
            return false;
		}

		xm_inline bool computeFaceEE(DualityVertexEE& dualityVertex)
		{
			Vect3f temp_a, temp_b, temp_c, temp_d;
			if(checkFaceEE(dualityVertex))
			{
				temp_a.sub(dualityVertex.edgeP().vertex0(), dualityVertex.edgeP().vertex1());
				temp_b.sub(dualityVertex.edgeQ().vertex1(), dualityVertex.edgeQ().vertex0());
				temp_c.cross(temp_a, temp_b);
				//temp_c.normalize();
				temp_d.sub(dualityVertex.edgeP().vertex0(), dualityVertex.edgeQ().vertex0());
				dualityVertex.minkowskiFace().set(temp_c, temp_d.dot(temp_c));
				return true;
			}
			return false;
		}

		xm_inline float signedVolume(Vect3f& temp_a, Vect3f& temp_b, Vect3f& temp_c)
		{
			return temp_a.x * temp_b.y * temp_c.z + temp_a.z * temp_b.x * temp_c.y + temp_a.y * temp_b.z * temp_c.x -
				temp_a.z * temp_b.y * temp_c.x - temp_a.x * temp_b.z * temp_c.y - temp_a.y * temp_b.x * temp_c.z;
		}
		xm_inline float computeDistance(Face& face)
		{
			float temp(c.dot(face.getNormal()));
			temp -= face.getDistance();
			temp *= cnorm;
			xassert(fabs(temp) > FLT_EPS);
			return face.getDistance() / temp;
		}

		xm_inline bool penetration()
		{
			if(searchFV())
				if(searchVF())
				{
					searchEE();
					if(maxEE.distance() < FLT_EPS)
						return true;
				}
			return false;
		}
		
		xm_inline void penetrationFV(Vect3f& pcontact, Vect3f& qcontact)
		{
			pcontact = maxFV.face().getNormal();
			pcontact *= (maxFV.face().getDistance() - maxFV.face().getNormal().dot(maxFV.vertex()));
			pcontact += maxFV.vertex();
			qcontact = maxFV.vertex();
		}

		xm_inline void penetrationVF(Vect3f& pcontact, Vect3f& qcontact)
		{
			pcontact = maxVF.vertex();
			qcontact = maxVF.face().getNormal();
			qcontact *= (maxVF.face().getDistance() - maxVF.face().getNormal().dot(maxVF.vertex()));
			qcontact += maxVF.vertex();
		}

		xm_inline void penetrationEE(Vect3f& pcontact, Vect3f& qcontact)
		{
			pcontact.sub(maxEE.edgeP().vertex0(), maxEE.edgeP().vertex1());
			qcontact.sub(maxEE.edgeQ().vertex0(), maxEE.edgeQ().vertex1());
			Vect3f normal0(qcontact), normal1(pcontact);
			float temp(pcontact.dot(qcontact));
			normal0.scale(-temp);
			normal1.scale(-temp);
			normal0.scaleAdd(pcontact, qcontact.norm2());
			normal1.scaleAdd(qcontact, pcontact.norm2());
			temp = maxEE.edgeP().vertex0().dot(normal0);
			pcontact.scale((maxEE.edgeQ().vertex0().dot(normal0) - temp)/(temp - maxEE.edgeP().vertex1().dot(normal0)));
			pcontact.add(maxEE.edgeP().vertex0());
			temp = maxEE.edgeQ().vertex0().dot(normal1);
			qcontact.scale((maxEE.edgeP().vertex0().dot(normal1) - temp)/(temp - maxEE.edgeQ().vertex1().dot(normal1)));
			qcontact.add(maxEE.edgeQ().vertex0());
		}

	private:
		const Convex& p_;
		const Convex& q_;
		Vect3f c;
		float cnorm;
		DualityVertexFV maxFV;
		DualityVertexFV maxVF;
		DualityVertexEE maxEE;
		vector<DualityVertexFV> adjacent;
		vector<DualityVertexEE> adjacentEE;
		vector<Edge> pEdges;
		vector<Edge> qEdges;
	};
}

#endif // __CD_DUAL_H_INCLUDED__
