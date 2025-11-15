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
            auto fieldReflection = cbReflection->GetVariableByIndex(i);
            D3D12_SHADER_VARIABLE_DESC fieldDesc;
            THROW_IF_FAILED(fieldReflection->GetDesc(&fieldDesc));
            
            Field field;
            field.name = StringHandle(fieldDesc.Name);
            field.offsetB = fieldDesc.StartOffset;
            field.sizeB = fieldDesc.Size;
            
            fields.emplace_back(field.name.Hash(), std::move(field));
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

    bool Cbuffer::Write(const string_hash name, const void* data, const uint32_t sizeB)
    {
        if (auto field = find(m_layout->fields, name))
        {
            auto writeSizeB = (std::min)(field->sizeB, sizeB);
            memcpy(static_cast<uint8_t*>(m_gpuWriteDest) + field->offsetB, data, writeSizeB); // TODO cmdbuffer
            return true;
        }

        return false;
    }

    void Cbuffer::CreateDxResource()
    {
        CD3DX12_HEAP_PROPERTIES cbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC cbHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(m_layout->desc.Size);
        m_dxResource = Dx()->CreateCommittedResource(
            cbHeapProps,
            cbHeapDesc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            D3D12_HEAP_FLAG_NONE,
            "Cbuffer");
        CD3DX12_RANGE range(0, 0);
        THROW_IF_FAILED(m_dxResource->Map(0, &range, &m_gpuWriteDest));
    }
}
