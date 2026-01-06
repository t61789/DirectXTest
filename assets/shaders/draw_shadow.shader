#include "built_in/shaders/lib/common.hlsl"

float4 TransformObjectToHClip0(float3 positionOS)
{
    return mul(mul(float4(positionOS, 1.0f), _M), _VP);
}

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = TransformObjectToHClip0(input.positionOS.xyz);

    return output;
}

void PS_Main(PSInput input)
{

}
