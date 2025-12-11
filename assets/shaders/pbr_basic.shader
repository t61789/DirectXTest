#include "lib/common.hlsl"

cbuffer PerMaterialCBuffer : register(b3)
{
    float4 _BaseColor;
};

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = TransformObjectToHClip(input.positionOS.xyz);

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    float3 albedo = float3(1.0f, 0.0f, 0.0f);
    albedo += _BaseColor.xyz;
    albedo += _Dummy.xyz;

    float4 finalColor = float4(albedo, 1.0f);

    return finalColor;
}
