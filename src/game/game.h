#pragma once
#include "render/directx.h"

namespace dt
{
    class RenderPipeline;
    class GameResource;

    class Game
    {
    public:
        Game() = default;

        void Init(uint32_t screenWidth, uint32_t screenHeight);
        void Release();
        
        void Update();
        void Render();

    private:
        void UpdateTime();
        
        sp<GameResource> m_gameResource;
        sp<RenderPipeline> m_renderPipeline;

        uint64_t m_frameCount = 0;
        LARGE_INTEGER m_timeCount;
        LARGE_INTEGER m_gameStartTimeCount;
        LARGE_INTEGER m_timeFrequency;
    };
}
