// Terrain shadow-caster fragment shader for the SDL GPU backend.
//
// Render/shader/Minimal/tile_map_shadow.psl exists only to write the interpolated
// light-space depth into a colour target (`return float4(v.uv0.xxxx)`), because D3D9 could
// not sample a depth buffer. Our caster pass has no colour target at all, and the terrain
// is one opaque mesh with no cutout, so there is nothing left to do here -- but SDL GPU
// still requires a fragment shader on every graphics pipeline.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-tilemap-shaders.sh.

void main(float4 position : SV_Position)
{
}
