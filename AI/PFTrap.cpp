#include "stdAfx.h"
#include "PFTrap.h"

#include "universe.h"
#include "secondMap.h"
#include "..\Environment\Environment.h"
#include "..\Terra\terra.h"
#include "normalMap.h"
#include "BaseUnit.h"
#include "UnitEnvironment.h"
#include "IronBuilding.h"

//=======================================================

PathFinder* pathFinder;
list<Rectf> tileMapUpdateRegions;

//=======================================================
PathFinder::PathFinder(int _sizeX, int _sizeY)
{
	sizeX = _sizeX >> world_shift;
	sizeY = _sizeY >> world_shift;

	map = new PFTile[sizeX*sizeY];
	secondMap = new PFTile[sizeX*sizeY];
	memset(map, 0, sizeX*sizeY*sizeof(PFTile));
	memset(secondMap, 0, sizeX*sizeY*sizeof(PFTile));

	clusterMap = new PFClusterIndex[sizeX*sizeY];
	clusterSecondMap = new PFClusterIndex[sizeX*sizeY];

	recalcClustersTimer_.stop();
	enableAutoImpassability_ = false;
	findNearestFlag_ = false;
}

//=======================================================
PathFinder::~PathFinder()
{
	delete[] map;
	delete[] secondMap;
	delete[] clusterMap;
	delete[] clusterSecondMap;
}

//=======================================================
void PathFinder::clusterization()
{
	memset(clusterMap, 0, sizeX*sizeY*sizeof(PFClusterIndex));

	clusterStorage.clear();

	int lineStart = 0;
	PFTile tileNow;

	for(int i=0;i<sizeY;i++) {
		tileNow = map[i*sizeX];
		int j;
		for(j=0;j<sizeX;j++){
			if (map[i*sizeX+j] != tileNow) {
				clusterMap[i*sizeX+lineStart] = j - lineStart;
				lineStart = j;
				tileNow = map[i*sizeX+j];
			}

		}
		clusterMap[i*sizeX+lineStart] = j - lineStart;
		lineStart = 0;
	}

	int left;
	PFCluster c;

	for(int i=0;i<sizeY;i++)
		for(int j=0;j<sizeX;j++)
			if (clusterMap[i*sizeX+j]!= 0 && ((i== (sizeY-1)) || (clusterMap[i*sizeX+j]!=clusterMap[(i+1)*sizeX+j]) ||(map[i*sizeX+j] != map[(i+1)*sizeX+j]))) {
				left = i-1;			
				while ((left >= 0)&& ((clusterMap[i*sizeX+j] == clusterMap[left*sizeX+j]) && (map[i*sizeX+j] == map[left*sizeX+j]))) left--;
				left++;
				
				c.x1 = j;
				c.x2 = j + clusterMap[i*sizeX+j];
				c.y1 = left;
				c.y2 = i+1;
				c.pathFinder = this;
				c.val = map[c.y1*sizeX+c.x1];
				c.isSecondMap = false;
				clusterStorage.push_back(c);
			}

	clusterizeSecondMap();

	memset(clusterMap, 0, sizeX*sizeY*sizeof(PFClusterIndex));
	memset(clusterSecondMap, 0, sizeX*sizeY*sizeof(PFClusterIndex));

	for(int i=0;i<clusterStorage.size();i++)
		if(!clusterStorage[i].isSecondMap)
			fillRegionMap(clusterStorage[i].x1, clusterStorage[i].y1, clusterStorage[i].x2, clusterStorage[i].y2, i);
		else
			fillRegionSecondMap(clusterStorage[i].x1, clusterStorage[i].y1, clusterStorage[i].x2, clusterStorage[i].y2, i);

	astar.Init(clusterStorage);

	for(int i=0; i<sizeY*sizeX; i++)
		map[i] &= 0xFFFF0000;
}

//=======================================================
__forceinline PathFinder::PFCluster* PathFinder::getClusterMap( int x, int y )
{
	return &clusterStorage[clusterMap[y * sizeX + x]];
}

//=======================================================
__forceinline PathFinder::PFCluster* PathFinder::getClusterSecondMap(  int x, int y )
{
	if(clusterSecondMap[ y * sizeX + x]== 0) return NULL;
	return &clusterStorage[clusterSecondMap[y * sizeX + x]];
}

