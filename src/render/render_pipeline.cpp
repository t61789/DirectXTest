#include "render_pipeline.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <directx/d3dx12_pipeline_state_stream.h>
#include <directx/d3dx12_barriers.h>

#include "directx.h"
#include "game/game_resource.h"

namespace dt
{
    RenderPipeline::RenderPipeline()
    {
        m_directx = msp<DirectX>();
        m_directx->Init(GR()->GetWindowHwnd());

        m_vertexShader = DirectX::CompileShader("shaders/pbr_basic.shader", "VS_Main", "vs_5_0");
        m_pixelShader = DirectX::CompileShader("shaders/pbr_basic.shader", "PS_Main", "ps_5_0");

        m_layout = vec<D3D12_INPUT_ELEMENT_DESC>
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        CreateRootSignature();
        CreatePso();
        CreateMesh();
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
            viewport.Width = static_cast<float>(this->m_directx->GetSwapChainDesc().BufferDesc.Width);
            viewport.Height = static_cast<float>(this->m_directx->GetSwapChainDesc().BufferDesc.Height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            
            cmdList->RSSetViewports(1, &viewport);

            auto backBufferTexHandle = this->m_directx->GetBackBufferHandle();
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_directx->GetBackBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmdList->ResourceBarrier(1, &barrier);

            cmdList->OMSetRenderTargets(1, &backBufferTexHandle, false, nullptr);

            cmdList->SetPipelineState(m_pso.Get());

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            cmdList->IASetIndexBuffer(&m_indexBufferView);

            cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

            cmdList->DrawInstanced(3, 1, 0, 0);

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
        auto mesh = vec<::DirectX::XMFLOAT3>{
            { 0.0f, 0.0f, 0.0f },
            {  0.0f, 1.0f, 0.0f },
            {  1.0f,  0.0f, 0.0f }
        };

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mesh.size() * sizeof(::DirectX::XMFLOAT3));

        THROW_IF_FAILED(m_directx->GetDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_ID3D12Resource,
            &m_vertexBuffer));

        void* vertexDataBegin = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        THROW_IF_FAILED(m_vertexBuffer->Map(0, &readRange, &vertexDataBegin));
        std::memcpy(vertexDataBegin, mesh.data(), mesh.size() * sizeof(::DirectX::XMFLOAT3));
        m_vertexBuffer->Unmap(0, nullptr);

        m_vertexBufferView.StrideInBytes = sizeof(::DirectX::XMFLOAT3);
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = mesh.size() * sizeof(::DirectX::XMFLOAT3);

        auto indexData = vec<uint16_t>{ 0, 1, 2 };

        CD3DX12_HEAP_PROPERTIES heapProperties2(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc2 = CD3DX12_RESOURCE_DESC::Buffer(indexData.size() * sizeof(uint16_t));

        THROW_IF_FAILED(m_directx->GetDevice()->CreateCommittedResource(
            &heapProperties2,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc2,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_ID3D12Resource,
            &m_indexBuffer));

        void* indexDataBegin = nullptr;
        CD3DX12_RANGE readRange2(0, 0);
        THROW_IF_FAILED(m_indexBuffer->Map(0, &readRange2, &indexDataBegin));
        std::memcpy(indexDataBegin, indexData.data(), indexData.size() * sizeof(uint16_t));
        m_indexBuffer->Unmap(0, nullptr);

        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = indexData.size() * sizeof(uint16_t);
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
        psoDesc.VS = { m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize() };
        psoDesc.PS = { m_pixelShader->GetBufferPointer(), m_pixelShader->GetBufferSize() };
        psoDesc.InputLayout = { m_layout.data(), static_cast<UINT>(m_layout.size()) };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
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
