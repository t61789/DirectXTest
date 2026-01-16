#pragma once
#include <DirectXMath.h>
#include <nlohmann/json.hpp>

#include "const.h"

namespace dt
{
    using namespace DirectX;
    
    struct Bounds
    {
        XMVECTOR center = { 0.f, 0.f, 0.f };
        XMVECTOR extents = { 0.1f, 0.1f, 0.1f };

        Bounds() = default;
        Bounds(const XMVECTOR c, const XMVECTOR e) : center(c), extents(e) {}

        Bounds ToWorld(cr<XMMATRIX>);

        template <typename Archive>
        void serialize(Archive& archive, unsigned int version)
        {
            XMFLOAT3 centerFloat3;
            XMStoreFloat3(&centerFloat3, center);
            archive & centerFloat3.x & centerFloat3.y & centerFloat3.z;
            center = XMLoadFloat3(&centerFloat3);

            XMFLOAT3 extentsFloat3;
            XMStoreFloat3(&extentsFloat3, extents);
            archive & extentsFloat3.x & extentsFloat3.y & extentsFloat3.z;
            extents = XMLoadFloat3(&extentsFloat3);
        }
    };
    
    struct SimdVec4
    {
        XMVECTOR x;
        XMVECTOR y;
        XMVECTOR z;
        XMVECTOR w;

        SimdVec4() = default;
        
        explicit SimdVec4(FXMVECTOR v)
        {
            x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
            y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
            z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
            w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
        }
        
