#ifndef __TERRAIN_RENDER_SDL_H__
#define __TERRAIN_RENDER_SDL_H__

class Camera;

// Off-Windows base-terrain fallback (P2 slice 4).
//
// The Windows world renderer draws the base terrain through
// cTileMap::Draw -> cTileMapRender::DrawBump -> gb_RenderDevice3D->dtAdvance, a
// hardware tilemap shader path (LOD / bump / detail-textures / shadow / reflection
// / vertex-pools) that is null off-Windows -- cTileMapRender is only constructed
// when gb_RenderDevice3D exists. Rather than port that whole stack, this reads the
// global heightfield (vMap) directly and draws a downsampled grid through the same
// SDL mesh pass the menu model uses (registerMesh / addMeshSubmesh / setMeshTransform).
//
// Builds the grid once (when vMap is loaded) and refreshes the MVP from the game
// camera every frame. No-op on Windows.
void renderTerrainSDL(Camera* camera);

#endif // __TERRAIN_RENDER_SDL_H__
