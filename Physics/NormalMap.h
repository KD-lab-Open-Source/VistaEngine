#ifndef __NORMAL_MAP_H_INCLUDED__
#define __NORMAL_MAP_H_INCLUDED__

#include "Terra.h"
#include "..\Water\ice.h"
#include "..\Environment\Environment.h"

class NormalMapTile {
public:
	enum { tileShift = 4 }; // Горизонтальный масштаб.(смещение)
	Vect3f& normal() { return normal_; }
	NormalMapTile():normal_(Vect3f::K) { }
private:
	Vect3f normal_;
};

class NormalMap {
public:

	NormalMap(int hsize, int vsize) {
		sizeX_ = (hsize >> NormalMapTile::tileShift) + 1;
		sizeY_ = (vsize >> NormalMapTile::tileShift) + 1;
		data = 	new NormalMapTile[sizeX_ * sizeY_];
	}

	~NormalMap() {
		delete [] data;
	}

	float heightLinear(float x, float y);
	Vect3f normalLinear(float x, float y);
	
	// Параметры - в координитах карты.
	float height(int x, int y) {
		x <<= NormalMapTile::tileShift;
		y <<= NormalMapTile::tileShift;
		if(x >= vMap.H_SIZE) x = vMap.H_SIZE - 1;
		if(y >= vMap.V_SIZE) y = vMap.V_SIZE - 1;
		float iceH = 0;
		if(environment->temperature() && environment->temperature()->checkTileWorld(x,y))
			iceH = environment->water()->GetZFast(x,y);
		return max((float)vMap.GetApproxAlt(x, y),iceH); 
	}
	
	// Лед. надо соптимизить.
	bool ice(int x, int y) {
		x <<= NormalMapTile::tileShift;
		y <<= NormalMapTile::tileShift;
		if(x >= vMap.H_SIZE) x = vMap.H_SIZE - 1;
		if(y >= vMap.V_SIZE) y = vMap.V_SIZE - 1;
		return environment->temperature() && environment->temperature()->checkTileWorld(x,y);
	}
	
	// Параметры - в координитах карты.
	Vect3f& normal(int x, int y) { return data[y * sizeX_ + x].normal(); };

	int sizeX() const {return sizeX_;};
	int sizeY() const {return sizeY_;};

	void updateRect(int x1, int y1, int dx, int dy);

	void showDebugInfo();

private:
	
	int sizeX_;
	int sizeY_;
	NormalMapTile * data;

	void updateRectMapCoord(int x1, int y1, int x2, int y2);
	void callcTileNormal(int x, int y);

};

extern NormalMap * normalMap;

#endif
