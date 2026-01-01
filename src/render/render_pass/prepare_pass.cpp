#include "prepare_pass.h"

#include "game/game_resource.h"
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
        RenderRes()->renderObjects = GR()->mainScene->GetRenderTree()->GetRenderObjects();

        Cbuffer::UpdateDirtyCbuffers();

        RT()->AddCmd([](ID3D12GraphicsCommandList* cmdList, RenderThreadContext&)
        {
            ZoneScopedN("Prepare Pass");
            
            DxHelper::PrepareCmdList(cmdList);
        });
    }
}
