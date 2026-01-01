#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "common/const.h"
#include "utils/recycle_bin.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class DxResource : public std::enable_shared_from_this<DxResource>, public IRecyclable
    {
    public:
        DxResource() = default;
        ~DxResource() override = default;
        DxResource(const DxResource& other) = delete;
        DxResource(DxResource&& other) noexcept = delete;
        DxResource& operator=(const DxResource& other) = delete;
        DxResource& operator=(DxResource&& other) noexcept = delete;

        D3D12_RESOURCE_DESC GetDesc() const { return m_desc; }
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

        static sp<DxResource> CreateUploadBuffer(const void* data, size_t sizeB);

    private:
        ComPtr<ID3D12Resource> m_resource;
        std::atomic<D3D12_RESOURCE_STATES> m_state;
        D3D12_RESOURCE_DESC m_desc;
        D3D12_HEAP_PROPERTIES m_heapProperties;

        friend class DxHelper;
    };
}
