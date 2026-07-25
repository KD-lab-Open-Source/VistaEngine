#include "stdafx.h"
#include "FieldOfView.h"
#include "Render/D3D/D3DRender.h"
#include "Render/src/cCamera.h"
#include "Render/SDLRenderDevice.h"        // sdlRenderDevice(), sdlWorldQuadRenderer()
#include "Render/SDLWorldQuadRenderer.h"   // the quad route the sight sectors ride, and eColorWriteMask
#include "Terra/VMAP.H"
#include "ScanPoly.h"
#include "DebugUtil.h"

FieldOfViewMap::FieldOfViewMap(int worldXSize, int worldYSize)
: cIUnkObj(0),
  map_(worldXSize, worldYSize)
{
	setAttribute(ATTRUNKOBJ_IGNORE_NORMALCAMERA | ATTRCAMERA_SHADOW);

	colors_.push_back(Color4c(0, 0, 106, 100));
	
	radiusFactor_ = 1.1f;
	sectorFactor_ = 1.f;

	for(int y = 0; y < map_.sizeY(); y++)
		for(int x = 0; x < map_.sizeX(); x++)
			map_(x, y).height = vMap.getZGrid(x << MAP2GRID_SHIFT, y << MAP2GRID_SHIFT) + 4;


	texture_ = GetTexLibrary()->CreateTexture(map_.sizeX(), map_.sizeY(), false);
	int pitch;
	BYTE* data = texture_->LockTexture(pitch);
	for(int y = 0; y < map_.sizeY(); y++)
		memset(data + y*pitch, 0, map_.sizeX());
	texture_->UnlockTexture();
}

void FieldOfViewMap::setColors(const Colors& colors)
{
	if(!colors.empty())
		colors_ = colors;
}

void FieldOfViewMap::AddOp::operator()(int x, int y, int c)
{
	if(map_.inside(Vect2i(x, y))){
		Cell& cell = map_(x, y);
		//if(cell.height + 8 < c)
		cell.height = 1024;
	}
}

void FieldOfViewMap::RemoveOp::operator()(int x, int y, int c)
{
	if(map_.inside(Vect2i(x, y))){
		Cell& cell = map_(x, y);
		if(cell.height >= c)
			cell.height = vMap.getZGrid(x << MAP2GRID_SHIFT, y << MAP2GRID_SHIFT);
	}
}

void FieldOfViewMap::add(c3dx* model)
{
	TriangleInfo triangleInfo;
	model->GetTriangleInfo(triangleInfo, TIF_TRIANGLES|TIF_POSITIONS);

	Vect2i points[3];
	int colors[3];
	vector<sPolygon>::iterator ti;
	FOR_EACH(triangleInfo.triangles, ti){
		for(int i = 0; i < 3; i++){
			const Vect3f& pos = triangleInfo.positions[(*ti)[i]];
			points[i] = map_.w2m(Vect2i(pos));
			colors[i] = pos.z;
		}
		
		AddOp addOp(map_);
		scanPolyByPointOp(points, colors, 3, addOp);
	}
}

void FieldOfViewMap::remove(c3dx* model)
{
	TriangleInfo triangleInfo;
	model->GetTriangleInfo(triangleInfo, TIF_TRIANGLES|TIF_POSITIONS);

	Vect2i points[3];
	int colors[3];
	vector<sPolygon>::iterator ti;
	FOR_EACH(triangleInfo.triangles, ti){
		for(int i = 0; i < 3; i++){
			const Vect3f& pos = triangleInfo.positions[(*ti)[i]];
			points[i] = map_.w2m(Vect2i(pos));
			colors[i] = pos.z;
		}
		
		RemoveOp removeOp(map_);
		scanPolyByPointOp(points, colors, 3, removeOp);
	}
}

