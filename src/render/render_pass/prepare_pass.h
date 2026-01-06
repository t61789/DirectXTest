#pragma once
#include "i_render_pass.h"
#include "common/const.h"

namespace dt
{
    class Cbuffer;

    class PreparePass final : public IRenderPass
    {
    public:
        PreparePass();
        
        const char* GetName() override { return "Prepare Pass"; }
        void Execute() override;
        void PrepareLights();

    private:
        sp<Cbuffer> m_mainCameraViewCbuffer = nullptr;
    };
}