//=======================================================
bool PathFinder::findPath( const Vect2i& _startPoint, const Vect2i& _endPoint, PFTile flags, vector<Vect2i>& points, bool secondMapP1, bool secondMapP2 )
{
	curFlags = flags;
	findNearestFlag_ = false;

	startPoint = Vect2f((float)_startPoint.x / (1<<world_shift), (float)_startPoint.y / (1<<world_shift) + 0.1);
	endPoint = Vect2f((float)_endPoint.x / (1<<world_shift) + 0.1, (float)_endPoint.y / (1<<world_shift));

	if(startPoint.x < 0 || startPoint.x >= sizeX || startPoint.y < 0 || startPoint.y >= sizeY)
		return false;

	if(endPoint.x < 0 || endPoint.x > sizeX || endPoint.y < 0 || endPoint.y > sizeY)
		return false;

	clusterHeuristic.startPoint = startPoint;
	clusterHeuristic.endPoint = endPoint;

	Vect2i startPointI((int)startPoint.x, (int)startPoint.y);
	Vect2i endPointI((int)endPoint.x, (int)endPoint.y);

	if(secondMapP1) {
	    clusterHeuristic.startCluster = getClusterSecondMap(startPointI.x, startPointI.y);
		if(!clusterHeuristic.startCluster)
			clusterHeuristic.startCluster = getClusterMap(startPointI.x, startPointI.y);
	} else 
	    clusterHeuristic.startCluster = getClusterMap(startPointI.x, startPointI.y);

	if(secondMapP2) {
		clusterHeuristic.endCluster = getClusterSecondMap(endPointI.x, endPointI.y);
		if(!clusterHeuristic.endCluster)
			clusterHeuristic.endCluster = getClusterMap(endPointI.x, endPointI.y);
	} else
		clusterHeuristic.endCluster = getClusterMap(endPointI.x, endPointI.y);
	
	clusterHeuristic.startCluster->p = startPoint;
	path.clear();
	bool avalible = astar.FindPath(clusterHeuristic.startCluster, &clusterHeuristic, path);
	
	flags |= 0x0000FFFF;

	if(avalible && (clusterHeuristic.endCluster->val & (~flags)))
		avalible = false;

	if(path.empty())
		return true;

	callcPoints(points);

	if(avalible) {
		points.push_back(_endPoint);
		return true;
	}
	else {
		PFCluster* endCluster = path[path.size()-1];
		Rectf rect(Vect2f(endCluster->x1, endCluster->y1), Vect2f(endCluster->x2 - endCluster->x1, endCluster->y2 - endCluster->y1));
        
		if(points.empty()) {
			rect.clipLine(endPoint, startPoint);
			points.push_back(Vect2i(startPoint.x * (1 << world_shift), startPoint.y * (1 << world_shift)));
			return false;
		}
		else {
			Vect2f clipPoint = Vect2f(points[points.size()-1]) / (1 << world_shift);
			rect.clipLine(endPoint, clipPoint);
			points.push_back(Vect2i(clipPoint.x * (1 << world_shift), clipPoint.y * (1 << world_shift)));
			return false;
		}
	}
	
	return true;
}

//=======================================================
__forceinline bool isIntersect(int x1, int x2, int y1, int y2) 
{
	if(x1<y1 && x2>y1) return true;
	if(x1<y2 && x2>y2) return true;
	if(x1>y1 && x1<y2) return true;
	return false;
}

//=======================================================
void PathFinder::buildXLineLinks(int x1, int x2, int y, int yinc, PFCluster* cluster)
{
	PFCluster* c;
	PFClusterIndex prev = 0xFFFF;

	int l1, l2;

	int cx = x1;
	if(c = getClusterMap(x1, y)) {
		cluster->links.push_back(c);
		l1 = max(c->x1, cluster->x1);
		l2 = min(c->x2, cluster->x2);
		cluster->lines.push_back(Vect2f(l1, y + yinc));
		cluster->lines.push_back(Vect2f(l2, y + yinc));
		cx = c->x2;
	}

	while(cx < x2)
		if(c = getClusterMap(cx, y)) {
			cluster->links.push_back(c);
			l1 = max(c->x1, cluster->x1);
			l2 = min(c->x2, cluster->x2);
			cluster->lines.push_back(Vect2f(l1, y + yinc));
			cluster->lines.push_back(Vect2f(l2, y + yinc));
			cx = c->x2;
		}
}

