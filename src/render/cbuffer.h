#pragma once
#include <d3d12.h>
#include <d3d12shader.h>
#include <wrl/client.h>

#include "param_types.h"
#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    using namespace Microsoft::WRL;

    struct CbufferLayout
    {
        struct Field
        {
            StringHandle name;
            ParamType type;
            uint32_t repeatCount;
            uint32_t offsetB;
            uint32_t logicSizeB; // 理论上这个字段的大小
            uint32_t realSizeB; // 这个字段在cbuffer里的实际大小，包含了padding
        };

        StringHandle name;
        D3D12_SHADER_BUFFER_DESC desc;
        vecpair<string_hash, Field> fields;

        CbufferLayout(ID3D12ShaderReflectionConstantBuffer* cbReflection, cr<D3D12_SHADER_BUFFER_DESC> cbDesc);

        const Field* GetField(const string_hash nameId) { return find(fields, nameId); }
    };
    
    class Cbuffer
    {
    public:
        explicit Cbuffer(sp<CbufferLayout> layout);
        ~Cbuffer();
        Cbuffer(const Cbuffer& other) = delete;
        Cbuffer(Cbuffer&& other) noexcept = delete;
        Cbuffer& operator=(const Cbuffer& other) = delete;
        Cbuffer& operator=(Cbuffer&& other) noexcept = delete;

        sp<CbufferLayout> GetLayout() const { return m_layout; }
        cr<ComPtr<ID3D12Resource>> GetDxResource() const { return m_dxResource; }
        
        bool HasField(const string_hash nameId, const ParamType type, const uint32_t repeatCount) const;

        bool Write(string_hash name, const void* data, uint32_t sizeB);

        template <typename F>
        void ForeachField(F&& f);

    private:
        void CreateDxResource();

        StringHandle m_name;
        
        void* m_gpuWriteDest;
        ComPtr<ID3D12Resource> m_dxResource;

        std::atomic<bool> m_using;

        sp<CbufferLayout> m_layout;
    };

    template <typename F>
    void Cbuffer::ForeachField(F&& f)
    {
        for (auto& field : m_layout->fields)
        {
            f(field.second);
        }
    }
}
