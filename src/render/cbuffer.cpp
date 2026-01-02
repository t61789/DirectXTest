#include "cbuffer.h"

#include <stdexcept>
#include <directx/d3dx12_core.h>

#include "directx.h"
#include "common/utils.h"
#include "game/game_resource.h"

namespace dt
{
    CbufferLayout::CbufferLayout(
        ID3D12ShaderReflectionConstantBuffer* cbReflection,
        cr<D3D12_SHADER_BUFFER_DESC> cbDesc)
    {
        desc = cbDesc;
        name = cbDesc.Name;
        fields.resize(desc.Variables);
        for (uint32_t i = 0; i < desc.Variables; i++)
        {
            auto varReflection = cbReflection->GetVariableByIndex(i);
            
            D3D12_SHADER_VARIABLE_DESC varDesc;
            THROW_IF_FAILED(varReflection->GetDesc(&varDesc));

            D3D12_SHADER_TYPE_DESC typeDesc;
            THROW_IF_FAILED(varReflection->GetType()->GetDesc(&typeDesc));
            
            Field field;
            field.name = StringHandle(varDesc.Name);
            field.type = GetParamType(typeDesc, field.repeatCount);
            field.offsetB = varDesc.StartOffset;
            field.logicSizeB = GetLogicSize(field.type, field.repeatCount);
            field.realSizeB = varDesc.Size;

            assert(field.logicSizeB <= field.realSizeB);

            fields[i] = std::make_pair(field.name.Hash(), std::move(field));
        }
    }

    Cbuffer::Cbuffer(sp<CbufferLayout> layout):
        m_layout(std::move(layout))
    {
        CreateDxResource();
    }

    Cbuffer::~Cbuffer()
    {
        m_dxResource->Unmap(0, nullptr);
        m_dxResource.Reset();
    }
    
    bool Cbuffer::HasField(const string_hash nameId, const ParamType type, const uint32_t repeatCount) const
    {
        return find_if(m_layout->fields, [this, nameId, type, repeatCount](cr<std::pair<string_hash, CbufferLayout::Field>> pair)
        {
            auto& field = pair.second;
            return field.name == nameId && field.type == type && field.repeatCount == repeatCount;
        });
    }

    bool Cbuffer::Write(const string_hash name, const void* data, const uint32_t sizeB)
    {
        auto field = find(m_layout->fields, name);
        if (!field)
        {
            return false;
        }
        auto dataSizeB = (std::min)(field->logicSizeB, sizeB);
        
        if (Utils::IsMainThread())
        {
            if (auto modify = find(m_modifies, field->name))
            {
                memcpy(modify->data(), data, dataSizeB);
            }
            else
            {
                vec<uint8_t> modifyData(dataSizeB);
                memcpy(modifyData.data(), data, dataSizeB);

                m_modifies.emplace_back(field->name, std::move(modifyData));
            }

            s_dirtyCbuffers.insert(shared_from_this());
        }
        else
        {
            memcpy(static_cast<uint8_t*>(m_gpuWriteDest) + field->offsetB, data, dataSizeB);
        }

        return true;
    }

    void Cbuffer::ApplyModifies()
    {
        assert(Utils::IsMainThread());

        for (auto& [fieldName, data] : m_modifies)
        {
            auto field = find(m_layout->fields, fieldName);
            assert(field);

            memcpy(static_cast<uint8_t*>(m_gpuWriteDest) + field->offsetB, data.data(), data.size());
        }

        m_modifies.clear();
    }

    void Cbuffer::UpdateDirtyCbuffers()
    {
        assert(Utils::IsMainThread());

        for (auto& cb : s_dirtyCbuffers)
        {
            cb->ApplyModifies();
        }

        s_dirtyCbuffers.clear();
    }

    void Cbuffer::CreateDxResource()
    {
        CD3DX12_HEAP_PROPERTIES cbHeapProps(D3D12_HEAP_TYPE_UPLOAD); // TODO 修改了才用Upload
        CD3DX12_RESOURCE_DESC cbHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(m_layout->desc.Size);
        m_dxResource = Dx()->CreateCommittedResource(
            cbHeapProps,
            cbHeapDesc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            D3D12_HEAP_FLAG_NONE,
            L"Cbuffer");
        CD3DX12_RANGE range(0, 0);
        THROW_IF_FAILED(m_dxResource->Map(0, &range, &m_gpuWriteDest));

        vec<uint8_t> emptyData(m_layout->desc.Size);
        memcpy(m_gpuWriteDest, emptyData.data(), emptyData.size());
    }
}
