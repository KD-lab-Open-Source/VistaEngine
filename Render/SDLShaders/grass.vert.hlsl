// Grass vertex shader for the SDL GPU backend.
//
// Ported from the original P2 shader Render/shader/Grass/Grass.vsl (the VSGrass class),
// in its SHADOW configuration -- which is the only one VSGrass::RestoreShader ever built:
//
//     shaderVS_->StaticSelect("SHADOW", 1);
//
// The ZBUFFER variant fed the float z-buffer camera, which the D3D9 retirement removed
// (Render/PORTING.md #12), so it is not ported. LIGHTMAP is a uniform here rather than a
// static define, exactly as in tilemap.vert.hlsl, because our lightmap may not exist yet.
//
// Each blade is a quad of four vertices that all share ONE world position -- the bush's
// root, in v.pos -- and are pushed apart entirely here, in the shader. What separates
// them is v.t0.zw (TEXCOORD0.zw): ±half-width in z, and height in w. That is why the
// vertex is so small: the geometry is a point cloud, and the blade is grown around it.
//
// Three things happen to that offset, in order:
//
//  * GROWTH. `sc = saturate(time - v.t1)` ramps 0->1 as the blade's own plant time
//    (v.t1, Bush::windPower, set when the grass was painted) passes. A new blade rises
//    out of the ground rather than popping in. Note z = y*(1-sc) lays the blade FLAT
//    while it grows: it unfolds from the ground up.
//  * WIND. A cubic ripple, `f = t^3 - t` over `frac(pos.xz + shift*time)`, added to the
//    blade's local x. `shift` comes from the vertex colour's alpha (Bush::shift), so each
//    bush has its own phase and the field ripples rather than swaying as one.
//  * BILLBOARD. The offset is finally rotated into world space by Billboard, so the quad
//    always faces the camera. See the note on that matrix below.
//
// Authored in HLSL; cross-compiled to SPIR-V/MSL with SDL_shadercross. See
// build-grass-shaders.sh.

cbuffer Constants : register(b0, space1)
{
    // The original's mVP (c0): camera->matViewProj. Engine row-major, so clip = mul(pos, MVP).
    row_major float4x4 MVP;

    // The original's mWorld (c9), and the reason the blades face you.
    //
    // VSGrass::Select sets it to transpose(Mat4f(camera->GetMatrix())). GetMatrix() returns
    // the camera's GlobalMatrix, which is the VIEW matrix -- world -> camera (see the comment
    // on Camera::SetPosition: "принимает матрицу в координатах камеры"). Its transpose is
    // therefore camera -> world, so multiplying the blade's local offset by it takes
    // (right, up, forward) in camera space to world space: a camera-facing billboard.
    //
    // The original declares this float4x3 and multiplies a float3 by it. We keep a float4x4
    // and multiply a float4 with w = 0, which drops the translation column and leaves exactly
    // the rotation -- the same result, without relying on how fxc padded the odd shape.
    row_major float4x4 Billboard;

    // The original's mShadow (c70): shadowMatViewProj() * shadowMatBias(), i.e. world space
    // -> the light's clip space -> shadow-map texture coords. Zero when there is no shadow
    // map; the fragment shader gates on ShadowParams.x, not on this.
    row_major float4x4 Shadow;

    // The original's fPlanarNode (c95), from cSDLRenderDevice::setPlanarTransform: xy is the
    // lightmap box's world origin, zw its inverse extent.
    float4 PlanarNode;

    // The original's vCameraPos (c8): Camera::GetPos(), the camera in world space. The blade
    // fades and stretches with distance from it.
    float4 CameraPos;

    // The original's vLightDirection (c4): Camera::GetLighting(), the direction the sunlight
    // TRAVELS, so a surface facing the sun has dot(N, dir) == -1.
    float4 LightDirection;

    // The original's SunDiffuse (c7): cTileMap::GetDiffuse() -- rgb is the sun's diffuse
    // colour, w its ambient. Same source as the terrain's, so grass and ground agree.
    float4 SunDiffuse;

    // x: the original's `time` (c5), GrassMap::time_ -- a seconds counter that drives both
    //    the wind ripple and each blade's growth ramp.
    // y: the original's `hideDistance` (c6), GrassMap::invHideDistance2_ = 1/hideDistance^2.
    //    Note it multiplies a SQUARED distance, so alpha falls off with distance^2.
    // z: OLD_LIGHTING. A static define in the original, chosen by VSGrass::SetOldLighting
    //    from GrassMap::oldLighting (which is serialized per world), so it must be a uniform.
    float4 Params;

    // Distance fog, from cSDLRenderDevice::fogPlane(camera): fog = dot(float4(world,1), it).
    // The original computed this here too (`o.fog = mul(pos,mView).z*vFog.z + vFog.y`), for
    // the cards with no table fog; the plane form is the same factor with the view matrix
    // folded in. (0,0,0,1) means fog is off. See SDLRenderDevice.h.
    float4 FogPlane;
};

