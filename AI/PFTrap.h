#ifndef __PF_TRAP_H_INCLUDED__
#define __PF_TRAP_H_INCLUDED__

#include "AiAStar.h"
#include "Timers.h"
#include "Rect.h"

using namespace std;

class UnitBase;
typedef unsigned short PFUnitMapTile;

class PathFinder {
public:
	
	enum { 	
		GROUND_FLAG = 0x010000,
		WATER_FLAG = 0x020000,
		IMPASSABILITY_FLAG = 0x040000,
		ELEVATOR_FLAG = 0x080000,
		SECOND_MAP_FLAG = 0x100000,
	};

	enum { world_shift =4 };

	typedef unsigned int PFTile;

	PathFinder(int _sizeX, int _sizeY);
	~PathFinder();

	PFTile* getMap() { return map; }
	PFTile* getSecondMap() { return secondMap; }

	bool checkFlag(int flag, int x_world, int y_world) { 
		x_world = x_world >> world_shift;
		y_world = y_world >> world_shift;
		if(clusterStorage.empty() || x_world < 0 || x_world >= sizeX || y_world < 0 || y_world >= sizeY)
			return flag & WATER_FLAG;
		PFCluster* cluster = getClusterMap(x_world, y_world);
		return cluster ? flag & cluster->val : flag & WATER_FLAG;
	}

	void updateTile(int x, int y);
	void updateRect(int x1, int y1, int dx, int dy);

	//Callback для воды
	friend void waterChangePF(int x, int y);
	friend void iceChangePF(int x, int y);

	void showDebugInfo();

	void recalcPathFinder();
	void clusterization();

	bool findPath(const Vect2i& _startPoint, const Vect2i& _endPoint, PFTile flags, vector<Vect2i>& points, bool seconMapP1, bool secondMapP2);
	bool findNearestPoint(const Vect2i& _startPoint, int scanRadius, PFTile flags, Vect2i& retPoint);
	
	bool impassabilityCheck( int xc, int yc, int r );
	bool checkWater( int xc, int yc, int r );
	float checkWaterFactor( int xc, int yc, int r );

	void enableAutoImpassability(bool enable = true) { enableAutoImpassability_ = enable; }

	void projectUnit( UnitBase* unit );
	PFUnitMapTile getUnitFlag( UnitBase* unit );

private:
	
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
			isSecondMap = other.isSecondMap;
			pathFinder = other.pathFinder;
			links.clear();
			lines.clear();
		}
		short x1, y1, x2, y2;
		Vect2f p;
		PFTile val;
		bool isSecondMap;
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
	};
	PFTile curFlags;
	int sizeX;
	int sizeY;
	PFTile * map;
	PFTile * secondMap;
	PFClusterIndex * clusterMap;
	PFClusterIndex * clusterSecondMap;
	vector<PFCluster> clusterStorage;
	vector<PFCluster*> path;
	Vect2f startPoint, endPoint;
	DurationTimer recalcClustersTimer_;
	bool findNearestFlag_;
	
	struct PFEdge { Vect2f v1, v2; };
	vector<PFEdge> edges;
	vector<Vect2f> wayPoints;

	PFCluster* getClusterMap( int x, int y);
	PFCluster* getClusterSecondMap( int x, int y);
	void clusterizeSecondMap();
	void buildClusterLinks(PFCluster* cluster);
	void buildXLineLinks(int x1, int x2, int y, int yinc, PFCluster* cluster);
	__forceinline void fillRegionMap(int x1, int y1, int x2, int y2, PFTile val);
	__forceinline void fillRegionSecondMap(int x1, int y1, int x2, int y2, PFTile val);
	void getClusterLine(PFCluster* c1, PFCluster* c2, Vect2f& p1, Vect2f& p2);
	void callcPoints(vector<Vect2i>& points);
	void callcWPoint(int i0, int i1, Vect2f& p0, Vect2f& p1);
	bool isWater(int x, int y);

	class PFClusterHeuristic {
	public:

		PFCluster* startCluster;
		PFCluster* endCluster;

		Vect2f startPoint;
		Vect2f endPoint;

		inline float GetH(PFCluster* pos1, PFCluster* pos2 );
		inline float GetG(PFCluster* pos1, PFCluster* pos2);
		inline bool IsEndPoint(PFCluster* pos);

	} clusterHeuristic;

	AIAStarGraph<PFClusterHeuristic, PFCluster> astar;

	__forceinline void cell(int x, int y, PFUnitMapTile data);
	void drawPoly(int pX[], int pY[], int n, PFUnitMapTile data);
	void drawCircle( int px, int py, int radius, PFUnitMapTile data);
};

extern list<Rectf> tileMapUpdateRegions;

extern PathFinder* pathFinder;
void waterTileChange(int x, int y);
void iceTileChange(int x, int y);


#endif
