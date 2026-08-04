// Skinned-object (3dx) vertex shader for the SDL GPU backend.
//
// Ported from two original P2 shaders, selected by cObject3dx::Draw per material:
//
//   BUMP=0  Render/shader/Skin/object_scene_light.vsl (vsSkin) -- the plain lit path,
//           taken when the material has no bump, no reflection and no second opacity
//           map. Per-vertex lit(N.L, N.H, power) against the *negated* vLightDirection
//           (which points the way the light travels): diffuse to COLOR0, specular to
//           COLOR1, NOLIGHT collapsing diffuse to the ambient colour. The lit()
//           intrinsic is spelled out by hand -- see the note at the lighting itself.
//
//   BUMP=1  Render/shader/Skin/object_scene_bump.vsl (vsSkinBump). No per-vertex
//           diffuse: instead the light and half vectors are pulled into the vertex's
//           tangent space (T, S, N) and interpolated, and the fragment shader does the
//           lambert against the bump map. Note the original's mul_trans(v, mWorld) is
//           R^T*v -- a world vector taken *into* bone space -- which is v.x*r0 +
//           v.y*r1 + v.z*r2 in our row-per-register layout. Only the specular COLOR1
//           output (zero here, before point lights) survives.
//
// REFLECTION=1 (with BUMP=0) is the original's vsSkinReflection, taken for a material
// with a 2D environment ("matcap") map -- the shiny-metal units. It adds one output over
// the plain lit path: a sphere-map UV built from the view-space normal, exactly the
// original's `mul(world_n, (float3x2)mView)*0.5 + 0.5` (the view rotation's first two
// columns are the camera right/up, so this is the normal's x,y in camera space, mapped to
// [0,1]). REFLECTION never combines with BUMP: cObject3dx::Draw picks the reflection
// technique before the bump one, and the two are mutually exclusive there.
//
// Both keep the same skinning (an index into mWorldM[] per vertex; for WEIGHT>1 a
// weighted sum of the indexed 4x3 world matrices) and the same affine UV transform
// (uvtrans.inl).
//
// Dropped for now, and each is a `#ifdef` in the original worth returning to: fog,
// shadow projection, cube/planar reflection, the lightmap/fog-of-war planar UV, the
// two dynamic point lights (pointcolor.inl), fur displacement and ZBUFFER output.
//
// Compiled once per (SKINNED, BUMP) pair. SKINNED mirrors the original's `#if(WEIGHT>1)`:
// cStatic3dx builds a vertex *without* the weight bytes when a lod binds one bone per
// vertex. BUMP mirrors the two source files -- and the vertex only carries its tangent
// frame when cStatic3dx::bump is set.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-object3dx-shaders.sh.

#ifndef SKINNED
#define SKINNED 0
#endif
#ifndef BUMP
#define BUMP 0
#endif
#ifndef REFLECTION
#define REFLECTION 0
#endif
// The original's `#if(REFLECTION==1)` / `#else` inside object_scene_light.vsl: the same
// shader served a 2D matcap and a cube, chosen at draw time by whether the bound texture
// was a cubemap (cObject3dx::Draw reads TEXTURE_CUBEMAP and calls VSSkin::SetReflection).
// Here that second case is its own permutation instead of a runtime branch.
#ifndef REFLECT_CUBE
#define REFLECT_CUBE 0
#endif
#ifndef SECOND_OPACITY
#define SECOND_OPACITY 0
#endif

// Matches StaticBunch::max_index -- the most bones one material group can reference.
#define MAX_BONES 20

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;      // engine row-major; clip = mul(float4(pos,1), MVP)

    float4 Ambient;              // vAmbient: NOLIGHT's flat colour (rgb) and alpha
    float4 Diffuse;              // vDiffuse: lit colour (rgb), object alpha (a)
    float4 Specular;             // vSpecular: colour (rgb), specular power (w)
    float4 CameraPos;            // vCameraPos (xyz)
    float4 LightDirection;       // vLightDirection (xyz): the way the light travels

    float4 UTrans;               // vUtrans (xyz); w != 0 applies the UV transform
    float4 VTrans;               // vVtrans (xyz)

    float4 Params;               // x = bone count per vertex (1..4), y != 0 = NOLIGHT

    // The original's mShadow (object_scene_light.vsl c70, under #ifdef SHADOW):
    // shadowMatViewProj() * shadowMatBias(), i.e. world space -> the light's clip space
    // -> shadow map texture coords. Zero when nothing casts; the fragment shader gates on
    // ShadowParams.x, not on this.
    row_major float4x4 Shadow;

    // Distance fog, from cSDLRenderDevice::fogPlane(camera): fog = dot(float4(world,1), it).
    // The original object shaders have no fog term to port -- D3D fogged them in fixed
    // function, per pixel (D3DFOG_LINEAR table fog), after the pixel shader ran. (0,0,0,1)
    // means fog is off, making the fragment shader's lerp the identity. See SDLRenderDevice.h.
    // It must stay ahead of World[], which is the tail the bone matrices are pushed into.
    float4 FogPlane;

    // The camera's view matrix (world -> camera space), the original's mView (c90). Read
    // only by the REFLECTION variant, for the sphere-map UV; pushed for every variant so
    // the one uniform block stays in step (object3dx_shadow.vert.hlsl declares it too).
    row_major float4x4 View;

    // The second-opacity map's affine UV transform (the original's vSecondUtrans/vSecondVtrans,
    // set from mat_chain.uv_displacement -- the moving mask that traces the menu shapes). Read
    // only by the SECOND_OPACITY variant; w != 0 applies it, otherwise the map reuses UV set 0
    // untransformed. Pushed for every variant so the one block stays in step.
    float4 SecondUTrans;
    float4 SecondVTrans;

    // mWorldM[20] as 20 x 3 rows of (R | T): world.k = dot(float4(pos,1), World[3i+k]).
    // The original ships the same 3 registers per bone (setMatrix4x3VS).
    float4 World[MAX_BONES * 3];
};

