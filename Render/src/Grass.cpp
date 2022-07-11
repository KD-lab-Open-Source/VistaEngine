#include "StdAfxRD.h"
#include "Grass.h"
#include "D3DRender.h"
#include "cCamera.h"
#include "FileImage.h"
#include "Terra\vmap.h"
#include "Scene.h"
#include "Serialization\Serialization.h"
#include "Serialization\ResourceSelector.h"
#include "Terra\vmap.h"
#include "TileMap.h"
#include "VisGeneric.h"
#include "Environment\Environment.h"

unsigned int ColorByNormalRGBA(Vect3f n);

DrawTile::DrawTile()
{
	bladeCount = 0;
}
DrawTile::~DrawTile()
{
	DeleteVertex();
}
void DrawTile::InitVertex(int bladeCount_)
{
	DeleteVertex();
	bladeCount = bladeCount_;
	int vertexCount = bladeCount*4;// 4 вершины на травинку
	//gb_RenderDevice3D->GetVertexPool()->CreatePage(vtx,VertexPoolParameter(vertexCount,GetVertexDeclaration()));
	gb_RenderDevice3D->CreateVertexBuffer(vertexBuffer,vertexCount,GetVertexDeclaration());
	InitIndex(bladeCount_);
}
void DrawTile::InitIndex(int bladeCount)
{
	DeleteIndex();
	int indexCount = bladeCount*2;// 2 полигона на травинку
	gb_RenderDevice3D->CreateIndexBuffer(forwardIndex,indexCount);
	gb_RenderDevice3D->CreateIndexBuffer(backwardIndex,indexCount);
}
void DrawTile::DeleteIndex()
{
	gb_RenderDevice3D->DeleteIndexBuffer(forwardIndex);
	gb_RenderDevice3D->DeleteIndexBuffer(backwardIndex);
}
void DrawTile::DeleteVertex()
{
	gb_RenderDevice3D->DeleteVertexBuffer(vertexBuffer);
	DeleteIndex();
}

void DrawTile::Init(int xpos, int ypos)
{
	tilePos.set(xpos,ypos);
}

BYTE *DrawTile::LockVB()
{
	return (BYTE*)gb_RenderDevice3D->LockVertexBuffer(vertexBuffer);
}

void DrawTile::UnlockVB()
{
	gb_RenderDevice3D->UnlockVertexBuffer(vertexBuffer);
}
sPolygon* DrawTile::LockForwardIB()
{
	return gb_RenderDevice3D->LockIndexBuffer(forwardIndex);
}
sPolygon* DrawTile::LockBackwardIB()
{
	return gb_RenderDevice3D->LockIndexBuffer(backwardIndex);
}
void DrawTile::UnlockForwardIB()
{
	gb_RenderDevice3D->UnlockIndexBuffer(forwardIndex);
}
void DrawTile::UnlockBackwardIB()
{
	gb_RenderDevice3D->UnlockIndexBuffer(backwardIndex);
}

//--------------------------------------------------------------------

GrassMap::GrassMap():BaseGraphObject(0)
{
	grassMap_ = 0;
	textureMap_ = 0;
	vsGrass = 0;
	psGrass = 0;
	psGrassShadow = 0;
	psSkinZBuffer=0;
	time = 0;
	textureCount_ = 7;
	texture_ = 0;
	grassMapName_ = "GrassMap.tga";
	time_ = 0;
	hideDistance_ = 800;
	hideDistance2_ = hideDistance_*hideDistance_;
	invHideDistance2_ = 1/hideDistance2_;
	enbaleShadow_ = true;
	dtime_ = 0;
	enable_ = true;
	oldLighting = true;
	density_ = 1.0f;
	setAttribute(ATTRCAMERA_FLOAT_ZBUFFER);

	vMap.registerUpdateMapClient(this);
}

GrassMap::~GrassMap()
{
	vMap.unregisterUpdateMapClient(this);

	delete [] grassMap_;
	RELEASE(texture_);
	RELEASE(textureMap_);
	delete vsGrass;
	delete psGrass;
	delete psGrassShadow;
	delete psSkinZBuffer;
	drawTileFreeAll();
}

