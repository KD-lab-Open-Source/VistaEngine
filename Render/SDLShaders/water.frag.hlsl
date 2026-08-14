// Water-surface fragment shader for the SDL GPU backend.
//
// Three techniques, selected by -DREFLECTION / -DCUBE.
//
// REFLECTION=0 -- the original's water_easy.psl (WATER_EMPTY), whole:
//
//     ot.rgb = vPS11Color;
//     ot.a   = v.diffuse.a;
//     ot.a  += ot.a*saturate((tex0.x+tex1.x));
//
// So the surface is a flat colour whose *opacity* carries the waves: vPS11Color is
// cWater's cur_reflect_sky_color (the reflected-sky colour for the current time of day,
// which the engine itself substitutes for a reflection here), the vertex alpha is the
// depth-derived opacity cWater::CalcColor bakes per grid node, and the wave maps then
// thicken it along the crests -- shallow water reads through, deep water does not, and
// the waves ripple across both.
//
// REFLECTION=1 -- the original's water_linear.psl (WATER_LINEAR_REFLECTION). The flat
// colour becomes a projective sample of the reflection render target, perturbed along
// the wave slopes; the target's *alpha* says how much of what was reflected was open sky
// (the clear leaves 0 there, and every caster writes 1), which brightens the reflection
// by fBrightnes. Then a sun glint: reflect the light direction about the wave normal and
// take a very tight smoothstep against the eye vector. The glint also thins the surface,
// which is what `ot.a = v.diffuse.a*(1+light)` does.
//
// CUBE=1 -- the original's water_cube.psl (WATER_REFLECTION), whole:
//
//     float3 cube = v.uv_mirror;
//     cube.xy += (tex0.xy + tex1.xy)*0.3;
//     float4 sky = texCUBE(SkySampler, cube);
//     ot.rgb = sky.rgb*vReflectionColor.a + vReflectionColor.rgb;
//     ot.a   = v.diffuse.a;
//     ot.a  += ot.a*saturate((tex0.x + tex1.x));
//
// This is what the original draws when the reflection option is OFF -- water_easy is the
// no-PS2.0 path, not the option-off one. Note what is *not* here: no glint term, and no
// brightness. The sun visible on the water is the sun rendered into the sky cubemap
// (cRenderSky), and it moves across the surface because the wave slopes shift the lookup
// direction; the crest term is water_easy's, on the alpha. So the colour is a reflection
// and the opacity is the depth gradient, each carrying half the look.
//
// Three departures, all forced:
//
//  * tex0.xy / tex1.xy. The wave maps (waves.dds / waves1.dds) are D3DFMT_V8U8: two
//    signed bytes, so the original's tex0.xy is already a signed slope in [-1,1]. The
//    portable DDS decoder (Render/src/DDSImage.cpp) stores those bytes biased by 128 into
//    R,G of a BGRA8 texture, so unbias here -- (raw*255 - 128)/127 -- exactly as
//    object3dx.frag.hlsl does.
//
//  * No FOG_OF_WAR. That branch lerps to vFogOfWar by a lightmap alpha; the SDL backend
//    has no lightmap yet, and the original compiles the branch out without one too.
//
//  * No FLOAT_ZBUFFER. That branch fades the surface out along the shoreline against a
//    float depth map the SDL backend does not render.
//
// ot.a exceeds 1 on a crest (the original's `ot.a += ot.a*...` doubles it at most, and
// the glint's `1+light` likewise). Both D3D9 and SDL GPU clamp a fragment's output to
// [0,1] before blending into a UNORM target, so the saturation is the hardware's, there
// and here. Left unclamped, as the original.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-water-shaders.sh.

Texture2D<float4> Tex0        : register(t0, space2);
SamplerState      Tex0Sampler : register(s0, space2);
Texture2D<float4> Tex1        : register(t1, space2);
SamplerState      Tex1Sampler : register(s1, space2);
#if REFLECTION
// The reflection camera's render target, sampled projectively. The original's stage 2,
// with sampler_clamp_anisotropic.
Texture2D<float4> Sky         : register(t2, space2);
SamplerState      SkySampler  : register(s2, space2);
#endif
#if CUBE
// The sky cubemap (cScene::GetSkyCubemap), the same stage 2 -- the original binds it there
// with sampler_wrap_linear.
TextureCube<float4> SkyCube       : register(t2, space2);
SamplerState        SkyCubeSampler : register(s2, space2);
#endif

