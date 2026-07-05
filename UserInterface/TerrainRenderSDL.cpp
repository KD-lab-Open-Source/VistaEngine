#include "stdafx.h"

#include "TerrainRenderSDL.h"

#ifndef _WIN32

#include "Terra/VMAP.H"
#include "Render/inc/IRenderDevice.h"   // gb_RenderDevice
#include "Render/inc/VertexFormat.h"    // sVertexXYZINT1, cSkinVertex
#include "Render/3dx/umath.h"           // sPolygon
#include "Render/src/cCamera.h"         // Camera::matViewProj
#include "Render/SDLRenderDevice.h"     // mesh-pass API
#include "Render/src/TexLibrary.h"      // GetTexLibrary() -> blank BGRA cTexture
#include "Render/src/Texture.h"         // cTexture (Lock/Unlock via device, Release)

#include <string>

// The heightfield grid is built lazily and lives for the process. The vMap<->world
// frame (verified via lldb on the Menu mission): vMap is H_SIZE x V_SIZE fine cells
// (512x512 on Menu); world XY == fine cell (world >> kmGrid = coarse GH index,
// GH_SIZE << kmGrid == H_SIZE), and the world Z of the surface is getZf(x,y) =
// getAlt/vx_fraction (~30 on Menu; the game camera sits at z~60). This is NOT the
// D3D tilemap's round(getZ*64) fixed-point, which is compensated by a world matrix
// there -- getZf gives world units that line up with the camera directly.
namespace {

sPtrVertexBuffer s_vb;
sPtrIndexBuffer  s_ib;
int              s_handle = -1;
cTexture*        s_tex = nullptr;    // baked per-cell surface colour (vMap.clrBuf)
bool             s_failed = false;   // build attempted and failed -> stop retrying
std::string      s_builtWorld;       // vMap world the current mesh was built for

// Bake the terrain's per-fine-cell surface colour into one BGRA texture spanning
// the whole map. The colour is stored per cell in vMap.clrBuf (RGB565), the same
// data the D3D tile renderer paints its tiles from; getTileColor32Layer expands it
// via getColor32() to ARGB DWORDs (0xAARRGGBB == B,G,R,A bytes in memory == exactly
// the device's BGRA staging order), so the surface colour lands correctly. The grid
// is downsampled so the texture stays <= MAX_TEX on a side: maps up to 2048 keep
// full per-cell resolution, larger maps are averaged down (the sampler interpolates).
cTexture* buildTerrainTexture(cSDLRenderDevice* dev, int H, int V)
{
	const int MAX_TEX = 2048;
	int step = 1;
	while((H / step) > MAX_TEX || (V / step) > MAX_TEX) step *= 2;
	const int tw = H / step, th = V / step;

	cTexture* tex = GetTexLibrary()->CreateTexture(tw, th, /*alpha*/false);
	if(!tex) return nullptr;

	int pitch = 0;
	void* px = dev->LockTexture(tex, pitch);   // staging is tightly packed (pitch == tw*4)
	if(!px){ tex->Release(); return nullptr; }
	vMap.getTileColor32Layer((unsigned char*)px, pitch, 0, 0, H, V, step);
	dev->UnlockTexture(tex);                    // staging -> GPU (own copy pass)
	return tex;
}

// Sample every STEP_BASE fine cells (512/4 = 128 quads/axis -> 129x129 = 16641 verts
// on the Menu). The step is doubled as needed below so the vertex count stays under
// 65536 -- campaign maps are larger than the Menu's 512, and a 16-bit index buffer
// can only address 65535 vertices (a 1024 map at step 4 would be 257x257 = 66049).
const int STEP_BASE = 4;

bool buildTerrainMesh(cSDLRenderDevice* dev)
{
	const int H = (int)vMap.H_SIZE;
	const int V = (int)vMap.V_SIZE;
	if(H <= 0 || V <= 0)
		return false;   // heightfield not loaded yet -- caller retries next frame

	// Pick the finest step whose grid fits in 16-bit indices: (H/step+1)*(V/step+1) < 65536.
	int step = STEP_BASE;
	while(((H / step) + 1) * ((V / step) + 1) >= 65536)
		step *= 2;

	const int nx = H / step, ny = V / step;   // grid quads per axis
	const int gw = nx + 1,   gh = ny + 1;     // vertices per axis
	const int vcount = gw * gh;
	const int pcount = nx * ny * 2;           // triangles

	cSkinVertex skin(0, false, false, false); // base format == sVertexXYZINT1, stride 36
	dev->CreateVertexBuffer(s_vb, vcount, skin.GetDeclaration(), 0);
	dev->CreateIndexBuffer(s_ib, pcount, sizeof(sPolygon));

	sVertexXYZINT1* vtx = (sVertexXYZINT1*)dev->LockVertexBuffer(s_vb, false);
	if(!vtx){ dev->DeleteVertexBuffer(s_vb); dev->DeleteIndexBuffer(s_ib); return false; }
	for(int gy = 0; gy < gh; ++gy){
		int y = gy * step; if(y > V - 1) y = V - 1;
		for(int gx = 0; gx < gw; ++gx){
			int x = gx * step; if(x > H - 1) x = H - 1;
			sVertexXYZINT1& v = vtx[gy * gw + gx];
			v.pos.set((float)x, (float)y, vMap.getZf(x, y));
			v.index[0] = v.index[1] = v.index[2] = v.index[3] = 0; // BLENDINDICES unused by the shader
			Vect3f nrm; vMap.getNormal(x, y, nrm);
			v.n = nrm;
			v.uv[0] = (float)x / (float)H;                         // fine cell -> [0,1) across the
			v.uv[1] = (float)y / (float)V;                         // baked surface-colour texture
		}
	}
	dev->UnlockVertexBuffer(s_vb);

	sPolygon* idx = (sPolygon*)dev->LockIndexBuffer(s_ib, false);
	if(!idx){ dev->DeleteVertexBuffer(s_vb); dev->DeleteIndexBuffer(s_ib); return false; }
	int t = 0;
	for(int gy = 0; gy < ny; ++gy)
		for(int gx = 0; gx < nx; ++gx){
			WORD a = (WORD)(gy * gw + gx), b = (WORD)(a + 1);
			WORD c = (WORD)(a + gw),       d = (WORD)(c + 1);
			idx[t++].set(a, c, b);   // CCW winding is irrelevant (cull mode NONE)
			idx[t++].set(b, c, d);
		}
	dev->UnlockIndexBuffer(s_ib);

	s_handle = dev->registerMesh(s_vb, s_ib);   // shares the buffers via CopyAddRef
	if(s_handle < 0){ dev->DeleteVertexBuffer(s_vb); dev->DeleteIndexBuffer(s_ib); return false; }

	// Base surface colour from vMap.clrBuf. A white tint passes the sampled colour
	// through unchanged (shader out = tex.rgb * Tint.rgb * Tint.a); the texels are
	// opaque (a==1), so with the filter pipeline (transparency 2, ONE/1-SRC_ALPHA)
	// terrain fully replaces the backdrop. If the colour bake fails, fall back to the
	// flat warm-earth tint (null texture -> white -> Tint.rgb).
	s_tex = buildTerrainTexture(dev, H, V);
	float white[4] = { 1.f, 1.f, 1.f, 1.f };
	float earth[4] = { 0.55f, 0.40f, 0.22f, 1.f };

	// Directional relief lighting: the per-vertex normal (vMap.getNormal, world
	// space) is already flowing to the fragment shader; feed it a light so slopes
	// shade and the heightfield reads as 3D instead of a flat colour field. dir is
	// *toward* the light — a high sun slanted from the NW; strength 0.45 keeps a
	// 0.55 ambient floor so the baked surface colour is modulated, not crushed.
	Vect3f L(0.35f, 0.45f, 0.82f); L.normalize();
	float light[4] = { L.x, L.y, L.z, 0.45f };
	// depthWrite = true: the terrain is opaque base geometry, so it writes depth. The
	// water sheet (drawn after, depth-write off) then depth-tests against it and gets
	// occluded by hills in front of it -- matching the D3D water path.
	dev->addMeshSubmesh(s_handle, 0, pcount * 3, s_tex, s_tex ? white : earth,
	                    /*transparency*/2, light, /*water*/nullptr, /*depthWrite*/true);
	return true;
}

} // namespace