GrassTile& GrassMap::getTile(int x, int y)
{
	xassert(x<tileNumber_.x);
	xassert(y<tileNumber_.y);
	return grassMap_[x+y*tileNumber_.x];
}
void GrassMap::Init(const char* world_path)
{
	world_path_=world_path;
	int hsize=vMap.H_SIZE;
	int vsize=vMap.V_SIZE;

	tileNumber_.set(hsize>>tileShift,vsize>>tileShift);
	grassMapSize_.set(hsize>>grassMapShift,vsize>>grassMapShift);

	grassMap_ = new GrassTile[tileNumber_.x*tileNumber_.y];
	vsGrass = new VSGrass;
	vsGrass->Restore();
	psGrass = new PSGrass;
	psGrass->Restore();
	psSkinZBuffer = new PSSkinZBufferAlpha();
	psSkinZBuffer->Restore();

	if(gb_RenderDevice3D->dtAdvanceOriginal && gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_RADEON9700)
	{
		psGrassShadow = new PSGrassShadow;
		psGrassShadow->Restore();
	}else
	if(gb_RenderDevice3D->dtAdvanceOriginal && gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_GEFORCEFX)
	{
		psGrassShadow = new PSGrassShadowFX;
		psGrassShadow->Restore();
	}
	textureNames_.resize(textureCount_);
	bushHights_.resize(textureCount_,3);
	//textureMap_ = GetTexLibrary()->CreateAlphaTexture(grassMapSize_.x,grassMapSize_.y);
	textureMap_ = GetTexLibrary()->CreateTexture(grassMapSize_.x,grassMapSize_.y,true);
	for(int y=0; y<tileNumber_.y; y++)
	for(int x=0; x<tileNumber_.x; x++)
	{
		GrassTile& tile = getTile(x,y);
		tile.pos.x = x*tileSize;
		tile.pos.y = y*tileSize;
	}
	CreateTexture();
	GenerateGrass();
}
void GrassMap::CreateTexture()
{
	BYTE* ImageData = 0;
	int x,y,byte_per_pixel;
	if (!textureMap_)
		return;
	//if (!LoadTGA(vMap.getTargetName(grassMapName_.c_str()),x,y,ImageData,byte_per_pixel))
	//	return;
	string fullname=world_path_+grassMapName_;
	if (!LoadTGA(fullname.c_str(),x,y,ImageData,byte_per_pixel))
		return;

	if (x!=grassMapSize_.x && y!=grassMapSize_.y)
	{
		xassert(0);
		return;
	}
	if (byte_per_pixel != 4)
		return;
	int Pitch;
	BYTE* out=textureMap_->LockTexture(Pitch);
	for(int yy=0;yy<y;yy++)
	{
		memcpy(out+Pitch*yy,ImageData+yy*x*4,x*4);
	}
	textureMap_->UnlockTexture();
	delete ImageData;
}
void GrassMap::InitTextures()
{
	RELEASE(texture_);
	texture_ = GetTexLibrary()->GetElement3DComplex(textureNames_,false,true);
	//texture_ = GetTexLibrary()->GetElement3D("Resource\\TerrainData\\textures\\G_Tex_Grass_09_02.tga");
	//texture_ = GetTexLibrary()->GetElement3D("grass.dds");
}

void GrassMap::SaveMap()
{
	if (!textureMap_)
		return;
	int bpp=textureMap_->bitsPerPixel();
	xassert(bpp==32);

	int Pitch;
	BYTE* out=textureMap_->LockTexture(Pitch);
//	SaveTga(vMap.getTargetName(grassMapName_.c_str()),grassMapSize_.x,grassMapSize_.y,out,4);
	string fullname=vMap.getTargetName("") + grassMapName_;
	SaveTga(fullname.c_str(),grassMapSize_.x,grassMapSize_.y,out,4);

	textureMap_->UnlockTexture();
}
void GrassMap::ClearGrass()
{
	for(int y=0; y<tileNumber_.y; y++)
		for(int x=0; x<tileNumber_.x; x++)
		{
			GrassTile& tile = getTile(x,y);
			tile.bushes_.clear();
		}
}
void GrassMap::UpdateTilesAfterWrapTime()
{
	for(int y=0; y<tileNumber_.y; y++)
	for(int x=0; x<tileNumber_.x; x++)
	{
		GrassTile& tile = getTile(x,y);
		vector<Bush>::iterator it;
		FOR_EACH(tile.bushes_,it)
		{
			if(time_-it->windPower)
				it->windPower = -1.f;
			else
				it->windPower = -(time_-it->windPower);
		}
	}

	vector<SortedTile>::iterator it;
	FOR_EACH(sortedTile_,it)
	{
		GrassTile* tile = it->tile;
		if (tile->recreate)
			continue;
		DrawTile* drawTile = drawTiles[tile->drawTileID];
		shortVertexGrass* buffer=(shortVertexGrass*)drawTile->LockVB();
		xassert(tile->bushes_.size()==drawTile->bladeCount);
		shortVertexGrass* v = buffer;
		vector<Bush>::iterator it;
		FOR_EACH(tile->bushes_,it)
		{
			v[0].uv2 = v[2].uv2 = it->windPower;
			v+=4;

		}
		drawTile->UnlockVB();
		tile->dtime = 0;
	}

}
void GrassMap::SetDensity(float density)
{
	if(density<FLT_EPS)
	{
		enable_ = false;
		return;
	}
	enable_ = true;
	if(fabs(density-density_)>FLT_EPS)
	{
		density_ = density;
		GenerateGrass();
	}
}

