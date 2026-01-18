#if !defined(__COMMON_HLSL_INCLUDED__)
    #define __COMMON_HLSL_INCLUDED__

    #include "built_in/shaders/lib/math.hlsl"

    #define DESC_HANDLE_POOL_SIZE 0xFFFF
    #define SAMPLER_DESC_POOL_SIZE 0xFF

    #define PIXEL_TYPE_LIT 0.1f
    #define PIXEL_TYPE_SKYBOX 0.2f

    #define MAX_POINT_LIGHT_COUNT 5
    #define POINT_LIGHT_STRIDE_VEC4 2

    #define PI 3.14159265359f
    #define EPSION 1e-5f

    struct VSInput 
    {
        float4 positionOS : POSITION;
        float4 normalOS : NORMAL;
        float4 tangentOS : TANGENT;
        float2 uv0 : TEXCOORD0;
        float2 uv1 : TEXCOORD1;
        uint instanceId : SV_InstanceID;
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

    cbuffer GlobalCBuffer : register(b0, space1)
    {
        uint _GBuffer0Tex;
        uint _GBuffer1Tex;
        uint _GBuffer2Tex;

        float4 _MainLightDir;
        float4 _MainLightColor;

        uint _PointLightCount;
        float4 _PointLightInfos[MAX_POINT_LIGHT_COUNT * POINT_LIGHT_STRIDE_VEC4]; // para0: xyz: positionWS, w: radius, para1: xyz: color, w: unused

        uint _SkyboxTex;

        float4 _Shc[7];

        uint _MainLightShadowTex;
        float4x4 _MainLightShadowVP;

        uint _BatchMatrices;
    };

    cbuffer PerViewCBuffer : register(b1, space1)
    {
        float4x4 _V;
        float4x4 _P;
        float4x4 _VP;
        float4x4 _IVP;
        float4 _CameraPositionWS;
    };

    cbuffer PerObjectCBuffer : register(b2, space1)
    {
        float4x4 _M;
        float4x4 _IM;
    };

    cbuffer RootConstantsCBuffer : register(b3, space1)
    {
        uint _BatchIndicesBuffer;
    };

    Texture2D _Bindless2dTextures[DESC_HANDLE_POOL_SIZE] : register(t0, space1);
    TextureCube _BindlessCubeTextures[DESC_HANDLE_POOL_SIZE] : register(t0, space2);
    ByteAddressBuffer _BindlessByteBuffers[DESC_HANDLE_POOL_SIZE] : register(t0, space3);
    SamplerState _BindlessSamplers[SAMPLER_DESC_POOL_SIZE] : register(s0, space1);

    #define LoadUintFromByteBuffer(buffer, index) asuint(_BindlessByteBuffers[buffer].Load((index) * 4))
    #define LoadMatrixFromByteBuffer(buffer, index) \
        float4x4( \
            asfloat(_BindlessByteBuffers[buffer].Load4((index) * 64 + 0)), \
            asfloat(_BindlessByteBuffers[buffer].Load4((index) * 64 + 16)), \
            asfloat(_BindlessByteBuffers[buffer].Load4((index) * 64 + 32)), \
            asfloat(_BindlessByteBuffers[buffer].Load4((index) * 64 + 48)) \
        )

    float4x4 GetLocalToWorld()
    {
        return _M;
    }

    float4x4 GetLocalToWorld(uint instanceId)
    {
        uint realInstanceId = LoadUintFromByteBuffer(_BatchIndicesBuffer, instanceId);
        return LoadMatrixFromByteBuffer(_BatchMatrices, realInstanceId * 2);
    }

    float4x4 GetWorldToLocal()
    {
        return _IM;
    }

    float4x4 GetWorldToLocal(uint instanceId)
    {
        uint realInstanceId = LoadUintFromByteBuffer(_BatchIndicesBuffer, instanceId);
        return LoadMatrixFromByteBuffer(_BatchMatrices, realInstanceId * 2 + 1);
    }

    float3 TransformObjectToWorld(float3 positionOS, float4x4 localToWorld)
    {
        return mul(float4(positionOS, 1.0f), localToWorld).xyz;
    }

    float3 TransformObjectToView(float3 positionOS, float4x4 localToWorld)
    {
        return mul(mul(float4(positionOS, 1.0f), localToWorld), _V).xyz;
    }

    float4 TransformObjectToHClip(float3 positionOS, float4x4 localToWorld)
    {
        return mul(mul(float4(positionOS, 1.0f), localToWorld), _VP);
    }

    float4 TransformWorldToHClip(float3 positionWS)
    {
        return mul(float4(positionWS, 1.0f), _VP);
    }

    float3 TransformObjectToWorldNormal(float3 normalOS, float4x4 worldToLocal)
    {
        return mul(float4(normalOS, 0.0f), transpose(worldToLocal)).xyz;
    }

    #if defined(ENABLE_INSTANCING)
        #define TransformObjectToWorld(positionOS) TransformObjectToWorld(positionOS, GetLocalToWorld(input.instanceId))
        #define TransformObjectToHClip(positionOS) TransformObjectToHClip(positionOS, GetLocalToWorld(input.instanceId))
        #define TransformObjectToWorldNormal(normalOS) TransformObjectToWorldNormal(normalOS, GetWorldToLocal(input.instanceId))
    #else
        #define TransformObjectToWorld(positionOS) TransformObjectToWorld(positionOS, GetLocalToWorld())
        #define TransformObjectToHClip(positionOS) TransformObjectToHClip(positionOS, GetLocalToWorld())
        #define TransformObjectToWorldNormal(normalOS) TransformObjectToWorldNormal(normalOS, GetWorldToLocal())
    #endif

    float4 SampleTexture(uint textureIndex, float2 uv)
    {
        uv.y = 1.0f - uv.y;
        uint srvIndex = textureIndex & 0xFFFF;
        uint samplerIndex = (textureIndex >> 20) & 0xFF;
        return _Bindless2dTextures[srvIndex].Sample(_BindlessSamplers[samplerIndex], uv);
    }

    float4 SampleCubeTexture(uint textureIndex, float3 direction)
    {
        uint srvIndex = textureIndex & 0xFFFF;
        uint samplerIndex = (textureIndex >> 20) & 0xFF;
        return _BindlessCubeTextures[srvIndex].Sample(_BindlessSamplers[samplerIndex], direction);
    }

    float4 SampleCubeTextureLod(uint textureIndex, float3 direction, float lod)
    {
        uint srvIndex = textureIndex & 0xFFFF;
        uint samplerIndex = (textureIndex >> 20) & 0xFF;
        return _BindlessCubeTextures[srvIndex].SampleLevel(_BindlessSamplers[samplerIndex], direction, lod);
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

    void ReadGBuffer1(float2 screenUV, out float3 normalWS, out float metallic, out float roughness)
    {
        float4 color = SampleTexture(_GBuffer1Tex, screenUV);
        normalWS = DecompressNormal(color.xy);

        float2 metallicRoughness = UnpackTwoFloats(color.z);
        metallic = metallicRoughness.x;
        roughness = metallicRoughness.y;
    }

    void ReadGBuffer2(float2 screenUV, out float depth)
    {
        float4 color = SampleTexture(_GBuffer2Tex, screenUV);
        depth = color.r;
    }

    GBufferPSOutput CreateOutput(float3 albedo, float3 normalWS, float metallic, float roughness, float pixelType, float depth)
    {
        GBufferPSOutput output;
        output.color0 = float4(albedo, pixelType);
        output.color1 = float4(CompressNormal(normalWS), PackTwoFloats(metallic, roughness), 0.0f);
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
        pos = mul(pos, _IVP);
        pos /= pos.w;
        return pos.xyz;
    }


#endif // !defined(__COMMON_HLSL_INCLUDED__)
