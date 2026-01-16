#include "built_in/shaders/lib/common.hlsl"
#include "built_in/shaders/lib/lighting.hlsl"

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);
    output.positionSS = output.positionCS;

    return output;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    float2 screenUv = (input.positionSS.xy / input.positionSS.w) * 0.5f + 0.5f;

    float3 albedo, normalWS;
    float pixelType, depth;

    ReadGBuffer0(screenUv, albedo, pixelType);
    ReadGBuffer1(screenUv, normalWS);
    ReadGBuffer2(screenUv, depth);

    float3 positionWS = RebuildWorldPosition(screenUv, depth);
    float3 viewDirWS = normalize(_CameraPositionWS.xyz - positionWS);

    float3 litColor = albedo;
    if (PixelTypeEquals(pixelType, PIXEL_TYPE_LIT))
    {
        // litColor = SimpleLit(normalWS);
        litColor = Lit(albedo, positionWS, normalWS, viewDirWS, 0.3f, 0.0f);
    }

    float4 finalColor = float4(litColor, 1.0f);
    return finalColor;
}
