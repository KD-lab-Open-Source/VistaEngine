// The KD-lab logo splash's metaball composite for the SDL GPU backend.
//
// A verbatim port of Render/shader/PostProcessing/blobs.psl's `#if(TYPE==3)` branch -- the
// only branch the game compiled (the file hard-codes `#define TYPE 3`; 0..2 are debug
// views of the field). PSBlobsShader::Select fed it the four constants below, the metaball
// field on stage 0 and the frame on stage 1.
//
// The field is the accumulation target SDLBlobsRenderer fills first: one additive
// falloff sprite per cell, so `Field.x` is how much metaball covers this pixel.
//
//   mul     = smoothstep(0.05, 0.2, field)   -- the metaball's edge, softened
//   t2 - t1 = the field's gradient one texel right and one texel down, which displaces
//             the frame lookup: the blobs act as lenses over the scene behind them
//   out1    = that displaced frame, tinted toward BlobsColor.rgb by BlobsColor.a
//   specUp  = the field 8 texels below, cubed -- a highlight along each blob's upper rim
//
// Two things read oddly and are the original's:
//   * `mul ? out1 : out2` is a hard comparison against zero, NOT a lerp. Every pixel the
//     field touches at all takes the tinted, displaced path; the rest take the frame
//     untouched. The soft `mul` is spent on the final `lerp(tout*0.8, tout, mul)`
//     darkening and on the highlight, not on the tint.
//   * the displacement is a scalar added to BOTH uv components.
//
// The original's dead `float mulnew = mul - 0.5;` is dropped -- nothing read it.

Texture2D<float4> Field        : register(t0, space2);
SamplerState      FieldSampler : register(s0, space2);
Texture2D<float4> Scene        : register(t1, space2);
SamplerState      SceneSampler : register(s1, space2);

cbuffer Constants : register(b0, space3)
{
    float4 PixelSize;       // .xy = 1/field width, 1/field height (PSBlobsShader's pixel_size)
    float4 BlobsColor;      // cBlobsSetting::color_: .rgb the tint, .a how much of it
    float4 SpecularColor;   // cBlobsSetting::specularColor_, .a forced to 1 as Select did
    float4 FadePhase;       // .x = the splash's fade in/out, multiplying the whole frame
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

// blobs.psl's clamp_smooth().
float clampSmooth(float f)
{
    return smoothstep(0.05f, 0.2f, f);
}

float4 main(VSOutput input) : SV_Target0
{
    const float2 uv = input.UV;

    float4 t1 = Field.Sample(FieldSampler, uv);
    float mul = clampSmooth(t1.x);

    float4 t2 = (Field.Sample(FieldSampler, uv + float2(0.0f, PixelSize.y)) +
                 Field.Sample(FieldSampler, uv + float2(PixelSize.x, 0.0f))) * 0.5f;

    float4 out1 = Scene.Sample(SceneSampler, uv + (t2.x - t1.x) * 0.15f) * (1.0f - BlobsColor.a)
                + float4(BlobsColor.rgb, 1.0f) * BlobsColor.a;
    float4 out2 = Scene.Sample(SceneSampler, uv);

    float4 tout = (mul != 0.0f) ? out1 : out2;

    // The rim highlight: the field below this pixel, cubed, so only the dense middle of a
    // blob throws one and it lands along the edge above.
    float4 specUp = Field.Sample(FieldSampler, uv + float2(0.0f, PixelSize.y * 8.0f));
    tout += specUp.x * specUp.x * specUp.x * mul * SpecularColor;

    // Darken everything the blobs do not cover to 80%, so they read as raised out of it.
    tout = lerp(tout * 0.8f, tout, mul);

    return tout * FadePhase.x;
}
