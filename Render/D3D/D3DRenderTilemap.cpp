#include "StdAfxRD.h"
#include "D3DRenderTilemap.h"
#include "D3DRender.h"
#include "cCamera.h"
#include "Scene.h"
#include "CChaos.h"
#include "MultiRegion.h"
#include "VisGeneric.h"
#include "ClippingMesh.h"
#include "Terra\vmap.h"

#define BUMP_IDXTYPE   unsigned short
#define BUMP_IDXSIZE   sizeof(BUMP_IDXTYPE)

int cTileMapRender::bumpGeoScale[TILEMAP_LOD] = { 1, 2, 3, 4, 5 };
int cTileMapRender::bumpTexScale[TILEMAP_LOD] = { 0, 0, 1, 2, 3 };

Vect2i u_to_pos[U_ALL+1]=
{
	Vect2i(-1,0),//U_LEFT=0,
	Vect2i(1,0),//U_RIGHT=1,
	Vect2i(0,-1),//U_TOP=2,
	Vect2i(0,+1),//U_BOTTOM=3,
	Vect2i(-1,-1),//U_LEFT_TOP=4,
	Vect2i(+1,-1),//U_RIGHT_TOP=5,
	Vect2i(-1,+1),//U_LEFT_BOTTOM=6,
	Vect2i(+1,+1),//U_RIGHT_BOTTOM=7,
	Vect2i(0,0),//U_ALL=8,
};

const int texture_border=2;

unsigned int ColorByNormalRGBA(Vect3f n);
unsigned short ColorByNormalWORD(Vect3f n);
unsigned int ColorByNormalDWORD(Vect3f n);

static cTexPools TexPools;
cTileMapRender* tileMapRender;

void cTexPools::GetTilemapTextureMemory(int& total,int& free)
{
	vector<sTilemapTexturePool*>::iterator it;
	total=free=0;
	FOR_EACH(bumpTexPools,it)
	{
		int cur_total,cur_free;
		(*it)->GetTextureMemory(cur_total,cur_free);
		total+=cur_total;
		free+=cur_free;
	}
}

void cTexPools::clear()
{
	for(int i=0;i<bumpTexPools.size();i++)
		delete bumpTexPools[i];
	bumpTexPools.clear();
}

void cTexPools::FindFreeTexture(TexPoolsHandle& h,int tex_width,int tex_height,D3DFORMAT format)
{
	int i;
	for (i = 0; i < bumpTexPools.size(); i++)
	{
		sTilemapTexturePool* p=bumpTexPools[i];
		if (p && p->IsFree()
			&& p->GetTileWidth() == tex_width
			&& p->GetTileHeight() == tex_height &&
			p->getFormat()==format)
				break;
	}

	if (i >= bumpTexPools.size())
	{
		for (i = 0; i < bumpTexPools.size(); i++)
			if (bumpTexPools[i] == 0) break;
		if (i >= bumpTexPools.size())
		{
			i = bumpTexPools.size();
			bumpTexPools.push_back(0);
		}
		bumpTexPools[i] = new sTilemapTexturePool(tex_width, tex_height,format);
	}

	h.pool = i;
	h.page = bumpTexPools[i]->allocPage();
}

Vect2f cTexPools::getUVStart(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->getUVStart(h.page);
}

Vect2f cTexPools::getUVStep(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->getUVStep();
}

void cTexPools::freePage(TexPoolsHandle& h)
{
	if(!h.IsInitialized())
		return;
	bumpTexPools[h.pool]->freePage(h.page);
	h.pool=-1;
	h.page=-1;
}

D3DLOCKED_RECT *cTexPools::LockPage(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->lockPage(h.page);
}

D3DFORMAT cTexPools::getTexFormat(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->getFormat();
}

void cTexPools::UnlockPage(TexPoolsHandle& h)
{
	bumpTexPools[h.pool]->unlockPage(h.page);
}

void cTexPools::AddToRenderList(TexPoolsHandle& h,struct sBumpTile* tile)
{
	bumpTexPools[h.pool]->tileRenderList.push_back(tile);
}

void cTexPools::ClearRenderList()
{
	for (int i = 0; i < bumpTexPools.size(); i++)
		bumpTexPools[i]->tileRenderList.clear();
	shadowTiles.clear();
}

void sTilemapTexturePool::GetTextureMemory(int &total,int& free)
{
	int byte_per_pixel=GetTextureFormatSize(format)/8;
	total=texture_width*texture_height*byte_per_pixel;
	int used=(Pages.size()-freePages)*tileRealWidth*tileRealHeight*byte_per_pixel;
	free=total-used;
}

/////////////////////cTileMapRender////////////////////

cTileMapRender::cTileMapRender(cTileMap *pTileMap)
{
	tileMapRender = this;
	tileMap_=pTileMap;
	tilemapIB=0;

	int dxy=tileMap_->tileNumber().x*tileMap_->tileNumber().y;
	//visMap=new BYTE[dxy];
	vis_lod=new char[dxy];
	for(int i=0;i<dxy;i++)
		vis_lod[i]=-1;

	int ddv=TILEMAP_SIZE+1;
	delta_buffer = new VectDelta[ddv*ddv];

	restoreManagedResource();
}

cTileMapRender::~cTileMapRender()
{
	deleteManagedResource();

	//delete visMap;
	delete vis_lod;
	delete delta_buffer;

	tileMapRender = 0;
}

void cTileMapRender::deleteManagedResource()
{
	for(int y=0;y<tileMap_->tileNumber().y;y++)
		for(int x=0;x<tileMap_->tileNumber().x;x++)
			tileMap_->GetTile(x, y).bumpTileID = -1;

	vector<sBumpTile*>::iterator it;
	FOR_EACH(bumpTiles, it)
		delete *it;
	bumpTiles.clear();

	RELEASE(tilemapIB);

	TexPools.clear();

	vertexPool_.Clear();
	indexPool_.Clear();
}

void cTileMapRender::restoreManagedResource()
{
	if(tilemapIB)
		return;
	int cur_offset=0;
	Vect2i TileSize=tileMap_->tileSize();

	int iLod;
	for(iLod = 0; iLod < TILEMAP_LOD; iLod++)
	{
		int xs = TileSize.x >> bumpGeoScale[iLod];
		int ys = TileSize.y >> bumpGeoScale[iLod];

		index_offset[iLod]=cur_offset;
		index_size[iLod]=6*xs*ys;//?????? может 2 а не 6
		cur_offset+=index_size[iLod];
	}

	sPolygon *pIndex = 0;
	RDCALL(gb_RenderDevice3D->D3DDevice_->CreateIndexBuffer(
		cur_offset * BUMP_IDXSIZE, D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_DEFAULT,
		&tilemapIB,0));
	RDCALL(tilemapIB->Lock(0, 0, (void**)&pIndex,
		D3DLOCK_NOSYSLOCK));
	for(iLod = 0; iLod < TILEMAP_LOD; iLod++){
		int lod_vertex=bumpGeoScale[iLod];
		bumpCreateIB(pIndex + bumpIndexOffset(lod_vertex)/3, lod_vertex);
	}
	tilemapIB->Unlock();
}