void GrassMap::BuildGrass(Vect2i min, Vect2i size)
{
	int bpp=textureMap_->bitsPerPixel();
	xassert(bpp==32);

	int pitch;
	Color4c* data = (Color4c*)textureMap_->LockTexture(pitch,min,size);
	pitch /= 4;
	vector<int> storedpos;
	for (int y=0; y<size.y; y++)
	{
		Color4c* p = data+y*pitch;
		for (int x=0; x<size.x; x++,p++)
		{
			int texNum = p->r >>textureCountShift;
			if (texNum == 0)
				continue;
			Bush bush;
			int xx = (min.x + x);
			int yy = (min.y + y);
			GrassTile& tile = getTile(xx/8,yy/8);
			tile.dtime = 0;
			int count = (p->a&0xf)+1;
			count = clamp(round(count*density_+.5f),1,count);
			//count = clamp(count,1,density_*16);
			float step = 8.f/count;
			tile.drawTileID = -1;
			tile.initVertex = false;
			unsigned char intensity = p->g;
			unsigned char intensity2 = p->b;
			storedpos.clear();
			for (int i=0; i<count; i++)
			{
				
				int n=0;
				int j=0;
				xassert(storedpos.size()<64);
				do{
					n = graphRnd.fabsRnd()*63;
					for(j=0; j<storedpos.size(); j++)
					{
						if (n == storedpos[j])
							break;
					}
				}while(j<storedpos.size());
				storedpos.push_back(n);
				bush.pos.x = xx/8*tileSize+xx%8*8+n%8;//+i*step+graphRnd.frnd()*4;
				bush.pos.x = clamp(bush.pos.x,0,vMap.H_SIZE - 1);
				bush.pos.y = yy/8*tileSize+yy%8*8+n/8;//+i*step+graphRnd.frnd()*4;
				bush.pos.y = clamp(bush.pos.y,0,vMap.V_SIZE - 1);
				bush.pos.z = vMap.getZ(bush.pos.x,bush.pos.y);
				bush.shift = graphRnd.fabsRnd()*256;
				if (bush.shift==0)
					bush.shift = 1;
				bush.material = texNum-1;
				bush.windPower = -2;
				//bush.intensity = intensity;
				Vect3f normal;
				vMap.getNormal(bush.pos.x,bush.pos.y,normal);
				bush.normal.RGBA() = ColorByNormalRGBA(normal);
				bush.normal.a = intensity;
				bush.pos.w = intensity2;

				//tile.bushesMap_[x%8][y%8].push_back(bush);
				//tile.bushesCount++;
				tile.bushes_.push_back(bush);
				//tile.pos.set(x<<tileShift,y<<tileShift);
			}
		}
	}
	textureMap_->UnlockTexture();

}
void GrassMap::GenerateGrass()
{
	if (!textureMap_ || !texture_)
		return;

	ClearGrass();
	BuildGrass(Vect2i(0,0),grassMapSize_);
	//BuildBuffers(Vect2i(0,0),tileNumber_);
}
void GrassMap::SetGrass(float x, float y, float radius, int texture, int count, bool clear, int intensity,int intensity2)
{
	if (!textureMap_)
		return;
	xassert(count>0 && count <=16);
	int xx = round(x)>>grassMapShift;
	int yy = round(y)>>grassMapShift;
	int rad = round(radius)>>grassMapShift;
	int pix = (texture+1)<<textureCountShift;
	Vect2i min(xx-rad,yy-rad);
	Vect2i max(xx+rad,yy+rad);
	min.x = clamp(min.x,0,grassMapSize_.x);
	min.y = clamp(min.y,0,grassMapSize_.y);
	max.x = clamp(max.x,0,grassMapSize_.x);
	max.y = clamp(max.y,0,grassMapSize_.y);
	Vect2i lock_size = max-min;
	int pitch;
	Color4c* data = (Color4c*)textureMap_->LockTexture(pitch,min,lock_size);
	pitch/=4;
	unsigned char intens = round(255.f/100*intensity);
	unsigned char intens2 = round(255.f/100*intensity2);
	for (int y=0; y<lock_size.y; y++)
	{
		Color4c* p = data+y*pitch;
		for (int x=0; x<lock_size.x; x++,p++)
		{
			Vect2i v(-lock_size.x/2+x,-lock_size.y/2+y);
			if (v.norm2() <= rad*rad)
			{
				if (clear)
					p->r = 0;
				else
				{
					p->r = pix;
					p->g = intens;
					p->b = intens2;
					p->a = count-1;
				}
			}
		}
	}
	textureMap_->UnlockTexture();
	GenerateGrass(min,lock_size);
}
void GrassMap::SetAllGrass(int texture, int intensity,int intensity2)
{
	int pitch;
	int pix = (texture+1)<<textureCountShift;
	Color4c* data = (Color4c*)textureMap_->LockTexture(pitch,Vect2i(0,0),grassMapSize_);
	pitch/=4;
	unsigned char intens = round(255.f/100*intensity);
	unsigned char intens2 = round(255.f/100*intensity2);
	for (int y=0; y<grassMapSize_.y; y++)
	{
		Color4c* p = data+y*pitch;
		for (int x=0; x<grassMapSize_.x; x++,p++)
		{
			if (p->r == pix)
			{
				p->g = intens;
				p->b = intens2;
			}
		}
	}
	textureMap_->UnlockTexture();
	GenerateGrass();
}

