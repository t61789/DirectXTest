#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "common/const.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class DxResource : public std::enable_shared_from_this<DxResource>
    {
    public:
        DxResource() = default;
        ~DxResource() = default;
        DxResource(const DxResource& other) = delete;
        DxResource(DxResource&& other) noexcept = delete;
        DxResource& operator=(const DxResource& other) = delete;
        DxResource& operator=(DxResource&& other) noexcept = delete;

        ID3D12Resource* GetResource() const { return m_resource.Get(); }
        D3D12_RESOURCE_STATES GetState() const { return m_state; }
        
        void Upload(const void* data, size_t sizeB);
        
        static sp<DxResource> Create(
            cr<D3D12_HEAP_PROPERTIES> heapProperties,
            cr<D3D12_RESOURCE_DESC> desc,
            D3D12_RESOURCE_STATES initialResourceState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue,
            D3D12_HEAP_FLAGS heapFlags,
            const wchar_t* name);

        static sp<DxResource> Create(
            const ComPtr<ID3D12Resource>& resource,
            D3D12_RESOURCE_STATES curState,
            const wchar_t* name);

    private:

        static ComPtr<ID3D12Resource> CreateUploadBuffer(const void* data, size_t sizeB);
        
        ComPtr<ID3D12Resource> m_resource;
        std::atomic<D3D12_RESOURCE_STATES> m_state;
        D3D12_RESOURCE_DESC m_desc;
        D3D12_HEAP_PROPERTIES m_heapProperties;

        friend class DxHelper;
    };
}
