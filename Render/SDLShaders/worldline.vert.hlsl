// World-line vertex shader for the SDL GPU backend.
//
// The 3D line primitive of cInterfaceRenderDevice::DrawLine(const Vect3f&, ...),
// the SDL stand-in for what cD3DRender::FlushLine3D drew with vsStandart /
// psStandart under SetWorldMaterial(ALPHA_BLEND, MatXf::ID) — world-space
// vertices, identity world matrix, so mWVP is just the camera's view-projection
// (cSDLRenderDevice::camera_->matViewProj).
//
//     o.pos   = mul(v.pos, mWVP);
//     o.color = v.diffuse;
//
// The editor draws its terrain grid through this (EngineViewport::drawGrid);
// the game's DrawLine callers (the D3D-only legacy paths) are dead on the SDL
// backend, so the editor grid is the one user today.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-worldline-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // the camera's view-projection; engine row-major, so clip = mul(pos, MVP)
};

struct VSInput
{
    // The semantics are TEXCOORD<location>, not what the data means -- SDL_GPU's D3D12
    // backend names every input element TEXCOORD; see SDLShaders/ShaderBlob.h.
    float3 Position : TEXCOORD0;   // sVertexXYZD: world-space position, offset 0
    float4 Color    : TEXCOORD1;   // D3DCOLOR diffuse, offset 12 -> UBYTE4_NORM (b,g,r,a)
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);

    // Color4c is stored BGRA, so UBYTE4_NORM gives (b,g,r,a); reorder to RGBA.
    // No premultiply here: lines blend (SRC_ALPHA, 1-SRC_ALPHA) and carry their
    // alpha straight through, exactly as cD3DRender's line path did under
    // ALPHA_BLEND (the fixed-function MODULATE kept the vertex alpha).
    output.Color = input.Color.bgra;
    return output;
}
