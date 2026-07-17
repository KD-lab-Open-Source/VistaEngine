// Skinned-object (3dx) fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Skin/object_scene_light.psl -- the psSkin and psSkinBump
// paths (and their SHADOW_9700 variants, psSkinSceneShadow / psSkinBumpSceneShadow), with
// REFLECTION, SECOND_OPACITY_TEXTURE, LIGHTMAP, FOG_OF_WAR, ZREFLECTION and ZBUFFER all
// off. What survives is the whole of the original's composition for those configurations.
//
// BUMP=0 (psSkin), lit per vertex:
//
//     t0.rgb  = t0.rgb*tLerpPre.a + tLerpPre.rgb;   // LERP_TEXTURE_COLOR (skin colour)
//     ot.rgb  = t0.rgb*v.diffuse;                   // lit per-vertex diffuse
//     Shadow(ot.rgb, ShadowSampler, v.shadow, 1);   // SHADOW_9700
//     ot.rgb += bumpAmbient*t0;                     // ambient
//     ot.rgb += v.specular;
//     ot.a    = t0.a*v.diffuse.a;                   // or SELF_ILLUMINATION's lerp
//
// BUMP=1 (psSkinBump), lit per pixel against the bump map, in tangent space:
//
//     tbump.z = sqrt(1 - tbump.x^2 - tbump.y^2);
//     ot.rgb  = saturate(dot(tbump, v.light_obj))*bumpDiffuse;
//     ot.rgb *= t0;
//     ot.rgb += pow(saturate(dot(tbump, normalize(v.half_obj))), P)*S;
//     Shadow(ot.rgb, ShadowSampler, v.shadow, 1);
//     ot.rgb += bumpAmbient*t0;  ot.rgb += v.specular;
//     ot.a    = t0.a*bumpDiffuse.a;
//
// Note where Shadow() sits: the ambient and the per-vertex specular are added after it,
// so a fully shadowed surface keeps both. The original's NOLIGHT and NOTEXTURE branches
// skip Shadow() entirely, which the renderer reproduces by clearing ShadowParams.x.
//
// where (P, S) is (tspecular.a*50, tspecular.rgb) under SPECULARMAP and
// (bumpSpecular.w, bumpSpecular.rgb) otherwise. We keep SPECULARMAP as a uniform flag
// rather than a fourth shader; the sampler is bound to a 1x1 white when unused.
//
// The bump map is D3DFMT_V8U8 -- signed dU,dV, which D3D sampled straight into [-1,1].
// The portable DDS decoder (Render/src/DDSImage.cpp) stores those bytes biased by 128
// into R,G of a BGRA8 texture, so unbias here: (raw*255 - 128)/127.
//
// The original's NOLIGHT branch skips the ambient term (its vertex diffuse already *is*
// the ambient colour). We don't carry a NOLIGHT flag: the renderer pushes Ambient = 0
// for it, which collapses the term to nothing. Same for LERP_TEXTURE_COLOR, whose shader
// variant we replace with Params.w.
//
// Alpha test was fixed-function state in the original (D3DRS_ALPHAREF = 80, set by
// SetBlendStateAlphaRef for blend == ALPHA_TEST). SDL GPU has no such state, so the
// reference value arrives in Params.x and we clip.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-object3dx-shaders.sh.

#ifndef BUMP
#define BUMP 0
#endif
#ifndef REFLECTION
#define REFLECTION 0
#endif

Texture2D<float4> DiffuseTexture : register(t0, space2);
SamplerState      DiffuseSampler : register(s0, space2);
#if BUMP
Texture2D<float4> BumpTexture     : register(t1, space2);
SamplerState      BumpSampler     : register(s1, space2);
Texture2D<float4> SpecularTexture : register(t2, space2);
SamplerState      SpecularSampler : register(s2, space2);
// The shadow map, at the first slot free past the bump path's three. The original always
// has it on s2 because it never binds a specular map and a bump map at once.
Texture2D<float>  ShadowTexture   : register(t3, space2);
SamplerState      ShadowSampler   : register(s3, space2);
#elif REFLECTION
// The 2D environment map (the original's ReflectionSampler), then the shadow map after it.
Texture2D<float4> ReflectionTexture : register(t1, space2);
SamplerState      ReflectionSampler : register(s1, space2);
Texture2D<float>  ShadowTexture     : register(t2, space2);
SamplerState      ShadowSampler     : register(s2, space2);
#else
Texture2D<float>  ShadowTexture   : register(t1, space2);
SamplerState      ShadowSampler   : register(s1, space2);
#endif