void cTileMapRender::dumpManagedResource(XBuffer& buffer)
{
	int FreeTileMemoty,TotalTileMemory;
	vertexPool_.GetUsedMemory(TotalTileMemory,FreeTileMemoty);
	if(TotalTileMemory)
		buffer < "TileMemoty = " <= TotalTileMemory < " byte, slack = " <= FreeTileMemoty*100.0f/TotalTileMemory < "%%\n";

	int total=0,free=0;
	TexPools.GetTilemapTextureMemory(total,free);
	if(total)
		buffer < "TileTexture = " <= total < " byte, slack = " <= free*100.0f/total < "%%\n";

	indexPool_.GetUsedMemory(TotalTileMemory, FreeTileMemoty);
	if(TotalTileMemory)
		buffer < "TileIndex = " <= TotalTileMemory < "byte, slack = " <= FreeTileMemoty*100.0f/TotalTileMemory < "%%\n";
}

cTexPools* cTileMapRender::texturePool()
{
	return &TexPools;
}

void cTileMapRender::bumpCreateIB(sPolygon *ib, int lod)
{
	Vect2i TileSize=tileMap_->tileSize();
	xassert(TileSize.x==TileSize.y);

	int dd = (TileSize.x >> lod);
	int ddv=dd+1;
	for(int y=0;y<dd;y++)
	for(int x=0;x<dd;x++)
	{
		int base=x+y*ddv;
		ib++->set(base,base+ddv,base+1);
		ib++->set(base+ddv,base+ddv+1,base+1);
	}

}

void cTileMapRender::PreDraw(Camera* camera)
{
	start_timer_auto();
	for(int y = 0; y < tileMap_->tileNumber().y; y++)
		for(int x = 0; x < tileMap_->tileNumber().x; x++){
			sTile& tile = tileMap_->GetTile(x, y);
			if(!tile.getAttribute(ATTRTILE_DRAW_VERTEX)){
				if(bumpTileValid(tile.bumpTileID)){
					sBumpTile*& bumpTile = bumpTiles[tile.bumpTileID];
					delete bumpTile;
					bumpTile = 0;
				}
				tile.bumpTileID = -1;
			}

			tile.clearAttribute(ATTRTILE_DRAW_VERTEX);
		}

	xassert(camera == camera->GetRoot());
	SetTilesToRender(camera);
	UpdateLine(tileMap_->tileNumber().y,tileMap_->tileNumber().x);
}

int cTileMapRender::bumpNumVertices(int lod)
{
	int xs = (tileMap_->tileSize().x >> lod);
	int ys = (tileMap_->tileSize().y >> lod);

	return (xs+1)*(ys+1);
}

int cTileMapRender::bumpTileValid(int id)
{
	return id >= 0 && id < bumpTiles.size() && bumpTiles[id];
}

int cTileMapRender::bumpTileAlloc(int lod,int xpos,int ypos)
{
	int i;
	for(i = 0; i < bumpTiles.size(); i++)
		if(!bumpTiles[i]) 
			break;
	if(i == bumpTiles.size()) 
		bumpTiles.push_back(0);
	bumpTiles[i] = new sBumpTile(xpos, ypos);
	bumpTiles[i]->InitVertex(tileMap_, bumpGeoScale[lod]);
	return i;
}

///////////////////////////sTilemapTexturePool/////////////////////////////////
sTilemapTexturePool::sTilemapTexturePool(int width, int height, D3DFORMAT format_)
{
	format=format_;
	texture_width=texture_height=512;
	tileWidth = width;
	tileHeight = height;

	tileRealWidth=tileWidth+texture_border*2;
	tileRealHeight=tileHeight+texture_border*2;
	ustep=1.0f/texture_height;
	vstep=1.0f/texture_width;

	CalcPagesPos();

	freePagesList = new int[Pages.size()];
	freePages = Pages.size();
	for (int i = 0; i < Pages.size(); i++)
		freePagesList[i] = i;

	IDirect3DTexture9* pTex=0;
	RDCALL(gb_RenderDevice3D->D3DDevice_->CreateTexture(
		texture_width, texture_height, 1,
		0, format, D3DPOOL_MANAGED, &pTex,0));
	texture = pTex;
	//D3DFMT_A8R8G8B8
/* Этот кусок теоретически лучше, но надо проверить на Geforce 2
	RDCALL(renderer->lpD3DDevice->CreateTexture(
		texture_width, texture_height, 1,
		D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture,0));
*/
}

sTilemapTexturePool::~sTilemapTexturePool()
{
	delete[] freePagesList;
	texture->Release();
}

void sTilemapTexturePool::CalcPagesPos()
{
	for(int y=0;y<=texture_height-tileRealHeight;y+=tileRealHeight)
	{
		for(int x=0;x<=texture_width-tileRealHeight;x+=tileRealWidth)
		{
			Vect2f page;
			page.x=x;
			page.y=y;
			Pages.push_back(page);
		}
	}
}

int sTilemapTexturePool::allocPage()
{
	if (!freePages) return -1;
	freePages--;
	return freePagesList[freePages];
}

void sTilemapTexturePool::freePage(int page)
{
	xassert(page>=0 && page<Pages.size());
	freePagesList[freePages] = page;
	freePages++;
}

D3DLOCKED_RECT *sTilemapTexturePool::lockPage(int page)
{
	static D3DLOCKED_RECT lockedRect;
	Vect2i pos=Pages[page];
	RECT rect;

	rect.left   = pos.x;
	rect.top    = pos.y;
	rect.right  = rect.left+ tileRealWidth;
	rect.bottom = rect.top + tileRealHeight;
	RDCALL(texture->LockRect(0, &lockedRect, &rect, 0));
	return &lockedRect;
}

void sTilemapTexturePool::unlockPage(int page)
{
	RDCALL(texture->UnlockRect(0));
}

Vect2f sTilemapTexturePool::getUVStart(int page)
{
	xassert(page>=0 && page<Pages.size());
	Vect2i pos=Pages[page];
	return Vect2f((pos.x+texture_border)*ustep,(pos.y+texture_border)*vstep);
}

//////////////////////////sBumpTile//////////////////////////////////////
float sBumpTile::SetVertexZ(int x,int y)
{
	float zi = vMap.getZf(x,y);
	if(cChaos::CurrentChaos() && zi<1)
		zi=-15;
	return zi;
}

