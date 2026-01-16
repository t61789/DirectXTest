#pragma once
#include "render/directx.h"

namespace dt
{
    class BatchRenderer;
    class Scene;
    class RenderPipeline;
    class GameResource;

    class Game
    {
    public:
        Game(uint32_t screenWidth, uint32_t screenHeight);
        ~Game();
        Game(const Game& other) = delete;
        Game(Game&& other) noexcept = delete;
        Game& operator=(const Game& other) = delete;
        Game& operator=(Game&& other) noexcept = delete;
        
        void Update();
        void Render();

    private:
        void UpdateTime();
        void UpdateComps();

        sp<GameResource> m_gameResource;
        sp<RenderPipeline> m_renderPipeline;
        sp<Scene> m_scene;
        sp<DirectX> m_directx;
        sp<BatchRenderer> m_batchRenderer;

        uint64_t m_frameCount = 0;
        LARGE_INTEGER m_timeCount;
        LARGE_INTEGER m_gameStartTimeCount;
        LARGE_INTEGER m_timeFrequency;
    };
}
