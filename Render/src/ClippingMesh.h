#ifndef __CLIPPING_MESH_H_INCLUDED__
#define __CLIPPING_MESH_H_INCLUDED__

#include "XTL\UniqueVector.h"
#include "XMath\XMath.h"
#include "XMath\Box6f.h"
#include "XMath\Mat4f.h"
#include "XMath\Plane.h"

class Camera;

struct CVertex
{
	Vect3f point;
	float distance;
	int occurs;
	bool visible;

	CVertex(){	distance=0;occurs=0;visible=true;}
};

struct CEdge
{
	typedef UniqueVector<int> Faces;

	int vertex[2];
	Faces faces;
	bool visible;

	CEdge() { visible = true; }
};

struct CFace
{
	typedef UniqueVector<int> Edges;

	Edges edges;
	bool visible;

	CFace(){visible=true;}
};

struct APolygons
{
	vector<Vect3f> points;

	//формат такой сначала идёт один int - количество элементов в полигоне (N).
	//потом N элементов - индексы точек в points
	vector<int> faces_flat;
};

///Класс для усечения выпуклого техмерного полигона плоскостями.
struct ClippingMesh
{
public:
	ClippingMesh(float zmax);	
	void createBox(Vect3f& vmin,Vect3f& vmax);
	void createTileBox(Camera* camera, Vect2i TileNumber,Vect2i TileSize);

	void calcBoundTransformed(const Mat3f& m, sBox6f& box);
	void calcBoundTransformed(const Mat4f& m, sBox6f& box);

	int clip(const Plane& clipplane);

	//Величина visMap должна быть TileMap->GetTileNumber().x*visMapDy=TileMap->GetTileNumber().y
	void calcVisMap(Camera* camera, Vect2i TileNumber,Vect2i TileSize,BYTE* visMap,bool clear);
	void calcVisBox(Camera* camera, Vect2i TileNumber,Vect2i TileSize,const Mat4f& direction,sBox6f& box);

	void BuildPolygon(APolygons& p);
	void draw();

protected:
	float epsilon;
	float zmax_;

	vector<CVertex> V;
	vector<CEdge> E;
	vector<CFace> F;

	void fillVisPoly(BYTE *buf,Vect2f* vert,int vert_size,int VISMAP_W,int VISMAP_H);
	bool GetOpenPolyline(const CFace& face,int& start,int& final);
};

void drawTestGrid(unsigned char* grid, const Vect2i& size);

#endif
