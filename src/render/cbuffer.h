#pragma once
#include <d3d12.h>
#include <d3d12shader.h>
#include <set>
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
    
    class Cbuffer : public std::enable_shared_from_this<Cbuffer>
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
        
        bool HasField(string_hash nameId, ParamType type, uint32_t repeatCount) const;

        template <typename T>
        bool Write(string_hash name, T val);
        bool Write(string_hash name, const void* data, uint32_t sizeB);
        
        static void UpdateDirtyCbuffers();

    private:
        void CreateDxResource();
        void ApplyModifies();

        StringHandle m_name;
        
        void* m_gpuWriteDest;
        ComPtr<ID3D12Resource> m_dxResource;

        sp<CbufferLayout> m_layout;

        vecpair<string_hash, vec<uint8_t>> m_modifies; // <fieldName, data>

        inline static std::set<sp<Cbuffer>> s_dirtyCbuffers;
    };

    template <typename T>
    bool Cbuffer::Write(const string_hash name, T val)
    {
        return Write(name, &val, sizeof(T));
    }
}
