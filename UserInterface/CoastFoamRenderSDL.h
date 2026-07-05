#ifndef __COAST_FOAM_RENDER_SDL_H__
#define __COAST_FOAM_RENDER_SDL_H__

class Camera;

// Off-Windows shoreline coast-sprite foam (continues WaterRenderSDL).
//
// The Windows renderer draws foam through cCoastSprites::Draw, which uses the D3D
// quad buffer (GetQuadBufferXYZDT1) and is driven by the scene draw -- both dead
// off-Windows. The particle *simulation* is real, though, so this drives it directly
// (cCoastSprites::animateSDL / collectSprites) and draws the live sprites as flat
// quads on the water surface through the SDL foam pipeline. dtSeconds is the frame
// time (GameShell::Show's realGraphDT). No-op on Windows.
void renderCoastFoamSDL(Camera* camera, float dtSeconds);

#endif // __COAST_FOAM_RENDER_SDL_H__
