// Terrain-lava vertex shader for the SDL GPU backend.
//
// Ported from Render/shader/Water/water_lava.vsl, the vertex half of
// ShaderSceneWaterLava. On D3D9 that shader drew the tile-map polygons a placement-zone
// LAVA material painted (cTileMap::setMaterial binds it, then draws the material's index
// run); here the same runs of SDLTileMapRenderer's terrain mesh are drawn with it instead
// of the plain terrain pipeline.
//
// The original derives two texture coordinates from the world position: a 3D volume
// coordinate (pos.xyz * volumeScale) scrolled in Z by time to animate the lava, and a 2D
// ground coordinate (pos.xy * groundScale). It computes them in the VS; we pass the world
// position through and scale in the fragment, which is the same linear result and keeps the
// scales in one cbuffer. The original's CONVERT_Z (pos.z *= 1/64) decoded the D3D tile-map's
// fixed-point Z back to world units before both the transform and the volume lookup;
// SDLTileMapRenderer's mesh already holds world Z (vMap.getZf), so there is nothing to undo
// -- exactly as tilemap.vert.hlsl notes for the plain terrain path.
//
// Fog and fog-of-war ride along as in tilemap.vert.hlsl: the distance-fog factor is a plane
// over the world position, and the fog-of-war lightmap UV is (pos.xy - PlanarNode.xy) *
// PlanarNode.zw. The lava PS reads neither the sun nor the shadow map (lava is emissive), so
// the normal and the shadow transform the terrain shader carries are not passed.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;   // engine row-major; clip = mul(float4(pos,1), MVP)
    // The original's fPlanarNode (c95): xy is the lightmap box's world origin, zw its inverse
    // extent, so this maps world xy onto the fog-of-war lightmap's texels.
    float4 PlanarNode;
    // Distance fog, from cSDLRenderDevice::fogPlane(camera). (0,0,0,1) means fog is off:
    // the factor is then 1 and the fragment shader's lerp is the identity.
    float4 FogPlane;
};

struct VSInput
{
    float3 Position : POSITION;   // world space, offset 0
    float3 Normal   : NORMAL;     // world space, offset 12 (unused here; kept for the shared layout)
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float3 WorldPos   : TEXCOORD0;   // world xyz -> volume + ground UVs, scaled in the FS
    float2 LightmapUV : TEXCOORD1;
    float  Fog        : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position   = mul(float4(input.Position, 1.0f), MVP);
    output.WorldPos    = input.Position;
    output.LightmapUV = (input.Position.xy - PlanarNode.xy) * PlanarNode.zw;
    // Unsaturated: linear in view depth, clamped per pixel in the FS -- the per-pixel fog
    // D3D's rasterizer computed. See tilemap.vert.hlsl.
    output.Fog = dot(float4(input.Position, 1.0f), FogPlane);
    return output;
}
