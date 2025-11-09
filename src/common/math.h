#pragma once
#include <DirectXMath.h>

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
}
