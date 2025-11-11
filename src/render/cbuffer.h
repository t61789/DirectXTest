#pragma once
#include <d3d12.h>
#include <d3d12shader.h>
#include <mutex>
#include <wrl/client.h>

#include "common/const.h"

namespace dt
{
    using namespace Microsoft::WRL;

    class CbufferLayout
    {
    public:
        struct Field
        {
            StringHandle name;
            uint32_t offsetB;
            uint32_t sizeB;
        };

        D3D12_SHADER_BUFFER_DESC desc;
        vecpair<string_hash, Field> fields;

        CbufferLayout(ID3D12ShaderReflectionConstantBuffer* cbReflection, cr<D3D12_SHADER_BUFFER_DESC> cbDesc);
    };
    
    class Cbuffer
    {
        struct Field;
        
    public:
        explicit Cbuffer(CbufferLayout layout);
        ~Cbuffer();
        Cbuffer(const Cbuffer& other) = delete;
        Cbuffer(Cbuffer&& other) noexcept = delete;
        Cbuffer& operator=(const Cbuffer& other) = delete;
        Cbuffer& operator=(Cbuffer&& other) noexcept = delete;
        
        cr<ComPtr<ID3D12Resource>> GetDxResource() const { return m_dxResource; }

        bool Write(string_hash name, const void* data, uint32_t sizeB);

    private:
        void CreateDxResource();
        
        void* m_gpuWriteDest;
        ComPtr<ID3D12Resource> m_dxResource;

        std::atomic<bool> m_using;

        CbufferLayout m_layout;
        
        struct Field
        {
            StringHandle name;
            uint32_t offset;
            uint32_t sizeB;
        };
    };
}
