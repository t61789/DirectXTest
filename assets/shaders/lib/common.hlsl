#define DESC_HANDLE_POOL_SIZE 16384
#define SAMPLER_DESC_POOL_SIZE 100

struct VSInput 
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
    float2 uv0 : TEXCOORD0;
};

struct PSInput
{
    float4 positionCS : SV_POSITION;
    float2 uv0 : TEXCOORD0;
};

cbuffer GlobalCBuffer : register(b0)
{
    float4 _Dummy;
};

cbuffer PerViewCBuffer : register(b1)
{
    float4x4 _VP;
    float4x4 _IVP;
    float4 _CameraPositionWS;
};

cbuffer PerObjectCBuffer : register(b2)
{
    float4x4 _M;
    float4x4 _IM;
};

Texture2D<float4> _BindlessTextures[DESC_HANDLE_POOL_SIZE] : register(t0, space1);
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
    return _BindlessTextures[textureIndex].Sample(_BindlessSamplers[textureIndex], uv);
}
