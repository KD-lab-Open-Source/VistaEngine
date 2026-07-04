#include "stdafx.h"

#include "TerrainRenderSDL.h"

#ifndef _WIN32

#include "Terra/VMAP.H"
#include "Render/inc/IRenderDevice.h"   // gb_RenderDevice
#include "Render/inc/VertexFormat.h"    // sVertexXYZINT1, cSkinVertex
#include "Render/3dx/umath.h"           // sPolygon
#include "Render/src/cCamera.h"         // Camera::matViewProj
#include "Render/SDLRenderDevice.h"     // mesh-pass API

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
bool             s_failed = false;   // build attempted and failed -> stop retrying
std::string      s_builtWorld;       // vMap world the current mesh was built for

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
			v.uv[0] = 0.f; v.uv[1] = 0.f;                          // untextured (white) fill
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

	// Opaque warm-earth fill so terrain reads clearly against the cyan/teal menu art.
	// transparency 2 == the alpha-over (filter) pipeline; null texture -> white, so
	// the fragment output is just Tint.rgb (Tint.a == 1 -> fully opaque).
	float tint[4] = { 0.55f, 0.40f, 0.22f, 1.f };
	dev->addMeshSubmesh(s_handle, 0, pcount * 3, /*tex*/0, tint, /*transparency*/2);
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
