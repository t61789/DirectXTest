#include "dx_helper.h"

#include <cstdint>
#include <directx/d3dx12_barriers.h>

#include "descriptor_pool.h"
#include "render_pipeline.h"
#include "render_resources.h"
#include "render_target.h"
#include "render_thread.h"
#include "common/const.h"
#include "common/material.h"
#include "common/mesh.h"
#include "common/render_texture.h"
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
        pso->SetDepthWrite(material->m_depthWrite && RenderRes()->curRenderTarget->GetDepthAttachment() != nullptr);
        pso->SetCullMode(material->m_cullMode);
        pso->SetRenderTarget(RenderRes()->curRenderTarget.get());

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
        // bind 2d textures
        auto bindResource = find_if(shader->GetBindResources(), [](cr<Shader::BindResource> x)
        {
            static auto textureRegisterType = GetRegisterType(D3D_SIT_TEXTURE);
            return x.resourceName == BINDLESS_2D_TEXTURES && x.registerType == textureRegisterType;
        });

        if (bindResource)
        {
            cmdList->SetGraphicsRootDescriptorTable(
                bindResource->rootParameterIndex,
                DescriptorPool::Ins()->GetSrvDescHeap()->GetGPUDescriptorHandleForHeapStart());
        }

        // bind cube textures
        bindResource = find_if(shader->GetBindResources(), [](cr<Shader::BindResource> x)
        {
            static auto textureRegisterType = GetRegisterType(D3D_SIT_TEXTURE);
            return x.resourceName == BINDLESS_CUBE_TEXTURES && x.registerType == textureRegisterType;
        });

        if (bindResource)
        {
            cmdList->SetGraphicsRootDescriptorTable(
                bindResource->rootParameterIndex,
                DescriptorPool::Ins()->GetSrvDescHeap()->GetGPUDescriptorHandleForHeapStart());
        }

        // bind samplers
        bindResource = find_if(shader->GetBindResources(), [](cr<Shader::BindResource> x)
        {
            static auto textureRegisterType = GetRegisterType(D3D_SIT_SAMPLER);
            return x.resourceName == BINDLESS_SAMPLERS && x.registerType == textureRegisterType;
        });

        if (bindResource)
        {
            cmdList->SetGraphicsRootDescriptorTable(
                bindResource->rootParameterIndex,
                DescriptorPool::Ins()->GetSamplerDescHeap()->GetGPUDescriptorHandleForHeapStart());
        }
    }

    void DxHelper::BindMesh(ID3D12GraphicsCommandList* cmdList, const Mesh* mesh)
    {
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
    }
    
    void DxHelper::AddTransition(crsp<DxResource> resource, const D3D12_RESOURCE_STATES state)
    {
        auto transition = find_if(RenderRes()->transitions, [resource](crpair<sp<DxResource>, D3D12_RESOURCE_STATES> x)
        {
            return x.first == resource;
        });
        if (transition)
        {
            transition->second = state;
            return;
        }
        
        RenderRes()->transitions.emplace_back(resource, state);
    }

    void DxHelper::ApplyTransitions(ID3D12GraphicsCommandList* cmdList)
    {
        if (RenderRes()->transitions.empty())
        {
            return;
        }
        
        static vec<D3D12_RESOURCE_BARRIER> dxTransitions;
        for (auto& [resource, state] : RenderRes()->transitions)
        {
            if (resource->GetState() != state)
            {
                dxTransitions.push_back(
                    CD3DX12_RESOURCE_BARRIER::Transition(
                        resource->GetResource(),
                        resource->GetState(),
                        state));

                resource->m_state = state;
            }
        }

        if (dxTransitions.empty())
        {
            return;
        }
        
        cmdList->ResourceBarrier(dxTransitions.size(), dxTransitions.data());
        
        RenderRes()->transitions.clear();
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

    void DxHelper::SetHeaps(ID3D12GraphicsCommandList* cmdList, DescriptorPool* descPool)
    {
        descPool->SetHeaps(cmdList);
    }

    void DxHelper::SetRenderTarget(ID3D12GraphicsCommandList* cmdList, crsp<RenderTarget> renderTarget, const bool clear)
    {
        if (RenderRes()->curRenderTarget == renderTarget)
        {
            return;
        }

        if (RenderRes()->curRenderTarget)
        {
            UnsetRenderTarget(RenderRes()->curRenderTarget);
        }
        
        for (auto& rt : renderTarget->GetColorAttachments())
        {
            AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        if (auto rt = renderTarget->GetDepthAttachment())
        {
            if (rt->GetDxTexture()->GetDesc().format == TextureFormat::SHADOW_MAP)
            {
                AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
            }
        }
        
        ApplyTransitions(cmdList);

        auto& rtvHandles = renderTarget->GetRtvHandles();
        auto& dsvHandle = renderTarget->GetDsvHandle();

        if (clear)
        {
            for (uint32_t i = 0; i < rtvHandles.size(); ++i)
            {
                cmdList->ClearRenderTargetView(rtvHandles[i]->data, &renderTarget->GetColorAttachments()[i]->GetClearColor().x, 0, nullptr);
            }

            if (dsvHandle)
            {
                cmdList->ClearDepthStencilView(dsvHandle->data, D3D12_CLEAR_FLAG_DEPTH, renderTarget->GetDepthAttachment()->GetClearColor().x, 0, 0, nullptr);
            }
        }

        cmdList->OMSetRenderTargets(
            rtvHandles.size(),
            rtvHandles.size() > 0 ? &rtvHandles[0]->data : nullptr,
            true,
            dsvHandle ? &dsvHandle->data : nullptr);

        SetViewport(cmdList, renderTarget->GetSize().x, renderTarget->GetSize().y);

        RenderRes()->curRenderTarget = renderTarget;
    }

    void DxHelper::UnsetRenderTarget(crsp<RenderTarget> renderTarget)
    {
        assert(RenderRes()->curRenderTarget == renderTarget);
        
        for (auto& rt : renderTarget->GetColorAttachments())
        {
            AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        if (auto rt = renderTarget->GetDepthAttachment())
        {
            if (rt->GetDxTexture()->GetDesc().format == TextureFormat::SHADOW_MAP)
            {
                AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
        }

        RenderRes()->curRenderTarget = nullptr;
    }

    void DxHelper::PrepareCmdList(ID3D12GraphicsCommandList* cmdList)
    {
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        SetHeaps(cmdList, DescriptorPool::Ins());
    }

    void DxHelper::Blit(ID3D12GraphicsCommandList* cmdList, const Material* material, crsp<RenderTarget> renderTarget)
    {
        SetRenderTarget(cmdList, renderTarget);

        auto blitMat = material ? material : GameResource::Ins()->blitMat.get();
        auto mesh = GameResource::Ins()->quadMesh.get();
        auto shader = blitMat->GetShader().get();

        BindRootSignature(cmdList, shader);
        BindPso(cmdList, blitMat);
        BindBindlessTextures(cmdList, shader);
        
        BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
        BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER).get());
        if (blitMat->GetCbuffer())
        {
            BindCbuffer(cmdList, shader, blitMat->GetCbuffer());
        }
        BindMesh(cmdList, mesh);
        
        cmdList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);

        UnsetRenderTarget(renderTarget);
    }

    void DxHelper::RenderScene(
        ID3D12GraphicsCommandList* cmdList,
        crvecsp<RenderObject> renderObjects,
        crsp<Cbuffer> viewCbuffer,
        crsp<RenderTarget> renderTarget,
        crsp<Material> replaceMaterial)
    {
        SetRenderTarget(cmdList, renderTarget, true);
        
        Shader* shader = nullptr;
        Material* material = nullptr;
        Mesh* mesh = nullptr;

        for (const auto& ro : renderObjects)
        {
            auto curShader = replaceMaterial ? replaceMaterial->GetShader().get() : ro->shader.get();
            auto curMaterial = replaceMaterial ? replaceMaterial.get() : ro->material.get();
            auto curMesh = ro->mesh.get();
            
            auto rebindRootSignature = curShader != shader;
            auto rebindPso = curMaterial != material;
            auto rebindTextures = rebindRootSignature;
            auto rebindGlobalCbuffer = rebindRootSignature;
            auto rebindPerViewCbuffer = rebindRootSignature;
            auto rebindPerMaterialCbuffer = rebindRootSignature || curMaterial != material;
            auto rebindMesh = curMesh != mesh;
            
            shader = curShader;
            material = curMaterial;
            mesh = curMesh;
            
            if (rebindRootSignature)
            {
                BindRootSignature(cmdList, shader);
            }
        
            if (rebindPso)
            {
                BindPso(cmdList, material);
            }

            if (rebindTextures)
            {
                BindBindlessTextures(cmdList, shader);
            }
        
            if (rebindGlobalCbuffer)
            {
                BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
            }
        
            if (rebindPerViewCbuffer)
            {
                BindCbuffer(cmdList, shader, viewCbuffer.get());
            }
        
            BindCbuffer(cmdList, shader, ro->perObjectCbuffer.get());
        
            if (rebindPerMaterialCbuffer)
            {
                if (material->GetCbuffer())
                {
                    BindCbuffer(cmdList, shader, material->GetCbuffer());
                }
            }
        
            if (rebindMesh)
            {
                BindMesh(cmdList, mesh);
            }
            
            cmdList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);
        }

        UnsetRenderTarget(renderTarget);
    }
}
