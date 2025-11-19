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

cbuffer GlobalCBuffer : register(b0)
{
    float4 _Dummy;
};

cbuffer PerViewCBuffer : register(b1)
{
    float4x4 _VP;
    float4 _CameraPositionWS;
};

cbuffer PerObjectCBuffer : register(b2)
{
    float4x4 _M;
    float4x4 _IM;
    float4 _Test;
};

cbuffer PerMaterialCBuffer : register(b3)
{
    float4 _BaseColor;
};

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    return _Dummy * _CameraPositionWS * _Test * _BaseColor;
}
