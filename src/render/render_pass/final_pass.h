#pragma once
#include "i_render_pass.h"
#include "common/const.h"

namespace dt
{
    class Image;
    class Material;

    class FinalPass final : public IRenderPass
    {
    public:
        FinalPass();
        
        const char* GetName() override { return "Prepare Pass"; }

        void ExecuteMainThread() override;
        func<void(ID3D12GraphicsCommandList*)> ExecuteRenderThread() override;

    private:
        sp<Material> m_blitMaterial;
    };
}
