// Coast-foam vertex shader for the SDL GPU backend (shoreline coast-sprite slice).
//
// Consumes a dynamic world-space quad stream built each frame from the real
// cCoastSprites simulation: float3 world position @0, packed BGRA colour @12
// (a premultiplied per-sprite fade), float2 uv @16 (stride 24). Transforms by the
// same model-view-projection matrix as terrain/water (row-major, applied v*M).
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // engine row-major; clip = mul(float4(pos,1), MVP)
};

struct VSInput
{
    float3 Position : POSITION;    // world-space, offset 0
    float4 Color    : COLOR0;      // BGRA (Color4c byte order), offset 12
    float2 UV       : TEXCOORD0;   // offset 16
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;      // RGBA
    float2 UV       : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.Color = input.Color.bgra;   // stored BGRA -> RGBA
    output.UV = input.UV;
    return output;
}
