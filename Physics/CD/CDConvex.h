#ifndef __CD_CONVEX_H_INCLUDED__
#define __CD_CONVEX_H_INCLUDED__

#include <my_STL.h>
#include <algorithm>

namespace CD
{
///////////////////////////////////////////////////////////////
//
//    class Vertex
//
///////////////////////////////////////////////////////////////

	class Vertex: public Vect3f
	{
	public:
		xm_inline Vertex& operator = (const Vect3f& vect)
		{
			Vect3f::operator = (vect);
			return *this;
		}
	};

///////////////////////////////////////////////////////////////
//
//    class Edge
//
///////////////////////////////////////////////////////////////

	class Edge
	{
	public:
		xm_inline Vertex& vertex0()
		{
			return vertex0_;
		}
		xm_inline Vertex& vertex1()
		{
			return vertex1_;
		}
		xm_inline const Vertex& getVertex0() const
		{
			return vertex0_;
		}
		xm_inline const Vertex& getVertex1() const
		{
			return vertex1_;
		}
	private:
		Vertex vertex0_;
		Vertex vertex1_;
	};

///////////////////////////////////////////////////////////////
//
//    class Face
//
///////////////////////////////////////////////////////////////

	class Face
	{
	public:
		Face() {}
		Face(const Vect3f& normal, float distance):
		normal_(normal), distance_(distance) {}
		xm_inline void set(const Vect3f& normal, float distance)
		{
			normal_=normal;
			distance_=distance;
		}
		xm_inline const Vect3f& getNormal() const
		{
			return normal_;
		}
		xm_inline float getDistance() const
		{
			return distance_;
		}
		xm_inline Vect3f& normal()
		{
			return normal_;
		}
		xm_inline float& distance()
		{
			return distance_;
		}

	private:
		Vect3f normal_;
		float distance_;
	};

///////////////////////////////////////////////////////////////
//
//    class DualityVertexFV
//
///////////////////////////////////////////////////////////////

	class DualityVertexFV
	{
	public:
		xm_inline Face& face()
		{
			return face_;
		}
		xm_inline Vertex& vertex()
		{
			return vertex_;
		}
		xm_inline Face& minkowskiFace()
		{
			return minkowskiFace_;
		}
		xm_inline float& distance()
		{
			return distance_;
		}
		xm_inline bool operator<(DualityVertexFV b)
		{
			return distance_ < b.distance_;
		}
		xm_inline bool operator>(DualityVertexFV b)
		{
			return distance_ > b.distance_;
		}

	private:
		Face face_;
		Vertex vertex_;
		Face minkowskiFace_;
		float distance_; 
	};

///////////////////////////////////////////////////////////////
//
//    class DualityVertexEE
//
///////////////////////////////////////////////////////////////

	class DualityVertexEE
	{
	public:
		xm_inline Edge& edgeP()
		{
			return edgeP_;
		}
		xm_inline Edge& edgeQ()
		{
			return edgeQ_;
		}
		xm_inline Face& minkowskiFace()
		{
			return minkowskiFace_;
		}
		xm_inline float& distance()
		{
			return distance_;
		}
		xm_inline bool operator<(DualityVertexEE b)
		{
			return distance_ < b.distance_;
		}
		xm_inline bool operator>(DualityVertexEE b)
		{
			return distance_ > b.distance_;
		}
		xm_inline bool operator<(DualityVertexFV b)
		{
			return distance_ < b.distance();
		}
		xm_inline bool operator>(DualityVertexFV b)
		{
			return distance_ > b.distance();
		}

	private:
		Edge edgeP_;
		Edge edgeQ_;
		Face minkowskiFace_;
		float distance_; 
	};

///////////////////////////////////////////////////////////////
//
//    class Convex
//
///////////////////////////////////////////////////////////////

