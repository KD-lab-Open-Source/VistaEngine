// Skinned-object (3dx) fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Skin/object_scene_light.psl -- the psSkin path, with
// BUMP, REFLECTION, SECOND_OPACITY_TEXTURE, SHADOW, LIGHTMAP, FOG_OF_WAR, ZREFLECTION
// and ZBUFFER all off. What survives is the whole of the original's composition for
// that configuration:
//
//     t0.rgb  = t0.rgb*tLerpPre.a + tLerpPre.rgb;   // LERP_TEXTURE_COLOR (skin colour)
//     ot.rgb  = t0.rgb*v.diffuse;                   // lit per-vertex diffuse
//     ot.rgb += bumpAmbient*t0;                     // ambient
//     ot.rgb += v.specular;
//     ot.a    = t0.a*v.diffuse.a;                   // or SELF_ILLUMINATION's lerp
//
// The original's NOLIGHT branch skips that ambient term (its vertex diffuse already
// *is* the ambient colour). We don't carry a NOLIGHT flag here: the renderer pushes
// Ambient = 0 for a no_light material, which collapses the term to nothing. Same for
// LERP_TEXTURE_COLOR, whose shader variant we replace with Params.w.
//
// Alpha test was fixed-function state in the original (D3DRS_ALPHAREF = 80, set by
// SetBlendStateAlphaRef for blend == ALPHA_TEST). SDL GPU has no such state, so the
// reference value arrives in Params.x and we clip.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-object3dx-shaders.sh.

Texture2D<float4> DiffuseTexture : register(t0, space2);
SamplerState      DiffuseSampler : register(s0, space2);

cbuffer Material : register(b0, space3)
{
    float4 Ambient;    // bumpAmbient (rgb); zero for a no_light material
    float4 tLerpPre;   // rgb = lerp.rgb*lerp.a, a = 1-lerp.a (PSSkin::SetMaterial)
    // x = alpha-test reference (0 = no test), y != 0 = textured,
    // z != 0 = SELF_ILLUMINATION, w != 0 = LERP_TEXTURE_COLOR
    float4 Params;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Diffuse  : COLOR0;
    float3 Specular : COLOR1;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 ot;

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

    if(Params.x > 0.0f)
        clip(ot.a - Params.x);

    return ot;
}
