#include "StdAfx.h"
#include "Field.h"
#include "DebugUtil.h"
#include "d3dx9.h"
#include "Render\D3d\D3DRender.h"
#include "Terra\vmap.h"
#include "XMath\SafeMath.h"
#include "Water\Water.h"
#include "Render\Src\cCamera.h"

FieldSource::FieldMoulds FieldSource::fieldMoulds_;

FieldPrm fieldPrm;

FieldPrm::FieldPrm()
{
	smoothFactor = 0.5f;

	evolveIterations = 1;
	timeStep = 0.2f;
	stiffness = 3;
	damping = 1.0005f;
	damping2 = 0.0001f;
	dampingMin = 0.1f;
	spawnFactor = 0.25f;
	deltaPhase = 1.f/2000.f;
}

void FieldPrm::serialize(Archive& ar)
{
	ar.serialize(smoothFactor, "smoothFactor", "smoothFactor");

	ar.serialize(evolveIterations, "evolveIterations", "evolveIterations");
	ar.serialize(timeStep, "timeStep", "timeStep");
	ar.serialize(stiffness, "stiffness", "stiffness");
	ar.serialize(damping, "damping", "damping");
	ar.serialize(damping2, "damping2", "damping2");
	ar.serialize(dampingMin, "dampingMin", "dampingMin");
	ar.serialize(spawnFactor, "spawnFactor", "spawnFactor");
	ar.serialize(deltaPhase, "deltaPhase", "deltaPhase");
}

TileStrip::TileStrip(int xSize, int ySize)
{
	xsize = xSize;
	ysize = ySize;

	gb_RenderDevice3D->CreateIndexBuffer(ib, numIndices());
	sPolygon* pIndex = gb_RenderDevice3D->LockIndexBuffer(ib);
	setIB(pIndex);
	gb_RenderDevice3D->UnlockIndexBuffer(ib);

	pagesize = (xsize+1)*(ysize+1);
	pagenumber = 8;
	curpage = 0;
	gb_RenderDevice3D->CreateVertexBuffer(vb, pagesize*pagenumber, sVertexXYZDT2::declaration, true);
}

TileStrip::~TileStrip()
{
	vb.Destroy();
}

void TileStrip::setIB(sPolygon* pIndex)
{
	#define RIDX(x,y) ((x) + (xsize + 1)*(y))

	WORD *ib = (WORD*)pIndex;

	for(int y = 0; y < ysize; y++)
		for(int x = 0; x < xsize; x++){
			*ib++ = RIDX(x, y);
			*ib++ = RIDX(x, y + 1);
			*ib++ = RIDX(x + 1, y);

			*ib++ = RIDX(x + 1, y);
			*ib++ = RIDX(x, y + 1);
			*ib++ = RIDX(x + 1, y + 1);
		}
	#undef RIDX
	
	xassert(ib - (WORD*)pIndex == numIndices());
}

sVertexXYZDT2* TileStrip::beginDraw()
{
	sVertexXYZDT2* pv;
	if(curpage < pagenumber){
		int size = pagesize*vb.GetVertexSize();
		RDCALL(vb.ptr->p->Lock(curpage*size, size,(void**)&pv,D3DLOCK_NOOVERWRITE));
	}
	else{
		RDCALL(vb.ptr->p->Lock(0,0,(void**)&pv,D3DLOCK_DISCARD));
		curpage = 0;
	}
	return pv;
}

void TileStrip::endDraw()
{
	vb.ptr->p->Unlock();
	gb_RenderDevice3D->SetVertexDeclaration(sVertexXYZDT2::declaration);
	gb_RenderDevice3D->SetIndices(ib);
	gb_RenderDevice3D->SetStreamSource(vb);
	RDCALL(gb_RenderDevice3D->D3DDevice_->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, curpage*pagesize, 0, pagesize, 0, numIndices()-2));
	curpage++;
}

/////////////////////////////////////////////////////////