void GrassMap::GenerateGrass(Vect2i lock_min, Vect2i lock_size)
{
	if (!textureMap_ || !texture_)
		return;

	int x1 = lock_min.x >> grassMapShift;
	int x2 = ((lock_min.x+lock_size.x) >> grassMapShift)+1;
	int y1 = lock_min.y >> grassMapShift;
	int y2 = ((lock_min.y+lock_size.y) >> grassMapShift)+1;
	x2 = clamp(x2,0,tileNumber_.x);
	y2 = clamp(y2,0,tileNumber_.y);

	xassert(x1>=0&&x1<=tileNumber_.x);
	xassert(x2>=0&&x2<=tileNumber_.x);
	xassert(y1>=0&&y1<=tileNumber_.y);
	xassert(y2>=0&&y2<=tileNumber_.y);

	lock_min.x = x1<<grassMapShift;
	lock_min.y = y1<<grassMapShift;
	lock_size.x = ((x2)<<grassMapShift) - lock_min.x;
	lock_size.y = ((y2)<<grassMapShift) - lock_min.y;
	//ClearGrass();
	for (int y=y1; y<y2; y++)
	for (int x=x1; x<x2; x++)
	{
		GrassTile& tile = getTile(x,y);
		tile.bushes_.clear();
		drawTileFree(tile.drawTileID);
		tile.drawTileID = -1;
		tile.initVertex = false;
	}
	BuildGrass(lock_min,lock_size);
	//BuildBuffers(Vect2i(x1,y1),Vect2i(x2,y2));
}
void GrassMap::updateMap(const Vect2i& pos1, const Vect2i& pos2, UpdateMapType type)
{
	int xmin=pos1.x>>tileShift;
	int ymin=pos1.y>>tileShift;
	int xmax=(pos2.x>>tileShift)+1;
	int ymax=(pos2.y>>tileShift)+1;
	xmax=min(xmax,tileNumber_.x-1);
	ymax=min(ymax,tileNumber_.y-1);
	for(int y=ymin; y<=ymax; y++)
	for(int x=xmin; x<=xmax; x++)
	{
		GrassTile& tile = getTile(x,y);
		if (tile.bushes_.size()==0)
			continue;
		vector<Bush>::iterator it;
		FOR_EACH(tile.bushes_,it)
		{
			it->pos.z = vMap.getZ(it->pos.x,it->pos.y);
		}
		tile.initVertex = false;
	}
}

void GrassMap::Animate(float dt)
{
	time_ += 0.0001f*dt;
	if (time_ > 100000)
	{
		UpdateTilesAfterWrapTime();
		time_ -= 100000;
	}
	dtime_ = dt;

}
void GrassMap::PreDraw(Camera* camera)
{
	start_timer_auto();
	if (!enable_)
		return;
	sortedTile_.clear();
	UpdateVB();
	for(int y=0; y<tileNumber_.y; y++)
	for(int x=0; x<tileNumber_.x; x++)
	{
		GrassTile& tile = getTile(x,y);
		if (tile.recreate)
		{
			for (int i=0; i<tile.bushes_.size(); i++)
				if (tile.bushes_[i].needDelete)
				{
					tile.bushes_.erase(tile.bushes_.begin()+i);
					i--;
				}
			tile.recreate = false;
			drawTileFree(tile.drawTileID);
			tile.drawTileID = -1;
			tile.initVertex = false;
		}
		if (!TestVisible(tile,camera))
		{
			tile.dtime += 0.0001f*dtime_;
			drawTileFree(tile.drawTileID);
			tile.drawTileID = -1;
			tile.initVertex = false;
			continue;
		}
		if (!drawTileValid(tile.drawTileID))
		{
			tile.drawTileID = drawTileAlloc(tile.bushes_.size());
		}
		if (!tile.initVertex)
			CalcVertex(tile);
		SortedTile stile;
		stile.tile = &tile;
		stile.distance = camera->GetPos().distance2(Vect3f(tile.pos.x,tile.pos.y,0));
		sortedTile_.push_back(stile);
	}
	camera->SetGrass(this);
}
DWORD GrassMap::GetVertexSize()
{
	DWORD vertexSize=0;
	for (int i = 0; i < drawTiles.size(); i++)
	{
		if (!drawTiles[i])
			continue;
		DrawTile* drawTile = drawTiles[i];
		vertexSize += drawTile->vertexBuffer.GetVertexSize()*drawTile->bladeCount*4;
	}
	return vertexSize;
}

