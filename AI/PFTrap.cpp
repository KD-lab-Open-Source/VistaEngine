#include "stdAfx.h"
#include "AI\PFTrap.h"

#include "universe.h"
#include "Environment\Environment.h"
#include "Terra\vmap.h"
#include "normalMap.h"
#include "BaseUnit.h"
#include "UnitEnvironment.h"
#include "IronBuilding.h"
#include "GlobalAttributes.h"

//=======================================================

PathFinder* pathFinder;
list<Rectf> tileMapUpdateRegions;

//=======================================================
PathFinder::PathFinder(int _sizeX, int _sizeY)
{
	sizeX = _sizeX >> world_shift;
	sizeY = _sizeY >> world_shift;

	map = new PFTile[sizeX*sizeY];
	memset(map, 0, sizeX*sizeY*sizeof(PFTile));

	clusterMap = new PFClusterIndex[sizeX*sizeY];
	
	recalcClustersTimer_.stop();
	enableAutoImpassability_ = false;
	findNearestFlag_ = false;
}

//=======================================================
PathFinder::~PathFinder()
{
	delete[] map;
	delete[] clusterMap;
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
				clusterStorage.push_back(c);
			}

	memset(clusterMap, 0, sizeX*sizeY*sizeof(PFClusterIndex));
	
	for(int i=0;i<clusterStorage.size();i++)
		fillRegionMap(clusterStorage[i].x1, clusterStorage[i].y1, clusterStorage[i].x2, clusterStorage[i].y2, i);

	astar.Init(clusterStorage);

	for(int i=0; i<sizeY*sizeX; i++)
		map[i] &= GROUND_FLAG | WATER_FLAG | IMPASSABILITY_FLAG;
}

//=======================================================
__forceinline PathFinder::PFCluster* PathFinder::getClusterMap( int x, int y )
{
	return &clusterStorage[clusterMap[y * sizeX + x]];
}

//=======================================================
bool PathFinder::findPath(const Vect2f& _startPoint, const Vect2f& _endPoint, PFTile flags, int impassibility, const float* possibilityFactors, vector<Vect2f>& points)
{
	curFlags = flags;
	impassability = impassibility;
	passabilityFactors_ = possibilityFactors;
	findNearestFlag_ = false;

	startPoint = Vect2f((float)_startPoint.x / (1<<world_shift), (float)_startPoint.y / (1<<world_shift) + 0.1f);
	endPoint = Vect2f((float)_endPoint.x / (1<<world_shift) + 0.1f, (float)_endPoint.y / (1<<world_shift));

	if(startPoint.x < 0 || startPoint.x >= sizeX || startPoint.y < 0 || startPoint.y >= sizeY)
		return false;

	if(endPoint.x < 0 || endPoint.x > sizeX || endPoint.y < 0 || endPoint.y > sizeY)
		return false;

	clusterHeuristic.startPoint = startPoint;
	clusterHeuristic.endPoint = endPoint;

	Vect2i startPointI((int)startPoint.x, (int)startPoint.y);
	Vect2i endPointI((int)endPoint.x, (int)endPoint.y);

	clusterHeuristic.startCluster = getClusterMap(startPointI.x, startPointI.y);

	clusterHeuristic.endCluster = getClusterMap(endPointI.x, endPointI.y);
	
	clusterHeuristic.startCluster->p = startPoint;
	path.clear();
	bool considerImpassabilities = !(clusterHeuristic.startCluster->checkImpassability(impassability, curFlags));
	bool avalible = astar.FindPath(clusterHeuristic.startCluster, &clusterHeuristic, considerImpassabilities, path);
	if(path.empty())
		return true;
	if(!avalible || (impassability & (1 << clusterHeuristic.endCluster->getTerrainType()))){
		if(!GlobalAttributes::instance().moveToImpassabilities)
			return false;
		callcPoints(points);
		PFCluster* endCluster = path[path.size()-1];
		Rectf rect(Vect2f(endCluster->x1, endCluster->y1), Vect2f(endCluster->x2 - endCluster->x1, endCluster->y2 - endCluster->y1));
		if(points.empty()){
			rect.clipLine(endPoint, startPoint);
			points.push_back(Vect2i(startPoint.x * (1 << world_shift), startPoint.y * (1 << world_shift)));
			return false;
		}
		Vect2f clipPoint = Vect2f(points[points.size()-1]) / (1 << world_shift);
		rect.clipLine(endPoint, clipPoint);
		points.push_back(Vect2f(clipPoint.x * (1 << world_shift), clipPoint.y * (1 << world_shift)));
		return false;
	}
	callcPoints(points);
	points.push_back(_endPoint);
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
inline float PathFinder::PFClusterHeuristic::GetG(PFCluster* pos1, PFCluster* pos2)
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

	if((pos2->val & (~(pos2->pathFinder->curFlags | IMPASSABILITY_FLAG))) || (pos1->val & (~(pos1->pathFinder->curFlags | IMPASSABILITY_FLAG))))
		moveFactor = HEURISTIC_MAX;

	if(!(pos1->pathFinder->impassability & (1 << pos1->getTerrainType())) && !(pos2->pathFinder->impassability & (1 << pos2->getTerrainType()))){
		float possibility = sqrtf(pos2->pathFinder->passabilityFactors_[pos2->getTerrainType()] * pos1->pathFinder->passabilityFactors_[pos1->getTerrainType()]);
		moveFactor /= possibility;
	}else
		moveFactor *= HEURISTIC_MAX;
	return moveFactor;
}

