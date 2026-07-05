// Water-surface fragment shader for the SDL GPU backend (P2 water slice).
//
// Faithful to the original P2 water shader (Render/shader/Water/water.vsl +
// water_linear.psl): the surface detail comes from TWO scrolling bump textures
// (waves.dds / waves1.dds), NOT procedural noise (summed sines make a regular
// dotted lattice). The two layers drift in different directions; their combined
// xy forms a surface normal that (a) ripples the water colour and (b) drives a
// tight, view-dependent specular glint -- the bright "foam" sparkle. Base colour
// and opacity come from the baked depth-opacity texture (our reflected-sky stand-
// in). Reuses mesh3d.vert (POSITION/NORMAL/UV + world-space WorldPos).
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross.

Texture2D<float4> ColorTex : register(t0, space2);   // baked depth-opacity (premult BGRA)
SamplerState      ColorSmp : register(s0, space2);
Texture2D<float4> Bump0    : register(t1, space2);   // waves.dds
SamplerState      Bump0Smp : register(s1, space2);
Texture2D<float4> Bump1    : register(t2, space2);   // waves1.dds
SamplerState      Bump1Smp : register(s2, space2);

cbuffer WaterFS : register(b0, space3)
{
    float4 LightDir;    // xyz = unit dir *toward* the light, w = specular strength
    float4 CameraPos;   // xyz = world camera position, w = time (seconds)
    float4 Params;      // x = bump uv scale, y = scroll speed, z = ripple strength, w spare
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

float4 main(VSOutput input) : SV_Target0
{
    // Base water colour + opacity from the baked depth texture (premultiplied:
    // rgb == waterColour * a, a == surface opacity). Dry/shallow areas -> a ~ 0.
    float4 base = ColorTex.Sample(ColorSmp, input.UV);
    float  a    = base.a;

    // Two scrolling bump layers (original: t0 = pos.xy, t1 = pos.yx swapped, offsets
    // scroll over time). World XY keeps the ripple scale constant regardless of mesh.
    float  t     = CameraPos.w;
    float  scale = Params.x;
    float  speed = Params.y;
    float2 uv0 = input.WorldPos.xy * scale        + float2( t * speed,        t * speed * 0.7f);
    float2 uv1 = input.WorldPos.yx * scale * 1.3f + float2(-t * speed * 0.6f, t * speed * 0.9f);
    float4 b0 = Bump0.Sample(Bump0Smp, uv0);
    float4 b1 = Bump1.Sample(Bump1Smp, uv1);

    // Surface normal from the two bumps. The V8U8 wave maps decode to [0,1] with 0.5
    // == flat, so the signed delta is (b*2-1); summed over both layers this is exactly
    // (b0.xy + b1.xy) - 1, matching the original's (t0.xy + t1.xy)*0.5 in signed space.
    float2 nxy = (b0.xy + b1.xy) - 1.0f;                 // ~[-1,1] signed wave normal xy
    float3 n = normalize(float3(nxy, 1.0f));

    // Ripple the base colour brightness from the bump detail (organic, no lattice).
    float ripple = nxy.x;
    float3 rgb = base.rgb * (1.0f + ripple * 0.20f * Params.z);

    // Tight specular glint (original: smoothstep(0.99,1,-dot(reflect(Ltravel,n),eye))).
    float3 Ltravel = -normalize(LightDir.xyz);           // light travel = -(toward light)
    float3 refl    = reflect(Ltravel, n);
    float3 eye     = normalize(input.WorldPos - CameraPos.xyz);
    // Tight threshold so only bump normals that align *exactly* sparkle -> sparse
    // glints along the glitter path instead of one broad blown-out glare.
    float  spec    = smoothstep(0.992f, 1.0f, -dot(refl, eye)) * LightDir.w;
    rgb += spec * float3(0.90f, 0.90f, 1.0f) * a;        // premultiplied glints (water only)

    return float4(rgb, a);
}