sBumpTile::sBumpTile(int xpos,int ypos)
{
	tile_pos.set(xpos,ypos);
	for(int i=0;i<U_ALL;i++)
		border_lod[i]=0;
	lod_vertex=lod_texture=0;

	init_vertex=false;
	init_texture=false;

	tex_uvStart.set(0,0);
	tex_uvStartBump.set(0,0);
	uv_base.set(0,0);
	uv_step.set(0,0);
	uv_base_bump.set(0,0);
}

sBumpTile::~sBumpTile()
{
	DeleteVertex();
	DeleteTexture();
}

void sBumpTile::InitVertex(cTileMap *TileMap, int lod_vertex_)
{
	DeleteVertex();
	lod_vertex=lod_vertex_;
	for(int i=0;i<U_ALL;i++)
		border_lod[i]=lod_vertex;

	int numVertex = TileMap->GetTilemapRender()->bumpNumVertices(lod_vertex);

	zavr = TileMap->GetTile(tile_pos.x, tile_pos.y).zavr;

	tileMapRender->vertexPool()->CreatePage(vtx,VertexPoolParameter(numVertex,GetVertexDeclaration()));
	init_vertex=false;
}

void sBumpTile::InitTexture(cTileMap *TileMap,int lod_texture_)
{
	DeleteTexture();
	lod_texture=lod_texture_;
	int w = TileMap->tileSize().x >> lod_texture;
	int h = TileMap->tileSize().y >> lod_texture;

	TexPools.FindFreeTexture(tex,w,h,TileMap->isTrueColorEnable() ? D3DFMT_A8R8G8B8 : D3DFMT_R5G6B5);

	if(!Option_tileMapVertexLight)
 		TexPools.FindFreeTexture(normal,w,h,D3DFMT_V8U8);

	tex_uvStart=TexPools.getUVStart(tex);
	tex_uvStartBump.set(0,0);
	if(normal.IsInitialized())
		tex_uvStartBump=TexPools.getUVStart(normal);
	Vect2f uvStep=TexPools.getUVStep(tex);
	uv_step.x = uvStep.x/(1<<lod_texture);
	uv_step.y = uvStep.y/(1<<lod_texture);

	int xStart = tile_pos.x * TileMap->tileSize().x;
	int yStart = tile_pos.y * TileMap->tileSize().y;

	uv_base.x=tex_uvStart.x-xStart*uv_step.x;
	uv_base.y=tex_uvStart.y-yStart*uv_step.y;
	uv_base_bump.x=tex_uvStartBump.x-xStart*uv_step.x;
	uv_base_bump.y=tex_uvStartBump.y-yStart*uv_step.y;

	init_texture=false;
}

void sBumpTile::DeleteVertex()
{
	tileMapRender->vertexPool()->DeletePage(vtx);
	DeleteIndex();
}

void sBumpTile::DeleteTexture()
{
	TexPools.freePage(tex);
	TexPools.freePage(normal);
}

void sBumpTile::DeleteIndex()
{
	vector<sPlayerIB>::iterator it;
	FOR_EACH(index, it)
		if(it->index.page>=0)
			tileMapRender->indexPool()->DeletePage(it->index);
	index.clear();
}

RENDER_API void GetNormalArrayShort(char* texture,int pitch,int xstart,int ystart,int xend,int yend,int shift)
{
	int step=1<<shift;
	const int maxbuffer=TILEMAP_SIZE+texture_border*2+2;
	static int buffer[maxbuffer*maxbuffer];
	int clip_mask_x=vMap.H_SIZE-1;
	int clip_mask_y=vMap.V_SIZE-1;
	vMap.getTileZ((char*)buffer,maxbuffer*4,xstart,ystart,xend+step,yend+step,step);
	//step -> 63
	//6-shift-8
	int bump_shift=2+shift;

	for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++)
	{
		int * pz=buffer+ybuffer*maxbuffer;
		char* tx=texture;
		for (int x = xstart; x < xend; x += step)
		{
			int z0=*pz;
			int dzx=*pz-pz[1];
			int dzy=*pz-pz[maxbuffer];
			dzx=max(min(dzx>>bump_shift,127),-128);
			dzy=max(min(dzy>>bump_shift,127),-128);

			tx[0] =dzx;
			tx[1] =dzy;
			tx+=2;
			pz++;
		}
		texture += pitch;
	}
}


void GetNormalArray(char* texture,int pitch,int xstart,int ystart,int xend,int yend,int shift)
{
	int step=1<<shift;
	const int maxbuffer=TILEMAP_SIZE+texture_border*2+1;
	static float buffer[maxbuffer*maxbuffer];
	int clip_mask_x=vMap.H_SIZE-1;
	int clip_mask_y=vMap.V_SIZE-1;

	for(int y = ystart,ybuffer=0; y < yend+step; y += step,ybuffer++)
	{
		float * tx=buffer+ybuffer*maxbuffer;
		int yy=min(max(0,y),clip_mask_y);;
		for (int x = xstart; x < xend+step; x += step,tx++)
		{
			int xx=min(max(0,x),clip_mask_x);
			float z = vMap.getZf(xx,yy);
			*tx=z;
		}
	}

	for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++)
	{
		float * pz=buffer+ybuffer*maxbuffer;
		WORD* tx=(WORD*)texture;
		for (int x = xstart; x < xend; x += step)
		{
			float z0=*pz;
			float dzx=pz[1]-*pz;
			float dzy=pz[maxbuffer]-*pz;
			Vect3f n(-dzx,-dzy,step);
			n.normalize();

			*tx = ColorByNormalWORD(n);
			tx++;
			pz++;
		}
		texture += pitch;
	}
}

