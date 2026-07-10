// Tilemap (terrain) fragment shader for the SDL GPU backend.
//
// Ported from Render/shader/Minimal/tile_map_scene.psl. That shader composes, in
// order: a light term, the surface colour, a lightmap, an optional detail texture,
// an optional shadow, and optional fog-of-war. This is the base of that stack -- the
// #ifdef VERTEX_LIGHT path with no lightmap, detail, shadow or fog-of-war:
//
//     o.color.rgb = -dot(v.n*2-1, vLightDirection);      // vsl
//     o.color.rgb = o.color.rgb*vColor.rgb + vColor.w;   // vsl
//     float3 light = v.color;                            // psl
//     float4 ot = tex2D(ColorSampler, v.uv_color);       // psl
//     ot.rgb *= light;                                   // psl
//
// Kept faithfully: the lambert is against the *negated* light direction, because
// vLightDirection points the way the light travels; and the light term is
// `ndl * diffuse + ambient`, so the ambient is a floor added on top rather than a
// blend factor. The one addition is saturate() on the dot -- the original leans on
// the fixed-function clamp of the older pixel pipeline, and without it a surface
// facing away from the sun drives the light term below its ambient floor.
//
// The remaining layers (bump, lightmap, detail texture, shadow, fog of war, fog) come
// back as this renderer grows; the original's structure is the map for that.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-tilemap-shaders.sh.

Texture2D<float4> ColorTexture : register(t0, space2);
SamplerState      ColorSampler : register(s0, space2);

cbuffer Light : register(b0, space3)
{
    // The original's vColor (vsl c2): rgb = diffuse light colour, w = ambient. Fed from
    // the scene sun via cTileMap::GetDiffuse(); rgb may exceed 1 for a bright sun.
    float4 LightColor;
    // The original's vLightDirection (vsl c5): unit vector pointing the way the light
    // travels, so a surface facing the light has dot(N, dir) == -1. This is the scene's
    // sun_direction, read through Camera::GetLighting().
    float4 LightDirection;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    float3 N = normalize(input.Normal);
    float ndl = saturate(-dot(N, LightDirection.xyz));
    float3 light = ndl * LightColor.rgb + LightColor.a;

    float4 ot = ColorTexture.Sample(ColorSampler, input.UV);

    // Terrain is opaque base geometry: it clears and writes depth, and nothing
    // blends against it, so the alpha it carries is irrelevant. Emit 1.
    return float4(ot.rgb * light, 1.0f);
}
