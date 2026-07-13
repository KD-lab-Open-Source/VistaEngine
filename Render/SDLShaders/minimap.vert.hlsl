// Minimap vertex shader for the SDL GPU backend. Shared by minimap.frag.hlsl (the map
// itself) and minimap_border.frag.hlsl (the events, view zone, border and start markers).
//
// The original has no minimap vertex shader at all: UI_Minimap writes pre-transformed
// vertices (sVertexXYZWDT1/2/4 -- D3DFVF_XYZRHW) straight in screen pixels and lets the
// fixed-function pipeline pass them through. SDL GPU has no such path, so this does the
// one thing that stage did: pixels to NDC. It is the UI vertex shader plus a second UV.
//
// The two UVs are the two the original's minimap vertices carry, whichever pixel shader
// consumes them:
//   MaskUV -- the control's border mask, in the minimap control's own 0..1 box
//             (UI_Minimap::setMaskUV). u4/v4 of sVertexXYZWDT4, u1/v1 of the others.
//   UV     -- the map itself for minimap.frag (u1..u3, all three identical), or the
//             sprite's atlas rect for minimap_border.frag (u2/v2 of sVertexXYZWDT2).
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-minimap-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    float2 InvScreenSize;   // (1/width, 1/height)
    float2 _pad;
};

struct VSInput
{
    float2 Position : POSITION;    // screen pixels
    float4 Color    : COLOR0;      // BGRA (Color4c byte order)
    float2 MaskUV   : TEXCOORD0;
    float2 UV       : TEXCOORD1;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;      // RGBA
    float2 MaskUV   : TEXCOORD0;
    float2 UV       : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    float2 ndc = input.Position * InvScreenSize * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    output.Position = float4(ndc, 0.0f, 1.0f);

    output.Color  = input.Color.bgra;
    output.MaskUV = input.MaskUV;
    output.UV     = input.UV;
    return output;
}
