#include "final_pass.h"

#include "common/render_texture.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/render_thread.h"

namespace dt
{
    void FinalPass::Execute()
    {
        RT()->AddCmd([](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Final Pass");
            
            DxHelper::Blit(cmdList, nullptr, Dx()->GetBackBufferRenderTarget());
            
            for (auto& rt : Dx()->GetBackBufferRenderTarget()->GetColorAttachments())
            {
                DxHelper::AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_PRESENT);
            }

            DxHelper::ApplyTransitions(cmdList);
        });

        RenderThreadMgr::Ins()->ExecuteAllThreads();
    }
}
