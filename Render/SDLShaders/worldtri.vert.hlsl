// World-triangle vertex shader for the SDL GPU backend.
//
// The same original as worldquad: Render/shader/NoMaterial/standart.vsl, the vertex half
// of vsStandart/psStandart that cD3DRender::SetWorldMaterial selects. This is its
// COLOR_OPERATION configuration -- two textures, so two texcoord sets -- drawn from the
// wider sVertexXYZDT2 vertex that cVertexBuffer<sVertexXYZDT2> carries:
//
//     o.pos = mul(v.pos, mWVP);
//     o.color = v.color;
//     o.uv0 = v.t0;
//     o.uv1 = v.t1;     // #ifdef COLOR_OPERATION
//
// The lightmap, fog and FLOAT_ZBUFFER varyings are dropped for the same reasons as in
// worldquad.vert.hlsl. Its caller is cEmitterColumnLight (the light columns and laser
// beams), whose quads are built in emitter space -- so unlike the quad callers, mWorld is
// usually NOT identity, and MVP here is world * the camera's view-projection.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-worldtri-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // the original's mWVP; engine row-major, so clip = mul(pos, MVP)
};

struct VSInput
{
    float3 Position : POSITION;    // sVertexXYZDT2: position, offset 0
    float4 Color    : COLOR0;      // D3DCOLOR diffuse, offset 12 -> UBYTE4_NORM (b,g,r,a)
    float2 UV0      : TEXCOORD0;   // offset 16
    float2 UV1      : TEXCOORD1;   // offset 24 (stride 32)
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.UV0 = input.UV0;
    output.UV1 = input.UV1;

    // Color4c is stored BGRA, so UBYTE4_NORM gives (b,g,r,a); reorder, then premultiply --
    // the textures decode premultiplied and this pipeline blends (ONE, 1-SRC_ALPHA). Same
    // reasoning as worldquad.vert.hlsl, which explains it in full.
    float4 color = input.Color.bgra;
    output.Color = float4(color.rgb * color.a, color.a);
    return output;
}
