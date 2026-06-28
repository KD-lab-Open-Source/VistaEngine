// UI vertex shader for the SDL GPU backend.
//
// Matches the engine's 2D sprite vertex sVertexXYZWDT1: screen-space pixel
// position (x,y in pixels; z,w unused here), a packed BGRA diffuse colour, and
// one UV. We transform pixel coords to NDC using the inverse screen size passed
// as a vertex uniform. Authored in HLSL; cross-compiled to SPIR-V/MSL/DXIL with
// SDL_shadercross (see build-shaders.sh).

cbuffer Constants : register(b0, space1)
{
    float2 InvScreenSize;   // (1/width, 1/height)
    float2 _pad;
};

struct VSInput
{
    float4 Position : POSITION;   // x,y in pixels
    float4 Color    : COLOR0;     // BGRA (Color4c byte order)
    float2 UV       : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;     // RGBA
    float2 UV       : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Pixel coords -> NDC. x: [0,w] -> [-1,1]; y: [0,h] -> [1,-1] (flip).
    float2 ndc = input.Position.xy * InvScreenSize * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    output.Position = float4(ndc, 0.0f, 1.0f);

    // Color4c is stored BGRA, so UBYTE4_NORM gives (b,g,r,a); reorder to RGBA.
    output.Color = input.Color.bgra;
    output.UV = input.UV;
    return output;
}
