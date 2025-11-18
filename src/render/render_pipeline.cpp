#include "render_pipeline.h"

#include <d3d12.h>

#include "directx.h"
#include "dx_helper.h"
#include "window.h"
#include "common/material.h"
#include "common/mesh.h"
#include "game/game_resource.h"

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
            
            DxHelper::BindRootSignature(cmdList, m_testMaterial.get());
            DxHelper::BindPso(cmdList, m_testMaterial.get());
            DxHelper::BindCbuffer(cmdList, m_testMaterial.get(), m_testMaterial->GetCbuffer());
            DxHelper::BindMesh(cmdList, m_testMesh.get());
            
            cmdList->DrawIndexedInstanced(m_testMesh->GetIndicesCount(), 1, 0, 0, 0);
            
            DxHelper::AddTransition(Dx()->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            DxHelper::ApplyTransitions(cmdList);
        });
        
        Dx()->FlushCommand();
        Dx()->IncreaseFence();
        Dx()->WaitForFence();
        Dx()->PresentSwapChain();
    }
}
