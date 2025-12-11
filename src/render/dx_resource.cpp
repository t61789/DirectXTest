#include "dx_resource.h"

#include <directx/d3dx12_core.h>

#include "directx.h"
#include "dx_helper.h"
#include "render_thread.h"

namespace dt
{
    void DxResource::Upload(const void* data, const size_t sizeB)
    {
        assert(m_heapProperties.Type == D3D12_HEAP_TYPE_DEFAULT);

        auto uploadBuffer = CreateUploadBuffer(data, sizeB);

        RT()->AddCmd([self=shared_from_this(), uploadBuffer](ID3D12GraphicsCommandList* cmdList)
        {
            auto preState = self->m_state.load();
            
            if (preState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                DxHelper::AddTransition(self, D3D12_RESOURCE_STATE_COPY_DEST);
                DxHelper::ApplyTransitions(cmdList);
            }

            cmdList->CopyResource(self->m_resource.Get(), uploadBuffer.Get());

            if (preState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                DxHelper::AddTransition(self, preState);
            }
        });
    }

    sp<DxResource> DxResource::Create(
        cr<D3D12_HEAP_PROPERTIES> heapProperties,
        cr<D3D12_RESOURCE_DESC> desc,
        const D3D12_RESOURCE_STATES initialResourceState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        const D3D12_HEAP_FLAGS heapFlags,
        const wchar_t* name)
    {
        ComPtr<ID3D12Resource> resource;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateCommittedResource(
            &heapProperties,
            heapFlags,
            &desc,
            initialResourceState,
            pOptimizedClearValue,
            IID_ID3D12Resource,
            &resource));

        if (name)
        {
            THROW_IF_FAILED(resource->SetName(name));
        }

        auto result = msp<DxResource>();
        result->m_resource = resource;
        result->m_state = initialResourceState;
        result->m_desc = desc;
        result->m_heapProperties = heapProperties;

        return result;
    }

    sp<DxResource> DxResource::Create(
        const ComPtr<ID3D12Resource>& resource,
        const D3D12_RESOURCE_STATES curState,
        const wchar_t* name)
    {
        if (name)
        {
            THROW_IF_FAILED(resource->SetName(name));
        }

        D3D12_HEAP_FLAGS heapFlags;
        D3D12_HEAP_PROPERTIES heapProperties;
        THROW_IF_FAILED(resource->GetHeapProperties(&heapProperties, &heapFlags));

        auto result = msp<DxResource>();
        result->m_resource = resource;
        result->m_state = curState;
        result->m_desc = resource->GetDesc();
        result->m_heapProperties = heapProperties;

        return result;
    }

    ComPtr<ID3D12Resource> DxResource::CreateUploadBuffer(const void* data, const size_t sizeB)
    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeB);
        
        ComPtr<ID3D12Resource> buffer;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_ID3D12Resource,
            &buffer));

        void* mappedData;
        THROW_IF_FAILED(buffer->Map(0, nullptr, &mappedData));
        memcpy(mappedData, data, sizeB);
        buffer->Unmap(0, nullptr);

        return buffer;
    }
}