cbuffer Water : register(b0, space3)
{
    // The original's vPS11Color (PSWater::SetPS11Color), fed cWater's
    // cur_reflect_sky_color. Alpha unused: the surface's comes from the vertex.
    // REFLECTION=0 only.
    float4 PS11Color;
    // vReflectionColor (PSWater::SetReflectionColor): rgb is the water's own tint,
    // premultiplied by its weight; a is what is left for the reflection itself.
    float4 ReflectionColor;
    // vLightColor: the scene's sun diffuse, with cWater's flashIntensity_ in alpha.
    float4 LightColor;
    float4 LightDirection;   // vLightDirection: scene lighting direction, xyz
    float4 CameraPos;        // vCameraPos: the main camera's world position, xyz
    float4 Params;           // x = fBrightnes (PSWater::SetReflectionBrightnes)
    // Distance fog: D3DRS_FOGCOLOR. The factor arrives interpolated, in VSOutput::Fog.
    float4 FogColor;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Diffuse  : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
#if REFLECTION
    float4 UVSky    : TEXCOORD2;
    float3 PointPos : TEXCOORD3;
#endif
    float  Fog      : TEXCOORD4;
#if CUBE
    float3 Mirror   : TEXCOORD5;
#endif
};

// One wave map's signed slope, undoing the decoder's +128 bias. The original reads the
// V8U8 texel directly; .x alone drives the easy technique's crests, .xy the normal.
float2 slope(Texture2D<float4> tex, SamplerState samp, float2 uv)
{
    return (tex.Sample(samp, uv).rg * 255.0f - 128.0f) / 127.0f;
}

float4 main(VSOutput input) : SV_Target0
{
    float2 tex0 = slope(Tex0, Tex0Sampler, input.UV0);
    float2 tex1 = slope(Tex1, Tex1Sampler, input.UV1);

#if CUBE
    float4 ot;

    // Perturb the reflection direction by the wave slopes, then reflect the sky. The 0.3
    // is the original's: far larger than the planar path's 0.03, because this offsets a
    // direction vector rather than a projective uv.
    float3 cube = input.Mirror;
    cube.xy += (tex0 + tex1) * 0.3f;
    float4 sky = SkyCube.Sample(SkyCubeSampler, cube);

    ot.rgb = sky.rgb * ReflectionColor.a + ReflectionColor.rgb;
    ot.a   = input.Diffuse.a;
    ot.a += ot.a * saturate(tex0.x + tex1.x);
#elif REFLECTION
    float4 ot;

    // Ripple the projective lookup along the wave slopes. Scaled by w so the offset is
    // constant in screen space after the divide, as the original does.
    float4 uvSky = input.UVSky;
    uvSky.xy += (tex0 + tex1) * uvSky.w * 0.03f;
    float4 sky = Sky.Sample(SkySampler, uvSky.xy / uvSky.w);

    ot.rgb = sky.rgb * ReflectionColor.a + ReflectionColor.rgb;
    ot.rgb = saturate(ot.rgb * ((1.0f - sky.a) * Params.x + 1.0f));

    // The sun glinting off a wave face: reflect the light about the wave normal, and
    // light up where that points straight back at the eye.
    float3 n = normalize(float3((tex0 + tex1) * 0.5f, 1.0f));
    float3 lightDir = LightDirection.xyz;
    float3 lightMirror = lightDir - 2.0f * dot(n, lightDir) * n;
    float3 eye = normalize(input.PointPos - CameraPos.xyz);

    float light = smoothstep(0.99f, 1.0f, -dot(lightMirror, eye)) * LightColor.a;
    ot.rgb += light * LightColor.rgb;
    ot.a = input.Diffuse.a * (1.0f + light);
#else
    float4 ot;
    ot.rgb = PS11Color.rgb;
    ot.a   = input.Diffuse.a;
    ot.a += ot.a * saturate(tex0.x + tex1.x);
#endif

    // Fog last, before the blend, as D3D9's fixed function applied it. Colour only: the
    // alpha is the water's own depth-driven opacity, which fog must not touch.
    ot.rgb = lerp(FogColor.rgb, ot.rgb, saturate(input.Fog));
    return ot;
}
