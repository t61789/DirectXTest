#include "lib/common.hlsl"

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);
    output.uv0 = input.uv0;

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    float3 albedo, normalWS;
    float pixelType;

    ReadGBuffer0(input.uv0, albedo, pixelType);
    ReadGBuffer1(input.uv0, normalWS);

    float4 finalColor = float4(albedo, 1.0f);
    if (PixelTypeEquals(pixelType, PIXEL_TYPE_LIT))
    {
        finalColor.rgb = saturate(dot(normalWS, normalize(float3(1,1,1)))) * albedo;
    }

    return finalColor;
}