struct VSInput
{
    // shortVertexGrass, stride 28. Attribute locations follow this declaration order.
    //
    // SHORT4, NOT normalized: these are world coordinates in whole units (Bush::pos), which
    // is why the position is only 8 bytes. D3D9's D3DDECLTYPE_SHORT4 hands them to the shader
    // as floats; SPIR-V hands them over as sint, so we take int4 and convert.
    int4   Position : POSITION;    // offset 0
    // Color4c is stored b,g,r,a in memory, so UBYTE4_NORM gives (b,g,r,a): swizzle to read it.
    // rgb is the terrain colour under the bush (vMap.getColor32), a is Bush::shift -- the
    // blade's own wind phase, and 0 on the two vertices that stay rooted in the ground.
    float4 Color    : COLOR0;      // offset 8,  D3DCOLOR
    // The bush's surface normal, biased into 0..1 (hence the *2-1 below). Its ALPHA is not a
    // normal component at all: it is a per-blade brightness added to the vertex colour.
    float4 Normal   : NORMAL;      // offset 12, D3DCOLOR
    // xy: the blade's texture coordinate in the grass atlas, pre-multiplied by 10000 so it
    //     survives as a short (undone by *0.0001 below).
    // z:  ±half-width of the blade, in world units.
    // w:  the blade's height, in world units.
    int4   TexCoord : TEXCOORD0;   // offset 16, SHORT4
    // Bush::windPower: the time at which this blade was planted. See `sc` below.
    float  PlantTime : TEXCOORD1;  // offset 24, FLOAT1 (stride 28)
};

struct VSOutput
{
    float4 Position   : SV_Position;
    float4 Color      : COLOR0;
    float2 UV         : TEXCOORD0;
    float4 ShadowPos  : TEXCOORD1;
    float2 LightmapUV : TEXCOORD2;
    float  Fog        : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    const float time         = Params.x;
    const float hideDistance = Params.y;
    const bool  oldLighting  = Params.z != 0.0f;

    const float3 pos    = float3(input.Position.xyz);
    const float4 t0     = float4(input.TexCoord);
    const float4 color  = input.Color.bgra;     // Color4c is BGRA in memory
    const float4 normal = input.Normal.bgra;

    // Growth: 0 while the blade is still coming up, 1 once it has. Both the local offset and
    // the lit colour are scaled by it, so a new blade rises AND fades in together.
    const float sc = saturate(time - input.PlantTime);

    const float halfWidth = t0.z;    // ±, so the two sides of the quad separate
    const float height    = t0.w;

    // While sc < 1 the blade is folded down: its tip is `height*(1-sc)` along the camera's
    // forward axis and only `height*sc` up. At sc == 1 it stands straight.
    float3 displ = float3(halfWidth, height * sc, height * (1.0f - sc));

    // Wind. `shift` (the vertex colour's alpha, Bush::shift) is this bush's phase, so
    // neighbouring bushes are at different points in the ripple. t^3 - t is a smooth,
    // zero-mean S over the [-1,1] the frac is remapped into, which is what makes the sway
    // return to centre instead of drifting.
    float2 fTime = frac(pos.xz + color.a * 2.5f * time);
    fTime = fTime * 2.0f - 1.0f;
    fTime = fTime * fTime * fTime - fTime;
    displ.x += (fTime.x + fTime.y) * 2.0f;

    // Distance fade and stretch. distanceSq is against the SQUARED distance, and hideDistance
    // is 1/hideDistance^2, so this reaches 0 exactly at the hide distance.
    const float3 toCamera  = pos - CameraPos.xyz;
    const float  distanceSq = dot(toCamera, toCamera);
    const float  alpha      = 1.0f - distanceSq * hideDistance;
    // Far blades are drawn taller, up to 2x. They cover fewer pixels, so without this they
    // would thin out into nothing before the alpha fade got a chance to hide them.
    displ.y *= min(2.0f, 1.0f + distanceSq * hideDistance);

    // Camera space -> world space: the billboard.
    displ = mul(float4(displ, 0.0f), Billboard).xyz;

    const float4 world = float4(pos + displ, 1.0f);
    output.Position = mul(world, MVP);

    // Lambert against the negated light direction, as everywhere else in this engine.
    // Normal.a is not part of the normal: it is a per-blade brightness, added to the terrain
    // colour the blade inherited.
    const float ndl = -dot(normal.xyz * 2.0f - 1.0f, LightDirection.xyz);
    const float3 albedo = color.rgb + normal.a;

    // The two lighting models the original compiles between. They differ in where the ambient
    // lands: OLD_LIGHTING adds it AFTER the albedo has been modulated, so it is a flat floor
    // independent of the blade's colour; the newer one folds it into the light term, so a dark
    // blade stays dark in shadow. Both scale by sc, which fades a growing blade in.
    output.Color.rgb = oldLighting
        ? (ndl * albedo * SunDiffuse.rgb + SunDiffuse.w) * sc
        : albedo * (ndl * SunDiffuse.rgb + SunDiffuse.w) * sc;
    output.Color.a = alpha;

    output.UV = t0.xy * 0.0001f;   // undo the *10000 that let the uv fit in a short

    output.ShadowPos  = mul(world, Shadow);
    output.LightmapUV = (world.xy - PlanarNode.xy) * PlanarNode.zw;
    output.Fog        = dot(world, FogPlane);
    return output;
}
