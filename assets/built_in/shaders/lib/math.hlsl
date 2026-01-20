#if !defined(__MATH_HLSL_INCLUDED__)
    #define __MATH_HLSL_INCLUDED__

    float2 OctWrap(float2 v) 
    {
        return (1.0f - abs(v.yx)) * (float2(v.x >= 0.0f ? 1.0f : -1.0f, v.y >= 0.0f ? 1.0f : -1.0f));
    }

    float2 CompressNormal(float3 n) 
    {
        n /= (abs(n.x) + abs(n.y) + abs(n.z));
        n.xy = n.z >= 0.0f ? n.xy : OctWrap(n.xy);
        n.xy = n.xy * 0.5f + 0.5f;
        return n.xy;
    }

    float3 DecompressNormal(float2 f) 
    {
        f = f * 2.0f - 1.0f;
        float3 n = float3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));
        float t = max(-n.z, 0.0f);
        n.x += n.x >= 0.0f ? -t : t;
        n.y += n.y >= 0.0f ? -t : t;
        return normalize(n);
    }

    float PackTwoFloats(float f0, float f1)
    {
        uint m = (uint)(saturate(f0) * 255.0f);
        uint r = (uint)(saturate(f1) * 255.0f);

        uint packedInt = (m << 8) | r;

        return (float)packedInt / 65535.0f;
    }

    float2 UnpackTwoFloats(float packedValue)
    {
        uint packedInt = (uint)(packedValue * 65535.0f + 0.5f);

        float2 result;
        result.x = (float)((packedInt >> 8) & 0xFF) / 255.0f;
        result.y = (float)(packedInt & 0xFF) / 255.0f;
        return result;
    }

#endif // !defined(__MATH_HLSL_INCLUDED__)