void sBumpTile::CalcTexture(cTileMap *TileMap)
{
	init_texture=true;
	int tilex = TileMap->tileSize().x;
	int tiley = TileMap->tileSize().y;
	int xStart = tile_pos.x * tilex;
	int yStart = tile_pos.y * tiley;
	int xFinish = xStart + tilex;
	int yFinish = yStart + tiley;

	D3DLOCKED_RECT* texRect = TexPools.LockPage(tex);
	int dd = 1 << lod_texture;

	D3DFORMAT texformat=TexPools.getTexFormat(tex);
	if(texformat==D3DFMT_A8R8G8B8 || texformat==D3DFMT_X8R8G8B8)
		vMap.getTileColor32Layer((unsigned char*)texRect->pBits,texRect->Pitch,
			xStart -texture_border*dd, yStart -texture_border*dd,
			xFinish+texture_border*dd, yFinish+texture_border*dd,dd);
	else if(texformat==D3DFMT_R5G6B5)
		vMap.getTileColor16Layer((unsigned char*)texRect->pBits,texRect->Pitch,
			xStart -texture_border*dd, yStart -texture_border*dd,
			xFinish+texture_border*dd, yFinish+texture_border*dd,dd);
	else 
		xassert(0&& "Not support bumpTile texture format!");

	TexPools.UnlockPage(tex);

	if(normal.IsInitialized()){//Get normal 
		D3DLOCKED_RECT *texRect=TexPools.LockPage(normal);
		int xstart=xStart -texture_border*dd;
		int xend=xFinish+texture_border*dd;
		int ystart=yStart -texture_border*dd;
		int yend=yFinish+texture_border*dd;
		int step=dd;
		char* texture=(char*)texRect->pBits;
		int pitch=texRect->Pitch;
		//GetNormalArray(TileMap->GetTerra(),texture,pitch,xstart,ystart,xend,yend,lod_texture);
		GetNormalArrayShort(texture,pitch,xstart,ystart,xend,yend,lod_texture);
		TexPools.UnlockPage(normal);
	}
}


//////////////////////////////////////////////////////////////////////////////

void cTileMapRender::UpdateLine(int dy,int dx)
{
	vMap.region().lock();

	for (int y = 0; y < dy; y++)
		for (int x = 0; x < dx; x++){
			sTile& tile = tileMap_->GetTile(x, y);
			if(tile.bumpTileID < 0)
				continue;
			sBumpTile* bumpTile = bumpTiles[tile.bumpTileID];
			bool update_line = false;
			for(int i = 0; i < U_ALL; i++){
				int xx = x + u_to_pos[i].x;
				int yy = y + u_to_pos[i].y;
				if(xx>=0 && xx<dx && yy>=0 && yy<dy){
					char lod = vis_lod[xx+yy*dx];
					char& cur_lod = bumpTile->border_lod[i];
					if(cur_lod != lod && lod!=-1){
						cur_lod=lod;
						update_line=true;
					}
				}
			}

			if((!bumpTile->init_vertex) || tile.getAttribute(ATTRTILE_UPDATE_VERTEX) || update_line){
				bumpTile->CalcVertex(tileMap_);
				tile.clearAttribute(ATTRTILE_UPDATE_VERTEX);
			}
		}
	vMap.region().unlock();
}


void cTileMapRender::SetTilesToRender(Camera* camera)
{
	start_timer_auto();
//	ClippingMesh(tileMap_->zMax()).calcVisMap(camera, tileMap_->GetTileNumber(), tileMap_->GetTileSize(), visMap, true);
	BYTE* visMap;
	Vect2i size;
	int shl;
	camera->GetTestGrid(visMap, size, shl);

	//drawTestGrid(cTileMapRender::visMap, tileMap_->GetTileNumber());
	//drawTestGrid(visMap, tileMap_->GetTileNumber());

	TexPools.ClearRenderList();

	// create/update tiles' vertices/textures
	float lod_focus = Option_MapLevel*camera->GetFocusViewPort().x;
	float DistLevelDetail[TILEMAP_LOD] = { // was: 1,2,4,6
		0.5f * lod_focus,
		1.5f * lod_focus,
		3.0f * lod_focus,
		6.0f * lod_focus,
		12.0f* lod_focus
	};

	Vect3f coord(0,0,0);
	Vect3f dcoord(
		tileMap_->tileSize().x * tileMap_->GetTileScale().x,
		tileMap_->tileSize().y * tileMap_->GetTileScale().y,
		255 * tileMap_->GetTileScale().z);

	int dx = tileMap_->tileNumber().x;
	int dy = tileMap_->tileNumber().y;

	for(int y = 0; y < dy; y++, coord.x = 0, coord.y += dcoord.y)
		for(int x = 0; x < dx; x++, coord.x += dcoord.x){
			if(visMap[x+y*dx]){ //if(camera->TestVisible(coord, coord+dcoord))
				// process visible tile
				sTile& Tile = tileMap_->GetTile(x, y);
				int& bumpTileID = Tile.bumpTileID;

				// calc LOD считается всегда по отношению к прямой камере для 
				// избежания случая 2 разных LOD в одно время 
				float dist = camera->GetPos().distance(coord + dcoord/2);
				int iLod;
				for(iLod = 0; iLod < TILEMAP_LOD; iLod++)
					if(dist < DistLevelDetail[iLod]) 
						break;
				if(iLod >= TILEMAP_LOD) 
					iLod = TILEMAP_LOD-1;
				int lod_vertex = bumpGeoScale[iLod];
				vis_lod[x+y*dx] = lod_vertex;

				// create/update render tile
				if(bumpTileValid(bumpTileID)){
					if(bumpTiles[bumpTileID]->lod_vertex != lod_vertex){
						sBumpTile* bumpTile = bumpTiles[bumpTileID];
						bumpTile->InitVertex(tileMap_, lod_vertex);
						if(bumpTile->lod_texture!=bumpTexScale[iLod])
							bumpTile->InitTexture(tileMap_, bumpTexScale[iLod]);

						// LOD changed, free old tile and allocate new
						//bumpTileFree(bumpTileID);
						//bumpTileID = bumpTileAlloc(iLod,x,y);
					}
				} 
				else 
					bumpTileID = bumpTileAlloc(iLod,x,y);

				sBumpTile* bumpTile = bumpTiles[bumpTileID];

				// set flags, add tile to list
				Tile.setAttribute(ATTRTILE_DRAW_VERTEX);
				//if(!shadow){
					if(!bumpTile->isTexture())
						bumpTile->InitTexture(tileMap_, bumpTexScale[iLod]);

					Tile.setAttribute(ATTRTILE_DRAW_TEXTURE);
					if((!bumpTile->init_texture) || Tile.getAttribute(ATTRTILE_UPDATE_TEXTURE)){
						bumpTile->CalcTexture(tileMap_);
						Tile.clearAttribute(ATTRTILE_UPDATE_TEXTURE);
					}

					TexPools.AddToRenderList(bumpTile->tex,bumpTile);
				//}
				//else
					TexPools.shadowTiles.push_back(bumpTile);
			}
			else
				vis_lod[x+y*dx]=-1;
		}
}

