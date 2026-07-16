// World-quad fragment shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/NoMaterial/standart.psl -- the pixel
// half of vsStandart/psStandart, which cD3DRender::SetWorldMaterial selects. With
// TFACTOR, COLOR_OPERATION (no second texture), FOG_OF_WAR and ZREFLECTION all off --
// the configuration every caller here draws in -- the whole shader is:
//
//     float4 ot = tex2D(t0, v.uv0);
//     ot *= v.color;
//     return ot;
//
// Plus FLOAT_ZBUFFER, the soft-particle fade (SetWorldMaterial's useZBuffer -- the
// emitters' softSmoke key and the coast sprites), which the original wrote as
//
//     float zb = tex2Dproj(ZbufferMap, v.zpos).r;
//     ot.a *= saturate((zb - v.zpos.z) * 0.1f);
//
// against a float texture a second scene walk had filled with pre-divide clip-space z
// (ZBuffer/tilemap_zbuffer.vsl and object_zbuffer.vsl both store o.p.z) -- D3D9 could not
// sample its own depth buffer. SDL GPU can, so SceneDepth below is a snapshot of the real
// depth buffer (cSDLRenderDevice::snapshotSceneDepth) and clipZOf() converts both its
// sample and this fragment's own hardware depth back into the original's clip-z units,
// which is what keeps the 0.1 falloff constant meaning what it meant on D3D9.
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
// The scene depth snapshot: a copy of the frame's D32_FLOAT depth buffer, taken after the
// opaque passes (terrain, objects, grass) and before the transparent ones -- so it holds
// exactly what the original's float-Z camera rendered. Sampled as a plain texture with a
// nearest/clamp sampler, like the shadow map. A 1x1 white stand-in when the fade is off.
Texture2D<float4> SceneDepth        : register(t1, space2);
SamplerState      SceneDepthSampler : register(s1, space2);

cbuffer Params : register(b0, space3)
{
    // .x != 0: take the colour from the vertex alone and the texture for its alpha only.
    // That is the fixed-function stage the terrain lightmap's circle shadows set --
    // D3DTSS_COLOROP = D3DTOP_SELECTARG2 (arg2 being DIFFUSE), with the alpha op left at
    // its MODULATE default (CameraPlanarLight::drawLights). The spherical texture supplies
    // the falloff through its alpha; its rgb is not wanted.
    float4 SelectDiffuse;
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
    // FLOAT_ZBUFFER, the soft-depth fade: the group camera's projection constants
    // (matProj._33, _43, _34, _44), with which clipZOf() below turns a hardware depth value
    // back into the pre-divide clip-space z the original's float map stored. All four zero
    // -- never a valid projection -- means the fade is off for this group: SetMaterial was
    // not asked for it, or the pass has no scene-depth snapshot to sample (an offscreen
    // target such as the water reflection).
    float4 ZBufferParams;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
    float  Fog      : TEXCOORD1;
};

// Fog a premultiplied pixel. See the cbuffer note above for why there are two rules.
float4 applyFog(float4 ot, float fog)
{
    const float f = saturate(fog);
    if(FogParams.x != 0.0f)
        return ot * f;                                    // a contribution: fade it out
    ot.rgb = lerp(FogColor.rgb * ot.a, ot.rgb, f);        // an occluder: fade it to the fog
    return ot;
}

// Hardware depth -> the pre-divide clip-space z the original's float map stored. With
// (a, b, c, e) = ZBufferParams and this engine's row-vector convention, a projected vertex
// has clip.z = a*viewz + b and clip.w = c*viewz + e, so a stored depth d = clip.z/clip.w
// inverts to viewz = (e*d - b)/(a - c*d). Solving through the general four constants
// matters here: Camera::Update leaves matProj._44 at 1 even for a perspective camera, so
// the textbook two-constant inversion would be wrong for it.
float clipZOf(float d)
{
    const float viewz = (ZBufferParams.w * d - ZBufferParams.y)
                      / (ZBufferParams.x - ZBufferParams.z * d);
    return ZBufferParams.x * viewz + ZBufferParams.y;
}

float4 main(VSOutput input) : SV_Target0
{
    float4 t = Tex0.Sample(Tex0Sampler, input.UV);

    float4 ot;
    if(SelectDiffuse.x != 0.0f)
    {
        // rgb = diffuse, a = texture.a * diffuse.a -- premultiplied, as this pipeline's
        // (ONE, 1-SRC_ALPHA) blend wants. input.Color is already the vertex colour times
        // its own alpha, so multiplying it by the texture's alpha is exactly
        // raw_rgb * (t.a * diffuse.a), the premultiplied form of the pixel D3D emits.
        ot = float4(input.Color.rgb * t.a, input.Color.a * t.a);
    }
    else
        ot = t * input.Color;

    // FLOAT_ZBUFFER: fade the pixel out as it approaches the scene behind it, so smoke
    // sinks softly into terrain and objects instead of clipping against them. Both depths
    // go back through clipZOf so the original's falloff constant keeps its units:
    //
    //     float zb = tex2Dproj(ZbufferMap, v.zpos).r;
    //     ot.a *= saturate((zb - v.zpos.z) * 0.1f);
    //
    // The uv is this fragment's own pixel (the snapshot is target-sized, so the viewport
    // needs no undoing), and its own depth comes from SV_Position.z rather than a second
    // interpolant. The original scaled only the alpha over its SRC_ALPHA src factor; ours
    // is ONE over a premultiplied source, so scale the whole pixel -- the same rule the
    // FIX_FOG_ADD_BLEND note above explains.
    if(any(ZBufferParams))
    {
        uint dw, dh;
        SceneDepth.GetDimensions(dw, dh);
        const float2 uv = input.Position.xy / float2(dw, dh);
        const float zb = clipZOf(SceneDepth.Sample(SceneDepthSampler, uv).r);
        const float zq = clipZOf(input.Position.z);
        ot *= saturate((zb - zq) * 0.1f);
    }

    return applyFog(ot, input.Fog);
}
