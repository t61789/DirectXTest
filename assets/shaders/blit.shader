#include "built_in/shaders/lib/common.hlsl"

cbuffer PerMaterialCBuffer : register(b3)
{
    uint _MainTex;
};

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);
    output.uv0 = input.uv0;

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 col = SampleTexture(_MainTex, input.uv0);

    return float4(col.rgb, 1);
}
