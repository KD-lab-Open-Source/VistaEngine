#include "stdafx.h"

#include "WaterRenderSDL.h"

#ifndef _WIN32

#include "Terra/VMAP.H"
#include "Water/Water.h"                // cWater, global 'water'
#include "Render/inc/IRenderDevice.h"   // gb_RenderDevice
#include "Render/inc/VertexFormat.h"    // sVertexXYZINT1, cSkinVertex
#include "Render/3dx/umath.h"           // sPolygon
#include "Render/src/cCamera.h"         // Camera::matProj / matView
#include "Render/SDLRenderDevice.h"     // mesh-pass API
#include "Render/src/TexLibrary.h"      // GetTexLibrary() -> blank BGRA cTexture
#include "Render/src/Texture.h"         // cTexture (Lock/Unlock via device, Release)

#include <string>
#include <algorithm>

// The water surface is a translucent sheet drawn over the terrain wherever the
// water height field says there is water. cWater keeps a coarse per-node grid
// (grid_size = (H_SIZE>>4)+1 x (V_SIZE>>4)+1; one node per 16 fine cells), each
// node carrying an absolute surface height realHeight() and a relative depth
// GetRelativeZ() (metres of water above the ground). A node counts as "water"
// when that relative depth clears the engine's own ignore threshold (~2, the same
// GetRelativeZ > 2 test cWater::UpdateTexture uses to paint the minimap). We build
// a mesh spanning only the water quads and draw it through the SDL mesh pass.
namespace {

sPtrVertexBuffer s_vb;
sPtrIndexBuffer  s_ib;
int              s_handle = -1;
cTexture*        s_tex = nullptr;      // baked per-node depth-opacity sheet (premultiplied)
cTexture*        s_bump0 = nullptr;    // waves.dds  (scrolling wave normal, layer 0)
cTexture*        s_bump1 = nullptr;    // waves1.dds (scrolling wave normal, layer 1)
bool             s_failed = false;     // build attempted, nothing to draw -> stop retrying
std::string      s_builtWorld;         // vMap world the current mesh was built for

// The original water shader (water_linear.psl) samples these two wave-normal DDS
// textures at scrolling UVs; they are the game's real ripple content (default names,
// set in cWater's ctor). Loaded once, shared for the process.
const char* WAVES0 = "Scripts\\Resource\\balmer\\shader\\waves.dds";
const char* WAVES1 = "Scripts\\Resource\\balmer\\shader\\waves1.dds";

const float WATER_MIN_DEPTH = 2.f;     // GetRelativeZ threshold: below this = dry

inline bool isWaterNode(cWater* w, int nx, int ny)
{
	return w->GetRelativeZ(nx, ny) > WATER_MIN_DEPTH;
}

// Bake the water surface into one premultiplied-BGRA texture over the node grid.
// Per node the alpha is the engine's own depth->opacity curve: cWater::CalcColor
// reads opacityBuffer_[clamp(z>>z_shift,0,255)] == opacityGradient_.Get(depth/255).a,
// where depth is GetRelativeZ in world units. Shallow water -> low alpha (terrain
// shows through), deep water -> opaque. Stored premultiplied (rgb*=a) so the mesh
// shader (out.rgb = tex.rgb, out.a = tex.a with a white tint) pairs correctly with
// the (ONE, 1-SRC_ALPHA) filter blend == lerp(terrain, waterColour, a).
cTexture* buildWaterTexture(cSDLRenderDevice* dev, cWater* w, int gx, int gy)
{
	cTexture* tex = GetTexLibrary()->CreateTexture(gx, gy, /*alpha*/true);
	if(!tex) return nullptr;

	int pitch = 0;
	unsigned char* px = (unsigned char*)dev->LockTexture(tex, pitch);
	if(!px){ tex->Release(); return nullptr; }

	// Deep-water colour = the reflected-sky base colour the D3D water shader derives
	// (mission reflection_color * scene light + cur_reflect_sky_color), computed
	// portably by cWater. Replaces the old fixed purple with the real per-mission
	// sky tint; the waves shader perturbs it per-pixel and the per-node opacity below
	// controls how much of it shows over the seabed.
	Color4f base = w->GetReflectedSurfaceColor();
	const float WATER_R = base.r, WATER_G = base.g, WATER_B = base.b;

	// Soft shoreline: fade the shallow near-shore band to transparent so the water
	// blends into the coast instead of meeting the terrain at a hard ~0.7-opaque line
	// (the original softens the edge with its FLOAT_ZBUFFER depth fade). The engine's
	// depth-opacity ramps almost vertically -- ~0.73 already at depth 2 -- so multiply
	// it by smoothstep(0, SHORE_BAND, depth). Water deeper than SHORE_BAND is untouched,
	// preserving the authentic depth-opacity; only the coastal band feathers.
	const float SHORE_BAND = 12.f;
	const KeysColor& grad = w->GetOpacity();
	for(int y = 0; y < gy; ++y){
		unsigned char* row = px + y * pitch;
		for(int x = 0; x < gx; ++x){
			float rz = w->GetRelativeZ(x, y);
			int depth = (int)rz;
			depth = std::max(0, std::min(depth, 255));
			float a = grad.Get(depth / 255.f).a;
			a = std::max(0.f, std::min(a, 1.f));
			float df = std::max(0.f, std::min(rz / SHORE_BAND, 1.f));
			a *= df * df * (3.f - 2.f * df);   // smoothstep(0, SHORE_BAND, rz)
			unsigned char* p = row + x * 4;   // staging is tightly packed BGRA
			p[0] = (unsigned char)(WATER_B * a * 255.f);
			p[1] = (unsigned char)(WATER_G * a * 255.f);
			p[2] = (unsigned char)(WATER_R * a * 255.f);
			p[3] = (unsigned char)(a * 255.f);
		}
	}
	dev->UnlockTexture(tex);
	return tex;
}

bool buildWaterMesh(cSDLRenderDevice* dev)
{
	cWater* w = water;
	if(!w) return false;                // no water object -> caller latches s_failed

	const int gx = w->GetGridSizeX();
	const int gy = w->GetGridSizeY();
	const int shift = w->GetCoordShift();
	const int H = (int)vMap.H_SIZE;
	const int V = (int)vMap.V_SIZE;
	if(gx <= 1 || gy <= 1 || H <= 0 || V <= 0)
		return false;

	// Sample the node grid at a stride that keeps the full vertex grid inside 16-bit
	// indices ((gx/step+1)*(gy/step+1) < 65536). The water grid is already coarse, so
	// step stays 1 up to ~2048 maps and only doubles past that.
	int step = 1;
	while((((gx - 1) / step) + 1) * (((gy - 1) / step) + 1) >= 65536)
		step *= 2;

	const int cols = (gx - 1) / step, rows = (gy - 1) / step;  // quads per axis
	const int gw = cols + 1,          gh = rows + 1;           // vertices per axis
	const int vcount = gw * gh;
	auto nodeX = [&](int ix){ int n = ix * step; return n < gx ? n : gx - 1; };
	auto nodeY = [&](int iy){ int n = iy * step; return n < gy ? n : gy - 1; };

	// First pass: count water quads for the index buffer. A quad is emitted if ANY
	// corner is submerged (not all four), so the mesh reaches right up to the shore;
	// the dry corners carry ~0 baked alpha (see buildWaterTexture's shore-fade), which
	// feathers the water/ground edge instead of cutting it off one node short.
	int nquads = 0;
	for(int iy = 0; iy < rows; ++iy)
		for(int ix = 0; ix < cols; ++ix){
			int x0 = nodeX(ix), x1 = nodeX(ix + 1);
			int y0 = nodeY(iy), y1 = nodeY(iy + 1);
			if(isWaterNode(w, x0, y0) || isWaterNode(w, x1, y0) ||
			   isWaterNode(w, x0, y1) || isWaterNode(w, x1, y1))
				++nquads;
		}

	if(nquads == 0)
		return false;                   // nothing submerged in this mission -> no mesh

	cSkinVertex skin(0, false, false, false);   // base format == sVertexXYZINT1, stride 36
	dev->CreateVertexBuffer(s_vb, vcount, skin.GetDeclaration(), 0);
	dev->CreateIndexBuffer(s_ib, nquads * 2, sizeof(sPolygon));

	sVertexXYZINT1* vtx = (sVertexXYZINT1*)dev->LockVertexBuffer(s_vb, false);
	if(!vtx){ dev->DeleteVertexBuffer(s_vb); dev->DeleteIndexBuffer(s_ib); return false; }
	for(int iy = 0; iy < gh; ++iy)
		for(int ix = 0; ix < gw; ++ix){
			int nx = nodeX(ix), ny = nodeY(iy);
			int wx = nx << shift; if(wx > H - 1) wx = H - 1;
			int wy = ny << shift; if(wy > V - 1) wy = V - 1;
			sVertexXYZINT1& v = vtx[iy * gw + ix];
			v.pos.set((float)wx, (float)wy, w->Get(nx, ny).realHeight());
			v.index[0] = v.index[1] = v.index[2] = v.index[3] = 0;
			v.n.set(0.f, 0.f, 1.f);     // flat sheet (unlit anyway)
			v.uv[0] = (float)nx / (float)(gx - 1);   // node -> baked opacity texel
			v.uv[1] = (float)ny / (float)(gy - 1);
		}
	dev->UnlockVertexBuffer(s_vb);

	// Second pass: emit two triangles for each water quad (matching the count above).
	sPolygon* idx = (sPolygon*)dev->LockIndexBuffer(s_ib, false);
	if(!idx){ dev->DeleteVertexBuffer(s_vb); dev->DeleteIndexBuffer(s_ib); return false; }
	int t = 0;
	for(int iy = 0; iy < rows; ++iy)
		for(int ix = 0; ix < cols; ++ix){
			int x0 = nodeX(ix), x1 = nodeX(ix + 1);
			int y0 = nodeY(iy), y1 = nodeY(iy + 1);
			if(!(isWaterNode(w, x0, y0) || isWaterNode(w, x1, y0) ||
			     isWaterNode(w, x0, y1) || isWaterNode(w, x1, y1)))
				continue;
			WORD a = (WORD)(iy * gw + ix),  b = (WORD)(a + 1);
			WORD c = (WORD)(a + gw),        d = (WORD)(c + 1);
			idx[t++].set(a, c, b);      // cull mode is NONE, winding irrelevant
			idx[t++].set(b, c, d);
		}
	dev->UnlockIndexBuffer(s_ib);

	s_handle = dev->registerMesh(s_vb, s_ib);
	if(s_handle < 0){ dev->DeleteVertexBuffer(s_vb); dev->DeleteIndexBuffer(s_ib); return false; }

	// Depth-opacity sheet: the baked premultiplied texture carries the water colour
	// and per-node alpha, so a white tint passes it through unchanged and the filter
	// pipeline (ONE, 1-SRC_ALPHA) blends it over the terrain as lerp(terrain,water,a).
	// Shallow water is nearly transparent (seabed shows), deep water reads solid.
	// (Depth-write is off in the mesh pass, so water drawn after terrain blends over
	// it; hills between camera and water won't occlude yet -- a later refinement.)
	s_tex = buildWaterTexture(dev, w, gx, gy);
	// Load the game's real wave-normal textures (shared; per-frame scroll + specular are
	// supplied to the device via setWaterRenderState in renderWaterSDL).
	if(!s_bump0) s_bump0 = GetTexLibrary()->GetElement3D(WAVES0);
	if(!s_bump1) s_bump1 = GetTexLibrary()->GetElement3D(WAVES1);

	// The baked premultiplied texture carries the depth-opacity water colour; the water
	// pipeline (selected by water[0] > 0) layers the scrolling bumps + specular on top.
	float white[4] = { 1.f, 1.f, 1.f, 1.f };
	Color4f fb = w->GetReflectedSurfaceColor();
	float fallback[4] = { fb.r, fb.g, fb.b, 0.6f };  // flat reflected-sky tint if the bake fails
	float water[3] = { 1.f, 0.f, 0.f };   // water[0]=1 flags the draw as water
	dev->addMeshSubmesh(s_handle, 0, nquads * 2 * 3, s_tex, s_tex ? white : fallback,
	                    /*transparency*/2, /*light*/nullptr, water);
	return true;
}

} // namespace