FieldMould::FieldMould(int radius, float height)
{
	radius_ = radius >> FIELD_SHIFT;
	buffer_ = new FieldCell[sqr(2*radius_ + 1)];
	float radius2 = sqr(radius_);
	FieldCell* cell = buffer_;
	for(int y = -radius_; y <= radius_; y++)
		for(int x = -radius_; x <= radius_; x++, cell++){
			Vect2f v(x, y);
			float norm2 = v.norm2();
			if(norm2 < radius2){
				cell->type = FieldCell::INTERNAL;
				cell->height = height*sqrtf(1.f - norm2/radius2 + FLT_EPS);
				cell->delta = Vect2f::ZERO;
				cell->normal.set(x << FIELD_SHIFT, y << FIELD_SHIFT, cell->height);
				cell->normal.normalize();
			}
			else{
				float norm = sqrtf(norm2);
				if(norm < (float)radius_ + 1.4f){
					cell->type = FieldCell::BORDER;
					cell->height = 0;
					v *= (float)radius_/norm - 1.f;
					cell->delta = v;
				}
				else{
					cell->type = FieldCell::EXTERNAL;
					cell->height = FieldCell::Z0;
					cell->delta = Vect2f::ZERO;
				}
				cell->normal.set(x << FIELD_SHIFT, y << FIELD_SHIFT, 0);
				cell->normal.normalize();
			}
		}
}

FieldMould::~FieldMould()
{
	delete buffer_;
}

/////////////////////////////////////////////////////////

FieldSource::FieldSource(int radius, float height, const Vect2i& position, Color4c color)
{
	FieldMoulds::iterator i = fieldMoulds_.find(radius);
	if(i == fieldMoulds_.end())
		i = fieldMoulds_.insert(FieldMoulds::value_type(radius, new FieldMould(radius, height))).first;
	fieldMould_ = i->second;
	position_ = position;
	color_ = color;
}

/////////////////////////////////////////////////////////

FieldDispatcher::FieldDispatcher(int worldXSize, int worldYSize)
: cUnkObj(0), 
  map_(worldXSize, worldYSize),
  tileStrip_(1 << FIELD_2_TILE_SHIFT, 1 << FIELD_2_TILE_SHIFT),
  tileMap_(worldXSize, worldYSize, 0)
{
	phase_ = 0;
	setAttribute(ATTRCAMERA_REFLECTION);
	SetTexture(0, GetTexLibrary()->GetElement2D("Scripts\\Resource\\Textures\\envmap01.tga"));
	SetTexture(1, GetTexLibrary()->GetElement2D("Scripts\\Resource\\Textures\\tesla01.tga"));
}

void FieldDispatcher::clear()
{
	fieldSources_.clear();
}

void FieldDispatcher::Cell::add(const FieldCell& cell, Color4c colorIn)
{
	type = max(type, cell.type);
	if(type == INTERNAL){
		if(heightInitial < cell.height){
			heightInitial = cell.height;
			int a = color.a;
			color = colorIn;
			color.a = min(a + 6, 255); 
			normal = cell.normal;
			delta = Vect2f::ZERO;
		}
	}
	else{
		heightInitial = 0;
		delta = cell.delta;
		color = Color4c::ZERO;
		normal = cell.normal;
	}
}

void FieldDispatcher::Cell::integrate()
{
	//  Vz *= damping
	//   z += Vz
	float k = fieldPrm.damping - sqr(velocity)*fieldPrm.damping2;
	if(k < fieldPrm.dampingMin)
		k = fieldPrm.dampingMin;
	velocity *= k;
	height += velocity*fieldPrm.timeStep;
	
	heightInitial -= 0.5f;
	color.a = max((int)color.a - 3, 0);
	if(height < Z0){
		type = EXTERNAL;
		color = Color4c::ZERO;
	}
}

void FieldDispatcher::add(const Vect2i& posWorld, const FieldMould& source, Color4c color)
{	
	Vect2i pos = map_.w2m(posWorld);
	int radius = source.radius();
	const FieldCell* cell = source.buffer();
	int y = pos.y - radius;
	if(y < 0){
		cell -= (2*radius + 1)*y;
		y = 0;
	}
	int y1 = min(pos.y + radius, map_.sizeY() - 1);
	for(; y <= y1; y++){
		int x = pos.x - radius;
		if(x < 0){
			cell -= x;
			x = 0;
		}
		int x1 = min(pos.x + radius, map_.sizeX() - 1);
		int x1Delta = pos.x + radius - x1;
		for(; x <= x1; x++, cell++){
			Cell& dest = map_(x, y);
			dest.add(*cell, color);
			short& tile = tileMap_(x >> FIELD_2_TILE_SHIFT, y >> FIELD_2_TILE_SHIFT);
			tile = 300;
		}
		cell += x1Delta;
	}
}

