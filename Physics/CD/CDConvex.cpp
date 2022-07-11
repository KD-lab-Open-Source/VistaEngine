#include "StdAfx.h"
#include "CDConvex.h"

using namespace CD;

void Box::computeSupportVertex(Vertex& supportVertex, const Vect3f& normal) const
{
	supportVertex.set(normal[0] < 0.0f ? -extent.x : extent.x,
		normal[1] < 0.0f ? -extent.y : extent.y,
		normal[2] < 0.0f ? -extent.z : extent.z); 
}

void Box::computeSupportFace(Face& supportFace, const Vect3f& normal) const
{
	Vect3f normal_scaled(normal.x / extent.x, normal.y / extent.y, normal.z / extent.z);

	switch(maxAbsIndex(normal_scaled))
	{
	case 0:
		supportFace.set(Vect3f::I*SIGN(normal.x),extent.x);
		return;
	case 1:
		supportFace.set(Vect3f::J*SIGN(normal.y),extent.y);
		return;
	case 2:
		supportFace.set(Vect3f::K*SIGN(normal.z),extent.z);
	}
}

void Box::computeAdjacentFaces(vector<DualityVertexFV>& adjacent, const Face& face) const
{
	adjacent.resize(4);

	switch(maxAbsIndex(face.getNormal()))
	{
	case 0:
		adjacent[0].face().set(Vect3f::J, extent.y);
		adjacent[1].face().set(-Vect3f::J, extent.y);
		adjacent[2].face().set(Vect3f::K, extent.z);
		adjacent[3].face().set(-Vect3f::K, extent.z);
		return;
	case 1:
		adjacent[0].face().set(Vect3f::K, extent.z);
		adjacent[1].face().set(-Vect3f::K, extent.z);
		adjacent[2].face().set(Vect3f::I, extent.x);
		adjacent[3].face().set(-Vect3f::I, extent.x);
		return;
	case 2:
		adjacent[0].face().set(Vect3f::J, extent.y);
		adjacent[1].face().set(-Vect3f::J, extent.y);
		adjacent[2].face().set(Vect3f::I, extent.x);
		adjacent[3].face().set(-Vect3f::I, extent.x);
	}
}

void Box::computeFaceEdges(vector<Edge>& incident, const Face& face) const
{
	incident.resize(4);

	Vect3f faceExtentX, faceExtentY, faceCenter;

	switch(maxAbsIndex(face.getNormal()))
	{
	case 0:
		faceCenter = Vect3f::I;
		faceCenter *= SIGN(face.getNormal().x) * extent.x;
		faceExtentX = Vect3f::J;
		faceExtentX *= extent.y;
		faceExtentY = Vect3f::K;
		faceExtentY *= extent.z;
		break;
	case 1:
		faceCenter = Vect3f::J;
		faceCenter *= SIGN(face.getNormal().y) * extent.y;
		faceExtentX = Vect3f::K;
		faceExtentX *= extent.z;
		faceExtentY = Vect3f::I;
		faceExtentY *= extent.x;
		break;
	case 2:
		faceCenter = Vect3f::K;
		faceCenter *= SIGN(face.getNormal().z) * extent.z;
		faceExtentX = Vect3f::J;
		faceExtentX *= extent.y;
		faceExtentY = Vect3f::I;
		faceExtentY *= extent.x;
	}

	incident[0].vertex0() = faceCenter;
	incident[0].vertex0() += faceExtentX;
	incident[0].vertex0() += faceExtentY;
	incident[3].vertex1() = incident[0].vertex0();

	incident[1].vertex0() = faceCenter;
	incident[1].vertex0() += faceExtentX;
	incident[1].vertex0() -= faceExtentY;
	incident[0].vertex1() = incident[1].vertex0();

	incident[2].vertex0() = faceCenter;
	incident[2].vertex0() -= faceExtentX;
	incident[2].vertex0() -= faceExtentY;
	incident[1].vertex1() = incident[2].vertex0();

	incident[3].vertex0() = faceCenter;
	incident[3].vertex0() -= faceExtentX;
	incident[3].vertex0() += faceExtentY;
	incident[2].vertex1() = incident[3].vertex0();
}

void Box::computeEdgeFacesNormals(Vect3f& normal0, Vect3f& normal1, const Edge& edge) const
{
	Vect3f temp;
	temp.add(edge.getVertex0(), edge.getVertex1());
	switch(minAbsIndex(temp))
	{
	case 0:
		normal0 = Vect3f::J;
		normal0 *= SIGN(temp.y);
		normal1 = Vect3f::K;
		normal1 *= SIGN(temp.z);
		break;
	case 1:
		normal0 = Vect3f::I;
		normal0 *= SIGN(temp.x);
		normal1 = Vect3f::K;
		normal1 *= SIGN(temp.z);
		break;
	case 2:
		normal0 = Vect3f::I;
		normal0 *= SIGN(temp.x);
		normal1 = Vect3f::J;
		normal1 *= SIGN(temp.y);
	}

}

