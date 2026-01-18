#include "material.h"

#include <deque>
#include <DirectXMath.h>

#include "image.h"
#include "shader.h"
#include "shader_variants.h"
#include "variant_keyword.h"
#include "game/game_resource.h"
#include "utils/data_table.h"

namespace dt
{
    sp<Material> Material::CreateFromShader(cr<StringHandle> shaderPath, cr<VariantKeyword> keywords)
    {
        auto shaderVariants = ShaderVariants::LoadFromFile(shaderPath);
        auto shader = shaderVariants->GetShader(keywords);
        
        auto result = msp<Material>();
        result->m_shaderVariants = shaderVariants;
        result->m_shader = shader;
        result->m_shaderKeywords = keywords;
        result->m_dataTable = msp<DataTable>();
        
        result->RebindShader();

        return result;
    }

    sp<Material> Material::LoadFromFile(cr<StringHandle> path)
    {
        {
            if (auto result = GR()->GetResource<Material>(path))
            {
                return result;
            }
        }

        auto matJson = Utils::LoadJson(path);

        vec<str> keywordsStr;
        try_get_val(matJson, "keywords", keywordsStr);
        auto shaderPath = matJson.at("shader").get<str>();
        
        auto shaderVariants = ShaderVariants::LoadFromFile(shaderPath);
        auto shaderKeywords = VariantKeyword(keywordsStr);
        auto shader = shaderVariants->GetShader(shaderKeywords);

        auto result = msp<Material>();
        result->m_shaderVariants = shaderVariants;
        result->m_shaderKeywords = std::move(shaderKeywords);
        result->m_shader = shader;
        result->m_dataTable = msp<DataTable>();
        
        result->LoadParams(matJson);
        result->RebindShader();

        GR()->RegisterResource(path, result);
        result->m_path = path;
        
        log_info("Load material: %s", path.CStr());
        
        return result;
    }

    void Material::RebindShader()
    {
        m_cbuffer = m_shader->CreateCbuffer();
        if (m_cbuffer)
        {
            m_dataTable->ForeachParam([this](cr<DataParamInfo> paramInfo)
            {
                m_cbuffer->Write(paramInfo.key, paramInfo.data.data(), paramInfo.data.size());
            });
        }

        rebindShaderEvent.Invoke();
    }

    void Material::LoadParams(cr<nlohmann::json> matJson)
    {
        auto finalMatJson = matJson;
        for (const auto& param : m_shader->GetDefaultParams().items())
        {
            if (!matJson.contains(param.key()))
            {
                finalMatJson[param.key()] = param.value();
            }
        }
        
        for (const auto& elem : finalMatJson.items())
        { 
            const auto& elemKey = StringHandle(elem.key());
            const auto& elemValue = elem.value();

            if (elemKey.Str() == "depthMode")
            {
                m_depthMode = RenderState::GetDepthMode(elemValue.get<str>());
                continue;
            }

            if (elemKey.CStr() == "depthWrite")
            {
                m_depthWrite = elemValue.get<bool>();
                continue;
            }

            if (elemKey.Str() == "cullMode")
            {
                m_cullMode = RenderState::GetCullMode(elemValue.get<str>());
                continue;
            }

            if (elemKey.Str() == "blendMode")
            {
                m_blendMode = RenderState::GetBlendMode(elemValue.get<str>());
                continue;
            }

            if (elemKey.Str() == "keywords")
            {
                continue;
            }

            if (elemKey.Str() == "shader")
            {
                continue;
            }
            
            if (elemKey.Str().empty() ||
                elemKey.Str()[0] != '_' ||
                find(m_params, elemKey.Hash()))
            {
                continue;
            }

            m_dataTable->AddParam(elemKey, elemValue);
        }
    }

    void Material::SetParamImp(crstrh name, const void* val, const uint32_t sizeB)
    {
        assert(Utils::IsMainThread());

        m_dataTable->Set(name, val, sizeB);
        if (m_cbuffer)
        {
            m_cbuffer->Write(name, val, sizeB);
        }
    }

    void Material::SetParamImp(crstrh name, sp<ITexture> texture)
    {
        assert(Utils::IsMainThread());

        if (!texture)
        {
            texture = GR()->errorTex;
        }
        
        auto val = texture->GetTextureIndex();
        m_dataTable->Set(name, texture);
        if (m_cbuffer)
        {
            m_cbuffer->Write(name, &val, sizeof(val));
        }
    }

    bool Material::GetParamImp(const string_hash nameId, void* val, const uint32_t sizeB)
    {
        assert(Utils::IsMainThread());
        
        auto param = find(m_params, nameId);
        if (!param)
        {
            return false;
        }

        auto clampedSizeB = (std::min)(sizeB, param->sizeB);
        memcpy(val, param->data.data(), clampedSizeB);

        return true;
    }
}