//=======================================================
void PathFinder::buildClusterLinks(PFCluster* cluster)
{
	if(cluster->isSecondMap) {
		PFCluster* c;
		PFClusterIndex prev = 0xFFFF;
		int l1, l2;

		int y = cluster->y1-1;
		if(cluster->y1)
			for(int i=cluster->x1; i<cluster->x2;i++)
				if(prev != clusterSecondMap[y*sizeX+i]) {
					prev = clusterSecondMap[y*sizeX+i];
					if(c = getClusterSecondMap(i, y)) {
						cluster->links.push_back(c);
						l1 = max(c->x1, cluster->x1);
						l2 = min(c->x2, cluster->x2);
						cluster->lines.push_back(Vect2f(l1, y+1));
						cluster->lines.push_back(Vect2f(l2, y+1));
					}
				}

		prev = 0xFFFF;
		y = cluster->y2;
		if(y < sizeY)
			for(int i=cluster->x1; i<cluster->x2;i++)
				if(prev != clusterSecondMap[y*sizeX+i]) {
					prev = clusterSecondMap[y*sizeX+i];
					if(c = getClusterSecondMap(i, y)) {
						cluster->links.push_back(c);
						l1 = max(c->x1, cluster->x1);
						l2 = min(c->x2, cluster->x2);
						cluster->lines.push_back(Vect2f(l1, y));
						cluster->lines.push_back(Vect2f(l2, y));
					}
				}

		prev = 0xFFFF;
		int x = cluster->x1-1;
		if(cluster->x1)
			for(int i=cluster->y1; i<cluster->y2;i++)
				if(prev != clusterSecondMap[i*sizeX+x]) {
					prev = clusterSecondMap[i*sizeX+x];
					if(c = getClusterSecondMap(x, i)) {
						cluster->links.push_back(c);
						l1 = max(c->y1, cluster->y1);
						l2 = min(c->y2, cluster->y2);
						cluster->lines.push_back(Vect2f(x+1, l1));
						cluster->lines.push_back(Vect2f(x+1, l2));
					}
				}

		prev = 0xFFFF;
		x = cluster->x2;
		if(x < sizeX)
			for(int i=cluster->y1; i<cluster->y2;i++)
				if(prev != clusterSecondMap[i*sizeX+x]) {
					prev = clusterSecondMap[i*sizeX+x];
					if(c = getClusterSecondMap(x, i)) {
						cluster->links.push_back(c);
						l1 = max(c->y1, cluster->y1);
						l2 = min(c->y2, cluster->y2);
						cluster->lines.push_back(Vect2f(x, l1));
						cluster->lines.push_back(Vect2f(x, l2));
					}
				}

		PFCluster* elevator = getClusterMap(cluster->x1, cluster->y1);
		if(elevator && elevator->val == ELEVATOR_FLAG &&
		   elevator->x1 == cluster->x1 &&
		   elevator->x2 == cluster->x2 &&
		   elevator->y1 == cluster->y1 &&
		   elevator->y2 == cluster->y2) {
			    cluster->links.push_back(elevator);
				cluster->lines.push_back(Vect2f(cluster->x1, cluster->y1));
				cluster->lines.push_back(Vect2f(cluster->x2, cluster->y2));
		   }
	} else {

		PFCluster* c;
		PFClusterIndex prev = 0xFFFF;
		int l1, l2;

		if(cluster->y1)
			buildXLineLinks(cluster->x1, cluster->x2, cluster->y1-1, 1, cluster);

		if(cluster->y2 < sizeY)
			buildXLineLinks(cluster->x1, cluster->x2, cluster->y2, 0, cluster);

		prev = 0xFFFF;
		int x = cluster->x1-1;
		if(cluster->x1)
			for(int i=cluster->y1; i<cluster->y2;i++)
				if(prev != clusterMap[i*sizeX+x]) {
					prev = clusterMap[i*sizeX+x];
					c = getClusterMap(x, i);
					cluster->links.push_back(c);
					l1 = max(c->y1, cluster->y1);
					l2 = min(c->y2, cluster->y2);
					cluster->lines.push_back(Vect2f(x+1, l1));
					cluster->lines.push_back(Vect2f(x+1, l2));
				}

		prev = 0xFFFF;
		x = cluster->x2;
		if(x < sizeX)
			for(int i=cluster->y1; i<cluster->y2;i++)
				if(prev != clusterMap[i*sizeX+x]) {
					prev = clusterMap[i*sizeX+x];
					c = getClusterMap(x, i);
					cluster->links.push_back(c);
					l1 = max(c->y1, cluster->y1);
					l2 = min(c->y2, cluster->y2);
					cluster->lines.push_back(Vect2f(x, l1));
					cluster->lines.push_back(Vect2f(x, l2));
				}

		PFCluster* elevator = getClusterSecondMap(cluster->x1, cluster->y1);
		if(elevator && elevator->val == ELEVATOR_FLAG &&
		   elevator->x1 == cluster->x1 &&
		   elevator->x2 == cluster->x2 &&
		   elevator->y1 == cluster->y1 &&
		   elevator->y2 == cluster->y2) {
			    cluster->links.push_back(elevator);
				cluster->lines.push_back(Vect2f(cluster->x1, cluster->y1));
				cluster->lines.push_back(Vect2f(cluster->x2, cluster->y2));
		   }

	};

	xassert(cluster->lines.size() == cluster->links.size()*2);
}

//=======================================================
__forceinline void PathFinder::fillRegionMap( int x1, int y1, int x2, int y2, PFTile val)
{
	for(int i=y1; i<y2; i++)
		for(int j=x1; j<x2; j++)
			clusterMap[i*sizeX+j] = val;
}

//=======================================================
__forceinline void PathFinder::fillRegionSecondMap( int x1, int y1, int x2, int y2, PFTile val)
{
	for(int i=y1; i<y2; i++)
		for(int j=x1; j<x2; j++)
			clusterSecondMap[i*sizeX+j] = val;
}

//=======================================================
inline float distLinePoint(const Vect2f& l1, const Vect2f& l2, const Vect2f& p)
{
	if(l1.eq(l2))
		return l1.distance(p);

	return ((l2.x-l1.x)*(l1.y-p.y)-(l1.x-p.x)*(l2.y-l1.y))/sqrt((l2.x-l1.x)*(l2.x-l1.x)+(l2.y-l1.y)*(l2.y-l1.y));
}

