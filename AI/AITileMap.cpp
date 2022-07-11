#include "StdAfx.h"
#include "AITileMap.h"
#include "AIMain.h"
#include "ClusterFind.h"
#include "RenderObjects.h"
#include "terra.h"

#include "..\Environment\Environment.h"
#include "..\Water\ice.h"

struct ClusterFindPrm
{
	int clusterSize;
	int rebuildQuants;
	int enableSmoothing;

	int levelOfDetail;
	int maxColor;
	int minimizeMinLevel;
	int showMap;

	int gunExtraRadius;
	int fieldExtraRadius;

	int chaosSmoothSteps;

	int ignorePathFindMinDistance2;

	ClusterFindPrm() {
		clusterSize = 10;
		rebuildQuants = 12;
		enableSmoothing = 1;

		levelOfDetail = 8;
		maxColor = 64;
		minimizeMinLevel = 2;
		showMap = 0;

		gunExtraRadius = 0;
		fieldExtraRadius = 0;

		chaosSmoothSteps = 6;

		ignorePathFindMinDistance2 = 100*100;
	}
};

ClusterFindPrm terrainPathFind;

bool AITile::update(int x,int y)
{
	bool prev_log = completed();

	height_min = 255;
	dig_work = 0;
	dig_less = false;
	
	int height_avr = 0;
	
	// Считаем непроходимость - если больше половины - то непроходимая.
	int num_of_impassability = 0;

	for(int yy = 0;yy < tile_size;yy++)
	{
		int offset = vMap.offsetGBuf(x*tile_size, y*tile_size + yy);
		unsigned char* h_buffer = vMap.GVBuf + offset;
		unsigned short* attr_buffer = vMap.GABuf + offset;

		for(int xx = 0;xx < tile_size;xx++)
		{
			unsigned char h = h_buffer[xx];
			unsigned short attr = attr_buffer[xx];

			//if(!GRIDTST_TALLER_HZEROPLAST(attr) || !(attr & GRIDAT_INDESTRUCTABILITY))
			//	dig_work += abs(h_buffer[xx] - hZeroPlast);
			//else
			//	dig_less = true;

			if(height_min > h)
				height_min = h;

			if(attr & GRIDAT_IMPASSABILITY)
				num_of_impassability++;
		}
	}

	if(num_of_impassability > (tile_size*tile_size)/2)
		impassability = true;

	if(environment && environment->water()){
		cWater::OnePoint op = environment->water()->Get(x,y);
		if (op.z > 0) 
			water = true;
		else
			water = false;
	}

	return completed() && prev_log != completed();
//	return true;
}

////////////////////////////////////////////////////////
//			AITileMap
////////////////////////////////////////////////////////
AITileMap::AITileMap(int hsize,int vsize) 
: Map2D<AITile, AITile::tile_size_world_shl>(hsize,vsize)
{ 
	pWalkMap=NULL;

	path_finder = new ClusterFind(sizeX(), sizeY(), terrainPathFind.clusterSize);
	path_finder2 = new ClusterFind(sizeX(), sizeY(), terrainPathFind.clusterSize);

	InitialUpdate(); 
}

AITileMap::~AITileMap()
{
	RELEASE(pWalkMap);
	delete path_finder;
	delete path_finder2;
}

void AITileMap::InitialUpdate()
{
	for(int y=0;y < sizeY();y++)
		for(int x=0;x < sizeX();x++)
			(*this)(x,y).update(x,y);

	rebuildWalkMap(path_finder->GetWalkMap());
	path_finder->Set(terrainPathFind.enableSmoothing);

	memcpy(path_finder2->GetWalkMap(),path_finder->GetWalkMap(),sizeX()*sizeY()*sizeof(path_finder2->GetWalkMap()[0]));
	path_finder2->SetLater(terrainPathFind.enableSmoothing, terrainPathFind.rebuildQuants);

}

void AITileMap::UpdateRect(int x1,int y1,int dx,int dy)
{
	int x2 = w2mFloor(x1 + dx);
	int y2 = w2mFloor(y1 + dy);

	x1 = w2mFloor(x1);
	y1 = w2mFloor(y1);

	for(int y = y1;y <= y2; y++)
	for(int x = x1; x <= x2; x++)
		(*this)(x,y).update(x,y);
}

void AITileMap::placeBuilding(const Vect2i& v1, const Vect2i& size, bool place)
{
	Vect2i v2 = v1 + size;

	for(int y = v1.y; y < v2.y; y++)
		for(int x = v1.x; x < v2.x; x++)
			(*this)(x,y).building = place;
}

