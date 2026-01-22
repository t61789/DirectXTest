#include "gui.h"

#include <algorithm>
#include <array>
#include <cmath>
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

        float EvalPlane(const ClipPlane& plane, const XMFLOAT4& p)
        {
            return plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d * p.w;
        }

        XMFLOAT4 LerpClip(const XMFLOAT4& a, const XMFLOAT4& b, const float t)
        {
            return {
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            };
        }

        XMFLOAT4 WSPosToClip(const XMFLOAT3& positionWS, const XMFLOAT4X4& vpMatrix)
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
        bool ClipLineSegmentToFrustumD3D(XMFLOAT4& p0, XMFLOAT4& p1)
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
                        t0 = (std::max)(t, t0);
                    }
                    else
                    {
                        // Leaving the volume.
                        t1 = (std::min)(t, t1);
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

        bool ClipToPixel(const XMFLOAT4& clipPos, const XMUINT2& screenSize, ImVec2& outPixel)
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
    
    void Gui::DrawLine(
        const XMFLOAT3& position0WS,
        const XMFLOAT3& position1WS,
        const XMFLOAT4X4& vpMatrix,
        const ImU32 color,
        const float thickness)
    {
        const XMUINT2& screenSize = RenderRes()->screenSize;

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
            DrawLine(vertices[s], vertices[e], vpMatrix, color, thickness);
        }
    }

    void Gui::DrawFrustum(
        const XMFLOAT4X4& targetVP,
        const XMFLOAT4X4& drawVP,
        const ImU32 color,
        const float thickness)
    {
        const XMMATRIX vp = Load(targetVP);
        XMVECTOR determinant;
        const XMMATRIX invVP = XMMatrixInverse(&determinant, vp);

        const float det = XMVectorGetX(determinant);
        if (std::abs(det) <= EPSILON)
        {
            return;
        }

        auto unProjectNdcToWorld = [&invVP](const float ndcX, const float ndcY, const float ndcZ) -> XMFLOAT3
        {
            const XMVECTOR clip = XMVectorSet(ndcX, ndcY, ndcZ, 1.0f);
            XMVECTOR worldH = XMVector4Transform(clip, invVP);

            const float w = XMVectorGetW(worldH);
            if (std::abs(w) <= EPSILON)
            {
                return {0.0f, 0.0f, 0.0f};
            }

            worldH = XMVectorScale(worldH, 1.0f / w);
            XMFLOAT3 world;
            XMStoreFloat3(&world, worldH);
            return world;
        };

        // D3D NDC: x,y in [-1,1], z in [0,1]
        const std::array<XMFLOAT3, 8> cornersWS = {
            unProjectNdcToWorld(-1.0f, -1.0f, 0.0f), // near left  bottom
            unProjectNdcToWorld(-1.0f,  1.0f, 0.0f), // near left  top
            unProjectNdcToWorld( 1.0f,  1.0f, 0.0f), // near right top
            unProjectNdcToWorld( 1.0f, -1.0f, 0.0f), // near right bottom
            unProjectNdcToWorld(-1.0f, -1.0f, 1.0f), // far  left  bottom
            unProjectNdcToWorld(-1.0f,  1.0f, 1.0f), // far  left  top
            unProjectNdcToWorld( 1.0f,  1.0f, 1.0f), // far  right top
            unProjectNdcToWorld( 1.0f, -1.0f, 1.0f), // far  right bottom
        };

        static constexpr std::pair<int, int> EDGES[12] = {
            {0,1}, {1,2}, {2,3}, {3,0}, // near plane
            {4,5}, {5,6}, {6,7}, {7,4}, // far plane
            {0,4}, {1,5}, {2,6}, {3,7}, // sides
        };

        for (const auto& [s, e] : EDGES)
        {
            DrawLine(cornersWS[s], cornersWS[e], drawVP, color, thickness);
        }
    }
}