//=======================================================
inline float PathFinder::PFClusterHeuristic::GetH(PFCluster* pos1, PFCluster* pos2 )
{
	if(!pos1) return 0;

	if(pos1->pathFinder->findNearestFlag_)
		return 1;

//	Vect2i p1, p2;
//	pos1->pathFinder->getClusterLine(pos1, pos2, p1, p2);
//	float d1 = distLinePoint(pos1->p, endPoint, p1);
//	float d2 = distLinePoint(pos1->p, endPoint, p2);

//	float dist;

//	if (d1*d2 < 0)
//		dist = 0;
//	else 
//		if (sqr(d1)<sqr(d2))
//			dist = fabs(d1);
//		else
//			dist = fabs(d2);
	
	// Немного евристики...
//	return (40*sqr(dist) + pos2->p.distance(endPoint))*(pos2->y2 - pos2->y1);
//	return sqr(dist)*(pos1->y2 - pos1->y1) + pos2->p.distance(endPoint)*(pos1->y2 - pos1->y1);
	return pos2->p.distance(endPoint);

}

//=======================================================
void PathFinder::getClusterLine( PFCluster* c1, PFCluster* c2, Vect2f& p1, Vect2f& p2 )
{
	for(int i=0;i<c1->links.size();i++)
		if(c1->links[i] == c2) {
			p1 = c1->lines[i*2];
			p2 = c1->lines[i*2+1];
			return;
		}
	assert(false);
}

//=======================================================
inline float PathFinder::PFClusterHeuristic::GetG( PFCluster* pos1, PFCluster* pos2 )
{
	if(pos1->pathFinder->findNearestFlag_){

		int i = 0;
		for( ; i<pos1->links.size(); i++)
			if(pos1->links[i] == pos2)
				break;

		pos2->p = Vect2f((pos1->lines[i*2].x + pos1->lines[i*2+1].x)/2.0f, (pos1->lines[i*2].y + pos1->lines[i*2+1].y)/2.0f);
		return pos1->p.distance2(pos2->p);
	}

	Vect2f p1f;
	Vect2f p2f;
	pos1->pathFinder->getClusterLine(pos1, pos2, p1f, p2f);

	//float d1 = distLinePoint(pos1->p, endPoint, p1f);
	//float d2 = distLinePoint(pos1->p, endPoint, p2f);

	// Оптимизированная версия.
	float sqrtLineDist = sqrt(sqr(endPoint.x - pos1->p.x) + sqr(endPoint.y-pos1->p.y));
	float d1 = ((endPoint.x - pos1->p.x) * (pos1->p.y - p1f.y) - (endPoint.y - pos1->p.y) * (pos1->p.x - p1f.x))/sqrtLineDist;
	float d2 = ((endPoint.x - pos1->p.x) * (pos1->p.y - p2f.y) - (endPoint.y - pos1->p.y) * (pos1->p.x - p2f.x))/sqrtLineDist;

	float x, y;

	if (d1*d2 < 0){
		float d1fabs = fabs(d1);
		float d2fabs = fabs(d2);
		x = p1f.x + ((p2f.x - p1f.x)/(d1fabs + d2fabs)) * d1fabs;
		y = p1f.y + ((p2f.y - p1f.y)/(d1fabs + d2fabs)) * d1fabs;
	} else 
		if (sqr(d1)<sqr(d2)) {
			x = p1f.x;
			y = p1f.y;
		} else {
			x = p2f.x;
			y = p2f.y;
		}

	pos2->p.set(x,y);

	float moveFactor = 1.0f;

	if((pos2->val & (~pos2->pathFinder->curFlags)) || (pos1->val & (~pos1->pathFinder->curFlags)))
		moveFactor = 1000.0f;

	if((!(startCluster->val & PathFinder::IMPASSABILITY_FLAG)) && ((pos2->val & PathFinder::IMPASSABILITY_FLAG) || (pos1->val & PathFinder::IMPASSABILITY_FLAG)))
		moveFactor = HEURISTIC_MAX;

//	if((pos2->pathFinder->curFlags & PathFinder::WATER_FLAG) && (pos2->pathFinder->curFlags & PathFinder::GROUND_FLAG) && (pos2->val & PathFinder::WATER_FLAG))
//		moveFactor = 2.0f;

	if(pos2 == endCluster)
		return (pos2->pathFinder->endPoint.distance(pos1->p))*moveFactor;
	
	return (pos2->p.distance(pos1->p))*moveFactor;
}

//=======================================================
inline bool PathFinder::PFClusterHeuristic::IsEndPoint( PFCluster* pos )
{
	if(pos->pathFinder->findNearestFlag_)
		return pos->val & pos->pathFinder->curFlags;

	return pos == endCluster;
}

//=======================================================
void PathFinder::callcPoints(vector<Vect2i>& points)
{
	edges.clear();
	PFEdge edge;

	for(int i=0;i<path.size()-1;i++) {
		getClusterLine(path[i], path[i+1], edge.v1, edge.v2);
		edges.push_back(edge);
	}

	points.clear();
	wayPoints.resize(edges.size());
	callcWPoint(0,edges.size(), startPoint, endPoint);

	for(int i=0;i<wayPoints.size();i++)
		if(!wayPoints[i].eq(Vect2f::ZERO))
			points.push_back(Vect2i(wayPoints[i].x *(1 << world_shift),wayPoints[i].y*(1 << world_shift)));

}

