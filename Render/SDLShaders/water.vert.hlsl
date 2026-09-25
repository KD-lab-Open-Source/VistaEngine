// Water-surface vertex shader for the SDL GPU backend.
//
// Three techniques, selected by -DREFLECTION / -DCUBE:
//
//   REFLECTION=0 -- the original's water_easy.vsl (WATER_EMPTY), which cWater::Draw
//                   selects on hardware without PS2.0. Now only a fallback: it is what
//                   we draw when the technique below has no cubemap to sample.
//   REFLECTION=1 -- the original's water_linear.vsl (WATER_LINEAR_REFLECTION): the same
//                   thing plus the projective lookup into the reflection render target
//                   and the world position the fragment shader needs for the sun glint.
//   CUBE=1       -- the original's water_cube.vsl (WATER_REFLECTION), which the original
//                   picks when the reflection option is OFF -- not water_easy, which is
//                   the no-PS2.0 path. It reflects the sky cubemap instead of a planar
//                   target. Note there is no glint term in the fragment shader: the sun
//                   you see sliding over the crests is the sun *in the cubemap*, moved by
//                   the wave slopes that perturb the lookup.
//
// Both compute, from water_easy.vsl:
//
//     o.pos      = mul(v.pos, mVP);
//     o.diffuse  = v.diffuse;
//     o.uv_tex0  = uvScaleOffset.zw  + v.pos.xy*uvScaleOffset.xy;
//     o.uv_tex1  = uvScaleOffset1.zw + v.pos.yx*uvScaleOffset1.xy;
//
// The two wave maps scroll over the surface at different speeds and, note, sample the
// position under opposite swizzles (xy vs yx), which is what stops them beating against
// each other into a visible grid.
//
// water_linear.vsl adds:
//
//     o.uv_sky    = mul(v.pos, vMirrorVP);   // projective; divided by w in the fragment
//     o.point_pos = v.pos;
//
// water_cube.vsl adds the cubemap direction, verbatim including its oddity:
//
//     float3 dir = vCameraPos - v.pos;
//     dir.z -= v.pos.z;                      // so dir.z = camera.z - 2*pos.z
//     o.uv_mirror = normalize(dir);
//
// -- the surface-to-eye vector with z mirrored about the plane z=0, which is the cheap
// reflection this technique settles for (the water is a heightfield near z=0, so a true
// per-vertex reflection about the local surface would cost a normal the vertex does not
// carry). Its uv_sky output is dropped: water_cube.psl declares it and never reads it, so
// uvScaleOffsetSky / VSWater::SetSpeedSky feed nothing here.
//
// water_lava.vsl is its own shader pair, still unported.
//
// Dropped, each for want of the input rather than by choice: o.uv_lightmap (the
// fog-of-war lightmap, behind #ifdef FOG_OF_WAR in the fragment shader) and o.fog (the
// fixed-function fog stage, which has no SDL GPU equivalent).
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-water-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // engine row-major; clip = mul(float4(pos,1), MVP)
    // The original's uvScaleOffset / uvScaleOffset1 (VSWater::SetSpeed/SetSpeed1):
    // xy is the world->uv scale, zw the scroll offset cWater::Draw advances with
    // animate_time. Both are affine in world XY -- the surface is a heightfield, so
    // there is no uv in the vertex at all.
    float4 UVScaleOffset;
    float4 UVScaleOffset1;
    // The original's vMirrorVP (VSWater::SetMirrorMatrix): the reflection camera's
    // view-projection, post-multiplied by the clip->uv adjust. Only read when
    // REFLECTION, but declared either way so one C++ uniform struct serves both.
    row_major float4x4 MirrorVP;
    // Distance fog, from cSDLRenderDevice::fogPlane(camera): fog = dot(float4(world,1), it).
    // water_easy.vsl wrote the same factor by hand (`o.fog = mul(pos,mView).z*vFog.z+vFog.y`)
    // for the cards with no table fog; the plane form folds the view matrix in. (0,0,0,1)
    // means fog is off. See SDLRenderDevice.h.
    float4 FogPlane;
    // The original's vCameraPos, the main camera's world position. water_cube.vsl builds
    // its cubemap direction from it; declared either way so one C++ uniform struct serves
    // all three variants. CUBE only.
    float4 CameraPos;
};

struct VSInput
{
    // The semantics are TEXCOORD<location>, not what the data means -- SDL_GPU's D3D12
    // backend names every input element TEXCOORD; see SDLShaders/ShaderBlob.h.
    float3 Position : TEXCOORD0;  // world space, offset 0
    // sVertexXYZD's D3DCOLOR diffuse, offset 12. Only the alpha is read (the fragment
    // shader's per-vertex water opacity, baked by cWater::CalcColor from the depth
    // gradient); .w is the alpha byte whichever way the other three are ordered.
    float4 Diffuse  : TEXCOORD1;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Diffuse  : COLOR0;
    float2 UV0      : TEXCOORD0;
    float2 UV1      : TEXCOORD1;
#if REFLECTION
    float4 UVSky    : TEXCOORD2;   // projective: sample as UVSky.xy/UVSky.w
    float3 PointPos : TEXCOORD3;   // world position, for the eye vector
#endif
    float  Fog      : TEXCOORD4;
#if CUBE
    float3 Mirror   : TEXCOORD5;   // water_cube.vsl's uv_mirror, the cubemap direction
#endif
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.Diffuse  = input.Diffuse;
    output.UV0 = UVScaleOffset.zw  + input.Position.xy * UVScaleOffset.xy;
    output.UV1 = UVScaleOffset1.zw + input.Position.yx * UVScaleOffset1.xy;
#if REFLECTION
    output.UVSky    = mul(float4(input.Position, 1.0f), MirrorVP);
    output.PointPos = input.Position;
#endif
#if CUBE
    float3 dir = CameraPos.xyz - input.Position;
    dir.z -= input.Position.z;
    output.Mirror = normalize(dir);
#endif
    output.Fog = dot(float4(input.Position, 1.0f), FogPlane);
    return output;
}
