#include "material.h"

#include <DirectXMath.h>

#include "shader.h"
#include "game/game_resource.h"

namespace dt
{
    void Material::SetValue(const string_hash name, const void* val, const size_t sizeB)
    {
        ASSERT_THROW(m_dataSet->TrySetImp(name, &val, sizeB));
        
        m_cbuffer && m_cbuffer->Write(name, &val, sizeB);
    }

    void Material::GetValue(const string_hash name, void* val, const size_t sizeB)
    {
        ASSERT_THROW(m_dataSet->TryGetImp(name, &val, sizeB));
    }

    void Material::ApplyRenderState(Pso* pso)
    {
        pso->SetDepthMode(m_depthMode);
        pso->SetDepthWrite(m_depthWrite);
        pso->SetCullMode(m_cullMode);
    }

    sp<Material> Material::LoadFromFile(cr<StringHandle> path)
    {
        {
            if (auto result = GR()->GetResource<Material>(path))
            {
                return result;
            }
        }

        auto json = Utils::LoadJson(path);

        auto result = msp<Material>();
        result->m_dataSet = msp<DataSet>();
        
        auto shader = Shader::LoadFromFile(json.at("shader").get<str>());
        
        for (const auto& elem : json.items())
        {
            const auto& elemKey = StringHandle(elem.key());
            const auto& elemValue = elem.value();

            if (elemKey.Str() == "depthMode")
            {
                result->m_depthMode = RenderState::GetDepthMode(elemValue.get<str>());
                continue;
            }

            if (elemKey.CStr() == "depthWrite")
            {
                result->m_depthWrite = elemValue.get<bool>();
                continue;
            }

            if (elemKey.Str() == "cullMode")
            {
                result->m_cullMode = RenderState::GetCullMode(elemValue.get<str>());
                continue;
            }

            if (elemKey.Str() == "blendMode")
            {
                result->m_blendMode = RenderState::GetBlendMode(elemValue.get<str>());
                continue;
            }
            
            if (elemKey.Str()[0] != '_')
            {
                continue;
            }
            
            if (Utils::IsVec4(elemValue))
            {
                auto vec4 = XMFLOAT4(elemValue[0], elemValue[1], elemValue[2], elemValue[3]);
                result->m_dataSet->TrySetImp(elemKey, &vec4, sizeof(XMFLOAT4));
                continue;
            }

            throw std::runtime_error("Unknown material param");
        }
        
        result->BindShader(shader);

        GR()->RegisterResource(path, result);
        result->m_path = path;
        
        log_info("Load material: %s", path.CStr());
        
        return result;
    }

    void Material::BindShader(crsp<Shader> shader)
    {
        m_shader = shader;
        m_cbuffer = shader->CreateCbuffer();
        if (m_cbuffer)
        {
            for (auto& elem : m_dataSet->GetAllData())
            {
                m_cbuffer->Write(elem.nameId, elem.data, elem.sizeB);
            }
        }
    }
}
