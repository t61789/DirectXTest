#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include <wrl/client.h>

#include "common/const.h"
#include "common/utils.h"

namespace dt
{
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

        void Render();
        static void RenderScene(ID3D12GraphicsCommandList* cmdList, const Scene* scene);

    private:
        sp<PreparePass> m_preparePass;
        sp<FinalPass> m_finalPass;
        sp<RenderResources> m_renderResources;
    };

    static RenderResources* RenderRes()
    {
        return RenderPipeline::Ins()->GetRenderResources();
    }
}
