#if !defined(__LIGHTING_HLSL_INCLUDED__)
    #define __LIGHTING_HLSL_INCLUDED__

    #include "built_in/shaders/lib/common.hlsl"

    float GetShadowAttenuation(float3 positionWS)
    {
        float4 shadowPos = mul(float4(positionWS.xyz, 1.0f), _MainLightShadowVP);
        shadowPos /= shadowPos.w;
        shadowPos.xy = shadowPos.xy * 0.5f + 0.5f;

        float shadowDepth = SampleTexture(_MainLightShadowTex, shadowPos.xy).r;
        float currentDepth = shadowPos.z;

        float bias = 0.001f;
        shadowDepth += bias;

        if (currentDepth < shadowDepth || 
            shadowPos.x < 0.0f || shadowPos.x > 1.0f ||
            shadowPos.y < 0.0f || shadowPos.y > 1.0f)
        {
            return 1.0f;
        }
        else
        {
            return 0.0f;
        }
    }

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

    LightData GetPointLight(uint index, float3 positionWS)
    {
        float4 lightInfo0 = _PointLightInfos[index * POINT_LIGHT_STRIDE_VEC4 + 0];
        float4 lightInfo1 = _PointLightInfos[index * POINT_LIGHT_STRIDE_VEC4 + 1];

        float3 lightPositionWS = lightInfo0.xyz;
        float lightRadius = lightInfo0.w;
        float3 lightColor = lightInfo1.xyz;

        float3 toLight = lightPositionWS - positionWS;
        float d = length(toLight);
        float i = 1 / (1 + lightRadius * d);
        lightColor *= i;

        LightData light = (LightData)0;
        light.dirWS = normalize(toLight);
        light.color = lightColor;

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

    float3 DirectLit(LightData light, float3 albedo, float3 positionWS, float3 normalWS, float3 viewDirWS, float roughness, float metallic)
    {
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

        float3 h = normalize(viewDirWS + light.dirWS);
        float ndv = saturate(dot(normalWS, viewDirWS));
        float ndl = saturate(dot(normalWS, light.dirWS));
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

        return (diffuse + specular) * light.color * ndl * GetShadowAttenuation(positionWS);
    }

    float3 IndirectLit(float3 albedo, float3 normalWS, float3 viewDirWS, float3 metallic)
    {
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
        float3 h = normalize(viewDirWS + normalWS);
        float vdh = saturate(dot(viewDirWS, h));
        float3 F = FresnelSchlick(vdh, F0);

        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metallic;

        return IndirectRadiance(normalWS) * albedo * kD;
    }

    float3 Lit(float3 albedo, float3 positionWS, float3 normalWS, float3 viewDirWS, float roughness, float metallic)
    {
        float3 result = float3(0.0f, 0.0f, 0.0f);

        // Main light
        result += DirectLit(GetMainLight(), albedo, positionWS, normalWS, viewDirWS, roughness, metallic);

        // Point lights
        for (uint i = 0; i < _PointLightCount; i++)
        {
            LightData pointLight = GetPointLight(i, positionWS);
            result += DirectLit(pointLight, albedo, positionWS, normalWS, viewDirWS, roughness, metallic);
        }

        // Indirect light
        result += IndirectLit(albedo, normalWS, viewDirWS, metallic);

        return result;
    }

#endif // !defined(__LIGHTING_HLSL_INCLUDED__)