//=======================================================
void PathFinder::callcWPoint(int i0, int i1, Vect2f& p0, Vect2f& p1)
{
	float dist1, dist2;
	float max_dist = -1.0f;
	int maxi = -1;
	Vect2f max_point;

	for(int i=i0;i<i1;i++){
		dist1 = distLinePoint(p0,p1,edges[i].v1);
		dist2 = distLinePoint(p0,p1,edges[i].v2);
		if ((dist1*dist2)<0 || fabs(dist1) < 0.8f || fabs(dist2) < 0.8f) {
			Vect2f tv = edges[i].v2-edges[i].v1;
			tv = tv/(fabs(dist1)+fabs(dist2))*fabs(dist1);
			wayPoints[i] = Vect2f(0,0);//ve[i].v1 + tv;
		} else {
			if (sqr(dist1) < sqr(dist2)) {
				wayPoints[i] = edges[i].v1;
				if (fabs(dist1)>max_dist) {
					max_dist = fabs(dist1);
					maxi = i;
					max_point = edges[i].v1;
				}
			}
			else {
				wayPoints[i] = edges[i].v2;
				if (fabs(dist2)>max_dist) {
					max_dist = fabs(dist2);
					maxi = i;
					max_point = edges[i].v2;
				}
			}
		}
	}

	if (maxi >= i0)
		callcWPoint(i0,maxi,p0,max_point);

	if ((maxi < i1)&&(maxi != -1))
		callcWPoint(maxi+1,i1,max_point,p1);

}

//=======================================================
void PathFinder::clusterizeSecondMap()
{
	
	memset(clusterSecondMap, 0, sizeX*sizeY*sizeof(PFClusterIndex));

	int lineStart = 0;
	PFTile tileNow;

	for(int i=0;i<sizeY;i++) {
		tileNow = secondMap[i*sizeX];
		int j;
		for(j=0;j<sizeX;j++){
			if (secondMap[i*sizeX+j] != tileNow) {
				clusterSecondMap[i*sizeX+lineStart] = j - lineStart;
				lineStart = j;
				tileNow = secondMap[i*sizeX+j];
			}

		}
		clusterSecondMap[i*sizeX+lineStart] = j - lineStart;
		lineStart = 0;
	}

	int left;
	PFCluster c;

	for(int i=0;i<sizeY;i++)
		for(int j=0;j<sizeX;j++)
			if (secondMap[i*sizeX+j] && (clusterSecondMap[i*sizeX+j]!= 0 && ( (i== (sizeY-1)) || (clusterSecondMap[i*sizeX+j]!=clusterSecondMap[(i+1)*sizeX+j]) ||(secondMap[i*sizeX+j] != secondMap[(i+1)*sizeX+j])))) {
				left = i-1;			
				while ((left >= 0)&& ((clusterSecondMap[i*sizeX+j] == clusterSecondMap[left*sizeX+j]) && (secondMap[i*sizeX+j] == secondMap[left*sizeX+j]))) left--;
				left++;
				
				c.x1 = j;
				c.x2 = j + clusterSecondMap[i*sizeX+j];
				c.y1 = left;
				c.y2 = i+1;
				c.pathFinder = this;
				c.val = secondMap[c.y1*sizeX+c.x1];
				c.isSecondMap = true;
				clusterStorage.push_back(c);
			}
}

//=======================================================

__forceinline bool PathFinder::isWater( int x, int y)
{
	if(x < sizeX && y < sizeY && environment->temperature() && environment->temperature()->checkTile(x >>  environment->temperature()->gridShift() - world_shift, y >>  environment->temperature()->gridShift() - world_shift))
		return false;

	return environment->water()->GetRelativeZ(x,y) > environment->waterPathFindingHeight() &&
		environment->water()->GetRelativeZ(x+1,y) > environment->waterPathFindingHeight() &&
		environment->water()->GetRelativeZ(x+1,y+1) > environment->waterPathFindingHeight() &&
		environment->water()->GetRelativeZ(x,y+1) > environment->waterPathFindingHeight();
}

//=======================================================
void PathFinder::updateTile( int x, int y )
{
	if(x < 0 || x >= sizeX || y < 0 || y >= sizeY)
		return;

	int tile_size = 4;
	int h = 0;

	// Считаем непроходимость - если больше половины - то непроходимая.
	int num_of_impassability = 0;

	for(int yy = 0;yy < tile_size;yy++)
	{
		int offset = vMap.offsetGBuf(x*tile_size, y*tile_size + yy);
		unsigned char* h_buffer = vMap.GVBuf + offset;
		unsigned short* attr_buffer = vMap.GABuf + offset;
		for(int xx = 0;xx < tile_size;xx++)
		{
			h += h_buffer[xx];
			unsigned short attr = attr_buffer[xx];
			if(attr & GRIDAT_IMPASSABILITY)
				num_of_impassability++;
		}
	}

	h /= (tile_size*tile_size);
	PFTile val = 0;
	PFTile secondVal = 0;

	if(environment && environment->water()){
//		if (isWater(x,y) || isWater(x+1,y) || isWater(x,y+1) || isWater(x+1,y+1) )
		if (isWater(x,y))
			val |= WATER_FLAG;
		else
			val |= GROUND_FLAG;
	} else
		val |= GROUND_FLAG;

	Vect3f terNormal = enableAutoImpassability_ ? normalMap->normal(x,y)+normalMap->normal(x+1,y)+normalMap->normal(x,y+1)+normalMap->normal(x+1,y+1) : Vect3f(0,0,4);
	if(((terNormal.z < 3.5f) && !(val & WATER_FLAG)) || num_of_impassability)
		val |= IMPASSABILITY_FLAG;

//	if((*universe()->secondMap)(x,y) && (*universe()->secondMap)(x,y) > h) {
	if((*universe()->secondMap)(x,y) || (*universe()->secondMap)(x,y).getElevator()) {
	
		if((*universe()->secondMap)(x,y).getElevator()) {
			val = ELEVATOR_FLAG;
			secondVal = ELEVATOR_FLAG;
		} else if(((*universe()->secondMap)(x,y) - h) < 30 && (*universe()->secondMap)(x,y) > h) {
			val |= IMPASSABILITY_FLAG;
			secondVal |= SECOND_MAP_FLAG;
		} else
			secondVal |= SECOND_MAP_FLAG;
	}

	map[y*sizeX+x] = val | (map[y*sizeX+x] & 0x0000FFFF);
	secondMap[y*sizeX+x] = secondVal;
}

