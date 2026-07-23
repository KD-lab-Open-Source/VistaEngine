// Ice-on-water fragment shader for the SDL GPU backend.
//
// A faithful port of Render/shader/Water/water_ice.psl (the PS2.0, MIRROR_LINEAR, USE_ALPHA
// path) as cTemperature::Draw drove it over the water surface:
//
//     float tex0 = tex2D(AlphaSampler, talpha).r;   // temperature coverage
//     float aa = diffuse.r;
//     float4 cleft = tex2D(CleftSampler, tcleft);
//     ot.a = tex0*aa;
//     ot.a = (ot.a-0.5)*4+0.5;                       // sharpen the coverage edge
//     ot.a = ot.a + (cleft.r-0.5)*2;                 // break it up with the cleft detail
//     snow = tex2D(SnowSampler, tsnow); snow.rgb *= vSnowColor.rgb;
//     bump = tex2D(BumpSampler, tbump);
//     cube_coord = mirror; cube_coord.xy += bump.xy*cube_coord.w*0.05;
//     ot.rgb = tex2Dproj(Reflection, cube_coord);
//     ot.rgb = lerp(ot, snow, snow.a);
//     // #ifdef FOG_OF_WAR: ot.rgb = lerp(ot.rgb, vFogOfWar, lightmap.a);
//     return ot;                                     // alpha-blended over the water
//
// So the ice is the reflective-snow surface of the terrain ice (tilemap_ice.frag.hlsl),
// blended over the already-drawn water by an alpha that the temperature coverage grid drives:
// the grid says how frozen each cell is (aa, the vertex red, weights it), the maths sharpens
// that into a hard-ish edge, and the cleft texture roughens the edge so the shoreline of the
// ice is not a clean contour. The original used a fixed-function alpha test at alpha_ref to
// drop the open water; there is no fixed-function test on SDL GPU, so clip() stands in, the
// same substitution the grass shader makes.
//
// Two SDL-specific reads. The coverage texture is the engine's 8-bit L8 (cTemperature's
// textureAlpha_), which cSDLRenderDevice widens to (255,255,255, coverage) -- so the coverage
// is in ALPHA here, not red. The bump is V8U8 decoded to BGRA, unbiased as water.frag.hlsl
// does. When there is no reflection camera the reflection falls back to plain snow.
//
// Distance fog is applied last (colour only -- the coverage alpha must not be fogged), where
// D3D9's fixed function put it.

Texture2D<float4>  AlphaTexture      : register(t0, space2);   // temperature coverage (in .a)
SamplerState       AlphaSampler      : register(s0, space2);
Texture2D<float4>  SnowTexture       : register(t1, space2);
SamplerState       SnowSampler       : register(s1, space2);
Texture2D<float4>  BumpTexture       : register(t2, space2);
SamplerState       BumpSampler       : register(s2, space2);
Texture2D<float4>  ReflectionTexture : register(t3, space2);
SamplerState       ReflectionSampler : register(s3, space2);
Texture2D<float4>  CleftTexture      : register(t4, space2);
SamplerState       CleftSampler      : register(s4, space2);
// The terrain lightmap; only its ALPHA (the fog of war) is read here.
Texture2D<float4>  LightMapTexture   : register(t5, space2);
SamplerState       LightMapSampler   : register(s5, space2);

cbuffer Ice : register(b0, space3)
{
    // vSnowColor (rgb): the scene's plain-lit colour, tinting the snow texture.
    float4 SnowColor;
    // Distance fog colour (D3DRS_FOGCOLOR); the factor arrives interpolated in VSOutput.Fog.
    float4 FogColor;
    // The RTS shroud colour (the original's vFogOfWar, psl c3); only rgb is read.
    float4 FogOfWarColor;
    // x != 0: a reflection camera rendered a target this frame, so sample it (else plain snow).
    // y != 0: fog of war on. z = the alpha-test reference (alpha_ref/255), the clip() threshold.
    float4 Params;
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float4 Diffuse    : COLOR0;
    float2 AlphaUV    : TEXCOORD0;
    float2 SnowUV     : TEXCOORD1;
    float2 BumpUV     : TEXCOORD2;
    float2 CleftUV    : TEXCOORD3;
    float2 LightmapUV : TEXCOORD4;
    float4 Mirror     : TEXCOORD5;
    float  Fog        : TEXCOORD6;
};

float4 main(VSOutput input) : SV_Target0
{
    // Coverage is in ALPHA (the L8 grid widened to (255,255,255,coverage)); aa is the vertex red.
    float coverage = AlphaTexture.Sample(AlphaSampler, input.AlphaUV).a;
    float aa = input.Diffuse.r;
    float cleft = CleftTexture.Sample(CleftSampler, input.CleftUV).r;

    float a = coverage * aa;
    a = (a - 0.5f) * 4.0f + 0.5f;
    a = a + (cleft - 0.5f) * 2.0f;
    // The original's D3DRS_ALPHAREF alpha test: drop the open water below the reference.
    clip(a - Params.z);

    float4 snow = SnowTexture.Sample(SnowSampler, input.SnowUV);
    float3 snowRGB = snow.rgb * SnowColor.rgb;

    // V8U8 decoded to BGRA: unbias to signed, exactly as water.frag.hlsl's slope().
    float2 bump = (BumpTexture.Sample(BumpSampler, input.BumpUV).rg * 255.0f - 128.0f) / 127.0f;

    float3 reflection;
    if(Params.x != 0.0f)
    {
        float4 coord = input.Mirror;
        coord.xy += bump * coord.w * 0.05f;
        reflection = ReflectionTexture.Sample(ReflectionSampler, coord.xy / coord.w).rgb;
    }
    else
        reflection = snowRGB;   // no reflection target -> plain snow

    float3 ot = lerp(reflection, snowRGB, snow.a);

    // Fog of war (lightmap alpha), where the original's #ifdef FOG_OF_WAR puts it.
    if(Params.y != 0.0f)
    {
        float4 lightmap = LightMapTexture.Sample(LightMapSampler, input.LightmapUV);
        ot = lerp(ot, FogOfWarColor.rgb, lightmap.a);
    }

    // Distance fog, last, colour only: the coverage alpha drives the blend and must not be fogged.
    ot = lerp(FogColor.rgb, ot, saturate(input.Fog));

    // Alpha-blended over the water (src_alpha, 1-src_alpha), so the coverage is the ice's opacity.
    return float4(ot, saturate(a));
}