	class Convex
	{
	public:
		virtual Vect3f getCenter() const = 0;
		virtual Vect3f getInternalPoint() const = 0;
		virtual void computeSupportVertex(Vertex& supportVertex, const Vect3f& normal) const = 0;
		virtual void computeSupportFace(Face& supportFace, const Vect3f& normal) const = 0;
		virtual void computeAdjacentFaces(vector<DualityVertexFV>& adjacent, const Face& face) const = 0;
		virtual void computeFaceEdges(vector<Edge>& incident, const Face& face) const = 0;
		virtual void computeEdgeFacesNormals(Vect3f& normal0, Vect3f& normal1, const Edge& edge) const = 0;
		virtual void computeIncidentEdges(vector<Edge>& incident, const Vertex& vertex) const = 0;
		virtual void computeIncidentEdges(vector<Edge>& incident, const Edge& edge) const = 0;
		virtual void computeIncidentFaceEdges(vector<Edge>& incident, const Face& face, const Vertex& vertex) const = 0;
		virtual void computeIncidentFace(Face& incident, const Vertex& vertex) const = 0;
	};

///////////////////////////////////////////////////////////////
//
//    class Transform
//
///////////////////////////////////////////////////////////////

	class Transform : public Convex 
	{
	public:
		Transform(const Se3f& _pose, const Convex& _child) :
		pose(_pose), child(_child) 
		{
		}

		Vect3f getCenter() const
		{
			Vect3f center(child.getCenter());
			return pose.xformPoint(center);
		}

		Vect3f getInternalPoint() const
		{
			Vect3f internalPoint(child.getInternalPoint());
			return pose.xformPoint(internalPoint);
		}

		void computeSupportVertex(Vertex& supportVertex, const Vect3f& normal) const
		{
			supportVertex = normal;
			pose.invXformVect(supportVertex);
			child.computeSupportVertex(supportVertex, supportVertex);
			pose.xformPoint(supportVertex);
		}
		void computeSupportFace(Face& supportFace, const Vect3f& normal) const
		{
			Vect3f temp = normal;
			pose.invXformVect(temp);
			child.computeSupportFace(supportFace, temp);
			pose.xformVect(supportFace.normal());
			supportFace.distance() += pose.trans().dot(supportFace.getNormal());
		}
		void computeAdjacentFaces(vector<DualityVertexFV>& adjacent, const Face& face) const
		{
			Face temp(face);
			pose.invXformVect(temp.normal());
			temp.distance() -= pose.trans().dot(temp.getNormal());
			child.computeAdjacentFaces(adjacent, temp);
			vector<DualityVertexFV>::iterator iadjacent;
			FOR_EACH(adjacent, iadjacent)
			{
				pose.xformVect(iadjacent->face().normal());
				iadjacent->face().distance() += pose.trans().dot(iadjacent->face().getNormal());
			}
		}
		void computeFaceEdges(vector<Edge>& incident, const Face& face) const
		{
			Face temp(face);
			pose.invXformVect(temp.normal());
			temp.distance() -= pose.trans().dot(temp.getNormal());
			child.computeFaceEdges(incident, temp);
			vector<Edge>::iterator iincident;
			FOR_EACH(incident, iincident)
			{
				pose.xformPoint(iincident->vertex0());
				pose.xformPoint(iincident->vertex1());
			}
		}
		void computeEdgeFacesNormals(Vect3f& normal0, Vect3f& normal1, const Edge& edge) const
		{
			Edge temp(edge);
			pose.invXformPoint(temp.vertex0());
			pose.invXformPoint(temp.vertex1());
			child.computeEdgeFacesNormals(normal0, normal1, temp);
			pose.xformVect(normal0);
			pose.xformVect(normal1);
		}
		void computeIncidentEdges(vector<Edge>& incident, const Vertex& vertex) const
		{
			Vertex temp(vertex);
			pose.invXformPoint(temp);
			child.computeIncidentEdges(incident, temp);
			vector<Edge>::iterator iincident;
			FOR_EACH(incident, iincident)
			{
				pose.xformPoint(iincident->vertex0());
				pose.xformPoint(iincident->vertex1());
			}
		}
		void computeIncidentEdges(vector<Edge>& incident, const Edge& edge) const
		{
			Edge temp(edge);
			pose.invXformPoint(temp.vertex0());
			pose.invXformPoint(temp.vertex1());
			child.computeIncidentEdges(incident, temp);
			vector<Edge>::iterator iincident;
			FOR_EACH(incident, iincident)
			{
				pose.xformPoint(iincident->vertex0());
				pose.xformPoint(iincident->vertex1());
			}
		}
		void computeIncidentFaceEdges(vector<Edge>& incident, const Face& face, const Vertex& vertex) const
		{
			Vertex tempVertex(vertex);
			pose.invXformPoint(tempVertex);
			Face tempFace(face);
			pose.invXformVect(tempFace.normal());
			tempFace.distance() -= pose.trans().dot(tempFace.getNormal());
			child.computeIncidentEdges(incident, tempVertex);
			vector<Edge>::iterator iincident;
			FOR_EACH(incident, iincident)
			{
				pose.xformPoint(iincident->vertex0());
				pose.xformPoint(iincident->vertex1());
			}
		}
		void computeIncidentFace(Face& incident, const Vertex& vertex) const
		{
			Vect3f temp = vertex;
			pose.invXformPoint(temp);
			child.computeSupportFace(incident, temp);
			pose.xformVect(incident.normal());
			incident.distance() += pose.trans().dot(incident.getNormal());
		}

