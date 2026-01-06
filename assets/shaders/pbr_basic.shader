#include "built_in/shaders/lib/common.hlsl"

cbuffer PerMaterialCBuffer : register(b3)
{
    uint _MainTex;
};

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionWS = TransformObjectToWorld(input.positionOS.xyz);
    output.positionCS = TransformWorldToHClip(output.positionWS);
    output.positionSS = output.positionCS;
    output.normalWS = TransformObjectToWorldNormal(input.normalOS.xyz);
    output.uv0 = input.uv0;

    return output;
}

GBufferPSOutput PS_Main(PSInput input)
{
    float depth = input.positionSS.z / input.positionSS.w;
    float3 viewDirWS = normalize(_CameraPositionWS.xyz - input.positionWS);

    float4 texColor = SampleTexture(_MainTex, input.uv0);

    float3 albedo = texColor.xyz;

    float3 normalWS = normalize(input.normalWS);

    return CreateOutput(albedo, normalWS, PIXEL_TYPE_LIT, depth);
}
