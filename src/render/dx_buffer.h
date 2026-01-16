#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "utils/storage.h"

namespace dt
{
    struct ShaderResource;
    using namespace Microsoft::WRL;
    
    class DxResource;

    class DxBuffer : public Storage<DxBuffer>, public std::enable_shared_from_this<DxBuffer>
    {
    public:
        using Storage::Write;
        using Storage::Read;

        sp<DxResource> GetDxResource() const { return m_dxResource; }
        sp<ShaderResource> GetShaderResource();
        cr<D3D12_VERTEX_BUFFER_VIEW> GetVertexBufferView();
        cr<D3D12_INDEX_BUFFER_VIEW> GetIndexBufferView();
        
        void Write(size_t offsetB, size_t sizeB, const void* data);
        void Read(size_t offsetB, size_t sizeB, void* data);
        size_t GetCapacity() const;
        void SetCapacity(size_t capacityB);
        void Submit();
        
        static sp<DxBuffer> Create(size_t capacityB, const wchar_t* name = nullptr);
        static sp<DxBuffer> CreateVertexBuffer(size_t capacityB, uint32_t strideB, const wchar_t* name = nullptr);

        static void SubmitDirtyData();

    private:
        
        const wchar_t* m_name = nullptr;
        size_t m_capacityB = 0;
        sp<DxResource> m_dxResource = nullptr;
        vec<uint8_t> m_cpuBuffer;

        sp<ShaderResource> m_shaderResource = nullptr;
        std::optional<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
        std::optional<D3D12_INDEX_BUFFER_VIEW> m_indexBufferView;
        std::optional<uint32_t> m_vertexDataStrideB;

        inline static uset<sp<DxBuffer>> m_dirtyBuffers;
    };
}
