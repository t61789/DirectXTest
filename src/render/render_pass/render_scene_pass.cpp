#include "render_scene_pass.h"

#include "common/mesh.h"
#include "objects/scene.h"
#include "render/dx_helper.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"
#include "render/batch_rendering/batch_renderer.h"

namespace dt
{
    void RenderScenePass::ExecuteMainThread()
    {
        BatchRenderer::Ins()->GetCommonRenderGroup()->EncodeCmd();
        
        // RT()->AddCmd([](ID3D12GraphicsCommandList* cmdList)
        // {
        //     ZoneScopedN("Render Scene Pass");
        //
        //     // DxHelper::RenderScene(
        //     //     cmdList,
        //     //     RenderRes()->renderObjects,
        //     //     RenderRes()->mainCameraViewCbuffer,
        //     //     RenderRes()->gBufferRenderTarget,
        //     //     nullptr);
        // });
    }

    func<void(ID3D12GraphicsCommandList*)> RenderScenePass::ExecuteRenderThread()
    {
        return BatchRenderer::Ins()->GetCommonRenderGroup()->CreateCmd(RenderRes()->mainCameraViewCbuffer, RenderRes()->gBufferRenderTarget);
    }
}
