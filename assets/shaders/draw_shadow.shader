#include "built_in/shaders/lib/common.hlsl"

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = TransformObjectToHClip(input.positionOS.xyz);

    return output;
}

void PS_Main(PSInput input)
{

}
