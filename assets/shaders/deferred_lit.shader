#include "built_in/shaders/lib/common.hlsl"
#include "built_in/shaders/lib/lighting.hlsl"

PSInput VS_Main(VSInput input)
{
    PSInput output = (PSInput)0;

    output.positionCS = float4(input.positionOS.xyz, 1.0f);
    output.positionSS = output.positionCS;

    return output;
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    float2 screenUv = (input.positionSS.xy / input.positionSS.w) * 0.5f + 0.5f;

    float3 albedo, normalWS;
    float metallic, roughness, pixelType, depth;

    ReadGBuffer0(screenUv, albedo, pixelType);
    ReadGBuffer1(screenUv, normalWS, metallic, roughness);
    ReadGBuffer2(screenUv, depth);

    float3 positionWS = RebuildWorldPosition(screenUv, depth);
    float3 viewDirWS = normalize(_CameraPositionWS.xyz - positionWS);

    float3 litColor = albedo;
    if (PixelTypeEquals(pixelType, PIXEL_TYPE_LIT))
    {
        // litColor = SimpleLit(normalWS);
        litColor = Lit(albedo, positionWS, normalWS, viewDirWS, metallic, roughness);
        // litColor = albedo;
        // litColor = float3(metallic, metallic, metallic);
        // litColor = float3(roughness, roughness, roughness);
        // litColor = normalWS * 0.5f + 0.5f;
    }

    float4 finalColor = float4(litColor, 1.0f);

    finalColor.rgb *= _Exposure;
    finalColor.rgb = ACESFilm(finalColor.rgb);

    return finalColor;
}
