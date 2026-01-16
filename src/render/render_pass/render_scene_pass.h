#pragma once
#include "i_render_pass.h"
#include "common/const.h"

namespace dt
{
    class RenderScenePass : public IRenderPass
    {
    public:
        const char* GetName() override { return "Render Scene Pass"; }
        void ExecuteMainThread() override;
        func<void(ID3D12GraphicsCommandList*)> ExecuteRenderThread() override;
    };
}