        SimdVec4(cr<__m128> x, cr<__m128> y, cr<__m128> z, cr<__m128> w)
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }
    };

    inline SimdVec4 Mul(cr<SimdVec4> a, cr<SimdVec4> b)
    {
        return SimdVec4(
            XMVectorMultiply(a.x, b.x),
            XMVectorMultiply(a.y, b.y),
            XMVectorMultiply(a.z, b.z),
            XMVectorMultiply(a.w, b.w));
    }

    inline SimdVec4 Add(cr<SimdVec4> a, cr<SimdVec4> b)
    {
        return SimdVec4(
            XMVectorAdd(a.x, b.x),
            XMVectorAdd(a.y, b.y),
            XMVectorAdd(a.z, b.z),
            XMVectorAdd(a.w, b.w));
    }

    inline SimdVec4 Sub(cr<SimdVec4> a, cr<SimdVec4> b)
    {
        return SimdVec4(
            XMVectorSubtract(a.x, b.x),
            XMVectorSubtract(a.y, b.y),
            XMVectorSubtract(a.z, b.z),
            XMVectorSubtract(a.w, b.w));
    }

    inline XMVECTOR Dot(cr<SimdVec4> a, cr<SimdVec4> b)
    {
        auto mul = Mul(a, b);
        return XMVectorAdd(XMVectorAdd(XMVectorAdd(mul.x, mul.y), mul.z), mul.w);
    }

    inline XMVECTOR QuaternionToEuler(cr<XMVECTOR> q)
    {
        XMFLOAT4 normalizedQ;
        XMStoreFloat4(&normalizedQ, XMQuaternionNormalize(q));

        XMFLOAT4 result;
        result.x = std::asin(2 * (normalizedQ.w * normalizedQ.x - normalizedQ.y * normalizedQ.z)) * RAD2DEG;
        result.y = std::atan2(2 * (normalizedQ.w * normalizedQ.y + normalizedQ.x * normalizedQ.z),
                              1 - 2 * (normalizedQ.x * normalizedQ.x + normalizedQ.y * normalizedQ.y)) * RAD2DEG;
        result.z = std::atan2(2 * (normalizedQ.w * normalizedQ.z + normalizedQ.x * normalizedQ.y),
                              1 - 2 * (normalizedQ.x * normalizedQ.x + normalizedQ.z * normalizedQ.z)) * RAD2DEG;
        return XMLoadFloat4(&result);
    }

    inline XMVECTOR GetForward(cr<XMMATRIX> m)
    {
        return m.r[2];
    }

    inline XMVECTOR GetUp(cr<XMMATRIX> m)
    {
        return m.r[1];
    }

    inline XMVECTOR GetRight(cr<XMMATRIX> m)
    {
        return m.r[0];
    }

    inline XMVECTOR GetPosition(cr<XMMATRIX> m)
    {
        return m.r[3];
    }

    inline XMFLOAT3 Store3(cr<XMVECTOR> f3)
    {
        XMFLOAT3 result;
        XMStoreFloat3(&result, f3);
        return result;
    }

    inline XMFLOAT4 Store4(cr<XMVECTOR> f4)
    {
        XMFLOAT4 result;
        XMStoreFloat4(&result, f4);
        return result;
    }
    
    inline XMFLOAT4X4 Store(cr<XMMATRIX> m)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, m);
        return result;
    }

    inline XMVECTOR Load(cr<XMFLOAT3> m)
    {
        return XMLoadFloat3(&m);
    }

    inline XMVECTOR Load(cr<XMFLOAT4> m)
    {
        return XMLoadFloat4(&m);
    }

    inline XMMATRIX Load(cr<XMFLOAT4X4> m)
    {
        return XMLoadFloat4x4(&m);
    }

    inline XMFLOAT3 Normalize(cr<XMFLOAT3> f3)
    {
        return Store3(XMVector3Normalize(Load(f3)));
    }

    inline XMMATRIX Inverse(cr<XMMATRIX> m)
    {
        XMVECTOR determinant;
        auto result = XMMatrixInverse(&determinant, m);
        auto det = XMVectorGetX(determinant);
        ASSERT_THROW(std::abs(det) > EPSILON);

        return result;
    }
    
    inline XMFLOAT4X4 Inverse(cr<XMFLOAT4X4> m)
    {
        return Store(Inverse(Load(m)));
    }

    inline XMVECTOR ToRotation(cr<XMVECTOR> eulerAngles)
    {
        auto eaRadius = Store3(eulerAngles * DEG2RAD);
        return XMQuaternionRotationRollPitchYaw(eaRadius.x, eaRadius.y, eaRadius.z);
    }

    inline float Dot3(cr<XMVECTOR> a, cr<XMVECTOR> b)
    {
        return XMVectorGetX(XMVector3Dot(a, b));
    }

    inline float Length3(cr<XMVECTOR> a)
    {
        return XMVectorGetX(XMVector3Length(a));
    }

    inline XMVECTOR Sign4(cr<FXMVECTOR> v)
    {
        auto positiveMask = XMVectorGreater(v, g_XMZero);
        auto negativeMask = XMVectorLess(v, g_XMZero);
    
        auto pos = XMVectorAndInt(positiveMask, g_XMOne);
        auto neg = XMVectorAndInt(negativeMask, g_XMOne);

        return XMVectorSubtract(pos, neg);
    }

    inline void GramSchmidtOrtho(
        cr<XMVECTOR> up,
        cr<XMVECTOR> forward,
        XMVECTOR& resultRight,
        XMVECTOR& resultUp,
        XMVECTOR& resultForward)
    {
        resultForward = XMVector3Normalize(forward);
        resultRight = XMVector3Cross(up, resultForward);
        resultRight = XMVector3Normalize(resultRight);
        resultUp = XMVector3Cross(resultForward, resultRight);
    }

    static void GetFrustumCorners(
        cr<XMVECTOR> cameraRight,
        cr<XMVECTOR> cameraUp,
        cr<XMVECTOR> cameraForward,
        cr<XMVECTOR> cameraPos,
        const float verticalFov,
        const float nearClip,
        const float farClip,
        const float aspect,
        XMVECTOR* corners)
    {
        auto tan = std::tan(verticalFov * 0.5f * DEG2RAD);
        auto nearCenter = cameraPos + cameraForward * nearClip;
        auto farCenter = cameraPos + cameraForward * farClip;
        auto nearHeight = tan * nearClip;
        auto farHeight = tan * farClip;
        auto nearWidth = nearHeight * aspect;
        auto farWidth = farHeight * aspect;

        corners[0] = nearCenter - cameraRight * nearWidth - cameraUp * nearHeight; // nlb
        corners[1] = nearCenter - cameraRight * nearWidth + cameraUp * nearHeight; // nlt
        corners[2] = nearCenter + cameraRight * nearWidth + cameraUp * nearHeight; // nrt
        corners[3] = nearCenter + cameraRight * nearWidth - cameraUp * nearHeight; // nrb
        
        corners[4] = farCenter - cameraRight * farWidth - cameraUp * farHeight; // flb
        corners[5] = farCenter - cameraRight * farWidth + cameraUp * farHeight; // flt
        corners[6] = farCenter + cameraRight * farWidth + cameraUp * farHeight; // frt
        corners[7] = farCenter + cameraRight * farWidth - cameraUp * farHeight; // frb
    }

    inline bool operator==(const XMINT2& lhs, const XMINT2& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    inline bool operator!=(const XMINT2& lhs, const XMINT2& rhs)
    {
        return !(lhs == rhs);
    }
    
    inline bool operator==(const XMFLOAT3& lhs, const XMFLOAT3& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    inline bool operator!=(const XMFLOAT3& lhs, const XMFLOAT3& rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator==(const XMFLOAT4& lhs, const XMFLOAT4& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
    }

    inline bool operator!=(const XMFLOAT4& lhs, const XMFLOAT4& rhs)
    {
        return !(lhs == rhs);
    }
    
    inline XMFLOAT4X4 Transpose(cr<XMFLOAT4X4> val)
    {
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, XMMatrixTranspose(XMLoadFloat4x4(&val)));
        return m;
    }
}

template <>
struct nlohmann::adl_serializer<DirectX::XMFLOAT3>
{
    static DirectX::XMFLOAT3 from_json(const json& j)
    {
        return DirectX::XMFLOAT3 {
            j.at(0).get<float>(),
            j.at(1).get<float>(),
            j.at(2).get<float>()
        };
    }
};

template <>
struct nlohmann::adl_serializer<DirectX::XMVECTOR>
{
    static DirectX::XMVECTOR from_json(const json& j)
    {
        auto f3 = DirectX::XMFLOAT3 {
            j.at(0).get<float>(),
            j.at(1).get<float>(),
            j.at(2).get<float>()
        };

        return XMLoadFloat3(&f3);
    }
};