void GrassMap::CalcVertex(GrassTile& tile)
{
	start_timer_auto();
	xassert(tile.drawTileID >= 0);
	DrawTile* drawTile = drawTiles[tile.drawTileID];
	shortVertexGrass* vbuf=(shortVertexGrass*)drawTile->LockVB();
	sPolygon* fwPoly = drawTile->LockForwardIB();
	sPolygon* bwPoly = drawTile->LockBackwardIB();
	shortVertexGrass* v = vbuf;
	Color4c color;
	bwPoly = bwPoly+(drawTile->bladeCount-1)*2;
	xassert(tile.bushes_.size() == drawTile->bladeCount);
	int nv= 0;
	int szx = 3;
	int szy = 6;
	int coef = 10000;
	for(int i=0; i<drawTile->bladeCount; i++)
	{
		Bush& bush = tile.bushes_[i];
		bush.windPower = (time_-bush.windPower)>1.f?(time_-1.f):bush.windPower;
		bush.vbOffset = int(v-vbuf);
		const sRectangle4f& rt = ((cTextureAviScale*)texture_)->GetFramePos(bush.material);
		float scaleX,scaleY;
		scaleX = scaleY = 1.0f;
		float w = ((cTextureComplex*)texture_)->sizes[bush.material].x;
		float h = ((cTextureComplex*)texture_)->sizes[bush.material].y;
		if (h>w)
		{
			scaleY = float(h)/float(w);
		}else if(w>h)
		{
			scaleX = float(w)/float(h);
		}
		int sx = scaleX*bushHights_[bush.material];
		int sy = scaleY*bushHights_[bush.material];

		color.RGBA() = vMap.getColor32((int)bush.pos.x, (int)bush.pos.y);

		v[0].pos.x=bush.pos.x; v[0].pos.y=bush.pos.y; v[0].pos.z=bush.pos.z+1;v[0].pos.w=1;
		v[0].diffuse.r=color.r;v[0].diffuse.g=color.g;v[0].diffuse.b=color.b;v[0].diffuse.a=bush.shift;
		v[0].n.r = bush.normal.r; v[0].n.g = bush.normal.g; v[0].n.b = bush.normal.b; v[0].n.a = bush.normal.a;
		v[0].uv[0] = rt.min.x*coef; v[0].uv[1] = rt.min.y*coef;	//	(0,0);
		v[0].uv[2] = -sx; v[0].uv[3] = sy*2-1;
		v[0].uv2 = bush.windPower;
		
		//v[0].uv2[0] = bush.shift; v[0].uv2[1] = bush.windPower*1000;
		//v[0].uv2[2] = -3; v[0].uv2[3] = 5;

		v[1].pos.x=bush.pos.x; v[1].pos.y=bush.pos.y; v[1].pos.z=bush.pos.z+1;v[1].pos.w=1;
		v[1].diffuse.r=color.r;v[1].diffuse.g=color.g;v[1].diffuse.b=color.b;v[1].diffuse.a=0;
		v[1].n.r = bush.normal.r; v[1].n.g = bush.normal.g; v[1].n.b = bush.normal.b; v[1].n.a = bush.pos.w;
		v[1].uv[0] = rt.min.x*coef; v[1].uv[1] = rt.max.y*coef;	//	(0,0);
		v[1].uv[2] = -sx; v[1].uv[3] = -1;
		v[1].uv2 = -2;
		
		//v[1].uv2[0] = 0; v[1].uv2[1] = -1000;
		//v[1].uv2[2] = -3; v[1].uv2[3] = -1;

		v[2].pos.x=bush.pos.x; v[2].pos.y=bush.pos.y; v[2].pos.z=bush.pos.z+1;v[2].pos.w=1;
		v[2].diffuse.r=color.r;v[2].diffuse.g=color.g;v[2].diffuse.b=color.b;v[2].diffuse.a=bush.shift;
		v[2].n.r = bush.normal.r; v[2].n.g = bush.normal.g; v[2].n.b = bush.normal.b;  v[2].n.a = bush.normal.a;
		v[2].uv[0] = rt.max.x*coef; v[2].uv[1] = rt.min.y*coef;	//	(0,0);
		v[2].uv[2] = sx; v[2].uv[3] = sy*2-1;
		v[2].uv2 = bush.windPower;

		//v[2].uv2[0] = bush.shift; v[2].uv2[1] = bush.windPower*1000;
		//v[2].uv2[2] = 3; v[2].uv2[3] = 5;

		v[3].pos.x=bush.pos.x; v[3].pos.y=bush.pos.y; v[3].pos.z=bush.pos.z+1;v[3].pos.w=1;
		v[3].diffuse.r=color.r;v[3].diffuse.g=color.g;v[3].diffuse.b=color.b;v[3].diffuse.a=0;
		v[3].n.r = bush.normal.r; v[3].n.g = bush.normal.g; v[3].n.b = bush.normal.b;  v[3].n.a = bush.pos.w;
		v[3].uv[0] = rt.max.x*coef; v[3].uv[1] = rt.max.y*coef;	//	(0,0);
		v[3].uv[2] = sx; v[3].uv[3] = -1;
		v[3].uv2 = -2;

		//v[3].uv2[0] = 0; v[3].uv2[1] = -1000; 
		//v[3].uv2[2] = 3; v[3].uv2[3] = -1;

		bwPoly[0].set(nv,nv+2,nv+1);
		bwPoly[1].set(nv+1,nv+2,nv+3);
		fwPoly[0].set(nv,nv+2,nv+1);
		fwPoly[1].set(nv+1,nv+2,nv+3);
		nv+=4;
		v+=4;
		fwPoly+=2;
		bwPoly-=2;

	}
	drawTile->UnlockBackwardIB();
	drawTile->UnlockForwardIB();
	drawTile->UnlockVB();
	tile.initVertex = true;
}