//=======================================================
void PathFinder::updateRect( int x1, int y1, int dx, int dy )
{
	int x2 = (x1 + dx)>>world_shift;
	int y2 = (y1 + dy)>>world_shift;

	x1 = x1>>world_shift;
	y1 = y1>>world_shift;

	for(int y = y1;y <= y2; y++)
	for(int x = x1; x <= x2; x++)
		updateTile(x,y);
}

//=======================================================
void PathFinder::recalcPathFinder()
{
	start_timer_auto();
	if(!recalcClustersTimer_) {
		clusterization();
		recalcClustersTimer_.start(3000);
	}
}

//=======================================================
bool PathFinder::findNearestPoint( const Vect2i& _startPoint, int scanRadius, PFTile flags, Vect2i& retPoint )
{
/*
	curFlags = flags;
	findNearestFlag_ = true;

	startPoint = Vect2f((float)_startPoint.x / (1<<world_shift), (float)_startPoint.y / (1<<world_shift) + 0.1);

	if(startPoint.x < 0 || startPoint.x >= sizeX || startPoint.y < 0 || startPoint.y >= sizeY)
		return false;

	clusterHeuristic.startPoint = startPoint;

	Vect2i startPointI((int)startPoint.x, (int)startPoint.y);

    clusterHeuristic.startCluster = getClusterSecondMap(startPointI);
	if(!clusterHeuristic.startCluster)
		clusterHeuristic.startCluster = getClusterMap(startPointI);

	clusterHeuristic.startCluster->p = startPoint;
	path.clear();
	bool avalible = astar.FindPath(clusterHeuristic.startCluster, &clusterHeuristic, path);
	
	if(!avalible)
		return false;

	PFCluster* retCluster = path[path.size()-1];

	retPoint = Vect2i(((retCluster->x1 << world_shift) + (retCluster->x2 << world_shift))/2, ((retCluster->y1 << world_shift) + (retCluster->y2 << world_shift))/2);

	return true;
*/

	if(clusterStorage.empty())
		return false;

	int xl = (_startPoint.x - scanRadius) >> world_shift;
	int xr = (_startPoint.x + scanRadius) >> world_shift;

	int yu = (_startPoint.y - scanRadius) >> world_shift;
	int yd = (_startPoint.y + scanRadius) >> world_shift;

	xl = clamp(xl, 0, sizeX-1);
	xr = clamp(xr, 0, sizeX-1);
	yu = clamp(yu, 0, sizeY-1);
	yd = clamp(yd, 0, sizeY-1);

	for(int y = yu; y <= yd; y++)
		for(int x = xl; x <= xr; x++) {
			PFCluster* cluster = getClusterMap(x, y);
			if(cluster && (flags & cluster->val)) {
				retPoint.x = (x << world_shift) + (1 << (world_shift - 1));
				retPoint.y = (y << world_shift) + (1 << (world_shift - 1));
				return true;
			}
		}

	return false;
}

//=======================================================
void waterChangePF(int x, int y)
{
	Rectf rect;
	rect.left((x-2) << environment->water()->GetCoordShift());
	rect.top((y-2) << environment->water()->GetCoordShift());
	rect.height(4 << environment->water()->GetCoordShift());
	rect.width(4 << environment->water()->GetCoordShift());
	tileMapUpdateRegions.push_back(rect);
}

//=======================================================
void iceChangePF(int x, int y)
{
	Rectf rect;
	rect.left(x-1 << environment->temperature()->gridShift());
	rect.top(y-1 << environment->temperature()->gridShift());
	rect.height(2 << environment->temperature()->gridShift());
	rect.width(2 << environment->temperature()->gridShift());
	tileMapUpdateRegions.push_back(rect);
}

//=======================================================
bool PathFinder::impassabilityCheck( int xc, int yc, int r )
{
	int xL = (xc-r) >> world_shift;
	int xR = (xc+r) >> world_shift;
	int yT = (yc-r) >> world_shift;
	int yD = (yc+r) >> world_shift;

	int x,y;
	int a=0;
	for(y=yT; y<=yD; y++){
		for(x=xL; x<=xR; x++){
			a |= map[y*sizeX+x];
		}
	}

	return (a & IMPASSABILITY_FLAG);
}

