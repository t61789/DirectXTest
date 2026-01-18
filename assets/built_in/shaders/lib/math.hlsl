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
        float m = floor(saturate(f0) * 255.0f);
        float r = saturate(f1);
        return m + r;
    }

    float2 UnpackTwoFloats(float packed)
    {
        float f0 = floor(packed) / 255.0f;
        float f1 = frac(packed);
        return float2(f0, f1);
    }

#endif
