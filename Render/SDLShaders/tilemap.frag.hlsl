// Tilemap (terrain) fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Minimal/tile_map_scene.psl. That shader composes, in
// order: a light term, the surface colour, a lightmap, an optional detail texture,
// an optional shadow, and optional fog-of-war. This is the base of that stack -- the
// #ifdef VERTEX_LIGHT path with no lightmap, detail, shadow or fog-of-war:
//
//     o.color.rgb = -dot(v.n*2-1, vLightDirection);      // vsl
//     o.color.rgb = o.color.rgb*vColor.rgb + vColor.w;   // vsl
//     float3 light = v.color;                            // psl
//     float4 ot = tex2D(ColorSampler, v.uv_color);       // psl
//     ot.rgb *= light;                                   // psl
//
// Kept faithfully: the lambert is against the *negated* light direction, because
// vLightDirection points the way the light travels; and the light term is
// `ndl * diffuse + ambient`, so the ambient is a floor added on top rather than a
// blend factor. The one addition is saturate() on the dot -- the original leans on
// the fixed-function clamp of the older pixel pipeline, and without it a surface
// facing away from the sun drives the light term below its ambient floor.
//
// The shadow is the SHADOW_9700 branch of the original's Shadow() (shader/Skin/
// shadow9700.inl):
//
//     mul  = Shadow9700(sh_sampler, shadow);   // one tap, or 2x2 under FILTER_SHADOW
//     mul *= shadowFactor;
//     mul  = vShade.rgb*(1-mul) + mul;
//     ot.rgb *= mul;
//
// D3D9 could not sample a depth buffer, so the original's caster writes its light-space
// z into a float colour target; we sample the depth texture itself, which holds exactly
// that. `mul == 1` means lit; `mul == 0` darkens the pixel to vShade, the scene's shadow
// intensity. shadowFactor is the original's per-vertex `smoothstep(0.15, 0.2, N.L)`: it
// fades the shadow out on terrain already turned away from the sun.
//
// The remaining layers (bump, lightmap, detail texture, fog of war, fog) come back as
// this renderer grows; the original's structure is the map for that.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-tilemap-shaders.sh.

Texture2D<float4> ColorTexture  : register(t0, space2);
SamplerState      ColorSampler  : register(s0, space2);
// The shadow map. A D32_FLOAT depth texture sampled as a plain texture -- the compare is
// done by hand, as the original's is -- so its sampler must be point/clamp: filtering
// depth values before comparing them is meaningless.
Texture2D<float>  ShadowTexture : register(t1, space2);
SamplerState      ShadowSampler : register(s1, space2);

cbuffer Light : register(b0, space3)
{
    // The original's vColor (vsl c2): rgb = diffuse light colour, w = ambient. Fed from
    // the scene sun via cTileMap::GetDiffuse(); rgb may exceed 1 for a bright sun.
    float4 LightColor;
    // The original's vLightDirection (vsl c5): unit vector pointing the way the light
    // travels, so a surface facing the light has dot(N, dir) == -1. This is the scene's
    // sun_direction, read through Camera::GetLighting().
    float4 LightDirection;
    // The original's vShade (psl c2): cScene::GetShadowIntensity(), the colour a fully
    // shadowed pixel is multiplied by.
    float4 ShadeIntensity;
    // x != 0: the shadow map holds this frame's casters. y != 0: 2x2 filter (the
    // original's FILTER_SHADOW, a static shader define there, Option_filterShadow here).
    float4 ShadowParams;
};

struct VSOutput
{
    float4 Position  : SV_Position;
    float3 Normal    : NORMAL;
    float2 UV        : TEXCOORD0;
    float4 ShadowPos : TEXCOORD1;
};

// shadow9700.inl's `#define ccx 0.0005`: the 2x2 tap offset, in shadow-map uv. Almost
// exactly one texel of a 2048 map, and 2048 is the only size the filter is ever on at --
// GameOptions::graphSetup turns FILTER_SHADOW on and sets Option_ShadowSizePower to 4
// together, both only when OPTION_SHADOW == 2 ("good").
static const float SHADOW_TAP = 0.0005f;

// Shadow9700. Returns 1 where the light reaches, 0 where it does not, or a quarter-step
// between the two under the filter.
//
// Note z is divided by w, where the original divides only xy. Its caster stores the
// pre-divide clip z in a colour target, so both sides stay pre-divide; ours stores
// hardware depth, which is z/w. With Option_shadowTSM the light matrix is a perspective
// warp and w != 1, so the difference is real.
float shadowLit(float4 shadowPos)
{
    float3 sh = shadowPos.xyz / shadowPos.w;

    // Outside the light's frustum there is no depth to compare against: the map is
    // fitted to the view frustum each frame, so this is the far edge of the terrain.
    // Treat it as lit rather than let the clamped edge texel smear a shadow outward.
    if(!all(sh.xy == saturate(sh.xy)) || sh.z > 1.0f)
        return 1.0f;

    if(ShadowParams.y == 0.0f)
        return (ShadowTexture.Sample(ShadowSampler, sh.xy) - sh.z > 0.0f) ? 1.0f : 0.0f;

    // Shadow97002x2: the four corners of a texel, averaged. Its compare is `>=` where the
    // unfiltered one is `>` -- step() gives exactly that.
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
    float3 N = normalize(input.Normal);
    float ndlRaw = -dot(N, LightDirection.xyz);
    float3 light = saturate(ndlRaw) * LightColor.rgb + LightColor.a;

    float4 ot = ColorTexture.Sample(ColorSampler, input.UV);
    ot.rgb *= light;

    if(ShadowParams.x != 0.0f)
    {
        // Shadow(ot.rgb, ShadowSampler, v.shadow, v.shadowFactor): the tilemap is the one
        // caller that passes a real k, fading the shadow out on terrain already turned
        // away from the sun. Objects pass 1.
        float lit = shadowLit(input.ShadowPos);
        lit *= smoothstep(0.15f, 0.2f, ndlRaw);
        ot.rgb *= ShadeIntensity.rgb * (1.0f - lit) + lit;
    }

    // Terrain is opaque base geometry: it clears and writes depth, and nothing
    // blends against it, so the alpha it carries is irrelevant. Emit 1.
    return float4(ot.rgb, 1.0f);
}