void FieldDispatcher::add(FieldSource& source) 
{ 
	fieldSources_.add(&source); 
}

void FieldDispatcher::remove(FieldSource& source) 
{ 
	fieldSources_.remove(&source); 
}

void FieldDispatcher::refresh()
{
	FieldSources::iterator fi;
	FOR_EACH(fieldSources_, fi){
		FieldSource& source = **fi;
		add(source.position_, *source.fieldMould_, source.color_);
	}

	int maskX = map_.sizeX() - 1;
	int maskY = map_.sizeY() - 1;
	for(int yTile = 0; yTile < tileMap_.sizeY(); yTile++)
		for(int xTile = 0; xTile < tileMap_.sizeX(); xTile++){
			if(!tileMap_(xTile, yTile))
				continue;

			for(int y = yTile << FIELD_2_TILE_SHIFT; y < (yTile + 1) << FIELD_2_TILE_SHIFT; y++)
				for(int x = xTile << FIELD_2_TILE_SHIFT; x < (xTile + 1) << FIELD_2_TILE_SHIFT; x++){
					Cell& cell = map_(x, y);
					if(cell.type == Cell::INTERNAL){
						cell.heightInitial += ((map_(x - 1 & maskX, y).heightInitial + map_(x + 1 & maskX, y).heightInitial 
							+ map_(x, y - 1 & maskY).heightInitial + map_(x, y + 1 & maskY).heightInitial)*0.25f 
							- cell.heightInitial)*fieldPrm.smoothFactor;
					}
				}
			}
}

void FieldDispatcher::Animate(float dt)
{
	start_timer_auto();

	__super::Animate(dt);

	phase_ = fmodFast(phase_ + dt*fieldPrm.deltaPhase, 1.00001f);

	refresh();
	evolveField();
}

void FieldDispatcher::evolveField()
{
	start_timer_auto();

	int maskX = map_.sizeX() - 1;
	int maskY = map_.sizeY() - 1;
	for(int i = 0; i < fieldPrm.evolveIterations; i++){
		//  Heights elastic evolution
		//  Vz -= k_elasticity*(z - z_avr)
		float dzFactor = fieldPrm.stiffness*fieldPrm.timeStep;
		for(int yTile = 0; yTile < tileMap_.sizeY(); yTile++)
			for(int xTile = 0; xTile < tileMap_.sizeX(); xTile++){
				if(!tileMap_(xTile, yTile))
					continue;

				for(int y = yTile << FIELD_2_TILE_SHIFT; y < (yTile + 1) << FIELD_2_TILE_SHIFT; y++)
					for(int x = xTile << FIELD_2_TILE_SHIFT; x < (xTile + 1) << FIELD_2_TILE_SHIFT; x++){
						Cell& cell = map_(x, y);
						if(!cell.type)
							continue;
						float dz = dzFactor*(cell.height - cell.heightInitial);
						cell.velocity -= dz;
						dz *= fieldPrm.spawnFactor;
						map_(x - 1 & maskX, y).velocity += dz;
						map_(x + 1 & maskX, y).velocity += dz;
						map_(x, y - 1 & maskY).velocity += dz;
						map_(x, y + 1 & maskY).velocity += dz;
					}
				}

		//  Vz *= damping
		//   z += Vz
		for(int yTile = 0; yTile < tileMap_.sizeY(); yTile++)
			for(int xTile = 0; xTile < tileMap_.sizeX(); xTile++){
				if(!tileMap_(xTile, yTile))
					continue;

				for(int y = yTile << FIELD_2_TILE_SHIFT; y < (yTile + 1) << FIELD_2_TILE_SHIFT; y++)
					for(int x = xTile << FIELD_2_TILE_SHIFT; x < (xTile + 1) << FIELD_2_TILE_SHIFT; x++){
						Cell& cell = map_(x, y);
						if(cell.type)
							cell.integrate();
					}
			}
	}
}

