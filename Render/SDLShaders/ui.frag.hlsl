// UI fragment shader for the SDL GPU backend.
//
// Samples one texture and modulates by the interpolated vertex colour — the
// engine's basic 2D sprite path (DrawSprite/DrawQuad). Authored in HLSL;
// cross-compiled to SPIR-V/MSL/DXIL with SDL_shadercross (see build-shaders.sh).

Texture2D<float4> Texture : register(t0, space2);
SamplerState      Sampler : register(s0, space2);

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    return Texture.Sample(Sampler, input.UV) * input.Color;
}
