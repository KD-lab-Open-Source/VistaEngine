// Environment-earth vertex shader for the SDL GPU backend.
//
// Ported from the original P2 path cEnvironmentEarth::Draw drew with: SetWorldMaterial selects
// vsStandart, and psEnvironmentEarth (Render/shader/NoMaterial/EnvironmentEarth.psl) overrides
// only the pixel half. So the vertex side is vsStandart in its plainest configuration -- one
// texture, no lightmap/fog/zbuffer varyings, which is all EnvironmentEarth.psl reads:
//
//     o.pos = mul(v.pos, mWVP);
//     o.uv0 = v.t0;
//
// The geometry is a flat plane at z = outsideHeight that fills the world beyond the map edge
// out to the horizon (cEnvironmentEarth's constructor, SetBoxBorder). Its texture coordinates
// are the world XY scaled by the ground texture size (cEnvironmentEarth::SetTexture), so the
// ground tiles as it recedes -- which is why the sampler wraps.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross.

cbuffer Constants : register(b0, space1)
{
    // The original's mWVP (c0): world * the camera's view-projection. cEnvironmentEarth has no
    // world matrix of its own (MatXf::ID), so this is just the camera's.
    row_major float4x4 MVP;
};

struct VSInput
{
    // sVertexXYZDT1, stride 24. The diffuse is carried to match the vertex declaration but not
    // used -- EnvironmentEarth.psl shades from the texture and the tfactor uniform alone.
    // The semantics are TEXCOORD<location>, not what the data means -- SDL_GPU's D3D12
    // backend names every input element TEXCOORD; see SDLShaders/ShaderBlob.h.
    float3 Position : TEXCOORD0;   // offset 0, world space
    float4 Color    : TEXCOORD1;   // offset 12, D3DCOLOR -> UBYTE4_NORM
    float2 UV0      : TEXCOORD2;   // offset 16
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV0      : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.UV0 = input.UV0;
    return output;
}
