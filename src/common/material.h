#pragma once

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "data_set.h"
#include "i_resource.h"
#include "render/render_state.h"
#include "math.h"
#include "render/pso.h"

namespace dt
{
    class Cbuffer;
    class Shader;

    using namespace Microsoft::WRL;

    class Material final : public IResource
    {
    public:
        Shader* GetShader() const { return m_shader.get(); }
        Cbuffer* GetCbuffer() const { return m_cbuffer.get(); }
        cr<StringHandle> GetPath() override { return m_path; }
        DepthMode GetDepthMode() const { return m_depthMode; }
        BlendMode GetBlendMode() const { return m_blendMode; }
        CullMode GetCullMode() const { return m_cullMode; }
        bool GetDepthWrite() const { return m_depthWrite; }
        
        template <typename T>
        void SetValue(string_hash name, T val);
        template <typename T>
        T GetValue(string_hash name);
        
        void SetValue(string_hash name, const void* val, size_t sizeB);
        void GetValue(string_hash name, void* val, size_t sizeB);

        static sp<Material> LoadFromFile(cr<StringHandle> path);

    private:
        void BindShader(crsp<Shader> shader);

        template <typename T>
        static void TypeCheck();

        DepthMode m_depthMode = DepthMode::LESS;
        bool m_depthWrite = true;
        BlendMode m_blendMode = BlendMode::NONE;
        CullMode m_cullMode = CullMode::BACK;
        
        sp<Shader> m_shader = nullptr;
        sp<Cbuffer> m_cbuffer = nullptr;
        sp<DataSet> m_dataSet = nullptr;

        StringHandle m_path;

        friend class DxHelper;
    };

    template <typename T>
    void Material::SetValue(const string_hash name, T val)
    {
        TypeCheck<T>();
        
        SetValue(name, &val, sizeof(T));
    }

    template <typename T>
    T Material::GetValue(const string_hash name)
    {
        TypeCheck<T>();
        
        T val;
        GetValue(name, &val, sizeof(T));
        return val;
    }

    template <typename T>
    void Material::TypeCheck()
    {
        static_assert(
            std::is_same_v<T, XMFLOAT4>||
            std::is_same_v<T, XMFLOAT4X4>||
            "Invalid type");
    }
}