void FieldOfViewMap::add(const Vect3f& posWorld, float psi, float radius, float sector, int colorIndex)
{
	start_timer_auto();
	xassert(colorIndex < colors_.size());
	xassert(radius > 1 && sector > FLT_EPS);
	
	colorIndex = clamp(colorIndex, 0, colors_.size() - 1);
	
	float factor = 1 << PRECISION - Map::tileSizeShl;
	Vect2f pos2(posWorld);
	pos2 -= Vect2f(Map::tileSize, Map::tileSize);
	Vect2i pos(pos2*factor);
	int zmin = posWorld.z;
	int zmax = posWorld.z + 30;
	radius *= factor*radiusFactor_;

	float R = sector*sectorFactor_/2;
	for(float t = -R; t < R; t += 0.01f){
		float angle = psi + t;
		int y = 100 + round(255*sqrtf(1.f - sqr(t/R) + FLT_EPS));
		// (int): round() is the C library's now and returns a double, which is equally
		// convertible to Vect2i's (int,int) and (float,float) constructors — ambiguous.
		trace(pos, Vect2i((int)-round(radius*sinf(angle)), (int)round(radius*cosf(angle))), zmin, zmax, colorIndex, y);
	}
}

void FieldOfViewMap::trace(const Vect2i& pos, const Vect2i& delta, int zmin, int zmax, int colorIndex, int visibility)
{
	start_timer_auto();

	int dx = delta.x;
	int dy = delta.y;

	int length;
	if(abs(dx) > abs(dy)){
		if(dx > 0){
			length = dx >> PRECISION;
			if(!length)
				return;
			dy /= length;
			dx = 1 << PRECISION;
		}
		else{
			length = -dx >> PRECISION;
			if(!length)
				return;
			dy /= length;
			dx = -(1 << PRECISION);
		}
	}
	else{
		if(dy > 0){
			length = dy >> PRECISION;
			if(!length)
				return;
			dx /= length;
			dy = 1 << PRECISION;
		}
		else{
			length = -dy >> PRECISION;
			if(!length)
				return;
			dx /= length;
			dy = -(1 << PRECISION);
		}
	}

	int x = pos.x;
	int y = pos.y;

	int v = visibility << PRECISION;
	int dv = 0;
	int ddv = (-v)/length/(length + 1);

	while(length > 0){
		int xx = (x + HALF) >> PRECISION;
		int yy = (y + HALF) >> PRECISION;
		if(xx < 0 || xx >= map_.sizeX() || yy < 0 || yy >= map_.sizeY())
			return;
		Cell& cell = map_(xx, yy);
		int z = cell.height;
		if(z > zmax)
			return;
		if(z + 10 >= zmin){
			cell.visibility = clamp(v >> PRECISION, cell.visibility, 255);
			cell.colorIndex = max(cell.colorIndex, colorIndex);
			zmin = max(zmin, z);
		}
		x += dx;
		y += dy;
		length--;
		v += dv;
		dv += ddv;
	}
}

void FieldOfViewMap::Cell::quant()
{
	if(visibility > 32)
		visibility -= 32;
	else{
		visibility = 0;
		colorIndex = 0;
	}
}

void FieldOfViewMap::updateTexture()
{
	start_timer_auto();

	cSDLRenderDevice* dev = sdlRenderDevice();
	if(!dev)
		return;
	Vect4f transform = dev->planarTransform();
	int x0 = max(0, map_.w2m(int(round(transform.x))));
	int y0 = max(0, map_.w2m(int(round(transform.y))));
	int x1 = min(map_.sizeX(), x0 + map_.w2m(int(round(1.f/transform.z))));
	int y1 = min(map_.sizeY(), y0 + map_.w2m(int(round(1.f/transform.w))));

	for(int y = y0 + 1; y < y1 - 1; y++)
		for(int x = x0 + 1; x < x1 - 1; x++){
			Cell& cell = map_(x, y);
			cell.quant();
		}

	int pitch;
	BYTE* data = texture_->LockTexture(pitch);
	for(int y = y0; y < y1; y++){
		BYTE* p = data + y*pitch + x0*4;
		for(int x = x0; x < x1; x++){
			Cell& cell = map_(x, y);
			int visibility = cell.visibility;
			Color4c& color = colors_[cell.colorIndex];
			*p++ = 128 + (color.b*visibility >> 9);
			*p++ = 128 + (color.g*visibility >> 9);
			*p++ = 128 + (color.r*visibility >> 9);
			*p++ = 128 + (color.a*visibility >> 9);
		}
	}
	texture_->UnlockTexture();
}

void FieldOfViewMap::PreDraw(Camera* camera)
{
	if(!getAttribute(ATTRUNKOBJ_IGNORE))
		camera->Attach(SCENENODE_OBJECTFIRST, this);
}

