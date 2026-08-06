// World-quad vertex shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/NoMaterial/standart.vsl -- the vertex
// half of vsStandart/psStandart, the pair cD3DRender::SetWorldMaterial selects for
// geometry drawn with no material of its own. Its callers here are the shoreline coast
// sprites and the wave sources, both of which take it in its plainest configuration: one
// texture (Texture1 == 0, so COLOR_OPERATION is off) and no ZREFLECTION.
//
//     o.pos   = mul(v.pos, mWVP);
//     o.color = v.color;
//     o.uv0   = v.t0;
//
// mWorld is MatXf::ID for every caller, so mWVP is just the camera's view-projection and
// the world-space position the original derives for its lightmap/fog varyings is the
// vertex position itself. Both of those varyings are dropped: FOG_OF_WAR needs a lightmap
// the SDL backend has no path for, and the fixed-function fog stage has no SDL GPU
// equivalent. FLOAT_ZBUFFER is off too -- SetWorldMaterial clears its useZBuffer argument
// when there is no float map, which is always, here.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-worldquad-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // the original's mWVP; engine row-major, so clip = mul(pos, MVP)
    // Distance fog, from cSDLRenderDevice::fogPlane -- but pushed through this group's world
    // matrix first (SDLWorldQuadRenderer::openGroup), because the position below is in the
    // group's own space, not the world's. So the dot is against the LOCAL position.
    // (0,0,0,1) means fog is off: the factor is 1, and the fragment shader leaves the pixel
    // alone whichever of its two fog rules it takes.
    float4 FogPlane;
};

struct VSInput
{
    // The semantics are TEXCOORD<location>, not what the data means -- SDL_GPU's D3D12
    // backend names every input element TEXCOORD; see SDLShaders/ShaderBlob.h.
    float3 Position : TEXCOORD0;   // sVertexXYZDT1: world-space position, offset 0
    float4 Color    : TEXCOORD1;   // D3DCOLOR diffuse, offset 12 -> UBYTE4_NORM (b,g,r,a)
    float2 UV       : TEXCOORD2;   // offset 16 (stride 24)
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
    float  Fog      : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.UV = input.UV;

    // Color4c is stored BGRA, so UBYTE4_NORM gives (b,g,r,a); reorder to RGBA. Then
    // premultiply: the textures are decoded premultiplied (Render/src/DDSImage.cpp), so
    // this pipeline blends (ONE, 1-SRC_ALPHA) rather than the original's
    // (SRC_ALPHA, 1-SRC_ALPHA). Scaling the vertex colour by its own alpha keeps the
    // fragment shader's `texel * colour` premultiplied, which makes the two blends emit
    // exactly the same pixel. See worldquad.frag.hlsl.
    float4 color = input.Color.bgra;
    output.Color = float4(color.rgb * color.a, color.a);

    output.Fog = dot(float4(input.Position, 1.0f), FogPlane);
    return output;
}