void renderWaterSDL(Camera* camera)
{
	if(!camera) return;
	cSDLRenderDevice* dev = dynamic_cast<cSDLRenderDevice*>(gb_RenderDevice);
	if(!dev) return;

	// Rebuild when the world changes (vMap/water are reloaded in place per mission).
	std::string world = vMap.getWorldName().c_str();
	if(s_handle >= 0 && !world.empty() && world != s_builtWorld){
		dev->releaseMesh(s_handle);
		dev->DeleteVertexBuffer(s_vb);
		dev->DeleteIndexBuffer(s_ib);
		if(s_tex){ s_tex->Release(); s_tex = nullptr; }
		s_handle = -1;
		s_failed = false;
	}
	if(!world.empty() && world != s_builtWorld)
		s_failed = false;   // give the new world a fresh build attempt

	if(s_handle < 0){
		if(s_failed) return;
		if(!buildWaterMesh(dev)){
			// H_SIZE==0 (map not loaded yet) -> retry next frame; a genuine "no water in
			// this mission" latches s_failed so we stop rebuilding every frame.
			if(vMap.H_SIZE != 0) s_failed = true;
			return;
		}
		s_builtWorld = world;
	}

	// Same near-corrected projection as the terrain pass (see TerrainRenderSDL).
	Mat4f proj = camera->matProj;
	if(proj._33 != 0.f && proj._33 != 1.f){
		const float n = -proj._43 / proj._33;
		const float f = proj._33 * n / (proj._33 - 1.f);
		const float nn = 1.f;
		proj._33 = f / (f - nn);
		proj._43 = -proj._33 * nn;
	}
	Mat4f mvp = camera->matView * proj;
	dev->setMeshTransform(s_handle, (const float*)&mvp);

	// Per-frame water shading state for the dedicated water pipeline: the two wave
	// textures, the world camera position (for the view-dependent specular glint), and
	// the sun direction (matched to the terrain light). Scale maps world units into the
	// tiling bump UV; scroll speed drifts the ripples; ripple/spec tune their strength.
	const Vect3f& cp = camera->GetPos();
	float camPos[3]   = { cp.x, cp.y, cp.z };
	float lightDir[3] = { 0.35f, 0.45f, 0.82f };   // toward the sun (== TerrainRenderSDL)
	// bumpScale 0.005 and scrollSpeed 0.03 match the original (Water.cpp: speed_scale=5e-3,
	// offset = animate_time*0.03). Apparent drift = scroll/scale ~= 6 world units/sec.
	dev->setWaterRenderState(s_bump0, s_bump1, camPos, lightDir,
	                         /*bumpScale*/0.005f, /*scrollSpeed*/0.03f,
	                         /*rippleStrength*/1.0f, /*specStrength*/0.25f);
}

#else  // _WIN32

void renderWaterSDL(Camera*) {}

#endif // !_WIN32
