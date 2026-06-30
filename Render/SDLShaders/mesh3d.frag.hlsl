// 3D static-mesh fragment shader for the SDL GPU backend (slice 3).
//
// The menu background is a 2D-style UI collage of glow/decal textures, so it is
// rendered UNLIT (full-bright) — applying a directional light only darkened and
// murked the cyan/teal art. The texture is premultiplied-alpha (see DDSImage),
// paired with a (ONE, ONE_MINUS_SRC_ALPHA) blend. Authored in HLSL; cross-
// compiled to SPIR-V/MSL with SDL_shadercross (see build-mesh-shaders.sh).

Texture2D<float4> Texture : register(t0, space2);
SamplerState      Sampler : register(s0, space2);

// Per-submesh material tint: rgb = diffuse color, a = opacity. The menu art is
// mostly near-white mask textures whose final color/opacity comes from the
// material diffuse Color4f (e.g. menu_button is white, tinted green @20%). Without
// this the masks render light grey. See MeshCacheGeometry::SubMesh::diffuse.
cbuffer Tint : register(b0, space3)
{
    float4 Tint;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    // Texture is premultiplied-alpha. Apply the diffuse tint (color) and opacity
    // (Tint.a) staying in premultiplied space: out.rgb = tex.rgb * tint.rgb * a,
    // out.a = tex.a * a. Pairs with (ONE, ONE_MINUS_SRC_ALPHA) / additive blend.
    float4 tex = Texture.Sample(Sampler, input.UV);
    return float4(tex.rgb * Tint.rgb * Tint.a, tex.a * Tint.a);
}
