#pragma once

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "data_set.h"
#include "event.h"
#include "image.h"
#include "i_resource.h"
#include "render/render_state.h"
#include "math.h"
#include "variant_keyword.h"
#include "render/param_types.h"
#include "render/pso.h"

namespace dt
{
    class ShaderVariants;
    class DataTable;
    struct VariantKeyword;
    class Cbuffer;
    class Shader;

    using namespace Microsoft::WRL;

    class Material final : public IResource
    {
    public:
        struct Param
        {
            string_hash nameId;
            uint32_t sizeB = 0;
            sp<ITexture> texture = nullptr;
            vec<uint8_t> data;
        };

        Material() = default;
        ~Material() override = default;
        Material(const Material& other) = delete;
        Material(Material&& other) noexcept = delete;
        Material& operator=(const Material& other) = delete;
        Material& operator=(Material&& other) noexcept = delete;

        sp<Shader> GetShader() const { return m_shader; }
        Cbuffer* GetCbuffer() const { return m_cbuffer.get(); }
        cr<StringHandle> GetPath() override { return m_path; }
        DepthMode GetDepthMode() const { return m_depthMode; }
        BlendMode GetBlendMode() const { return m_blendMode; }
        CullMode GetCullMode() const { return m_cullMode; }
        bool GetDepthWrite() const { return m_depthWrite; }
        VariantKeyword GetShaderKeywords() const { return m_shaderKeywords; }
        sp<ShaderVariants> GetShaderVariants() const { return m_shaderVariants; }
        
        template <typename T>
        void SetParam(crstrh name, T val);
        void SetParam(crstrh name, const float* val, uint32_t count);
        void SetParam(crstrh name, crsp<ITexture> texture);
        template <typename T>
        T GetParam(string_hash nameId);
        
        static sp<Material> CreateFromShader(cr<StringHandle> shaderPath, cr<VariantKeyword> keywords);
        static sp<Material> LoadFromFile(cr<StringHandle> path);

        Event<> rebindShaderEvent;

    private:
        void RebindShader();
        void LoadParams(cr<nlohmann::json> matJson);
        void SetParamImp(crstrh name, const void* val, uint32_t sizeB);
        void SetParamImp(crstrh name, sp<ITexture> texture);
        bool GetParamImp(string_hash nameId, void* val, uint32_t sizeB);

        template <typename T>
        static void TypeCheck();

        DepthMode m_depthMode = DepthMode::LESS;
        bool m_depthWrite = true;
        BlendMode m_blendMode = BlendMode::NONE;
        CullMode m_cullMode = CullMode::BACK;

        sp<ShaderVariants> m_shaderVariants = nullptr;
        sp<Shader> m_shader = nullptr;
        VariantKeyword m_shaderKeywords;
        sp<Cbuffer> m_cbuffer = nullptr;

        vecpair<string_hash, Param> m_params;

        StringHandle m_path;

        sp<DataTable> m_dataTable;

        friend class DxHelper;
        friend class GlobalMaterialParams;
    };

    template <typename T>
    void Material::SetParam(crstrh name, T val)
    {
        TypeCheck<T>();
        
        SetParamImp(name, &val, sizeof(T));
    }

    inline void Material::SetParam(crstrh name, const float* val, const uint32_t count)
    {
        SetParamImp(name, val, sizeof(float) * count);
    }

    inline void Material::SetParam(crstrh name, crsp<ITexture> texture)
    {
        SetParamImp(name, texture);
    }

    template <typename T>
    T Material::GetParam(const string_hash nameId)
    {
        TypeCheck<T>();
        
        T val;
        GetParamImp(nameId, &val, sizeof(T));
        return val;
    }

    template <typename T>
    void Material::TypeCheck()
    {
        static_assert(
            std::is_same_v<T, float> ||
            std::is_same_v<T, int32_t> ||
            std::is_same_v<T, uint32_t> ||
            std::is_same_v<T, XMFLOAT4>||
            std::is_same_v<T, XMFLOAT4X4>||
            "Invalid type");
    }
}
