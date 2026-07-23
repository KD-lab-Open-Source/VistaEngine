// Ice-on-water vertex shader for the SDL GPU backend.
//
// Ported from Render/shader/Water/water_ice.vsl (the USE_ALPHA path) as cTemperature::Draw
// drove ShaderSceneWaterIce over the water surface: iceShader_->beginDraw(snow, bump,
// coverage, cleft, ...) then pWater->DrawPolygons. The ice sheet is the same grid of water
// polygons drawn a second time, blended over the surface, keyed off a temperature coverage
// grid so ice appears only where the water froze.
//
// The vertex is cWater's (position + a per-node diffuse whose red the fragment uses as the
// coverage weight aa). From the world position the VS derives: the coverage-grid UV
// (pos.xy * AlphaScale, AlphaScale = the map's inverse size, so the grid spans the map); the
// snow, bump and cleft tiling UVs; the fog-of-war lightmap UV; and the reflection coordinate
// (mul(pos, MirrorVP)) into the reflection camera's render target. Unlike the opaque terrain
// ice (tilemap_ice.vert.hlsl), this path never touched CONVERT_Z -- the water polygons hold
// world Z already, as they do for the water shader.
//
// Distance fog rides along as a plane over the world position; the fragment shader applies it.

cbuffer Constants : register(b0, space1)
{
    row_major float4x4 MVP;        // clip = mul(float4(pos,1), MVP)
    // The reflection camera's view-projection folded with the clip->texture map
    // (Water.cpp::fillMirrorMatrix), row-vector convention.
    row_major float4x4 MirrorVP;
    // fPlanarNode (c95): xy the fog-of-war lightmap box origin, zw its inverse extent.
    float4 PlanarNode;
    // Distance fog, from cSDLRenderDevice::fogPlane(camera). (0,0,0,1) -> factor 1 (off).
    float4 FogPlane;
    // x = bump UV scale, y = snow UV scale, z = cleft UV scale (the original's
    // fScaleBumpSnow = 0.01, 0.003, 0.002).
    float4 ScaleBumpSnow;
    // xy = the coverage-grid UV scale (the original's uvScaleOffset = tilemap_inv_size).
    float4 AlphaScale;
};

struct VSInput
{
    float3 Position : POSITION;   // world space, offset 0
    float4 Diffuse  : COLOR0;     // UBYTE4_NORM @12; .r is the coverage weight aa
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float4 Diffuse    : COLOR0;
    float2 AlphaUV    : TEXCOORD0;
    float2 SnowUV     : TEXCOORD1;
    float2 BumpUV     : TEXCOORD2;
    float2 CleftUV    : TEXCOORD3;
    float2 LightmapUV : TEXCOORD4;
    float4 Mirror     : TEXCOORD5;   // projective reflection coordinate (xy/w in the FS)
    float  Fog        : TEXCOORD6;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 pos = float4(input.Position, 1.0f);
    output.Position   = mul(pos, MVP);
    output.Diffuse    = input.Diffuse;
    output.AlphaUV    = input.Position.xy * AlphaScale.xy;
    output.SnowUV     = input.Position.xy * ScaleBumpSnow.y;
    output.BumpUV     = input.Position.xy * ScaleBumpSnow.x;
    output.CleftUV    = input.Position.xy * ScaleBumpSnow.z;
    output.LightmapUV = (input.Position.xy - PlanarNode.xy) * PlanarNode.zw;
    output.Mirror     = mul(pos, MirrorVP);
    output.Fog        = dot(pos, FogPlane);
    return output;
}