void FieldDispatcher::PreDraw(Camera* camera)
{
	if(getAttribute(ATTRUNKOBJ_IGNORE)) 
		return;
	camera->Attach(SCENENODE_OBJECTSPECIAL,this);
}

void FieldDispatcher::Draw(Camera* camera)
{
	start_timer_auto();

	xassert(GetTexture(0) && GetTexture(1));

	Vect3f uv[2];
	const Mat3f& mC = camera->GetMatrix().rot();
	uv[0].set(0.5f*mC[0][0],0.5f*mC[0][1],0.5f*mC[0][2]);
	uv[1].set(0.5f*mC[1][0],0.5f*mC[1][1],0.5f*mC[1][2]);

	DWORD AlphaTest = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHATESTENABLE);
	DWORD AlphaRef = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHAREF);
	DWORD zwrite = gb_RenderDevice3D->GetRenderState(D3DRS_ZWRITEENABLE);
	DWORD cullMode = gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
	
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	gb_RenderDevice3D->SetWorldMaterial(ALPHA_ADDBLENDALPHA, MatXf::ID, phase_, GetTexture(0), GetTexture(1), COLOR_ADD, false, true);

	{//Немного не к месту, зато быстро по скорости, для отражений.
		gb_RenderDevice3D->SetTexture(5,water->reflectionTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
	}

	for(int yTile = 0; yTile < tileMap_.sizeY(); yTile++)
		for(int xTile = 0; xTile < tileMap_.sizeX(); xTile++){
			int tileHeight = tileMap_(xTile, yTile);
			if(!tileHeight)
				continue;
			Vect3f boxMin(tileMap_.m2w(Vect2f(xTile, yTile)), 0);
			Vect3f boxMax(tileMap_.m2w(Vect2f(xTile + 1, yTile + 1)), tileHeight);
			if(!camera->TestVisible(boxMin, boxMax))
				continue;
			
			sVertexXYZDT2* pv = tileStrip_.beginDraw();
			int x_begin = xTile << FIELD_2_TILE_SHIFT;
			int y_begin = yTile << FIELD_2_TILE_SHIFT;
			int tile_size = 1 << FIELD_2_TILE_SHIFT;
			for(int y = 0; y <= tile_size; y++)
				for(int x = 0; x <= tile_size; x++){
					sVertexXYZDT2& v = *pv++;
					Vect2f pointMap(x + x_begin, y + y_begin);
					Cell& cell = map_(pointMap);
					pointMap = map_.m2w(pointMap + cell.delta);
					Vect3f point(pointMap, water->GetZFast(pointMap.xi(), pointMap.yi()));
					point.z += cell.height;
					v.pos = point;
					v.diffuse = cell.color;
					const Vect3f& n = cell.normal;
					v.GetTexel().set(n.dot(uv[0]) + 0.5f, n.dot(uv[1]) + 0.5f);
					v.GetTexel2().set((n.y + 1)*0.5f, (n.z + 1)*0.5f - phase_);
				}
				
				tileStrip_.endDraw();
		}
		
	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,zwrite);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,AlphaTest);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHAREF,AlphaRef);
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE, cullMode);
}

void FieldDispatcher::debugDraw(Camera* camera)
{
	gb_RenderDevice->setCamera(camera);
	for(int y = 0; y < map_.sizeY(); y++)
		for(int x = 0; x < map_.sizeX(); x++){
			Cell& cell = map_(x, y);
			if(cell.type == Cell::EXTERNAL)
				continue;
			Vect3f point = To3D(map_.m2w(Vect2f(x, y) + cell.delta));
			Vect3f pointH = point;
			pointH.z += cell.height;
			if(cell.type == Cell::BORDER){
				gb_RenderDevice->DrawLine(point, pointH, cell.delta.norm2() > FLT_EPS ? Color4c::RED : Color4c::WHITE);
				gb_RenderDevice->DrawLine(point, To3D(map_.m2w(Vect2f(x, y))), Color4c::GREEN);
				gb_RenderDevice->DrawLine(pointH, pointH + cell.normal*10, Color4c::YELLOW);
			}
		}

	gb_RenderDevice->FlushPrimitive2D();
}

