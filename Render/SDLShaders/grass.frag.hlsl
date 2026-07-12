// Grass fragment shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/Grass/Grass.psl (the PSGrass and
// PSGrassShadow classes), which composes, in order: the blade texture, the lightmap, the
// per-vertex lit colour, and the shadow.
//
//     float4 ot = tex2D(t0, v.uv0);
//     float4 lightmap = tex2D(LightMapSampler, v.uv_lightmap);   // #ifdef LIGHTMAP
//     lightmap.rgb = (lightmap.rgb - 0.5);                       // #ifdef LIGHTMAP
//     float3 diffuse = v.color;
//     diffuse += lightmap;  diffuse = saturate(diffuse);         // #ifdef LIGHTMAP
//     ot.rgb *= diffuse;
//     ot.a   *= v.color.a;
//     Shadow(ot.rgb, ShadowSampler, v.shadow, 1);                // #if SHADOW_9700
//     return ot;
//
// Note the lightmap is applied as `lm - 0.5`, NOT the `2*(lm - 0.5)` the terrain uses
// (tilemap.frag.hlsl) -- the original really is inconsistent between the two, and this is
// deliberately faithful to the grass one. It is also saturated here and not there.
//
// The alpha test is the other half of what makes grass work, and it is not a blend state
// we can set: D3D9's D3DRS_ALPHAFUNC/D3DRS_ALPHAREF have no SDL GPU equivalent, so the
// clip() below IS the alpha test. GrassMap::DrawGrass sets ALPHAREF 100, so a texel has to
// be at least 100/255 opaque to survive -- which is also what hides a distant blade, since
// its alpha carries the distance fade the vertex shader computed.
//
// The FOG_OF_WAR path is not ported (Render/PORTING.md #10), and neither is the ZBUFFER
// one (PORTING.md #12).
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-grass-shaders.sh.

// The grass atlas (the original's t0/s0): every blade texture in one image, which is why
// GrassMap::CalcVertex bakes each bush's frame rectangle into the vertex uv. Sampled with
// sampler_clamp_anisotropic, as GrassMap::DrawGrass asks.
Texture2D<float4> GrassTexture  : register(t0, space2);
SamplerState      GrassSampler  : register(s0, space2);
// The shadow map. A D32_FLOAT depth texture sampled as a plain texture -- the compare is
// done by hand, as the original's is -- so its sampler must be point/clamp.
Texture2D<float>  ShadowTexture : register(t1, space2);
SamplerState      ShadowSampler : register(s1, space2);
// The terrain lightmap (the original's LightMapSampler, s3), which CameraPlanarLight draws
// the scene's light sources and circle shadows into. Its neutral is mid-grey, hence the
// signed `- 0.5`.
Texture2D<float4> LightMapTexture : register(t2, space2);
SamplerState      LightMapSampler : register(s2, space2);

cbuffer Constants : register(b0, space3)
{
    // The original's vShade (psl c2): cScene::GetShadowIntensity(), what a fully shadowed
    // pixel is multiplied by.
    float4 ShadeIntensity;
    // x != 0: the shadow map holds this frame's casters AND this draw receives shadows
    //         (GrassMap::DrawGrass picks psGrassShadow over psGrass on exactly that test).
    // y != 0: 2x2 filter -- the original's FILTER_SHADOW static define, Option_filterShadow.
    float4 ShadowParams;
    // x != 0: the lightmap holds this frame's light sources. The original has no such flag
    // (LIGHTMAP is a define and the map always exists once the device is up); ours may not
    // have been created, or its pass may not have run.
    float4 LightMapParams;
    // x: the alpha-test reference, D3DRS_ALPHAREF/255. See the note on clip() above.
    float4 Params;
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float4 Color      : COLOR0;
    float2 UV         : TEXCOORD0;
    float4 ShadowPos  : TEXCOORD1;
    float2 LightmapUV : TEXCOORD2;
};

// shadow9700.inl's `#define ccx 0.0005`: the 2x2 tap offset, in shadow-map uv. The same
// constant, and the same reasoning, as tilemap.frag.hlsl -- see the long note there.
static const float SHADOW_TAP = 0.0005f;

// Shadow9700. 1 where the light reaches, 0 where it does not, quarter-steps under the filter.
float shadowLit(float4 shadowPos)
{
    float3 sh = shadowPos.xyz / shadowPos.w;

    // Outside the light's frustum there is nothing to compare against -- the map is fitted
    // to the view frustum each frame -- so treat it as lit rather than let the clamped edge
    // texel smear a shadow outward.
    if(!all(sh.xy == saturate(sh.xy)) || sh.z > 1.0f)
        return 1.0f;

    if(ShadowParams.y == 0.0f)
        return (ShadowTexture.Sample(ShadowSampler, sh.xy) - sh.z > 0.0f) ? 1.0f : 0.0f;

    const float c = SHADOW_TAP;
    float4 taps;
    taps.x = ShadowTexture.Sample(ShadowSampler, sh.xy + float2(-c, -c)) - sh.z;
    taps.y = ShadowTexture.Sample(ShadowSampler, sh.xy + float2( c,  c)) - sh.z;
    taps.z = ShadowTexture.Sample(ShadowSampler, sh.xy + float2(-c,  c)) - sh.z;
    taps.w = ShadowTexture.Sample(ShadowSampler, sh.xy + float2( c, -c)) - sh.z;
    return dot(step(0.0f, taps), 0.25f);
}

float4 main(VSOutput input) : SV_Target0
{
    float4 ot = GrassTexture.Sample(GrassSampler, input.UV);

    // The vertex shader already lit the blade; the lightmap is a signed offset on top of
    // that, so its neutral mid-grey adds nothing, a light source brightens and a circle
    // shadow darkens.
    float3 diffuse = input.Color.rgb;
    if(LightMapParams.x != 0.0f)
    {
        float3 lm = LightMapTexture.Sample(LightMapSampler, input.LightmapUV).rgb;
        diffuse = saturate(diffuse + (lm - 0.5f));
    }

    ot.rgb *= diffuse;
    ot.a   *= input.Color.a;   // the distance fade the vertex shader computed

    // The alpha test. Must come after the fade is folded in: that is what makes a blade
    // vanish as it passes the hide distance, rather than lingering as a ghost.
    clip(ot.a - Params.x);

    // Shadow(ot.rgb, ..., 1): grass passes k = 1, unlike the terrain, which fades its shadow
    // out on ground already turned away from the sun.
    if(ShadowParams.x != 0.0f)
    {
        float lit = shadowLit(input.ShadowPos);
        ot.rgb *= ShadeIntensity.rgb * (1.0f - lit) + lit;
    }

    return ot;
}
