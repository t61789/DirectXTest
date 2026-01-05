#pragma once
#include "i_render_pass.h"
#include "common/const.h"

namespace dt
{
    class Cbuffer;
    class RenderTarget;
    class Material;
    class RenderTexture;

    class MainLightShadowPass : public IRenderPass
    {
    public:
        MainLightShadowPass();
        
        const char* GetName() override { return "Main Light Shadow Pass"; }
        void Execute() override;
        void PrepareContext(RenderResources* context) override;

    private:
        sp<RenderTexture> m_shadowmapRt;
        sp<RenderTarget> m_shadowmapRenderTarget;
        sp<Material> m_drawShadowMtl;
        sp<Cbuffer> m_shadowViewCbuffer;
    };
}
