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
        return m.r[0];
    }

    inline XMVECTOR GetUp(cr<XMMATRIX> m)
    {
        return m.r[1];
    }

    inline XMVECTOR GetRight(cr<XMMATRIX> m)
    {
        return m.r[2];
    }

    inline XMVECTOR GetPosition(cr<XMMATRIX> m)
    {
        return m.r[3];
    }

    inline XMMATRIX Inverse(cr<XMMATRIX> m)
    {
        XMVECTOR determinant;
        auto result = XMMatrixInverse(&determinant, m);
        auto det = XMVectorGetX(determinant);
        ASSERT_THROW(std::abs(det) > EPSILON);

        return result;
    }

    inline bool operator==(const XMINT2& lhs, const XMINT2& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    inline bool operator!=(const XMINT2& lhs, const XMINT2& rhs)
    {
        return !(lhs == rhs);
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
