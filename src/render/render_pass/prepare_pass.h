#pragma once
#include "i_render_pass.h"
#include "common/const.h"
#include "common/event.h"

namespace dt
{
    class Cbuffer;

    class PreparePass final : public IRenderPass
    {
    public:
        PreparePass();
        
        const char* GetName() override { return "Prepare Pass"; }
        void PrepareContext(RenderResources* context) override;
        void ExecuteMainThread() override;
        func<void(ID3D12GraphicsCommandList*)> ExecuteRenderThread() override;

    private:
        void PrepareLights();
        
        sp<Cbuffer> m_mainCameraViewCbuffer = nullptr;
    };
}