void GrassMap::UpdateVB()
{
	vector<UpdateTile>::iterator it;
	FOR_EACH(updatedTiles_,it)
	{
		GrassTile* tile = (*it).tile;
		if (tile->recreate||tile->drawTileID == -1)
			continue;
		DrawTile* drawTile = drawTiles[tile->drawTileID];
		shortVertexGrass* buffer=(shortVertexGrass*)drawTile->LockVB();
		xassert(tile->bushes_.size()==drawTile->bladeCount);
		shortVertexGrass* v = buffer;
		vector<Bush*>::iterator itb;
		FOR_EACH((*it).bushes,itb)
		{
			Bush* bush = (*itb);
			if(bush)
				v=buffer + bush->vbOffset;
			v[0].uv2 = v[2].uv2 = bush->windPower;

		}
		drawTile->UnlockVB();
	}
	updatedTiles_.clear();
}
void GrassMap::DownGrass(Vect3f pos, float radius)
{
	Vect2i tmin;
	Vect2i tmax;
	tmin.x = round(pos.x - radius)>>tileShift;
	tmin.y = round(pos.y - radius)>>tileShift;
	tmax.x = round(pos.x + radius)>>tileShift;
	tmax.y = round(pos.y + radius)>>tileShift;
	tmin.x = max(0,tmin.x);
	tmin.y = max(0,tmin.y);
	tmax.x = min(tileNumber_.x-1,tmax.x);
	tmax.y = min(tileNumber_.y-1,tmax.y);
	for(int y=tmin.y; y<=tmax.y; y++)
	for(int x=tmin.x; x<=tmax.x; x++)
	{
		GrassTile& tile = getTile(x,y);
		UpdateTile updTile;
		updTile.tile = &tile;
		for (int i=0;i<tile.bushes_.size(); i++)
		{
			Bush& bush = tile.bushes_[i];
			Vect2i v(bush.pos.x-pos.x,bush.pos.y-pos.y);
			if (v.norm2() <= radius*radius)
			{
				bush.windPower = time_-0.3f;
				updTile.bushes.push_back(&bush);
			}
		}
		if(updTile.bushes.size()>0)
			updatedTiles_.push_back(updTile);
	}

}

