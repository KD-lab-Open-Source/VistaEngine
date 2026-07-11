// Skinned-object (3dx) fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Skin/object_scene_light.psl -- the psSkin and psSkinBump
// paths, with REFLECTION, SECOND_OPACITY_TEXTURE, SHADOW, LIGHTMAP, FOG_OF_WAR,
// ZREFLECTION and ZBUFFER all off. What survives is the whole of the original's
// composition for those two configurations.
//
// BUMP=0 (psSkin), lit per vertex:
//
//     t0.rgb  = t0.rgb*tLerpPre.a + tLerpPre.rgb;   // LERP_TEXTURE_COLOR (skin colour)
//     ot.rgb  = t0.rgb*v.diffuse;                   // lit per-vertex diffuse
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
//     ot.rgb += bumpAmbient*t0;  ot.rgb += v.specular;
//     ot.a    = t0.a*bumpDiffuse.a;
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

Texture2D<float4> DiffuseTexture : register(t0, space2);
SamplerState      DiffuseSampler : register(s0, space2);
#if BUMP
Texture2D<float4> BumpTexture     : register(t1, space2);
SamplerState      BumpSampler     : register(s1, space2);
Texture2D<float4> SpecularTexture : register(t2, space2);
SamplerState      SpecularSampler : register(s2, space2);
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
};

struct VSOutput
{
    float4 Position : SV_Position;
#if BUMP
    float3 LightObj : TEXCOORD1;
    float3 HalfObj  : TEXCOORD2;
#else
    float4 Diffuse  : COLOR0;
#endif
    float3 Specular : COLOR1;
    float2 UV       : TEXCOORD0;
};

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

    return ot;
}
