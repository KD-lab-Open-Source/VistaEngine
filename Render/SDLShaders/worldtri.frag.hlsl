// World-triangle fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/NoMaterial/standart.psl, the pixel half of vsStandart/
// psStandart. This is the half worldquad.frag.hlsl leaves out: the second texture and its
// four colour operations, which cD3DRender::SetWorldMaterial maps from its eColorMode
// argument (COLOR_ADD -> 1, COLOR_MOD -> 2, COLOR_MOD2 -> 3, COLOR_MOD4 -> 4, and 0 when
// there is no second texture). With TFACTOR, FOG_OF_WAR, FLOAT_ZBUFFER and ZREFLECTION off
// -- the configuration cEmitterColumnLight draws in -- the original is exactly:
//
//     float4 ot = tex2D(t0, v.uv0);
//     ot *= v.color;
//     if(COLOR_OPERATION==1) ot.rgb += tex2D(t1, v.uv1);        // add
//     if(COLOR_OPERATION==2) ot.rgb *= tex2D(t1, v.uv1);        // mod
//     if(COLOR_OPERATION==3) ot.rgb *= tex2D(t1, v.uv1)*2;      // mod2x
//     if(COLOR_OPERATION==4) ot.rgb *= tex2D(t1, v.uv1)*4;      // mod4x
//     return ot;
//
// COLOR_OPERATION is a shader define in the original (one variant per value); here it is a
// uniform, so it costs no pipeline variant. The second sampler is always declared and
// always bound -- a white 1x1 when there is no second texture -- so the pipeline's sampler
// count is fixed.
//
// The premultiplied caveat: t0 and the vertex colour are premultiplied (see
// worldquad.frag.hlsl), and so is t1, since every texture goes through the same decoder.
// The colour operations read t1.rgb only, and the original read it raw. They agree while
// t1 is opaque, which a modulate/detail map is; a t1 with real alpha would come out darker
// than on D3D.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-worldtri-shaders.sh.

Texture2D<float4> Tex0        : register(t0, space2);
SamplerState      Tex0Sampler : register(s0, space2);
Texture2D<float4> Tex1        : register(t1, space2);
SamplerState      Tex1Sampler : register(s1, space2);

cbuffer Params : register(b0, space3)
{
    float4 ColorOp;   // .x = COLOR_OPERATION (0 none, 1 add, 2 mod, 3 mod2x, 4 mod4x)

    // Distance fog: D3DRS_FOGCOLOR, and which of the two fog rules this group takes.
    // The factor itself arrives interpolated, in VSOutput::Fog.
    float4 FogColor;
    // x == 0: an OCCLUDER (ALPHA_NONE / ALPHA_TEST / ALPHA_BLEND / ALPHA_MUL). It hides what
    //         is behind it, so it fades toward the fog colour -- what D3D9's fixed function
    //         did to every pixel. Note the lerp below is written for a PREMULTIPLIED source:
    //         D3D fogged the raw colour and then multiplied by alpha, so the equivalent here
    //         is lerp(FogColor * a, rgb, f) -- the fog colour has to be premultiplied too, or
    //         a transparent pixel would fog to a solid one.
    // x != 0: a CONTRIBUTION (ALPHA_ADDBLEND / ALPHA_ADDBLENDALPHA / ALPHA_SUBBLEND). It adds
    //         to, or takes from, what is behind it, so it must fade to NOTHING -- lerping it
    //         toward the fog colour would add the fog colour to the frame and make a distant
    //         particle glow. This is the original's FIX_FOG_ADD_BLEND, which scaled the vertex
    //         alpha by the fog factor; its src blend factor was SRC_ALPHA, so that scaled the
    //         contribution. Ours is ONE over a premultiplied source, so scale the whole thing.
    float4 FogParams;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
    float  Fog      : TEXCOORD2;
};

float4 main(VSOutput input) : SV_Target0
{
    float4 ot = Tex0.Sample(Tex0Sampler, input.UV0);
    ot *= input.Color;

    const int op = (int)ColorOp.x;
    if(op != 0)
    {
        const float3 t1 = Tex1.Sample(Tex1Sampler, input.UV1).rgb;
        if(op == 1)
            ot.rgb += t1;
        else if(op == 2)
            ot.rgb *= t1;
        else if(op == 3)
            ot.rgb *= t1 * 2.0f;
        else
            ot.rgb *= t1 * 4.0f;
    }

    // Fog, on a premultiplied pixel. Two rules -- see the cbuffer note above. The light
    // columns and laser beams that take this route are ALPHA_ADDBLENDALPHA, so in practice
    // they fade out with distance rather than toward the fog colour.
    const float f = saturate(input.Fog);
    if(FogParams.x != 0.0f)
        ot *= f;
    else
        ot.rgb = lerp(FogColor.rgb * ot.a, ot.rgb, f);

    return ot;
}
