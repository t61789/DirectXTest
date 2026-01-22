#include "game/game.h"

#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <tracy/Tracy.hpp>

#include "game_resource.h"
#include "objects/scene.h"
#include "render/render_pipeline.h"
#include "render/batch_rendering/batch_renderer.h"

namespace dt
{
    Game::Game(const uint32_t screenWidth, const uint32_t screenHeight)
    {
        QueryPerformanceFrequency(&m_timeFrequency);
        QueryPerformanceCounter(&m_timeCount);
        m_gameStartTimeCount = m_timeCount;

        m_directx = msp<DirectX>();

        m_gameResource = msp<GameResource>();
        m_gameResource->m_screenWidth = screenWidth;
        m_gameResource->m_screenHeight = screenHeight;
        
        m_batchRenderer = msp<BatchRenderer>();

        m_renderPipeline = msp<RenderPipeline>();

        m_scene = Scene::LoadScene("scenes/test_scene/test_scene.json");
        // m_scene = Scene::LoadScene("scenes/Scene_A/scene.json");
    }

    Game::~Game()
    {
        m_scene.reset();
        m_renderPipeline.reset();
        m_batchRenderer.reset();
        m_gameResource.reset();

        m_directx.reset();
    }

    void Game::Update()
    {
        ZoneScoped;
        
        UpdateTime();
        UpdateComps();
    }

    void Game::Render()
    {
        ZoneScoped;
        
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

    void Game::UpdateComps()
    {
        auto allComps = GR()->mainScene->GetRegistry()->GetCompStorage()->GetAllComps();
        for (auto& comp : allComps)
        {
            if (comp.expired())
            {
                continue;
            }
            
            comp.lock()->CallUpdate();
        }

        for (auto& comp : allComps)
        {
            if (comp.expired())
            {
                continue;
            }
            
            comp.lock()->LateUpdate();
        }
    }
}
