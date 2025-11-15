#include "render_pipeline.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <directx/d3dx12_pipeline_state_stream.h>
#include <directx/d3dx12_barriers.h>

#include "directx.h"
#include "window.h"
#include "common/material.h"
#include "common/mesh.h"
#include "common/shader.h"
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
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = static_cast<float>(Dx()->GetSwapChainDesc().BufferDesc.Width);
            viewport.Height = static_cast<float>(Dx()->GetSwapChainDesc().BufferDesc.Height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            cmdList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = {};
            scissorRect.left = 0;
            scissorRect.top = 0;
            scissorRect.right = Dx()->GetSwapChainDesc().BufferDesc.Width;
            scissorRect.bottom = Dx()->GetSwapChainDesc().BufferDesc.Height;
            cmdList->RSSetScissorRects(1, &scissorRect);

            Dx()->AddTransition(Dx()->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            Dx()->ApplyTransitions(cmdList);
        
            auto backBufferTexHandle = Dx()->GetBackBufferHandle();
            cmdList->OMSetRenderTargets(1, &backBufferTexHandle, false, nullptr);
            
            cmdList->SetGraphicsRootSignature(m_testMaterial->GetShader()->GetRootSignature().Get());
            
            m_testMaterial->ApplyRenderState(m_testMaterial->GetShader()->GetPso());
            cmdList->SetPipelineState(m_testMaterial->GetShader()->GetPso()->CurPso().Get());
            
            cmdList->SetGraphicsRootConstantBufferView(0, m_testMaterial->GetCbuffer()->GetDxResource()->GetGPUVirtualAddress());
            
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            cmdList->IASetIndexBuffer(&m_testMesh->GetIndexBufferView());
            
            cmdList->IASetVertexBuffers(0, 1, &m_testMesh->GetVertexBufferView());
            
            cmdList->DrawIndexedInstanced(m_testMesh->GetIndicesCount(), 1, 0, 0, 0);
        
            Dx()->AddTransition(Dx()->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            Dx()->ApplyTransitions(cmdList);
        });
        
        Dx()->FlushCommand();
        Dx()->IncreaseFence();
        Dx()->WaitForFence();
        Dx()->PresentSwapChain();
    }
}
