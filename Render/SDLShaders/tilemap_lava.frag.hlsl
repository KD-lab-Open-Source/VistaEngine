// Terrain-lava fragment shader for the SDL GPU backend.
//
// A faithful port of Render/shader/Water/water_lava.psl (the PS2.0 path). The lava is a
// four-octave value-noise fBm sampled through a 64^3 random volume, scrolled in Z by time
// so the pattern boils, tinted between an ambient floor and a hot colour, then floored by a
// ground texture so cooling crust shows through:
//
//     Coord = uv_volume; Coord.z += time;                 // psl
//     for i in 0..3: fnoise = tex3D(Volume, Coord*fcoord); // psl
//                    fnoise = (fnoise-0.55)*2;              // psl
//                    rnd += fnoise.x * f;                   // psl
//                    f *= 0.5; fcoord *= 3;                 // psl
//     ot.rgb = rnd*vLavaColor + vLavaColorAmbient;         // psl
//     ot.rgb = max(ot.rgb, tex2D(Ground, uv_ground));      // psl
//
// The original's uv_volume / uv_ground are pos.xyz*volumeScale / pos.xy*groundScale, built
// in the VS; here the world position arrives interpolated and the scales come from the
// cbuffer, so the two coordinates are formed below. The volume sampler wraps and filters
// linearly, exactly the original's SetSamplerData(0, sampler_wrap_linear).
//
// Lava is emissive: unlike the terrain shader it reads no sun, no shadow map and no lightmap
// RGB. It keeps only the two layers the original's PS keeps -- the fog of war (lightmap
// alpha) under #ifdef FOG_OF_WAR, and distance fog, added last where D3D9's fixed function
// applied it (see tilemap.frag.hlsl). Both are the identity when off.

Texture3D<float4>  VolumeTexture   : register(t0, space2);
SamplerState       VolumeSampler   : register(s0, space2);
Texture2D<float4>  GroundTexture   : register(t1, space2);
SamplerState       GroundSampler   : register(s1, space2);
// The terrain lightmap; only its ALPHA (the fog of war) is read here.
Texture2D<float4>  LightMapTexture : register(t2, space2);
SamplerState       LightMapSampler : register(s2, space2);

cbuffer Lava : register(b0, space3)
{
    // The original's vLavaColor (hot) and vLavaColorAmbient (floor), from
    // ShaderSceneWaterLava::SetColors -> the material's lavaColor / colorAmbient.
    float4 LavaColor;
    float4 LavaColorAmbient;
    // x = ground texture scale (uv_ground = pos.xy*x), y = volume scale
    // (uv_volume = pos.xyz*y), z = animation time (Coord.z += z). The original passes
    // textureScale as a float2 and time as a separate constant; folded here.
    float4 Params;
    // Distance fog colour (D3DRS_FOGCOLOR); the factor arrives interpolated in VSOutput.Fog.
    float4 FogColor;
    // The RTS shroud colour (the original's vFogOfWar, psl c3); only rgb is read.
    float4 FogOfWarColor;
    // x != 0: the fog of war is on and the lightmap alpha holds this frame's coverage.
    // The original's FOG_OF_WAR define; a uniform here, fed from cSDLRenderDevice::fogOfWar().
    float4 LightMapParams;
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float3 WorldPos   : TEXCOORD0;
    float2 LightmapUV : TEXCOORD1;
    float  Fog        : TEXCOORD2;
};

float4 main(VSOutput input) : SV_Target0
{
    float3 uv_volume = input.WorldPos * Params.y;
    float2 uv_ground = input.WorldPos.xy * Params.x;

    float rnd = 0.0f;
    float f = 1.0f;
    float fcoord = 0.2f;
    float3 Coord = uv_volume;
    Coord.z += Params.z;   // animation time
    for(int i = 0; i < 4; i++)
    {
        float4 fnoise = VolumeTexture.Sample(VolumeSampler, Coord * fcoord);
        fnoise -= 0.55f;
        fnoise *= 2.0f;
        rnd += fnoise.x * f;
        f *= 0.5f;
        fcoord *= 3.0f;
    }

    float4 ot;
    ot.rgb = rnd * LavaColor.rgb + LavaColorAmbient.rgb;
    ot.rgb = max(ot.rgb, GroundTexture.Sample(GroundSampler, uv_ground).rgb);
    ot.a = 1.0f;

    // Fog of war (lightmap alpha), where the original's #ifdef FOG_OF_WAR puts it.
    if(LightMapParams.x != 0.0f)
    {
        float4 lightmap = LightMapTexture.Sample(LightMapSampler, input.LightmapUV);
        ot.rgb = lerp(ot.rgb, FogOfWarColor.rgb, lightmap.a);
    }

    // Distance fog, last: Fog == 1 when off, so the lerp is then the identity.
    ot.rgb = lerp(FogColor.rgb, ot.rgb, saturate(input.Fog));

    // Lava is opaque terrain: it writes depth and nothing blends against it.
    return float4(ot.rgb, 1.0f);
}
