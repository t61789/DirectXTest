#include "dx_helper.h"

#include <cstdint>
#include <directx/d3dx12_barriers.h>

#include "desc_handle_pool.h"
#include "common/const.h"
#include "common/material.h"
#include "common/mesh.h"
#include "common/shader.h"
#include "common/utils.h"

namespace dt
{
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

        cmdList->SetPipelineState(pso->CurPso());
    }

    void DxHelper::BindCbuffer(ID3D12GraphicsCommandList* cmdList, const Shader* shader, Cbuffer* cbuffer)
    {
        auto bindResource = find_if(shader->GetBindResources(), [&cbuffer](cr<Shader::BindResource> x)
        {
            static auto cbufferRegisterType = GetRegisterType(D3D_SIT_CBUFFER);
            return x.resourceName == cbuffer->GetLayout()->name && x.registerType == cbufferRegisterType;
        });

        if (bindResource)
        {
            cmdList->SetGraphicsRootConstantBufferView(bindResource->rootParameterIndex, cbuffer->GetDxResource()->GetGPUVirtualAddress());
        }
    }

    void DxHelper::BindBindlessTextures(ID3D12GraphicsCommandList* cmdList, const Shader* shader)
    {
        auto bindResource = find_if(shader->GetBindResources(), [](cr<Shader::BindResource> x)
        {
            static auto textureRegisterType = GetRegisterType(D3D_SIT_TEXTURE);
            return x.resourceName == BINDLESS_TEXTURES && x.registerType == textureRegisterType;
        });

        if (bindResource)
        {
            SrvDescPool::Ins()->Bind(cmdList, bindResource->rootParameterIndex);
        }
        
        bindResource = find_if(shader->GetBindResources(), [](cr<Shader::BindResource> x)
        {
            static auto textureRegisterType = GetRegisterType(D3D_SIT_SAMPLER);
            return x.resourceName == BINDLESS_SAMPLERS && x.registerType == textureRegisterType;
        });

        if (bindResource)
        {
            SamplerDescPool::Ins()->Bind(cmdList, bindResource->rootParameterIndex);
        }
    }

    void DxHelper::BindMesh(ID3D12GraphicsCommandList* cmdList, const Mesh* mesh)
    {
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
    }
    
    void DxHelper::AddTransition(crsp<DxResource> resource, const D3D12_RESOURCE_STATES state)
    {
        m_transitions.push_back({ resource, state });
    }

    void DxHelper::ApplyTransitions(ID3D12GraphicsCommandList* cmdList)
    {
        static vec<D3D12_RESOURCE_BARRIER> dxTransitions;
        for (auto& transition : m_transitions)
        {
            if (transition.resource->GetState() == transition.state)
            {
                continue;
            }
            
            dxTransitions.push_back(
                CD3DX12_RESOURCE_BARRIER::Transition(
                    transition.resource->GetResource(),
                    transition.resource->GetState(),
                    transition.state));

            transition.resource->m_state = transition.state;
        }
        
        cmdList->ResourceBarrier(m_transitions.size(), dxTransitions.data());
        
        m_transitions.clear();
        dxTransitions.clear();
    }

    uint32_t DxHelper::GetRegisterType(const D3D_SHADER_INPUT_TYPE resourceType)
    {
        switch (resourceType)
        {
            case D3D_SIT_CBUFFER:
                return 0;
            case D3D_SIT_TEXTURE:
                return 1;
            case D3D_SIT_SAMPLER:
                return 2;
            default:
                THROW_ERROR("Invalid resource type");
        }
    }

    void DxHelper::SetHeaps(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* const* heaps, uint32_t heapCount)
    {
        cmdList->SetDescriptorHeaps(heapCount, heaps);
    }
}
