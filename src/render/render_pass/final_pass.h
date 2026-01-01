#pragma once
#include "i_render_pass.h"

namespace dt
{
    class FinalPass final : public IRenderPass
    {
    public:
        const char* GetName() override { return "Prepare Pass"; }
        
        void Execute() override;
    };
}
