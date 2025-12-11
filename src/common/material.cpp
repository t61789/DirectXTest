#include "material.h"

#include <deque>
#include <DirectXMath.h>

#include "image.h"
#include "shader.h"
#include "game/game_resource.h"
#include "render/global_material_params.h"

namespace dt
{
    Material::~Material()
    {
        for (auto& [nameId, param] : m_params)
        {
            if (param.isGlobal)
            {
                GlobalMaterialParams::Ins()->UnregisterParam(param.name, this);
            }
        }
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

        auto result = msp<Material>();
        
        auto shader = Shader::LoadFromFile(matJson.at("shader").get<str>());
        result->BindShader(shader);
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
        for (const auto& elem : matJson.items())
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

            auto param = LoadParam(matJson, elemKey);
            AddParam(param.name, param.data.data(), param.sizeB, false, param.image);
        }
        
        if (m_cbuffer)
        {
            // cbuffer里还没被添加的参数就是全局参数了
            m_cbuffer->ForeachField([this](cr<CbufferLayout::Field> field)
            {
                if (find(m_params, field.name))
                {
                    return;
                }

                // 是纹理的话要给个默认值，但不持有引用
                if (IsTextureParam(field.name))
                {
                    auto i = GR()->errorTex->GetSrvDescIndex();
                    AddParam(field.name, &i, field.logicSizeB, true, nullptr);
                }
                else
                {
                    AddParam(field.name, nullptr, field.logicSizeB, true, nullptr);
                }
            });
        }
    }

    Material::Param* Material::AddParam(const string_hash nameId, const void* val, const uint32_t sizeB, const bool isGlobal, crsp<Image> image)
    {
        Param param;
        param.name = nameId;
        param.image = nullptr;
        param.isGlobal = isGlobal;

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
        
        param.data.reserve(param.sizeB);

        m_params.push_back(std::make_pair(param.name, std::move(param)));
        
        auto result = &m_params.back().second;

        if (val)
        {
            DoWriteParam(result, val, sizeB);
        }

        param.image = image;

        if (param.isGlobal)
        {
            GlobalMaterialParams::Ins()->RegisterParam(param.name, this);
        }

        return result;
    }

    void Material::SetParamImp(const string_hash name, const void* val, const size_t sizeB, crsp<Image> image)
    {
        auto param = find(m_params, name);
        if (!param)
        {
            AddParam(name, val, sizeB, false, image);
            return;
        }
        
        if (param->isGlobal)
        {
            param->isGlobal = false;
            GlobalMaterialParams::Ins()->UnregisterParam(param->name, this);
        }
        
        DoWriteParam(param, val, sizeB);
        param->image = image;
    }
    
    void Material::OnGlobalParamChanged(const string_hash nameId, const void* val, const size_t sizeB)
    {
        auto param = find(m_params, nameId);
        
        assert(param && param->isGlobal);

        DoWriteParam(param, val, sizeB);
    }

    void Material::DoWriteParam(Param* param, const void* val, const uint32_t sizeB)
    {
        auto clampedSizeB = (std::min)(sizeB, param->sizeB);
        if (memcmp(param->data.data(), val, clampedSizeB) != 0)
        {
            memcpy(param->data.data(), val, clampedSizeB);

            if (param->isCbuffer)
            {
                m_cbuffer->Write(param->name, param->data.data(), param->sizeB);
            }
        }
    }
    
    bool Material::GetParamImp(const string_hash name, void* val, const uint32_t sizeB)
    {
        auto param = find(m_params, name);
        if (!param)
        {
            return false;
        }

        auto clampedSizeB = (std::min)(sizeB, param->sizeB);
        memcpy(val, param->data.data(), clampedSizeB);

        return true;
    }

    Material::Param Material::LoadParam(cr<nlohmann::json> matJson, cr<StringHandle> paramName)
    {
        const auto& elemValue = matJson.at(paramName.Str());

        Param param;
        param.name = paramName;
        
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
            param.image = image;
            param.sizeB = sizeof(uint32_t);
            
            auto srvDescIndex = image->GetSrvDescIndex();
            param.data.resize(param.sizeB);
            memcpy(param.data.data(), &srvDescIndex, param.sizeB);
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
