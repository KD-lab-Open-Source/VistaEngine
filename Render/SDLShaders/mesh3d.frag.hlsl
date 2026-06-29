// 3D static-mesh fragment shader for the SDL GPU backend (slice 3).
//
// Samples the model texture and applies a simple fixed directional light so the
// shape reads even before real material/lighting is ported. Authored in HLSL;
// cross-compiled to SPIR-V/MSL with SDL_shadercross (see build-shaders.sh).

Texture2D<float4> Texture : register(t0, space2);
SamplerState      Sampler : register(s0, space2);

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float3 n = normalize(input.Normal);
    float ndl = saturate(dot(n, normalize(float3(0.3f, 0.6f, 0.7f))));
    float light = ndl * 0.7f + 0.3f;   // ambient + diffuse
    float4 tex = Texture.Sample(Sampler, input.UV);
    return float4(tex.rgb * light, tex.a);
}
