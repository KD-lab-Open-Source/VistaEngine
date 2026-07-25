// Terrain-ice vertex shader for the SDL GPU backend.
//
// Ported from Render/shader/Water/water_ice.vsl, the vertex half of ShaderSceneWaterIce as
// cTileMap::setMaterial drove it for a placement-zone ICE material (beginDraw with a null
// alpha texture -- opaque terrain ice, USE_ALPHA off). Here the same runs of
// SDLTileMapRenderer's terrain mesh are drawn with it instead of the plain terrain pipeline.
//
// The original derives three tiling UVs from the world position -- a snow colour coordinate
// (pos.xy * snowScale), a bump coordinate (pos.xy * bumpScale) and, for the alpha path, a
// cleft coordinate -- plus the fog-of-war lightmap UV and a reflection coordinate. The ice is
// mostly the snow texture with a planar reflection showing through where the snow is not
// opaque, so the reflection is the star: MIRROR_LINEAR projects the world position into the
// reflection camera's render target (mul(pos, vMirrorVP)), exactly as water.vert.hlsl does.
// When there is no reflection camera the original falls back to the sky cubemap, which has no
// SDL consumer yet (see Documents/Render-PORTING.md); the fragment shader then shows plain snow.
//
// The original's CONVERT_Z (pos.z *= 1/64, the USE_ALPHA-off branch) decoded the D3D
// tile-map's fixed-point Z; SDLTileMapRenderer's mesh already holds world Z, so nothing is
// undone -- as tilemap.vert.hlsl notes. Distance fog rides along as a plane over the world
// position (D3D9 applied it in fixed function); the fragment shader clamps and applies it.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;        // clip = mul(float4(pos,1), MVP)
    // The reflection camera's view-projection folded with the clip->texture map, as
    // Water.cpp::fillMirrorMatrix builds it; the reflection UV is mul(float4(pos,1), MirrorVP).
    row_major float4x4 MirrorVP;
    // fPlanarNode (c95): xy the fog-of-war lightmap box origin, zw its inverse extent.
    float4 PlanarNode;
    // Distance fog, from cSDLRenderDevice::fogPlane(camera). (0,0,0,1) -> factor 1 (off).
    float4 FogPlane;
    // x = bump UV scale, y = snow UV scale (the original's fScaleBumpSnow.xy = 0.01, 0.003).
    float4 ScaleBumpSnow;
};

struct VSInput
{
    float3 Position : POSITION;   // world space, offset 0
    float3 Normal   : NORMAL;     // offset 12 (unused here; kept for the shared mesh layout)
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float2 SnowUV     : TEXCOORD0;
    float2 BumpUV     : TEXCOORD1;
    float2 LightmapUV : TEXCOORD2;
    float4 Mirror     : TEXCOORD3;   // projective reflection coordinate (xy/w in the FS)
    float  Fog        : TEXCOORD4;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 pos = float4(input.Position, 1.0f);
    output.Position   = mul(pos, MVP);
    output.SnowUV     = input.Position.xy * ScaleBumpSnow.y;
    output.BumpUV     = input.Position.xy * ScaleBumpSnow.x;
    output.LightmapUV = (input.Position.xy - PlanarNode.xy) * PlanarNode.zw;
    output.Mirror     = mul(pos, MirrorVP);
    // Unsaturated: linear in view depth, clamped per pixel in the FS.
    output.Fog        = dot(pos, FogPlane);
    return output;
}
