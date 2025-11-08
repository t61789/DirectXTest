#pragma once

#include <window.h>

#include "common/utils.h"

namespace dt
{
    class GameResource : public Singleton<GameResource>
    {
    public:
        uint64_t GetFrameCount() const { return m_frameCount; }
        double GetTime() const { return m_time; }
        double GetDeltaTime() const { return m_deltaTime; }
        void GetScreenSize(uint32_t& width, uint32_t& height) const { width = m_screenWidth; height = m_screenHeight; }
        HWND GetWindowHwnd() const { return m_windowHwnd; }

    private:
        uint64_t m_frameCount;
        double m_time;
        double m_deltaTime;

        uint32_t m_screenWidth;
        uint32_t m_screenHeight;

        HWND m_windowHwnd;

        friend class Game;
    };

    static GameResource* GR() { return GameResource::Ins(); }
}