void cTileMapRender::DrawBump(Camera* camera,eBlendMode MatMode,bool shadow,bool zbuffer)
{
	start_timer_auto();

	//Camera* pShadowMapCamera=camera->FindChildCamera(ATTRCAMERA_SHADOWMAP);
	Camera* pNormalCamera=camera->GetRoot();
	gb_RenderDevice3D->SetNoMaterial(MatMode,MatXf::ID);
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_anisotropic);

	gb_RenderDevice3D->SetVertexDeclaration(sBumpTile::GetVertexDeclaration());

	gb_RenderDevice3D->dtAdvance->SetReflection(camera->getAttribute(ATTRCAMERA_REFLECTION));
	gb_RenderDevice3D->dtAdvance->BeginDrawTilemap(tileMap_);
	//bool zbuffer = camera->getAttribute(ATTRCAMERA_ZBUFFER);
	if(shadow)
		gb_RenderDevice3D->dtAdvance->SetMaterialTilemapShadow();
	else if(zbuffer){
		gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,FALSE);
		gb_RenderDevice3D->dtAdvance->SetMaterialTilemapZBuffer();
	}
	else{
//		gb_RenderDevice3D->dtAdvance->SetMaterialTilemap();
	}

	int zReflection = 0;
	int counterReflection = 0;
	int lastPool = -1;
	bool drawBump = !shadow && !zbuffer;
	int poolSize = drawBump ? TexPools.bumpTexPools.size() : 1;
	for(int i = 0; i < poolSize; i++){
		sTilemapTexturePool* curpool = drawBump ? TexPools.bumpTexPools[i] : 0;
		vector<sBumpTile*>& tileRenderList = drawBump ? curpool->tileRenderList : TexPools.shadowTiles;
		vector<sBumpTile*>::iterator it_tile;
		FOR_EACH(tileRenderList, it_tile){
			sBumpTile* bumpTile = *it_tile;
			if(drawBump){
				IDirect3DTexture9* pBump = bumpTile->normal.IsInitialized() ? TexPools.bumpTexPools[bumpTile->normal.pool]->GetTexture() : 0;
				gb_RenderDevice3D->dtAdvance->NextTile(curpool->GetTexture(),bumpTile->uv_base,bumpTile->uv_step,
														pBump, bumpTile->uv_base_bump,bumpTile->uv_step);
			}

			int lod_vertex = bumpTile->lod_vertex;
			int nVertex = bumpNumVertices(lod_vertex);

			if(lastPool != bumpTile->vtx.pool){
				tileMapRender->vertexPool()->Select(bumpTile->vtx);
				lastPool = bumpTile->vtx.pool;
			}

			int nPolygon=0;
			DWORD pageSize=tileMapRender->vertexPool()->GetPageSize(bumpTile->vtx);
			DWORD base_vertex_index = pageSize*bumpTile->vtx.page;

			for(int j = 0; j < bumpTile->index.size(); j++){
				sPlayerIB& pib=bumpTile->index[j];

				if(drawBump){ //не оптимально
					bool isIce = tileMap_->setMaterial(pib.material, MatMode);
					if(isIce){
						zReflection += bumpTile->zavr;
						counterReflection++;
					}
				}

				int nIndex, nIndexOffset;
				if(pib.nindex==-1){
					nIndex = bumpNumIndex(lod_vertex);
					nIndexOffset = bumpIndexOffset(lod_vertex);
					gb_RenderDevice3D->SetIndices(tilemapIB);
				}
				else{
					nIndex = pib.nindex;
					nIndexOffset = tileMapRender->indexPool()->GetBaseIndex(pib.index);
					tileMapRender->indexPool()->Select(pib.index);
				}
				nPolygon=nIndex/3;

				RDCALL(gb_RenderDevice3D->D3DDevice_->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
					base_vertex_index, 0, nVertex, nIndexOffset, nPolygon));
			}
	
			gb_RenderDevice3D->NumberTilemapPolygon += nPolygon;
			gb_RenderDevice3D->NumDrawObject++;
			
			if(false)
			if(pNormalCamera==camera){
				int x=bumpTile->tile_pos.x;
				int y=bumpTile->tile_pos.y;
				int xs = tileMap_->tileSize().x;
				int ys = tileMap_->tileSize().y;

				int z=tileMap_->GetTile(x,y).zmax;

				Vect3f v00(xs*x    ,ys*y    ,z),
					   v01(xs*(x+1),ys*y    ,z),
					   v10(xs*x    ,ys*(y+1),z),
					   v11(xs*(x+1),ys*(y+1),z);

				Color4c color(0,0,255,255);
				gb_RenderDevice->DrawLine(v00,v01,color);
				gb_RenderDevice->DrawLine(v01,v11,color);
				gb_RenderDevice->DrawLine(v11,v10,color);
				gb_RenderDevice->DrawLine(v10,v00,color);
			}
		}
	}

	if(counterReflection)
		camera->scene()->setZReflection(float(zReflection)/counterReflection, 0.0025f);

	// restore state
	gb_RenderDevice3D->dtAdvance->EndDrawTilemap();

	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);

	gb_RenderDevice3D->SetVertexShader(0);
	gb_RenderDevice3D->SetPixelShader(0);

	for(int i = 0; i < 6; i++)
		gb_RenderDevice3D->SetTextureBase(i,0);
}

/*
 Ищет граничные точки у полигона в квадрате xmin,ymin,xmin+dx,ymin+dy.

 В Interval xr - включается в интервал. 
 Линия считается "толстой", то есть от точки y до y+1
*/
RENDER_API void EdgeDetection(MultiRegion& region,int xmin,int ymin,int dx,int dy,vector<Vect2i>& edge_point)
{
#define xrr(i) ((i+1)->x0)
	int ymax=ymin+dy;
	int xmax=xmin+dx;

	MultiRegion::Lines& lines=region.lines();
	if(ymin<0)
		ymin=0;
	if(ymax>region.height())
		ymax=region.height();

	for(int y=ymin;y<ymax;y++)
	{
		MultiRegion::Line& line=lines[y];
		MultiRegion::Line& linem=(y-1>=0)?lines[y-1]:line;
		MultiRegion::Line& linep=(y+1<lines.size())?lines[y+1]:line;
	
		//Двигаемся по линии вперед, и смотрим
		//Если сверху и снизу все закрыто - значит не граничная.
		MultiRegion::Line::iterator it,itm,itp;
		it=line.begin();
		itm=linem.begin();
		itp=linep.begin();
		MultiRegion::Line::iterator it_end,itm_end,itp_end;
		it_end=line.end();
		itm_end=linem.end();
		itp_end=linep.end();

		unsigned char type=0,typem=0,typep=0;

		for(int x=xmin;x<xmax;x++)
		{
			while(it!=it_end && it->x0<x)
			{
				type=it->type;
				it++;
			}

			while(itp!=itp_end && itp->x0<x)
			{
				typep=itp->type;
				itp++;
			}

			while(itm!=itm_end && itm->x0<x)
			{
				typem=itm->type;
				itm++;
			}

			if(it==it_end)
				break;

			MultiRegion::Interval& i=*it;
			if(type)//Есть точка
			{
				bool add_up=false,add_down=false;
				if(i.x0==x)
				{
					add_up=true;
				}else
				{
					if(typem!=type)
						add_up=true;
				}

				if(typep!=type)
					add_down=true;

				if(add_up)
					edge_point.push_back(Vect2i(x,y));

//				if(add_down)
//					edge_point.push_back(Vect2i(x,y+1));
			}
		}
	}
#undef xrr
}

