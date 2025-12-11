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
            string_hash name = 0;
            bool isGlobal = false;
            bool isCbuffer = false;
            uint32_t sizeB = 0;
            sp<Image> image = nullptr;
            vec<uint8_t> data;
        };

        Material() = default;
        ~Material() override;
        Material(const Material& other) = delete;
        Material(Material&& other) noexcept = delete;
        Material& operator=(const Material& other) = delete;
        Material& operator=(Material&& other) noexcept = delete;

        Shader* GetShader() const { return m_shader.get(); }
        Cbuffer* GetCbuffer() const { return m_cbuffer.get(); }
        cr<StringHandle> GetPath() override { return m_path; }
        DepthMode GetDepthMode() const { return m_depthMode; }
        BlendMode GetBlendMode() const { return m_blendMode; }
        CullMode GetCullMode() const { return m_cullMode; }
        bool GetDepthWrite() const { return m_depthWrite; }
        
        template <typename T>
        void SetParam(string_hash name, T val);
        void SetParam(string_hash nameId, const float* val, uint32_t count);
        void SetParam(string_hash nameId, crsp<Image> image);
        template <typename T>
        T GetParam(string_hash name);
        
        static sp<Material> LoadFromFile(cr<StringHandle> path);

    private:
        void BindShader(crsp<Shader> shader);
        void LoadParams(cr<nlohmann::json> matJson);
        Param* AddParam(string_hash nameId, const void* val, uint32_t sizeB, bool isGlobal, crsp<Image> image);
        void SetParamImp(string_hash name, const void* val, size_t sizeB, crsp<Image> image = nullptr);
        bool GetParamImp(string_hash name, void* val, uint32_t sizeB);
        void OnGlobalParamChanged(string_hash nameId, const void* val, size_t sizeB);
        void DoWriteParam(Param* param, const void* val, uint32_t sizeB);

        template <typename T>
        static void TypeCheck();
        static bool IsTextureParam(cr<StringHandle> name);
        static Param LoadParam(cr<nlohmann::json> matJson, cr<StringHandle> paramName);

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

    inline void Material::SetParam(const string_hash nameId, crsp<Image> image)
    {
        auto i = image->GetSrvDescIndex();
        SetParamImp(nameId, &i, sizeof(i), nullptr);
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
