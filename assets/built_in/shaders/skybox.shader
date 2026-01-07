#include "built_in/shaders/lib/common.hlsl"

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = TransformObjectToHClip(input.positionOS.xyz);
    output.positionCS.z = output.positionCS.w - 0.00001f;
    output.normalWS = TransformObjectToWorldNormal(input.normalOS.xyz);

    return output;
}

GBufferPSOutput PS_Main(PSInput input)
{
    float4 texColor = SampleCubeTexture(_SkyboxTex, normalize(input.normalWS));

    float3 albedo = texColor.xyz;

    return CreateOutput(albedo, input.normalWS, PIXEL_TYPE_SKYBOX, 1.0f);
}
