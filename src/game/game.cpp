#include "game/game.h"

namespace dt
{
    void Game::Init()
    {
        m_directx = msp<DirectX>();

        m_directx->Init();
    }

    void Game::Release()
    {
        m_directx->Release();

        m_directx.reset();
    }

    void Game::Update()
    {
    }

    void Game::Render()
    {
    }
}