struct VSInput
{
    float3 Position     : POSITION;       // offset 0
    uint4  BlendIndices : BLENDINDICES;   // offset 12, D3DCOLOR -> UBYTE4 (memory order)
    float3 Normal       : NORMAL;         // offset 16
#if SKINNED
    float4 BlendWeight  : COLOR0;         // offset 28, D3DCOLOR -> UBYTE4_NORM (b,g,r,a)
#endif
    float2 UV           : TEXCOORD0;      // offset 28 (rigid) / 32 (skinned)
#if BUMP
    // cSkinVertex's tangent frame, right after the uv: BINORMAL is GetBumpS, TANGENT is
    // GetBumpT, and cStatic3dx::CalcBumpSTNorm makes the normal their cross product.
    float3 Binormal     : BINORMAL;       // uv + 8
    float3 Tangent      : TANGENT;        // uv + 20
#endif
};

struct VSOutput
{
    float4 Position  : SV_Position;
#if BUMP
    float3 LightObj  : TEXCOORD1;   // light vector in tangent space
    float3 HalfObj   : TEXCOORD2;   // half vector in tangent space
#else
    float4 Diffuse   : COLOR0;
#endif
    float3 Specular  : COLOR1;
    float2 UV        : TEXCOORD0;
    float4 ShadowPos : TEXCOORD3;   // the original's o.tshadow
    float  Fog       : TEXCOORD4;
#if REFLECTION
#if REFLECT_CUBE
    float3 Reflect   : TEXCOORD5;   // world-space reflection vector, for the sky cubemap
#else
    float2 Reflect   : TEXCOORD5;   // sphere-map UV: view-space normal.xy mapped to [0,1]
#endif
#endif
#if SECOND_OPACITY
    float2 UV1       : TEXCOORD6;   // the original's o.t1: the second-opacity map's UV
#endif
};

