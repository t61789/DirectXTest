#pragma once
#include <wrl/client.h>
#include "d3d12.h"

#include "common/const.h"

namespace dt
{
    using namespace Microsoft::WRL;

    class DxBuffer : public std::enable_shared_from_this<DxBuffer>
    {
    public:
        void UploadData(uint32_t destOffsetB, uint32_t sizeB, const void* data);
        
        static sp<DxBuffer> CreateStatic(D3D12_RESOURCE_STATES state, uint32_t sizeB, const void* initData = nullptr);
        static sp<DxBuffer> CreateDynamic(uint32_t sizeB);

    private:
        ComPtr<ID3D12Resource> CreateUploadBuffer();
        
        ComPtr<ID3D12Resource> m_buffer = nullptr;
        uint8_t* m_writeDest = nullptr;
        uint32_t m_sizeB = 0;
        D3D12_RESOURCE_STATES m_resourceStates;
    };
}
