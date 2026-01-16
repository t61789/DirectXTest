#pragma once
#include "i_render_pass.h"
#include "common/const.h"

namespace dt
{
    class Cbuffer;
    class RenderTarget;
    class Material;
    class RenderTexture;

    class MainLightShadowPass final : public IRenderPass
    {
    public:
        MainLightShadowPass();
        
        const char* GetName() override { return "Main Light Shadow Pass"; }
        
        void PrepareContext(RenderResources* context) override;
        void ExecuteMainThread() override;
        func<void(ID3D12GraphicsCommandList*)> ExecuteRenderThread() override;

    private:
        sp<RenderTexture> m_shadowmapRt;
        sp<RenderTarget> m_shadowmapRenderTarget;
        sp<Material> m_drawShadowMtl;
        sp<Cbuffer> m_shadowViewCbuffer;
    };
}
