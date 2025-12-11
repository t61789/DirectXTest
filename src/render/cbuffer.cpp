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
        assert(!m_using);
        m_dxResource->Unmap(0, nullptr);
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
        if (auto field = find(m_layout->fields, name))
        {
            auto writeSizeB = (std::min)(field->logicSizeB, sizeB);
            memcpy(static_cast<uint8_t*>(m_gpuWriteDest) + field->offsetB, data, writeSizeB); // TODO cmdbuffer
            return true;
        }

        return false;
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
    }
}
