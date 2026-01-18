#include "gui.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "window.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"

namespace dt
{
    Gui::Gui()
    {
        m_mainThreadContext = ImGui::CreateContext();
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
        
        io.Fonts->Clear();
        // 加载中文字体（确保你的项目中有相应的字体文件）
        io.Fonts->AddFontFromFileTTF(Utils::ToAbsPath("others/msyh.ttc").c_str(), 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
        // 强制生成像素数据
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        // 重新创建字体纹理
        ImGui_ImplDX12_CreateDeviceObjects();
    }

    Gui::~Gui()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(m_mainThreadContext);
    }

    void Gui::Render()
    {
        ImGui_ImplWin32_NewFrame();
        ImGui_ImplDX12_NewFrame();

        RECT rect;
        GetClientRect(Window::Ins()->GetHandle(), &rect); // 获取的是经过 DPI 适配后的真实像素宽度
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
        
        ImGui::NewFrame();

        // todo call gui events
        ImGui::Begin("Shit");
        ImGui::Text("FPS: %f", 1.0f / GR()->GetDeltaTime());
        drawGuiEvent.Invoke();
        ImGui::End();
        
        ImGui::Render();
    }
    
    // void Gui::DrawCoordinateDirLine()
    // {
    //     DrawLine(Vec3::Zero(), Vec3::Right(), IM_COL32(255, 60, 60, 255), 7.0f);
    //     DrawLine(Vec3::Zero(), Vec3::Forward(), IM_COL32(60, 60, 255, 255), 7.0f);
    //     DrawLine(Vec3::Zero(), Vec3::Up(), IM_COL32(60, 255, 60, 255), 7.0f);
    // }
    //
    // void Gui::DrawBounds(const RenderContext* renderContext)
    // {
    //     if (!renderContext)
    //     {
    //         return;
    //     }
    //     
    //     for (auto const& renderComp : renderContext->visibleRenderObjs)
    //     {
    //         DrawCube(renderComp->GetWorldBounds(), ImColor(155, 155, 155, 255));
    //     }
    //     
    //     for (auto const& renderComp : GetGR()->GetMainScene()->GetIndices()->GetCompStorage()->GetComps<BatchRenderComp>())
    //     {
    //         DrawCube(renderComp.lock()->GetWorldBounds(), ImColor(155, 0, 0, 255));
    //     }
    // }
    //
    // void Gui::DrawCube(const Bounds& bounds, const ImU32 color, const float thickness)
    // {
    //     // 计算八个顶点
    //     std::array<Vec3, 8> vertices =
    //     {
    //         bounds.center + Vec3(-bounds.extents.x, -bounds.extents.y, -bounds.extents.z),
    //         bounds.center + Vec3( bounds.extents.x, -bounds.extents.y, -bounds.extents.z),
    //         bounds.center + Vec3( bounds.extents.x,  bounds.extents.y, -bounds.extents.z),
    //         bounds.center + Vec3(-bounds.extents.x,  bounds.extents.y, -bounds.extents.z),
    //         bounds.center + Vec3(-bounds.extents.x, -bounds.extents.y,  bounds.extents.z),
    //         bounds.center + Vec3( bounds.extents.x, -bounds.extents.y,  bounds.extents.z),
    //         bounds.center + Vec3( bounds.extents.x,  bounds.extents.y,  bounds.extents.z),
    //         bounds.center + Vec3(-bounds.extents.x,  bounds.extents.y,  bounds.extents.z)
    //     };
    //
    //     // 定义12条边 [11]()
    //     const std::array<std::pair<int, int>, 12> edges =
    //     {{
    //         {0,1}, {1,2}, {2,3}, {3,0}, // 底面 
    //         {4,5}, {5,6}, {6,7}, {7,4}, // 顶面
    //         {0,4}, {1,5}, {2,6}, {3,7}  // 侧面连接 
    //     }};
    //
    //     // 绘制所有边
    //     for (const auto& [start, end] : edges)
    //     {
    //         DrawLine(vertices[start], vertices[end], color, thickness);
    //     }
    // }
    //
    // void Gui::DrawFrustumPlanes(cr<Matrix4x4> vpMatrix, const ImU32 color, const float thickness)
    // {
    //     auto ivp = vpMatrix.Inverse();
    //
    //     auto toWorld = [&ivp](cr<Vec3> positionSS)
    //     {
    //         auto p = positionSS;
    //         p.x = p.x * 2.0f - 1.0f;
    //         p.y = p.y * 2.0f - 1.0f;
    //         auto result = ivp * Vec4(p, 1.0f);
    //         return Vec3(result.x, result.y, result.z) / result.w;
    //     };
    //
    //     auto flb = toWorld(Vec3(0.0f, 0.0f, 1.0f));
    //     auto flt = toWorld(Vec3(0.0f, 1.0f, 1.0f));
    //     auto frb = toWorld(Vec3(1.0f, 0.0f, 1.0f));
    //     auto frt = toWorld(Vec3(1.0f, 1.0f, 1.0f));
    //     auto nlb = toWorld(Vec3(0.0f, 0.0f, 0.0f));
    //     auto nlt = toWorld(Vec3(0.0f, 1.0f, 0.0f));
    //     auto nrb = toWorld(Vec3(1.0f, 0.0f, 0.0f));
    //     auto nrt = toWorld(Vec3(1.0f, 1.0f, 0.0f));
    //
    //     DrawLine(flb, flt, color, thickness);
    //     DrawLine(flt, frt, color, thickness);
    //     DrawLine(frt, frb, color, thickness);
    //     DrawLine(frb, flb, color, thickness);
    //     
    //     DrawLine(nlb, nlt, color, thickness);
    //     DrawLine(nlt, nrt, color, thickness);
    //     DrawLine(nrt, nrb, color, thickness);
    //     DrawLine(nrb, nlb, color, thickness);
    //
    //     DrawLine(flb, nlb, color, thickness);
    //     DrawLine(flt, nlt, color, thickness);
    //     DrawLine(frt, nrt, color, thickness);
    //     DrawLine(frb, nrb, color, thickness);
    // }
    //
    // void Gui::DrawFrustumPlanes(const Vec3* corners, const ImU32 color, const float thickness)
    // {
    //     DrawLine(corners[0], corners[1], color, thickness);
    //     DrawLine(corners[1], corners[2], color, thickness);
    //     DrawLine(corners[2], corners[3], color, thickness);
    //     DrawLine(corners[3], corners[0], color, thickness);
    //     
    //     DrawLine(corners[4], corners[5], color, thickness);
    //     DrawLine(corners[5], corners[6], color, thickness);
    //     DrawLine(corners[6], corners[7], color, thickness);
    //     DrawLine(corners[7], corners[4], color, thickness);
    //     
    //     DrawLine(corners[0], corners[4], color, thickness);
    //     DrawLine(corners[1], corners[5], color, thickness);
    //     DrawLine(corners[2], corners[6], color, thickness);
    //     DrawLine(corners[3], corners[7], color, thickness);
    // }