cbuffer Material : register(b0, space3)
{
    float4 Ambient;    // bumpAmbient (rgb); zero for a no_light material
    float4 Diffuse;    // bumpDiffuse: the lit colour (BUMP only; a = opacity)
    float4 Specular;   // bumpSpecular: colour (rgb), specular power (w)
    float4 tLerpPre;   // rgb = lerp.rgb*lerp.a, a = 1-lerp.a (PSSkin::SetMaterial)
    // x = alpha-test reference (0 = no test), y != 0 = textured,
    // z != 0 = SELF_ILLUMINATION, w != 0 = LERP_TEXTURE_COLOR
    float4 Params;
    float4 Params2;    // x != 0 = SPECULARMAP
    // vShade (PSSkin::SetShadowIntensity, from cScene::GetShadowIntensity): what a fully
    // shadowed pixel's lit colour is multiplied by.
    float4 ShadeIntensity;
    // x != 0 = sample the shadow map, y != 0 = 2x2 filter (the original's FILTER_SHADOW,
    // a static shader define there, Option_filterShadow here).
    float4 ShadowParams;
    // Distance fog: D3DRS_FOGCOLOR. The factor arrives interpolated, in VSOutput::Fog.
    float4 FogColor;
    // reflectionAmount (PSSkin::SetReflection): rgb = reflect_amount * material diffuse,
    // the weight the environment map is added at. Read only by the REFLECTION variant.
    float4 ReflectAmount;
};

// shadow9700.inl's `#define ccx 0.0005`: the 2x2 tap offset, in shadow-map uv. That is
// almost exactly one texel of a 2048 map, and 2048 is the only size the filter is ever
// on at: GameOptions::graphSetup turns FILTER_SHADOW on and sets Option_ShadowSizePower
// to 4 together, both only when OPTION_SHADOW == 2 ("good"). So it stays a literal.
static const float SHADOW_TAP = 0.0005f;

struct VSOutput
{
    float4 Position  : SV_Position;
#if BUMP
    float3 LightObj  : TEXCOORD1;
    float3 HalfObj   : TEXCOORD2;
#else
    float4 Diffuse   : COLOR0;
#endif
    float3 Specular  : COLOR1;
    float2 UV        : TEXCOORD0;
    float4 ShadowPos : TEXCOORD3;
    float  Fog       : TEXCOORD4;
#if REFLECTION
    float2 Reflect   : TEXCOORD5;   // sphere-map UV, from the vertex shader
#endif
};

// Shadow9700 from Render/shader/Skin/shadow9700.inl. One deliberate difference: it
// divides only xy by w, because its caster wrote pre-divide clip z into a float colour
// target. We sample hardware depth, which *is* z/w, so z is divided too. That also makes
// the constant bias in shadowMatBias survive the divide as a constant -- exactly what
// that file's "bias нельзя передавать через матрицу из за TSM" comment complains about.
//
// Returns 1 where the light reaches, 0 where it does not, or a quarter-step between the
// two under the filter.
float shadowLit(float4 shadowPos)
{
    float3 sh = shadowPos.xyz / shadowPos.w;
    if(!all(sh.xy == saturate(sh.xy)) || sh.z > 1.0f)
        return 1.0f;   // outside the light's frustum: nothing recorded, so nothing casts

    if(ShadowParams.y == 0.0f)
        return (ShadowTexture.Sample(ShadowSampler, sh.xy) - sh.z > 0.0f) ? 1.0f : 0.0f;

    // Shadow97002x2: the four corners of a texel, averaged. Its compare is `>=` where the
    // unfiltered one is `>` -- step() gives exactly that. The taps read the point/clamp
    // sampler, so a tap that leaves the map repeats its edge, as the original's does.
    const float c = SHADOW_TAP;
    float4 taps;
    taps.x = ShadowTexture.Sample(ShadowSampler, sh.xy + float2(-c, -c)) - sh.z;
    taps.y = ShadowTexture.Sample(ShadowSampler, sh.xy + float2( c,  c)) - sh.z;
    taps.z = ShadowTexture.Sample(ShadowSampler, sh.xy + float2(-c,  c)) - sh.z;
    taps.w = ShadowTexture.Sample(ShadowSampler, sh.xy + float2( c, -c)) - sh.z;
    return dot(step(0.0f, taps), 0.25f);
}