void sBumpTile::CalcVertex(cTileMap *TileMap)
{
	start_timer_auto();

	init_vertex=true;
	MultiRegion& region = vMap.region();
	Vect2i pos=tile_pos;

	int tilenumber = TileMap->multiRegionLayersNumber;
	int step=lod_vertex;

	char dd=TILEMAP_SIZE>>step;
	int xstep=1<<step;
	int xstep2=1<<(step-1);
	int xstep4=1<<(step-2);

	int minx=pos.x*TILEMAP_SIZE;
	int miny=pos.y*TILEMAP_SIZE;
	int maxx=minx+TILEMAP_SIZE;
	int maxy=miny+TILEMAP_SIZE;

	int ddv=dd+1;
	VectDelta* points = TileMap->GetTilemapRender()->GetDeltaBuffer();

	vector<vector<sPolygon> >& index=TileMap->GetTilemapRender()->GetIndexBuffer();

	{
		index.resize(tilenumber+1);
		for(int i=0;i<index.size();i++)
			index[i].clear();
	}

	int x,y;

	for(y=0;y<ddv;y++)
		for(x=0;x<ddv;x++)
		{
			VectDelta& p=points[x+y*ddv];
			p.set(x*xstep+minx,y*xstep+miny);
			p.delta.set(0,0);
			p.delta2=INT_MAX;
			p.player=0;
			p.fix=0;

			int xx=min(p.x,maxx-1);
			int yy=min(p.y,maxy-1);

			int player = region.filled(xx,yy)-1;
			xassert(p.player==0);
			xassert(player>=0 && player<index.size());
			p.player=player;
		}

	int fix_out=FixLine(points,ddv,TileMap);

//	1280,1536
// x=19,20
//	if(tile_pos.x==20 && tile_pos.y==23)
//		TestPoint(Vect2i(1280,1536));

//if(columns[player]->filled(xx,yy)) //Эта операция должна ускориться с переходом на мультирегион.
//EdgeDetection - Ускорится, потому как будет в 2 раза меньше точек.
//Добавление в регион с одной стороны ускорится, потому как будет меньше регионов, с другой стороны замедлится, 
// потому как регионы будут более сложной формы, но в среднем должна ускориться.


	int ymin=tile_pos.y*TILEMAP_SIZE;
	int xmin=tile_pos.x*TILEMAP_SIZE;
	vector<Vect2i> edge_point;
	EdgeDetection(region,xmin-xstep,ymin-xstep,TILEMAP_SIZE+xstep*2,TILEMAP_SIZE+xstep*2,edge_point);

	for(vector<Vect2i>::iterator it=edge_point.begin();it!=edge_point.end();++it)
	{
		Vect2i p=*it;

		Vect2i pround;
		VectDelta* pnt=0;

		int dx=p.x-minx,dy=p.y-miny;
		if(!CollapsePoint(pround,dx,dy))
			continue;

		pnt=&points[pround.x+pround.y*ddv];
		if(pnt->fix&VectDelta::FIX_FIXED)
		{
			xassert(0);
			pround.x&=~1;
			pround.y&=~1;
			pnt=&points[pround.x+pround.y*ddv];
			xassert(pnt->fix&(VectDelta::FIX_RIGHT|VectDelta::FIX_BOTTOM));
		}

		int delta2=sqr(pnt->x-p.x)+sqr(pnt->y-p.y);
		if(delta2<pnt->delta2)
		{
			pnt->delta2=delta2;
			pnt->delta=p-*pnt;
			pnt->player=-1;
			if(pnt->fix)
			{
				if(pnt->fix&VectDelta::FIX_RIGHT)
				{
					VectDelta& p=points[pround.x+1+pround.y*ddv];
					xassert(pnt->x==p.x && pnt->y==p.y);
					xassert(p.fix==VectDelta::FIX_FIXED);
					p.copy_no_fix(*pnt);
				}
				if(pnt->fix&VectDelta::FIX_BOTTOM)
				{
					VectDelta& p=points[pround.x+(pround.y+1)*ddv];
					xassert(pnt->x==p.x && pnt->y==p.y);
					xassert(p.fix==VectDelta::FIX_FIXED);
					p.copy_no_fix(*pnt);
				}
			}
		}
	}

	sPolygon poly;
	for(y=0;y<dd;y++)
	for(x=0;x<dd;x++)
	{
		int base=x+y*ddv;
		VectDelta *p00,*p01,*p10,*p11;
		p00=points+base;
		p01=points+(base+1);
		p10=points+(base+ddv);
		p11=points+(base+ddv+1);

		bool no;
		char cur_player_add;

#define X(p) \
		{ \
			if(cur_player_add<0) \
				cur_player_add=p->player; \
			else \
				if(p->player>=0 && cur_player_add!=p->player)\
					no=true;\
		}
		
		if(p01->player==p10->player || p01->player<0 || p10->player<0)
		{
			cur_player_add=-1;
			no=false;
			X(p00);
			X(p01);
			X(p10);

			if(cur_player_add<0 || no)
			{
				int x=(p00->x+p00->delta.x+p01->x+p01->delta.x+p10->x+p10->delta.x)/3;
				int y=(p00->y+p00->delta.y+p01->y+p01->delta.y+p10->y+p10->delta.y)/3;
				cur_player_add=0;
				cur_player_add=region.filled(x,y)-1;
				xassert(cur_player_add>=0 && cur_player_add<index.size());
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base,base+ddv,base+1);
			index[cur_player_add].push_back(poly);

			cur_player_add=-1;
			no=false;
			X(p01);
			X(p10);
			X(p11);

			if(cur_player_add<0 || no)
			{
				int x=(p01->x+p01->delta.x+p10->x+p10->delta.x+p11->x+p11->delta.x)/3;
				int y=(p01->y+p01->delta.y+p10->y+p10->delta.y+p11->y+p11->delta.y)/3;
				cur_player_add=0;
				cur_player_add = region.filled(x,y)-1;
				xassert(cur_player_add>=0 && cur_player_add<index.size());
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base+ddv,base+ddv+1,base+1);
			index[cur_player_add].push_back(poly);
		}else
		{
			cur_player_add=-1;
			no=false;
			X(p00);
			X(p01);
			X(p11);


			if(cur_player_add<0 || no)
			{
				int x=(p00->x+p00->delta.x+p01->x+p01->delta.x+p11->x+p11->delta.x)/3;
				int y=(p00->y+p00->delta.y+p01->y+p01->delta.y+p11->y+p11->delta.y)/3;
				cur_player_add=0;
				cur_player_add = region.filled(x,y)-1;
				xassert(cur_player_add>=0 && cur_player_add<index.size());
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base,base+ddv+1,base+1);
			index[cur_player_add].push_back(poly);

			cur_player_add=-1;
			no=false;
			X(p00);
			X(p10);
			X(p11);

			if(cur_player_add<0 || no)
			{
				int x=(p00->x+p00->delta.x+p10->x+p10->delta.x+p11->x+p11->delta.x)/3;
				int y=(p00->y+p00->delta.y+p10->y+p10->delta.y+p11->y+p11->delta.y)/3;
				cur_player_add=0;
				cur_player_add = region.filled(x,y)-1;
				xassert(cur_player_add>=0 && cur_player_add<index.size());
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base,base+ddv,base+ddv+1);
			index[cur_player_add].push_back(poly);
		}
	}
#undef X

	/////////////////////set vertex buffer
	int is = TileMap->tileSize().x; // tile size in vMap units
	int js = TileMap->tileSize().y;
	int xMap = is * TileMap->tileNumber().x; // vMap size
	int yMap = js * TileMap->tileNumber().y;
	int xStart = tile_pos.x * is;
	int yStart = tile_pos.y * js;
	int xFinish = xStart + is;
	int yFinish = yStart + js;

	uv_base.x=tex_uvStart.x-xStart*uv_step.x;
	uv_base.y=tex_uvStart.y-yStart*uv_step.y;
	uv_base_bump.x=tex_uvStartBump.x-xStart*uv_step.x;
	uv_base_bump.y=tex_uvStartBump.y-yStart*uv_step.y;

	shortVertexXYZD* vb = (shortVertexXYZD*)tileMapRender->vertexPool()->LockPage(vtx);

	for(y=0;y<ddv;y++)
		for(x=0;x<ddv;x++){
			int i=x+y*ddv;
			VectDelta& p=points[i];
			int xx=p.x+p.delta.x;
			int yy=p.y+p.delta.y;

			vb->pos.x = xx;
			vb->pos.y = yy;
			vb->pos.z = round(TileMap->getZ(xx,yy)*64);
			vb->pos.w = 1;

			Vect3f normal;
			TileMap->getNormal(xx,yy,xstep,normal);
			vb->diffuse.RGBA()=ColorByNormalRGBA(normal);

			vb++;
		}

	tileMapRender->vertexPool()->UnlockPage(vtx);

	////////////////////set index buffer
	DeleteIndex();

	int num_non_empty=0;
	int one_player=0;
	for(int i=0;i<index.size();i++)
	{
		if(index[i].size()>0)
		{
			num_non_empty++;
			one_player=i;
		}
	}

	if(num_non_empty<=1)
	{
		sBumpTile::index.resize(1);
		sBumpTile::index[0].material = one_player;
		sBumpTile::index[0].nindex=-1;
	}else
	{
		int sum_index=0;
		sBumpTile::index.resize(num_non_empty);
		int cur=0;
		for(int i=0;i<index.size();i++)
		if(index[i].size()>0)
		{
			int num=index[i].size()*3;
			sum_index+=num;
			int num2=Power2up(num);
			tileMapRender->indexPool()->CreatePage(sBumpTile::index[cur].index,num2);

			sPolygon* p=tileMapRender->indexPool()->LockPage(sBumpTile::index[cur].index);
			memcpy(p,&(index[i][0]),num*sizeof(WORD));
			tileMapRender->indexPool()->UnlockPage(sBumpTile::index[cur].index);

			sBumpTile::index[cur].nindex = num;
			sBumpTile::index[cur].material = i;
			cur++;
		}

		int lod_index=TileMap->GetTilemapRender()->bumpNumIndex(lod_vertex);
		xassert(lod_index==sum_index);
	}
}

