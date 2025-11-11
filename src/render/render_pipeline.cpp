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
        m_directx = msp<DirectX>();
        m_directx->Init(Window::Ins()->GetHandle());

        m_testMesh = Mesh::LoadFromFile("meshes/quad.obj");
        m_testMaterial = Material::LoadFromFile("materials/test_mat.json");
    }

    RenderPipeline::~RenderPipeline()
    {
        m_testMaterial.reset();
        m_testMesh.reset();
        
        m_directx->Release();
        m_directx.reset();
    }

    void RenderPipeline::Render()
    {
        m_directx->AddCommand([this](ID3D12GraphicsCommandList* cmdList)
        {
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = static_cast<float>(m_directx->GetSwapChainDesc().BufferDesc.Width);
            viewport.Height = static_cast<float>(m_directx->GetSwapChainDesc().BufferDesc.Height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            
            cmdList->RSSetViewports(1, &viewport);

            auto backBufferTexHandle = m_directx->GetBackBufferHandle();
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_directx->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmdList->ResourceBarrier(1, &barrier);

            cmdList->OMSetRenderTargets(1, &backBufferTexHandle, false, nullptr);

            cmdList->SetGraphicsRootSignature(m_testMaterial->GetShader()->GetRootSignature().Get());

            m_testMaterial->ApplyRenderState(m_testMaterial->GetShader()->GetPso());
            cmdList->SetPipelineState(m_testMaterial->GetShader()->GetPso()->CurPso().Get());

            cmdList->SetGraphicsRootConstantBufferView(0, m_testMaterial->GetCbuffer()->GetDxResource()->GetGPUVirtualAddress());

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            cmdList->IASetIndexBuffer(&m_testMesh->GetIndexBufferView());

            cmdList->IASetVertexBuffers(0, 1, &m_testMesh->GetVertexBufferView());

            cmdList->DrawIndexedInstanced(m_testMesh->GetIndicesCount(), 1, 0, 0, 0);

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_directx->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            cmdList->ResourceBarrier(1, &barrier);
        });
        
        m_directx->FlushCommand();
        m_directx->IncreaseFence();
        m_directx->WaitForFence();
        m_directx->PresentSwapChain();
    }
}
