#pragma once
#include "i_render_pass.h"

namespace dt
{
    class RenderScenePass : public IRenderPass
    {
    public:
        const char* GetName() override { return "Render Scene Pass"; }
        void Execute() override;
    };
}