VSOutput main(VSInput input)
{
    // --- skin ---------------------------------------------------------------
    // The original does `int4 blend = D3DCOLORtoUBYTE4(v.blend)`, whose .zyxw swizzle
    // undoes D3D's D3DCOLOR->float4 expansion and hands back the four index bytes in
    // memory order. UBYTE4 gives us those bytes directly, so no swizzle here.
    float4 r0, r1, r2;
#if SKINNED
    // cSkinVertex::GetWeight(i) writes weight i to byte {2,1,0,3} -- i.e. the D3DCOLOR
    // R,G,B,A channels, which is what COLOR0 delivered as .xyzw in D3D. UBYTE4_NORM
    // gives memory order (b,g,r,a) instead, so swizzle back to (r,g,b,a).
    float4 bw = input.BlendWeight.zyxw;
    uint b = input.BlendIndices.x * 3;
    r0 = World[b + 0] * bw.x;
    r1 = World[b + 1] * bw.x;
    r2 = World[b + 2] * bw.x;
    int count = (int)Params.x;
    [loop] for(int i = 1; i < count; i++){
        uint bi = input.BlendIndices[i] * 3;
        r0 += World[bi + 0] * bw[i];
        r1 += World[bi + 1] * bw[i];
        r2 += World[bi + 2] * bw[i];
    }
#else
    uint b = input.BlendIndices.x * 3;
    r0 = World[b + 0];
    r1 = World[b + 1];
    r2 = World[b + 2];
#endif

    float4 pos = float4(input.Position, 1.0f);
    float3 worldPos = float3(dot(pos, r0), dot(pos, r1), dot(pos, r2));

    VSOutput output;
    output.Position = mul(float4(worldPos, 1.0f), MVP);

    // The original's `o.tshadow = mul(world_pos4, mShadow)`. Left unprojected: the
    // fragment shader divides, so the TSM warp the light matrix may carry survives.
    output.ShadowPos = mul(float4(worldPos, 1.0f), Shadow);
    output.Fog = dot(float4(worldPos, 1.0f), FogPlane);

    // --- uv (uvtrans.inl) ---------------------------------------------------
    if(UTrans.w != 0.0f){
        float3 uv1 = float3(input.UV, 1.0f);
        output.UV = float2(dot(uv1, UTrans.xyz), dot(uv1, VTrans.xyz));
    }
    else
        output.UV = input.UV;

#if SECOND_OPACITY
    // uvtrans.inl's SECOND_OPACITY_TEXTURE (!= 2) branch: the second map reads UV set 0,
    // transformed by its own matrix when SECOND_UVTRANS is on (SecondUTrans.w != 0). The
    // menu model is isUV2 == false, so this is the only mode that ships.
    if(SecondUTrans.w != 0.0f){
        float3 uvs = float3(input.UV, 1.0f);
        output.UV1 = float2(dot(uvs, SecondUTrans.xyz), dot(uvs, SecondVTrans.xyz));
    }
    else
        output.UV1 = input.UV;
#endif

    // --- light --------------------------------------------------------------
#if BUMP
    // mul_trans(v, mWorld) == R^T*v: a world-space vector expressed in bone space,
    // where the vertex's T/S/N frame lives. Then project onto that frame, in the
    // original's axis order (x=T, y=S, z=N).
    float3 light = -(LightDirection.x * r0.xyz + LightDirection.y * r1.xyz + LightDirection.z * r2.xyz);
    output.LightObj = normalize(float3(dot(light, input.Tangent),
                                       dot(light, input.Binormal),
                                       dot(light, input.Normal)));

    // The original leaves half_v unnormalized before the frame change, and normalizes
    // only the tangent-space result.
    float3 dir = normalize(CameraPos.xyz - worldPos);
    float3 halfV = dir - LightDirection.xyz;
    float3 halfObj = halfV.x * r0.xyz + halfV.y * r1.xyz + halfV.z * r2.xyz;
    output.HalfObj = normalize(float3(dot(halfObj, input.Tangent),
                                      dot(halfObj, input.Binormal),
                                      dot(halfObj, input.Normal)));

    output.Specular = 0.0f;   // the bump fragment shader computes its own specular
#else
    if(Params.y != 0.0f){        // NOLIGHT
        output.Diffuse = Ambient;
        output.Specular = 0.0f;
#if REFLECTION
        // A reflection material is always lit (cObject3dx::Draw picks it after the NOLIGHT
        // branch), so this path is never taken for one at runtime -- but every output must
        // still be written for the shader to compile.
#if REFLECT_CUBE
        output.Reflect = float3(0.0f, 0.0f, 1.0f);
#else
        output.Reflect = float2(0.5f, 0.5f);
#endif
#endif
    }
    else{
        float3 n = float3(dot(input.Normal, r0.xyz), dot(input.Normal, r1.xyz), dot(input.Normal, r2.xyz));
        n = normalize(n);

        float3 ldir = -LightDirection.xyz;
        float3 dir = normalize(CameraPos.xyz - worldPos);
        float3 halfV = normalize(dir + ldir);

        // The original writes lit(N.L, N.H, power) and reads .y and .z. Do NOT use the
        // lit() intrinsic here: shadercross lowers its specular term to `N.H * power`
        // instead of `pow(N.H, power)`, which with power=10 blows the highlight out to
        // white on every surface facing the eye. Spelled out, with lit()'s own semantics:
        // the specular is zero unless both dot products are positive.
        float ndl = dot(n, ldir);
        float ndh = dot(n, halfV);

        output.Diffuse.rgb = max(ndl, 0.0f) * Diffuse.rgb;
        output.Diffuse.a = Diffuse.a;
        output.Specular = (ndl > 0.0f && ndh > 0.0f) ? pow(ndh, Specular.w) * Specular.rgb
                                                     : float3(0.0f, 0.0f, 0.0f);

#if REFLECTION
#if REFLECT_CUBE
        // The original's cube branch, verbatim: `dir + (2*dot(world_n,dir))*world_n`, with
        // dir the normalized vertex->camera vector computed above. That is the view ray
        // mirrored about the normal, in world space, which is how the cube is addressed --
        // the sky cubemap's faces were rendered with world-oriented cameras.
        output.Reflect = dir + (2.0f * dot(n, dir)) * n;
#else
        // The original's `mul(world_n, (float3x2)mView)*0.5 + 0.5`: the world normal taken
        // into camera space (w = 0 drops the translation), its x,y are the projections onto
        // the camera right/up axes -- a view-space sphere map.
        float3 nView = mul(float4(n, 0.0f), View).xyz;
        output.Reflect = nView.xy * 0.5f + 0.5f;
#endif
#endif
    }
#endif
    return output;
}
