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
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
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
    return ot;
}