void FieldOfViewMap::Draw(Camera* camera)
{
	// One world-sized quad into the terrain LIGHTMAP, on the planar light camera -- a sibling of
	// FogOfWar::Draw (Render-PORTING.md #10), NOT the fog of war. The sight sectors live in the lightmap's
	// RGB (the tint the terrain shader adds as `light += 2*(lightmap.rgb - 0.5)`), the fog of war
	// in its ALPHA. updateTexture packs them centred on 128 = 0.5 = neutral, so the untraced
	// ground contributes nothing; a traced cell rises above 0.5 and tints with the player colour.
	//
	// ATTRUNKOBJ_IGNORE_NORMALCAMERA keeps us off the scene camera; ATTRCAMERA_SHADOW is what
	// marks a camera the planar light one, as FogOfWar::Draw and cCloudShadow::Draw guard on.
	if(!camera->getAttribute(ATTRCAMERA_SHADOW))
		return;

	SDLWorldQuadRenderer* quad = sdlWorldQuadRenderer();
	if(!quad || !texture_)
		return;
	quad->SetCamera(camera);

	// The collision Render-PORTING.md #10b warned about: the original was ALPHA_BLEND with NO mask, so
	// its alpha (128 + (color.a*visibility >> 9)) would land in the channel the fog of war now
	// owns and read as coverage. Mask to RGB -- exactly as CameraPlanarLight::drawLights does for
	// the light quads -- so only the colour tint lands and the fog channel is left untouched. The
	// mask hides the alpha *result*, not the src-alpha blend factor, so the RGB blend is unchanged
	// from D3D9. The alpha was never meaningfully read anyway: FogOfWar::Draw (sortIndex 10)
	// overwrites it after us (sortIndex 0) when fog is on, and nothing samples it when fog is off.
	quad->SetColorWriteMask(COLOR_WRITE_RGB);

	// ALPHA_BLEND with the BGRA coverage texture updateTexture rewrites each frame: the quad
	// shader's `t * diffuse` with a white diffuse passes it straight through, and the src alpha
	// is the per-cell visibility that fades the sector in and out.
	quad->SetMaterial(ALPHA_BLEND, texture_);

	const int dx = vMap.H_SIZE;
	const int dy = vMap.V_SIZE;
	const Color4c diffuse(255, 255, 255);

	quad->BeginDraw();
	sVertexXYZDT1* v = quad->Get();
	v[0].pos.x=0;  v[0].pos.y=0;  v[0].pos.z=0; v[0].u1()=0; v[0].v1()=0; v[0].diffuse=diffuse;
	v[1].pos.x=0;  v[1].pos.y=dy; v[1].pos.z=0; v[1].u1()=0; v[1].v1()=1; v[1].diffuse=diffuse;
	v[2].pos.x=dx; v[2].pos.y=0;  v[2].pos.z=0; v[2].u1()=1; v[2].v1()=0; v[2].diffuse=diffuse;
	v[3].pos.x=dx; v[3].pos.y=dy; v[3].pos.z=0; v[3].u1()=1; v[3].v1()=1; v[3].diffuse=diffuse;
	quad->EndDraw();

	quad->SetColorWriteMask(COLOR_WRITE_ALL);
}

void FieldOfViewMap::serialize(Archive& ar)
{
	ar.serialize(colors_, "colors", "����� �������� ���������");
	ar.serialize(radiusFactor_, "radiusFactor", "��������� �������");
	ar.serialize(sectorFactor_, "sectorFactor", "��������� �������");
}

void FieldOfViewMap::debugDraw(Camera* camera)
{
	gb_RenderDevice->setCamera(camera);
	for(int y = 0; y < map_.sizeY(); y++)
		for(int x = 0; x < map_.sizeX(); x++){
			Cell& cell = map_(x, y);
			Vect3f point = To3D(map_.m2w(Vect2f(x, y)));
			Vect3f pointH = point;
			pointH.z = cell.height;
			gb_RenderDevice->DrawLine(point, pointH, cell.visibility ? Color4c::RED : Color4c::WHITE);
		}

	gb_RenderDevice->FlushPrimitive2D();
}

