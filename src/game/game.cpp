#include "game/game.h"

#include "game_resource.h"

namespace dt
{
    void Game::Init(const HWND windowHwnd)
    {
        QueryPerformanceFrequency(&m_timeFrequency);
        QueryPerformanceCounter(&m_timeCount);
        m_gameStartTimeCount = m_timeCount;

        m_gameResource = msp<GameResource>();
        m_directx = msp<DirectX>();
        m_directx->Init(windowHwnd);
    }

    void Game::Release()
    {
        m_directx->Release();
        m_directx.reset();
        m_gameResource.reset();
    }

    void Game::Update()
    {
        UpdateTime();
    }

    void Game::Render()
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

            cmdList->OMSetRenderTargets(1, &backBufferTexHandle, false, nullptr);
        });
        
        m_directx->FlushCommand();
        m_directx->IncreaseFence();
        m_directx->WaitForFence();
        m_directx->PresentSwapChain();
    }

    void Game::UpdateTime()
    {
        auto timePrev = m_timeCount;
        QueryPerformanceCounter(&m_timeCount);
        
        GetGR()->m_time = static_cast<double>(m_timeCount.QuadPart - m_gameStartTimeCount.QuadPart) / static_cast<double>(m_timeFrequency.QuadPart);
        GetGR()->m_deltaTime = static_cast<double>(m_timeCount.QuadPart - timePrev.QuadPart) / static_cast<double>(m_timeFrequency.QuadPart);
        
        m_frameCount++;
        GetGR()->m_frameCount = m_frameCount;
    }
}
