#include "StdAfx.h"
#include "Universe.h"
#include "SecondMap.h"
#include "PFTrap.h"
#include "Nature.h"

namespace {

	bool drawToElevatorMap;

}

//=======================================================
class PlaceModels {

	cObject3dx* ignoredModel;

public:

	PlaceModels(cObject3dx* _ignoredModel) { ignoredModel = _ignoredModel; }

	void operator () (UnitBase* p) {
	
		if(!p->attr().isEnvironment())
			return;

		UnitEnvironmentBuilding* eu = safe_cast<UnitEnvironmentBuilding*>(p);
		if(eu->environmentType() != ENVIRONMENT_BRIDGE)
			return;

		if(eu->model() == ignoredModel)
			return;

		eu->placeToSecondMap();
	}
};

//=======================================================
SecondMap::SecondMap( int hsize, int vsize )
: Map2D<SecondMapTile, SecondMapTile::h_shift>(hsize,vsize)
{
	mapPresent_ = false;
}

//=======================================================
SecondMap::~SecondMap()
{

}

//=======================================================
void SecondMap::removeModel( cObject3dx* model )
{
	clearRegion(w2m(model->GetPosition().trans().x), w2m(model->GetPosition().trans().y), w2m(model->GetBoundRadius()));
	
	PlaceModels pm(model);
	universe()->unitGrid.Scan(model->GetPosition().trans().x, model->GetPosition().trans().y, model->GetBoundRadius(), pm);
}

//=======================================================
void SecondMap::clearRegion( int x, int y, int r )
{
	int xl = x-r;
	int xr = x+r;
	int yl = y-r;
	int yr = y+r;
	int rr = sqr(r);
	
	for(int yy = yl; yy<yr; yy++)
		for(int xx = xl; xx<xr; xx++)
			if(sqr(xx-x)+sqr(yy-y) <= rr) {
				(*this)(xx,yy).setH(0);
				(*this)(xx,yy).setElevator(false);
				pathFinder->updateTile(xx,yy);
			}
}

//=======================================================
void SecondMap::projectModel( cObject3dx* model )
{
	string name = model->GetVisibilityGroup()->name;
	
	if(model->SetVisibilityGroup("secondmap", true)){
		
		drawToElevatorMap = false;

		vector<sPolygon> polygonArr;
		vector<sPolygon>::iterator pIt;
		vector<Vect3f> pointArr;

		TriangleInfo all;
		model->GetTriangleInfo(all,TIF_TRIANGLES|TIF_POSITIONS);
		polygonArr.swap(all.triangles);
		pointArr.swap(all.positions);

		MatXf pose = model->GetPosition();
		pose.rot().scale(model->GetScale());

		for(int i = 0; i<pointArr.size(); i++)
			pose.xformPoint(pointArr[i]);
		
		FOR_EACH(polygonArr, pIt)
			drawTriangle(pointArr[pIt->p1], pointArr[pIt->p2], pointArr[pIt->p3]);
	}
	
	if(model->SetVisibilityGroup("elevator", true)){
		
		drawToElevatorMap = true;

		vector<sPolygon> polygonArr;
		vector<sPolygon>::iterator pIt;
		vector<Vect3f> pointArr;

		TriangleInfo all;
		model->GetTriangleInfo(all,TIF_TRIANGLES|TIF_POSITIONS);
		polygonArr.swap(all.triangles);
		pointArr.swap(all.positions);

		MatXf pose = model->GetPosition();
		pose.rot().scale(model->GetScale());

		for(int i = 0; i<pointArr.size(); i++)
			pose.xformPoint(pointArr[i]);
		
		FOR_EACH(polygonArr, pIt)
			drawTriangle(pointArr[pIt->p1], pointArr[pIt->p2], pointArr[pIt->p3]);
	}

	model->SetVisibilityGroup(name.c_str());
}

//=======================================================
inline void swap(Vect3f** v1, Vect3f** v2)
{
	Vect3f* t = *v2;
	*v2 = *v1;
	*v1 = t;
}

