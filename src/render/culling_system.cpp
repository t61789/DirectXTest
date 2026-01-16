#include "culling_system.h"

#include <tracy/Tracy.hpp>

namespace dt
{
    CullingSystem::CullingSystem()
    {
    }
    
    void CullingSystem::CullBatch(cr<arr<XMVECTOR, 6>> planes, const uint32_t start, const uint32_t end)
    {
        ZoneScoped;
        
        arr<SimdVec4, 6> plane;
        arr<SimdVec4, 6> planeSign;

        for (uint32_t i = 0; i < 6; ++i)
        {
            plane[i] = SimdVec4(planes[i]);

            auto s = Sign4(planes[i]);
            planeSign[i] = SimdVec4(s);
            planeSign[i].w = XMVectorReplicate(1.0f);
        }
        
        for (uint32_t j = start; j < end; j++)
        {
            auto i = j * 4;

            SimdVec4 center = {
                _mm_load_ps(m_cullData.centerX.Data() + i),
                _mm_load_ps(m_cullData.centerY.Data() + i),
                _mm_load_ps(m_cullData.centerZ.Data() + i),
                XMVectorReplicate(0.0f)
            };
            SimdVec4 extents = {
                _mm_load_ps(m_cullData.extentsX.Data() + i),
                _mm_load_ps(m_cullData.extentsY.Data() + i),
                _mm_load_ps(m_cullData.extentsZ.Data() + i),
                XMVectorReplicate(0.0f)
            };

            auto resultP = XMVectorTrueInt();
            for (uint32_t k = 0; k < 6; ++k)
            {
                auto offset = Mul(extents, planeSign[k]);
                auto a = Add(center, offset);
                auto s = Add(center, offset);
                a.w = s.w = XMVectorReplicate(1.0f);
                auto d0 = Dot(plane[k], a);
                auto d1 = Dot(plane[k], s);
                
                auto cmp_d0 = XMVectorGreaterOrEqual(d0, g_XMZero);
                auto cmp_d1 = XMVectorGreaterOrEqual(d1, g_XMZero);

                resultP = XMVectorAndInt(resultP, XMVectorOrInt(cmp_d0, cmp_d1));
            }

            _mm_store_ps(m_cullData.visible.Data() + i, resultP);
        }
    }

    CullingSystem::CullData::CullData()
    {
        centerX = sl<float>(1024);
        centerY = sl<float>(1024);
        centerZ = sl<float>(1024);
        extentsX = sl<float>(1024);
        extentsY = sl<float>(1024);
        extentsZ = sl<float>(1024);
        visible = sl<float>(1024);
    }

    void CullingSystem::CullData::Add()
    {
        centerX.Add(0);
        centerY.Add(0);
        centerZ.Add(0);
        extentsX.Add(0);
        extentsY.Add(0);
        extentsZ.Add(0);
        visible.Add(1);
    }
}
