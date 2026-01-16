#pragma once
#include "i_render_pass.h"
#include "common/const.h"

namespace dt
{
    class RenderTarget;
    struct RenderResources;
    class RenderTexture;
    class Material;

    class DeferredLitPass final : public IRenderPass
    {
    public:
        DeferredLitPass();
        
        const char* GetName() override { return "Deferred Lit Pass"; }
        
        void PrepareContext(RenderResources* context) override;
        func<void(ID3D12GraphicsCommandList*)> ExecuteRenderThread() override;

    private:
        sp<Material> m_litMaterial;
        sp<RenderTexture> m_litResultRt;
        sp<RenderTarget> m_litResultRenderTarget;
        
        sp<RenderTexture> m_gBufferRt0;
        sp<RenderTexture> m_gBufferRt1;
        sp<RenderTexture> m_gBufferRt2;
        sp<RenderTexture> m_gBufferRtDepth;
        sp<RenderTarget> m_gBufferRenderTarget;
    };
}
