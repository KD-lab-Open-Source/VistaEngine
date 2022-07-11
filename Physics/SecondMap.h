#ifndef __SECOND_MAP_H_INCLUDED__
#define __SECOND_MAP_H_INCLUDED__

#include "Map2D.h"
#include "..\render\3dx\Node3DX.h"

//=======================================================
class SecondMapTile {

	unsigned char h; // Высота
	bool elevator;

public:

	enum {
		h_shift = 4, // Горизонтальный масштаб.(смещение)
		v_shift = 1  // Вертикальный масштаб.(смещение)
	};

	int getH() { return h << v_shift; }
	bool getElevator() { return elevator; }
	void setH(int _h) {	if(_h < 0 || _h >= 512) return; h = _h >> v_shift; }
	void setHCheck(int _h) { if( (_h >> v_shift) <= h || _h < 0 || _h >= 512 ) return; h = _h >> v_shift; }
	void setElevator(bool val) { elevator = val; }

	SecondMapTile() { h = 0; elevator = false;  }

	operator const int() const { return h << v_shift; }

};

//=======================================================
class SecondMap : public Map2D<SecondMapTile, SecondMapTile::h_shift> {

	void drawTriangle(const Vect3f& _v1, const Vect3f& _v2, const Vect3f& _v3 );
	void drawScanLine(int xl, int xr, int y, int zl, int zr);
	inline void drawPoint(int x, int y, int z);
	void clearRegion(int x, int y, int r);
	
	bool mapPresent_;

public:

	SecondMap(int hsize,int vsize);
	~SecondMap();

	void projectModel(cObject3dx* model);
	void removeModel(cObject3dx* model);

	bool mapPresent() {return mapPresent_; }

};

#endif
