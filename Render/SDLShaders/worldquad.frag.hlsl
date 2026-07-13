// World-quad fragment shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/NoMaterial/standart.psl -- the pixel
// half of vsStandart/psStandart, which cD3DRender::SetWorldMaterial selects. With
// TFACTOR, COLOR_OPERATION (no second texture), FOG_OF_WAR, FLOAT_ZBUFFER and
// ZREFLECTION all off -- the configuration every caller here draws in -- the whole shader
// is:
//
//     float4 ot = tex2D(t0, v.uv0);
//     ot *= v.color;
//     return ot;
//
// The vertex colour is the tilemap's lit diffuse with a per-quad fade in alpha: the
// sprite's triangle wave for cCoastSprites, the wave's phase for cFixedWaves.
//
// One departure, forced by the decoder rather than chosen: the textures arrive
// premultiplied (Render/src/DDSImage.cpp premultiplies every colour DDS, so bilinear
// filtering cannot bleed transparent-texel RGB into the opaque edges). A premultiplied
// texel must be blended (ONE, 1-SRC_ALPHA), not the original's (SRC_ALPHA, 1-SRC_ALPHA).
// Multiplying by a vertex colour that worldquad.vert.hlsl has likewise premultiplied keeps
// the product premultiplied, and the two blends then emit identical pixels.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-worldquad-shaders.sh.

Texture2D<float4> Tex0        : register(t0, space2);
SamplerState      Tex0Sampler : register(s0, space2);

cbuffer Params : register(b0, space3)
{
    // .x != 0: take the colour from the vertex alone and the texture for its alpha only.
    // That is the fixed-function stage the terrain lightmap's circle shadows set --
    // D3DTSS_COLOROP = D3DTOP_SELECTARG2 (arg2 being DIFFUSE), with the alpha op left at
    // its MODULATE default (CameraPlanarLight::drawLights). The spherical texture supplies
    // the falloff through its alpha; its rgb is not wanted.
    float4 SelectDiffuse;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 t = Tex0.Sample(Tex0Sampler, input.UV);

    if(SelectDiffuse.x != 0.0f)
    {
        // rgb = diffuse, a = texture.a * diffuse.a -- premultiplied, as this pipeline's
        // (ONE, 1-SRC_ALPHA) blend wants. input.Color is already the vertex colour times
        // its own alpha, so multiplying it by the texture's alpha is exactly
        // raw_rgb * (t.a * diffuse.a), the premultiplied form of the pixel D3D emits.
        return float4(input.Color.rgb * t.a, input.Color.a * t.a);
    }

    return t * input.Color;
}
