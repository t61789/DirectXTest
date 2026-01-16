#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "common/const.h"
#include "utils/recycle_bin.h"

namespace dt
{
    using namespace Microsoft::WRL;

    struct DxResourceDesc
    {
        D3D12_HEAP_PROPERTIES heapProperties = {};
        D3D12_RESOURCE_DESC resourceDesc = {};
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
        D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;
        D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
        const wchar_t* name = nullptr;
        ComPtr<ID3D12Resource> unmanagedResource = nullptr;
    };
    
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
        
        void CopyTo(crsp<DxResource> dstBuffer, size_t dstOffset, size_t sizeB);
        
        static sp<DxResource> Create(cr<DxResourceDesc> desc);
        static sp<DxResource> GetUploadBuffer(const void* data, size_t sizeB);
        static void ClearUploadBuffers();

    private:
        static DxResource* CreateRaw(DxResourceDesc desc);
            
        ComPtr<ID3D12Resource> m_resource;
        std::atomic<D3D12_RESOURCE_STATES> m_state;
        D3D12_RESOURCE_DESC m_desc;
        D3D12_HEAP_PROPERTIES m_heapProperties;

        // for upload buffer only
        uint32_t m_usedFrameCount = 0;
        void* m_mappedPtr = nullptr;

        inline static vec<DxResource*> m_uploadBufferPool;

        friend class DxHelper;
    };
}
