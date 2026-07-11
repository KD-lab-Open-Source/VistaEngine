// Water-surface vertex shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/Water/water_easy.vsl -- the
// WATER_EMPTY technique, which cWater::Draw selects on hardware without PS2.0. It is
// the one water technique whose inputs all exist off-Windows: the two other techniques
// sample a planar-reflection render target (water_linear) or the sky cubemap
// (water_cube), and neither render target has an SDL path yet.
//
//     o.pos      = mul(v.pos, mVP);
//     o.diffuse  = v.diffuse;
//     o.uv_tex0  = uvScaleOffset.zw  + v.pos.xy*uvScaleOffset.xy;
//     o.uv_tex1  = uvScaleOffset1.zw + v.pos.yx*uvScaleOffset1.xy;
//
// The two wave maps scroll over the surface at different speeds and, note, sample the
// position under opposite swizzles (xy vs yx), which is what stops them beating against
// each other into a visible grid.
//
// Dropped, each for want of the input rather than by choice: o.uv_lightmap (the
// fog-of-war lightmap, behind #ifdef FOG_OF_WAR in the fragment shader) and o.fog (the
// fixed-function fog stage, which has no SDL GPU equivalent).
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-water-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // engine row-major; clip = mul(float4(pos,1), MVP)
    // The original's uvScaleOffset / uvScaleOffset1 (VSWater::SetSpeed/SetSpeed1):
    // xy is the world->uv scale, zw the scroll offset cWater::Draw advances with
    // animate_time. Both are affine in world XY -- the surface is a heightfield, so
    // there is no uv in the vertex at all.
    float4 UVScaleOffset;
    float4 UVScaleOffset1;
};

struct VSInput
{
    float3 Position : POSITION;   // world space, offset 0
    // sVertexXYZD's D3DCOLOR diffuse, offset 12. Only the alpha is read (the fragment
    // shader's per-vertex water opacity, baked by cWater::CalcColor from the depth
    // gradient); .w is the alpha byte whichever way the other three are ordered.
    float4 Diffuse  : COLOR0;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Diffuse  : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.Diffuse  = input.Diffuse;
    output.UV0 = UVScaleOffset.zw  + input.Position.xy * UVScaleOffset.xy;
    output.UV1 = UVScaleOffset1.zw + input.Position.yx * UVScaleOffset1.xy;
    return output;
}
