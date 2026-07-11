// Water-surface fragment shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/Water/water_easy.psl, whole:
//
//     ot.rgb = vPS11Color;
//     ot.a   = v.diffuse.a;
//     float4 tex0 = tex2D(Tex0Sampler, v.uv_tex0);
//     float4 tex1 = tex2D(Tex1Sampler, v.uv_tex1);
//     ot.a += ot.a*saturate((tex0.x+tex1.x));
//
// So the surface is a flat colour whose *opacity* carries the waves: vPS11Color is
// cWater's cur_reflect_sky_color (the reflected-sky colour for the current time of day,
// which the engine itself substitutes for a reflection here), the vertex alpha is the
// depth-derived opacity cWater::CalcColor bakes per grid node, and the wave maps then
// thicken it along the crests -- shallow water reads through, deep water does not, and
// the waves ripple across both.
//
// Two departures, both forced:
//
//  * tex0.x / tex1.x. The wave maps (waves.dds / waves1.dds) are D3DFMT_V8U8: two signed
//    bytes, so the original's tex0.x is already a signed slope in [-1,1] and its
//    saturate() clips the troughs to zero. The portable DDS decoder (Render/src/
//    DDSImage.cpp) stores those bytes biased by 128 into R,G of a BGRA8 texture, so
//    unbias here -- (raw*255 - 128)/127 -- exactly as object3dx.frag.hlsl does.
//
//  * No FOG_OF_WAR. That branch lerps to vFogOfWar by a lightmap alpha; the SDL backend
//    has no lightmap yet, and the original compiles the branch out without one too.
//
// ot.a exceeds 1 on a crest (the original's `ot.a += ot.a*...` doubles it at most). Both
// D3D9 and SDL GPU clamp a fragment's output to [0,1] before blending into a UNORM target,
// so the saturation is the hardware's, there and here. Left unclamped, as the original.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-water-shaders.sh.

Texture2D<float4> Tex0        : register(t0, space2);
SamplerState      Tex0Sampler : register(s0, space2);
Texture2D<float4> Tex1        : register(t1, space2);
SamplerState      Tex1Sampler : register(s1, space2);

cbuffer Water : register(b0, space3)
{
    // The original's vPS11Color (PSWater::SetPS11Color), fed cWater's
    // cur_reflect_sky_color. Alpha unused: the surface's comes from the vertex.
    float4 PS11Color;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Diffuse  : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
};

// One wave map's signed x slope, undoing the decoder's +128 bias.
float slope(Texture2D<float4> tex, SamplerState samp, float2 uv)
{
    return (tex.Sample(samp, uv).r * 255.0f - 128.0f) / 127.0f;
}

float4 main(VSOutput input) : SV_Target0
{
    float4 ot;
    ot.rgb = PS11Color.rgb;
    ot.a   = input.Diffuse.a;

    float wave = slope(Tex0, Tex0Sampler, input.UV0) + slope(Tex1, Tex1Sampler, input.UV1);
    ot.a += ot.a * saturate(wave);
    return ot;
}
