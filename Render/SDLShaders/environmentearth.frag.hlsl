// Environment-earth fragment shader for the SDL GPU backend.
//
// A verbatim port of Render/shader/NoMaterial/EnvironmentEarth.psl (PSEnvironmentEarth), with
// FOG_OF_WAR off -- the configuration cEnvironmentEarth::Draw drew in (it never set the define):
//
//     float4 ot = tex2D(t0, v.uv0);
//     ot.rgb = ot.rgb * tfactor * 2;
//     return ot;
//
// `tfactor` is a per-draw constant cEnvironmentEarth::Draw built from the tile map's diffuse and
// the sun angle -- the same terrain tint the map itself lights with -- so the ground beyond the
// edge matches the ground inside it and darkens together at dusk. The *2 is the original's, a
// brightness the fixed-function `x2` modulate baked in; carried at full precision here rather
// than folded into an 8-bit vertex colour, which would clamp it.
//
// The ground texture is opaque (alpha 1), and the pass is ALPHA_NONE, so alpha is carried
// through but neither blended nor sampled.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross.

Texture2D<float4> Tex0        : register(t0, space2);
SamplerState      Tex0Sampler : register(s0, space2);

cbuffer Params : register(b0, space3)
{
    float4 TFactor;   // the original's `tfactor`: rgb the tile/sun tint, a unused
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV0      : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 ot = Tex0.Sample(Tex0Sampler, input.UV0);
    ot.rgb = ot.rgb * TFactor.rgb * 2.0f;
    return ot;
}
