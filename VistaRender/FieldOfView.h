#ifndef __FIELD_OF_VISION_H__
#define __FIELD_OF_VISION_H__

#include "Render\inc\IUnkObj.h"
#include "XTL\Map2D.h"

class FieldOfViewMap : public cIUnkObj
{
	typedef vector<Color4c> Colors;

public:
	FieldOfViewMap(int worldXSize, int worldYSize);

	void setColors(const Colors& colors);

	void add(c3dx* model);
	void remove(c3dx* model);

	void add(const Vect3f& posWorld, float psi, float radius, float sector, int colorIndex);

	void updateTexture();

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);

	void serialize(Archive& ar);

	void debugDraw(Camera* camera);

private:
	struct Cell
	{
		short height;
		unsigned char visibility;
		unsigned char colorIndex;
		Cell() : height(0), visibility(0), colorIndex(0) {}
		void quant();
	};
	typedef Map2D<Cell, 3> Map;
	Map map_;
	UnknownHandle<cTexture> texture_;
	Colors colors_;
	float radiusFactor_;
	float sectorFactor_;

	enum {
		MAP2GRID_SHIFT = Map::tileSizeShl - 2, // kmGrid
		PRECISION = 16,
		_1_ = 1 << PRECISION,
		HALF = 1 << (PRECISION - 1),
	};

	struct AddOp
	{
		AddOp(Map& map) : map_(map) {}
		void operator()(int x, int y, int c);
		Map& map_;
	};

	struct RemoveOp
	{
		RemoveOp(Map& map) : map_(map) {}
		void operator()(int x, int y, int c);
		Map& map_;
	};

	void trace(const Vect2i& pos, const Vect2i& delta, int zmin, int zmax, int colorIndex, int visibility);
};

#endif //__FIELD_OF_VISION_H__