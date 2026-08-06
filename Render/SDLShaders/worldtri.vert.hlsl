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
    // Distance fog, from cSDLRenderDevice::fogPlane -- but pushed through this group's world
    // matrix first (SDLWorldQuadRenderer::openGroup), because the position below is in the
    // group's own space, not the world's. So the dot is against the LOCAL position.
    // (0,0,0,1) means fog is off: the factor is 1, and the fragment shader leaves the pixel
    // alone whichever of its two fog rules it takes.
    float4 FogPlane;

    // --- ZREFLECTION only -----------------------------------------------------
    // The original's mWorld (standart.vsl c-something; VSStandart::Select uploads it beside
    // mWVP). MVP above has already folded the world matrix away into clip space, and the
    // height lookup below needs a WORLD position, so it has to come across on its own.
    row_major float4x4 World;
    // The original's vReflectionMul: (1/H_SIZE, 1/V_SIZE, 1, 0) from cD3DRender's
    // tilemap_inv_size. World XY lands in the height texture's 0..1; world Z passes through
    // unscaled, to be compared against what the texture decodes to.
    float4 ReflectionMul;
};

struct VSInput
{
    // The semantics are TEXCOORD<location>, not what the data means -- SDL_GPU's D3D12
    // backend names every input element TEXCOORD; see SDLShaders/ShaderBlob.h.
    float3 Position : TEXCOORD0;   // sVertexXYZDT2: position, offset 0
    float4 Color    : TEXCOORD1;   // D3DCOLOR diffuse, offset 12 -> UBYTE4_NORM (b,g,r,a)
    float2 UV0      : TEXCOORD2;   // offset 16
    float2 UV1      : TEXCOORD3;   // offset 24 (stride 32)
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
    float  Fog      : TEXCOORD2;
    // The original's `o.treflection = world_pos * vReflectionMul` (TEXCOORD5 there): xy is
    // where this vertex sits in the height texture, z is its world height. Only ZREFLECTION
    // reads it.
    float3 ZRef     : TEXCOORD3;
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

    output.Fog = dot(float4(input.Position, 1.0f), FogPlane);

    float3 worldPos = mul(float4(input.Position, 1.0f), World).xyz;
    output.ZRef = worldPos * ReflectionMul.xyz;
    return output;
}
