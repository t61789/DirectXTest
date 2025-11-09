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

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
