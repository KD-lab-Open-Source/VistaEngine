#include "stdafx.h"

#include "CoastFoamRenderSDL.h"

#ifndef _WIN32

#include "Water/CoastSprites.h"          // cCoastSprites, RenderSprite (drive the real sim)
#include "Environment/Environment.h"     // global 'environment' -> GetCoastSprites()
#include "Render/inc/IRenderDevice.h"    // gb_RenderDevice
#include "Render/src/cCamera.h"          // Camera::matProj / matView
#include "Render/SDLRenderDevice.h"      // cSDLRenderDevice, FoamVertex, submitFoam
#include "Render/src/Texture.h"          // cTexture
#include "Render/src/TileMap.h"          // global 'tileMap' -> visibility-grid params

#include <vector>

namespace {

std::vector<cCoastSprites::RenderSprite> s_sprites;
std::vector<cSDLRenderDevice::FoamVertex> s_verts;

// Push one sprite quad (two triangles) from four corners + their UVs. The corner /
// UV pairing follows the original DrawSimple/MovingCoastSprite triangle strip
// (c0,c1,c2,c3): c0=min.uv, c1=(min.x,max.y), c2=(max.x,min.y), c3=max.uv.
inline void emitQuad(std::vector<cSDLRenderDevice::FoamVertex>& out,
                     const Vect2f& p0, const Vect2f& p1, const Vect2f& p2, const Vect2f& p3,
                     float z, unsigned int col, float u0, float v0, float u1, float v1)
{
	cSDLRenderDevice::FoamVertex c0{ p0.x, p0.y, z, col, u0, v0 };
	cSDLRenderDevice::FoamVertex c1{ p1.x, p1.y, z, col, u0, v1 };
	cSDLRenderDevice::FoamVertex c2{ p2.x, p2.y, z, col, u1, v0 };
	cSDLRenderDevice::FoamVertex c3{ p3.x, p3.y, z, col, u1, v1 };
	out.push_back(c0); out.push_back(c1); out.push_back(c2);
	out.push_back(c2); out.push_back(c1); out.push_back(c3);
}

} // namespace

void renderCoastFoamSDL(Camera* camera, float dtSeconds)
{
	if(!camera) return;
	cSDLRenderDevice* dev = dynamic_cast<cSDLRenderDevice*>(gb_RenderDevice);
	if(!dev) return;
	if(!environment) return;
	cCoastSprites* cs = environment->GetCoastSprites();
	if(!cs) return;

	// Build the camera visibility grid. The original builds it every frame in the scene
	// pre-draw (Scene.cpp: camera->EnableGridTest), which is skipped off-Windows -- so
	// TestVisible(x,y) has no grid and the coast sim would spawn foam at every coast cell
	// map-wide instead of only the on-screen ones. calcVisMap is pure CPU (frustum ->
	// ground raster); the global tilemap it needs is built at world load (Universe.cpp).
	if(tileMap)
		camera->EnableGridTest(tileMap->tileNumber().x, tileMap->tileNumber().y, tileMap->tileSize().x);

	// Drive the real simulation: spawn in the coast band (culled to the visible region),
	// then advance/retire and collect the live sprites.
	float dt_ms = dtSeconds * 1000.f;
	if(dt_ms > 100.f) dt_ms = 100.f;      // clamp after a pause/load hitch
	cs->animateSDL(camera, dt_ms);
	s_sprites.clear();
	cs->collectSprites(s_sprites);
	if(s_sprites.empty()){ dev->submitFoam(nullptr, 0, nullptr, 0, nullptr, nullptr); return; }

	// Faithful textures: the sim loads the mission's bubble atlases (G_Tex_Bubbles_*.avi)
	// through the cache path -- a "stay" atlas for the simple sprites and a "moving" atlas
	// of faint drifting arcs. Each RenderSprite already carries its animation-frame UV
	// (collectSprites -> GetFramePosInt). Pack the verts [stay | moving] so the two atlases
	// draw from one buffer; the moving quad is oriented along the sprite's flow direction
	// (DrawMovingCoastSprite), the simple quad is axis-aligned (DrawSimpleCoastSprite).
	cTexture* texStay = cs->stayTexture();
	cTexture* texMov  = cs->movTexture();

	s_verts.clear();
	s_verts.reserve(s_sprites.size() * 6);

	// Pass 1: simple (stay) sprites -> foamTexA_.
	for(size_t i = 0; i < s_sprites.size(); ++i){
		const cCoastSprites::RenderSprite& s = s_sprites[i];
		if(s.moving) continue;
		unsigned int a = (unsigned int)(s.alpha * 255.f);
		if(a > 255u) a = 255u;
		unsigned int col = (a << 24) | (a << 16) | (a << 8) | a;   // premultiplied white fade
		const float x = s.pos.x, y = s.pos.y, sz = s.size;
		emitQuad(s_verts, Vect2f(x - sz, y + sz), Vect2f(x + sz, y + sz),
		                  Vect2f(x - sz, y - sz), Vect2f(x + sz, y - sz),
		         s.pos.z, col, s.u0, s.v0, s.u1, s.v1);
	}
	const int splitA = (int)s_verts.size();

	// Pass 2: moving sprites -> foamTexB_, quad rotated to the drift direction.
	for(size_t i = 0; i < s_sprites.size(); ++i){
		const cCoastSprites::RenderSprite& s = s_sprites[i];
		if(!s.moving) continue;
		unsigned int a = (unsigned int)(s.alpha * 255.f);
		if(a > 255u) a = 255u;
		unsigned int col = (a << 24) | (a << 16) | (a << 8) | a;
		const float x = s.pos.x, y = s.pos.y, sz = s.size;
		Vect2f dir = s.dir;
		if(dir.norm2() < 1e-8f) dir.set(0.f, 1.f);
		const Vect2f sx(dir.y * sz, -dir.x * sz);   // perpendicular axis
		const Vect2f sy(dir.x * sz,  dir.y * sz);   // along-flow axis
		const Vect2f p(x, y);
		emitQuad(s_verts, p - sx - sy, p - sx + sy, p + sx - sy, p + sx + sy,
		         s.pos.z, col, s.u0, s.v0, s.u1, s.v1);
	}

	// Same near-corrected projection as the terrain/water passes (see TerrainRenderSDL).
	Mat4f proj = camera->matProj;
	if(proj._33 != 0.f && proj._33 != 1.f){
		const float n = -proj._43 / proj._33;
		const float f = proj._33 * n / (proj._33 - 1.f);
		const float nn = 1.f;
		proj._33 = f / (f - nn);
		proj._43 = -proj._33 * nn;
	}
	Mat4f mvp = camera->matView * proj;
	dev->submitFoam(s_verts.data(), (int)s_verts.size(), texStay, splitA, texMov, (const float*)&mvp);
}

#else  // _WIN32

void renderCoastFoamSDL(Camera*, float) {}

#endif // !_WIN32
