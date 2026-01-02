#include "prepare_pass.h"

#include "window.h"
#include "game/game_resource.h"
#include "objects/camera_comp.h"
#include "objects/scene.h"
#include "render/cbuffer.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"

namespace dt
{
    void PreparePass::Execute()
    {
        RenderThreadMgr::Ins()->WaitForDone();

        *RenderRes() = RenderResources();
        RenderRes()->screenSize = { Window::Ins()->GetWidth(), Window::Ins()->GetHeight() };
        auto aspect = static_cast<float>(RenderRes()->screenSize.x) / static_cast<float>(RenderRes()->screenSize.y);
        RenderRes()->gBufferRenderTarget = RenderPipeline::Ins()->GetGBufferRenderTarget();
        RenderRes()->mainCameraVp = CameraComp::GetMainCamera()->CreateVPMatrix(aspect);
        RenderRes()->renderObjects = GR()->mainScene->GetRenderTree()->GetRenderObjects();

        RT()->AddCmd([](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Prepare Pass");
            
            DxHelper::PrepareCmdList(cmdList);
        });
    }
}
