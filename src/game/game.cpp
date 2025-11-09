#include "game/game.h"

#include "game_resource.h"
#include "render/render_pipeline.h"

namespace dt
{
    void Game::Init(const uint32_t screenWidth, const uint32_t screenHeight)
    {
        QueryPerformanceFrequency(&m_timeFrequency);
        QueryPerformanceCounter(&m_timeCount);
        m_gameStartTimeCount = m_timeCount;

        m_gameResource = msp<GameResource>();
        m_gameResource->m_screenWidth = screenWidth;
        m_gameResource->m_screenHeight = screenHeight;

        m_renderPipeline = msp<RenderPipeline>();
    }

    void Game::Release()
    {
        m_renderPipeline.reset();
        m_gameResource.reset();
    }

    void Game::Update()
    {
        UpdateTime();
    }

    void Game::Render()
    {
        m_renderPipeline->Render();
    }

    void Game::UpdateTime()
    {
        auto timePrev = m_timeCount;
        QueryPerformanceCounter(&m_timeCount);
        
        GR()->m_time = static_cast<double>(m_timeCount.QuadPart - m_gameStartTimeCount.QuadPart) / static_cast<double>(m_timeFrequency.QuadPart);
        GR()->m_deltaTime = static_cast<double>(m_timeCount.QuadPart - timePrev.QuadPart) / static_cast<double>(m_timeFrequency.QuadPart);
        
        m_frameCount++;
        GR()->m_frameCount = m_frameCount;
    }
}
