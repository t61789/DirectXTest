#pragma once
#include "render/directx.h"

namespace dt
{
    class Game
    {
    public:
        Game() = default;

        void Init();
        void Release();
        
        void Update();
        void Render();

    private:
        sp<DirectX> m_directx;
    };
}
