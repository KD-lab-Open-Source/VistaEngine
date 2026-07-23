// Terrain-ice fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Water/water_ice.psl (the PS2.0, MIRROR_LINEAR path) as
// cTileMap::setMaterial drove it for a placement-zone ICE material with a null alpha texture:
//
//     snow = tex2D(Snow, tsnow); snow.rgb *= vSnowColor.rgb;      // psl
//     bump = tex2D(Bump, tbump);                                  // psl
//     cube_coord = mirror; cube_coord.xy += bump.xy*cube_coord.w*0.05;
//     ot.rgb = tex2Dproj(Reflection, cube_coord);                 // psl
//     ot.rgb = lerp(ot, snow, snow.a);                            // psl
//     // #ifdef FOG_OF_WAR: ot.rgb = lerp(ot.rgb, vFogOfWar, lightmap.a);
//
// So the ice is a planar reflection (the reflection camera's render target, sampled through
// the bump-perturbed mirror coordinate) with the snow texture laid over it wherever the snow
// alpha is opaque. The snow is tinted by the scene's plain-lit colour (vSnowColor <-
// cScene::GetPlainLitColor). The bump texture is the engine's V8U8 decoded to BGRA, unbiased
// here the same way water.frag.hlsl does its wave bump.
//
// The opaque terrain path drops water_ice.psl's alpha maths (ot.a from the cleft texture and
// the vertex diffuse): cTileMap drew this with Z-write on and no blend, so only the rgb
// matters. The alpha coverage and cleft belong to the ice-on-water case (cTemperature::Draw),
// a separate pipeline. When there is no reflection camera (Params.x == 0) the reflection
// falls back to the snow colour, so the surface reads as plain snow rather than sampling an
// empty target -- the sky-cubemap fallback of the original has no SDL consumer yet.
//
// Distance fog is applied last, where D3D9's fixed function put it (see tilemap.frag.hlsl).

Texture2D<float4>  SnowTexture       : register(t0, space2);
SamplerState       SnowSampler       : register(s0, space2);
Texture2D<float4>  BumpTexture       : register(t1, space2);
SamplerState       BumpSampler       : register(s1, space2);
Texture2D<float4>  ReflectionTexture : register(t2, space2);
SamplerState       ReflectionSampler : register(s2, space2);
// The terrain lightmap; only its ALPHA (the fog of war) is read here.
Texture2D<float4>  LightMapTexture   : register(t3, space2);
SamplerState       LightMapSampler   : register(s3, space2);

cbuffer Ice : register(b0, space3)
{
    // vSnowColor (rgb): the scene's plain-lit colour, tinting the snow texture.
    float4 SnowColor;
    // Distance fog colour (D3DRS_FOGCOLOR); the factor arrives interpolated in VSOutput.Fog.
    float4 FogColor;
    // The RTS shroud colour (the original's vFogOfWar, psl c3); only rgb is read.
    float4 FogOfWarColor;
    // x != 0: a reflection camera rendered a target this frame, so sample it (else plain snow).
    // y != 0: the fog of war is on and the lightmap alpha holds this frame's coverage.
    float4 Params;
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float2 SnowUV     : TEXCOORD0;
    float2 BumpUV     : TEXCOORD1;
    float2 LightmapUV : TEXCOORD2;
    float4 Mirror     : TEXCOORD3;
    float  Fog        : TEXCOORD4;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 snow = SnowTexture.Sample(SnowSampler, input.SnowUV);
    float3 snowRGB = snow.rgb * SnowColor.rgb;

    // V8U8 decoded to BGRA: unbias to signed, exactly as water.frag.hlsl's sampleBump.
    float2 bump = (BumpTexture.Sample(BumpSampler, input.BumpUV).rg * 255.0f - 128.0f) / 127.0f;

    float3 reflection;
    if(Params.x != 0.0f)
    {
        // tex2Dproj: perturb the projective coordinate by the bump, then divide by w.
        float4 coord = input.Mirror;
        coord.xy += bump * coord.w * 0.05f;
        reflection = ReflectionTexture.Sample(ReflectionSampler, coord.xy / coord.w).rgb;
    }
    else
        reflection = snowRGB;   // no reflection target -> plain snow (see header)

    float3 ot = lerp(reflection, snowRGB, snow.a);

    // Fog of war (lightmap alpha), where the original's #ifdef FOG_OF_WAR puts it.
    if(Params.y != 0.0f)
    {
        float4 lightmap = LightMapTexture.Sample(LightMapSampler, input.LightmapUV);
        ot = lerp(ot, FogOfWarColor.rgb, lightmap.a);
    }

    // Distance fog, last: Fog == 1 when off, so the lerp is then the identity.
    ot = lerp(FogColor.rgb, ot, saturate(input.Fog));

    // Opaque terrain ice: writes depth, nothing blends against it.
    return float4(ot, 1.0f);
}
