#include "dx_resource.h"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12.h>

#include "directx.h"
#include "dx_helper.h"
#include "render_thread.h"
#include "utils/recycle_bin.h"

namespace dt
{
    void DxResource::CopyTo(crsp<DxResource> dstBuffer, const size_t dstOffset, const size_t sizeB)
    {
        assert(m_heapProperties.Type == D3D12_HEAP_TYPE_DEFAULT);
        assert(m_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && dstBuffer->m_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

        auto curBufferSizeB = m_desc.Width;
        auto dstBufferSizeB = dstBuffer->m_desc.Width;
        
        assert(dstOffset < dstBufferSizeB && sizeB > 0);
        
        auto realSizeB = (std::min)(sizeB, dstBufferSizeB - dstOffset);
        realSizeB = (std::min)(realSizeB, curBufferSizeB);
        
        RT()->AddCmd([self=shared_from_this(), dstBuffer, dstOffset, realSizeB](ID3D12GraphicsCommandList* cmdList)
        {
            auto preSrcState = self->m_state.load();
            auto preDstState = dstBuffer->m_state.load();

            if ((preSrcState & D3D12_RESOURCE_STATE_COPY_SOURCE) == 0)
            {
                DxHelper::AddTransition(self, D3D12_RESOURCE_STATE_COPY_SOURCE);
            }
            
            if (preDstState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                DxHelper::AddTransition(dstBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
            }
            
            DxHelper::ApplyTransitions(cmdList);

            cmdList->CopyBufferRegion(dstBuffer->GetResource(), dstOffset, self->GetResource(), 0, realSizeB);

            if ((preSrcState & D3D12_RESOURCE_STATE_COPY_SOURCE) == 0)
            {
                DxHelper::AddTransition(self, preSrcState);
            }

            if (preDstState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                DxHelper::AddTransition(dstBuffer, preDstState);
            }

            DxHelper::ApplyTransitions(cmdList);
        });
    }

    sp<DxResource> DxResource::Create(cr<DxResourceDesc> desc)
    {
        assert(Utils::IsMainThread());
        
        auto result = CreateRaw(desc);
        return set_recyclable(result);
    }

    sp<DxResource> DxResource::GetUploadBuffer(const void* data, const size_t sizeB)
    {
        assert(Utils::IsMainThread());
        
        DxResource* result = nullptr;
        
        auto it = std::find_if(m_uploadBufferPool.begin(), m_uploadBufferPool.end(), [sizeB](const DxResource* item)
        {
            return item->m_desc.Width == sizeB;
        });
        if (it != m_uploadBufferPool.end())
        {
            result = *it;
            m_uploadBufferPool.erase(it);
        }

        if (!result)
        {
            DxResourceDesc desc;
            desc.heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            desc.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeB);
            desc.initialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            desc.pOptimizedClearValue = nullptr;
            desc.heapFlags = D3D12_HEAP_FLAG_NONE;
            desc.name = L"Upload Buffer";
            desc.unmanagedResource = nullptr;

            result = CreateRaw(desc);
            THROW_IF_FAILED(result->GetResource()->Map(0, nullptr, &result->m_mappedPtr));
        }

        result->m_usedFrameCount = GR()->GetFrameCount();

        if (data)
        {
            memcpy(result->m_mappedPtr, data, sizeB);
        }

        return std::shared_ptr<DxResource>(result, [](DxResource* ptr)
        {
            m_uploadBufferPool.push_back(ptr);
        });
    }

    void DxResource::ClearUploadBuffers() // TODO CALL
    {
        assert(Utils::IsMainThread());

        vec<DxResource*> toDelete;
        for (auto& item : m_uploadBufferPool)
        {
            if (GR()->GetFrameCount() - item->m_usedFrameCount > 10)
            {
                toDelete.push_back(item);
            }
        }

        for (auto& item : toDelete)
        {
            item->m_resource->Unmap(0, nullptr);
            delete item;

            remove(m_uploadBufferPool, item);
        }
    }

    DxResource* DxResource::CreateRaw(DxResourceDesc desc)
    {
        if (desc.unmanagedResource)
        {
            THROW_IF_FAILED(desc.unmanagedResource->GetHeapProperties(&desc.heapProperties, &desc.heapFlags));
            desc.resourceDesc = desc.unmanagedResource->GetDesc();
            if (desc.name)
            {
                THROW_IF_FAILED(desc.unmanagedResource->SetName(desc.name));
            }
        }
        else
        {
            desc.unmanagedResource = Dx()->CreateCommittedResource(
                desc.heapProperties,
                desc.resourceDesc,
                desc.initialResourceState,
                desc.pOptimizedClearValue,
                desc.heapFlags,
                desc.name);
        }

        auto result = new DxResource();
        result->m_resource = desc.unmanagedResource;
        result->m_state = desc.initialResourceState;
        result->m_desc = desc.resourceDesc;
        result->m_heapProperties = desc.heapProperties;

        return result;
    }
}
