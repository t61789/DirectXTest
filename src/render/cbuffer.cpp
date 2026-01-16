#include "cbuffer.h"

#include <stdexcept>
#include <directx/d3dx12_core.h>

#include "directx.h"
#include "dx_buffer.h"
#include "dx_resource.h"
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
        m_dxBuffer = DxBuffer::Create(m_layout->desc.Size, L"Cbuffer");
    }

    Cbuffer::~Cbuffer()
    {
        m_dxBuffer.reset();
    }

    ID3D12Resource* Cbuffer::GetDxResource() const
    {
        return m_dxBuffer->GetDxResource()->GetResource();
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
        assert(Utils::IsMainThread());
        
        auto field = find(m_layout->fields, name);
        if (!field)
        {
            return false;
        }
        auto dataSizeB = (std::min)(field->logicSizeB, sizeB);

        m_dxBuffer->Write(field->offsetB, dataSizeB, data);
        
        return true;
    }
}
