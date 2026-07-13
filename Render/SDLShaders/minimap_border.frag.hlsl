// Everything drawn *on* the minimap: fragment shader for the SDL GPU backend.
//
// A port of Render/shader/miniMapBorder.psl, which the original uses for the unit and event
// symbols, the view-zone outline, the placement rectangles and the minimap's own border:
//
//     o = tex2D(baseTexture, uv1) * v.diffuse   // USE_TEXTURE (sprites)
//     o = v.diffuse                             // otherwise   (lines, rectangles)
//     o.a *= tex2D(borderTexture, uv0).a        // USE_BORDER
//
// The border mask is what keeps a round minimap round: everything drawn over the map is
// clipped to the same mask the map is, by multiplying its alpha with the mask's at that
// point of the *control* (not of the sprite) -- hence the second UV set.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-minimap-shaders.sh.

Texture2D<float4> BaseTexture   : register(t0, space2);   // the sprite's atlas
SamplerState      BaseSampler   : register(s0, space2);
Texture2D<float4> BorderTexture : register(t1, space2);   // the control's mask
SamplerState      BorderSampler : register(s1, space2);

cbuffer Params : register(b0, space3)
{
    // x != 0: sample the sprite texture (the original's USE_TEXTURE).
    // y != 0: multiply the alpha by the border mask (USE_BORDER).
    float4 Flags;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 MaskUV   : TEXCOORD0;
    float2 UV       : TEXCOORD1;
};

float4 main(VSOutput v) : SV_Target0
{
    float4 o = (Flags.x != 0.0f) ? BaseTexture.Sample(BaseSampler, v.UV) * v.Color : v.Color;

    if(Flags.y != 0.0f)
        o.a *= BorderTexture.Sample(BorderSampler, v.MaskUV).a;

    return o;
}
