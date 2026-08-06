// Shadow-caster vertex shader for the SDL GPU backend.
//
// Ported from Render/shader/Skin/object_shadow.vsl: the same skinning as the lit path,
// transformed by the *light* camera's view-projection, plus the uv the fragment shader
// needs for the alpha-cutout clip. The original also writes o.pos.z to TEXCOORD2 so its
// pixel shader can store depth in a float colour target; we render depth directly, so
// there is nothing to carry.
//
// The constant buffer is declared identically to object3dx.vert.hlsl on purpose. The
// caster and the lit path both come from SDLObject3dxRenderer::SetState, which pushes
// one block; only MVP (here the light's), UTrans/VTrans, Params.x and World[] are read.
//
// Compiled twice, -DSKINNED=0/1, for the same reason as object3dx.vert.hlsl: a lod that
// binds one bone per vertex carries no weight bytes.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-object3dx-shaders.sh.

#ifndef SKINNED
#define SKINNED 0
#endif

#define MAX_BONES 20

// The SAME block the lit path pushes (SDLObject3dxRenderer::VSHead, then the bone rows) --
// the caster pass reuses it whole rather than building a second one, and only reads MVP,
// Params and World. So every field must be declared here, in order, even the ones this
// shader never touches: drop one and World[] slides forward, the bones are read from the
// wrong offsets, and the casters come out as garbage. Keep it in step with
// object3dx.vert.hlsl.
cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;      // the light camera's matViewProj
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 CameraPos;
    float4 LightDirection;
    float4 UTrans;               // xyz = u row; w != 0 applies the UV transform
    float4 VTrans;
    float4 Params;               // x = bone count per vertex (1..4)
    row_major float4x4 Shadow;   // the lit path's receiver matrix; unused here
    float4 FogPlane;             // the lit path's fog plane; unused here
    row_major float4x4 View;     // the lit path's REFLECTION view matrix; unused here
    float4 SecondUTrans;         // the lit path's SECOND_OPACITY UV transform; unused here
    float4 SecondVTrans;
    float4 World[MAX_BONES * 3];
};

// SDL_GPU's D3D12 backend names every vertex input element TEXCOORD<location>, whatever
// the data means, so the semantics below spell out the attribute location rather than the
// D3D9 usage (see SDLShaders/ShaderBlob.h). SKINNED drops one attribute in the middle, so
// the uv that follows it shifts down a location -- exactly as SDLObject3dxRenderer's
// pipelineFor() numbers them, which skips the weight slot for a rigid lod.
#if SKINNED
#define SEM_UV TEXCOORD4
#else
#define SEM_UV TEXCOORD3
#endif

struct VSInput
{
    float3 Position     : TEXCOORD0;
    uint4  BlendIndices : TEXCOORD1;
    float3 Normal       : TEXCOORD2;
#if SKINNED
    float4 BlendWeight  : TEXCOORD3;
#endif
    float2 UV           : SEM_UV;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    float4 r0, r1, r2;
#if SKINNED
    float4 bw = input.BlendWeight.zyxw;   // see object3dx.vert.hlsl on the swizzle
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

    if(UTrans.w != 0.0f){
        float3 uv1 = float3(input.UV, 1.0f);
        output.UV = float2(dot(uv1, UTrans.xyz), dot(uv1, VTrans.xyz));
    }
    else
        output.UV = input.UV;

    return output;
}
