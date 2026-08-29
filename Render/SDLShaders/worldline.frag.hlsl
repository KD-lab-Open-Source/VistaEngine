// World-line fragment shader for the SDL GPU backend.
//
// The fragment half of the 3D line primitive: pass the interpolated vertex
// colour through. The D3D original drew lines through the fixed-function
// pipeline (stage 0 = MODULATE(TEXTURE, DIFFUSE) with the white texture), so
// the pixel is just the diffuse colour -- this is that, in one instruction.
//
// SDL GPU requires a fragment shader on every graphics pipeline, so this exists
// even though the vertex shader could carry the colour in SV_Position's shadow.

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
};

float4 main(VSOutput input) : SV_Target
{
    return input.Color;
}