int sBumpTile::FixLine(VectDelta* points,int ddv,cTileMap *TileMap)
{
	int fix_out=0;
	int dmax=ddv*ddv;
	int dv=ddv-1;
	for(int side=0;side<4;side++)
	{
		if(border_lod[side]<=lod_vertex)
			continue;
//		VISASSERT(border_lod[side]==LOD+1);

		fix_out|=1<<side;
		
		VectDelta* cur;
		int delta;
		char fix=0;
		switch(side)
		{
		case U_LEFT:
			cur=points;
			delta=ddv;
			fix=VectDelta::FIX_BOTTOM;
			break;
		case U_RIGHT:
			cur=points+ddv-1;
			delta=ddv;
			fix=VectDelta::FIX_BOTTOM;
			points[ddv*ddv-1].fix|=VectDelta::FIX_FIXEMPTY;
			break;
		case U_TOP:
			cur=points;
			delta=1;
			fix=VectDelta::FIX_RIGHT;
			break;
		case U_BOTTOM:
			cur=points+ddv*(ddv-1);
			delta=1;
			fix=VectDelta::FIX_RIGHT;
			points[ddv*ddv-1].fix|=VectDelta::FIX_FIXEMPTY;
			break;
		}

		for(int i=0;i<dv;i+=2,cur+=2*delta)
		{
//			VISASSERT(cur-points<dmax);
			VectDelta* p=cur+delta;
//			VISASSERT(p-points<dmax);
			p->copy_no_fix(*cur);
			//*p=*cur;
			xassert(p->fix==0);
			cur->fix|=fix;
			p->fix|=VectDelta::FIX_FIXED;
		}
	}

	return fix_out;
}

////////////////////////////////////////////////////
//		Collapse point
////////////////////////////////////////////////////
U_E* InitPosToU();
static U_E pos_to_u[5*5];
static U_E* pos_to_u_offset=InitPosToU();
static U_E move_table[U_ALL+1][4];

U_E GetPosToU(int x,int y)
{
//	xassert(x>=-2 && x<=2);
//	xassert(y>=-2 && y<=2);
//	return pos_to_u[x+2+(y+2)*5];
	return pos_to_u_offset[x+y*5];
}

U_E Move(U_E move_to, U_E what)
{
	Vect2i& m=u_to_pos[move_to];
	Vect2i& w=u_to_pos[what];
	int x=m.x+w.x,y=m.y+w.y;
	//return GetPosToU(x,y);
	return pos_to_u_offset[x+y*5];
}