//=======================================================
inline float PathFinder::PFClusterHeuristic::GetDistance(PFCluster* pos1, PFCluster* pos2)
{
	if(pos2 == endCluster)
		return (pos2->pathFinder->endPoint.distance(pos1->p));

	return (pos2->p.distance(pos1->p));

}

//=======================================================
inline bool PathFinder::PFClusterHeuristic::IsEndPoint( PFCluster* pos )
{
	if(pos->pathFinder->findNearestFlag_)
		return pos->val & pos->pathFinder->curFlags;

	return pos == endCluster;
}

//=======================================================
void PathFinder::callcPoints(vector<Vect2f>& points)
{
	edges.clear();
	PFEdge edge;

	for(int i=0;i<path.size()-1;i++) {
		getClusterLine(path[i], path[i+1], edge.v1, edge.v2);
		edges.push_back(edge);
	}

	wayPoints.resize(edges.size());
	callcWPoint(0,edges.size(), startPoint, endPoint);

	for(int i=0;i<wayPoints.size();i++)
		if(!wayPoints[i].eq(Vect2f::ZERO))
			points.push_back(Vect2f(wayPoints[i].x * (1 << world_shift), wayPoints[i].y * (1 << world_shift)));

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

__forceinline bool PathFinder::isWater( int x, int y)
{
	if(x < sizeX && y < sizeY && environment->temperature() && environment->temperature()->checkTile(x >>  environment->temperature()->gridShift() - world_shift, y >>  environment->temperature()->gridShift() - world_shift))
		return false;

	return environment->water()->GetRelativeZ(x,y) > water->relativeWaterLevel() &&
		environment->water()->GetRelativeZ(x+1,y) > water->relativeWaterLevel() &&
		environment->water()->GetRelativeZ(x+1,y+1) > water->relativeWaterLevel() &&
		environment->water()->GetRelativeZ(x,y+1) > water->relativeWaterLevel();
}

//=======================================================
void PathFinder::updateTile( int x, int y )
{
	if(x < 0 || x >= sizeX || y < 0 || y >= sizeY)
		return;

	int tile_size = 4;
	
	int counters[TERRAIN_TYPES_NUMBER];
	memset(&counters, 0, sizeof(counters));
	for(int yy = 0; yy < tile_size; ++yy){
		int offset = vMap.offsetGBuf(x * tile_size, y * tile_size + yy);
		unsigned short* attr_buffer = vMap.gABuf + offset;
		for(int xx = 0; xx < tile_size; ++xx){
			++counters[attr_buffer[xx] & GRIDAT_MASK_SURFACE_KIND];
		}
	}

	int iMax = 0, nMax = 0;
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
		if(nMax < counters[i]){
			nMax = counters[i];
			iMax = i;
		}

	PFTile val = iMax << 16;
	
	if(environment && environment->water() && isWater(x,y))
		val |= WATER_FLAG;
	else
		val |= GROUND_FLAG;

	map[y*sizeX+x] = val | (map[y*sizeX+x] & (ENVIRONMENT_FLAG | FIELD_FLAG));
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
	if(!recalcClustersTimer_.busy()){
		clusterization();
		recalcClustersTimer_.start(3000);
	}
}

//=======================================================
bool PathFinder::findNearestPoint( const Vect2i& _startPoint, int scanRadius, PFTile flags, Vect2i& retPoint )
{
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
int PathFinder::getFlag(int xc, int yc, int r)
{
	int xL = (xc - r) >> world_shift;
	int xR = (xc + r) >> world_shift;
	int yT = (yc - r) >> world_shift;
	int yD = (yc + r) >> world_shift;

	int a = 0;
	for(int y = yT; y <= yD; ++y){
		for(int x = xL; x <= xR; ++x){
			a |= map[y * sizeX + x];
		}
	}
	return a;
}

//=======================================================
int PathFinder::getTerrainType(int xc, int yc, int r)
{
	int xL = (xc - r) >> world_shift;
	int xR = (xc + r) >> world_shift;
	int yT = (yc - r) >> world_shift;
	int yD = (yc + r) >> world_shift;
	int counters[TERRAIN_TYPES_NUMBER];
	memset(&counters, 0, sizeof(counters));
	for(int y = yT; y <= yD; ++y){
		for(int x = xL; x <= xR; ++x){
			++counters[(map[y * sizeX + x] & IMPASSABILITY_FLAG) >> 16];
		}
	}
	int iMax = 0, nMax = 0;
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
		if(nMax < counters[i]){
			nMax = counters[i];
			iMax = i;
		}
	return iMax;
}

//=======================================================
bool PathFinder::impassabilityCheckForUnit(int xc, int yc, int r, int impassibility)
{
	return (1 << getTerrainType(xc, yc, r)) & impassibility;
}

//=======================================================
bool PathFinder::checkField(int xc, int yc, int r, int fieldFlag)
{
	return getFlag(xc, yc, r) & fieldFlag;
}

//=======================================================
bool PathFinder::checkWater(int xc, int yc, int r)
{
	return getFlag(xc, yc, r) & WATER_FLAG;
}

//=======================================================
void PathFinder::projectUnit( UnitBase* unit )
{
	start_timer_auto();

	PFUnitMapTile flag = getUnitFlag(unit);

	if(!flag)
		return;

	if(unit->rigidBody()->ptBoundCheck()){
		Vect3f extent(unit->rigidBody()->extent());
		unit->orientation().xform(extent);
		Vect2f p1(unit->position2D() - extent);
		Vect2f p2(unit->position2D() + extent);
		Vect2i p1i(round(p1.x + 0.5f), round(p1.y + 0.5f));
		Vect2i p2i(round(p2.x + 0.5f), round(p2.y + 0.5f));
		int px[4] = {p1i.x, p1i.x, p2i.x, p2i.x};
		int py[4] = {p1i.y, p2i.y, p2i.y, p1i.y};
		drawPoly(px, py, 4, flag);
	}else
		drawCircle(unit->position2D().xi(), unit->position2D().yi(), unit->radius(), flag);
}

//=======================================================
PFUnitMapTile PathFinder::getUnitFlag( UnitBase* unit )
{
	if(unit->attr().isEnvironment()) {
		UnitEnvironment* unitEnv = safe_cast<UnitEnvironment*>(unit);
		switch(unitEnv->environmentType()) {
			case ENVIRONMENT_PHANTOM2:
			case ENVIRONMENT_STONE: 
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
			case ENVIRONMENT_STONE:
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

	return 0;
}

//=======================================================
__forceinline void PathFinder::cell(int x, int y, PFTile data)
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
void PathFinder::drawCircle( int px, int py, int radius, PFTile data)
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
	vector<PFCluster>::iterator si;
	FOR_EACH(clusterStorage, si){
		Color4c terrainColor(si->getTerrainType() * 15 + 30, 0, 0);
		show_terrain_line(Vect2f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), terrainColor);
		show_terrain_line(Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), Vect2f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift)), terrainColor);
		show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y2 * (1 << world_shift)), Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), terrainColor);
		show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y1 * (1 << world_shift)), terrainColor);
		if(si->val & ENVIRONMENT_FLAG)
			show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), Color4c::MAGENTA);
		if(si->val & FIELD_FLAG)
			show_terrain_line(Vect2f(si->x2 * (1 << world_shift), si->y1 * (1 << world_shift)), Vect2f(si->x1 * (1 << world_shift), si->y2 * (1 << world_shift)), Color4c::CYAN);
	}
}