void GrassMap::DeleteGrass(Vect3f pos, float radius)
{
	Vect2i tmin;
	Vect2i tmax;
	tmin.x = round(pos.x - radius)>>tileShift;
	tmin.y = round(pos.y - radius)>>tileShift;
	tmax.x = round(pos.x + radius)>>tileShift;
	tmax.y = round(pos.y + radius)>>tileShift;
	tmin.x = max(0,tmin.x);
	tmin.y = max(0,tmin.y);
	tmax.x = min(tileNumber_.x-1,tmax.x);
	tmax.y = min(tileNumber_.y-1,tmax.y);
	bool recreate = false;
	for(int y=tmin.y; y<=tmax.y; y++)
	for(int x=tmin.x; x<=tmax.x; x++)
	{
		GrassTile& tile = getTile(x,y);
		recreate = false;
		for (int i=0;i<tile.bushes_.size(); i++)
		{
			Bush& bush = tile.bushes_[i];
			Vect2i v(bush.pos.x-pos.x,bush.pos.y-pos.y);
			if (v.norm2() <= radius*radius)
			{
				bush.needDelete = true;
				tile.recreate = true;
			}
		}
		//if (recreate)
		//{
		//	//drawTileFree(tile.drawTileID);
		//	tile.drawTileID = -1;
		//	tile.initVertex = false;
		//	tile.recreate = true;
		//}
	}
}

bool GrassMap::TestVisible(GrassTile& tile,Camera* camera)
{
	if (tile.bushes_.size()==0)
		return false;
	if (!camera->TestVisible(tile.pos.x,tile.pos.y))
		return false;
	float dist=camera->GetPos().distance2(Vect3f(tile.pos.x,tile.pos.y,0));
	if (dist>hideDistance2_)
		return false;
	return true;

}

void GrassMap::DrawGrass(eBlendMode mode,Camera* camera)
{
	if (sortedTile_.size()==0)
		return;
	Color4c color(255,255,255,64);
	
	bool is_shadow=camera->IsShadow()&&enbaleShadow_;
	
	gb_RenderDevice3D->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,texture_);
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_anisotropic);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
	DWORD oldAlphaFunc = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHAFUNC);
	gb_RenderDevice3D->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	gb_RenderDevice3D->SetRenderState( D3DRS_ALPHAREF, 100);
	gb_RenderDevice3D->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	gb_RenderDevice3D->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	gb_RenderDevice3D->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);

	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->GetLightMap());
	gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_linear);

	vsGrass->SetOldLighting(oldLighting);
	bool toZbuffer = camera->getAttribute(ATTRCAMERA_ZBUFFER);
	bool toFloatZBuffer = camera->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER);
	vsGrass->Select(time_,invHideDistance2_,scene()->GetTileMap()->GetDiffuse(),toZbuffer||toFloatZBuffer);
	if (toZbuffer||toFloatZBuffer)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
		gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
		psSkinZBuffer->SetSecondOpacity(0);
		psSkinZBuffer->Select();
		//gb_RenderDevice3D->dtAdvance->pPSZBuffer->Select(toFloatZBuffer);
	}else
	if (is_shadow)
	{
		psGrassShadow->SetShadowIntensity(scene()->GetShadowIntensity());
		psGrassShadow->Select();
	}else
	{
		psGrass->Select();
	}
	Vect2f vec1(0.5,0.5);

	vector<SortedTile>::iterator it;
	FOR_EACH(sortedTile_,it)
	{
		GrassTile* tile = it->tile;
		Vect2f vec2(tile->pos.x+tileSize/2 - camera->GetPos().x,tile->pos.y+tileSize/2 - camera->GetPos().y);
		float a = vec2.dot(vec1);
		DrawTile* drawTile = drawTiles[tile->drawTileID];
		if (tile->recreate)
			continue;
		if (a>0)
		{
			gb_RenderDevice3D->DrawIndexedPrimitive(drawTile->vertexBuffer,0,drawTile->bladeCount*4,drawTile->backwardIndex,0,drawTile->bladeCount*2);
		}
		else
		{
			gb_RenderDevice3D->DrawIndexedPrimitive(drawTile->vertexBuffer,0,drawTile->bladeCount*4,drawTile->forwardIndex,0,drawTile->bladeCount*2);
		}

	}
	
	//for(int y=0; y<tileNumber_.y; y++)
	//for(int x=0; x<tileNumber_.x; x++)
	//{
	//	GrassTile& tile = getTile(x,y);
	//	if (!TestVisible(tile,UCamera))
	//		continue;

	//	gb_RenderDevice3D->DrawIndexedPrimitive(tile.vertexBuffer,0,tile.bushes_.size()*4,tile.indexBuffer,0,tile.bushes_.size()*2);
	//}
	gb_RenderDevice3D->SetRenderState( D3DRS_ALPHAFUNC, oldAlphaFunc);

}

