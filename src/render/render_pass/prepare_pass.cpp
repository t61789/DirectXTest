#include "prepare_pass.h"

#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/render_thread.h"

namespace dt
{
    void PreparePass::Execute()
    {
        RT()->AddCmd([this](ID3D12GraphicsCommandList* cmdList)
        {
            DxHelper::PrepareRendering(cmdList);
            DxHelper::SetViewport(cmdList, Dx()->GetSwapChainDesc().Width, Dx()->GetSwapChainDesc().Height);

            DxHelper::AddTransition(Dx()->GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            DxHelper::ApplyTransitions(cmdList);
        });
    }
}
