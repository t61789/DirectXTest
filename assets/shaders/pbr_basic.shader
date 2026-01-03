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
    output.normalWS = TransformObjectToWorldNormal(input.normalOS);

    return output;
}

GBufferPSOutput PS_Main(PSInput input)
{
    float4 texColor = SampleTexture(_MainTex, input.uv0);
    texColor = float4(1,1,1,1);

    float3 albedo = texColor.xyz;
    albedo += _BaseColor.xyz;
    albedo += _Dummy.xyz;

    float3 normalWS = normalize(input.normalWS);

    return CreateOutput(albedo, normalWS, PIXEL_TYPE_LIT);
}
