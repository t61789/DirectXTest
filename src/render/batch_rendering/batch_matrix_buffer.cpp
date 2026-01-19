#include "batch_matrix_buffer.h"

#include "common/utils.h"
#include "render/descriptor_pool.h"
#include "render/dx_buffer.h"
#include "render/dx_resource.h"
#include "render/render_thread.h"

namespace dt
{
    BatchMatrixBuffer::BatchMatrixBuffer()
    {
        m_cpuBuffer.reserve(128);
        RecreateGpuBuffer();
    }

    uint32_t BatchMatrixBuffer::GetBufferIndex() const
    {
        return m_gpuBuffer->GetShaderResource()->GetSrvIndex();
    }

    size_t BatchMatrixBuffer::Alloc()
    {
        auto key = Utils::GetRandomSizeT();

        m_cpuBuffer.emplace_back();
        m_keyMapper[key] = m_cpuBuffer.size() - 1;
        m_indexMapper[m_cpuBuffer.size() - 1] = key;

        return key;
    }

    void BatchMatrixBuffer::Set(const size_t key, cr<BatchMatrix> matrix)
    {
        m_cpuBuffer[m_keyMapper.at(key)] = matrix;
    }

    void BatchMatrixBuffer::Upload()
    {
        m_gpuBuffer->Write(0, m_cpuBuffer.size() * sizeof(BatchMatrix), m_cpuBuffer.data());
    }

    uint32_t BatchMatrixBuffer::GetMatrixIndex(const size_t key)
    {
        return m_keyMapper.at(key);
    }

    void BatchMatrixBuffer::Swap(const uint32_t index0, const uint32_t index1)
    {
        std::swap(m_cpuBuffer[index0], m_cpuBuffer[index1]);
        auto key0 = m_indexMapper[index0];
        auto key1 = m_indexMapper[index1];
        m_indexMapper[index0] = key1;
        m_indexMapper[index1] = key0;
        m_keyMapper[key0] = index1;
        m_keyMapper[key1] = index0;
    }

    void BatchMatrixBuffer::RecreateGpuBuffer()
    {
        auto targetSizeB = m_cpuBuffer.capacity() * sizeof(BatchMatrix);
        if (!m_gpuBuffer || m_gpuBuffer->GetCapacity() != targetSizeB)
        {
            m_gpuBuffer = DxBuffer::Create(targetSizeB, L"Batch Matrix Buffer");
        }
    }
}
