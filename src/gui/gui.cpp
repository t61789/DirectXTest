#include "gui.h"

#include <array>
#include <utility>

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "window.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"

namespace dt
{
    namespace
    {
        struct ClipPlane
        {
            float a, b, c, d;
        };

        inline float EvalPlane(const ClipPlane& plane, const XMFLOAT4& p)
        {
            return plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d * p.w;
        }

        inline XMFLOAT4 LerpClip(const XMFLOAT4& a, const XMFLOAT4& b, const float t)
        {
            return XMFLOAT4(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t);
        }

        inline XMFLOAT4 WSPosToClip(const XMFLOAT3& positionWS, const XMFLOAT4X4& vpMatrix)
        {
            auto vp = Load(vpMatrix);
            XMVECTOR ws = XMVectorSet(positionWS.x, positionWS.y, positionWS.z, 1.0f);
            XMFLOAT4 clip;
            XMStoreFloat4(&clip, XMVector4Transform(ws, vp));
            return clip;
        }

        // Clip a clip-space segment against D3D clip volume:
        //  -w <= x <= w
        //  -w <= y <= w
        //   0 <= z <= w
        inline bool ClipLineSegmentToFrustumD3D(XMFLOAT4& p0, XMFLOAT4& p1)
        {
            // Planes are expressed as: f(p) = a*x + b*y + c*z + d*w >= 0
            static constexpr ClipPlane planes[6] = {
                {  1.0f,  0.0f,  0.0f, 1.0f }, // left:   x + w >= 0
                { -1.0f,  0.0f,  0.0f, 1.0f }, // right: -x + w >= 0
                {  0.0f,  1.0f,  0.0f, 1.0f }, // bottom: y + w >= 0
                {  0.0f, -1.0f,  0.0f, 1.0f }, // top:   -y + w >= 0
                {  0.0f,  0.0f,  1.0f, 0.0f }, // near:   z >= 0
                {  0.0f,  0.0f, -1.0f, 1.0f }, // far:   -z + w >= 0
            };

            constexpr float kEps = 1e-6f;

            const XMFLOAT4 a = p0;
            const XMFLOAT4 b = p1;
            float t0 = 0.0f;
            float t1 = 1.0f;

            for (const auto& plane : planes)
            {
                const float f0 = EvalPlane(plane, a);
                const float f1 = EvalPlane(plane, b);

                if (f0 < 0.0f && f1 < 0.0f)
                {
                    return false;
                }

                if (f0 < 0.0f || f1 < 0.0f)
                {
                    const float denom = f0 - f1;
                    if (denom > -kEps && denom < kEps)
                    {
                        // Segment is (almost) parallel to this plane; keep previous interval.
                        continue;
                    }

                    const float t = f0 / denom; // intersection parameter along original segment [0,1]
                    if (f0 < 0.0f)
                    {
                        // Entering the volume.
                        if (t > t0) t0 = t;
                    }
                    else
                    {
                        // Leaving the volume.
                        if (t < t1) t1 = t;
                    }

                    if (t0 > t1)
                    {
                        return false;
                    }
                }
            }

            p0 = LerpClip(a, b, t0);
            p1 = LerpClip(a, b, t1);
            return true;
        }

        inline bool ClipToPixel(const XMFLOAT4& clipPos, const XMUINT2& screenSize, ImVec2& outPixel)
        {
            constexpr float kEps = 1e-6f;
            if (screenSize.x == 0 || screenSize.y == 0)
            {
                return false;
            }

            if (clipPos.w > -kEps && clipPos.w < kEps)
            {
                return false;
            }

            const float invW = 1.0f / clipPos.w;
            const float ndcX = clipPos.x * invW;
            const float ndcY = clipPos.y * invW;

            const float sx = (ndcX * 0.5f + 0.5f) * static_cast<float>(screenSize.x);
            const float sy = (0.5f - ndcY * 0.5f) * static_cast<float>(screenSize.y);

            outPixel = ImVec2(sx, sy);
            return true;
        }
    }

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

    void Gui::DrawLine(
        const XMFLOAT3& position0WS,
        const XMFLOAT3& position1WS,
        const XMFLOAT4X4& vpMatrix,
        const XMUINT2& screenSize,
        const ImU32 color,
        const float thickness)
    {
        XMFLOAT4 clip0 = WSPosToClip(position0WS, vpMatrix);
        XMFLOAT4 clip1 = WSPosToClip(position1WS, vpMatrix);

        if (!ClipLineSegmentToFrustumD3D(clip0, clip1))
        {
            return;
        }

        ImVec2 p0, p1;
        if (!ClipToPixel(clip0, screenSize, p0) || !ClipToPixel(clip1, screenSize, p1))
        {
            return;
        }

        ImGui::GetBackgroundDrawList()->AddLine(p0, p1, color, thickness);
    }

    void Gui::DrawAabb(
        const XMFLOAT3& centerWS,
        const XMFLOAT3& extentsWS,
        const XMFLOAT4X4& vpMatrix,
        const XMUINT2& screenSize,
        const ImU32 color,
        const float thickness)
    {
        const std::array<XMFLOAT3, 8> vertices = {
            XMFLOAT3(centerWS.x - extentsWS.x, centerWS.y - extentsWS.y, centerWS.z - extentsWS.z),
            XMFLOAT3(centerWS.x + extentsWS.x, centerWS.y - extentsWS.y, centerWS.z - extentsWS.z),
            XMFLOAT3(centerWS.x + extentsWS.x, centerWS.y + extentsWS.y, centerWS.z - extentsWS.z),
            XMFLOAT3(centerWS.x - extentsWS.x, centerWS.y + extentsWS.y, centerWS.z - extentsWS.z),
            XMFLOAT3(centerWS.x - extentsWS.x, centerWS.y - extentsWS.y, centerWS.z + extentsWS.z),
            XMFLOAT3(centerWS.x + extentsWS.x, centerWS.y - extentsWS.y, centerWS.z + extentsWS.z),
            XMFLOAT3(centerWS.x + extentsWS.x, centerWS.y + extentsWS.y, centerWS.z + extentsWS.z),
            XMFLOAT3(centerWS.x - extentsWS.x, centerWS.y + extentsWS.y, centerWS.z + extentsWS.z),
        };

        static constexpr std::pair<int, int> edges[12] = {
            {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
            {4,5}, {5,6}, {6,7}, {7,4}, // top face
            {0,4}, {1,5}, {2,6}, {3,7}, // sides
        };

        for (const auto& [s, e] : edges)
        {
            DrawLine(vertices[s], vertices[e], vpMatrix, screenSize, color, thickness);
        }
    }
}
