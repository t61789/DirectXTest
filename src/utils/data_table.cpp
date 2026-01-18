#include "data_table.h"

#include "common/image.h"
#include "common/utils.h"
#include "common/math.h"
#include "game/game_resource.h"

namespace dt
{
    void DataTable::Set(crstrh key, const void* data, const size_t sizeB)
    {
        if (Set(key.Hash(), data, sizeB))
        {
            return;
        }

        AddParam(key, sizeB);

        Set(key.Hash(), data, sizeB);
    }

    bool DataTable::Set(const string_hash key, const void* data, const size_t sizeB)
    {
        if (auto paramInfo = GetParamInfo(key))
        {
            memcpy(paramInfo->data.data(), data, (std::min)(sizeB, paramInfo->data.size()));
            return true;
        }

        return false;
    }

    bool DataTable::Set(const string_hash key, sp<ITexture> texture)
    {
        if (!texture)
        {
            texture = GR()->errorTex;
        }
        
        if (auto paramInfo = GetParamInfo(key))
        {
            assert(paramInfo->data.size() == sizeof(uint32_t));

            auto val = texture->GetSrvDescIndex();
            memcpy(paramInfo->data.data(), &val, sizeof(uint32_t));
            paramInfo->texture = texture;
            return true;
        }

        return false;
    }

    bool DataTable::Get(const string_hash key, void* data, const size_t sizeB)
    {
        if (auto paramInfo = GetParamInfo(key))
        {
            memcpy(data, paramInfo->data.data(), (std::min)(sizeB, paramInfo->data.size()));
            return true;
        }

        return false;
    }

    bool DataTable::Exists(string_hash key)
    {
        return find_if(m_params, [key](CR_ELEM_TYPE(m_params) a)
        {
            return a.key.Hash() == key;
        });
    }

    void DataTable::AddParam(crstrh key, cr<nlohmann::json> value)
    {
        if (value.is_number_integer())
        {
            auto val = value.get<int>();
            AddParam(key, sizeof(int));
            Set(key.Hash(), &val, sizeof(int));
        }
        else if (value.is_number_float())
        {
            auto val = value.get<float>();
            AddParam(key, sizeof(float));
            Set(key.Hash(), &val, sizeof(float));
        }
        else if (Utils::IsVec4(value))
        {
            auto val = XMFLOAT4(value[0], value[1], value[2], value[3]);
            AddParam(key, sizeof(XMFLOAT4));
            Set(key.Hash(), &val, sizeof(XMFLOAT4));
        }
        else if (IsTextureParam(key))
        {
            ASSERT_THROW(value.is_string());
            
            auto image = Image::LoadFromFile(value.get<str>());

            AddParam(key, sizeof(uint32_t));
            Set(key.Hash(), image);
        }
        else
        {
            throw std::runtime_error("Unknown param type");
        }
    }

    void DataTable::AddParam(crstrh key, const size_t sizeB)
    {
        assert(!Exists(key));

        m_params.push_back({
            key,
            vec<uint8_t>(sizeB),
            nullptr
        });
    }

    DataParamInfo* DataTable::GetParamInfo(string_hash key)
    {
        return find_if(m_params, [key](CR_ELEM_TYPE(m_params) a)
        {
            return a.key.Hash() == key;
        });
    }

    bool DataTable::IsTextureParam(crstrh name)
    {
        return Utils::EndsWith(name.Str(), "Tex");
    }
}
