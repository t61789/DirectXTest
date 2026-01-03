#if !defined(__LIGHTING_HLSL_INCLUDED__)
    #define __LIGHTING_HLSL_INCLUDED__

    #include "lib/common.hlsl"

    struct LightData
    {
        float3 dirWS;
        float3 color;
    };

    LightData GetMainLight()
    {
        LightData light = (LightData)0;
        light.dirWS = normalize(_MainLightDir.xyz);
        light.color = _MainLightColor.xyz;
        return light;
    }

    float3 SimpleLit(float3 normalWS)
    {
        LightData mainLight = GetMainLight();
        return saturate(dot(mainLight.dirWS, normalWS) * 0.5f + 0.5f) * mainLight.color;
    }

    float TakeShc(uint index)
    {
        uint vecIndex = index >> 2;
        uint componentIndex = index & 3;
        return _Shc[vecIndex][componentIndex];
    }

    float3 IndirectRadiance(float3 direction)
    {
        direction = normalize(direction);
        
        float baseShc[9];
        baseShc[0] = 0.282095f;
        baseShc[1] = 0.488603f * direction.y;
        baseShc[2] = 0.488603f * direction.z;
        baseShc[3] = 0.488603f * direction.x;
        baseShc[4] = 1.092548f * direction.y * direction.x;
        baseShc[5] = 1.092548f * direction.y * direction.z;
        baseShc[6] = 0.315392f * (-direction.x * direction.x - direction.y * direction.y + 2 * direction.z * direction.z);
        baseShc[7] = 1.092548f * direction.z * direction.x;
        baseShc[8] = 0.546274f * (direction.x * direction.x - direction.y * direction.y);
        
        float3 result = float3(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 9; i++)
        {
            result.r += baseShc[i] * TakeShc(i * 3 + 0);
            result.g += baseShc[i] * TakeShc(i * 3 + 1);
            result.b += baseShc[i] * TakeShc(i * 3 + 2);
        }
        
        return result;
    }

    float DistributionGGX(float ndh, float roughness)
    {
        float a = roughness * roughness;
        float a2 = a * a;
        float denom = (ndh * ndh) * (a2 - 1.0f) + 1.0f;
        return a2 / (PI * denom * denom + EPSION);
    }

    float3 FresnelSchlick(float cosTheta, float3 F0)
    {
        return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
    }

    float GeometrySchlickGGX(float ndv, float roughness)
    {
        float r = roughness + 1.0f;
        float k = (r * r) / 8.0f;
        return ndv / (ndv * (1.0f - k) + k + EPSION);
    }

    float GeometrySmith(float ndv, float ndl, float roughness)
    {
        float ggx1 = GeometrySchlickGGX(ndv, roughness);
        float ggx2 = GeometrySchlickGGX(ndl, roughness);
        return ggx1 * ggx2;
    }

    float3 Lit(float3 albedo, float3 normalWS, float3 viewDirWS, float roughness, float metallic)
    {
        LightData mainLight = GetMainLight();

        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

        float3 h = normalize(viewDirWS + mainLight.dirWS);
        float ndv = saturate(dot(normalWS, viewDirWS));
        float ndl = saturate(dot(normalWS, mainLight.dirWS));
        float ndh = saturate(dot(normalWS, h));
        float vdh = saturate(dot(viewDirWS, h));

        float D = DistributionGGX(ndh, roughness);
        float G = GeometrySmith(ndv, ndl, roughness);
        float3 F = FresnelSchlick(vdh, F0);

        float3 numerator = D * G * F;
        float denominator = 4.0f * ndv * ndl + EPSION;
        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metallic;
        float3 diffuse = kD * albedo / PI;

        float directResult = (diffuse + specular) * mainLight.color * ndl;

        float3 indirectResult = IndirectRadiance(normalWS) * albedo * kD;

        return directResult + indirectResult;
    }

#endif // !defined(__LIGHTING_HLSL_INCLUDED__)
