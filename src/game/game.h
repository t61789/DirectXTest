#pragma once
#include "render/directx.h"

namespace dt
{
    class GameResource;

    class Game
    {
    public:
        Game() = default;

        void Init(HWND windowHwnd);
        void Release();
        
        void Update();
        void Render();

    private:
        void UpdateTime();
        
        sp<DirectX> m_directx;
        sp<GameResource> m_gameResource;

        uint64_t m_frameCount = 0;
        LARGE_INTEGER m_timeCount;
        LARGE_INTEGER m_gameStartTimeCount;
        LARGE_INTEGER m_timeFrequency;
    };
}
