#include "dx_helper.h"

#include <cstdint>
#include <directx/d3dx12_barriers.h>

#include "common/const.h"
#include "common/material.h"
#include "common/mesh.h"
#include "common/shader.h"
#include "common/utils.h"

namespace dt
{
    void DxHelper::PrepareRendering(ID3D12GraphicsCommandList* cmdList)
    {
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void DxHelper::SetViewport(ID3D12GraphicsCommandList* cmdList, const uint32_t width, const uint32_t height)
    {
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(width);
        viewport.Height = static_cast<float>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<long>(width);
        scissorRect.bottom = static_cast<long>(height);
        cmdList->RSSetScissorRects(1, &scissorRect);
    }

    void DxHelper::BindRootSignature(ID3D12GraphicsCommandList* cmdList, const Shader* shader)
    {
        cmdList->SetGraphicsRootSignature(shader->GetRootSignature().Get());
    }

    void DxHelper::BindPso(ID3D12GraphicsCommandList* cmdList, const Material* material)
    {
        auto pso = material->GetShader()->GetPso();
        
        pso->SetDepthMode(material->m_depthMode);
        pso->SetDepthWrite(material->m_depthWrite);
        pso->SetCullMode(material->m_cullMode);

        cmdList->SetPipelineState(pso->CurPso().Get());
    }

    void DxHelper::BindCbuffer(ID3D12GraphicsCommandList* cmdList, const Shader* shader, Cbuffer* cbuffer)
    {
        auto bindResource = find_if(shader->GetBindResources(), [&cbuffer](cr<Shader::BindResource> x)
        {
            static auto cbufferRegisterIndex = GetRegisterType(D3D_SIT_CBUFFER);
            return x.resourceName == cbuffer->GetLayout()->name && x.registerIndex == cbufferRegisterIndex;
        });

        if (bindResource)
        {
            cmdList->SetGraphicsRootConstantBufferView(bindResource->rootParameterIndex, cbuffer->GetDxResource()->GetGPUVirtualAddress());
        }
    }

    void DxHelper::BindMesh(ID3D12GraphicsCommandList* cmdList, const Mesh* mesh)
    {
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
    }
    
    void DxHelper::AddTransition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after)
    {
        m_transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after));
    }

    void DxHelper::ApplyTransitions(ID3D12GraphicsCommandList* cmdList)
    {
        cmdList->ResourceBarrier(m_transitions.size(), m_transitions.data());
        m_transitions.clear();
    }

    uint32_t DxHelper::GetRegisterType(const D3D_SHADER_INPUT_TYPE resourceType)
    {
        static umap<D3D_SHADER_INPUT_TYPE, uint32_t> mapper = {
            { D3D_SIT_CBUFFER, 0 },
        };

        return mapper.at(resourceType);
    }
}
