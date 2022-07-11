#include "StdAfx.h"
#include "NormalMap.h"
#include "DebugUtil.h"

NormalMap * normalMap;

void NormalMap::updateRect( int x1, int y1, int dx, int dy )
{
	float x2 = float(x1 + dx);
	float y2 = float(y1 + dy);

	float xm1 = (float) x1 / (1 << NormalMapTile::tileShift);
	float ym1 = (float) y1 / (1 << NormalMapTile::tileShift);
	float xm2 = (float) x2 / (1 << NormalMapTile::tileShift);
	float ym2 = (float) y2 / (1 << NormalMapTile::tileShift);

	int xil = round(xm1 - 0.5f);
	int xir = round(xm2 + 0.5f);
	int yil = round(ym1 - 0.5f);
	int yir = round(ym2 + 0.5f);

	xil = clamp(xil, 0, sizeX());
	xir = clamp(xir, 0, sizeX());
	yil = clamp(yil, 0, sizeY());
	yir = clamp(yir, 0, sizeY());

	updateRectMapCoord(xil, yil, xir, yir);
}

void NormalMap::updateRectMapCoord( int x1, int y1, int x2, int y2 )
{
	for(int i = x1; i < x2; i++)
		for(int j = y1; j < y2; j++)
			callcTileNormal(i,j);
}

void NormalMap::showDebugInfo()
{
	for(int i = 0; i < sizeX(); i++)
		for(int j = 0; j < sizeY(); j++) {
			Vect3f point(float(i << NormalMapTile::tileShift), float(j << NormalMapTile::tileShift), height(i,j));
			show_line(point, point + normal(i,j)*10.f, Color4c::RED);
		}
}

void NormalMap::callcTileNormal( int x, int y )
{
	if(x < 1 || x >= sizeX()-1 || y< 1 || y >= sizeY()-1)
		return;

	int xw = x << NormalMapTile::tileShift;
	int yw = y << NormalMapTile::tileShift;
	if(xw >= vMap.H_SIZE) xw = vMap.H_SIZE - 1;
	if(yw >= vMap.V_SIZE) yw = vMap.V_SIZE - 1;
	if(environment->temperature() && (ice(x,y) || ice(x+1,y) || ice(x,y+1) || ice(x-1,y) || ice(x,y-1))) {
		normal(x,y) = Vect3f::K;
		return;
	}

	float mapStep = float(1 << NormalMapTile::tileShift);

	Vect3f tmp1(mapStep, 0.0f, height(x+1,y) - height(x,y));
	Vect3f tmp2(0.0f, mapStep, height(x,y+1) - height(x,y));
	Vect3f tmp3(-mapStep, 0.0f, height(x-1,y) - height(x,y));
	Vect3f tmp4(0.0f, -mapStep, height(x,y-1) - height(x,y));

	Vect3f tmp5(Vect3f::ZERO);
	tmp5.crossAdd(tmp1, tmp2);
	tmp5.crossAdd(tmp2, tmp3);
	tmp5.crossAdd(tmp3, tmp4);
	tmp5.crossAdd(tmp4, tmp1);

	normal(x,y) = tmp5.normalize();
}

Vect3f NormalMap::normalLinear( float x, float y )
{
	if(x < 0 || x >= vMap.H_SIZE || y < 0 || y >= vMap.V_SIZE)
		return Vect3f::K;

	x = (float) x / (1 << NormalMapTile::tileShift);
	y = (float) y / (1 << NormalMapTile::tileShift);

	int xil = round(x - 0.5f);
	int xir = xil + 1;
	int yil = round(y - 0.5f);
	int yir = yil + 1;
	Vect3f lineYL = normal(xil, yil)*(xir-x) + normal(xir, yil)*(x-xil);
	Vect3f lineYR = normal(xil, yir)*(xir-x) + normal(xir, yir)*(x-xil);

	return (lineYL*(yir-y) + lineYR*(y-yil)).normalize();
}

float NormalMap::heightLinear( float x, float y )
{
	x = (float) x / (1 << NormalMapTile::tileShift);
	y = (float) y / (1 << NormalMapTile::tileShift);

	int xil = round(x - 0.5f);
	int xir = xil + 1;
	int yil = round(y - 0.5f);
	int yir = yil + 1;
	float lineYL = height(xil, yil)*(xir-x) + height(xir, yil)*(x-xil);
	float lineYR = height(xil, yir)*(xir-x) + height(xir, yir)*(x-xil);
	return lineYL*(yir-y) + lineYR*(y-yil);
}