// The minimap itself: fragment shader for the SDL GPU backend.
//
// A faithful port of Render/shader/miniMap.psl, whose four #ifdefs (USE_TERRA_COLOR,
// USE_WATER, ADDITION_TEXTURE, USE_BORDER) the original recompiles the shader for and we
// carry in Flags instead:
//
//     o = terraColor                                  // USE_TERRA_COLOR
//     o = tex2D(baseTexture, uv0)                     // otherwise
//     o.rgb = lerp(o.rgb, waterColor, wa)             // USE_WATER
//     o.a   = c.a - c.a * t.z * additionAlpha.a       // ADDITION_TEXTURE == 1 (fog of war)
//     o.rgb = lerp(o.rgb, t, additionAlpha.a * t.a)   // ADDITION_TEXTURE == 2 (place zones)
//     o.a  *= tex2D(borderTexture, uv3).a             // USE_BORDER
//
// Note what the original does *not* do: it never multiplies the map by the vertex colour.
// The control's fade alpha only reaches the output through the fog-of-war branch, which is
// where `c.a` comes from. Kept as-is.
//
// One divergence, forced by how the backends store an 8-bit texture. The water mask and the
// fog-of-war map are cTexLibrary::CreateAlphaTexture textures -- TEXTURE_GRAY|ALPHA_BLEND,
// i.e. SURFMT_L8 -- and D3D9 samples L8 as (L,L,L,1), which is why the original reads their
// *rgb* (`float3 wa`, `t.z`). cSDLRenderDevice widens 8-bit staging to BGRA as
// (255,255,255,coverage) so the font atlas comes out white with its coverage in alpha, so
// here the same value arrives in .a instead. Read it there; the arithmetic is unchanged.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-minimap-shaders.sh.

Texture2D<float4> BaseTexture     : register(t0, space2);   // the world's map.tga
SamplerState      BaseSampler     : register(s0, space2);
Texture2D<float4> WaterTexture    : register(t1, space2);   // cWater::GetTextureMiniMap (L8)
SamplerState      WaterSampler    : register(s1, space2);
Texture2D<float4> AdditionTexture : register(t2, space2);   // fog of war (L8), or place zones (BGRA)
SamplerState      AdditionSampler : register(s2, space2);
Texture2D<float4> BorderTexture   : register(t3, space2);   // the control's mask
SamplerState      BorderSampler   : register(s3, space2);

cbuffer Params : register(b0, space3)
{
    float4 WaterColor;      // the original's waterColor (c0); Environment::minimapWaterColor
    float4 AdditionAlpha;   // the original's additionAlpha (c1), splatted; .x is used
    float4 TerraColor;      // the original's terraColor (c2), for a world with no map.tga
    // x != 0: a map texture is bound (else TerraColor).  y != 0: blend water.
    // z: the addition texture's mode -- 0 none, 1 fog of war, 2 placement zones.
    // w != 0: multiply the alpha by the border mask.
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
    float4 o = (Flags.x != 0.0f) ? BaseTexture.Sample(BaseSampler, v.UV) : TerraColor;

    if(Flags.y != 0.0f)
    {
        // The water mask, one channel: 1 where the map is under water.
        float wa = WaterTexture.Sample(WaterSampler, v.UV).a;
        o.rgb = lerp(o.rgb, WaterColor.rgb, wa);
    }

    if(Flags.z == 1.0f)
    {
        // Fog of war. FogOfWar::GetInvFogAlpha is the *inverse* alpha, so an unexplored
        // cell (mask 1) drives the map's alpha towards zero and the panel shows through.
        float t = AdditionTexture.Sample(AdditionSampler, v.UV).a;
        o.a = v.Color.a - v.Color.a * t * AdditionAlpha.x;
    }
    else if(Flags.z == 2.0f)
    {
        // The placement zones, painted per pixel into a BGRA texture by
        // UI_Minimap::renderPlazeZones -- so this one really is a colour, alpha and all.
        float4 t = AdditionTexture.Sample(AdditionSampler, v.UV);
        o.rgb = lerp(o.rgb, t.rgb, AdditionAlpha.x * t.a);
    }

    if(Flags.w != 0.0f)
        o.a *= BorderTexture.Sample(BorderSampler, v.MaskUV).a;

    return o;
}
