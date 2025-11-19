#include "render_pipeline.h"

#include <d3d12.h>

#include "directx.h"
#include "dx_helper.h"
#include "render_context.h"
#include "window.h"
#include "common/material.h"
#include "common/mesh.h"
#include "game/game_resource.h"
#include "objects/scene.h"

namespace dt
{
    RenderPipeline::RenderPipeline()
    {
        m_testMesh = Mesh::LoadFromFile("meshes/quad.obj");
        m_testMaterial = Material::LoadFromFile("materials/test_mat.json");
    }

    RenderPipeline::~RenderPipeline()
    {
        m_testMaterial.reset();
        m_testMesh.reset();
    }

    void RenderPipeline::Render()
    {
        Dx()->AddCommand([this](ID3D12GraphicsCommandList* cmdList)
        {
            DxHelper::PrepareRendering(cmdList);
            
            DxHelper::SetViewport(cmdList, Dx()->GetSwapChainDesc().BufferDesc.Width, Dx()->GetSwapChainDesc().BufferDesc.Height);

            DxHelper::AddTransition(Dx()->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            DxHelper::ApplyTransitions(cmdList);
        
            auto backBufferTexHandle = Dx()->GetBackBufferHandle();
            cmdList->OMSetRenderTargets(1, &backBufferTexHandle, false, nullptr);

            RenderScene(cmdList, GR()->mainScene);
            
            DxHelper::AddTransition(Dx()->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            DxHelper::ApplyTransitions(cmdList);
        });
        
        Dx()->FlushCommand();
        Dx()->IncreaseFence();
        Dx()->WaitForFence();
        Dx()->PresentSwapChain();
    }

    void RenderPipeline::RenderScene(ID3D12GraphicsCommandList* cmdList, const Scene* scene)
    {
        Shader* shader = nullptr;
        Material* material = nullptr;
        Mesh* mesh = nullptr;
        
        scene->GetRenderTree()->Foreach([&](RenderObject* ro)
        {
            auto rebindRootSignature = ro->shader != shader;
            auto rebindPso = ro->material != material;
            auto rebindAllCbuffer = rebindRootSignature || rebindPso;
            auto rebindPerMaterialCbuffer = rebindAllCbuffer || ro->material != material;
            auto rebindPerViewCbuffer = rebindAllCbuffer;
            auto rebindGlobalCbuffer = rebindAllCbuffer;
            auto rebindMesh = rebindRootSignature || rebindPso || ro->mesh != mesh;
            
            if (rebindRootSignature)
            {
                DxHelper::BindRootSignature(cmdList, ro->material->GetShader());
            }

            if (rebindPso)
            {
                DxHelper::BindPso(cmdList, ro->material);
            }

            if (rebindPerMaterialCbuffer)
            {
                if (ro->material->GetCbuffer())
                {
                    DxHelper::BindCbuffer(cmdList, ro->shader, ro->material->GetCbuffer());
                }
            }

            if (rebindGlobalCbuffer)
            {
                DxHelper::BindCbuffer(cmdList, ro->shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
            }

            if (rebindPerViewCbuffer)
            {
                DxHelper::BindCbuffer(cmdList, ro->shader, GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER).get());
            }
            
            DxHelper::BindCbuffer(cmdList, ro->shader, ro->perObjectCbuffer.get());

            if (rebindMesh)
            {
                DxHelper::BindMesh(cmdList, ro->mesh);
            }
            
            cmdList->DrawIndexedInstanced(ro->mesh->GetIndicesCount(), 1, 0, 0, 0);

            shader = ro->shader;
            material = ro->material;
            mesh = ro->mesh;
        });
    }
}
