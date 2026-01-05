#include "material.h"

#include <deque>
#include <DirectXMath.h>

#include "image.h"
#include "shader.h"
#include "game/game_resource.h"

namespace dt
{
    sp<Material> Material::CreateFromShader(cr<StringHandle> shaderPath)
    {
        auto shader = Shader::LoadFromFile(shaderPath);
        auto result = msp<Material>();
        result->BindShader(shader);

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

        auto result = CreateFromShader(matJson.at("shader").get<str>());
        result->LoadParams(matJson);

        GR()->RegisterResource(path, result);
        result->m_path = path;
        
        log_info("Load material: %s", path.CStr());
        
        return result;
    }

    void Material::BindShader(crsp<Shader> shader)
    {
        m_shader = shader;
        m_cbuffer = shader->CreateCbuffer();
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
            
            if (elemKey.Str().empty() ||
                elemKey.Str()[0] != '_' ||
                find(m_params, elemKey.Hash()))
            {
                continue;
            }

            auto param = LoadParamInfo(finalMatJson, elemKey);
            SetParamImp(param.nameId, param.data.data(), param.sizeB, param.texture);
        }
    }

    Material::Param* Material::AddParam(const string_hash nameId, const uint32_t sizeB)
    {
        Param param;
        param.nameId = nameId;
        param.texture = nullptr;

        const CbufferLayout::Field* cbufferField = nullptr;
        
        if (m_cbuffer)
        {
            cbufferField = m_cbuffer->GetLayout()->GetField(nameId);
        }

        if (cbufferField)
        {
            param.isCbuffer = true;
            param.sizeB = cbufferField->logicSizeB;
        }
        else
        {
            param.isCbuffer = false;
            param.sizeB = sizeB;
        }
        
        param.data.resize(param.sizeB);

        m_params.emplace_back(param.nameId, std::move(param));
        return &m_params.back().second;
    }

    void Material::SetParamImp(const string_hash nameId, const void* val, const uint32_t sizeB, crsp<ITexture> texture)
    {
        assert(Utils::IsMainThread());
        
        auto param = find(m_params, nameId);
        if (!param)
        {
            param = AddParam(nameId, sizeB);
        }

        auto clampedSizeB = (std::min)(sizeB, param->sizeB);
        memcpy(param->data.data(), val, clampedSizeB);

        if (param->isCbuffer)
        {
            m_cbuffer->Write(param->nameId, param->data.data(), param->sizeB);
        }
        
        param->texture = texture;
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

    Material::Param Material::LoadParamInfo(cr<nlohmann::json> matJson, cr<StringHandle> paramName)
    {
        const auto& elemValue = matJson.at(paramName.Str());

        Param param;
        param.nameId = paramName;
        
        if (elemValue.is_number_integer())
        {
            param.sizeB = sizeof(int);
            
            auto val = elemValue.get<int>();
            param.data.resize(param.sizeB);
            memcpy(param.data.data(), &val, param.sizeB);
        }
        else if (elemValue.is_number_float())
        {
            param.sizeB = sizeof(float);
            
            auto val = elemValue.get<float>();
            param.data.resize(param.sizeB);
            memcpy(param.data.data(), &val, param.sizeB);
        }
        else if (Utils::IsVec4(elemValue))
        {
            param.sizeB = sizeof(XMFLOAT4);
            
            auto val = XMFLOAT4(elemValue[0], elemValue[1], elemValue[2], elemValue[3]);
            param.data.resize(param.sizeB);
            memcpy(param.data.data(), &val, param.sizeB);
        }
        else if (IsTextureParam(paramName))
        {
            ASSERT_THROW(elemValue.is_string());
            
            auto image = Image::LoadFromFile(elemValue.get<str>());
            param.texture = image;
            param.sizeB = sizeof(uint32_t);
            
            auto textureIndex = image->GetTextureIndex();
            param.data.resize(param.sizeB);
            memcpy(param.data.data(), &textureIndex, param.sizeB);
        }
        else
        {
            throw std::runtime_error("Unknown material param type");
        }

        return param;
    }

    bool Material::IsTextureParam(cr<StringHandle> name)
    {
        return Utils::EndsWith(name.Str(), "Tex");
    }
}
