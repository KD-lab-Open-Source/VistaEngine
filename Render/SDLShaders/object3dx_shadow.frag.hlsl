// Shadow-caster fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Skin/object_shadow.psl. That shader exists to write depth
// into a float colour target (`return (float4)v.tdepth`) because D3D9 could not sample a
// depth buffer. We render into a depth texture directly, so the only job left is the
// original's `clip(o.a - 0.32)`: cutout foliage must not cast a solid rectangle.
//
// The pass has no colour target, so this returns void. The constant buffer matches
// object3dx.frag.hlsl exactly -- one SetState pushes both -- but only Params.x (the
// alpha-test reference, 0 when the material is not alpha tested) and Params.y (textured)
// are read.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-object3dx-shaders.sh.

Texture2D<float4> DiffuseTexture : register(t0, space2);
SamplerState      DiffuseSampler : register(s0, space2);

cbuffer Material : register(b0, space3)
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 tLerpPre;
    float4 Params;     // x = alpha-test reference (0 = no test), y != 0 = textured
    float4 Params2;
    float4 ShadeIntensity;
    float4 ShadowParams;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

void main(VSOutput input)
{
    if(Params.y != 0.0f && Params.x > 0.0f)
        clip(DiffuseTexture.Sample(DiffuseSampler, input.UV).a - Params.x);
}
