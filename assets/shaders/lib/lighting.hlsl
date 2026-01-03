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

        return (diffuse + specular) * mainLight.color * ndl;
    }

#endif // !defined(__LIGHTING_HLSL_INCLUDED__)