	private:
		const MatXf pose;
		const Convex& child;
	};

	xm_inline int maxAbsIndex(const Vect3f& v)
	{
		Vect3f vabs(v.x * v.x, v.y * v.y, v.z * v.z);
		return vabs.x > vabs.y ? (vabs.x > vabs.z ? 0 : 2) : (vabs.y > vabs.z ? 1 : 2);
	}

	xm_inline int minAbsIndex(const Vect3f& v)
	{
		Vect3f vabs(v.x * v.x, v.y * v.y, v.z * v.z);
		return vabs.x < vabs.y ? (vabs.x < vabs.z ? 0 : 2) : (vabs.y < vabs.z ? 1 : 2);
	}

///////////////////////////////////////////////////////////////
//
//    class Box
//
///////////////////////////////////////////////////////////////

	class Box : public Convex
	{
	public:
		Box(const Vect3f& _extent) : extent(_extent) {}
		void setExtent(const Vect3f& _extent)	
		{ 
			xassert(_extent.x > FLT_EPS && _extent.y > FLT_EPS && _extent.z > FLT_EPS); 
			extent = _extent;	
		}
		const Vect3f& getExtent() const	{ return extent; }
		Vect3f getCenter() const { return Vect3f::ZERO;	}
		Vect3f getInternalPoint() const	{ return extent / 2.0f;	}
        void computeSupportVertex(Vertex& supportVertex, const Vect3f& normal) const;
		void computeSupportFace(Face& supportFace, const Vect3f& normal) const;
		void computeAdjacentFaces(vector<DualityVertexFV>& adjacent, const Face& face) const;
		void computeFaceEdges(vector<Edge>& incident, const Face& face) const;
		void computeEdgeFacesNormals(Vect3f& normal0, Vect3f& normal1, const Edge& edge) const;
		void computeIncidentEdges(vector<Edge>& incident, const Vertex& vertex) const;
		void computeIncidentEdges(vector<Edge>& incident, const Edge& edge) const;
		void computeIncidentFaceEdges(vector<Edge>& incident, const Face& face, const Vertex& vertex) const;
		void computeIncidentFace(Face& incident, const Vertex& vertex) const;
		bool computeBoxSectionPenetration(Vect3f& sectionBegin, Vect3f& sectionEnd) const;
		
	private:
		Vect3f extent;
	};
}

#endif // __CD_CONVEX_H_INCLUDED__
