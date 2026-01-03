#if !defined(__COMMON_HLSL_INCLUDED__)
    #define __COMMON_HLSL_INCLUDED__

    #define DESC_HANDLE_POOL_SIZE 0xFFFF
    #define SAMPLER_DESC_POOL_SIZE 0xFF

    #define PIXEL_TYPE_LIT 1.0f

    #define PI 3.14159265359f
    #define EPSION 1e-5f

    struct VSInput 
    {
        float4 positionOS : POSITION;
        float4 normalOS : NORMAL;
        float4 tangentOS : TANGENT;
        float2 uv0 : TEXCOORD0;
        float2 uv1 : TEXCOORD1;
    };

    struct PSInput
    {
        float4 positionCS : SV_POSITION;
        float4 positionSS : TEXCOORD0;
        float3 positionWS : TEXCOORD1;
        float3 normalWS : TEXCOORD2;
        float2 uv0 : TEXCOORD3;
    };

    struct GBufferPSOutput
    {
        float4 color0 : SV_TARGET0;
        float4 color1 : SV_TARGET1;
        float color2 : SV_TARGET2;
    };

    cbuffer GlobalCBuffer : register(b0)
    {
        uint _GBuffer0Tex;
        uint _GBuffer1Tex;
        uint _GBuffer2Tex;

        float4 _MainLightDir;
        float4 _MainLightColor;
    };

    cbuffer PerViewCBuffer : register(b1)
    {
        float4x4 _V;
        float4x4 _P;
        float4x4 _VP;
        float4x4 _IVP;
        float4 _CameraPositionWS;
    };

    cbuffer PerObjectCBuffer : register(b2)
    {
        float4x4 _M;
        float4x4 _IM;
    };

    Texture2D _BindlessTextures[DESC_HANDLE_POOL_SIZE] : register(t0, space1);
    SamplerState _BindlessSamplers[SAMPLER_DESC_POOL_SIZE] : register(s0, space1);

    float4x4 GetLocalToWorld()
    {
        return _M;
    }

    float4x4 GetWorldToLocal()
    {
        return _IM;
    }

    float3 TransformObjectToWorld(float3 positionOS, float4x4 localToWorld)
    {
        return mul(float4(positionOS, 1.0f), localToWorld).xyz;
    }

    float3 TransformObjectToWorld(float3 positionOS)
    {
        return mul(float4(positionOS, 1.0f), GetLocalToWorld()).xyz;
    }

    float3 TransformObjectToView(float3 positionOS)
    {
        return mul(mul(float4(positionOS, 1.0f), GetLocalToWorld()), _V).xyz;
    }

    float4 TransformObjectToHClip(float3 positionOS, float4x4 localToWorld)
    {
        return mul(mul(float4(positionOS, 1.0f), localToWorld), _VP);
    }

    float4 TransformObjectToHClip(float3 positionOS)
    {
        return TransformObjectToHClip(positionOS, GetLocalToWorld());
    }

    float4 TransformWorldToHClip(float3 positionWS)
    {
        return mul(float4(positionWS, 1.0f), _VP);
    }

    float3 TransformObjectToWorldNormal(float3 normalOS, float4x4 worldToLocal)
    {
        return mul(float4(normalOS, 0.0f), transpose(worldToLocal)).xyz;
    }

    float3 TransformObjectToWorldNormal(float3 normalOS)
    {
        return TransformObjectToWorldNormal(normalOS, GetWorldToLocal());
    }

    float4 SampleTexture(uint textureIndex, float2 uv)
    {
        uv.y = 1.0f - uv.y;
        uint srvIndex = textureIndex & 0xFFFF;
        uint samplerIndex = (textureIndex >> 20) & 0xFF;
        return _BindlessTextures[srvIndex].Sample(_BindlessSamplers[samplerIndex], uv);
    }

    void ReadGBuffer0(float2 screenUV, out float3 albedo)
    {
        float4 color = SampleTexture(_GBuffer0Tex, screenUV);
        albedo = color.xyz;
    }

    void ReadGBuffer0(float2 screenUV, out float3 albedo, out float pixelType)
    {
        float4 color = SampleTexture(_GBuffer0Tex, screenUV);
        albedo = color.xyz;
        pixelType = color.w;
    }

    void ReadGBuffer1(float2 screenUV, out float3 normalWS)
    {
        float4 color = SampleTexture(_GBuffer1Tex, screenUV);
        normalWS = color.xyz * 2.0f - 1.0f;
    }

    void ReadGBuffer2(float2 screenUV, out float depth)
    {
        float4 color = SampleTexture(_GBuffer2Tex, screenUV);
        depth = color.r;
    }

    GBufferPSOutput CreateOutput(float3 albedo, float3 normalWS, float pixelType, float depth)
    {
        GBufferPSOutput output;
        output.color0 = float4(albedo, pixelType);
        output.color1 = float4(normalWS * 0.5f + 0.5f, 0.0f);
        output.color2 = depth;
        return output;
    }

    bool PixelTypeEquals(float pixelType, float checkType)
    {
        return abs(pixelType - checkType) < 0.01f;
    }

    float3 RebuildWorldPosition(float2 screenUv, float depth)
    {
        float4 pos = float4(screenUv * 2.0f - 1.0f, depth, 1.0f);
        pos = mul(transpose(_IVP), pos);
        pos /= pos.w;
        return pos.xyz;
    }

#endif // !defined(__COMMON_HLSL_INCLUDED__)
