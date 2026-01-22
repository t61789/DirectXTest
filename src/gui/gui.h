#pragma once
#include <DirectXMath.h>
#include <imgui.h>

#include "common/math.h"
#include "common/event.h"
#include "render/descriptor_pool.h"

struct ImGuiContext;

namespace dt
{
    class IGui
    {
    public:
        IGui() = default;
        virtual ~IGui() = default;
        virtual void OnGui() = 0;
    };
    
    class Gui : public Singleton<Gui>
    {
    public:
        Gui();
        ~Gui();

        ImGuiContext* GetMainThreadContext() { return m_mainThreadContext; }

        void Render();
        
        static bool SliderFloat3(crstr label, XMFLOAT3& val, float vMin, float vMax, crstr format = "%.2f");
        static bool InputFloat3(crstr label, XMFLOAT3& val, crstr format = "%.2f");
        static bool DragFloat3(crstr label, XMFLOAT3& val, float speed = 1.0f, crstr format = "%.2f");
        
        static void DrawLine(
            const XMFLOAT3& position0WS,
            const XMFLOAT3& position1WS,
            const XMFLOAT4X4& vpMatrix,
            ImU32 color = IM_COL32(255, 255, 255, 255),
            float thickness = 1.0f);

        static void DrawAabb(
            const XMFLOAT3& centerWS,
            const XMFLOAT3& extentsWS,
            const XMFLOAT4X4& vpMatrix,
            ImU32 color = IM_COL32(255, 255, 255, 255),
            float thickness = 1.0f);

        static void DrawFrustum(
            const XMFLOAT4X4& targetVP,
            const XMFLOAT4X4& drawVP,
            ImU32 color = IM_COL32(255, 255, 255, 255),
            float thickness = 1.0f);

        Event<> drawGuiEvent;

    private:
        sp<SrvPool::Handle> m_imguiSrvHandle;
        ImGuiContext* m_mainThreadContext;
    };
}