    // void Gui::DrawLine(cr<Vec3> start, cr<Vec3> end, const ImU32 color, const float thickness)
    // {
    //     m_drawLineCmds.push_back({
    //         start,
    //         end,
    //         color,
    //         thickness
    //     });
    // }

    bool Gui::SliderFloat3(crstr label, XMFLOAT3& val, const float vMin, const float vMax, crstr format)
    {
        return ImGui::SliderFloat3(label.c_str(), &val.x, vMin, vMax, format.c_str());
    }

    bool Gui::InputFloat3(crstr label, XMFLOAT3& val, crstr format)
    {
        return ImGui::InputFloat3(label.c_str(), &val.x, format.c_str());
    }

    bool Gui::DragFloat3(crstr label, XMFLOAT3& val, const float speed, crstr format)
    {
        return ImGui::DragFloat3(label.c_str(), &val.x, speed, 0, 0, format.c_str());
    }
    
    // void Gui::DoDrawLines()
    // {
    //     auto vpInfo = GetRC()->mainVPInfo;
    //     auto screenSize = Vec2(static_cast<float>(GetRC()->screenWidth), static_cast<float>(GetRC()->screenHeight));
    //     for (auto const& cmd : m_drawLineCmds)
    //     {
    //         ImGuiDrawLine(
    //             cmd.start,
    //             cmd.end,
    //             vpInfo->vMatrix,
    //             vpInfo->pMatrix,
    //             screenSize,
    //             cmd.color,
    //             cmd.thickness);
    //     }
    //     m_drawLineCmds.clear();
    // }
    
    void Gui::ImGuiDrawLine(cr<XMVECTOR> worldStart, cr<XMVECTOR> worldEnd, const ImU32 color, const float thickness)
    {
        auto vpMatrix = Load(RenderRes()->mainCameraVp->vpMatrix);
        auto screenSize = RenderRes()->screenSize;
        
        auto drawList = ImGui::GetBackgroundDrawList();
        
        // 转换世界坐标到屏幕坐标
        auto screenStart = Store3(WorldToScreen01(worldStart, vpMatrix));
        auto screenEnd = Store3(WorldToScreen01(worldEnd, vpMatrix));

        if(screenStart.z < 0 || screenEnd.z < 0)
        {
            return;
        }

        XMFLOAT2 p0 = { screenStart.x, screenStart.y };
        XMFLOAT2 p1 = { screenEnd.x, screenEnd.y };
        const XMFLOAT2 rectMin = { 0, 0 };
        const XMFLOAT2 rectMax = XMFLOAT2(screenSize.x, screenSize.y);

        auto inScreen = CohenSutherlandClip(p0, p1, rectMin, rectMax);
        if(!inScreen)
        {
            return;
        }

        // 使用 ImGui 的绘图 API 绘制线条
        drawList->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), color, thickness);
    }
}
