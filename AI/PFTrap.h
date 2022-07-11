#ifndef __PF_TRAP_H_INCLUDED__
#define __PF_TRAP_H_INCLUDED__

#include "AiAStar.h"
#include "Timers.h"
#include "XTL\Rect.h"

using namespace std;

class UnitBase;
typedef unsigned short PFUnitMapTile;

class PathFinder {
public:
	
	enum { 	
		ENVIRONMENT_FLAG = 0x00003FFF,
		GROUND_FLAG = 0x00004000,
		WATER_FLAG = 0x00008000,
		IMPASSABILITY_FLAG = 0x000F0000,
		FIELD_FLAG = 0xFFF00000,
	};

	enum { world_shift =4 };

	typedef unsigned int PFTile;

	PathFinder(int _sizeX, int _sizeY);
	~PathFinder();

	PFTile* getMap() { return map; }
	
	bool checkImpassability(int x_world, int y_world, int impassabability, int passability = ENVIRONMENT_FLAG | GROUND_FLAG | WATER_FLAG | FIELD_FLAG) { 
		x_world = x_world >> world_shift;
		y_world = y_world >> world_shift;
		if(clusterStorage.empty() || x_world < 0 || x_world >= sizeX || y_world < 0 || y_world >= sizeY)
			return true;
		PFCluster* cluster = getClusterMap(x_world, y_world);
		return cluster ? cluster->checkImpassability(impassabability, passability) : true;
	}

	void updateTile(int x, int y);
	void updateRect(int x1, int y1, int dx, int dy);

	//Callback для воды
	friend void waterChangePF(int x, int y);
	friend void iceChangePF(int x, int y);

	void showDebugInfo();

	void recalcPathFinder();
	void clusterization();

	bool findPath(const Vect2f& _startPoint, const Vect2f& _endPoint, PFTile flags, int passibility, const float* possibilityFactors, vector<Vect2f>& points);
	bool findNearestPoint(const Vect2i& _startPoint, int scanRadius, PFTile flags, Vect2i& retPoint);
	
	bool impassabilityCheckForUnit(int xc, int yc, int r, int passibility);
	bool checkField(int xc, int yc, int r, int fieldFlag);
	bool checkWater(int xc, int yc, int r);
	
	void enableAutoImpassability(bool enable = true) { enableAutoImpassability_ = enable; }

	void projectUnit( UnitBase* unit );
	void drawCircle( int px, int py, int radius, PFTile data);
	PFUnitMapTile getUnitFlag( UnitBase* unit );
	int getTerrainType(int x_world, int y_world)
	{
		x_world = x_world >> world_shift;
		y_world = y_world >> world_shift;
		if(clusterStorage.empty() || x_world < 0 || x_world >= sizeX || y_world < 0 || y_world >= sizeY)
			return true;
		PFCluster* cluster = getClusterMap(x_world, y_world);
		return cluster ? cluster->getTerrainType() : 0;
	}
	int getTerrainType(int xc, int yc, int r);

private:
	
	int getFlag(int xc, int yc, int r);
	
	bool enableAutoImpassability_;
	
	typedef unsigned short PFClusterIndex;

	class PFCluster {
	public:
		
		PFCluster() {}
		PFCluster(const PFCluster& other) {
			x1 = other.x1;
			y1 = other.y1;
			x2 = other.x2;
			y2 = other.y2;
			val = other.val;
			pathFinder = other.pathFinder;
			links.clear();
			lines.clear();
		}
		short x1, y1, x2, y2;
		Vect2f p;
		PFTile val;
		void* AIAStarPointer;
		PathFinder* pathFinder;
		vector<PFCluster*> links;
		vector<Vect2f> lines;
		
		typedef vector<PFCluster*>::iterator iterator;
		
		inline iterator begin() {
			if(links.empty()) pathFinder->buildClusterLinks(this);
			return links.begin();
		}
		
		inline iterator end() { return links.end(); }

		int getTerrainType() { return (val & IMPASSABILITY_FLAG) >> 16; }

		bool checkImpassability(int impassability, int passability) { return ((impassability & (1 << getTerrainType())) || (val & (~(passability | IMPASSABILITY_FLAG)))); }
	};
	const float* passabilityFactors_;
	int sizeX;
	int sizeY;
	PFTile * map;
	PFClusterIndex * clusterMap;
	vector<PFCluster> clusterStorage;
	vector<PFCluster*> path;
	Vect2f startPoint, endPoint;
	LogicTimer recalcClustersTimer_;
	bool findNearestFlag_;

	int impassability;
	PFTile curFlags;

	struct PFEdge { Vect2f v1, v2; };
	vector<PFEdge> edges;
	vector<Vect2f> wayPoints;

	PFCluster* getClusterMap( int x, int y);
	void buildClusterLinks(PFCluster* cluster);
	void buildXLineLinks(int x1, int x2, int y, int yinc, PFCluster* cluster);
	__forceinline void fillRegionMap(int x1, int y1, int x2, int y2, PFTile val);
	void getClusterLine(PFCluster* c1, PFCluster* c2, Vect2f& p1, Vect2f& p2);
	void callcPoints(vector<Vect2f>& points);
	void callcWPoint(int i0, int i1, Vect2f& p0, Vect2f& p1);
	bool isWater(int x, int y);

	class PFClusterHeuristic {
	public:

		PFCluster* startCluster;
		PFCluster* endCluster;

		Vect2f startPoint;
		Vect2f endPoint;

		inline float GetH(PFCluster* pos1, PFCluster* pos2);
		inline float GetG(PFCluster* pos1, PFCluster* pos2);
		inline float GetDistance(PFCluster* pos1, PFCluster* pos2);
		inline bool IsEndPoint(PFCluster* pos);

	} clusterHeuristic;

	AIAStarGraph<PFClusterHeuristic, PFCluster> astar;

	__forceinline void cell(int x, int y, PFTile data);
	void drawPoly(int pX[], int pY[], int n, PFUnitMapTile data);
};

extern list<Rectf> tileMapUpdateRegions;

extern PathFinder* pathFinder;
void waterTileChange(int x, int y);
void iceTileChange(int x, int y);


#endif