//=======================================================
bool PathFinder::checkWater( int xc, int yc, int r )
{
	int xL = (xc-r) >> world_shift;
	int xR = (xc+r) >> world_shift;
	int yT = (yc-r) >> world_shift;
	int yD = (yc+r) >> world_shift;

	int x,y;
	int a=0;
	for(y=yT; y<=yD; y++){
		for(x=xL; x<=xR; x++){
			a |= map[y*sizeX+x];
		}
	}

	return a & WATER_FLAG;
}

//=======================================================
float PathFinder::checkWaterFactor( int xc, int yc, int r )
{
	if((xc - r + 0.5)<0 || (xc + r + 0.5) >= vMap.H_SIZE) return 0;
	if((yc - r + 0.5)<0 || (yc + r + 0.5) >= vMap.V_SIZE) return 0;

	int xL = (xc-r) >> world_shift;
	int xR = (xc+r) >> world_shift;
	int yT = (yc-r) >> world_shift;
	int yD = (yc+r) >> world_shift;

	int x,y;
	float a=0;
	for(y=yT; y<=yD; y++){
		for(x=xL; x<=xR; x++){
			a  += (map[y*sizeX+x] & WATER_FLAG)?1:0;
		}
	}

	return a/((xR-xL+1)*(yD-yT+1));
}

//=======================================================
void PathFinder::projectUnit( UnitBase* unit )
{
	PFUnitMapTile flag = getUnitFlag(unit);

	if(!flag)
		return;

	start_timer_auto();
/*
	Vect3f p1(unit->rigidBody()->boxMin());
	Vect3f p3(unit->rigidBody()->boxMax().x, unit->rigidBody()->boxMax().y, p1.z);
	Vect3f p2(p1.x, p3.y, p1.z);
	Vect3f p4(p3.x, p1.y, p1.z);

	unit->rigidBody()->pose().xformPoint(p1);
	unit->rigidBody()->pose().xformPoint(p2);
	unit->rigidBody()->pose().xformPoint(p3);
	unit->rigidBody()->pose().xformPoint(p4);

	int px[4] = {p1.xi(), p2.xi(), p3.xi(), p4.xi()};
	int py[4] = {p1.yi(), p2.yi(), p3.yi(), p4.yi()};

	drawPoly(px, py, 4, flag);*/

	drawCircle(unit->position2D().xi(), unit->position2D().yi(), unit->radius(), flag);
}

//=======================================================
PFUnitMapTile PathFinder::getUnitFlag( UnitBase* unit )
{
	if(unit->attr().isEnvironment()) {
		UnitEnvironment* unitEnv = safe_cast<UnitEnvironment*>(unit);
		switch(unitEnv->environmentType()) {
			case ENVIRONMENT_PHANTOM2:
			case ENVIRONMENT_FENCE:
			case ENVIRONMENT_FENCE2:
			case ENVIRONMENT_ROCK:
			case ENVIRONMENT_BASEMENT:
			case ENVIRONMENT_BARN:
			case ENVIRONMENT_BUILDING:
			case ENVIRONMENT_BIG_BUILDING:
			case ENVIRONMENT_INDESTRUCTIBLE:
				return unitEnv->environmentType();
		}
		return 0;
	}

	if(unit->attr().isBuilding())
		switch(safe_cast_ref<const AttributeBuilding&>(unit->attr()).interactionType) {
			case ENVIRONMENT_PHANTOM2:
			case ENVIRONMENT_FENCE:
			case ENVIRONMENT_FENCE2:
			case ENVIRONMENT_ROCK:
			case ENVIRONMENT_BASEMENT:
			case ENVIRONMENT_BARN:
			case ENVIRONMENT_BUILDING:
			case ENVIRONMENT_BIG_BUILDING:
			case ENVIRONMENT_INDESTRUCTIBLE:
				return safe_cast_ref<const AttributeBuilding&>(unit->attr()).interactionType;
		}

//	if(unit->attr().isResourceItem() && unit->attr().enablePathFind)
//		return ENVIRONMENT_INDESTRUCTIBLE;

	return 0;
}

//=======================================================
__forceinline void PathFinder::cell(int x, int y, PFUnitMapTile data)
{
	if(x >= 0 && x < sizeX && y >= 0 && y < sizeY)
		map[y*sizeX+x] |= data;
}

