#include "common/math.h"

namespace dt
{
    Bounds Bounds::ToWorld(cr<XMMATRIX> m)
    {
        auto centerWS = center;
        XMVectorSetW(centerWS, 1.0f);
        centerWS = XMVector4Transform(centerWS, m);
        
        XMVECTOR extentsWS = {
            XMVectorGetX(XMVector3Dot(XMVectorAbs(m.r[0]), extents)),
            XMVectorGetX(XMVector3Dot(XMVectorAbs(m.r[1]), extents)),
            XMVectorGetX(XMVector3Dot(XMVectorAbs(m.r[2]), extents))
        };

        return { centerWS, extentsWS };
    }
}