U_E* InitPosToU()
{
	for(int i=0;i<=U_ALL;i++)
	{
		Vect2i& p=u_to_pos[i];
		pos_to_u[p.x+2+(p.y+2)*5]=(U_E)i;
	}

	for(int x=-2;x<=+2;x++)
	for(int y=-2;y<=+2;y++)
	{
		int xx=clamp(x,-1,+1);
		int yy=clamp(y,-1,+1);
		pos_to_u[x+2+(y+2)*5]=pos_to_u[xx+2+(yy+2)*5];
	}

	pos_to_u_offset=pos_to_u+2+2*5;

	for(int move_to=0;move_to<=U_ALL;move_to++)
	{
		for(int what=0;what<4;what++)
			move_table[move_to][what]=Move((U_E)move_to,(U_E)what);
	}

	return pos_to_u_offset;
}


//Как переделать. Если точка снаружи, проверяем тайл.
//Генерим border_lod для нее ищем точку, а потом сдвигаем обратно.

enum E_COLLAPSE
{
	E_OUT=0,
	E_IN,
	E_BORDER
};

E_COLLAPSE CollapsePointIn(Vect2i& pround,int dx,int dy,int LOD,char* border_lod)
{
	int step=LOD;
	int xstep2=1<<(step-1);
	char dd=TILEMAP_SIZE>>step;
	pround.x=(dx+xstep2)>>step;
	pround.y=(dy+xstep2)>>step;

	if(pround.x>0 && pround.x<dd && 
		pround.y>0 && pround.y<dd)//Точка внктри тайла и не соприкасается с внешними.
	{
		return E_IN;
	}

	//Граничные точки.
	//Если точка внутри, смотрим с какими граничит по осям x и y, и округляем.
	if(dx>=0 && dx<=TILEMAP_SIZE && dy>=0 && dy<=TILEMAP_SIZE)
	{
		int xstep=1<<step;
		if(pround.x==0)
		{
			if(border_lod[U_LEFT]>LOD)
				pround.y=((dy+xstep)>>(step+1))<<1;
			return E_BORDER;
		}
		if(pround.x==dd)
		{
			if(border_lod[U_RIGHT]>LOD)
				pround.y=((dy+xstep)>>(step+1))<<1;
			return E_BORDER;
		}

		if(pround.y==0)
		{
			if(border_lod[U_TOP]>LOD)
				pround.x=((dx+xstep)>>(step+1))<<1;
			return E_BORDER;
		}
		if(pround.y==dd)
		{
			if(border_lod[U_BOTTOM]>LOD)
				pround.x=((dx+xstep)>>(step+1))<<1;
			return E_BORDER;
		}
		xassert(0);
	}
	
	return E_OUT;
}

bool sBumpTile::CollapsePoint(Vect2i& pround,int dx,int dy)
{
	if(CollapsePointIn(pround,dx,dy,lod_vertex,border_lod))
	{
		return true;
	}

	//Если точка снаружи, смотрим в какой тайл она попала, 
	//проверяем, граничная ли она, если не граничная - false,
	//если граничная, приводим к границе.
	int side=-1;
	if(dy<0)
	{
		if(dx<0)
			side=U_LEFT_TOP;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT_TOP;
		else
			side=U_TOP;
	}else
	if(dy>TILEMAP_SIZE)
	{
		if(dx<0)
			side=U_LEFT_BOTTOM;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT_BOTTOM;
		else
			side=U_BOTTOM;
	}else
	{
		if(dx<0)
			side=U_LEFT;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT;
		else
			xassert(0);
	}
//	xassert(side>=0);

	char side_border_lod[4];//Для уменьшения количества расчетов.
	char side_lod=border_lod[side];

	for(int u=0;u<4;u++)
	{
		//U_E u_in=Move((U_E)side,(U_E)u);
		U_E u_in=move_table[side][u];
		int lod_from;
		if(u_in==U_ALL)
			lod_from=lod_vertex;
		else
			lod_from=border_lod[u_in];

		side_border_lod[u]=lod_from;
	}


	Vect2i& offset=u_to_pos[side];
	int side_dx=dx-offset.x*TILEMAP_SIZE;
	int side_dy=dy-offset.y*TILEMAP_SIZE;

	if(CollapsePointIn(pround,side_dx,side_dy,side_lod,side_border_lod)!=E_BORDER)
	{
		return false;
	}

	int side_step=side_lod;
	char side_dd=TILEMAP_SIZE>>side_step;

	switch(side)
	{
	case U_LEFT:
		if(!(pround.x==side_dd))
			return false;
		break;
	case U_RIGHT:
		if(!(pround.x==0))
			return false;
		break;
	case U_TOP:
		if(!(pround.y==side_dd))
			return false;
		break;
	case U_BOTTOM:
		if(!(pround.y==0))
			return false;
		break;
	case U_LEFT_TOP:
		if(!(pround.x==side_dd && pround.y==side_dd))
			return false;
		break;
	case U_RIGHT_TOP:
		if(!(pround.x==0 && pround.y==side_dd))
			return false;
		break;
	case U_LEFT_BOTTOM:
		if(!(pround.x==side_dd && pround.y==0))
			return false;
		break;
	case U_RIGHT_BOTTOM:
		if(!(pround.x==0 && pround.y==0))
			return false;
		break;
	}

	pround.x=pround.x<<side_step;
	pround.y=pround.y<<side_step;

	pround.x+=offset.x*TILEMAP_SIZE;
	pround.y+=offset.y*TILEMAP_SIZE;

	{
		//xassert((pround.x>>step<<step)==pround.x);
		//xassert((pround.y>>step<<step)==pround.y);
	}

	int step=lod_vertex;
	pround.x=pround.x>>step;
	pround.y=pround.y>>step;


	char dd=TILEMAP_SIZE>>step;

	if(pround.x>=0 && pround.x<=dd && 
		pround.y>=0 && pround.y<=dd)//Точка внктри тайла и не соприкасается с внешними.
	{
		return true;
	}

	return false;
}

/*
1) Убрать Fixed pipeline.
2) Написать шейдер для Geforce FX c с освещением. С тенями и без.
   4 текстуры - diffuse, light map, bump map, shadow map.
   миниум - diffuse, bump map.
3) Распространить результаты на Radeon 9800, Geforce 3, Radeon 8500.

Варианты вывода.
Два варианта освещением на pixel shader и vertex shader.

Минимальный - без теней, с освещением на vertex shader. 
              Будет работать более менее быстро на всём заявленном оборудовании.

Средний без теней - освещение + light map.

Максимальный - освещение на pixel shader + тени + light map.
               Разные варианты для Geforce FX и Radeon 9800.
			   4 и 16 сэмплов.

Варианты под Geforce 3 и Radeon 8500 делать потом.


Настройки (влияющие на выбор шейдера)-
   Shadow map - on,off
   Normal detail - vertex shader, pixel shader
   Shadow map quality - 2x2, 4x4
*/
