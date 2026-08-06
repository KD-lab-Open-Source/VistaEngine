// Cloud-shadow vertex shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/NoMaterial/CloudShadow.vsl (the
// VSCloudShadow class), which does almost nothing: transform, and pass the two texture
// coordinates and the vertex colour through.
//
//     o.pos = mul(v.pos, mWVP);
//     o.color = v.color;  o.uv0 = v.t0;  o.uv1 = v.t1;
//     o.fog = 1;
//
// `o.fog = 1` is the original saying "never fog this", and it is right: the quad is drawn
// into the terrain LIGHTMAP, not into the view. cSDLRenderDevice keeps fog off around
// CameraPlanarLight::DrawScene for the same reason, so there is no fog term here at all.
//
// The geometry is one quad over the whole world (cCloudShadow's constructor: (0,0) to
// (H_SIZE, V_SIZE) at z = 0), and the two texture coordinate sets are the SAME cloud
// texture scrolling at two different speeds -- cCloudShadow::Animate advances uv1 and uv2
// and rewrites them into the vertex buffer each frame. Multiplying the two together is
// what stops the clouds from looking like one sliding tile; see cloudshadow.frag.hlsl.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-cloudshadow-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    // The original's mWVP (c0): world * the camera's view-projection. The camera here is
    // CameraPlanarLight, so this maps the world quad onto the lightmap's box.
    row_major float4x4 MVP;
};

struct VSInput
{
    // sVertexXYZDT2, stride 32.
    // The semantics are TEXCOORD<location>, not what the data means -- SDL_GPU's D3D12
    // backend names every input element TEXCOORD; see SDLShaders/ShaderBlob.h.
    float3 Position : TEXCOORD0;   // offset 0, world space
    float4 Color    : TEXCOORD1;   // offset 12, D3DCOLOR -> UBYTE4_NORM (b,g,r,a)
    float2 UV0      : TEXCOORD2;   // offset 16, the first scroll
    float2 UV1      : TEXCOORD3;   // offset 24, the second
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
    // Color4c is stored BGRA, so UBYTE4_NORM gives (b,g,r,a). The fragment shader does not
    // actually read this -- cCloudShadow writes a flat white into all four vertices and the
    // intensity arrives as a uniform instead -- but carry it, as the original does.
    output.Color = input.Color.bgra;
    output.UV0 = input.UV0;
    output.UV1 = input.UV1;
    return output;
}