void Box::computeIncidentEdges(vector<Edge>& incident, const Vertex& vertex) const
{
	incident.resize(3);

	incident[0].vertex0() = vertex;
	incident[0].vertex1() = vertex;
	incident[0].vertex1().x *= -1;

	incident[1].vertex0() = vertex;
	incident[1].vertex1() = vertex;
	incident[1].vertex1().y *= -1;

	incident[2].vertex0() = vertex;
	incident[2].vertex1() = vertex;
	incident[2].vertex1().z *= -1;
}

void Box::computeIncidentEdges(vector<Edge>& incident, const Edge& edge) const
{
	incident.resize(4);

	incident[0].vertex0() = edge.getVertex0();
	incident[0].vertex1() = edge.getVertex0();
	incident[1].vertex0() = edge.getVertex0();
	incident[1].vertex1() = edge.getVertex0();

	incident[2].vertex0() = edge.getVertex1();
	incident[2].vertex1() = edge.getVertex1();
	incident[3].vertex0() = edge.getVertex1();
	incident[3].vertex1() = edge.getVertex1();

	switch(maxAbsIndex(edge.getVertex0() - edge.getVertex1()))
	{
	case 0:
		incident[0].vertex1().y *= -1;
		incident[1].vertex1().z *= -1;
		incident[2].vertex1().y *= -1;
		incident[3].vertex1().z *= -1;
		break;
	case 1:
		incident[0].vertex1().x *= -1;
		incident[1].vertex1().z *= -1;
		incident[2].vertex1().x *= -1;
		incident[3].vertex1().z *= -1;
		break;
	case 2:
		incident[0].vertex1().y *= -1;
		incident[1].vertex1().x *= -1;
		incident[2].vertex1().y *= -1;
		incident[3].vertex1().x *= -1;
		break;
	}
}

void Box::computeIncidentFaceEdges(vector<Edge>& incident, const Face& face, const Vertex& vertex) const
{
	incident.resize(2);

	switch(maxAbsIndex(face.getNormal()))
	{
	case 0:
		incident[0].vertex0() = vertex;
		incident[0].vertex1() = vertex;
		incident[0].vertex1().y *= -1;
		incident[1].vertex0() = vertex;
		incident[1].vertex1() = vertex;
		incident[1].vertex1().z *= -1;
		break;
	case 1:
		incident[0].vertex0() = vertex;
		incident[0].vertex1() = vertex;
		incident[0].vertex1().x *= -1;
		incident[1].vertex0() = vertex;
		incident[1].vertex1() = vertex;
		incident[1].vertex1().z *= -1;
		break;
	case 2:
		incident[0].vertex0() = vertex;
		incident[0].vertex1() = vertex;
		incident[0].vertex1().x *= -1;
		incident[1].vertex0() = vertex;
		incident[1].vertex1() = vertex;
		incident[1].vertex1().y *= -1;
	}
}

void Box::computeIncidentFace(Face& incident, const Vertex& vertex) const
{
	if(vertex.x > 0.0f)
		incident.set(Vect3f::I,extent.x);
	else
		incident.set(-Vect3f::I,extent.x);
}

bool Box::computeBoxSectionPenetration(Vect3f& sectionBegin, Vect3f& sectionEnd) const
{
	float begin, end;
	Vect3f dir;
	bool initialised(false);
	for(int i = 0; i < 3; ++i){
		if(sectionBegin[i] < sectionEnd[i]) {
			if(sectionBegin[i] > extent[i] || sectionEnd[i] < -extent[i])
				return false;
			dir[i] = sectionEnd[i] - sectionBegin[i];
			if(dir[i] > FLT_EPS) {
				float beginTemp((-extent[i] - sectionBegin[i]) / dir[i]);
				if(!initialised || beginTemp > begin)
					begin = beginTemp;
				float endTemp((extent[i] - sectionBegin[i]) / dir[i]);
				if(!initialised || endTemp < end)
					end = endTemp;
				initialised = true;
			}

		} else {
			if(sectionEnd[i] > extent[i] || sectionBegin[i] < -extent[i])
				return false;
			dir[i] = sectionEnd[i] - sectionBegin[i];
			if(dir[i] < -FLT_EPS) {
				float beginTemp((extent[i] - sectionBegin[i]) / dir[i]);
				if(!initialised || beginTemp > begin)
					begin = beginTemp;
				float endTemp((-extent[i] - sectionBegin[i]) / dir[i]);
				if(!initialised || endTemp < end)
					end = endTemp;
				initialised = true;
			}
		}
	}

	if(initialised){
		if(end < begin)
			return false;
		sectionBegin.scaleAdd(dir, end > 0 ? begin : end);
	}
	return true;
}
