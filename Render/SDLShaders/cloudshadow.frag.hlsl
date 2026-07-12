// Cloud-shadow fragment shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/NoMaterial/CloudShadow.psl (the
// PSCloudShadow class):
//
//     float3 c = tex0*tex1 + 0.5f*tex0 + 0.5*tex1;
//     ot.rgb = c*tfactor + tfactorm05;      // tfactorm05 = 0.5 - 0.75*tfactor
//     ot.a   = tfactor.a;
//
// which the original's own commented-out derivation (kept at the bottom of the .psl) shows
// is just a two-instruction form of
//
//     ot.rgb = 0.5 + ((tex0 + 0.5) * (tex1 + 0.5) - 1) * tfactor
//
// and THAT is the whole effect. Read it as: take the product of the same cloud texture
// sampled at two different scroll offsets, centre it on 1 (so a mid-grey cloud field is
// neutral), scale the deviation by tfactor, and re-centre on 0.5.
//
// 0.5 is the point. This is drawn into the terrain LIGHTMAP, whose neutral is mid-grey --
// the terrain adds `2*(lightmap - 0.5)` to its light and the grass adds `lightmap - 0.5`.
// So a cloud pixel darker than the field's average darkens the ground under it and a
// brighter one lifts it, and where tfactor is zero (see below) the quad writes a flat 0.5
// and changes nothing at all. The blend is ALPHA_NONE: this OVERWRITES the lightmap, and
// CameraPlanarLight::drawLights then blends the world's light sources over the top of it,
// which is why cCloudShadow sorts first (sortIndex -1, SCENENODE_OBJECTFIRST).
//
// tfactor is built on the CPU, in cCloudShadow::Draw, out of three things: the tile map's
// diffuse colour flattened to its own grey average, the world's serialized cloud intensity
// (`color`, 0-255), and -sunDirection.z. That last one is why the shadows fade out at dawn
// and dusk and vanish at night: a sun on the horizon casts no cloud shadow on the ground.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-cloudshadow-shaders.sh.

// The same cloud texture on both stages -- cCloudShadow::Draw binds texture1 to 0 and 1 --
// sampled at the two scrolling coordinates. sampler_wrap_linear: the coordinates run off
// the edge as they scroll, so this has to WRAP, not clamp.
Texture2D<float4> Tex0        : register(t0, space2);
SamplerState      Tex0Sampler : register(s0, space2);
Texture2D<float4> Tex1        : register(t1, space2);
SamplerState      Tex1Sampler : register(s1, space2);

cbuffer Constants : register(b0, space3)
{
    // The original's tfactor: how strongly the clouds bite, per channel, with the same
    // value in alpha. Zero means no cloud shadow at all.
    float4 TFactor;
    // The original's tfactorm05, which PSCloudShadow::Select computes as 0.5 - 0.75*tfactor.
    // Kept as its own uniform rather than derived here, so the arithmetic stays the
    // original's to the last bit.
    float4 TFactorM05;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
};

float4 main(VSOutput input) : SV_Target0
{
    float3 tex0 = Tex0.Sample(Tex0Sampler, input.UV0).rgb;
    float3 tex1 = Tex1.Sample(Tex1Sampler, input.UV1).rgb;

    float3 c = tex0 * tex1 + 0.5f * tex0 + 0.5f * tex1;

    float4 ot;
    ot.rgb = c * TFactor.rgb + TFactorM05.rgb;
    ot.a   = TFactor.a;
    return ot;
}
