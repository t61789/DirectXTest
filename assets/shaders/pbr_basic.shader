struct VSInput 
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
    float2 uv0 : TEXCOORD0;
};

struct PSInput
{
    float4 positionCS : SV_POSITION;
};

cbuffer TestBuffer : register(b0)
{
    float4 color;
};

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    return color;
}
