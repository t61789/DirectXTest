#include "final_pass.h"

#include <imgui_impl_dx12.h>

#include "common/render_texture.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/cbuffer.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/render_context.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"

namespace dt
{
    FinalPass::FinalPass()
    {
        m_blitMaterial = Material::CreateFromShader("shaders/blit.shader");
    }

    void FinalPass::Execute()
    {
        RT()->AddCmd([this, imguiDrawData=ImGui::GetDrawData()](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Final Pass");
            
            m_blitMaterial->GetCbuffer()->Write(MAIN_TEX, RenderRes()->litResultRt->GetTextureIndex());
            DxHelper::Blit(cmdList, m_blitMaterial.get(), Dx()->GetBackBufferRenderTarget());

            ImGui_ImplDX12_RenderDrawData(imguiDrawData, cmdList);
            
            for (auto& rt : Dx()->GetBackBufferRenderTarget()->GetColorAttachments())
            {
                DxHelper::AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_PRESENT);
            }

            DxHelper::ApplyTransitions(cmdList);
        });

        RenderThreadMgr::Ins()->ExecuteAllThreads();
    }
}
