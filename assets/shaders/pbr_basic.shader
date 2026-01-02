#include "lib/common.hlsl"

cbuffer PerMaterialCBuffer : register(b3)
{
    uint _MainTex;
    float4 _BaseColor;
};

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = TransformObjectToHClip(input.positionOS.xyz);
    output.positionVS = mul(float4(0.0f, 0.0f, 5.0f, 1.0f), _M);

    return output;
}

GBufferPSOutput PS_Main(PSInput input) : SV_TARGET
{
    float4 texColor = SampleTexture(_MainTex, input.uv0);
    texColor = float4(1,1,1,1);

    float3 albedo = texColor.xyz;
    albedo += _BaseColor.xyz;
    albedo += _Dummy.xyz;

    float4 finalColor = float4(albedo, 1.0f);

    GBufferPSOutput output;
    output.color0 = finalColor;
    output.color1 = float4(input.positionVS.xyz, 1.0f);
    output.color2 = float4(0.0f, 1.0f, 0.0f, 1.0f);
    return output;
}