void renderTerrainSDL(Camera* camera)
{
	if(!camera) return;
	cSDLRenderDevice* dev = dynamic_cast<cSDLRenderDevice*>(gb_RenderDevice);
	if(!dev) return;

	// The mesh is built once per world, but vMap is reloaded in place on every mission
	// change (Menu's flat placeholder -> a campaign map's real heightfield). Keyed on
	// the world name (set by vrtMap::load; empty mid-load -> ignored), tear the stale
	// mesh down and let the build path below reconstruct it for the new terrain.
	std::string world = vMap.getWorldName().c_str();
	if(s_handle >= 0 && !world.empty() && world != s_builtWorld){
		dev->releaseMesh(s_handle);          // drop the device's buffer reference
		dev->DeleteVertexBuffer(s_vb);        // drop ours -> SDL buffers freed
		dev->DeleteIndexBuffer(s_ib);
		if(s_tex){ s_tex->Release(); s_tex = nullptr; }  // ~cTexture -> DeleteTexture
		s_handle = -1;
		s_failed = false;
	}

	if(s_handle < 0){
		if(s_failed) return;
		if(!buildTerrainMesh(dev)){
			// H_SIZE==0 (heightfield not loaded yet) leaves s_vb/s_ib uninitialised and
			// s_handle<0; retry next frame. A genuine build failure latches s_failed.
			if(vMap.H_SIZE != 0) s_failed = true;
			return;
		}
		s_builtWorld = world;   // remember which world this mesh matches
	}

	// The world camera's near plane (30 on the Menu mission) can sit *exactly* at a
	// flat heightfield's view-depth: the Menu terrain is a flat plane at world z==30
	// and the camera looks straight down from z~60, so every terrain vertex lands at
	// view-depth ~30 -- right on (a hair inside) the near plane -- and the whole grid
	// is near-clipped. In a real mission the overhead camera sits well beyond near=30,
	// so terrain projects normally; but to keep the fallback robust (and visible on the
	// Menu) rebuild the projection with a pulled-in near. Depth-write is off in the mesh
	// pass, so the reduced z-precision is immaterial.
	Mat4f proj = camera->matProj;
	if(proj._33 != 0.f && proj._33 != 1.f){
		const float n = -proj._43 / proj._33;        // near = -_43/_33
		const float f = proj._33 * n / (proj._33 - 1.f); // far  = _33*n/(_33-1)
		const float nn = 1.f;                         // pulled-in near
		proj._33 = f / (f - nn);
		proj._43 = -proj._33 * nn;
	}
	Mat4f mvp = camera->matView * proj;               // view then (near-corrected) proj
	dev->setMeshTransform(s_handle, (const float*)&mvp);
}

#else  // _WIN32

void renderTerrainSDL(Camera*) {}

#endif // !_WIN32