void GrassMap::Draw(Camera* camera)
{
	start_timer_auto();
	
	if(!enable_ || debugShowSwitch.grass)
		return;

	stable_sort(sortedTile_.begin(),sortedTile_.end(),TilesSortByRadius());
	//gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_GREATEREQUAL);
	//gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	gb_RenderDevice3D->SetRenderState( RS_CULLMODE, D3DCULL_NONE );
	DWORD colorEnableState = gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
	if (camera->getAttribute(ATTRCAMERA_ZBUFFER)||camera->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA);
	}else
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	//DWORD fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
	//gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,FALSE);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_EQUAL);
	DrawGrass(ALPHA_TEST,camera);
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,colorEnableState);
	//gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_EQUAL);
	//gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,fogenable);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ALPHAREF,128);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
	//DrawGrass(ALPHA_BLEND,UCamera);
	
	//gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
}

void GrassMap::serialize(Archive& ar)
{
	const ResourceSelector::Options textureOpts("*.tga", "Resource\\TerrainData\\Textures");

	xassert(textureNames_.size()==textureCount_);
	ar.serialize(hideDistance_,"hideDistance","Расстояние исчезновения");
	ar.serialize(oldLighting,"oldLighting","Старая модель освещения");

	if(ar.filter(SERIALIZE_WORLD_DATA)){ // пишется только в spg
		ar.openBlock("Textures","Текстуры");
		ar.serialize(ResourceSelector(textureNames_[0], textureOpts),"textureName0","Текстура травы - 1");
		ar.serialize(ResourceSelector(textureNames_[1], textureOpts),"textureName1","Текстура травы - 2");
		ar.serialize(ResourceSelector(textureNames_[2], textureOpts),"textureName2","Текстура травы - 3");
		ar.serialize(ResourceSelector(textureNames_[3], textureOpts),"textureName3","Текстура травы - 4");
		ar.serialize(ResourceSelector(textureNames_[4], textureOpts),"textureName4","Текстура травы - 5");
		ar.serialize(ResourceSelector(textureNames_[5], textureOpts),"textureName5","Текстура травы - 6");
		ar.serialize(ResourceSelector(textureNames_[6], textureOpts),"textureName6","Текстура травы - 7");
		ar.closeBlock();
	}
	ar.openBlock("bushHeights","Размер травы");
		ar.serialize(bushHights_[0],"bushHeight0","Размер травы 0");
		ar.serialize(bushHights_[1],"bushHeight1","Размер травы 1");
		ar.serialize(bushHights_[2],"bushHeight2","Размер травы 2");
		ar.serialize(bushHights_[3],"bushHeight3","Размер травы 3");
		ar.serialize(bushHights_[4],"bushHeight4","Размер травы 4");
		ar.serialize(bushHights_[5],"bushHeight5","Размер травы 5");
		ar.serialize(bushHights_[6],"bushHeight6","Размер травы 6");
	ar.closeBlock();
	hideDistance2_ = hideDistance_*hideDistance_;
	invHideDistance2_ = 1/hideDistance2_;
	if(ar.isInput()){
		InitTextures();
		GenerateGrass();
	}
	if(!ar.isEdit() && ar.isOutput())
		SaveMap();
}

void GrassMap::SetTexture(const char* name, int num)
{
	if (!name)
		return;
	xassert(num<=textureCount_);
	textureNames_[num] = name;
	InitTextures();
	GenerateGrass();
}
const char* GrassMap::GetTextureName(int num)
{
	xassert(num<=textureCount_);
	return textureNames_[num].c_str();
}

int GrassMap::drawTileValid(int id)
{
	return ((id >= 0) && (id < drawTiles.size()) && (drawTiles[id]));
}

void GrassMap::drawTileFree(int id)
{
	if (drawTileValid(id))
	{
		delete drawTiles[id];
		drawTiles[id] = 0;
		//deadDrawTiles.push_back(id);
	}
}
void GrassMap::drawTileDeath()
{
	for (int i = 0; i < deadDrawTiles.size(); i++)
	{
		int tile=deadDrawTiles[i];
		//if (++bumpTiles[tile]->age > 4)
		{
			delete drawTiles[i];
			drawTiles[i] = 0;
			deadDrawTiles.erase(deadDrawTiles.begin() + i);
			i--;
		}
	}
}

void GrassMap::drawTileFreeAll()
{
	for (int i = 0; i < drawTiles.size(); i++)
		if (drawTileValid(i))
		{
			delete drawTiles[i];
			drawTiles[i] = 0;
		}
	drawTiles.clear();
}

int GrassMap::drawTileAlloc(int bladeCount)
{
	int i;
	for (i = 0; i < drawTiles.size(); i++)
		if (!drawTiles[i]) break;
	if (i == drawTiles.size()) drawTiles.push_back(0);
	drawTiles[i] = new DrawTile();
	drawTiles[i]->InitVertex(bladeCount);
	return i;
}