bool AITileMap::readyForBuilding(const Vect2i& v1, const Vect2i& size)
{
	Vect2i v2 = v1 + size;

	for(int y = v1.y; y < v2.y; y++)
		for(int x = v1.x; x < v2.x; x++)
		{
			AITile& tile = (*this)(x,y);
			if(!tile.completed() || tile.building)
				return false;
		}

	return true;
}
bool AITileMap::findPath(const Vect2i& from_w, const Vect2i& to_w, vector<Vect2i>& out_path, AStarTile flags = ClusterFind::GROUND_MASK)
{
	Vect2i from = w2m(from_w);
	Vect2i to = w2m(to_w);

	if(!inside(from) || !inside(to))
		return false;

	flags_ = flags;
	bool b = path_finder->FindPath(from, to, out_path, ClusterHeuristicComplex());

	vector<Vect2i>::iterator it;
	FOR_EACH(out_path,it)
		*it = m2w(*it);

	if(!out_path.empty() && b)
		out_path.back() = to_w;

	return true;
}

void AITileMap::rebuildWalkMap(AStarTile* walk_map)
{
	int size = sizeY()*sizeX();
	memset(walk_map,0,size*sizeof(AStarTile));
	for(int i = 0;i < size;i++) {
/*		if(!map()[i].impassability && !map()[i].water) {

			int dh=0;
			if(!walk_map[i])
				walk_map[i] = dh;
			if(dh==terrainPathFind.levelOfDetail)
			{
				if(i-1>=0)
					walk_map[i-1]=dh;
				if(i+1<size)
					walk_map[i+1]=dh;
				if(i-sizeX()>=0)
					walk_map[i-sizeX()]=dh;
				if(i+sizeX()<size)
					walk_map[i+sizeX()]=dh;
			}
		}
		else 
			walk_map[i] = ClusterHeuristicDitch::heuristic_ditch;*/
		
		int walk = 0;
		if(map()[i].impassability)
			walk_map[i] |= ClusterFind::IMPASSABILITY_MASK;
		if(map()[i].water)
			walk_map[i] |= ClusterFind::WATER_MASK;
		else 
			walk_map[i] |= ClusterFind::GROUND_MASK;
	}
	if(terrainPathFind.showMap==1)
		updateWalkMap(walk_map);
}

void AITileMap::recalcPathFind()
{
	if(path_finder2->SetLaterQuant()){
		swap(path_finder2,path_finder);

		rebuildWalkMap(path_finder2->GetWalkMap());
		path_finder2->SetLater(terrainPathFind.enableSmoothing,terrainPathFind.rebuildQuants);
	}
}

void AITileMap::updateWalkMap(AStarTile* walk_map)
{
	if(!pWalkMap)
		pWalkMap=GetTexLibrary()->CreateTexture(sizeX(),sizeY(),false);
	int Pitch;
	BYTE* pBits=pWalkMap->LockTexture(Pitch);
	BYTE mul=255/terrainPathFind.levelOfDetail;

	for(int y=0;y<sizeY();y++)
	{
		sColor4c* p=(sColor4c*)pBits;
		AStarTile* pwalk=walk_map+y*sizeX();
		for(int x=0;x<sizeX();x++,p++,pwalk++)
		{
			p->set(*pwalk,*pwalk,*pwalk);
//			AStarTile c=mul* *pwalk;
//			BYTE up=ClusterFind::UP_MASK & *pwalk;
//			BYTE up=ClusterFind::WATER_MASK & *pwalk;
//			if(up)
//				p->set(0,255,0);
//			else
//				p->set(c,c,c);
		}

		pBits+=Pitch;
	}
	pWalkMap->UnlockTexture();
}

void AITileMap::drawWalkMap()
{
	if(!(pWalkMap && terrainPathFind.showMap))
		return;

	gb_RenderDevice->DrawSprite(0,32,256,256,0,0,1,1,pWalkMap);

	char str[256];
	cFont* pFont=gb_VisGeneric->CreateDebugFont();
	gb_RenderDevice->SetFont(pFont);
	sprintf(str,"cluster=%i",path_finder->GetNumCluster());
	gb_RenderDevice->OutText(0,288,str,sColor4f(1,1,1,1));

	gb_RenderDevice->SetFont(NULL);
	pFont->Release();
}

void waterTileChange(int x, int y)
{
	int grid_x = environment->water()->GetGridSizeX();
	int grid_y = environment->water()->GetGridSizeY();

	if(x==(grid_x-1)) x=grid_x-2;
	if(y==(grid_y-1)) y=grid_y-2;

	int grid_s = environment->water()->GetCoordShift();
	ai_tile_map->UpdateRect(x<<grid_s,y<<grid_s,(1<<grid_s)-1,(1<<grid_s)-1);

}
