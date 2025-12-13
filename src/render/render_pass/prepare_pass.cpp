#include "prepare_pass.h"

#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/render_thread.h"

namespace dt
{
    void PreparePass::Execute()
    {
        RT()->AddCmd([](ID3D12GraphicsCommandList* cmdList)
        {
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            DxHelper::SetViewport(cmdList, Dx()->GetSwapChainDesc().Width, Dx()->GetSwapChainDesc().Height);

            DxHelper::AddTransition(Dx()->GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            DxHelper::ApplyTransitions(cmdList);

            ID3D12DescriptorHeap* heaps[] = {
                SrvDescPool::Ins()->GetHeap(),
                SamplerDescPool::Ins()->GetHeap()
            };
            DxHelper::SetHeaps(cmdList, heaps, 2);
            
            auto backBufferTexHandle = Dx()->GetBackBufferHandle();
            cmdList->OMSetRenderTargets(1, &backBufferTexHandle, false, nullptr);
        });
    }
}
