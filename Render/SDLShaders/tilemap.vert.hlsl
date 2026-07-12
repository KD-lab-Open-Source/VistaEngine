// Tilemap (terrain) vertex shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/Minimal/tile_map_scene.vsl,
// which takes exactly two attributes -- position and normal -- and derives every
// texture coordinate from the world XY of the vertex:
//
//     o.pos = mul(pos, mWVP);
//     o.t0  = pos.xy*UV.zw + UV.xy;
//
// Two deliberate departures from the original:
//
//  * No ConvertPos.inl. The D3D tilemap stores Z as fixed point and decodes it with
//    `pos.z *= 1/64`, compensating with a world matrix. SDLTileMapRenderer builds its
//    own vertex buffer straight from vMap.getZf(), which is already world units, so
//    the position arrives ready to transform.
//  * The normal arrives unbiased. The original packs it into COLOR0 and unbiases with
//    `v.n*2-1`; we own the buffer, so it holds a plain world-space float3.
//
// The original lights per vertex under #ifdef VERTEX_LIGHT. We interpolate the normal
// and light per pixel instead (same lambert, see tilemap.frag.hlsl): our grid samples
// every few fine cells, so per-vertex lighting would visibly facet.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-tilemap-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // engine row-major; clip = mul(float4(pos,1), MVP)
    // Surface-colour texture mapping, straight from the original's `UV` (c0):
    // uv = pos.xy * UV.zw + UV.xy. The whole terrain colour map spans the world, so
    // zw is 1/H_SIZE, 1/V_SIZE and xy is zero -- but keep the original's affine form.
    float4 UV;
    // The original's mShadow (vsl c10): shadowMatViewProj() * shadowMatBias(), i.e. world
    // space -> the light's clip space -> shadow map texture coords. Identity when there
    // is no shadow map; the fragment shader gates on ShadowParams.x, not on this.
    row_major float4x4 Shadow;
    // The original's fPlanarNode (vsl c95), from cD3DRender::setPlanarTransform: xy is the
    // lightmap box's world origin, zw its inverse extent. cScene::AddPlanarCamera sets it
    // to the same box the lightmap camera renders, so this maps world xy onto its texels.
    float4 PlanarNode;
};

struct VSInput
{
    float3 Position : POSITION;   // world space, offset 0
    float3 Normal   : NORMAL;     // world space, offset 12
};

struct VSOutput
{
    float4 Position  : SV_Position;
    float3 Normal    : NORMAL;
    float2 UV        : TEXCOORD0;
    float4 ShadowPos : TEXCOORD1;
    float2 LightmapUV : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position  = mul(float4(input.Position, 1.0f), MVP);
    output.Normal    = input.Normal;
    output.UV        = input.Position.xy * UV.zw + UV.xy;
    // The original's `o.tshadow = mul(pos, mShadow)`. Its companion `o.shadowFactor` is
    // computed per pixel instead: the fragment shader already has the normal and the
    // light direction, so interpolating it would only cost a varying.
    output.ShadowPos = mul(float4(input.Position, 1.0f), Shadow);
    // The original's `o.uv_lightmap = (pos.xy - fPlanarNode.xy) * fPlanarNode.zw`.
    output.LightmapUV = (input.Position.xy - PlanarNode.xy) * PlanarNode.zw;
    return output;
}
