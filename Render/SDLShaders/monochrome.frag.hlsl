// Monochrome post effect for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/PostProcessing/monochrome.psl (the
// PSMonochrome class):
//
//     float4 c = tex2D(SrcTexture,v.Tex)*v.diffuse;
//     return float4(c.rgb*fTimeInv + dot(c.rgb,LuminanceConv)*fTime, c.a);
//
// Phase 0 is the untouched scene, phase 1 fully grey; PostEffectMonochrome fades the
// phase at 1/s when a trigger (the pause screen, the campaign's flashback missions)
// flips it. The original's v.diffuse was DrawQuad's colour, which its post-effect
// callers always left white, so the term is dropped. fTimeInv arrived as its own
// constant (1 - fTime, PSMonochrome::Select); here it is derived -- one float either way.

Texture2D<float4> Scene        : register(t0, space2);
SamplerState      SceneSampler : register(s0, space2);

cbuffer Constants : register(b0, space3)
{
    float4 Phase;   // .x = the fade phase; .yzw unused
};

static const float3 LuminanceConv = { 0.2125f, 0.7154f, 0.0721f };

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 c = Scene.Sample(SceneSampler, input.UV);
    return float4(c.rgb * (1.0f - Phase.x) + dot(c.rgb, LuminanceConv) * Phase.x, c.a);
}
