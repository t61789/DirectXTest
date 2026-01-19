#include "dx_buffer.h"

#include <directx/d3dx12_core.h>

#include "directx.h"
#include "dx_helper.h"
#include "dx_resource.h"

namespace dt
{
    sp<DxBuffer> DxBuffer::Create(const size_t capacityB, const wchar_t* name)
    {
        assert(capacityB > 0);
        
        auto result = msp<DxBuffer>();
        result->m_name = name;
        
        result->SetCapacity(capacityB);

        return result;
    }

    sp<DxBuffer> DxBuffer::CreateVertexBuffer(const size_t capacityB, uint32_t strideB, const wchar_t* name)
    {
        auto result = Create(capacityB, name);

        result->m_vertexDataStrideB = strideB;

        return result;
    }

    void DxBuffer::SubmitDirtyData()
    {
        auto& b = m_dirtyBuffers;
        for (auto& buffer : m_dirtyBuffers)
        {
            buffer->Submit();
        }

        m_dirtyBuffers.clear();
    }

    sp<ShaderResource> DxBuffer::GetShaderResource()
    {
        if (!m_shaderResource)
        {
            m_shaderResource = DescriptorPool::Ins()->AllocBufferSrv(m_dxResource.get(), m_capacityB);
        }
        return m_shaderResource;
    }

    cr<D3D12_VERTEX_BUFFER_VIEW> DxBuffer::GetVertexBufferView()
    {
        assert(m_vertexDataStrideB.has_value());
        
        if (!m_vertexBufferView.has_value())
        {
            D3D12_VERTEX_BUFFER_VIEW vbView = {};
            vbView.SizeInBytes = m_capacityB;
            vbView.StrideInBytes = m_vertexDataStrideB.value();
            vbView.BufferLocation = m_dxResource->GetResource()->GetGPUVirtualAddress();
            m_vertexBufferView = vbView;
        }

        return m_vertexBufferView.value();
    }

    cr<D3D12_INDEX_BUFFER_VIEW> DxBuffer::GetIndexBufferView()
    {
        if (!m_indexBufferView.has_value())
        {
            D3D12_INDEX_BUFFER_VIEW ibView = {};
            ibView.Format = DXGI_FORMAT_R32_UINT;
            ibView.SizeInBytes = m_capacityB;
            ibView.BufferLocation = m_dxResource->GetResource()->GetGPUVirtualAddress();
            m_indexBufferView = ibView;
        }

        return m_indexBufferView.value();
    }

    void DxBuffer::Write(const size_t offsetB, const size_t sizeB, const void* data)
    {
        m_dirtyBuffers.insert(shared_from_this());
        memcpy(m_cpuBuffer.data() + offsetB, data, sizeB);
    }

    void DxBuffer::Read(const size_t offsetB, const size_t sizeB, void* data)
    {
        memcpy(data, m_cpuBuffer.data() + offsetB, sizeB);
    }

    size_t DxBuffer::GetCapacity() const
    {
        return m_capacityB;
    }

    void DxBuffer::SetCapacity(const size_t capacityB)
    {
        if (m_capacityB == capacityB)
        {
            return;
        }

        DxResourceDesc desc;
        desc.heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        desc.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(capacityB);
        desc.initialResourceState = D3D12_RESOURCE_STATE_COMMON;
        desc.pOptimizedClearValue = nullptr;
        desc.heapFlags = D3D12_HEAP_FLAG_NONE;
        desc.name = m_name ? m_name : L"Dx Buffer";
        desc.unmanagedResource = nullptr;

        auto dxResource = DxResource::Create(desc);

        m_dxResource = dxResource;
        m_cpuBuffer.resize(capacityB);
        m_capacityB = capacityB;
        m_shaderResource = nullptr;
        m_vertexBufferView = std::nullopt;
        m_indexBufferView = std::nullopt;
        
        m_dirtyBuffers.insert(shared_from_this());
    }

    void DxBuffer::Submit()
    {
        auto uploadBuffer = DxResource::GetUploadBuffer(m_cpuBuffer.data(), m_cpuBuffer.size());
        
        RT()->AddCmd([self=shared_from_this(), uploadBuffer](ID3D12GraphicsCommandList* cmdList)
        {
            auto preState = self->m_dxResource->GetState();
            
            if (preState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                DxHelper::AddTransition(self->m_dxResource, D3D12_RESOURCE_STATE_COPY_DEST);
                DxHelper::ApplyTransitions(cmdList);
            }

            cmdList->CopyBufferRegion(self->m_dxResource->GetResource(), 0, uploadBuffer->GetResource(), 0, self->m_capacityB);

            if (preState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                DxHelper::AddTransition(self->m_dxResource, preState);
            }

            DxHelper::ApplyTransitions(cmdList);
        });
    }
}
