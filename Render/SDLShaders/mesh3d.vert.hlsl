// 3D static-mesh vertex shader for the SDL GPU backend (slice 3).
//
// Consumes the engine's sVertexXYZINT1 layout (stride 36): float3 position @0,
// BYTE[4] bone index @12 (skinning ignored for now), float3 normal @16, float2
// uv @28. We only declare the attributes used. Transforms position by a single
// model-view-projection matrix (row-major, applied as v*M). Authored in HLSL;
// cross-compiled to SPIR-V/MSL with SDL_shadercross (see build-shaders.sh).

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // engine row-major; clip = mul(float4(pos,1), MVP)
};

struct VSInput
{
    float3 Position : POSITION;    // offset 0
    float3 Normal   : NORMAL;      // offset 16
    float2 UV       : TEXCOORD0;   // offset 28
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.Normal = input.Normal;
    output.UV = input.UV;
    return output;
}
