#pragma once
#include "common/const.h"

#include "common/math.h"

namespace dt
{
    class CullingSystem
    {
    public:
        CullingSystem();

        void CullingSystem::CullBatch(cr<arr<XMVECTOR, 6>> planes, uint32_t start, uint32_t end);

    private:
        struct CullData
        {
            sl<float> centerX;
            sl<float> centerY;
            sl<float> centerZ;
            sl<float> extentsX;
            sl<float> extentsY;
            sl<float> extentsZ;

            sl<float> visible;

            CullData();

            void Add();
        };

        CullData m_cullData;
    };
}
