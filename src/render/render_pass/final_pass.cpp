#include "final_pass.h"

#include <imgui_impl_dx12.h>
#include <directx/d3d12.h>

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
        m_blitMaterial = Material::CreateFromShader("shaders/blit.shader", {});
    }

    void FinalPass::ExecuteMainThread()
    {
        m_blitMaterial->GetCbuffer()->Write(MAIN_TEX, RenderRes()->litResultRt->GetTextureIndex());
    }

    func<void(ID3D12GraphicsCommandList*)> FinalPass::ExecuteRenderThread()
    {
        return [this](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Final Pass");
            
            DxHelper::Blit(cmdList, m_blitMaterial.get(), Dx()->GetBackBufferRenderTarget());

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
            
            for (auto& rt : Dx()->GetBackBufferRenderTarget()->GetColorAttachments())
            {
                DxHelper::AddTransition(rt->GetDxTexture()->GetDxResource(), D3D12_RESOURCE_STATE_PRESENT);
            }

            DxHelper::ApplyTransitions(cmdList);
            
            THROW_IF_FAILED(cmdList->Close());
            
            vec<ID3D12CommandList*> cmdLists;
            cmdLists.push_back(cmdList);
            Dx()->GetCommandQueue()->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());

            Dx()->WaitForFence();
            Dx()->PresentSwapChain();

            RT()->ResetCmdList();
        };
    }
}
