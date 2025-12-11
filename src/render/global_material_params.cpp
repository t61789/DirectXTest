#include "global_material_params.h"

#include "common/image.h"
#include "common/material.h"

namespace dt
{
    GlobalMaterialParams::GlobalMaterialParams()
    {
        m_dataSet = msp<DataSet>();
    }

    GlobalMaterialParams::~GlobalMaterialParams()
    {
        for (auto& param : m_params)
        {
            assert(param.second.listenerMaterials.empty());
        }
    }

    void GlobalMaterialParams::SetParam(const string_hash nameId, const void* val, const size_t sizeB)
    {
        auto param = find(m_params, nameId);

        if (param == nullptr)
        {
            param = CreateParam(nameId, sizeB);
        }

        if (param->sizeB == ~0u)
        {
            param->sizeB = sizeB;
        }
        
        if (m_dataSet->TrySetImp(param->nameId, val, sizeB)) // TODO render前统一设置一次
        {
            for (auto& material : param->listenerMaterials)
            {
                material->OnGlobalParamChanged(nameId, val, sizeB);
            }
        }
    }

    bool GlobalMaterialParams::GetParam(const string_hash nameId, void* val, const size_t sizeB)
    {
        return m_dataSet->TryGetImp(nameId, val, sizeB);
    }

    void GlobalMaterialParams::SetParam(const string_hash name, const float* arr, const uint32_t count)
    {
        SetParam(name, static_cast<const void*>(arr), sizeof(float) * count);
    }

    void GlobalMaterialParams::SetParam(const string_hash name, crsp<Image> image)
    {
        auto i = image->GetSrvDescIndex();
        SetParam(name, &i, sizeof(uint32_t));
        
        auto param = find(m_params, name);
        param->image = image;
    }

    void GlobalMaterialParams::RegisterParam(const string_hash nameId, Material* material)
    {
        auto param = find(m_params, nameId);
        
        if (param == nullptr)
        {
            param = CreateParam(nameId, ~0u); // 预留个param位，等待之后设置值时再确定大小
        }

        if (param->sizeB != ~0u)
        {
            vec<uint8_t> val(param->sizeB);
            m_dataSet->TryGetImp(param->nameId, val.data(), param->sizeB);
            material->OnGlobalParamChanged(param->nameId, val.data(), param->sizeB);
        }

        assert(!exists(param->listenerMaterials, material));

        param->listenerMaterials.push_back(material);
    }

    void GlobalMaterialParams::UnregisterParam(const string_hash nameId, Material* material)
    {
        auto param = find(m_params, nameId);

        assert(param);
        
        assert(exists(param->listenerMaterials, material));

        remove(param->listenerMaterials, material);
    }

    GlobalMaterialParams::Param* GlobalMaterialParams::CreateParam(const string_hash nameId, const size_t sizeB)
    {
        Param param;
        param.nameId = nameId;
        param.sizeB = sizeB;

        m_params.emplace_back(nameId, std::move(param));

        return &m_params.back().second;
    }
}
