#include "gui.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "window.h"

namespace dt
{
    Gui::Gui()
    {
        m_mainThreadContext = ImGui::CreateContext();
        m_renderThreadContext = ImGui::CreateContext();
        
        ImGui::SetCurrentContext(m_mainThreadContext);
        
        ImGui_ImplWin32_Init(Window::Ins()->GetHandle());
        
        m_imguiSrvHandle = DescriptorPool::Ins()->AllocEmptySrvHandle();
        
        ImGui_ImplDX12_Init(
            DirectX::Ins()->GetDevice().Get(),
            DirectX::Ins()->GetSwapChainDesc().BufferCount,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DescriptorPool::Ins()->GetSrvDescHeap(),
            m_imguiSrvHandle->data.cpuHandle,
            m_imguiSrvHandle->data.gpuHandle);
        
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        // 强制生成像素数据
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }
}
