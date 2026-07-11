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
    float4 Position : SV_Position;
#if BUMP
    float3 LightObj : TEXCOORD1;   // light vector in tangent space
    float3 HalfObj  : TEXCOORD2;   // half vector in tangent space
#else
    float4 Diffuse  : COLOR0;
#endif
    float3 Specular : COLOR1;
    float2 UV       : TEXCOORD0;
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

    // --- uv (uvtrans.inl) ---------------------------------------------------
    if(UTrans.w != 0.0f){
        float3 uv1 = float3(input.UV, 1.0f);
        output.UV = float2(dot(uv1, UTrans.xyz), dot(uv1, VTrans.xyz));
    }
    else
        output.UV = input.UV;

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
    }
#endif
    return output;
}
