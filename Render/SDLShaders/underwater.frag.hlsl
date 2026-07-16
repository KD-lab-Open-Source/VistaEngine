// Under-water post effect for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/PostProcessing/underwater.psl (the
// PSUnderWater class):
//
//     float4 c  = tex2D(SrcTexture1, v.Tex + float2(0,shift));            // wave texture
//     float4 uv = tex2D(SrcTexture0, v.Tex*(1-scale.x*2) + c.xy*scale.x*2);
//     return uv + lerp(0, color, scale.y);
//
// The wave texture scrolls downward and its red/green channels displace the scene lookup:
// the screen zooms in slightly (the UV range shrinks by scale.x each side) and wobbles by
// up to the same amount, then the water colour fades in on top. PSUnderWater::Select
// packed Scale as (scale*0.05, scale, ...) -- the CPU side here does the same, so the
// arithmetic stays the original's.
//
// Both samplers are wrap/linear, as PostEffectUnderWater::setSamplerState set stages
// 0 and 1 (the displaced scene UV can nudge past 1 on the right/bottom edge).

Texture2D<float4> Scene        : register(t0, space2);
SamplerState      SceneSampler : register(s0, space2);
Texture2D<float4> Wave         : register(t1, space2);
SamplerState      WaveSampler  : register(s1, space2);

cbuffer Constants : register(b0, space3)
{
    float4 ShiftScale;   // .x = shift; .y = scale*0.05 (the original's scale.x); .z = scale (its scale.y)
    float4 WaterColor;   // the world's underWaterColor
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 wave = Wave.Sample(WaveSampler, input.UV + float2(0.0f, ShiftScale.x));
    float4 c = Scene.Sample(SceneSampler,
                            input.UV * (1.0f - ShiftScale.y * 2.0f) + wave.xy * ShiftScale.y * 2.0f);
    return c + WaterColor * ShiftScale.z;
}
