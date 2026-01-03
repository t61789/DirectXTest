#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include <wrl/client.h>

#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    class IRenderPass;
    class RenderScenePass;
    class RenderTexture;
    class RenderTarget;
    class FinalPass;
    class RenderThread;
    struct RenderResources;
    class PreparePass;
    class Scene;
    class Material;
    class Mesh;
    class DirectX;
    class Shader;
    
    using namespace Microsoft::WRL;

    class RenderPipeline : public Singleton<RenderPipeline>
    {
    public:
        RenderPipeline();
        ~RenderPipeline();
        RenderPipeline(const RenderPipeline& other) = delete;
        RenderPipeline(RenderPipeline&& other) noexcept = delete;
        RenderPipeline& operator=(const RenderPipeline& other) = delete;
        RenderPipeline& operator=(RenderPipeline&& other) noexcept = delete;

        RenderResources* GetRenderResources() const { return m_renderResources.get(); }
        crvecsp<IRenderPass> GetPasses() const { return m_passes; }

        void Render();

    private:
        vecsp<IRenderPass> m_passes;
        sp<RenderResources> m_renderResources;
    };

    static RenderResources* RenderRes()
    {
        return RenderPipeline::Ins()->GetRenderResources();
    }
}
