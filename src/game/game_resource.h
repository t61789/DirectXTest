#pragma once
#include "common/utils.h"

namespace dt
{
    class GameResource : public Singleton<GameResource>
    {
    public:
        uint64_t GetFrameCount() const { return m_frameCount; }
        double GetTime() const { return m_time; }
        double GetDeltaTime() const { return m_deltaTime; }

    private:
        uint64_t m_frameCount;
        double m_time;
        double m_deltaTime;

        friend class Game;
    };

    static GameResource* GetGR() { return GameResource::Ins(); }
}