//=======================================================
void PathFinder::drawPoly(int pX[], int pY[], int n, PFUnitMapTile data)
{
	#define DIV(a, b)	(((a) << 16)/(b))
	#define CCW(i)	(i == 0 ? n - 1 : i - 1)
	#define CW(i)	(i == n - 1 ? 0 : i + 1)

	for(int i = 0; i < n; i++){
		pX[i] >>= world_shift;
		pY[i] >>= world_shift;
	}		

	int vals_up = 0;
	for(int i = 1; i < n; i++)
		if(pY[vals_up] > pY[i])
			vals_up = i;

	int lfv = vals_up;
	int rfv = vals_up;
	int ltv = CCW(lfv);
	int rtv = CW(rfv);

	int Y = pY[lfv]; 
	int xl = pX[lfv];
	int al = pX[ltv] - xl; 
	int bl = pY[ltv] - Y;
	int ar = pX[rtv] - xl; 
	int br = pY[rtv] - Y;
	int xr = xl = (xl << 16) + (1 << 15);

	if(bl)
		al = DIV(al, bl);
	if(br)
		ar = DIV(ar, br);

	int d, where;

	while(1){
		if(bl > br){
			d = br;
			where = 0;
			}
		else{
			d = bl;
			where = 1;
			}
		while(d-- > 0){
			int x1 = xl >> 16;
			int x2 = xr >> 16;

			if(x1 > x2)
				swap(x1, x2);

			while(x1 <= x2)
				cell(x1++, Y, data);

			Y++;
			xl += al;
			xr += ar;
			}
		if(where){
			if(ltv == rtv){
				int x1 = xl >> 16;
				int x2 = xr >> 16;

				if(x1 > x2)
					swap(x1, x2);

				while(x1 <= x2)
					cell(x1++, Y, data);
				return;
			}
			lfv = ltv;
			ltv = CCW(ltv);

			br -= bl;
			xl = pX[lfv];
			al = pX[ltv] - xl;
			bl = pY[ltv] - Y;
			xl = (xl << 16) + (1 << 15);
			if(bl)
				al = DIV(al, bl);
			}
		else{
			if(rtv == ltv){
				int x1 = xl >> 16;
				int x2 = xr >> 16;

				if(x1 > x2)
					swap(x1, x2);

				while(x1 <= x2)
					cell(x1++, Y, data);
				return;
			}
			rfv = rtv;
			rtv = CW(rtv);

			bl -= br;
			xr = pX[rfv];
			ar = pX[rtv] - xr;
			br = pY[rtv] - Y;
			xr = (xr << 16) + (1 << 15);
			if(br)
				ar = DIV(ar, br);
			}
		}

	#undef DIV
	#undef CCW
	#undef CW
}

//=======================================================
void PathFinder::drawCircle( int px, int py, int radius, PFUnitMapTile data)
{
	int xl = (px - radius - (1 << (world_shift -1))) >> world_shift;
	int xr = (px + radius + (1 << (world_shift -1))) >> world_shift;

	int yu = (py - radius - (1 << (world_shift -1))) >> world_shift;
	int yd = (py + radius + (1 << (world_shift -1))) >> world_shift;

	for(int i = xl; i <= xr; i++){
		int iw = (i << world_shift) + (1 << (world_shift -1));
		for(int j = yu; j <= yd; j++){
			int jw = (j << world_shift) + (1 << (world_shift -1));
			if((sqr(iw - px) + sqr(jw-py)) < sqr(radius + 5.66f))
				cell(i, j, data);
		}
	}
}

//=======================================================
void PathFinder::showDebugInfo()
{
//	if(path.empty())
//		return;

	vector<PFCluster>::iterator si;
	FOR_EACH(clusterStorage, si)
	if(!si->isSecondMap) {
		show_terrain_line(Vect2f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), RED);
		show_terrain_line(Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), Vect2f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift)), RED);
		show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift)), Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), RED);
		show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift)), RED);
		if(si->val & ELEVATOR_FLAG)
			show_terrain_line(Vect2f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift)), RED);
		if(si->val & IMPASSABILITY_FLAG)
			show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), GREEN);
		if(si->val & 0x0000FFFF)
			show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), MAGENTA);
	} else {
		show_line(Vect3f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift), 260), Vect3f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift), 260), BLUE);
		show_line(Vect3f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift), 260), Vect3f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift), 260), BLUE);
		show_line(Vect3f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift), 260), Vect3f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift), 260), BLUE);
		show_line(Vect3f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift), 260), Vect3f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift), 260), BLUE);
		if(si->val & ELEVATOR_FLAG)
			show_line(Vect3f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift), 260), Vect3f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift), 260), BLUE);
	}

/*
	vector<PFCluster*>::iterator si;
	FOR_EACH(path, si) {
		show_line(Vect3f((*si)->x1 * (1 << world_shift), (*si)->y1 * (1 << world_shift), 0), Vect3f((*si)->x1 * (1 << world_shift), (*si)->y2 * (1 << world_shift), 0), RED);
		show_line(Vect3f((*si)->x1 * (1 << world_shift), (*si)->y2 * (1 << world_shift), 0), Vect3f((*si)->x2 * (1 << world_shift), (*si)->y2 * (1 << world_shift), 0), RED);
		show_line(Vect3f((*si)->x2 * (1 << world_shift), (*si)->y2 * (1 << world_shift), 0), Vect3f((*si)->x2 * (1 << world_shift), (*si)->y1 * (1 << world_shift), 0), RED);
		show_line(Vect3f((*si)->x2 * (1 << world_shift), (*si)->y1 * (1 << world_shift), 0), Vect3f((*si)->x1 * (1 << world_shift), (*si)->y1 * (1 << world_shift), 0), RED);
	}*/
}

