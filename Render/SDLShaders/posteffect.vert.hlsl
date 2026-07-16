// Post-effect vertex shader for the SDL GPU backend: one fullscreen triangle.
//
// The D3D9 post effects (VistaRender/postEffects.cpp) drew a pre-transformed fullscreen
// quad through cD3DRender::DrawQuad. Here the whole screen is covered by a single
// triangle generated from SV_VertexID -- no vertex buffer, no uniforms; the pipeline is
// bound and drawn with num_vertices = 3.
//
// UV derivation: SDL GPU normalizes clip space to y-up and texture coordinates to a
// top-left origin (D3D's conventions), so NDC (-1,+1) -- the top-left corner -- must
// sample uv (0,0). With corner in {0,2}: ndc = (2c.x-1, 1-2c.y) and uv comes out as
// exactly `corner`.

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

VSOutput main(uint id : SV_VertexID)
{
    // id 0 -> (0,0), id 1 -> (2,0), id 2 -> (0,2): a triangle whose inner square is the screen.
    float2 corner = float2((id << 1) & 2, id & 2);

    VSOutput output;
    output.Position = float4(corner.x * 2.0f - 1.0f, 1.0f - corner.y * 2.0f, 0.0f, 1.0f);
    output.UV = corner;
    return output;
}
