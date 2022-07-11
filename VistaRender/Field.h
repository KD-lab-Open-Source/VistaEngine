#pragma once

#include "XTL\Handle.h"
#include "XTL\Map2D.h"
#include "XTL\UniqueVector.h"
#include "XTL\StaticMap.h"
#include "Render\Inc\IRenderDevice.h"
#include "Render\Src\UnkObj.h"

struct sVertexXYZDT2;

class TileStrip
{
public:
	TileStrip(int xSize, int ySize);
	~TileStrip();

	sVertexXYZDT2* beginDraw();
	void endDraw();

private:
	int xsize,ysize;
	sPtrIndexBuffer ib;
	sPtrVertexBuffer vb;
	int pagesize;//in vertex
	int pagenumber;
	int curpage;

	int numIndices(){return 3*2*xsize*ysize; }
	void setIB(sPolygon* pIndex);
};

const int FIELD_SHIFT = 4;
const int FIELD_TILE_SHIFT = 6;
const int FIELD_2_TILE_SHIFT = FIELD_TILE_SHIFT - FIELD_SHIFT;

struct FieldPrm
{
	float smoothFactor;
	int evolveIterations;
	float timeStep;
	float stiffness;
	float damping;
	float damping2;
	float dampingMin;
	float spawnFactor;
	float deltaPhase;

	FieldPrm();
	void serialize(Archive& ar);
};

struct FieldCell
{
	enum Type{
		EXTERNAL,
		BORDER,
		INTERNAL
	};
	Type type;
	float height;
	Vect2f delta;
	Vect3f normal;
	
	FieldCell() : type(EXTERNAL), height(Z0), delta(Vect2i::ZERO), normal(Vect3f::ZERO) {}

	enum { Z0 = -50 };
};

class FieldMould : public ShareHandleBase
{
public:
	FieldMould(int radius, float height);
	~FieldMould();

	int radius() const { return radius_; }
	const FieldCell* buffer() const { return buffer_; }

private:
	int radius_;
	FieldCell* buffer_;
};

class FieldSource
{
public:
	FieldSource(int radius, float height, const Vect2i& position, Color4c color);

private:
	typedef ShareHandle<FieldMould> FieldMouldHandle;
	FieldMouldHandle fieldMould_;
	Vect2i position_;
	Color4c color_;

	typedef StaticMap<int, FieldMouldHandle> FieldMoulds;
	static FieldMoulds fieldMoulds_;

	friend class FieldDispatcher;
};

class FieldDispatcher : public cUnkObj
{
public:
	FieldDispatcher(int worldXSize, int worldYSize);

	void clear();
	void add(FieldSource& source);
	void remove(FieldSource& source);

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);

	int sortIndex() const { return 0; }

	//const Map& map() const { return map_; }

	void debugDraw(Camera* camera);

private:
	struct Cell : FieldCell
	{
		float heightInitial;
		float velocity;
		Color4c color;
		Cell() : color(Color4c::RED), heightInitial(Z0), velocity(0) {}
		void add(const FieldCell& cell, Color4c colorIn);
		void integrate();
	};
	typedef Map2D<Cell, FIELD_SHIFT> Map;
	Map map_;
	
	typedef Map2D<short, FIELD_TILE_SHIFT> TileMap;
	TileMap tileMap_;

	TileStrip tileStrip_;

	typedef UniqueVector<FieldSource*> FieldSources;
	FieldSources fieldSources_;
	float phase_;

	void add(const Vect2i& pos, const FieldMould& source, Color4c color);
	void refresh();
	void evolveField();
};