//=======================================================
void SecondMap::drawTriangle(const Vect3f& _v1, const Vect3f& _v2, const Vect3f& _v3 )
{
	// Переводим в коорд. карты.
	Vect3f v1(w2mFloor(_v1.x), w2mFloor(_v1.y), _v1.z);
	Vect3f v2(w2mFloor(_v2.x), w2mFloor(_v2.y), _v2.z);
	Vect3f v3(w2mFloor(_v3.x), w2mFloor(_v3.y), _v3.z);
	
	Vect3f* sv1 = &v1; // Для сортировки по Y.
	Vect3f* sv2 = &v2;
	Vect3f* sv3 = &v3;

	// Сортируем.

	if(sv3->y < sv2->y) swap(&sv2, &sv3);
  	if(sv2->y < sv1->y) swap(&sv1, &sv2);
	if(sv3->y < sv2->y) swap(&sv2, &sv3);

	// Рисуем.

	int ys = sv1->y;
	int ye = sv2->y;

	int xs = sv1->x;
	int zs = sv1->z;

	float dx1 = (ye-ys)?(float)(sv2->x - sv1->x)/(ye-ys):0;
	float dx2 = (sv3->y - sv1->y)?(float)(sv3->x - sv1->x)/(sv3->y - sv1->y):0;

	float dz1 = (ye-ys)?(float)(sv2->z - sv1->z)/(ye-ys):0;
	float dz2 = (sv3->y - sv1->y)?(float)(sv3->z - sv1->z)/(sv3->y - sv1->y):0;

	int x1, x2;
	int z1, z2;
	
	bool revert = false;
	if(sv3->x < sv2->x) revert = true;

	for(int y = sv1->y; y <= sv3->y; y++) {
		if(y == ye) {
			ys = sv2->y;
			ye = sv3->y;
			xs = sv2->x;
			zs = sv2->z;
			dx1 = (ye-ys)?(float)(sv3->x - sv2->x)/(ye-ys):0;
			dz1 = (ye-ys)?(float)(sv3->z - sv2->z)/(ye-ys):0;
		}
		
		x1 = xs + dx1 * (y - ys);
		x2 = sv1->x + dx2 * (y - sv1->y);
		
		z1 = zs + dz1 * (y - ys);
		z2 = sv1->z + dz2 * (y - sv1->y);
		
		if(revert)
			drawScanLine(x2, x1, y, z2, z1);
		else
			drawScanLine(x1, x2, y, z1, z2);
	}
}

//=======================================================
void SecondMap::drawScanLine( int xl, int xr, int y, int zl, int zr )
{
	float dz = (xr - xl)?(float)(zr - zl) / (xr - xl):0;
	for(int x = xl; x <= xr; x++) {
		drawPoint(x, y, zl + dz*(x-xl));
/*		if(drawToElevatorMap) {
			drawPoint(x+1, y, zl + dz*(x-xl));
			drawPoint(x-1, y, zl + dz*(x-xl));
			drawPoint(x+1, y-1, zl + dz*(x-xl));
			drawPoint(x-1, y+1, zl + dz*(x-xl));
			drawPoint(x, y+1, zl + dz*(x-xl));
			drawPoint(x, y-1, zl + dz*(x-xl));
			drawPoint(x+1, y+1, zl + dz*(x-xl));
			drawPoint(x-1, y-1, zl + dz*(x-xl));
		}*/
	}
}

//=======================================================
__forceinline void SecondMap::drawPoint( int x, int y, int z )
{
	mapPresent_ = true;

	if(drawToElevatorMap) {
		if(x >= 0 && x < sizeX() && y >= 0 && y < sizeY()) {
			(*this)(x,y).setElevator(true);		
			pathFinder->updateTile(x,y);
		}
	} else
		if(x >= 0 && x < sizeX() && y >= 0 && y < sizeY()) {
			(*this)(x,y).setHCheck(z);		
			pathFinder->updateTile(x,y);
		}
}




