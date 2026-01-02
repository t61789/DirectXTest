#pragma once

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "data_set.h"
#include "image.h"
#include "i_resource.h"
#include "render/render_state.h"
#include "math.h"
#include "render/param_types.h"
#include "render/pso.h"

namespace dt
{
    class Cbuffer;
    class Shader;

    using namespace Microsoft::WRL;

    class Material final : public IResource
    {
    public:
        struct Param
        {
            string_hash nameId;
            bool isCbuffer = false;
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
        
        template <typename T>
        void SetParam(string_hash name, T val);
        void SetParam(string_hash nameId, const float* val, uint32_t count);
        void SetParam(string_hash nameId, crsp<ITexture> texture);
        template <typename T>
        T GetParam(string_hash name);
        
        static sp<Material> LoadFromFile(cr<StringHandle> path);

    private:
        void BindShader(crsp<Shader> shader);
        void LoadParams(cr<nlohmann::json> matJson);
        Param* AddParam(string_hash nameId, uint32_t sizeB);
        void SetParamImp(string_hash nameId, const void* val, uint32_t sizeB, crsp<ITexture> texture = nullptr);
        bool GetParamImp(string_hash nameId, void* val, uint32_t sizeB);

        template <typename T>
        static void TypeCheck();
        static bool IsTextureParam(cr<StringHandle> name);
        static Param LoadParamInfo(cr<nlohmann::json> matJson, cr<StringHandle> paramName);
        static uint32_t GetTextureIndex(crsp<ITexture> texture);

        DepthMode m_depthMode = DepthMode::LESS;
        bool m_depthWrite = true;
        BlendMode m_blendMode = BlendMode::NONE;
        CullMode m_cullMode = CullMode::BACK;
        
        sp<Shader> m_shader = nullptr;
        sp<Cbuffer> m_cbuffer = nullptr;

        vecpair<string_hash, Param> m_params;

        StringHandle m_path;

        friend class DxHelper;
        friend class GlobalMaterialParams;
    };

    template <typename T>
    void Material::SetParam(const string_hash name, T val)
    {
        TypeCheck<T>();
        
        SetParamImp(name, &val, sizeof(T), nullptr);
    }

    inline void Material::SetParam(const string_hash nameId, const float* val, const uint32_t count)
    {
        SetParamImp(nameId, val, sizeof(float) * count, nullptr);
    }

    inline void Material::SetParam(const string_hash nameId, crsp<ITexture> texture)
    {
        auto val = GetTextureIndex(texture);
        SetParamImp(nameId, &val, sizeof(uint32_t), texture);
    }

    template <typename T>
    T Material::GetParam(const string_hash name)
    {
        TypeCheck<T>();
        
        T val;
        GetParamImp(name, &val, sizeof(T));
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
