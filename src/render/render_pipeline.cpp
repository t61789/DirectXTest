#include "render_pipeline.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <directx/d3dx12_pipeline_state_stream.h>
#include <directx/d3dx12_barriers.h>

#include "directx.h"
#include "window.h"
#include "common/mesh.h"
#include "common/shader.h"
#include "game/game_resource.h"

namespace dt
{
    RenderPipeline::RenderPipeline()
    {
        m_directx = msp<DirectX>();
        m_directx->Init(Window::Ins()->GetHandle());

        m_testShader = Shader::LoadFromFile("shaders/pbr_basic.shader");

        CreateMesh();
        CreateRootSignature();
        CreatePso();
    }

    RenderPipeline::~RenderPipeline()
    {
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

            cmdList->SetPipelineState(m_pso.Get());

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

    void RenderPipeline::CreateMesh()
    {
        m_testMesh = Mesh::LoadFromFile("meshes/quad.obj");
    }

    void RenderPipeline::CreateRootSignature()
    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 0;
        rootSignatureDesc.pParameters = nullptr;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;

        if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob)))
        {
            if (errorBlob)
            {
                log_error("Serialize root signature error: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }

            throw std::runtime_error("Failed to serialize root signature.");
        }

        THROW_IF_FAILED(m_directx->GetDevice()->CreateRootSignature(
            0,
            rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(),
            IID_ID3D12RootSignature,
            &m_rootSignature));
    }

    void RenderPipeline::CreatePso()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { m_testShader->GetVSPointer(), m_testShader->GetVSSize() };
        psoDesc.PS = { m_testShader->GetPSPointer(), m_testShader->GetPSSize() };
        psoDesc.InputLayout = { m_testMesh->GetInputLayout().data(), static_cast<UINT>(m_testMesh->GetInputLayout().size()) };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_directx->GetSwapChainDesc().BufferDesc.Format;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;

        THROW_IF_FAILED(m_directx->GetDevice()->CreateGraphicsPipelineState(
            &psoDesc,
            IID_ID3D12PipelineState,
            &m_pso));
    }
}
