// Terrain shadow-caster vertex shader for the SDL GPU backend.
//
// Ported from Render/shader/9700/tile_map_shadow.vsl, which is `o.pos = mul(pos, mWVP)`
// and nothing else -- plus `o.t0 = o.pos.z`, the light-space depth its pixel shader wrote
// into a float colour target. We render into a depth texture, so that carries nothing.
//
// Like tilemap.vert.hlsl this skips ConvertPos.inl: SDLTileMapRenderer's vertices are
// already world units. It also ignores the normal the vertex carries, so the pipeline
// binds only the position attribute.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-tilemap-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // the light camera's matViewProj
};

float4 main(float3 position : POSITION) : SV_Position
{
    return mul(float4(position, 1.0f), MVP);
}
