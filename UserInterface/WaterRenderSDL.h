#ifndef __WATER_RENDER_SDL_H__
#define __WATER_RENDER_SDL_H__

class Camera;

// Off-Windows water-surface fallback (P2 slice 4, continues TerrainRenderSDL).
//
// The Windows renderer draws water through cWater::Draw -> VSWater/PSWater (custom
// D3D shaders) + reflection/bump/border-tile passes on gb_RenderDevice3D, which is
// null off-Windows -- and cScene::Draw returns early there, so cWater::Draw is never
// even reached. The water CPU state IS populated off-Windows, though: cWater::Init
// has a no-GPU guard that still runs InitZBuffer()/updateMap(), so the per-node
// height field (cWater::zbuffer) is real. This reads that field directly and draws a
// translucent surface over the filled cells through the same SDL mesh pass the
// terrain uses (registerMesh / addMeshSubmesh / setMeshTransform).
//
// Rebuilt when the world (or its water extent) changes; MVP refreshed each frame
// from the game camera. No-op on Windows.
void renderWaterSDL(Camera* camera);

#endif // __WATER_RENDER_SDL_H__
