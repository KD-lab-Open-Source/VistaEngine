// Coast-foam fragment shader for the SDL GPU backend (shoreline coast-sprite slice).
//
// The bubble texture is decoded premultiplied (cDDSImage premultiplies colour DDS),
// and the vertex colour is a premultiplied per-sprite fade (rgb == a == fade, times
// the scene-lit tint), so texel * colour stays premultiplied and pairs with the
// (ONE, ONE_MINUS_SRC_ALPHA) blend the foam pipeline uses. Authored in HLSL;
// cross-compiled to SPIR-V/MSL with SDL_shadercross.

Texture2D<float4> Tex : register(t0, space2);
SamplerState      Smp : register(s0, space2);

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    return Tex.Sample(Smp, input.UV) * input.Color;
}