// shadow9700.inl's Shadow(), with its k == 1: the objects' call site passes a constant,
// unlike the tilemap's, which fades the shadow out where the surface turns away anyway.
void applyShadow(inout float3 rgb, float4 shadowPos)
{
    if(ShadowParams.x == 0.0f)
        return;
    float lit = shadowLit(shadowPos);
    rgb *= ShadeIntensity.rgb * (1.0f - lit) + lit;
}

float4 main(VSOutput input) : SV_Target0
{
    float4 ot;

#if BUMP
    // BUMP always samples: the original has no NOTEXTURE variant of psSkinBump.
    float4 t0 = DiffuseTexture.Sample(DiffuseSampler, input.UV);
    if(Params.w != 0.0f)
        t0.rgb = t0.rgb * tLerpPre.a + tLerpPre.rgb;

    float3 tbump;
    tbump.xy = (BumpTexture.Sample(BumpSampler, input.UV).rg * 255.0f - 128.0f) / 127.0f;
    tbump.z = sqrt(saturate(1.0f - tbump.x*tbump.x - tbump.y*tbump.y));

    ot.rgb = saturate(dot(tbump, input.LightObj)) * Diffuse.rgb;
    float3 halfV = normalize(input.HalfObj);
    ot.rgb *= t0.rgb;

    if(Params2.x != 0.0f){
        float4 tspecular = SpecularTexture.Sample(SpecularSampler, input.UV);
        ot.rgb += pow(saturate(dot(tbump, halfV)), tspecular.a * 50.0f) * tspecular.rgb;
    }
    else
        ot.rgb += pow(saturate(dot(tbump, halfV)), Specular.w) * Specular.rgb;

    // Ambient and the vertex specular stay outside the shadow, as in the original.
    applyShadow(ot.rgb, input.ShadowPos);
    ot.rgb += Ambient.rgb * t0.rgb;
    ot.rgb += input.Specular;

    if(Params.z != 0.0f){    // SELF_ILLUMINATION
        ot.a = Diffuse.a;
        ot.rgb = lerp(ot.rgb, t0.rgb, t0.a);
    }
    else
        ot.a = t0.a * Diffuse.a;
#else
    if(Params.y == 0.0f){        // NOTEXTURE
        ot.rgb = input.Diffuse.rgb + input.Specular;
        ot.a = input.Diffuse.a;
    }
    else{
        float4 t0 = DiffuseTexture.Sample(DiffuseSampler, input.UV);

        if(Params.w != 0.0f)     // LERP_TEXTURE_COLOR: tint toward the skin colour
            t0.rgb = t0.rgb * tLerpPre.a + tLerpPre.rgb;

        ot.rgb = t0.rgb * input.Diffuse.rgb;
#if REFLECTION
        // The original adds the environment map before the shadow multiply and the ambient,
        // so a shadowed metal surface loses its reflection too. reflectionAmount already
        // folds in reflect_amount and the material's diffuse tint (a = 0).
        ot.rgb += ReflectionTexture.Sample(ReflectionSampler, input.Reflect).rgb * ReflectAmount.rgb;
#endif
        applyShadow(ot.rgb, input.ShadowPos);
        ot.rgb += Ambient.rgb * t0.rgb;
        ot.rgb += input.Specular;

        if(Params.z != 0.0f){    // SELF_ILLUMINATION: alpha is the illumination mask
            ot.a = input.Diffuse.a;
            ot.rgb = lerp(ot.rgb, t0.rgb, t0.a);
        }
        else
            ot.a = t0.a * input.Diffuse.a;
    }
#endif

    if(Params.x > 0.0f)
        clip(ot.a - Params.x);

    // Fog last, where D3D9's fixed function applied it: to the finished pixel, before the
    // blend. Colour only -- the alpha still carries the material's own transparency, so a
    // distant object recedes into the fog instead of being tinted by it.
    ot.rgb = lerp(FogColor.rgb, ot.rgb, saturate(input.Fog));

    return ot;
}
