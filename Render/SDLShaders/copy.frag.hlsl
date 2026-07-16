// Pass-through composite for the SDL GPU post-effect chain: the scene capture, verbatim.
//
// This is the stand-in for "no effect drew after all": the scene was redirected into the
// capture target (cSDLRenderDevice::armSceneCapture), so *something* must put it on the
// swapchain. It has no D3D9 original -- there, the scene was already on the back buffer
// and an idle effect simply did not draw.

Texture2D<float4> Scene        : register(t0, space2);
SamplerState      SceneSampler : register(s0, space2);

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0
{
    return Scene.Sample(SceneSampler, input.UV);
}
