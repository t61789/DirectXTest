#pragma once

#include "common/math.h"

namespace dt
{
    class DxBuffer;

    struct BatchMatrix
    {
        XMFLOAT4X4 localToWorld;
        XMFLOAT4X4 worldToLocal;
    };
    
    class BatchMatrixBuffer
    {
    public:
        BatchMatrixBuffer();

        uint32_t GetBufferIndex() const;
        
        size_t Alloc();
        void Set(size_t key, cr<BatchMatrix> matrix);
        void RecreateGpuBuffer();
        void Upload();
        uint32_t GetMatrixIndex(size_t key);

    private:
        void Swap(uint32_t index0, uint32_t index1);
        
        umap<uint32_t, size_t> m_indexMapper;
        umap<size_t, uint32_t> m_keyMapper;
        
        vec<BatchMatrix> m_cpuBuffer;
        sp<DxBuffer> m_gpuBuffer;
    };
}
